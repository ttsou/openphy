/*
 * LTE Physical Broadcast Channel (PBCH)
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "lte.h"
#include "sigproc.h"
#include "crc.h"
#include "pbch.h"
#include "ofdm.h"
#include "scramble.h"
#include "precode.h"
#include "qam.h"
#include "si.h"
#include "subframe.h"
#include "pbch_block.h"
#include "log.h"
#include "sigvec_internal.h"

#define LTE_PBCH_NUM_RB		6
#define LTE_RB_LEN		12

struct pbch_sym {
	struct lte_sym *sym;
	struct cxvec *data;
};

struct pbch_slot {
	struct lte_slot *slot;
	struct pbch_sym syms[4];

	struct {
		signed char d[480];
		signed char d_uns[480];
		struct cxvec *e;
	} mib;
};

static int pbch_sym_init(struct pbch_sym *sym, struct lte_sym *lsym,
			 struct cxvec *e, int idx, int len)
{
	if (lsym->l >= 4) {
		LOG_PBCH_ERR("Invalid symbol");
		return -1;
	}

	sym->sym = lsym;
	sym->data = cxvec_subvec(e, idx, 0, 0, len);

	return 0;
}

static struct pbch_slot *pbch_slot_alloc(struct lte_slot *slot)
{
	int len, idx = 0;
	struct pbch_slot *pbch;

	if (slot->rbs != LTE_PBCH_NUM_RB) {
		printf("slot->rbs %i\n", slot->rbs);
		LOG_PBCH_ERR("Invalid number of resouce blocks");
		return NULL;
	}

	pbch = malloc(sizeof *pbch);
	pbch->slot = slot;
	pbch->mib.e = cxvec_alloc_simple(240);

	/* PBCH is 4 symbols long */
	for (int i = 0; i < 4; i++) {
		if (i < 2)
			len = 48;
		else
			len = 72;

		pbch_sym_init(&pbch->syms[i],
			      &pbch->slot->syms[i],
			      pbch->mib.e, idx, len);
		idx += len;
	}

	return pbch;
}

static void pbch_sym_free(struct pbch_sym *sym)
{
	cxvec_free(sym->data);
}

static void free_pbch_slot(struct pbch_slot *pbch)
{
	for (int i = 0; i < 4; i++)
		pbch_sym_free(&pbch->syms[i]);

	cxvec_free(pbch->mib.e);
	free(pbch);
}

static int pbch_extract_simple(struct pbch_slot **pbch, int l, int chans)
{
	int idx = 0;
	struct pbch_sym *sym[chans];

	if ((l != 2) && (l != 3)) {
		LOG_PBCH_ERR("Invalid symbol");
		return -1;
	}

	for (int i = 0; i < chans; i++)
		sym[i] = &pbch[i]->syms[l];

	for (int i = 0; i < LTE_PBCH_NUM_RB; i++) {
		for (int n = 0; n < LTE_RB_LEN / 2; n++) {
			if (chans == 2) {
				lte_unprecode_2x2(sym[0]->sym, sym[1]->sym,
						  i, 2 * n, 2 * n + 1,
						  sym[0]->data, idx);
			} else {
				lte_unprecode_2x1(sym[0]->sym,
						  i, 2 * n, 2 * n + 1,
						  sym[0]->data, idx);
			}

			idx += 2;
		}
	}

	return idx;
}

static int chk_reserved(struct pbch_slot * pbch, int sc)
{
	int *pos = pbch->slot->subframe->ref_indices;

	if ((sc == pos[0]) || (sc == pos[1]) ||
	    (sc == pos[2]) || (sc == pos[3]))
		return 1;

	return 0;
}

static int pbch_extract(struct pbch_slot **pbch, int l, int chans)
{
	int sc0, sc1, sc2, sc3, idx = 0;
	struct pbch_sym *sym[2];

	if ((l != 0) && (l != 1)) {
		LOG_PBCH_ERR("Invalid symbol");
		return -1;
	}

	for (int i = 0; i < chans; i++)
		sym[i] = &pbch[i]->syms[l];

	for (int i = 0; i < LTE_PBCH_NUM_RB; i++) {
		for (int n = 0; n < LTE_RB_LEN; n += 6) {
			if (chk_reserved(pbch[0], n)) {
				sc0 = n + 1;
				sc1 = n + 2;
				sc2 = n + 4;
				sc3 = n + 5;
			} else if (chk_reserved(pbch[0], n + 1)) {
				sc0 = n + 0;
				sc1 = n + 2;
				sc2 = n + 3;
				sc3 = n + 5;
			} else if (chk_reserved(pbch[0], n + 2)) {
				sc0 = n + 0;
				sc1 = n + 1;
				sc2 = n + 3;
				sc3 = n + 4;
			} else {
				LOG_PBCH_ERR("Reference map fault");
				return -1;
			}

			if (chans == 2) {
				lte_unprecode_2x2(sym[0]->sym, sym[1]->sym,
						  i, sc0, sc1, sym[0]->data, idx);
				idx += 2;
				lte_unprecode_2x2(sym[0]->sym, sym[1]->sym,
						  i, sc2, sc3, sym[0]->data, idx);
				idx += 2;
			} else {
				lte_unprecode_2x1(sym[0]->sym,
						  i, sc0, sc1, sym[0]->data, idx);
				idx += 2;
				lte_unprecode_2x1(sym[0]->sym,
						  i, sc2, sc3, sym[0]->data, idx);
				idx += 2;
			}

		}
	}

	return idx;
}

static int pbch_extract_syms(struct pbch_slot **pbch, int chans)
{
	if ((chans < 1) || (chans > 2)) {
		LOG_PBCH_ERR("Invalid channel");
		return -1;
	}

	for (int l = 0; l < 4; l++) {
		switch (l) {
		case 0:
		case 1:
			if (pbch_extract(pbch, l, chans) < 0)
				return -1;
			break;
		case 2:
		case 3:
			if (pbch_extract_simple(pbch, l, chans) < 0)
				return -1;
			break;
		default:
			LOG_PBCH_ERR("Invalid symbol");
			return -1;
		}
	}

	return 0;
}

static void pbch_log_info(int ant, int rbs, int fn,
			  int phich_dur, int phich_ng)
{
	char sbuf[80];
	const char *phich_dur_str[2] = { "Normal", "Extended" };
	const char *phich_ng_str[4] = { "Sixth", "Half", "One", "Two" };

	if ((phich_dur < 0) || (phich_dur >= 2)) {
		LOG_PBCH_ERR("Invalid PHICH duration");
		return;
	} else if ((phich_ng < 0) || (phich_ng >= 4)) {
		LOG_PBCH_ERR("Invalid Ng");
		return;
	}

	snprintf(sbuf, 80, "PBCH  : "
		 "Antennas %i, RBs = %i, FN = %i, "
		 "PHICH duration = %s, Ng = %s",
		 ant, rbs, fn,
		 phich_dur_str[phich_dur], phich_ng_str[phich_ng]);

	LOG_CTRL(sbuf);
}

static int pbch_unpack(unsigned char *bits, int len, int ant,
		       int frame, struct lte_mib *mib)
{
	int n, rbs;
	unsigned fn = 0;
	int phich_dur, phich_ng;

	if (len != 40) {
		LOG_PBCH_ERR("Invalid MIB length");
		return -1;
	}
	if ((frame < 0) || (frame >3)) {
		LOG_PBCH_ERR("Invalid frame");
		return -1;
	}

	n = ((bits[0] & 0x01) << 2) |
	    ((bits[1] & 0x01) << 1) |
	    ((bits[2] & 0x01) << 0);

	switch (n) {
	case 0:
		rbs = 6;
		break;
	case 1:
		rbs = 15;
		break;
	case 2:
		rbs = 25;
		break;
	case 3:
		rbs = 50;
		break;
	case 4:
		rbs = 75;
		break;
	case 5:
		rbs = 100;
		break;
	default:
		LOG_PBCH_ERR("Invalid number of resource blocks");
		return -1;
	}

	fn = (((bits[ 6] & 0x01) << 7) |
	      ((bits[ 7] & 0x01) << 6) |
	      ((bits[ 8] & 0x01) << 5) |
	      ((bits[ 9] & 0x01) << 4) |
	      ((bits[10] & 0x01) << 3) |
	      ((bits[11] & 0x01) << 2) |
	      ((bits[12] & 0x01) << 1) |
	      ((bits[13] & 0x01) << 0)) << 2;

	fn |= (0x03 & frame);

	phich_dur = bits[3];
	phich_ng = ((bits[4] & 0x01) << 1) |((bits[5] & 0x01) << 0);

	mib->ant = ant;
	mib->rbs = rbs;
	mib->fn = fn;
	mib->phich_dur = phich_dur;
	mib->phich_ng = phich_ng;

	pbch_log_info(ant, rbs, fn, phich_dur, phich_ng);

	return 0;
}

#define PBCH_E		480

static int pbch_descramble(signed char *in, signed char *out,
			   signed char *seq, int n)
{
	if (n > 3) {
		LOG_PBCH_ERR("Descrambling error");
		return -1;
	}

	signed char *c = seq + n * PBCH_E;

	for (int i = 0; i < PBCH_E; i++)
		out[i] = in[i] * (2 * c[i] - 1);

	return 0;
}

static int pbch_decode(struct pbch_slot *pbch, int cell_id,
		       struct lte_mib *mib)
{
	int success = 0;
	signed char *e;
	unsigned char *a;

	signed char seq[PBCH_E];
	lte_pbch_gen_scrambler(cell_id, seq, PBCH_E);

	struct lte_pbch_blk *cblk = lte_pbch_blk_alloc();
	if (lte_pbch_blk_init(cblk, PBCH_E) < 0) {
		LOG_PBCH_ERR("Control block initialization failed");
		return -1;
	}

	e = lte_pbch_blk_ebuf(cblk, PBCH_E);
	a = lte_pbch_blk_abuf(cblk, PBCH_A);

	for (int n = 0; n < 4; n++) {
		pbch_descramble(pbch->mib.d, e, seq, n);

		int ant = lte_pbch_blk_decode(cblk);
		if (ant > 0) {
			pbch_unpack(a, PBCH_K, ant, n, mib);
			success = 1;
			break;
		}
	}

	lte_pbch_blk_free(cblk);

	return success;
}

/*
 * Decode one slot of sample data using specified reference signal map
 */
int lte_decode_pbch(struct lte_mib *mib,
		    struct lte_subframe **subframe, int chans)
{
	int i, rc = -1;
	struct pbch_slot *pbch[chans];

	for (int i = 0; i < chans; i++) {
		if (lte_subframe_convert(subframe[i]) < 0) {
			LOG_PBCH_ERR("Subframe conversion failed");
			return -1;
		}
	}

	for (i = 0; i < chans; i++) {
		pbch[i] = pbch_slot_alloc(&subframe[i]->slot[1]);
		if (!pbch[i]) {
			LOG_PBCH_ERR("Failed to allocate slot object");
			goto release;
		}
	}

	if (pbch_extract_syms(pbch, chans) < 0)
		goto release;

	lte_qpsk_decode2(pbch[0]->mib.e, pbch[0]->mib.d, 480);
	rc = pbch_decode(pbch[0], subframe[0]->cell_id, mib);

release:
	for (i = 0; i < chans; i++)
		free_pbch_slot(pbch[i]);

	return rc;
}
