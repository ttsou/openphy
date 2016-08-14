/*
 * LTE rate matching for turbo encoded channels 
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

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#include "rate_match.h"

#define API_EXPORT	__attribute__((__visibility__("default")))

/*
 * Maximum block lengths
 * 3GPP TS 36.212 5.3 "Downlink transport channels and control information"
 */
#define C_TC_SUBBLOCK		32
#define MAX_D		(MAX_K + 4)
#define MAX_B_PR	(MAX_B + MAX_C * L_CRC)
#define MAX_V		6176
#define MAX_E           28800
#define MAX_W_NULL	93

/*
 * Rate matching for turbo coded transport channels (forward)
 *
 * 3GPP TS 35.212 Table 5.1.4-1
 *     "Inter-column permutation pattern for sub-block interleaver"
 */
static void permute_fw(signed char *in, signed char *out)
{
	out[ 0] = in[ 0];
	out[ 1] = in[16];
	out[ 2] = in[ 8];
	out[ 3] = in[24];
	out[ 4] = in[ 4];
	out[ 5] = in[20];
	out[ 6] = in[12];
	out[ 7] = in[28];
	out[ 8] = in[ 2];
	out[ 9] = in[18];
	out[10] = in[10];
	out[11] = in[26];
	out[12] = in[ 6];
	out[13] = in[22];
	out[14] = in[14];
	out[15] = in[30];
	out[16] = in[ 1];
	out[17] = in[17];
	out[18] = in[ 9];
	out[19] = in[25];
	out[20] = in[ 5];
	out[21] = in[21];
	out[22] = in[13];
	out[23] = in[29];
	out[24] = in[ 3];
	out[25] = in[19];
	out[26] = in[11];
	out[27] = in[27];
	out[28] = in[ 7];
	out[29] = in[23];
	out[30] = in[15];
	out[31] = in[31];
}

/*
 * Rate matching for turbo coded transport channels (reverse)
 *
 * 3GPP TS 35.212 Table 5.1.4-1
 *     "Inter-column permutation pattern for sub-block interleaver"
 */
static void permute_rv(signed char *in, signed char *out)
{
	out[ 0] = in[ 0];
	out[16] = in[ 1];
	out[ 8] = in[ 2];
	out[24] = in[ 3];
	out[ 4] = in[ 4];
	out[20] = in[ 5];
	out[12] = in[ 6];
	out[28] = in[ 7];
	out[ 2] = in[ 8];
	out[18] = in[ 9];
	out[10] = in[10];
	out[26] = in[11];
	out[ 6] = in[12];
	out[22] = in[13];
	out[14] = in[14];
	out[30] = in[15];
	out[ 1] = in[16];
	out[17] = in[17];
	out[ 9] = in[18];
	out[25] = in[19];
	out[ 5] = in[20];
	out[21] = in[21];
	out[13] = in[22];
	out[29] = in[23];
	out[ 3] = in[24];
	out[19] = in[25];
	out[11] = in[26];
	out[27] = in[27];
	out[ 7] = in[28];
	out[23] = in[29];
	out[15] = in[30];
	out[31] = in[31];
}

/*
 * 3GPP TS 36.212 Table 5.1.4-1
 *     "Inter-column permutation pattern for sub-block interleaver"
 */
static const int permute_table[32] = {
	 0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
	 1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31,
};

static int p_fcn(int i)
{
	if ((i < 0) || (i > 31))
		return -1;

	return permute_table[i];
}

/*
 * 3GPP TS 35.306 Table 4.1-1
 *     Downlink physical layer parameter values set by the field ue-Category
 */
#define N_SOFT_CAT1		250368
#define N_SOFT_CAT2		1237248
#define N_SOFT_CAT3		1237248
#define N_SOFT_CAT4		1827072
#define N_SOFT_CAT5		3667200
#define N_SOFT_CAT6		3667200
#define N_SOFT_CAT7		3667200
#define N_SOFT_CAT8		35982720

/*
 * UE category type
 */
#define UE_SOFT_CAT		N_SOFT_CAT3

/*
 * In most cases the soft buffer size will be equal to the circular buffer size,
 * K_w, which is 3 times the size of the sub-block interleaver output. For cases
 * of spatial multiplexing and/or large number of code blocks 'C', the soft
 * buffer size may be smaller than K_w.
 *
 * Handling these corner cases requires enabling Tx mode 3 or 4 at compile time.
 */
#if defined(TX_MODE_3) || defined(TX_MODE_4)
#define K_MIMO			2
#else
#define K_MIMO			1
#endif

/* 3GPP TS 35.212 5.1.4.1.2 "Bit collection, selection, and transmission" */
static void rate_match_fw(struct lte_rate_matcher *match,
			  signed char *e, int E, int rv)
{
	int i, n, m = 0;
	int V = match->V;
	int K_w = 3 * V;
	int w_len = 3 * V;
	int *w_null = match->w_null;

#if defined(TX_MODE_3) || defined(TX_MODE_4)
	int C = 1;
	int K_mimo = K_MIMO;
	int M_limit = 8;
	int M_dl_harq = 8;
	int N_soft = N_SOFT_CAT;
	int N_ir = N_soft / (K_mimo * MIN(M_dl_harq, M_limit));
	int N_cb = MIN(N_ir / C, K_w);
#else
	int N_cb = K_w;
#endif
	int R_TC_sb = match->rows;
	int k_0 = R_TC_sb * (2 * (int) ceilf(N_cb / (8.0f * R_TC_sb)) * rv + 2);

	/* For redundancy version 0 */
	signed char w[w_len];

	memcpy(w, match->v[0], V * sizeof(char));
	for (i = 0; i < V; i++) {
		w[V + 2 * i + 0] = match->v[1][i];
		w[V + 2 * i + 1] = match->v[2][i];
	}

	n = k_0;
	for (i = 0; i < E; n++) {
		n = n % w_len;

		if (w[n] == -128) {
			if (m < MAX_W_NULL)
				w_null[m++] = n;
			continue;
		}

		e[i++] = w[n];
	}

	match->w_null_cnt = m;
}

/* 3GPP TS 35.212 5.1.4.1.2 "Bit collection, selection, and transmission" */
static void rate_match_rv(struct lte_rate_matcher *match,
			  signed char *e, int E, int rv)
{
	int i, n, val;
	int V = match->V;
	int R_TC_sb = match->rows;
	int *w_null = match->w_null;
	int w_len = 3 * V;
	signed char *w;

	w = (signed char *) calloc(w_len, sizeof(char));

	for (i = 0; i < match->w_null_cnt; i++)
		w[w_null[i]] = -128;

	n = R_TC_sb * (2 * (int) ceilf(w_len / (8.0f * R_TC_sb)) * rv + 2);
	for (i = 0; i < E; n++) {
		n = n % w_len;

		if (w[n] == -128)
			continue;

		val = (int) w[n] + (int) e[i++];
#ifdef SATURATE
		if (val > SCHAR_MAX)
			val = SCHAR_MAX;
		else if (val < -SCHAR_MAX)
			val = -SCHAR_MAX;
#endif
		w[n] = val;
	}

	memcpy(match->v[0], w, V * sizeof(char));
	for (i = 0; i < V; i++) {
		match->v[1][i] = w[V + 2 * i + 0];
		match->v[2][i] = w[V + 2 * i + 1];
	}

	free(w);
}

static void scramble(signed char* d, int rows, signed char *v,
		     void (*perm)(signed char *, signed char *))
{
	int i;

	for (i = 0; i < rows; i++)
		perm(&d[i * C_TC_SUBBLOCK], &v[i * C_TC_SUBBLOCK]);
}

static void readout_rows(signed char *v, int rows)
{
	int i, n;
	signed char val[C_TC_SUBBLOCK * rows];

	for (i = 0; i < C_TC_SUBBLOCK; i++) {
		for (n = 0; n < rows; n++)
			val[rows * i + n] = v[C_TC_SUBBLOCK * n + i];
	}

	memcpy(v, val, C_TC_SUBBLOCK * rows * sizeof(char));
}

static void readin_rows(signed char *v, int rows)
{
	int i, n;
	signed char val[C_TC_SUBBLOCK * rows];

	for (i = 0; i < C_TC_SUBBLOCK; i++) {
		for (n = 0; n < rows; n++)
			val[C_TC_SUBBLOCK * n + i] = v[rows * i + n];
	}

	memcpy(v, val, C_TC_SUBBLOCK * rows * sizeof(char));
}

/*
 * Forward sub-block interleaving 
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
static int interlv(signed char *v, int len, signed char *z)
{
	int i, rows = (int) ceilf((float) len / 32.0f);
	int shift = rows * 32 - len;
	signed char d_shift[rows * 32];

	for (i = 0; i < rows * 32; i++) {
		if (i < shift)
			d_shift[i] = -128;
		else
			d_shift[i] = z[i - shift];
	}

	scramble(d_shift, rows, v, permute_fw);
	readout_rows(v, rows);

	return rows * 32;
}

/*
 * Sub-block interleaving (forward)
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
static int interlv_v2(struct lte_rate_matcher *match)
{
	int i, k, pi_k, len = match->D;
	int rows = match->rows;
	signed char *v, *z;
	int shift = rows * 32 - len;
	signed char d_shift[rows * 32];

	v = match->v[2];
	z = match->z[2];

	for (i = 0; i < rows * 32; i++) {
		if (i < shift)
			d_shift[i] = -128;
		else
			d_shift[i] = z[i - shift];
	}

	for (k = 0; k < rows * 32; k++) {
		pi_k = (p_fcn(k / rows) + 32 * (k % rows) + 1) % match->V;
		v[k] = d_shift[pi_k];
		match->pi[k] = pi_k;
	}

	return 0;
}

/*
 * Reverse sub-block interleaving 
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
static void deinterlv(signed char *v, int len, signed char *z, int rows)
{
	readin_rows(v, rows);
	scramble(v, rows, z, permute_rv);
}

static void deinterlv_v2(struct lte_rate_matcher *match)
{
	int k, len = match->V;
	int *pi;
	signed char *v, *z;

	v = match->v[2];
	z = match->z[2];
	pi = match->pi;

	for (k = 0; k < len; k++)
		z[pi[k]] = v[k];
}

static int rate_match_init_fw(struct lte_rate_matcher *match,
			      signed char **d, int D,
			      signed char *e, int E, int rv)
{
	int i, rows, shift, V;

	if ((rv < 0) || (rv > 3))
		return -EINVAL;

	rows = (int) ceil((float) D / (float) C_TC_SUBBLOCK);
	V = rows * C_TC_SUBBLOCK;

	/* Lengths */
	match->E = E;
	match->D = D;
	match->V = V;
	match->rows = rows;
	match->rv = rv;

	shift = V - D;
	if ((shift < 0) || (shift > 31))
		return -EINVAL;

	free(match->w_null);
	match->w_null = (int *) calloc(MAX_W_NULL, sizeof(int));

	free(match->pi);
	match->pi = (int *) calloc(V, sizeof(int));

	for (i = 0; i < 3; i++) {
		free(match->z[i]);
		match->z[i] = (signed char *) calloc(V, sizeof(char));

		if (d)
			memcpy(match->z[i], d[i], D * sizeof(char));

		free(match->v[i]);
		match->v[i] = (signed char *) malloc(V * sizeof(char));

		if (i == 2)
			interlv_v2(match);
		else
			interlv(match->v[i], D, match->z[i]);
	}

	rate_match_fw(match, e, E, rv);

	return 0;
}

static int rate_match_init_rv(struct lte_rate_matcher *match,
			      int D, int E, int rv)
{
	if ((D <= 0) || (E <= 0))
		return -1;

	signed char *e = calloc(E, sizeof(char));

	rate_match_init_fw(match, NULL, D, e, E, rv);

	free(e);

	return 0;
}

API_EXPORT
int lte_rate_match_rv(struct lte_rate_matcher *match,
		      struct lte_rate_matcher_io *io, int rv)
{
	int i, shift;

	if (!match || !io || (io->E < 1) || (io->E > MAX_E) || (io->D < 1))
		return -EINVAL;

	/* Reinitialize the rate matcher if the IO values don't match */
	if ((match->E != io->E) || (match->D != io->D) || (match->rv != rv)) {
		if (rate_match_init_rv(match, io->D, io->E, rv))
			return -EINVAL;
	}

	if (match->V % C_TC_SUBBLOCK)
		return -EINVAL;

	shift = match->V - match->D;
	if (shift < 0)
		return -EINVAL;

	rate_match_rv(match, io->e, io->E, rv);

	for (i = 0; i < 3; i++) {
		if (i == 2)
			deinterlv_v2(match);
		else
			deinterlv(match->v[i], match->V,
				  match->z[i], match->rows);

		memcpy(io->d[i], &match->z[i][shift],
		       match->D * sizeof(char));
	}

	return 0;
}

API_EXPORT
int lte_rate_match_fw(struct lte_rate_matcher *match,
		      struct lte_rate_matcher_io *io, int rv)
{
	if (!match || !io || (io->E < 1) || (io->E > MAX_E) || (io->D < 1))
		return -EINVAL;

	/* Reinitialize the rate matcher if the IO values don't match */
	if (rate_match_init_fw(match, io->d, io->D, io->e, io->E, rv))
		return -EINVAL;

	return 0;
}

API_EXPORT
struct lte_rate_matcher *lte_rate_matcher_alloc()
{
	struct lte_rate_matcher *match;

	match = (struct lte_rate_matcher *)
		calloc(1, sizeof(struct lte_rate_matcher));

	return match;
}

API_EXPORT
void lte_rate_matcher_free(struct lte_rate_matcher *match)
{
	int i;

	if (!match)
		return;

	for (i = 0; i < 3; i++) {
		free(match->z[i]);
		free(match->v[i]);
	}

	free(match->w_null);
	free(match->pi);
	free(match);
}
