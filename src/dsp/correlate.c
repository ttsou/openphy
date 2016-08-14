/*
 * Complex Correlation 
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
#include <string.h>
#include <complex.h>

#include "sigvec.h"
#include "correlate.h"
#include "convolve.h"
#include "mac.h"
#include "sigvec_internal.h"

float complex fautocorr(float complex *a, int len, int offset)
{
	float complex *b  = a + offset;
	float complex sum = 0.0f;

	for (int i = 0; i < len; i++)
		sum += a[i] * conj(b[i]);

	return sum;
}

float complex cxvec_mac(struct cxvec *a, struct cxvec *b)
{
	float complex sum = 0.0f;

	if (a->len != b->len) {
		fprintf(stderr, "cxvec_mac: invalid length\n");
		return 0.0f;
	}

	for (int i = 0; i < a->len; i++) {
		sum += crealf(a->data[i]) * crealf(b->data[i]) -
		       cimagf(a->data[i]) * cimagf(b->data[i]) +
		       (crealf(a->data[i]) * cimagf(b->data[i]) +
		       cimagf(a->data[i]) * crealf(b->data[i])) * I;
	}

	return sum;
}

int cxvec_corr(struct cxvec *x_vec, struct cxvec *h_vec, struct cxvec *y_vec, int start, int len)
{
	float complex *x, *h, *y;

	if ((x_vec->len < y_vec->len) || (start + len > y_vec->len)) {
		fprintf(stderr, "cxvec_corr: invalid length\n");
		fprintf(stderr, "x->len %i, y->len %i, start %i, len %i\n",
			(int) x_vec->len, (int) y_vec->len, start, len);
		return -1;
	}

	x = x_vec->data;
	h = h_vec->data;
	y = y_vec->data;

	memset(y, 0, y_vec->len * sizeof(float complex));

	for (int i = start; i < start + len; i++) {
		mac_cmplx_vec_n((const float *) &x[i - (h_vec->len - 1)],
				(const float *) h,
				(float *) &y[i],
				h_vec->len);
	}

	return 0;
}
