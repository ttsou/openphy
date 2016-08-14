/*
 * LTE Physical Downlink Control Channel (PDCCH)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sigproc.h"
#include "lte.h"
#include "subframe.h"
#include "pdcch_interleave.h"
#include "crc.h"
#include "slot.h"
#include "ofdm.h"
#include "dci.h"
#include "pcfich.h"
#include "phich.h"
#include "qam.h"
#include "scramble.h"
#include "precode.h"
#include "si.h"
#include "log.h"
#include "pdcch_block.h"
#include "sigvec_internal.h"

#define LTE_MAX_NUM_PDCCH		3
#define LTE_PDCCH_CRC_LEN		16
#define LTE_PCFICH_LEN			LTE_PCFICH_CFI_LEN / 2

/* DCI message aggrgation level lengths */
#define LTE_DCI_A1_LEN			72
#define LTE_DCI_A2_LEN			144
#define LTE_DCI_A4_LEN			288
#define LTE_DCI_A8_LEN			576

/* Common search space spans two blocks at aggregation level 8 */
#define LTE_PDCCH_COMMON_LEN		LTE_DCI_A8_LEN * 2

#define LTE_SI_RNTI	0xffff

//#define PDCCH_DEBUG 1

struct pdcch_res_map {
	int sc;
	int rb;
	int l;
};

struct pdcch_sym {
	struct lte_sym *sym;
	struct cxvec *data;
};

struct pdcch_slot {
	int cfi;
	int len;
	int tx_ants;
	struct lte_slot *slot;
	struct pdcch_sym syms[LTE_MAX_NUM_PDCCH];
	struct cxvec *pdcch;
	struct cxvec *pdcch_deinterlv;
	signed char *bits;
	int *reserve;

	int num_dci;
	struct lte_dci dci[LTE_DCI_MAX];
};

static int pdcch_sym_init(struct pdcch_sym *sym,
			  struct lte_sym *lsym, struct cxvec *data)
{
	if (lsym->l >= LTE_MAX_NUM_PDCCH) {
		fprintf(stderr, "PDCCH: Invalid symbol number %i\n", lsym->l);
		return -1;
	}

	sym->sym = lsym;
	sym->data = cxvec_alias(data);

	return 0;
}

static void pdcch_sym_free(struct pdcch_sym *sym)
{
	cxvec_free(sym->data);
}

static struct pdcch_slot *pdcch_slot_alloc(struct lte_slot *slot,
					   int cfi, int phich_groups, int *res)
{
	int pcfich_len = LTE_PCFICH_LEN;
	int len = slot->rbs * 8 - pcfich_len -
		  12 * phich_groups + (cfi - 1) * (slot->rbs * LTE_RB_LEN);
	struct pdcch_slot *pdcch;

	if ((cfi > 3) || (cfi < 0)) {
		fprintf(stderr, "PDCCH: Invalid CFI value %i\n", cfi);
		return NULL;
	}

	pdcch = malloc(sizeof *pdcch);
	pdcch->cfi = cfi;
	pdcch->len = len;
	pdcch->tx_ants = slot->subframe->tx_ants;
	pdcch->slot = slot;
	pdcch->pdcch = cxvec_alloc_simple(len);
	pdcch->pdcch_deinterlv = cxvec_alloc_simple(len);
	pdcch->bits = malloc(len * 2 * sizeof *pdcch->bits);
	pdcch->num_dci = 0;
	pdcch->reserve = res;

	/* Up to 3 PDCCH symbols */
	for (int i = 0; i < cfi; i++)
		pdcch_sym_init(&pdcch->syms[i],
			       &pdcch->slot->syms[i],
			       pdcch->pdcch);

	/* Clear all the data bits until we handle blind search correctly */
	//memset(pdcch->data.pdcch, 0, LTE_PDCCH_COMMON_LEN * sizeof(char));

	return pdcch;
}

static void free_pdcch_slot(struct pdcch_slot *pdcch)
{
	free(pdcch->bits);
	cxvec_free(pdcch->pdcch);
	cxvec_free(pdcch->pdcch_deinterlv);

	for (int i = 0; i < pdcch->cfi; i++)
		pdcch_sym_free(&pdcch->syms[i]);

	free(pdcch);
}

static int next_element(int rbs, struct pdcch_res_map *map, int *x, int num)
{
	int l;
	int res = rbs * LTE_RB_LEN;

	if ((x[0] > res + 1) || (x[1] > res) || (x[2] > res)) {
		fprintf(stderr, "PDCCH: Resource counters are not sane!\n");
		fprintf(stderr, "PDCCH: 0 %i, 1 %i, 2 %i\n", x[0], x[1], x[2]);
		return -1;
	}

	switch (num) {
	case 1:
		if (x[0] >= res)
			return 0;

		l = 0;
		break;
	case 2:
		if ((x[0] >= res) && (x[1] >= res))
			return 0;

		if (x[0] <= x[1])
			l = 0;
		else
			l = 1;
		break;
	case 3:
		if ((x[0] >= res) &&
		    (x[1] >= res) && (x[2] >= res))
			return 0;

		if ((x[0] <= x[1]) && (x[0] <= x[2]))
			l = 0;
		else if (x[1] <= x[2])
			l = 1;
		else
			l = 2;
		break;
	default:
		fprintf(stderr, "PDCCH: Invalid PCFICH value\n");
		return -1;
	}

	map->l = l;
	map->rb = x[l] / LTE_RB_LEN;
	map->sc = x[l] % LTE_RB_LEN;

	return 1;
}

static int chk_ref_pos(struct pdcch_slot *pdcch, int rb, int sc)
{
	int *pos = pdcch->slot->subframe->ref_indices;

	if ((sc == pos[0]) || (sc == pos[1]) ||
	    (sc == pos[2]) || (sc == pos[3]))
		return 1;

	return 0;
}

static int chk_reserved(struct pdcch_slot *pdcch, int rb, int sc)
{
	if (pdcch->reserve[rb * 12 + sc])
		return 1;

	return 0;
}

/*
 * Unmap from reference interleaved resource blocks to continuous
 * data symbols and perform two antenna processing. Other antenna
 * combinations are not supported at this time.
 */
static int pdcch_extract_syms(struct pdcch_slot **pdcch, int chans)
{
	int l, rb, sc;
	int sc0, sc1, sc2, sc3;
	int cnt = 0, phich_cnt = 0;
	int cfi = pdcch[0]->cfi;
	int sc_cnt[3] = { 0, 0, 0 };

	struct pdcch_res_map map;
	struct pdcch_sym *syms[chans][cfi];

	if ((chans < 0) || (chans > 2)) {
		fprintf(stderr, "PDCCH: Invalid channels %i\n", chans);
		exit(-1);
	}

	for (int i = 0; i < cfi; i++) {
		for (int n = 0; n < chans; n++)
			syms[n][i] = &pdcch[n]->syms[i];

		sc_cnt[i] = 0;
	}

	while (next_element(pdcch[0]->slot->rbs, &map, sc_cnt, cfi) > 0) {
		l = map.l;
		rb = map.rb;
		sc = map.sc;

		switch (l) {
		case 0:
			if (sc >= 7) {
				sc_cnt[0]++;
				continue;
			}

			if ((chk_reserved(pdcch[0], rb, sc)) ||
			    (chk_reserved(pdcch[0], rb, sc + 1))) {
				sc_cnt[0]++;
				continue;
			}

			if (chk_ref_pos(pdcch[0], rb, sc)) {
				sc0 = sc + 1;
				sc1 = sc + 2;
				sc2 = sc + 4;
				sc3 = sc + 5;
				sc_cnt[0] += 6;
			} else if (chk_ref_pos(pdcch[0], rb, sc + 1)) {
				sc0 = sc + 0;
				sc1 = sc + 2;
				sc2 = sc + 3;
				sc3 = sc + 5;
				sc_cnt[0] += 6;
			} else if (chk_ref_pos(pdcch[0], rb, sc + 2)) {
				sc0 = sc + 0;
				sc1 = sc + 1;
				sc2 = sc + 3;
				sc3 = sc + 4;
				sc_cnt[0] += 6;
			} else {
				fprintf(stderr,
					"PDCCH: Reference map fault %i\n", sc);
				return -1;
			}
			break;
		case 1:
		case 2:
			sc0 = sc + 0;
			sc1 = sc + 1;
			sc2 = sc + 2;
			sc3 = sc + 3;
			break;
		default:
			fprintf(stderr, "PDCCH: Invalid symbol %i\n", l);
			return -1;
		}
#if PDCCH_DEBUG
		printf("rb %i, l %i, sc0 %i\n", rb, l, sc0);
		printf("rb %i, l %i, sc1 %i\n", rb, l, sc1);
		printf("rb %i, l %i, sc2 %i\n", rb, l, sc2);
		printf("rb %i, l %i, sc3 %i\n", rb, l, sc3);
#endif
		struct lte_sym *sym0, *sym1;

		sym0 = syms[0][l]->sym;
		if (chans == 2)
			sym1 = syms[1][l]->sym;
		else
			sym1 = NULL;

		if (pdcch[0]->tx_ants > 2) {
			printf("too many ants\n");
			exit(1);
		}

		lte_unprecode(sym0, sym1, pdcch[0]->tx_ants, chans,
			      rb, sc0, sc1, syms[0][l]->data, cnt + 0);
		lte_unprecode(sym0, sym1, pdcch[0]->tx_ants, chans,
			      rb, sc2, sc3, syms[0][l]->data, cnt + 2);
		cnt += 4;
		if (!l)
			phich_cnt += 4;
		else
			sc_cnt[l] += 4;
	}

	if (cnt != pdcch[0]->len) {
		fprintf(stderr, "PDCCH: Extracted %i symbols but expected %i\n",
			cnt, pdcch[0]->len);
		exit(-1);
	}

	return 0;
}

static void log_dci_info(struct lte_dci *dci, int len,
			 unsigned rnti, int lev, int blk)
{
	char sbuf[80];
	snprintf(sbuf, 80, "PDCCH : DCI Length %i, CRC %i, Agg. Level %i, Block %i",
		 len, rnti, lev, blk * lev);
	LOG_CTRL(sbuf);
	lte_dci_print(dci);
}

#ifdef LOG_CCE_INFO
static void log_cce_info(int cfi, int regs, int cces, int nbits)
{
	char sbuf[80];
	snprintf(sbuf, 80, "PDCCH : CFI %i, Bits %i, REGs %i, CCEs %i",
		 cfi, nbits, regs, cces);
	LOG_CTRL(sbuf);
}
#endif

static int pdcch_decode_bits(struct pdcch_slot *pdcch,
			     int lev, int blk, int type, uint16_t rnti)
{
	int A, E, found = 0;
	int rbs = pdcch->slot->rbs;
	int8_t *e;
	uint8_t *a;
	struct lte_pdcch_blk *dblk;

	switch (lev) {
	case 1:
		E = LTE_DCI_A1_LEN;
		break;
	case 2:
		E = LTE_DCI_A2_LEN;
		break;
	case 4:
		E = LTE_DCI_A4_LEN;
		break;
	case 8:
		E = LTE_DCI_A8_LEN;
		break;
	default:
		LOG_PDCCH_ERR("Invalid aggregation level");
		return -1;
	}

	if (pdcch->num_dci >= LTE_DCI_MAX) {
		LOG_PDCCH_ERR("Maximum number of DCI values reached");
		return 0;
	}

	A = lte_dci_format_size(rbs, LTE_MODE_FDD, type);
	if (A < 0) {
		LOG_PDCCH_ERR("Could not find valid DCI format");
		return -1;
	}

	dblk = lte_pdcch_blk_alloc();
	if (lte_pdcch_blk_init(dblk, A, E) < 0) {
		LOG_PDCCH_ERR("Downlink control block failed");
		return -1;
	}

	a = lte_pdcch_blk_abuf(dblk, A);
	e = lte_pdcch_blk_ebuf(dblk, E);

	memcpy(e, &pdcch->bits[E * blk], E * sizeof(char));

	if (lte_pdcch_blk_decode(dblk, rnti)) {
		struct lte_dci *dci = &pdcch->dci[pdcch->num_dci];
		lte_dci_decode(dci, rbs, LTE_MODE_FDD, a, A, rnti);

		if (dci->type != LTE_DCI_FORMAT0) {
			log_dci_info(dci, A, rnti, lev, blk);
			pdcch->num_dci++;
			found = 1;
		}
	}

	lte_pdcch_blk_free(dblk);

	return found;
}

static int pdcch_decode_blk(struct pdcch_slot *pdcch,
			    int lev, int blk, uint16_t rnti)
{
	if (rnti == LTE_SI_RNTI) {
		return (pdcch_decode_bits(pdcch, lev, blk,
					  LTE_DCI_FORMAT1A, rnti) ||
			pdcch_decode_bits(pdcch, lev, blk,
					  LTE_DCI_FORMAT1C, rnti));
	}

	return (pdcch_decode_bits(pdcch, lev, blk,
				  LTE_DCI_FORMAT1, rnti) ||
		pdcch_decode_bits(pdcch, lev, blk,
				  LTE_DCI_FORMAT1A, rnti) ||
		pdcch_decode_bits(pdcch, lev, blk,
				  LTE_DCI_FORMAT1B, rnti) ||
		pdcch_decode_bits(pdcch, lev, blk,
				  LTE_DCI_FORMAT1C, rnti));

}

static int pdcch_dci_power_search_si(struct pdcch_slot *pdcch, int ncce,
				     int agg8_blk, uint16_t rnti)
{
	struct cxvec *agg1;
	int shift, blk, success = 0;
	unsigned agg4_mask = 0, agg2_mask = 0, agg1_mask = 0;

	if (ncce == 8) {
		if (agg8_blk && (pdcch->len * 2 < 2 * LTE_DCI_A8_LEN)) {
			return -1;
		} else if (pdcch->len * 2 < LTE_DCI_A8_LEN) {
			return -1;
		}
	}

	shift = agg8_blk * LTE_DCI_A8_LEN / 2;
	for (int i = 0; i < ncce; i++) {
		agg1 = cxvec_subvec(pdcch->pdcch_deinterlv,
				    shift + 36 * i, 0, 0, 36);

                if ((cxvec_avgpow(agg1) > 0.5f))
			agg1_mask |= 1 << i;

		cxvec_free(agg1);
	}

	/* Aggregation level 8 */
	if (ncce == 8) {
		if ((agg1_mask & 0xff) == 0xff) {
			blk = agg8_blk;
			if (pdcch_decode_blk(pdcch, 8, blk, rnti))
				return 1;
		}
	}

	/* Aggregation level 4 */
	if (ncce >= 4) {
		if ((agg1_mask & 0x0f) == 0x0f) {
			blk = 2 * agg8_blk;
			if (pdcch_decode_blk(pdcch, 4, blk, rnti)) {
				agg4_mask = 1 << 0;
				success = 1;
			}
		}
	}
	if (ncce == 8) {
		if ((agg1_mask & 0xf0) == 0xf0) {
			blk = 2 * agg8_blk + 1;
			if (pdcch_decode_blk(pdcch, 4, blk, rnti)) {
				agg4_mask |= 1 << 1;
				success = 1;
			}
		}
	}

	if (rnti == LTE_SI_RNTI)
		return success;

	/* Aggregation level 2 */
	if (ncce >= 2) {
		if (((agg1_mask & 0x03) == 0x03) && (!(agg4_mask & 0x01))) {
			blk = 4 * agg8_blk;
			if (pdcch_decode_blk(pdcch, 2, blk, rnti)) {
				agg2_mask = 1 << 0;
				success = 1;
			}
		}
	}
	if (ncce >= 4) {
		if (((agg1_mask & 0x0c) == 0x0c) && (!(agg4_mask & 0x01))) {
			blk = 4 * agg8_blk + 1;
			if (pdcch_decode_blk(pdcch, 2, blk, rnti)) {
				agg2_mask |= 1 << 1;
				success = 1;
			}
		}
	}
	if (ncce >= 6) {
		if (((agg1_mask & 0x30) == 0x30) && (!(agg4_mask & 0x02))) {
			blk = 4 * agg8_blk + 2;
			if (pdcch_decode_blk(pdcch, 2, blk, rnti)) {
				agg2_mask |= 1 << 2;
				success = 1;
			}
		}
	}
	if (ncce == 8) {
		if (((agg1_mask & 0xc0) == 0xc0) && (!(agg4_mask & 0x02))) {
			blk = 4 * agg8_blk + 3;
			if (pdcch_decode_blk(pdcch, 2, blk, rnti)) {
				agg2_mask |= 1 << 3;
				success = 1;
			}
		}
	}

	/* Aggregation level 1 - Doesn't precheck other aggregation levels */
	if ((agg1_mask & 0x01) &&
	    (!(agg4_mask & 0x01)) && (!(agg2_mask & 0x01))) {
		blk = 8 * agg8_blk;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}
	if (ncce == 1)
		return success;
	if ((agg1_mask & 0x02) &&
	    (!(agg4_mask & 0x01)) && (!(agg2_mask & 0x01))) {
		blk = 8 * agg8_blk  + 1;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}

	if (ncce == 2)
		return success;
	if ((agg1_mask & 0x04) &&
	    (!(agg4_mask & 0x01)) && (!(agg2_mask & 0x02))) {
		blk = 8 * agg8_blk + 2;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}
	if (ncce == 3)
		return success;
	if ((agg1_mask & 0x08) &&
	    (!(agg4_mask & 0x01)) && (!(agg2_mask & 0x02))) {
		blk = 8 * agg8_blk + 3;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}
	if (ncce == 4)
		return success;
	if ((agg1_mask & 0x10) &&
	    (!(agg4_mask & 0x02)) && (!(agg2_mask & 0x04))) {
		blk = 8 * agg8_blk + 4;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}
	if (ncce == 5)
		return success;
	if ((agg1_mask & 0x20) &&
	    (!(agg4_mask & 0x02)) && (!(agg2_mask & 0x04))) {
		blk = 8 * agg8_blk + 5;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}
	if (ncce == 6)
		return success;
	if ((agg1_mask & 0x40) &&
	    (!(agg4_mask & 0x02)) && (!(agg2_mask & 0x08))) {
		blk = 8 * agg8_blk + 6;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}
	if (ncce == 7)
		return success;
	if ((agg1_mask & 0x80) &&
	    (!(agg4_mask & 0x02)) && (!(agg2_mask & 0x08))) {
		blk = 8 * agg8_blk + 7;
		if (pdcch_decode_blk(pdcch, 1, blk, rnti))
			success = 1;
	}

	return success;
}

static int pdcch_num_cce(struct pdcch_slot *pdcch)
{
	return pdcch->len / 4 / 9;
}

#ifdef LOG_CCE_INFO 
static int pdcch_num_bits(struct pdcch_slot *pdcch)
{
	return pdcch->len * 2;
}
#endif

static int pdcch_num_cce_bits(struct pdcch_slot *pdcch)
{
	return pdcch->len / 4 / 9 * 72;
}

static int pdcch_si_search(struct pdcch_slot *pdcch, uint16_t rnti,
			   signed char *seq, int n_cell_id)
{
	int i, nbits;
	struct lte_pdcch_deinterlv *d;

#ifdef LOG_CCE_INFO
	log_cce_info(pdcch->cfi, pdcch->len / 4,
		     pdcch_num_cce(pdcch), pdcch_num_bits(pdcch));
#endif
	/* Make the deinterleaver persistent */
	d = lte_alloc_pdcch_deinterlv(pdcch->len / 4, n_cell_id);
	pdcch_deinterlv(d, (cquadf *) pdcch->pdcch->data,
		(cquadf *) pdcch->pdcch_deinterlv->data);
	lte_free_pdcch_deinterlv(d);

	nbits = pdcch_num_cce_bits(pdcch);

	lte_qpsk_decode2(pdcch->pdcch_deinterlv, pdcch->bits, nbits);
	lte_scramble2(pdcch->bits, seq, nbits);

	if (pdcch_num_cce(pdcch) < 8) {
		pdcch_dci_power_search_si(pdcch, pdcch_num_cce(pdcch), 0, rnti);
		return pdcch->num_dci;
	}

	for (i = 0; i < pdcch_num_cce(pdcch) / 8; i++) {
		if (pdcch_dci_power_search_si(pdcch, 8, i, rnti) < 0)
			return -1;
	}

	int leftover_cces = pdcch_num_cce(pdcch) % 8;

	pdcch_dci_power_search_si(pdcch, leftover_cces, i, rnti);

	return pdcch->num_dci;
}

/* Decode one slot of sample data using specified reference signal map */
int lte_decode_pdcch(struct lte_subframe **subframe, int chans,
		     int cfi, int n_cell_id, int ng, uint16_t rnti,
		     signed char *seq)
{
	int num = 0, phich_groups;
	struct pdcch_slot *pdcch[chans];

	if (!subframe[0]->assigned) {
		fprintf(stderr, "PDCCH: Subframe not assigned\n");
		return -1;
	}

	phich_groups = lte_phich_num_groups(subframe[0]->rbs, ng,
					    LTE_PHICH_DUR_NORMAL);
	if (phich_groups < 0) {
		fprintf(stderr, "PDCCH: Invalid PHICH group\n");
		return -1;
	}

	if (subframe[0]->rbs <= 10)
		cfi++;

	int *res = subframe[0]->reserve;
	for (int i = 0; i < chans; i++) {
		pdcch[i] = pdcch_slot_alloc(&subframe[i]->slot[0],
					    cfi, phich_groups, res);
		if (!pdcch[i]) {
			fprintf(stderr, "PDCCH: Allocation failed\n");
			exit(-1);
		}
	}

	if (lte_gen_phich_indices(res, subframe[0]->rbs,
				  n_cell_id, ng, LTE_PHICH_DUR_NORMAL) < 0) {
		fprintf(stderr, "PDCCH: Failed to set PHICH symbol indices\n");
		goto release;
	}

#if PDCCH_DEBUGxxx
	printf("PHICH: Reservation table\n");
	for (int i = 0; i < 600; i++)
		printf("%2i   %i   %i\n",
			i, subframe[0]->reserve[i],
			chk_reserved(pdcch[0], i / 12, i % 12));
#endif

	if (pdcch_extract_syms(pdcch, chans) < 0) {
		fprintf(stderr, "PDCCH: Symbol extraction failed\n");
		goto release;
	}

	if (pdcch_si_search(pdcch[0], rnti, seq, n_cell_id) > 0) {
		num = pdcch[0]->num_dci;
		memcpy(subframe[0]->dci,
		       pdcch[0]->dci, num * sizeof(struct lte_dci));
		subframe[0]->num_dci = num;
	}

release:
	for (int i = 0; i < chans; i++)
		free_pdcch_slot(pdcch[i]);

	return num;
}
