/*
 * Complex Signal Vectors
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
#include <stdio.h>
#include <math.h>

#include "lte/sigvec.h"

void mul_cx_neon4(float *a, float *b, float *out, int len);
void mac_cx_neon4(float *a, float *b, float *out, int len);
void mac4_conj_neon(float *a, float *b, float *out, int len);
void mul_const_neon4(float *a, float *b, float *out, int len);
void norm4_neon(float *a, float *out, int len);

/*! \brief Complex multiply the contents of two complex vectors
 *  \param[in] a Input complex vector 'A'
 *  \param[in] b Input complex vector 'B'
 *  \param[out] out Output complex vector
 */
int cxvec_mul4(struct cxvec *a, struct cxvec *b, struct cxvec *out)
{
	int len;

	if ((a->len != b->len) || (b->len != out->len)) {
		fprintf(stderr, "cxvec_mul4: invalid input\n");
		return -1;
	}

	len = a->len >> 2;

	mul_cx_neon4((float *) a->data,
		     (float *) b->data,
		     (float *) out->data, len);

	return a->len;
}

float complex cxvec_mac4(float complex *a, float complex *b, float complex *out, int len)
{
	if (len < 4) {
		for (int i = 0; i < len; i++)
			*out += a[i] * b[i];

		return len;
	}

	int remain = len % 4;

	mac_cx_neon4((float *) a,
		     (float *) b,
		     (float *) out, len >> 2);

	for (int i = 0; i < remain; i++) {
		*out += a[len - 1 - i] * b[len - 1 - i];
	}

	return *out;
}

void cxvec_mac4_conj(float complex *a, float complex *b, float complex *out, int len)
{
	mac4_conj_neon((float *) (a - 3),
		       (float *) b,
		       (float *) out, len >> 2);
}

void cxvec_mul4_const(float complex *a, float complex *b, float complex *out, int len)
{
	mul_const_neon4((float *) (a - 3),
			(float *) b,
			(float *) out, len >> 2);
}

void cxvec_norm4(float complex *a, float complex *out, int len)
{
	norm4_neon((float *) a, (float *) out, len >> 2);
}

void cxvec_norm(float complex *a, float complex *out, int len)
{
	float complex sum = 0.0;
	for (int i = 0; i < len; i++)
		sum += crealf(a[i]) * crealf(a[i]) +
		       cimagf(a[i]) * cimagf(a[i]);

	*out = sqrt(sum);
}

void cxvec_mac_conj(float complex *a, float complex *b, float complex *out, int len)
{
	for (int i = 0; i < len; i++)
		*out += a[-i] * conjf(b[i]);
}

void cxvec_mul_const(float complex *a, float complex *b, float complex *out, int len)
{
	for (int i = 0; i < len; i++)
		out[i] += a[-i] * *b;
}
