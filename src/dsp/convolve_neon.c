/*
 * convolve_neon.c
 *
 * ARM NEON optimized complex convolution
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

void conv_real_neon4(float *restrict x,
		     float *restrict h,
		     float *restrict y,
		     int len);

void conv_real_neon8(float *restrict x,
		     float *restrict h,
		     float *restrict y,
		     int len);

void conv_real_neon12(float *restrict x,
		      float *restrict h,
		      float *restrict y,
		      int len);

void conv_real_neon16(float *restrict x,
		      float *restrict h,
		      float *restrict y,
		      int len);

void conv_real_neon20(float *restrict x,
		      float *restrict h,
		      float *restrict y,
		      int len);

/* Generic non-optimized complex-real convolution */
static void conv_real_generic(complex float *x,
			      complex float *h,
			      complex float *y,
			      int h_len, int len)
{
	int i;

	memset(y, 0, len * sizeof(complex float));

	for (i = 0; i < len; i++) {
		mac_real_vec_n(&x[i], h, &y[i], h_len);
	}
}

/*! \brief Convolve two complex vectors 
 *  \param[in] in_vec Complex input signal
 *  \param[in] h_vec Complex filter taps (stored in reverse order)
 *  \param[out] out_vec Complex vector output
 *
 * Modified convole with filter length dependent SSE optimization.
 * See generic convole call for additional information.
 */
int cxvec_convolve(struct cxvec *restrict in_vec,
		   struct cxvec *restrict h_vec,
		   struct cxvec *restrict out_vec)
{
	complex float *x, *h, *y;
	void (*conv_func)(float *, float *, float *, int);

	if (in_vec->len < out_vec->len) {
		fprintf(stderr, "conv4: Invalid vector lengths of %i and %i\n",
			in_vec->len, out_vec->len);
		return -1;
	}

	if (in_vec->flags & CXVEC_FLG_REAL_ONLY) {
		fprintf(stderr, "conv4: Input data must be complex\n");
		return -1;
	}

	if (!(h_vec->flags & CXVEC_FLG_REAL_ONLY)) {
		fprintf(stderr, "conv4: Taps must be real\n");
		return -1;
	}

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_neon4;
		break;
	case 8:
		conv_func = conv_real_neon8;
		break;
	case 12:
		conv_func = conv_real_neon12;
		break;
	case 16:
		conv_func = conv_real_neon16;
		break;
	case 20:
		conv_func = conv_real_neon20;
		break;
	default:
		conv_func = NULL;
	}

	x = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	y = out_vec->data;	/* output */

	if (!conv_func) {
		conv_real_generic(&x[-(h_vec->len - 1)],
				  h,
				  y,
				  h_vec->len, out_vec->len);
	} else {
		conv_func((float *) &x[-(h_vec->len - 1)],
			  (float *) h,
			  (float *) y, out_vec->len);
	}

	return out_vec->len;
}

/*! \brief Single output convolution 
 *  \param[in] in Pointer to complex input samples
 *  \param[in] h Complex vector filter taps
 *  \param[out] out Pointer to complex output sample
 *
 * Modified single output convole with filter length dependent SSE
 * optimization. See generic convole call for additional information.
 */
int single_convolve(complex float *restrict in,
		    struct cxvec *restrict h_vec,
		    complex float *restrict out)
{
	void (*conv_func)(float *, float *, float *, int);

	if (!(h_vec->flags & (CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN))) {
		fprintf(stderr, "convolve: Taps must be real and aligned\n");
		return -1;
	}

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_neon4;
		break;
	case 8:
		conv_func = conv_real_neon8;
		break;
	case 12:
		conv_func = conv_real_neon12;
		break;
	case 16:
		conv_func = conv_real_neon16;
		break;
	case 20:
		conv_func = conv_real_neon20;
		break;
	default:
		conv_func = NULL;
	}

	if (!conv_func) {
		conv_real_generic(&in[-(h_vec->len - 1)],
				  h_vec->data,
				  out,
				  h_vec->len, 1);
	} else {
		conv_func((float *) &in[-(h_vec->len - 1)],
			  (float *) h_vec->data,
			  (float *) out, 1);
	}

	return 1;
}

static int check_params(struct cxvec *in,
			struct cxvec *h, struct cxvec *out)
{
	if (in->len < out->len) {
		fprintf(stderr, "convolve: Invalid vector length\n");
		return -1;
	}

	if (in->flags & CXVEC_FLG_REAL_ONLY) {
		fprintf(stderr, "convolve: Input data must be complex\n");
		return -1;
	}

	if (!(h->flags & (CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN))) {
		fprintf(stderr, "convolve: Taps must be real and aligned\n");
		return -1;
	}

	return 0;
}

int cxvec_convolve_nodelay(struct cxvec *restrict in_vec,
			   struct cxvec *restrict h_vec,
			   struct cxvec *restrict out_vec)
{
	float complex *x, *h, *y;
	void (*conv_func)(float *, float *, float *, int);

	if (check_params(in_vec, h_vec, out_vec) < 0)
		return -1;

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_neon4;
		break;
	case 8:
		conv_func = conv_real_neon8;
		break;
	case 12:
		conv_func = conv_real_neon12;
		break;
	case 16:
		conv_func = conv_real_neon16;
		break;
	case 20:
		conv_func = conv_real_neon20;
		break;
	default:
		conv_func = NULL;
	}

	x = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	y = out_vec->data;	/* output */

	/* Reset output */
	memset(y, 0, out_vec->len * sizeof(float complex));

	if (!conv_func) {
		conv_real_generic(&x[-(h_vec->len / 2)],
				  h,
				  y,
				  h_vec->len, out_vec->len);
	} else {
		conv_func((float *) &x[-(h_vec->len / 2)],
			  (float *) h,
			  (float *) y, out_vec->len);
	}

	return out_vec->len;
}
