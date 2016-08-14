/*
 * Interpolation
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


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "complex.h"

#include "interpolate.h"
#include "convolve.h"
#include "sigvec_internal.h"

#define M_PIf 3.141592653589793238462643383

static int init_filter(struct interp_hdl *hdl, int len, float p)
{
	float midpt = (len - 1) / 2.0;
	float complex sum = 0.0;
	float complex *taps;
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;

	hdl->h = cxvec_alloc(len, 0, 0, NULL, flags);

	/* 
	 * Generate the prototype filter with a Blackman-harris window.
	 * Scale coefficients with DC filter gain set to unity divided
	 * by the number of filter partitions. 
	 */
	float a0 = 0.35875;
	float a1 = 0.48829;
	float a2 = 0.14128;
	float a3 = 0.01168;

	taps = hdl->h->data;

	for (int i = 0; i < len; i++) {
		taps[i] = cxvec_sinc(((float) i - midpt) / p);
		taps[i] *= a0 -
			   a1 * cos(2.0 * M_PIf * i / (len - 1)) +
			   a2 * cos(4.0 * M_PIf * i / (len - 1)) -
			   a3 * cos(6.0 * M_PIf * i / (len - 1));
		sum += taps[i];
	}

	/* 
	 * Populate partition filters and reverse the coefficients per
	 * convolution requirements.
	 */
	for (int i = 0; i < len; i++)
		taps[i] *= 3.0 / sum;

	cxvec_rvrs(hdl->h, hdl->h);

	return 0;
}

struct interp_hdl *init_interp(int len, float p)
{
	struct interp_hdl *hdl;

	hdl = malloc(sizeof *hdl);
	init_filter(hdl, len, p);

	return hdl;
};

void free_interp(struct interp_hdl *hdl)
{
	if (!hdl)
		return;

	cxvec_free(hdl->h);
	free(hdl);
}

int cxvec_interp(struct interp_hdl *hdl, struct cxvec *x, struct cxvec *y)
{
	int len = x->len;
	int head = x->start_idx;
	int tail = x->buf_len - (x->start_idx + x->len);
	int min = hdl->h->len >> 1;

	if ((head < min) || (tail < min)) {
		fprintf(stderr, "cxvec_interp: invalid input length\n");
		return -1;
	}

	/*
	 * Cyclic convolution: There is a slight of hand trick here because we
	 * do not want to convolve over the center carrier of the OFDM signal,
	 * so we offset the history by one to avoid the null.
	 *
	 * Be aware if this interpolation method is used in other cases.
	 */
	memcpy(&x->data[-head + 1], &x->data[len - head], head * sizeof(float complex));
	memcpy(&x->data[len], &x->data[1], tail * sizeof(float complex));

	cxvec_convolve_nodelay(x, hdl->h, y);

	return 0;
}
