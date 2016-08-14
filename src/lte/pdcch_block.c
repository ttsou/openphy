/*
 * LTE Physical Downlink Control Channel (PDCCH) Block Processing
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
#include <errno.h>
#include <../turbo/conv.h>
#include <../turbo/rate_match.h>

#include "pdcch_block.h"
#include "crc.h"
#include "log.h"

#define L_CRC16		16
#define MAX_A		51
#define MAX_K		(MAX_A + L_CRC16)
#define MAX_D		MAX_K
#define MAX_E		576

/* 3GPP TS 36.212 Release 8: 5.3.3 "Downlink control information" */
struct lte_pdcch_blk {
	uint8_t *a;
	int A;

	uint8_t c[MAX_K];
	int K;

	int8_t d[3][MAX_D];
	int D;

	int8_t e[MAX_E];
	int E;

	struct lte_rate_matcher *match;
};

/* Allocate block processing chain */
struct lte_pdcch_blk *lte_pdcch_blk_alloc()
{
	struct lte_pdcch_blk *dblk;

	dblk = (struct lte_pdcch_blk *) calloc(1, sizeof(struct lte_pdcch_blk));
	dblk->match = lte_rate_matcher_alloc();

	return dblk;
}

/* Release block processing chain */
void lte_pdcch_blk_free(struct lte_pdcch_blk *dblk)
{
	if (!dblk)
		return;

	lte_rate_matcher_free(dblk->match);
	free(dblk);

	dblk = NULL;
}

/* 3GPP TS 36.212 Release 8: 5.3.3 "Downlink control information" */
int lte_pdcch_blk_init(struct lte_pdcch_blk *dblk, int A, int E)
{
	if ((A < 0) || (A > MAX_A) || (E < 0) || (E > MAX_E)) {
		fprintf(stderr, "PDCCH: Invalid rate match size %i\n", E);
		return -1;
	}

	dblk->A = A;
	dblk->K = A + L_CRC16;
	dblk->D = dblk->K;
	dblk->E = E;

	dblk->a = dblk->c;

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.3 "Channel coding" */
static int lte_pdcch_blk_chan_decode(struct lte_pdcch_blk *dblk)
{
	struct lte_conv_code code = {
		.n = 3,
		.k = 7,
		.len = dblk->K,
		.gen = { 0133, 0171, 0165 },
		.rgen = 0,
		.punc = NULL,
		.term = CONV_TERM_TAIL_BITING,
        };

	int8_t d[dblk->D * 3];

	for (int i = 0; i < dblk->D; i++) {
		d[3 * i + 0] = dblk->d[0][i];
		d[3 * i + 1] = dblk->d[1][i];
		d[3 * i + 2] = dblk->d[2][i];
        }

	lte_conv_decode(&code, d, dblk->c);

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.4 "Rate matching" */
static int lte_pdcch_blk_rate_unmatch(struct lte_pdcch_blk *dblk)
{
	struct lte_rate_matcher_io io = {
		.D = dblk->D,
		.E = dblk->E,
		.d = { dblk->d[0], dblk->d[1], dblk->d[2] },
		.e = dblk->e,
	};

	if (lte_conv_rate_match_rv(dblk->match, &io)) {
		fprintf(stderr, "Block: Rate matcher failed to initialize\n");
		return -1;
	}

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.2 "CRC attachment" */
static int lte_pdcch_blk_crc(struct lte_pdcch_blk *dblk, uint16_t rnti)
{
	uint16_t reg0 = lte_crc16_gen(dblk->c, dblk->A);
	uint16_t reg1 = lte_crc16_pack(&dblk->c[dblk->A], L_CRC16);

	if (reg0 == (reg1 ^ rnti))
		return 1;

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.3 "Downlink control information"
 *
 * Execute block processing chain
 */
int lte_pdcch_blk_decode(struct lte_pdcch_blk *dblk, uint16_t rnti)
{
	if (lte_pdcch_blk_rate_unmatch(dblk))
		return -1;
	if (lte_pdcch_blk_chan_decode(dblk))
		return -1;

	return lte_pdcch_blk_crc(dblk, rnti);
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 *
 * Return concatenated 'f' buffer of length 'G'
 */
uint8_t *lte_pdcch_blk_abuf(struct lte_pdcch_blk *dblk, int len)
{
	if (len != dblk->A) {
		fprintf(stderr, "Block: Invalid block size %i\n", len);
		fprintf(stderr, "       Expected size %i\n", dblk->E);
		return NULL;
	}

	return dblk->a;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 *
 * Return concatenated 'f' buffer of length 'G'
 */
int8_t *lte_pdcch_blk_ebuf(struct lte_pdcch_blk *dblk, int len)
{
	if (len != dblk->E) {
		fprintf(stderr, "Block: Invalid block size %i\n", len);
		fprintf(stderr, "       Expected size %i\n", dblk->E);
		return NULL;
	}

	return dblk->e;
}
