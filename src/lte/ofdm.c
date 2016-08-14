/*
 * LTE OFDM Core 
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
#include <string.h>
#include <stdlib.h>

#include "ofdm.h"
#include "log.h"
#include "slot.h"
#include "subframe.h"
#include "ref.h"
#include "interpolate.h"
#include "fft.h"
#include "sigvec_internal.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define INTERP_TAPS		32

/*
 * Map center resource block that spans the Nyquist edge
 *
 * This occurs on 3, 5, and 15 MHz channel allocations with 15, 25, and 75
 * resource block respectively. Address the split center block by reallocating
 * to a contiguous vector.
 */
static int lte_sym_rb_map_special(struct lte_sym *sym, int rb)
{
	int len, rb0, rb1;
	int rbs = sym->slot->subframe->rbs;

	if ((rbs == 15) && (rb == 7)) {
		rb0 = LTE_N15_RB7;
		rb1 = LTE_N15_RB7_1;
	} else if ((rbs == 25) && (rb == 12)) {
		rb0 = LTE_N25_RB12;
		rb1 = LTE_N25_RB12_1;
	} else if ((rbs == 75) && (rb == 37)) {
		rb0 = LTE_N75_RB37;
		rb1 = LTE_N75_RB37_1;
	} else {
		LOG_DSP_ARG("Resource block mapping error ", rb);
		return -1;
	}

	if (!sym->rb[rb])
		sym->rb[rb] = cxvec_alloc_simple(LTE_RB_LEN);

	len = LTE_RB_LEN / 2;
	cxvec_cp(sym->rb[rb], sym->fd, 0, rb0, len);
	cxvec_cp(sym->rb[rb], sym->fd, len, rb1, len);

	return 0;
}

/*
 * FIXME: Map symbols to resource blocks
 *
 * We don't do this at initial allocation because the frequency domain symbols 
 * are not available until after performing the FFT.
 */
static void lte_sym_rb_map(struct lte_sym *sym)
{
	int pos;
	int rbs = sym->slot->subframe->rbs;

	for (int i = 0; i < rbs; i++) {
		if (((rbs == 15) && (i == 7)) ||
		    ((rbs == 25) && (i == 12)) ||
		    ((rbs == 75) && (i == 37))) {
			lte_sym_rb_map_special(sym, i);
			continue;
		}

		pos = lte_rb_pos(rbs, i);
		sym->rb[i] = cxvec_subvec(sym->fd, pos,  0, 0, LTE_RB_LEN);
	}
}

static int lte_sym_chan_rb_map_special(struct lte_ref *ref, int rb, int p)
{
	int len = LTE_RB_LEN / 2;
	int rbs = ref->sym->slot->subframe->rbs;
	int rb0, rb1;

	if ((rbs == 15) && (rb == 7)) {
		rb0 = LTE_N15_RB7;
		rb1 = LTE_N15_RB7_1;
	} else if ((rbs == 25) && (rb == 12)) {
		rb0 = LTE_N25_RB12;
		rb1 = LTE_N25_RB12_1;
	} else if ((rbs == 75) && (rb == 37)) {
		rb0 = LTE_N75_RB37;
		rb1 = LTE_N75_RB37_1;
	} else {
		LOG_DSP_ARG("Resource block mapping error ", rb);
		return -1;
	}

	if (!ref->rb[p][rb])
		ref->rb[p][rb] = cxvec_alloc_simple(LTE_RB_LEN);

	cxvec_cp(ref->rb[p][rb], ref->chan[p], 0, rb0, len);
	cxvec_cp(ref->rb[p][rb], ref->chan[p], len, rb1, len);

	return 0;
}

/*
 * Map center resource block for antenna 'p'
 *
 * This occurs on 3, 5, and 15 MHz channel allocations with 15, 25, and 75
 * resource block respectively. Address the split center block by reallocating
 * to a contiguous vector.
 */
static int lte_sym_chan_rb_map(struct lte_ref *ref, int p)
{
	int pos;
	int rbs = ref->sym->slot->subframe->rbs;

	/* We have an extra magnitude channel */
	if (p >= 2 + 1) {
		LOG_DSP_ARG("Invalid antenna configuration ", p - 1);
		return -1;
	}

	for (int i = 0; i < rbs; i++) {
		if (((rbs == 15) && (i == 7)) ||
		    ((rbs == 25) && (i == 12)) ||
		    ((rbs == 75) && (i == 37))) {
			lte_sym_chan_rb_map_special(ref, i, p);
			continue;
		}

		pos = lte_rb_pos(rbs, i);
		ref->rb[p][i] = cxvec_subvec(ref->chan[p],
					     pos, 0, 0, LTE_RB_LEN);
	}

	return 0;
}

/* Assign reference symbol maps to slots containing reference symbols */
static int assign_ref_maps(struct lte_slot *slot, struct lte_ref_map **maps)
{
	int i, p;

	for (i = 0; i < 4; i++) {
		p = maps[i]->p;
		if (p > 1) {
			LOG_DSP_ARG("Invalid antenna configuration ", p);
			return -1;
		}

		switch (maps[i]->l) {
		case 0:
			slot->refs[0].map[p] = maps[i];
			break;
		case 4:
			slot->refs[1].map[p] = maps[i];
			break;
		default:
			LOG_DSP_ARG("Invalid symbol map ", maps[i]->l);
			return -1;
		}
	}

	return 0;
}

/* Set the per-resource block reference symbol indices */
static int init_ref_indices(struct lte_subframe *subframe)
{
	int base;
	struct lte_ref *ref = &subframe->slot[0].refs[0];

	if ((!ref->map[0]) || (!ref->map[1])) {
		LOG_DSP_ERR("Reference map not assigned");
		return -1;
	}

	if (ref->map[0]->v_shift != ref->map[1]->v_shift) {
		LOG_DSP_ERR("Invalid reference map");
		return -1;
	}

	base = (ref->map[0]->v + ref->map[0]->v_shift) % 3;

	subframe->ref_indices[0] = base;
	subframe->ref_indices[1] = (base + 3) % LTE_RB_LEN;
	subframe->ref_indices[2] = (base + 6) % LTE_RB_LEN;
	subframe->ref_indices[3] = (base + 9) % LTE_RB_LEN;

	return 0;
}

int lte_chk_ref(struct lte_subframe *subframe, int slot, int l, int sc, int p)
{
	int base;
	int *pos = subframe->ref_indices;
	struct lte_ref *ref;

	if ((!slot && !l) || (p == 2)) {
		if ((sc == pos[0]) || (sc == pos[1]) ||
		    (sc == pos[2]) || (sc == pos[3]))
			return 1;
	} else if ((!slot && (l == 4)) && (p == 1)) {
		ref = &subframe->slot[0].refs[1];
		base = (ref->map[0]->v + ref->map[0]->v_shift) % 6;
		if (sc == base)
			return 1;
		if (sc == (base + 6) % LTE_RB_LEN)
			return 1;
	} else if ((slot && !l) && (p == 1)) {
		ref = &subframe->slot[1].refs[0];
		base = (ref->map[0]->v + ref->map[0]->v_shift) % 6;
		if (sc == base)
			return 1;
		if (sc == (base + 6) % LTE_RB_LEN)
			return 1;
	} else if ((slot && (l == 4)) && (p == 1)) {
		ref = &subframe->slot[1].refs[1];
		base = (ref->map[0]->v + ref->map[0]->v_shift) % 6;
		if (sc == base)
			return 1;
		if (sc == (base + 6) % LTE_RB_LEN)
			return 1;
	}

        return 0;
}

static int lte_ref_init(struct lte_slot *slot, int l)
{
	int rbs = slot->rbs;
	int sym_len = lte_sym_len(rbs);
	int delay = INTERP_TAPS / 2;
	int flags = CXVEC_FLG_MEM_INIT;

	int ref_num = l == 0 ? 0 : 1;
	struct lte_ref *ref = &slot->refs[ref_num];

	ref->slot = slot;
	ref->sym = &slot->syms[l];

	for (int i = 0; i < 2; i++)
		ref->refs[i] = cxvec_alloc(sym_len, delay, delay, NULL, flags);

	for (int p = 0; p < 3; p++) {
		ref->chan[p] = cxvec_alloc_simple(sym_len);
		ref->rb[p] = (struct cxvec **)
				  calloc(rbs, sizeof(struct cxvec *));
		lte_sym_chan_rb_map(ref, p);
	}

	ref->map[0] = NULL;
	ref->map[1] = NULL;

	return 0;
}

static void lte_ref_free(struct lte_ref *ref)
{
	int rbs = ref->sym->slot->rbs;

	for (int p = 0; p < 3; p++) {
		for (int n = 0; n < rbs; n++)
			cxvec_free(ref->rb[p][n]);

		free(ref->rb[p]);
		cxvec_free(ref->chan[p]);
	}

	for (int i = 0; i < 2; i++)
		cxvec_free(ref->refs[i]);
}

/* Initialize a LTE symbol struct  */
static int lte_sym_init(struct lte_slot *slot, int l)
{
	int rbs = slot->rbs;
	int sym_len = lte_sym_len(rbs);
	int td_pos = lte_sym_pos(rbs, l);
	int fd_pos = l * sym_len;

	struct lte_sym *sym = &slot->syms[l];

	sym->l = l;
	sym->slot = slot;
	sym->td = cxvec_subvec(slot->td, td_pos, 0, 0, sym_len);
	sym->fd = cxvec_subvec(slot->fd, fd_pos, 0, 0, sym_len);
	sym->rb = (struct cxvec **) calloc(rbs, sizeof(struct cxvec *));
	lte_sym_rb_map(sym);

	/* All symbols use the same reference symbol */
	sym->ref = &slot->subframe->slot[0].refs[0];

	return 0;
}

/*
 * Release LTE symbol resources. The slot allocated object is not released.
 */
static void lte_sym_free(struct lte_sym *sym)
{
	int rbs = sym->slot->rbs;

	cxvec_free(sym->td);
	cxvec_free(sym->fd);

	for (int i = 0; i < rbs; i++)
		cxvec_free(sym->rb[i]);
	free(sym->rb);
}

/*
 * Allocate an LTE slot object
 *
 * Each slot containing 7 symbols - 2 of which contain reference symbols.
 */
static int lte_slot_init(struct lte_subframe *subframe,
			 struct lte_ref_map **maps, int ns)

{
	int start, rbs = subframe->rbs;
	int slot_len = lte_slot_len(rbs);
	int sym_len = lte_sym_len(rbs);
	struct lte_slot *slot;

	if (maps[0]->rbs != rbs) {
		LOG_DSP_ARG("Invalid reference map ", maps[0]->rbs);
		return -1;
	}

	if (ns % 2) {
		start = slot_len;
		slot = &subframe->slot[1];
	} else {
		start = 0;
		slot = &subframe->slot[0];
	}

	slot->rbs = rbs;
	slot->subframe = subframe;
	slot->td = cxvec_subvec(subframe->samples, start, 0, 0, slot_len);
	slot->fd = cxvec_alloc(sym_len * 7, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	/* Initialize 7 symbols and 2 reference symbols */
	for (int l = 0; l < 7; l++) {
		lte_sym_init(slot, l);

		if ((l == 0) || (l == 4))
			lte_ref_init(slot, l);
	}

	/* Set reference symbol locations */
	assign_ref_maps(slot, maps);

	return 0;
}

/*
 * Release an LTE slot object
 *
 * Do not release the sample vector or interpolator as those are always
 * allocated by the parent subframe.
 */
static void lte_slot_free(struct lte_slot *slot)
{
	lte_ref_free(&slot->refs[0]);
	lte_ref_free(&slot->refs[1]);

	for (int l = 0; l < 7; l++)
		lte_sym_free(&slot->syms[l]);

	cxvec_free(slot->td);
	cxvec_free(slot->fd);
}

static struct fft_hdl *create_fft(int rbs)
{
	int ilen, olen;
	struct cxvec *in = NULL, *out = NULL;
	int slen = lte_sym_len(rbs);
	int clen = lte_cp_len(rbs);

	ilen = clen + slen;
	olen = slen;

	in = cxvec_alloc(7 * ilen, 0 , 0, NULL, CXVEC_FLG_FFT_ALIGN);
	out = cxvec_alloc(7 * olen, 0 , 0, NULL, CXVEC_FLG_FFT_ALIGN);

	return init_fft(0, slen, 7, ilen, olen, 1, 1, in, out, 0);
}

struct lte_subframe *lte_subframe_alloc(int rbs, int cell_id, int tx_ants,
					struct lte_ref_map **maps0,
					struct lte_ref_map **maps1)
{
	struct lte_subframe *subframe;
	int subframe_len = lte_subframe_len(rbs);
	int flags = CXVEC_FLG_FFT_ALIGN;

	if (subframe_len < 0)
		return NULL;

	subframe = (struct lte_subframe *)
		   calloc(1, sizeof(struct lte_subframe));
	subframe->rbs = rbs;
	subframe->assigned = 0;
	subframe->samples = cxvec_alloc(subframe_len, 0, 0, NULL, flags);
	subframe->num_dci = 0;
	subframe->cell_id = cell_id;
	subframe->tx_ants = tx_ants;

	lte_slot_init(subframe, maps0, 0);
	lte_slot_init(subframe, maps1, 1);

	subframe->fft = create_fft(rbs);
	if (!subframe->fft) {
		LOG_DSP_ERR("Internal FFT failure");
		return NULL;
	}

	subframe->interp = init_interp(INTERP_TAPS, (float) INTERP_TAPS / 1.5f);

	/* Bit reservevation table */
	subframe->reserve = (int *) calloc(rbs * 12, sizeof(int));

	if (init_ref_indices(subframe) < 0) {
		LOG_DSP_ERR("Internal reference symbol failure");
                return NULL;
	}

	return subframe;
}

void lte_subframe_free(struct lte_subframe *subframe)
{
	if (!subframe)
		return;

	lte_slot_free(&subframe->slot[0]);
	lte_slot_free(&subframe->slot[1]);
	cxvec_free(subframe->samples);

	free_interp(subframe->interp);
	fft_free_hdl(subframe->fft);

	free(subframe->reserve);
	free(subframe);
}

static void ref_reset(struct lte_ref *ref)
{
	cxvec_reset(ref->refs[0]);
	cxvec_reset(ref->refs[1]);
}

static void slot_reset(struct lte_slot *slot,
		       struct lte_ref_map **maps)
{
	ref_reset(&slot->refs[0]);
	ref_reset(&slot->refs[1]);

	assign_ref_maps(slot, maps);
}

int lte_subframe_reset(struct lte_subframe *subframe,
		       struct lte_ref_map **map0, struct lte_ref_map **map1)
{
	/* Set new values */
	subframe->assigned = 0;
	subframe->num_dci = 0;

	slot_reset(&subframe->slot[0], map0);
	slot_reset(&subframe->slot[1], map1);

	memset(subframe->reserve, 0, subframe->rbs * 12 * sizeof(int));

	return 0;
}

/*
 * Extract reference symbol for antenna 'p'
 *
 * Operate on the raw frequency domain output and not the sectionized block
 * vectors because those have not been allocated yet. So we use the original
 * reference map and not the computed within-block reference indices.
 */
static int lte_extract_pilots(struct lte_ref *ref, int p)
{
	int idx;
	int rbs = ref->sym->slot->rbs;
	int res = rbs * LTE_RB_LEN;
	int sym_len = lte_sym_len(rbs);

	struct lte_ref_map *map = ref->map[p];
	struct cxvec *refs = ref->refs[p];
	float complex first = 0.0;
	float complex last = 0.0;

	for (int i = 0; i < map->len; i++) {
		idx = map->k[i] + lte_rb_pos(rbs, 0);
		if (idx >= sym_len)
			idx = map->k[i] - res / 2 + lte_rb_pos_mid(rbs);

		refs->data[idx] = ref->sym->fd->data[idx] / map->a->data[i];

		if (i == 0)
			first = refs->data[idx];
		else if (i == map->len - 1)
			last = refs->data[idx];
	}

	/* Create lower virtual reference signals */
	idx = map->k[0] + lte_rb_pos(rbs, 0);
	refs->data[idx - 6] = first;
	refs->data[idx - 12] = first;
	refs->data[idx - 18] = first;

	/* Create upper virtual reference signals */
	idx = map->k[map->len - 1] - res / 2 + lte_rb_pos_mid(rbs);
	refs->data[idx + 6] = last;
	refs->data[idx + 12] = last;
	refs->data[idx + 18] = last;

	return 0;
}

/*
 * Compute channel magnitude
 *
 * Value is later used for normalizing the precoded values. Store the value in
 * the 'P' channel index, where 'P' is the number of transmit antennas at the
 * eNodeB.
 */
static int lte_combine_chan(struct lte_ref *ref, int ant)
{
	struct cxvec *chan_p0 = ref->chan[0];
	struct cxvec *chan_mag = ref->chan[2];

	for (int i = 0; i < chan_mag->len; i++) {
		float a, b;

		a = crealf(chan_p0->data[i]);
		b = cimagf(chan_p0->data[i]);
		chan_mag->data[i] = a * a + b * b;
	}

	if (ant != 2)
		return 0;

	struct cxvec *chan_p1 = ref->chan[1];

	for (int i = 0; i < chan_mag->len; i++) {
		float c, d;

		c = crealf(chan_p1->data[i]);
		d = cimagf(chan_p1->data[i]);
		chan_mag->data[i] += c * c + d * d;
	}


	return 0;
}

/*
 * Compute frequency offset from reference signals
 *
 * Second stage frequency offset correction used after successful PBCH decoding.
 * Prior to PBCH decode, PSS/SSS based correction is used for wider offset range.
 */
float lte_ofdm_offset(struct lte_subframe *subframe)
{
	if (!subframe->assigned)
		return 0.0f;

	struct lte_ref *ref0 = &subframe->slot[0].refs[0];
	struct lte_ref *ref1 = &subframe->slot[0].refs[1];
	struct lte_ref *ref2 = &subframe->slot[1].refs[0];
	struct lte_ref *ref3 = &subframe->slot[1].refs[1];

	int len = ref0->refs[0]->len;

	int a_cnt = 0, b_cnt= 0;

	double ang0 = 0.0f;
	double ang1 = 0.0f;

	for (int i = 0; i < len; i++) {
		for (int n = 0; n < 2; n++) {
			float complex a = ref0->refs[n]->data[i];
			float complex b = ref1->refs[n]->data[i];
			float complex c = ref2->refs[n]->data[i];
			float complex d = ref3->refs[n]->data[i];

			if ((cabsf(a) > 0.0f) && (cabsf(c) > 0.0f)) {
				float x = cargf(c) - cargf(a);
				if (x > M_PI)
					x = x - 2.0f * M_PI;
				else if (x < -M_PI)
					x = 2.0f * M_PI + x;

				ang0 += x;
				a_cnt++;
			}

			if ((cabsf(b) > 0.0f) && (cabsf(d) > 0.0f)) {
				float x = cargf(d) - cargf(b);
				if (x > M_PI)
					x = x - 2.0f * M_PI;
				else if (x < -M_PI)
					x = 2.0f * M_PI + x;

				ang1 += x;
				b_cnt++;
			}
		}
	}

	if (!a_cnt || !b_cnt)
		return 0.0;

	/* 1000 Hz based on reference symbol spacing of one slot */
	return ((ang1 / (float) b_cnt) +
	        (ang0 / (float) a_cnt)) / 2.0f / M_PI * 1000.0f;
}

static int avg_pilots(struct lte_subframe *subframe)
{
	struct lte_ref *ref0 = &subframe->slot[0].refs[0];
	struct lte_ref *ref1 = &subframe->slot[0].refs[1];
	struct lte_ref *ref2 = &subframe->slot[1].refs[0];
	struct lte_ref *ref3 = &subframe->slot[1].refs[1];

	int i, len = ref0->refs[0]->len;

	/* Antenna 0 */
	for (i = 0; i < len; i++) {
		ref0->refs[0]->data[i] += ref1->refs[0]->data[i];
		ref0->refs[0]->data[i] += ref2->refs[0]->data[i];
		ref0->refs[0]->data[i] += ref3->refs[0]->data[i];
		ref0->refs[0]->data[i] /= 2.0f;
	}

	if (subframe->tx_ants != 2)
		return 0;

	/* Antenna 1 */
	for (i = 0; i < len; i++) {
		ref0->refs[1]->data[i] += ref1->refs[1]->data[i];
		ref0->refs[1]->data[i] += ref2->refs[1]->data[i];
		ref0->refs[1]->data[i] += ref3->refs[1]->data[i];
		ref0->refs[1]->data[i] /= 2.0f;
	}

	return 0;
}

/*
 * Compute channel information
 *
 * Channel processing consists of reference symbol extraction, averaging, and
 * interpolation. Squared channel amplitude values are also stored.
 */
static int lte_slot_chan_recov(struct lte_slot *slot)
{
	int i, p;

	for (i = 0; i < 2; i++) {
		for (p = 0; p < 2; p++)
			lte_extract_pilots(&slot->refs[i], p);
	}

	return 0;
}

/*
 * Run the FFT
 *
 * Compute frequency domain symbols for all 7 time domain symbols. For resource
 * block combinations with split center resource blocks, re-map the center block
 * to be consistent with converted samples.
 */
static int lte_slot_convert(struct lte_subframe *subframe, int ns)
{
	int edge_rb;
	struct lte_slot *slot = &subframe->slot[ns];

	cxvec_fft(subframe->fft, slot->syms[0].td, slot->fd);

	switch (slot->rbs) {
	case 15:
		edge_rb = 7;
		break;
	case 25:
		edge_rb = 12;
		break;
	case 75:
		edge_rb = 37;
		break;
	default:
		edge_rb = 0;
	}

	if (edge_rb) {
		for (int l = 0; l < 7; l++)
			lte_sym_rb_map_special(&slot->syms[l], edge_rb);
	}

	return 0;
}

static int lte_subframe_convert_refs(struct lte_subframe *subframe)
{
	int i, p, edge_rb;
	struct lte_ref *ref0 = &subframe->slot[0].refs[0];

	for (i = 0; i < 2; i++) {
		lte_slot_convert(subframe, i);
		lte_slot_chan_recov(&subframe->slot[i]);
	}

	avg_pilots(subframe);
	p = 0;
	cxvec_interp(subframe->interp, ref0->refs[p], ref0->chan[p]);

	if (subframe->tx_ants == 2) {
		p = 1;
		cxvec_interp(subframe->interp, ref0->refs[p], ref0->chan[p]);
	}

	lte_combine_chan(ref0, subframe->tx_ants);

	switch (subframe->rbs) {
	case 15:
		edge_rb = 7;
		break;
	case 25:
		edge_rb = 12;
		break;
	case 75:
		edge_rb = 37;
		break;
	default:
		edge_rb = 0;
	}

	if (edge_rb) {
		lte_sym_chan_rb_map_special(ref0, edge_rb, 0);
		lte_sym_chan_rb_map_special(ref0, edge_rb, 1);
		lte_sym_chan_rb_map_special(ref0, edge_rb, 2);
	}

	subframe->assigned = 1;

	return 0;
}

int lte_subframe_convert(struct lte_subframe *subframe)
{
	if (subframe->assigned)
		return 0;

	return lte_subframe_convert_refs(subframe);
}
