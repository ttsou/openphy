/*
 * Intel SSE Complex Convolution 
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

#include <string.h>
#include <assert.h>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <immintrin.h>
#include <complex.h>

#include <stdio.h>

#include "convolve.h"
#include "mac.h"
#include "sigvec_internal.h"

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) \
	(((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | ((fp0)))
#endif

//#define HAVE_FMA

/* Generic non-optimized complex-real convolution */
static void conv_real_generic(const float *x,
			      const float *h,
			      float *y,
			      int h_len, int len)
{
	int i;

	memset(y, 0, len * sizeof(float complex));

	for (i = 0; i < len; i++)
		mac_real_vec_n(&x[2 * i], h, &y[2 * i], h_len);
}

/* 4-tap SSE3 complex-real convolution */
static void conv_real_sse4(const float *x,
			   const float *h,
			   float *y, int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m7 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2 * i + 0]);
		m1 = _mm_loadu_ps(&x[2 * i + 4]);
		m2 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m3 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));

		/* Quad multiply */
		m4 = _mm_mul_ps(m2, m7);
		m5 = _mm_mul_ps(m3, m7);

		/* Sum and store */
		m6 = _mm_hadd_ps(m4, m5);
		m0 = _mm_hadd_ps(m6, m6);

		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m0));
	}
}

/* 8-tap SSE3 complex-real convolution */
static void conv_real_sse8(const float *x,
			   const float *h,
			   float *y,
			   int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7, m8, m9;
#ifdef HAVE_FMA
	__m128 m10, m11;
#endif
	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);

	m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
	m5 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2 * i + 0]);
		m1 = _mm_loadu_ps(&x[2 * i + 4]);
		m2 = _mm_loadu_ps(&x[2 * i + 8]);
		m3 = _mm_loadu_ps(&x[2 * i + 12]);

		m6 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m7 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
		m8 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
		m9 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));
#ifdef HAVE_FMA
		m10 = _mm_setzero_ps();
		m11 = _mm_setzero_ps();

		m10 = _mm_fmadd_ps(m6, m4, m10);
		m10 = _mm_fmadd_ps(m8, m5, m10);
		m11 = _mm_fmadd_ps(m9, m5, m11);
		m11 = _mm_fmadd_ps(m7, m4, m11);

		m8 = _mm_hadd_ps(m10, m11);
		m9 = _mm_hadd_ps(m8, m8);
#else
		/* Quad multiply */
		m0 = _mm_mul_ps(m6, m4);
		m1 = _mm_mul_ps(m7, m4);
		m2 = _mm_mul_ps(m8, m5);
		m3 = _mm_mul_ps(m9, m5);

		/* Sum and store */
		m6 = _mm_add_ps(m0, m2);
		m7 = _mm_add_ps(m1, m3);
		m8 = _mm_hadd_ps(m6, m7);
		m9 = _mm_hadd_ps(m8, m8);
#endif
		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m9));
	}
}

/* 12-tap SSE3 complex-real convolution */
static void conv_real_sse12(const float *x,
			    const float *h,
			    float *y, int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11, m12, m13, m14;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);
	m4 = _mm_load_ps(&h[16]);
	m5 = _mm_load_ps(&h[20]);

	m12 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
	m13 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
	m14 = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(2, 0, 2, 0));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2 * i + 0]);
		m1 = _mm_loadu_ps(&x[2 * i + 4]);
		m2 = _mm_loadu_ps(&x[2 * i + 8]);
		m3 = _mm_loadu_ps(&x[2 * i + 12]);

		m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m5 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
		m6 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
		m7 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));

		m0 = _mm_loadu_ps(&x[2 * i + 16]);
		m1 = _mm_loadu_ps(&x[2 * i + 20]);

		m8 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m9 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));

		/* Quad multiply */
		m0 = _mm_mul_ps(m4, m12);
		m1 = _mm_mul_ps(m5, m12);
		m2 = _mm_mul_ps(m6, m13);
		m3 = _mm_mul_ps(m7, m13);
		m4 = _mm_mul_ps(m8, m14);
		m5 = _mm_mul_ps(m9, m14);

		/* Sum and store */
		m8  = _mm_add_ps(m0, m2);
		m9  = _mm_add_ps(m1, m3);
		m10 = _mm_add_ps(m8, m4);
		m11 = _mm_add_ps(m9, m5);

		m2 = _mm_hadd_ps(m10, m11);
		m3 = _mm_hadd_ps(m2, m2);

		_mm_store_ss(&y[2 * i + 0], m3);
		m3 = _mm_shuffle_ps(m3, m3, _MM_SHUFFLE(0, 3, 2, 1));
		_mm_store_ss(&y[2 * i + 1], m3);
	}
}

/* 16-tap SSE3 complex-real convolution */
static void conv_real_sse16(const float *x,
			    const float *h,
			    float *y, int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11, m12, m13, m14, m15;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);

	m4 = _mm_load_ps(&h[16]);
	m5 = _mm_load_ps(&h[20]);
	m6 = _mm_load_ps(&h[24]);
	m7 = _mm_load_ps(&h[28]);

	m12 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
	m13 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
	m14 = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(2, 0, 2, 0));
	m15 = _mm_shuffle_ps(m6, m7, _MM_SHUFFLE(2, 0, 2, 0));

	for (i = 0; i < in_len; i++) {
		m10 = _mm_setzero_ps();
		m11 = _mm_setzero_ps();

		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2 * i + 0]);
		m1 = _mm_loadu_ps(&x[2 * i + 4]);
		m2 = _mm_loadu_ps(&x[2 * i + 8]);
		m3 = _mm_loadu_ps(&x[2 * i + 12]);

		m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m5 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
		m6 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
		m7 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));

		m0 = _mm_loadu_ps(&x[2 * i + 16]);
		m1 = _mm_loadu_ps(&x[2 * i + 20]);
		m2 = _mm_loadu_ps(&x[2 * i + 24]);
		m3 = _mm_loadu_ps(&x[2 * i + 28]);

		m8 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m9 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
		m0 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
		m1 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));
#ifdef HAVE_FMA
		m10 = _mm_fmadd_ps(m4, m12, m10);
		m10 = _mm_fmadd_ps(m6, m13, m10);
		m10 = _mm_fmadd_ps(m8, m14, m10);
		m10 = _mm_fmadd_ps(m0, m15, m10);
		m11 = _mm_fmadd_ps(m5, m12, m11);
		m11 = _mm_fmadd_ps(m7, m13, m11);
		m11 = _mm_fmadd_ps(m9, m14, m11);
		m11 = _mm_fmadd_ps(m1, m15, m11);

		m2 = _mm_hadd_ps(m10, m11);
		m3 = _mm_hadd_ps(m2, m2);

		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m3));
#else
		/* Quad multiply */
		m0 = _mm_mul_ps(m4, m12);
		m1 = _mm_mul_ps(m5, m12);
		m2 = _mm_mul_ps(m6, m13);
		m3 = _mm_mul_ps(m7, m13);

		m4 = _mm_mul_ps(m8, m14);
		m5 = _mm_mul_ps(m9, m14);
		m6 = _mm_mul_ps(m10, m15);
		m7 = _mm_mul_ps(m11, m15);

		/* Sum and store */
		m8  = _mm_add_ps(m0, m2);
		m9  = _mm_add_ps(m1, m3);
		m10 = _mm_add_ps(m4, m6);
		m11 = _mm_add_ps(m5, m7);

		m0 = _mm_add_ps(m8, m10);
		m1 = _mm_add_ps(m9, m11);
		m2 = _mm_hadd_ps(m0, m1);
		m3 = _mm_hadd_ps(m2, m2);
#endif
		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m3));
	}
}

/* 20-tap SSE3 complex-real convolution */
static void conv_real_sse20(const float *x,
			    const float *h,
			    float *y, int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m11, m12, m13, m14, m15;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);
	m4 = _mm_load_ps(&h[16]);
	m5 = _mm_load_ps(&h[20]);
	m6 = _mm_load_ps(&h[24]);
	m7 = _mm_load_ps(&h[28]);
	m8 = _mm_load_ps(&h[32]);
	m9 = _mm_load_ps(&h[36]);

	m11 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
	m12 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
	m13 = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(2, 0, 2, 0));
	m14 = _mm_shuffle_ps(m6, m7, _MM_SHUFFLE(2, 0, 2, 0));
	m15 = _mm_shuffle_ps(m8, m9, _MM_SHUFFLE(2, 0, 2, 0));

	for (i = 0; i < in_len; i++) {
		/* Multiply-accumulate first 12 taps */
		m0 = _mm_loadu_ps(&x[2 * i + 0]);
		m1 = _mm_loadu_ps(&x[2 * i + 4]);
		m2 = _mm_loadu_ps(&x[2 * i + 8]);
		m3 = _mm_loadu_ps(&x[2 * i + 12]);
		m4 = _mm_loadu_ps(&x[2 * i + 16]);
		m5 = _mm_loadu_ps(&x[2 * i + 20]);

		m6  = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m7  = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
		m8  = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
		m9  = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));
		m0  = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(2, 0, 2, 0));
		m1  = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(3, 1, 3, 1));

		m2 = _mm_mul_ps(m6, m11);
		m3 = _mm_mul_ps(m7, m11);
		m4 = _mm_mul_ps(m8, m12);
		m5 = _mm_mul_ps(m9, m12);
		m6 = _mm_mul_ps(m0, m13);
		m7 = _mm_mul_ps(m1, m13);

		m0  = _mm_add_ps(m2, m4);
		m1  = _mm_add_ps(m3, m5);
		m8  = _mm_add_ps(m0, m6);
		m9  = _mm_add_ps(m1, m7);

		/* Multiply-accumulate last 8 taps */
		m0 = _mm_loadu_ps(&x[2 * i + 24]);
		m1 = _mm_loadu_ps(&x[2 * i + 28]);
		m2 = _mm_loadu_ps(&x[2 * i + 32]);
		m3 = _mm_loadu_ps(&x[2 * i + 36]);

		m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
		m5 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
		m6 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
		m7 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));

		m0 = _mm_mul_ps(m4, m14);
		m1 = _mm_mul_ps(m5, m14);
		m2 = _mm_mul_ps(m6, m15);
		m3 = _mm_mul_ps(m7, m15);

		m4  = _mm_add_ps(m0, m2);
		m5  = _mm_add_ps(m1, m3);

		/* Final sum and store */
		m0 = _mm_add_ps(m8, m4);
		m1 = _mm_add_ps(m9, m5);
		m2 = _mm_hadd_ps(m0, m1);
		m3 = _mm_hadd_ps(m2, m2);

		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m3));
	}
}

#ifdef HAVE_FMA 
/* 20-tap SSE3 complex-real convolution */
static void conv_real_fma_8n(const float *x,
			     const float *h,
			     float *y,
			     int h_len, int len)
{
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11;

	for (int i = 0; i < len; i++) {
		/* Zero */
		m10 = _mm_setzero_ps();
		m11 = _mm_setzero_ps();

		for (int n = 0; n < h_len / 8; n++) {
			/* Load (aligned) filter taps */
			m0 = _mm_load_ps(&h[16 * n + 0]);
			m1 = _mm_load_ps(&h[16 * n + 4]);
			m2 = _mm_load_ps(&h[16 * n + 8]);
			m3 = _mm_load_ps(&h[16 * n + 12]);

			m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
			m5 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));

			/* Load (unaligned) input data */
			m0 = _mm_loadu_ps(&x[2 * i + 16 * n + 0]);
			m1 = _mm_loadu_ps(&x[2 * i + 16 * n + 4]);
			m2 = _mm_loadu_ps(&x[2 * i + 16 * n + 8]);
			m3 = _mm_loadu_ps(&x[2 * i + 16 * n + 12]);

			m6 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
			m7 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
			m8 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
			m9 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));

			m10 = _mm_fmadd_ps(m6, m4, m10);
			m10 = _mm_fmadd_ps(m8, m5, m10);
			m11 = _mm_fmadd_ps(m7, m4, m11);
			m11 = _mm_fmadd_ps(m9, m5, m11);
		}

		m0 = _mm_hadd_ps(m10, m11);
		m0 = _mm_hadd_ps(m0, m0);

		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m0));
	}
}
#else
/* 8N tap SSE3 complex-real convolution */
static void sse_conv_real8n(const float *x, const float *h,
			    float *y, int h_len, int len)
{
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11, m12, m13, m14, m15;

	for (int i = 0; i < len; i++) {
		m14 = _mm_setzero_ps();
		m15 = _mm_setzero_ps();

		for (int n = 0; n < h_len / 8; n++) {
			/* Load (aligned) filter taps */
			m0 = _mm_load_ps(&h[16 * n + 0]);
			m1 = _mm_load_ps(&h[16 * n + 4]);
			m2 = _mm_load_ps(&h[16 * n + 8]);
			m3 = _mm_load_ps(&h[16 * n + 12]);

			m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
			m5 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));

			/* Load (unaligned) input data */
			m0 = _mm_loadu_ps(&x[2 * i + 16 * n + 0]);
			m1 = _mm_loadu_ps(&x[2 * i + 16 * n + 4]);
			m2 = _mm_loadu_ps(&x[2 * i + 16 * n + 8]);
			m3 = _mm_loadu_ps(&x[2 * i + 16 * n + 12]);

			m6 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(2, 0, 2, 0));
			m7 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(3, 1, 3, 1));
			m8 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(2, 0, 2, 0));
			m9 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(3, 1, 3, 1));

			/* Quad multiply */
			m10 = _mm_mul_ps(m6, m4);
			m11 = _mm_mul_ps(m8, m5);
			m12 = _mm_mul_ps(m7, m4);
			m13 = _mm_mul_ps(m9, m5);

			/* Accumulate */
			m10 = _mm_add_ps(m10, m11);
			m12 = _mm_add_ps(m12, m13);
			m14 = _mm_add_ps(m14, m10);
			m15 = _mm_add_ps(m15, m12);
		}

		m0 = _mm_hadd_ps(m14, m15);
		m0 = _mm_hadd_ps(m0, m0);

		_mm_store_sd((double *) &y[2 * i], _mm_castps_pd(m0));
	}
}
#endif

static int check_params(const struct cxvec *in,
			const struct cxvec *h,
			const struct cxvec *out)
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

/*! \brief Convolve two complex vectors 
 *  \param[in] in_vec Complex input signal
 *  \param[in] h_vec Complex filter taps (stored in reverse order)
 *  \param[out] out_vec Complex vector output
 *
 * Modified convole with filter length dependent SSE optimization.
 * See generic convole call for additional information.
 */
int cxvec_convolve(const struct cxvec *in_vec,
		   const struct cxvec *h_vec,
		   struct cxvec *out_vec)
{
	int i;
	const float complex *x, *h;
	float complex *y;

	void (*conv_func)
		(const float *, const float *, float *, int) = NULL;
	void (*conv_func_n)
		(const float *, const float *, float *, int, int) = NULL;

	if (check_params(in_vec, h_vec, out_vec) < 0)
		return -1;

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_sse4;
		break;
	case 8:
		conv_func = conv_real_sse8;
		break;
	case 12:
		conv_func = conv_real_sse12;
		break;
	case 16:
		conv_func = conv_real_sse16;
		break;
	case 20:
		conv_func = conv_real_sse20;
		break;
	default:
#ifdef HAVE_FMA
		if (!(h_vec->len % 8))
			conv_func_n = conv_real_fma_8n;
#else
		if (!(h_vec->len % 8))
			conv_func_n = sse_conv_real8n;
#endif
	}

	x = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	y = out_vec->data;	/* output */

	if (conv_func) {
		conv_func((const float *) &x[-(h_vec->len - 1)],
			  (const float *) h,
			  (float *) y, out_vec->len);
	} else if (conv_func_n) {
		conv_func_n((const float *) &x[-(h_vec->len - 1)],
			    (const float *) h,
			    (float *) y,
			    h_vec->len, out_vec->len);
	} else {
		conv_real_generic((const float *) &x[-(h_vec->len - 1)],
				  (const float *) h,
				  (float *) y,
				  h_vec->len, out_vec->len);
	}

	return i;
}

int cxvec_convolve_nodelay(const struct cxvec *in_vec,
			   const struct cxvec *h_vec,
			   struct cxvec *out_vec)
{
	int i;
	float complex *x, *h, *y;
	void (*conv_func)(const float *, const float *, float *, int) = NULL;
	void (*conv_func2)(const float *, const float *,
			   float *, int, int) = NULL;

	if (check_params(in_vec, h_vec, out_vec) < 0)
		return -1;

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_sse4;
		break;
	case 8:
		conv_func = conv_real_sse8;
		break;
	case 12:
		conv_func = conv_real_sse12;
		break;
	case 16:
		conv_func = conv_real_sse16;
		break;
	case 20:
		conv_func = conv_real_sse20;
		break;
	default:
#ifdef HAVE_FMA
		if (!(h_vec->len % 8))
			conv_func2 = conv_real_fma_8n;
#else
		if (!(h_vec->len % 8))
			conv_func2 = sse_conv_real8n;
#endif
	}

	x = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	y = out_vec->data;	/* output */

	if (conv_func2) {
		conv_func2((const float *) &x[-(h_vec->len / 2)],
			   (const float *) h,
			   (float *) y,
			   h_vec->len, out_vec->len);
	} else if (!conv_func) {
		conv_real_generic((const float *) &x[-(h_vec->len / 2)],
				  (const float *) h,
				  (float *) y,
				  h_vec->len, out_vec->len);
	} else {
		conv_func((const float *) &x[-(h_vec->len / 2)],
			  (const float *) h,
			  (float *) y, out_vec->len);
	}

	return i;
}

/*! \brief Single output convolution 
 *  \param[in] in Pointer to complex input samples
 *  \param[in] h Complex vector filter taps
 *  \param[out] out Pointer to complex output sample
 *
 * Modified single output convole with filter length dependent SSE
 * optimization. See generic convole call for additional information.
 */
int single_convolve(const float *in,
		    const float *h,
		    size_t hlen,
		    float *out)
{
	const float complex *_in = (const float complex *) in;
	float complex *_out = (float complex *) out;

	void (*conv_func)
		(const float *, const float *, float *, int) = NULL;
	void (*conv_func_n)
		(const float *, const float *, float *, int, int) = NULL;

	switch (hlen) {
	case 4:
		conv_func = conv_real_sse4;
		break;
	case 8:
		conv_func = conv_real_sse8;
		break;
	case 12:
		conv_func = conv_real_sse12;
		break;
	case 16:
		conv_func = conv_real_sse16;
		break;
	case 20:
		conv_func = conv_real_sse20;
		break;
	default:
#ifdef HAVE_FMA
		if (!(hlen % 8))
			conv_func_n = conv_real_fma_8n;
#else
		if (!(hlen % 8))
			conv_func_n = sse_conv_real8n;
#endif
		break;
	}

	if (conv_func) {
		conv_func((const float *) &_in[-(hlen - 1)],
			  (const float *) h,
			  (float *) _out, 1);
	} else if (conv_func_n) {
		conv_func_n((const float *) &_in[-(hlen - 1)],
			    (const float *) h,
			    (float *) _out, hlen, 1);

	} else {
		conv_real_generic((const float *) &_in[-(hlen - 1)],
				  (const float *) h,
				  (float *) _out, hlen, 1);
	}

	return 1;
}
