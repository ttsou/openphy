/*
 * LTE Physical Broadcast Channel (PBCH) Block Processing
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

#include "pbch_block.h"
#include "crc.h"
#include "log.h"

#define MAX_E		1920
#define PBCH_D		PBCH_K

/* 3GPP TS 36.212 Release 8: 5.3.1 "Broadcast channel" */
struct lte_pbch_blk {
	uint8_t *a;
	uint8_t c[PBCH_K];

	int8_t d[3][PBCH_D];
	int8_t e[MAX_E];
	int E;

	struct lte_rate_matcher *match;
};

/* Allocate transport block processing chain */
struct lte_pbch_blk *lte_pbch_blk_alloc()
{
	struct lte_pbch_blk *cblk;

	cblk = (struct lte_pbch_blk *) calloc(1, sizeof(struct lte_pbch_blk));
	cblk->match = lte_rate_matcher_alloc();

	return cblk;
}

/* Release transport block processing chain */
void lte_pbch_blk_free(struct lte_pbch_blk *cblk)
{
	if (!cblk)
		return;

	lte_rate_matcher_free(cblk->match);
	cblk = NULL;
}

/* 3GPP TS 36.212 Release 8: 5.3.1 "Broadcast channel" */
int lte_pbch_blk_init(struct lte_pbch_blk *cblk, int E)
{
	if ((E < 0) || (E > MAX_E)) {
		fprintf(stderr, "PBCH: Invalid rate match size %i\n", E);
		return -1;
	}

	cblk->a = cblk->c;
	cblk->E = E;

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.1.2 "Channel coding" */
static int lte_pbch_blk_chan_decode(struct lte_pbch_blk *cblk)
{
	struct lte_conv_code code = {
		.n = 3,
		.k = 7,
		.len = PBCH_K,
		.gen = { 0133, 0171, 0165 },
		.rgen = 0,
		.punc = NULL,
		.term = CONV_TERM_TAIL_BITING,
        };

	int8_t d[PBCH_D * 3];

	for (int i = 0; i < PBCH_D; i++) {
		d[3 * i + 0] = cblk->d[0][i];
		d[3 * i + 1] = cblk->d[1][i];
		d[3 * i + 2] = cblk->d[2][i];
        }

	lte_conv_decode(&code, d, cblk->c);

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.1.3 "Rate matching" */
static int lte_pbch_blk_rate_unmatch(struct lte_pbch_blk *cblk)
{
	struct lte_rate_matcher_io io = {
		.D = PBCH_D,
		.E = cblk->E,
		.d = { cblk->d[0], cblk->d[1], cblk->d[2] },
		.e = cblk->e,
	};

	if (lte_conv_rate_match_rv(cblk->match, &io)) {
		fprintf(stderr, "Block: Rate matcher failed to initialize\n");
		return -1;
	}

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.1.1 "Transport block CRC attachment" */
static int lte_pbch_blk_crc(struct lte_pbch_blk *cblk)
{
	uint16_t mask_1tx = 0x0000;
	uint16_t mask_2tx = 0xffff;
	uint16_t mask_4tx = 0x5555;

	uint16_t reg0 = lte_crc16_gen(cblk->c, PBCH_A);
	uint16_t reg1 = lte_crc16_pack(&cblk->c[PBCH_A], L_CRC16);

	if (reg0 == (reg1 ^ mask_1tx))
		return 1;
	if (reg0 == (reg1 ^ mask_2tx))
		return 2;
	if (reg0 == (reg1 ^ mask_4tx))
		return 4;

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.1 "Broadcast channel"
 *
 * Execute block decode processing chain
 */
int lte_pbch_blk_decode(struct lte_pbch_blk *cblk)
{
	if (lte_pbch_blk_rate_unmatch(cblk))
		return -1;
	if (lte_pbch_blk_chan_decode(cblk))
		return -1;

	return lte_pbch_blk_crc(cblk);
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 *
 * Return concatenated 'f' buffer of length 'G'
 */
uint8_t *lte_pbch_blk_abuf(struct lte_pbch_blk *cblk, int len)
{
	if (len != PBCH_A) {
		fprintf(stderr, "Block: Invalid block size %i\n", len);
		fprintf(stderr, "       Expected size %i\n", cblk->E);
		return NULL;
	}

	return cblk->a;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.2.5 "Code block concatentation"
 *
 * Return concatenated 'f' buffer of length 'G'
 */
int8_t *lte_pbch_blk_ebuf(struct lte_pbch_blk *cblk, int len)
{
	if (len != cblk->E) {
		fprintf(stderr, "Block: Invalid block size %i\n", len);
		fprintf(stderr, "       Expected size %i\n", cblk->E);
		return NULL;
	}

	return cblk->e;
}
