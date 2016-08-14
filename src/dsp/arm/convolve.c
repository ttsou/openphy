/*
 * NEON Convolution
 * Copyright (C) 2012, 2013 Thomas Tsou <tom@tsou.cc>
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
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Forward declarations from base implementation */
int _base_convolve_real(float *x, int x_len,
			float *h, int h_len,
			float *y, int y_len,
			int start, int len,
			int step, int offset);

int _base_convolve_complex(float *x, int x_len,
			   float *h, int h_len,
			   float *y, int y_len,
			   int start, int len,
			   int step, int offset);

int bounds_check(int x_len, int h_len, int y_len,
		 int start, int len, int step);

#ifdef HAVE_NEON
/* Calls into NEON assembler */
void neon_conv_real4(float *x, float *h, float *y, int len);
void neon_conv_real8(float *x, float *h, float *y, int len);
void neon_conv_real12(float *x, float *h, float *y, int len);
void neon_conv_real16(float *x, float *h, float *y, int len);
void neon_conv_real20(float *x, float *h, float *y, int len);
void mac_cx_neon4(float *x, float *h, float *y, int len);

/* Complex-complex convolution */
static void neon_conv_cmplx_4n(float *x, float *h, float *y, int h_len, int len)
{
	for (int i = 0; i < len; i++)
		mac_cx_neon4(&x[2 * i], h, &y[2 * i], h_len >> 2);
}
#endif

/* API: Aligned complex-real */
int convolve_real(float *x, int x_len,
		  float *h, int h_len,
		  float *y, int y_len,
		  int start, int len,
		  int step, int offset)
{
	void (*conv_func)(float *, float *, float *, int) = NULL;

	if (bounds_check(x_len, h_len, y_len, start, len, step) < 0)
		return -1;

	memset(y, 0, len * 2 * sizeof(float));

#ifdef HAVE_NEON
	if (step <= 4) {
		switch (h_len) {
		case 4:
			conv_func = neon_conv_real4;
			break;
		case 8:
			conv_func = neon_conv_real8;
			break;
		case 12:
			conv_func = neon_conv_real12;
			break;
		case 16:
			conv_func = neon_conv_real16;
			break;
		case 20:
			conv_func = neon_conv_real20;
			break;
		}
	}
#endif
	if (conv_func) {
		conv_func(&x[2 * (-(h_len - 1) + start)],
			  h, y, len);
	} else {
		_base_convolve_real(x, x_len,
				    h, h_len,
				    y, y_len,
				    start, len, step, offset);
	}

	return len;
}


/* API: Aligned complex-complex */
int convolve_complex(float *x, int x_len,
		     float *h, int h_len,
		     float *y, int y_len,
		     int start, int len,
		     int step, int offset)
{
	void (*conv_func)(float *, float *, float *, int, int) = NULL;

	if (bounds_check(x_len, h_len, y_len, start, len, step) < 0)
		return -1;

	memset(y, 0, len * 2 * sizeof(float));

#ifdef HAVE_NEON
	if (step <= 4 && !(h_len % 4))
		conv_func = neon_conv_cmplx_4n;
#endif
	if (conv_func) {
		conv_func(&x[2 * (-(h_len - 1) + start)],
			  h, y, h_len, len);
	} else {
		_base_convolve_complex(x, x_len,
				       h, h_len,
				       y, y_len,
				       start, len, step, offset);
	}

	return len;
}
