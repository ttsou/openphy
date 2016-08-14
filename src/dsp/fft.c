/*
 * FFT Backend
 *
 * Copyright (C) 2015 Ettus Research LLC
 * Author Tom Tsou <tom.tsou@ettus.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fftw3.h>

#include "sigvec.h"
#include "sigvec_internal.h"
#include "fft.h"

struct fft_hdl {
	struct cxvec *fft_in;
	struct cxvec *fft_out;
	fftwf_plan fft_plan;
};

/*! \brief Initialize FFT backend 
 *  \param[in] reverse FFT direction
 *  \param[in] m FFT length 
 *  \param[in] istride input stride count
 *  \param[in] ostride output stride count
 *  \param[in] in input buffer (FFTW aligned)
 *  \param[in] out output buffer (FFTW aligned)
 *  \param[in] ooffset initial offset into output buffer
 *
 * If the reverse is non-NULL, then an inverse FFT will be used. This is a
 * wrapper for advanced non-contiguous FFTW usage. See FFTW documentation for
 * further details.
 *
 *   http://www.fftw.org/doc/Advanced-Complex-DFTs.html
 *
 * It is currently unknown how the offset of the output buffer affects FFTW
 * memory alignment.
 */

#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct fft_hdl *init_fft(int reverse, int m, int many,
			 int idist, int odist, int istride, int ostride,
			 struct cxvec *in, struct cxvec *out, int no_align)
{
	int flags = 0, rank = 1;
	int n[] = { m };
	int howmany = many;
	int *inembed = n;
	int *onembed = n;
	fftwf_complex *obuffer = NULL, *ibuffer = NULL;

	struct fft_hdl *hdl = malloc(sizeof *hdl);
	if (!hdl)
		return NULL;

	int direction = FFTW_FORWARD;
	if (reverse)
		direction = FFTW_BACKWARD;

	if (in && out) {
		ibuffer = (fftwf_complex *) in->data;
		obuffer = (fftwf_complex *) out->data;
	}

	if (no_align)
		flags = FFTW_UNALIGNED;

	hdl->fft_in = in;
	hdl->fft_out = out;

	pthread_mutex_lock(&mutex);

	hdl->fft_plan = fftwf_plan_many_dft(rank, n, howmany,
					    ibuffer, inembed, istride, idist,
					    obuffer, onembed, ostride, odist,
					    direction, FFTW_MEASURE | flags);
	pthread_mutex_unlock(&mutex);

	return hdl;
}

void *fft_malloc(size_t size)
{
	return fftwf_malloc(size);
}

void fft_free_buf(void *buf)
{
	fftwf_free(buf);
}

/*! \brief Free FFT backend resources 
 */
void fft_free_hdl(struct fft_hdl *hdl)
{
	fftwf_destroy_plan(hdl->fft_plan);
	free(hdl);
}

/*! \brief Run multiple DFT operations with the initialized plan
 *  \param[in] hdl handle to an intitialized fft struct
 *
 * Input and output buffers are configured with init_fft().
 */
void cxvec_fft(struct fft_hdl *hdl, struct cxvec *in, struct cxvec *out)
{
	if (in && out)
		fftwf_execute_dft(hdl->fft_plan,
				  (fftwf_complex *) in->data,
				  (fftwf_complex *) out->data);
	else
		fftwf_execute(hdl->fft_plan);
}
