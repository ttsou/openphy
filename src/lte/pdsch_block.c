/*
 * LTE Physical Downlink Shared Channel (PDSCH) Transport Block Processing
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

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <../turbo/turbo.h>
#include <../turbo/rate_match.h>

#include "pdsch_block.h"
#include "crc.h"
#include "log.h"

#define MAX_I		188

/*
 * 3GPP TS 36.212 Release 8: Table 5.3.3-3
 * "Turbo code internal interleaver parameters"
 */
static int lte_interlv_k[MAX_I] = {
	  40,   48,   56,   64,   72,   80,   88,   96,
	 104,  112,  120,  128,  136,  144,  152,  160,
	 168,  176,  184,  192,  200,  208,  216,  224,
	 232,  240,  248,  256,  264,  272,  280,  288,
	 296,  304,  312,  320,  328,  336,  344,  352,
	 360,  368,  376,  384,  392,  400,  408,  416,
	 424,  432,  440,  448,  456,  464,  472,  480,
	 488,  496,  504,  512,  528,  544,  560,  576,
	 592,  608,  624,  640,  656,  672,  688,  704,
	 720,  736,  752,  768,  784,  800,  816,  832,
	 848,  864,  880,  896,  912,  928,  944,  960,
	 976,  992, 1008, 1024, 1056, 1088, 1120, 1152,
	1184, 1216, 1248, 1280, 1312, 1344, 1376, 1408,
	1440, 1472, 1504, 1536, 1568, 1600, 1632, 1664,
	1696, 1728, 1760, 1792, 1824, 1856, 1888, 1920,
	1952, 1984, 2016, 2048, 2112, 2176, 2240, 2304,
	2368, 2432, 2496, 2560, 2624, 2688, 2752, 2816,
	2880, 2944, 3008, 3072, 3136, 3200, 3264, 3328,
	3392, 3456, 3520, 3584, 3648, 3712, 3776, 3840,
	3904, 3968, 4032, 4096, 4160, 4224, 4288, 4352,
	4416, 4480, 4544, 4608, 4672, 4736, 4800, 4864,
	4928, 4992, 5056, 5120, 5184, 5248, 5312, 5376,
	5440, 5504, 5568, 5632, 5696, 5760, 5824, 5888,
	5952, 6016, 6080, 6144,
};

/*
 * 3GPP TS 36.212 Release 8: 5.1.2 "Code block segmentation"
 *     Z_MAX - Maximum code block size
 *     L_CRC - Segment CRC length
 */
#define Z_MAX		6144
#define L_CRC		24

/*
 * Min and max transport block and segment sizes. These sizes
 * correspond to single layer limits only.
 */
#define MIN_A		16
#define MAX_A		75376
#define MAX_B		(MAX_A + L_CRC)
#define MAX_C		13
#define MAX_K		Z_MAX
#define MAX_D		(MAX_K + 4)
#define MAX_B_PR	(MAX_B + MAX_C * L_CRC)
#define MAX_E		28800
#define MAX_G		86400

/* 3GPP TS 36.212 Release 8: 5.3.2 "Downlink shared channel" */
struct lte_pdsch_blk {
	uint8_t *a;
	int A;

	uint8_t *b;
	int B;

	uint8_t c[MAX_K / 8];
	int K[MAX_C];
	int C;
	int F;

	int8_t d[3][MAX_D];
	int D[MAX_C];

	int8_t *e[MAX_C];
	int E[MAX_C];
	int N_l;
	int Q_m;

	int8_t *f;
	int G;

	struct tdecoder *tdec;
	struct lte_rate_matcher *match;
};

/*
 * Allocate transport block processing chain
 *
 * Maintain a single turbo decoder instance throughout the lifecycle of the
 * PDSCH block. Constituent buffers are allocated and released upon
 * initialization and reinitialization.
 */
struct lte_pdsch_blk *lte_pdsch_blk_alloc()
{
	struct lte_pdsch_blk *tblk;

	tblk = (struct lte_pdsch_blk *) calloc(1, sizeof(struct lte_pdsch_blk));
	tblk->match = lte_rate_matcher_alloc();
	tblk->tdec = alloc_tdec();

	return tblk;
}

/* Release transport block processing chain */
void lte_pdsch_blk_free(struct lte_pdsch_blk *tblk)
{
	if (!tblk)
		return;

	free_tdec(tblk->tdec);
	lte_rate_matcher_free(tblk->match);
	free(tblk->b);
	free(tblk->f);
	free(tblk);

	tblk = NULL;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.1 "Transport block CRC attachment"
 *
 * The 'a' sequence is the 'b' sequence without the CRC attached. Perform
 * bounds checking and let the 'b' sequence handle buffer management.
 */
static int lte_pdsch_blk_init_a(struct lte_pdsch_blk *tblk, int A)
{
	if ((A < MIN_A) || (A > MAX_A))
		return -1;

	tblk->a = NULL;
	tblk->A = A;

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.1 "Transport block CRC attachment"
 */
static int lte_pdsch_blk_init_b(struct lte_pdsch_blk *tblk)
{
	tblk->B = tblk->A + L_CRC;

	if (tblk->B % 8)
		return -1;

	free(tblk->b);
	tblk->b = malloc(tblk->B / 8 * sizeof *tblk->b);
	tblk->a = tblk->b;

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.1.2
 * "Code block segmentation and code block CRC attachment"
 */
static int lte_pdsch_blk_init_c(struct lte_pdsch_blk *tblk)
{
	int i, r, C, B_pr, F, K_pl, K_mi, K_dl, C_mi, C_pl, L;
	int B = tblk->B;
	int Z = Z_MAX;

	if (B <= Z) {
		L = 0;
		C = 1;
		B_pr = B;
	} else {
		L = 24;
		C = ceilf((float) B / (float) (Z - L));
		B_pr = B + C * L;
	}

	for (i = 0; i < MAX_I; i++) {
		if (C * lte_interlv_k[i] >= B_pr) {
			K_pl = lte_interlv_k[i];
			break;
		}
	}

	/* Smallest block size with segmentation is probably wrong */
	if (((C > 1) && !i) || (i == MAX_I)) {
		LOG_PDSCH_ERR("Block: Invalid segment block");
		return -1;
	}

	if (C == 1) {
		C_pl = 1;
		K_mi = 0;
		C_mi = 0;
	} else {
		K_mi = lte_interlv_k[i - 1];
		K_dl = K_pl - K_mi;

		C_mi = (C * K_pl - B_pr) / K_dl;
		C_pl = C - C_mi;
	}

	F = C_pl * K_pl + C_mi * K_mi - B_pr;
	if (F < 0) {
		LOG_PDSCH_ERR("Block: Invalid filler");
		return -1;
	}

	/* Segmented code block */
	tblk->C = C;
	tblk->F = F;

	/* Is it possible to have C > 1 with filler bits? */
	if ((C == 1) && (!F)) {
		tblk->K[0] = B;
		return 0;
	} else if (C == 1) {
		fprintf(stderr, "Block: Filler bits on single block? %i\n", F);
		return -1;
	}

	for (r = 0; r < C; r++) {
		if (r < C_mi)
			tblk->K[r] = K_mi;
		else
			tblk->K[r] = K_pl;
	}

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.2.4 "Channel coding */
static int lte_pdsch_blk_init_d(struct lte_pdsch_blk *tblk)
{
	int r;

	for (r = 0; r < tblk->C; r++)
		tblk->D[r] = tblk->K[r] + 4;

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 *
 * This is the full buffer that maps bits of modulated symbols. The 'f' buffer
 * is segmented into 'e' buffers and consequently must be allocated before 'e'
 * buffers are assigned.
 */
static int lte_pdsch_blk_init_f(struct lte_pdsch_blk *tblk, int G)
{
	if ((G < 1) || (G > MAX_G))
		return -1;

	free(tblk->f);
	tblk->f = malloc(G * sizeof *tblk->f);
	tblk->G = G;

	return 0;
}

/*
 * Segment 'f' buffer into 'C' segments.
 * 3GPP TS 36.212 Release 8: 5.1.4 "Rate Matching"
 *                           5.1.2 "Code block segmentation"
 */
static int lte_pdsch_blk_init_e(struct lte_pdsch_blk *tblk, int N_l, int Q_m)
{
	int r, G_pr, gamma;
	int *E = tblk->E;
	int C = tblk->C;
	int G = tblk->G;
	int8_t *f = tblk->f;

	if ((N_l != 1) && (N_l != 2))
		return -1;
	if ((Q_m != 2) && (Q_m != 4) && (Q_m != 6) && (Q_m != 8))
		return -1;

	G_pr = G / (N_l * Q_m);
	gamma = G_pr % C;

	for (r = 0; r < C; r++) {
		if (r <= C - gamma - 1)
			E[r] = N_l * Q_m * (G_pr / C);
		else
			E[r] = N_l * Q_m * (int) ceilf((float) G_pr / (float) C);
	}

	/* Check if our segments match the total number of bits */
	for (r = 0; r < C; r++)
		G -= E[r];
	if (G)
		return -1;

	for (r = 0; r < C; r++) {
		tblk->e[r] = f;
		f += tblk->E[r];
	}

	tblk->N_l = N_l;
	tblk->Q_m = Q_m;

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.2 "Downlink shared channel" */
int lte_pdsch_blk_init(struct lte_pdsch_blk *tblk,
		       int A, int G, int N_l, int Q_m)
{
	if ((tblk->A == A) && (tblk->G == G) &&
	    (tblk->N_l == N_l) && (tblk->Q_m == Q_m))
		return 0;

	if (lte_pdsch_blk_init_a(tblk, A) || lte_pdsch_blk_init_b(tblk)) {
		fprintf(stderr, "Block: Bad transport block size %i\n", A);
		return -1;
	}

	if (lte_pdsch_blk_init_c(tblk)) {
		fprintf(stderr, "Block: Segmentation failure %i\n", A);
		return -1;
	}

	lte_pdsch_blk_init_d(tblk);

	if (lte_pdsch_blk_init_f(tblk, G)) {
		fprintf(stderr, "Block: Invalid number of bits %i\n", G);
		return -1;
	}

	if (lte_pdsch_blk_init_e(tblk, N_l, Q_m)) {
		fprintf(stderr, "Block: Invalid rate matcher %i\n", G);
		exit(1);
		return -1;
	}

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.2.3 "Channel coding" */
static int lte_pdsch_blk_chan_decode(struct lte_pdsch_blk *tblk, int r)
{
	int iter = 8;

	if (r >= tblk->C) {
		fprintf(stderr, "Block: Invalid segment %i\n", r);
		return -1;
	}

	LOG_PDSCH_ARG("    Turbo decode length K=", tblk->K[r]);

	lte_turbo_decode(tblk->tdec, tblk->K[r], iter, tblk->c,
			 tblk->d[0], tblk->d[1], tblk->d[2]);

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.2.4 "Rate matching" */
static int lte_pdsch_blk_rate_unmatch(struct lte_pdsch_blk *tblk, int r, int rv)
{
	if (r >= tblk->C) {
		fprintf(stderr, "Block: Invalid segment %i\n", r);
		return -1;
	}

	struct lte_rate_matcher_io io = {
		.D = tblk->D[r],
		.E = tblk->E[r],
		.d = { tblk->d[0], tblk->d[1], tblk->d[2] },
		.e = tblk->e[r],
	};

	LOG_PDSCH_ARG("    Rate match length D=", io.D);

	if (lte_rate_match_rv(tblk->match, &io, rv)) {
		fprintf(stderr, "Block: Rate matcher failed to initialize\n");
		return -1;
	}

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.2
 * "Code block segmentation and code block CRC attachment"
 */
static int lte_pdsch_blk_segment_crc(struct lte_pdsch_blk *tblk, int r)
{
	int bytes;

	if (r >= tblk->C) {
		fprintf(stderr, "Block: Invalid segment %i\n", r);
		return -1;
	}

	if (tblk->C == 1)
		return 0;

	bytes = (tblk->K[r] - L_CRC) / 8;

	if (lte_crc24b_chk(tblk->c, bytes, &tblk->c[bytes], L_CRC / 8) != 1) {
		LOG_PDSCH_ERR("Transport block segment failed CRC");
		return -1;
	}

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.2.1 "Transport block CRC attachment" */
static int lte_pdsch_blk_crc(struct lte_pdsch_blk *tblk)
{
	int bytes = tblk->A / 8;

	if (lte_crc24a_chk(tblk->b, bytes, &tblk->b[bytes], L_CRC / 8) != 1) {
		LOG_PDSCH_ERR("Transport block failed CRC");
		return -1;
	}

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.2
 * "Code block segmentation and code block CRC attachment"
 */
static int lte_pdsch_blk_combine(struct lte_pdsch_blk *tblk, int r)
{
	int i, bytes, wr = 0;

	if (r >= tblk->C) {
		fprintf(stderr, "Block: Invalid segment %i\n", r);
		return -1;
	}

	if (tblk->C == 1) {
		bytes = tblk->K[r] / 8;
		memcpy(tblk->b, tblk->c, bytes);
		return 0;
	}

	for (i = 0; i < r; i++)
		wr += (tblk->K[i] - L_CRC) / 8;

	if ((wr < 0) || (wr >= MAX_B / 8)) {
		LOG_PDSCH_ERR("Block: Invalid segment write pointer");
		return -1;
	}

	bytes = (tblk->K[r] - L_CRC) / 8;
	memcpy(&tblk->b[wr], tblk->c, bytes);

	return 0;
}

static int log_tblk(struct lte_pdsch_blk *tblk)
{
	LOG_PDSCH("Decoded transport block:");

	fprintf(stdout, "%s                     ", LOG_COLOR_DATA);
	for (int i = 0; i < tblk->A / 8; i++) {
		if (i && !(i % 20))
			printf("\n                     ");
		printf("%02x ", tblk->a[i]);
	}
	fprintf(stdout, "%s\n", LOG_COLOR_NONE);

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2 "Downlink shared channel"
 *
 * Execute block decode processing chain consisting of rate unmatch, turbo
 * decode, segmentation combining, and segment/block CRC checks.
 */
int lte_pdsch_blk_decode(struct lte_pdsch_blk *tblk, int rv)
{
	int r;

	for (r = 0; r < tblk->C; r++) {
		LOG_PDSCH_ARG("CRC Segmentation block r=", r);
		LOG_PDSCH_ARG("    Rate match length E=", tblk->E[r]);

		if (lte_pdsch_blk_rate_unmatch(tblk, r, rv))
			break;
		if (lte_pdsch_blk_chan_decode(tblk, r))
			break;
		if (lte_pdsch_blk_segment_crc(tblk, r))
			break;
		if (lte_pdsch_blk_combine(tblk, r))
			break;
	}

	if (r < tblk->C) {
		LOG_PDSCH_ERR("Block: Failed to decode");
		return -1;
	}

	/* Final block CRC check */
	if (lte_pdsch_blk_crc(tblk) < 0)
		return -1;

	log_tblk(tblk);

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 * Return concatenated 'f' buffer of length 'G'
 */
int8_t *lte_pdsch_blk_fbuf(struct lte_pdsch_blk *tblk, int *len)
{
	if (len)
		*len = tblk->G;

	return tblk->f;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 * Return 'a' transport block buffer of length 'A' (without CRC)
 */
uint8_t *lte_pdsch_blk_abuf(struct lte_pdsch_blk *tblk, int *len)
{
	if (len)
		*len = tblk->A;

	return tblk->a;
}
