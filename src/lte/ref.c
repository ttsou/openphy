/*
 * LTE Reference Signals
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
#include <math.h>

#include "gold.h"
#include "ref.h"
#include "sigvec.h"
#include "sigvec_internal.h"
#include "log.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

struct cxvec *lte_gen_ref(struct cxvec *gold)
{
	int len = 2 * LTE_N_MAX_DL_RB;
	float complex *c = gold->data;

	if (gold->len < 2 * len) {
		LOG_DSP_ARG("Invalid gold sequence length ", gold->len);
		return NULL;
	}

	struct cxvec *r = cxvec_alloc_simple(len);

	for (int m = 0; m < len; m++) {
		r->data[m] = 1.0 / sqrt(2.0) * (1.0 - 2.0 * c[2 * m + 0]) +
			     1.0 / sqrt(2.0) * (1.0 - 2.0 * c[2 * m + 1]) * I;
	}

	return r;
}

/* 3GPP 26.211 6.10 "Reference signals" */
unsigned int gen_c_init(int N_cp, int cell_id, int ns, int l)
{
	unsigned int c = 0;

	if ((l != 0) && (l != 4)) {
		LOG_DSP_ARG("Invalid symbol ", l);
		return -1;
	}
	if ((ns < 0) && (ns > 20)) {
		LOG_DSP_ARG("Invalid slot ", ns);
		return -1;
	}
	if ((N_cp != 0) && (N_cp != 1)) {
		LOG_DSP_ARG("Invalid cyclic prefix configuration ", N_cp);
		return -1;
	}

	c = 1024 * (7 * (ns + 1) + l + 1) * (2 * cell_id + 1) +
	    2 * cell_id + N_cp;

	return c;
}

static int compute_v(int p, int l, int ns)
{
	int v = -1;

	if ((p == 0) && (l == 0))
		v = 0;
	else if ((p == 0) && (l != 0))
		v = 3;
	else if ((p == 1) && (l == 0))
		v = 3;
	else if ((p == 1) && (l != 0))
		v = 0;
	else if (p == 2)
		v = 3 * (ns % 2);
	else if (p == 3)
		v = 3 + 3 * (ns % 2);

	return v;
}

static void gen_ak(struct lte_ref_map *map, int v,
		   int cell_id, struct cxvec *r)
{
	int rbs = map->rbs;
	int *k = map->k;
	float complex *a = map->a->data;

	map->v_shift = cell_id % 6;

	for (int m = 0; m < 2 * rbs; m++) {
		k[m] = 6 * m + (v + map->v_shift) % 6;
		a[m] = r->data[m + LTE_N_MAX_DL_RB - rbs];
	}
}

/* 3GPP TS 36.211 6.10.1.2 "Mapping to resource elements" */
static struct lte_ref_map *gen_ref_map(int cell_id, int l, int p,
				       int ns, struct cxvec *r, int rbs)
{
	struct lte_ref_map *map;

	if ((p != 0) && (p != 1)) {
		LOG_DSP_ARG("Unsupported antenna combination ", p);
		return NULL;
	}
	if ((l != 0) && (l != 4)) {
		LOG_DSP_ARG("Invalid symbol ", l);
		return NULL;
	}
	if ((ns < 0) || (ns > 19)) {
		LOG_DSP_ARG("Invalid slot ", ns);
		return NULL;
	}

	map = malloc(sizeof *map);
	if (!map) {
		LOG_DSP_ERR("Memory error");
		return NULL;
	}

	map->a = cxvec_alloc_simple(rbs * 2);
	map->rbs = rbs;
	map->k = malloc(rbs * 2 * sizeof *map->k);
	map->l = l;
	map->p = p;
	map->ns = ns;
	map->len = rbs * 2;

	/* Frequency position */
	int v = compute_v(p, l, ns);
	if (v < 0) {
		LOG_DSP_ERR("Invalid reference state");
		lte_free_ref_map(map);
		return NULL;
	}
	map->v = v;

	/* Fill out the reference map */
	gen_ak(map, v, cell_id, r);

	return map;
}

struct lte_ref_map *lte_gen_ref_map(int cell_id, int p,
				    int ns, int l, int rbs)
{
	unsigned c_init;
	struct lte_ref_map *map;
	struct cxvec *gold;
	struct cxvec *ref;

	c_init = gen_c_init(1, cell_id, ns, l);
	gold = lte_gen_gold_cxvec(c_init, 4 * LTE_N_MAX_DL_RB);
	ref = lte_gen_ref(gold);

	map = gen_ref_map(cell_id, l, p, ns, ref, rbs);

	cxvec_free(gold);
	cxvec_free(ref);

	return map;
}


void lte_free_ref_map(struct lte_ref_map *map)
{
	if (!map)
		return;

	cxvec_free(map->a);
	free(map->k);
	free(map);
}
