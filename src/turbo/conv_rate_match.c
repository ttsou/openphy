/*
 * LTE rate matching for convolutionally encoded channels 
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "rate_match.h"

#define API_EXPORT	__attribute__((__visibility__("default")))

/*
 * These maximums are carried over from the turbo rate matcher
 * and may be overkill for the convolutional rate matcher.
 */
#define C_TC_SUBBLOCK		32
#define MAX_D		(MAX_K + 4)
#define MAX_B_PR	(MAX_B + MAX_C * L_CRC)
#define MAX_V		6176
#define MAX_E           28800
#define MAX_W_NULL	93

/*
 * Forward sub-block column permutation
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
static void permute_fw(signed char *in, signed char *out)
{
	out[ 0] = in[ 1];
	out[ 1] = in[17];
	out[ 2] = in[ 9];
	out[ 3] = in[25];
	out[ 4] = in[ 5];
	out[ 5] = in[21];
	out[ 6] = in[13];
	out[ 7] = in[29];
	out[ 8] = in[ 3];
	out[ 9] = in[19];
	out[10] = in[11];
	out[11] = in[27];
	out[12] = in[ 7];
	out[13] = in[23];
	out[14] = in[15];
	out[15] = in[31];
	out[16] = in[ 0];
	out[17] = in[16];
	out[18] = in[ 8];
	out[19] = in[24];
	out[20] = in[ 4];
	out[21] = in[20];
	out[22] = in[12];
	out[23] = in[28];
	out[24] = in[ 2];
	out[25] = in[18];
	out[26] = in[10];
	out[27] = in[26];
	out[28] = in[ 6];
	out[29] = in[22];
	out[30] = in[14];
	out[31] = in[30];
}

/*
 * Reverse sub-block column permutation
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
static void permute_rv(signed char *in, signed char *out)
{
	out[ 1] = in[ 0];
	out[17] = in[ 1];
	out[ 9] = in[ 2];
	out[25] = in[ 3];
	out[ 5] = in[ 4];
	out[21] = in[ 5];
	out[13] = in[ 6];
	out[29] = in[ 7];
	out[ 3] = in[ 8];
	out[19] = in[ 9];
	out[11] = in[10];
	out[27] = in[11];
	out[ 7] = in[12];
	out[23] = in[13];
	out[15] = in[14];
	out[31] = in[15];
	out[ 0] = in[16];
	out[16] = in[17];
	out[ 8] = in[18];
	out[24] = in[19];
	out[ 4] = in[20];
	out[20] = in[21];
	out[12] = in[22];
	out[28] = in[23];
	out[ 2] = in[24];
	out[18] = in[25];
	out[10] = in[26];
	out[26] = in[27];
	out[ 6] = in[28];
	out[22] = in[29];
	out[14] = in[30];
	out[30] = in[31];
}

/* 3GPP TS 35.212 5.1.4.2.2 "Bit collection, selection, and transmission" */
static void rate_match_fw(struct lte_rate_matcher *match, signed char *e, int E)
{
	int i, n = 0, m = 0;
	int V = match->V;
	int *w_null = match->w_null;

	int w_len = V * 3;
	signed char w[w_len];

	for (i = 0; i < 3; i++)
		memcpy(&w[i * V], match->v[i], V * sizeof(char));

	for (i = 0; i < E; n++) {
		n = n % w_len;

		if (w[n] == -128) {
			if (m < MAX_W_NULL)
				w_null[m++] = n;
		} else {
			e[i++] = w[n];
		}
	}

	match->w_null_cnt = m;
}

/* 3GPP TS 35.212 5.1.4.2.2 "Bit collection, selection, and transmission" */
static void rate_match_rv(struct lte_rate_matcher *match, signed char *e, int E)
{
	int i, n = 0;
	int V = match->V;
	int *w_null = match->w_null;

	int w_len = V * 3;
	signed char *w = (signed char *) calloc(w_len, 1);

	for (i = 0; i < match->w_null_cnt; i++)
		w[w_null[i]] = -128;

	for (i = 0; i < E; n++) {
		n = n % w_len;

		if (w[n] == -128)
			continue;

		w[n] += e[i++];
	}

	for (i = 0; i < 3; i++)
		memcpy(match->v[i], &w[i * V], V * sizeof(char));

	free(w);
}

static void lte_conv_scramble(signed char *d, int rows, signed char *v,
			      void (*perm)(signed char *, signed char *))
{
	int i;

	for (i = 0; i < rows; i++)
		perm(&d[i * 32], &v[i * 32]);
}

static void readout_rows(signed char *v, int rows)
{
	int i, n;
	signed char val[32 * rows];

	for (i = 0; i < 32; i++) {
		for (n = 0; n < rows; n++)
			val[rows * i + n] = v[32 * n + i];
	}

	memcpy(v, val, 32 * rows * sizeof(char));
}

static void readin_rows(signed char *v, int rows)
{
	int i, n;
	signed char val[32 * rows];

	for (i = 0; i < 32; i++) {
		for (n = 0; n < rows; n++)
			val[32 * n + i] = v[rows * i + n];
	}

	memcpy(v, val, 32 * rows * sizeof(char));
}

/*
 * Forward sub-block interleaving
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
int interlv(signed char *v, int len, signed char *d)
{
	int i, rows = (int) ceil((float) len / 32.0);
	int shift = rows * 32 - len;
	signed char d_shift[rows * 32];

	for (i = 0; i < rows * 32; i++) {
		if (i < shift)
			d_shift[i] = -128;
		else
			d_shift[i] = d[i - shift];
	}

	lte_conv_scramble(d_shift, rows, v, permute_fw);
	readout_rows(v, rows);

	return rows * 32;
}

/*
 * Reverse sub-block interleaving 
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
static int deinterlv(signed char *v, int len, signed char *d, int rows)
{
	readin_rows(v, rows);
	lte_conv_scramble(v, rows, d, permute_rv);

	return 0;
}

static int rate_match_init_fw(struct lte_rate_matcher *match,
			      signed char **d, int D, signed char *e, int E)
{
	int i, rows, shift, V;

	if ((E < 1) || (E > MAX_E))
		return -EINVAL;

	rows = (int) ceil((float) D / (float) 32);
	V = rows * 32;

	/* Lengths */
	match->E = E;
	match->D = D;
	match->V = V;
	match->rows = rows;

	shift = V - D;
	if ((shift < 0) || (shift > 31))
		return -EINVAL;

	free(match->w_null);
	match->w_null = (int *) calloc(MAX_W_NULL, sizeof(int));

	for (i = 0; i < 3; i++) {
		free(match->z[i]);
		match->z[i] = (signed char *) calloc(V, sizeof(char));

		if (d)
			memcpy(match->z[i], d[i], D * sizeof(char));

		free(match->v[i]);
		match->v[i] = (signed char *) malloc(V * sizeof(char));

		interlv(match->v[i], D, match->z[i]);
	}

	rate_match_fw(match, e, E);

	return 0;
}

static int rate_match_init_rv(struct lte_rate_matcher *match, int D, int E)
{
	if ((D <= 0) || (E <= 0))
		return -1;

	signed char *e = calloc(E, sizeof(char));

	rate_match_init_fw(match, NULL, D, e, E);

	free(e);

	return 0;
}

API_EXPORT
int lte_conv_rate_match_rv(struct lte_rate_matcher *match,
			   struct lte_rate_matcher_io *io)
{
	int i, shift;

	if (!match || !io)
		return -EFAULT;

	if ((match->E != io->E) || (match->D != io->D)) {
		if (rate_match_init_rv(match, io->D, io->E))
			return -1;
	}

	if (match->V % 32)
		return -EINVAL;

	shift = match->V - match->D;
	if ((shift < 0) || (shift > 31))
		return -EINVAL;

	rate_match_rv(match, io->e, io->E);

	for (i = 0; i < 3; i++) {
		deinterlv(match->v[i], match->V,
			  match->z[i], match->rows);

		memcpy(io->d[i], &match->z[i][shift],
		       match->D * sizeof(char));
	}

	return 0;
}

API_EXPORT
int lte_conv_rate_match_fw(struct lte_rate_matcher *match,
			   struct lte_rate_matcher_io *io)
{
	if (!match || !io)
		return -EFAULT;

	if (rate_match_init_fw(match, io->d, io->D, io->e, io->E))
		return -1;

	return 0;
}
