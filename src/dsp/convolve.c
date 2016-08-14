/*
 * convolve.c
 *
 * Complex convolution
 *
 * Copyright (C) 2012 Thomas Tsou <ttsou@vt.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * See the COPYING file in the main directory for details.
 */

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "openphy/convolve.h"
#include "openphy/mac.h"

/*! \brief Convolve two complex vectors 
 *  \param[in] in_vec Complex input signal
 *  \param[in] h_vec Complex filter taps (stored in reverse order)
 *  \param[out] out_vec Complex vector output
 *
 * The starting index of the input vector must point to the n'th
 * complex sample where 'n' is the number of filter taps. Alternatively, 'n-1'
 * samples must exist in the headroom that precedes the data start pointer.
 *
 * All vectors are 'complex', but the filter taps may consist of real values
 * in that the imaginary component is ignored and complex-real multiplication
 * is performed instead.
 *
 * Convolution format is loosely influenced by the FIR filtering operations
 * in TI DSPLIB. For more information on TI DSPLIB, see:
 *
 * "TMS320C64x+ DSP Little-Endian DSP Library Programmer’s Reference", Texas
 *     Instruments, Literature Number: SPRUEB8B, March 2006.
 */
int cxvec_convolve(const struct cxvec *in_vec,
		   const struct cxvec *h_vec,
		   struct cxvec *out_vec)
{
	int i;
	float complex *a, *h, *c;
	void (*mac_func)(float complex *, float complex *, float complex *, int);

	if (in_vec->len < out_vec->len) {
		fprintf(stderr, "convolve: Invalid vector length\n");
		return -1;
	}

	if (in_vec->flags & CXVEC_FLG_REAL_ONLY) {
		fprintf(stderr, "convolve: Input data must be complex\n");
		return -1;
	}

	if (!(h_vec->flags & CXVEC_FLG_REAL_ONLY)) {
		fprintf(stderr, "convolve: Filter taps must be real\n");
		return -1;
	}

	switch (h_vec->len) {
	default:
		mac_func = mac_real_vec_n;
	}

	a = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	c = out_vec->data;	/* output */

	/* Reset output */
	memset(c, 0, out_vec->len * sizeof(float complex));

	for (i = 0; i < out_vec->len; i++) {
		(*mac_func)(&a[i - (h_vec->len - 1)], h, &c[i], h_vec->len);
	}

	return i;
}

int cxvec_convolve_nodelay(const struct cxvec *in_vec,
			   const struct cxvec *h_vec,
			   struct cxvec *out_vec)
{
	int i;
	float complex *a, *h, *c;
	void (*mac_func)(float complex *, float complex *, float complex *, int);

	if (in_vec->len < out_vec->len) {
		fprintf(stderr, "convolve: Invalid vector length\n");
		return -1;
	}

	if (in_vec->flags & CXVEC_FLG_REAL_ONLY) {
		fprintf(stderr, "convolve: Input data must be complex\n");
		return -1;
	}

	if (!(h_vec->flags & CXVEC_FLG_REAL_ONLY)) {
		fprintf(stderr, "convolve: Filter taps must be real\n");
		return -1;
	}

	switch (h_vec->len) {
	default:
		mac_func = mac_real_vec_n;
	}

	a = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	c = out_vec->data;	/* output */

	/* Reset output */
	memset(c, 0, out_vec->len * sizeof(float complex));

	for (i = 0; i < out_vec->len; i++) {
		(*mac_func)(&a[i - (h_vec->len / 2)], h, &c[i], h_vec->len);
	}

	return i;
}

/*! \brief Single output convolution 
 *  \param[in] in Pointer to complex input samples
 *  \param[in] h Complex vector filter taps
 *  \param[out] out Pointer to complex output sample
 *
 * Convolve a single output value given without using complex vectors for input
 * and output. The rules for the normal complex convolution apply here as well.
 * Arguably, this is currently a convenience hack.
 */
void single_convolve(const float *in,
		     const float *h,
		     size_t hlen,
		     float *out)
{
	const float complex *_in = (const float complex *) in;
	const float complex *_h = (const float complex *) h;
	float complex *_out = (float complex *) out;

	*_out = 0.0f;
	mac_real_vec_n(&_in[- (h->len - 1)], _h, _out, hlen);
}
