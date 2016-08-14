/*
 * LTE Physical Control Format Indicator Channel (PCFICH) 
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
#include "sigproc.h"
#include "subframe.h"
#include "pcfich.h"
#include "ofdm.h"
#include "qam.h"
#include "scramble.h"
#include "precode.h"
#include "slot.h"
#include "sigvec_internal.h"

#define LTE_RB_LEN		12

/* 3GPP TS 36.212 Release 8: 5.4.3 Control format indicator */
static int pcfich_cfi[4][32] = {
	{ 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
	  1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 },
	{ 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
	  0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 },
	{ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
	  1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

struct pcfich_sym {
	struct lte_sym *sym;
	struct cxvec *data;
};

struct pcfich_slot {
	struct lte_slot *slot;
	struct pcfich_sym syms[1];

	struct {
		struct cxvec *vec;
		signed char code[LTE_PCFICH_CFI_LEN];
	} cfi;
};

static int pcfich_sym_init(struct pcfich_sym *sym, struct lte_sym *lsym,
			   struct cxvec *data)
{
	if (lsym->l) {
		fprintf(stderr, "PCFICH: Invalid symbol number %i\n", lsym->l);
		return -1;
	}

	sym->sym = lsym;
	sym->data = cxvec_alias(data);

	return 0;
}

static void pcfich_sym_free(struct pcfich_sym *sym)
{
	cxvec_free(sym->data);
}

static struct pcfich_slot *pcfich_slot_alloc(struct lte_slot *slot)
{
	struct pcfich_slot *pcfich;

	pcfich = malloc(sizeof *pcfich);
	pcfich->slot = slot;
	pcfich->cfi.vec = cxvec_alloc_simple(LTE_PCFICH_CFI_LEN / 2);
	pcfich_sym_init(&pcfich->syms[0],
			&pcfich->slot->syms[0], pcfich->cfi.vec);

	return pcfich;
}

static void pcfich_slot_free(struct pcfich_slot *pcfich)
{
	pcfich_sym_free(&pcfich->syms[0]);
	cxvec_free(pcfich->cfi.vec);
	free(pcfich);
}

static int chk_reserved(struct pcfich_slot *pcfich, int sc)
{
	int *pos = pcfich->slot->subframe->ref_indices;

	if ((sc == pos[0]) || (sc == pos[1]) ||
	    (sc == pos[2]) || (sc == pos[3]))
		return 1;

	return 0;
}

/* 3GPP TS 36.211 Release 8: 6.7.4: Mapping to resource elements */
static int pcfich_extract_syms(struct pcfich_slot **pcfich,
			       int chans, int n_cell_id, int *reserve)
{
	int tx_ants = pcfich[0]->slot->subframe->tx_ants;
	int map, rb, sc, sc0, sc1, sc2, sc3, idx = 0;
	int rbs = pcfich[0]->slot->rbs;
	int k_bar = LTE_RB_LEN / 2 * (n_cell_id % (2 * rbs));
	struct pcfich_sym *syms[chans];

	if ((chans < 0) || (chans > 2)) {
		fprintf(stderr, "PCFICH: Invalid channels %i\n", chans);
		exit(-1);
	}

	for (int i = 0; i < chans; i++)
		syms[i] = &pcfich[i]->syms[0];

	for (int i = 0; i < 4; i++) {
		map = (k_bar + (i * rbs) / 2 * LTE_RB_LEN / 2);
		map = map % (rbs * LTE_RB_LEN);
		rb = map / LTE_RB_LEN;
		sc = map % LTE_RB_LEN;

		if (chk_reserved(pcfich[0], sc)) {
			sc0 = sc + 1;
			sc1 = sc + 2;
			sc2 = sc + 4;
			sc3 = sc + 5;
		} else if (chk_reserved(pcfich[0], sc + 1)) {
			sc0 = sc + 0;
			sc1 = sc + 2;
			sc2 = sc + 3;
			sc3 = sc + 5;
		} else if (chk_reserved(pcfich[0], sc + 2)) {
			sc0 = sc + 0;
			sc1 = sc + 1;
			sc2 = sc + 3;
			sc3 = sc + 4;
		} else {
			fprintf(stderr, "PCFICH: Reference map fault\n");
			return -1;
		}

                struct lte_sym *sym0, *sym1;

		sym0 = syms[0]->sym;
		if (chans == 2)
			sym1 = syms[1]->sym;
		else
			sym1 = NULL;

		if (tx_ants > 2) {
			printf("Too many ants %i\n", tx_ants);
			exit(1);
		}

		lte_unprecode(sym0, sym1, tx_ants, chans,
			      rb, sc0, sc1, syms[0]->data, idx);
		idx += 2;
		lte_unprecode(sym0, sym1, tx_ants, chans,
			      rb, sc2, sc3, syms[0]->data, idx);
		idx += 2;

		/* Mark symbols as reserved in main slot object */
		reserve[rb * 12 + sc0] = 1;
		reserve[rb * 12 + sc1] = 1;
		reserve[rb * 12 + sc2] = 1;
		reserve[rb * 12 + sc3] = 1;
	}

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.4 Control format indicator */
static int pcfich_decode(struct pcfich_slot *pcfich)
{
	int i, n, err;
	signed char *b = pcfich->cfi.code;

	for (i = 0; i < 4; i++) {
		err = 0;
		for (n = 0; n < LTE_PCFICH_CFI_LEN; n++)
			err += b[n] ^ pcfich_cfi[i][n];

		if (err < 5)
			return i + 1;
	}

	return -1;
}

/* Decode one slot of sample data using specified reference signal map */
int lte_decode_pcfich(struct lte_pcfich_info *info,
		      struct lte_subframe **subframe,
		      int n_cell_id, signed char *seq, int chans)
{
	int cfi;
	struct pcfich_slot *pcfich[chans];

	for (int i = 0; i < chans; i++) {
		if (lte_subframe_convert(subframe[i]) < 0) {
			fprintf(stdout, "PCFICH: Failed to demdulate symbol(s)\n");
			exit(-1);
		}

		pcfich[i] = pcfich_slot_alloc(&subframe[i]->slot[0]);
		if (!pcfich[i]) {
			fprintf(stdout, "PCFICH: Failed to allocate slot object\n");
			exit(-1);
		}
	}

	pcfich_extract_syms(pcfich, chans, n_cell_id, subframe[0]->reserve);
	lte_qpsk_decode(pcfich[0]->cfi.vec,
			pcfich[0]->cfi.code, LTE_PCFICH_CFI_LEN);

	lte_scramble(pcfich[0]->cfi.code, seq, LTE_PCFICH_CFI_LEN);

	cfi = pcfich_decode(pcfich[0]);
	for (int i = 0; i < chans; i++)
		pcfich_slot_free(pcfich[i]);

	if (cfi > 0) {
		info->cfi = cfi;
		return 1;
	}

	return 0;
}
