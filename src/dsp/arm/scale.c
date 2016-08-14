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
#include <scale.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void neon_scale_4n(float *, float *, float *, int);

static void scale_ps(float *out, float *in, float *scale, int len)
{
	float ai, aq, bi, bq;

	bi = scale[0];
	bq = scale[1];

	for (int i = 0; i < len; i++) {
		ai = in[2 * i + 0];
		aq = in[2 * i + 1];

		out[2 * i + 0] = ai * bi - aq * bq;
		out[2 * i + 1] = ai * bq + aq * bi;
	}
}

void scale_complex(float *out, float *in, float* scale, int len)
{
#ifdef HAVE_NEON
	if (len % 4)
		scale_ps(out, in, scale, len);
	else
		neon_scale_4n(in, scale, out, len >> 2);
#else
	scale_ps(out, in, scale, len);
#endif
}
