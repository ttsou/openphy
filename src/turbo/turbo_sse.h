/*
 * LTE Max-Log-MAP turbo decoder - SSE recursions
 *
 * Copyright (C) 2015 Ettus Research LLC
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
 *
 * Author: Tom Tsou <tom.tsou@ettus.com>
 */

#ifdef HAVE_SSE3
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <immintrin.h>

#if defined(HAVE_SSE4_1) || defined(HAVE_SSE41)
#ifdef HAVE_AVX2
#include <immintrin.h>
#endif
#include <smmintrin.h>
#define MAXPOS(M0,M1,M2) \
{ \
	M1 = _mm_set1_epi16(32767); \
	M2 = _mm_sub_epi16(M1, M0); \
	M2 = _mm_minpos_epu16(M2); \
	M2 = _mm_sub_epi16(M1, M2); \
}
#else
#define MAXPOS(M0,M1,M2) \
{ \
	M1 = _mm_shuffle_epi32(M0, _MM_SHUFFLE(0, 0, 3, 2)); \
	M2 = _mm_max_epi16(M0, M1); \
	M1 = _mm_shufflelo_epi16(M2, _MM_SHUFFLE(0, 0, 3, 2)); \
	M2 = _mm_max_epi16(M2, M1); \
	M1 = _mm_shufflelo_epi16(M2, _MM_SHUFFLE(0, 0, 0, 1)); \
	M2 = _mm_max_epi16(M2, M1); \
}
#endif

/*
 * LTE parity and systematic output bits
 *
 * Bits are ordered consecutively from state 0 with even-odd referring to 0 and
 * 1 transitions respectively. Only bits for the upper paths (transitions from
 * states 0-3) are shown. Trellis symmetry and anti-symmetry applies, so for
 * lower paths (transitions from states 4-7), systematic bits will be repeated
 * and parity bits will repeated with inversion.
 */
#define LTE_SYSTEM_OUTPUT	1, -1, -1, 1, -1, 1, 1, -1
#define LTE_PARITY_OUTPUT	-1, 1, 1, -1, -1, 1, 1, -1

/*
 * Shuffled systematic bits
 *
 * Rearranged systematic output based on the ordering of branch metrics used for
 * forward metric calculation. This avoids shuffling within the iterating
 * portion. The shuffle pattern is shown below. After shuffling, only upper bits
 * are used - lower portion is repeated and inverted.
 *
 * 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15
 */
#define LTE_SYSTEM_FW_SHUFFLE	-1, 1, -1, 1, 1, -1, 1, -1
#define LTE_PARITY_FW_SHUFFLE	-1, -1, 1, 1, 1, 1, -1, -1
#define LTE_PARITY_BW_SHUFFLE	LTE_PARITY_FW_SHUFFLE

/*
 * Max-Log-MAP Forward Recursion
 *
 * Forward trellis traversal of Max-Log-MAP variation of BCJR algorithm. This
 * includes generating branch metrics (gamma) and foward metrics (alpha). Branch
 * metrics are stored in shuffled form (i.e. ordering that matches the forward
 * metric output).
 */
#define FW_SHUFFLE_MASK0 13, 12, 9, 8, 5, 4, 1, 0, 13, 12, 9, 8, 5, 4, 1, 0
#define FW_SHUFFLE_MASK1 15, 14, 11, 10, 7, 6, 3, 2, 15, 14, 11, 10, 7, 6, 3, 2

static inline int16_t gen_fw_metrics(int16_t *bm, int8_t x, int8_t z,
		       int16_t *sums_p, int16_t *sums_c, int16_t le)
{
	__m128i m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13;

	m0 = _mm_set1_epi16(x);
	m1 = _mm_set1_epi16(z);
	m2 = _mm_set1_epi16(le);
	m3 = _mm_setzero_si128();
	m4 = _mm_set_epi16(LTE_SYSTEM_FW_SHUFFLE);
	m5 = _mm_set_epi16(LTE_PARITY_FW_SHUFFLE);

	/* Branch metrics */
	m6 = _mm_sign_epi16(m0, m4);
	m7 = _mm_sign_epi16(m1, m5);
	m8 = _mm_sign_epi16(m2, m4);
	m8 = _mm_srai_epi16(m8, 1);

	m6 = _mm_adds_epi16(m6, m7);
	m6 = _mm_adds_epi16(m6, m8);
	m7 = _mm_subs_epi16(m3, m6);

	/* Pre-interleave for backward recursion */
	m8 = _mm_unpacklo_epi16(m6, m7);
	_mm_store_si128((__m128i *) bm, m8);

	/* Forward metrics */
	m9  = _mm_load_si128((__m128i *) sums_p);
	m10 = _mm_set_epi8(FW_SHUFFLE_MASK0);
	m11 = _mm_set_epi8(FW_SHUFFLE_MASK1);

	m12 = _mm_shuffle_epi8(m9, m10);
	m13 = _mm_shuffle_epi8(m9, m11);
	m12 = _mm_adds_epi16(m12, m6);
	m13 = _mm_adds_epi16(m13, m7);

	m0 = _mm_max_epi16(m12, m13);
#ifdef HAVE_AVX2
	m1 = _mm_broadcastw_epi16(m0);
#else
	m1 =_mm_unpacklo_epi16(m0, m0);
	m1 =_mm_unpacklo_epi32(m1, m1);
	m1 =_mm_unpacklo_epi64(m1, m1);
#endif
	m0 = _mm_subs_epi16(m0, m1);

	_mm_store_si128((__m128i *) sums_c, m0);

	/* Return 32-bit integer with garbage in upper 16-bits */
	return _mm_cvtsi128_si32(m1);
}

/*
 * Max-Log-MAP Backard Recursion
 *
 * Reverse trellis traversal of Max-Log-MAP variation of BCJR algorithm. This
 * includes generating reverse metrics (beta) and outputing log likihood ratios.
 * There is only a single buffer for backward metrics; the previous metric is
 * overwritten with the new metric.
 *
 * Note: Because the summing pattern of 0 and 1 LLR bits is coded into the
 * interleaver, this implementation is specific to LTE generator polynomials.
 * The shuffle mask will need to be recomputed for non-LTE generators.
 */
#define LV_BW_SHUFFLE_MASK0 7, 6, 15, 14, 13, 12, 5, 4, 3, 2, 11, 10, 9, 8, 1, 0
#define LV_BW_SHUFFLE_MASK1 15, 14, 7, 6, 5, 4, 13, 12, 11, 10, 3, 2, 1, 0, 9, 8

static inline int16_t gen_bw_metrics(int16_t *bm, const int8_t z,
		       int16_t *fw, int16_t *bw, int16_t norm)
{
	__m128i m0, m1, m3, m4, m5, m6, m9, m10, m11, m12, m13, m14, m15;

	m0 = _mm_set1_epi16(z);
	m1 = _mm_set_epi16(LTE_PARITY_BW_SHUFFLE);

	/* Partial branch metrics */
	m13 = _mm_sign_epi16(m0, m1);

	/* Backward metrics */
	m0 = _mm_load_si128((__m128i *) bw);
	m1 = _mm_load_si128((__m128i *) bm);
	m6 = _mm_load_si128((__m128i *) fw);
	m3 = _mm_set1_epi16(norm);

	m4 = _mm_unpacklo_epi16(m0, m0);
	m5 = _mm_unpackhi_epi16(m0, m0);
	m4 = _mm_adds_epi16(m4, m1);
	m5 = _mm_subs_epi16(m5, m1);

	m1 = _mm_max_epi16(m4, m5);
	m1 = _mm_subs_epi16(m1, m3);
	_mm_store_si128((__m128i *) bw, m1);

	/* L-values */
	m9  = _mm_set_epi8(LV_BW_SHUFFLE_MASK0);
	m10 = _mm_set_epi8(LV_BW_SHUFFLE_MASK1);
	m9  = _mm_shuffle_epi8(m0, m9);
	m10 = _mm_shuffle_epi8(m0, m10);

	m11 = _mm_adds_epi16(m6, m13);
	m12 = _mm_subs_epi16(m6, m13);
	m11 = _mm_adds_epi16(m11, m9);
	m12 = _mm_adds_epi16(m12, m10);

	/* Maximums */
#if defined(HAVE_SSE4_1) || defined(HAVE_SSE41)
	m13 = _mm_set1_epi16(32767);
	m14 = _mm_sub_epi16(m13, m11);
	m15 = _mm_sub_epi16(m13, m12);
	m14 = _mm_minpos_epu16(m14);
	m15 = _mm_minpos_epu16(m15);
	m14 = _mm_sub_epi16(m13, m14);
	m15 = _mm_sub_epi16(m13, m15);
#else
	MAXPOS(m11, m13, m14);
	MAXPOS(m12, m13, m15);
#endif
	m13 = _mm_sub_epi16(m15, m14);

	/* Return cast should truncate upper 16-bits */
	return _mm_cvtsi128_si32(m13);
}
#else
static inline int16_t gen_fw_metrics(int16_t *bm, int8_t x, int8_t z,
		       int16_t *sums_p, int16_t *sums_c, int16_t le)
{
	return 0;
}

static inline int16_t gen_bw_metrics(int16_t *bm, const int8_t z,
		       int16_t *fw, int16_t *bw, int16_t norm)
{
	return 0;
}
#endif /* HAVE_SSE3 */
