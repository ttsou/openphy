/*
 * NEON scaling
 * Copyright (C) 2012,2013 Thomas Tsou <tom@tsou.cc>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <malloc.h>
#include <string.h>
#include <mult.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void neon_cmplx_mul_4n(float *, float *, float *, int);

static void cmplx_mul_ps(float *out, float *a, float *b, int len)
{
	float ai, aq, bi, bq;

	for (int i = 0; i < len; i++) {
		ai = a[2 * i + 0];
		aq = a[2 * i + 1];

		bi = b[2 * i + 0];
		bq = b[2 * i + 1];

		out[2 * i + 0] = ai * bi - aq * bq;
		out[2 * i + 1] = ai * bq + aq * bi;
	}
}

void mul_complex(float *out, float *a, float *b, int len)
{
#ifdef HAVE_NEON
	if (len % 4)
		cmplx_mul_ps(out, a, b, len);
	else
		neon_cmplx_mul_4n(out, a, b, len >> 2);
#else
	cmplx_mul_ps(out, a, b, len);
#endif
}
