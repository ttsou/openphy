/*
 * LTE Secondary Synchronization Signal (SSS)
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
#include <math.h>

#include "sss.h"
#include "sigvec.h"
#include "sigvec_internal.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

enum sss_var_type {
	SSS_S_TILDA,
	SSS_C_TILDA,
	SSS_Z_TILDA,
};

static void gen_tilda(int *tilda, enum sss_var_type type)
{
	int x[31] = { 0, 0, 0, 0, 1 };

	switch (type) {
	case SSS_S_TILDA:
		for (int i = 5; i < 31; i++)
			x[i] = (x[i - 5 + 2] + x[i - 5]) % 2;
		break;
	case SSS_C_TILDA:
		for (int i = 5; i < 31; i++)
			x[i] = (x[i - 5 + 3] + x[i - 5]) % 2;
		break;
	case SSS_Z_TILDA:
		for (int i = 5; i < 31; i++)
			x[i] = (x[i - 5 + 4] + x[i - 5 + 2] +
				x[i - 5 + 1] + x[i - 5 + 0]) % 2;
	}

	for (int i = 0; i < 31; i++)
		tilda[i] = 1 - 2 * x[i];

	return;
}

static int gen_m(unsigned n_id_1, int num)
{
	int mp, q, qp;

	qp = n_id_1 / 30;
	q = (n_id_1 + qp * (qp + 1) / 2) / 30;
	mp = n_id_1 + q * (q + 1) / 2;

	if (num == 0)
		return mp % 31;
	else
		return (mp % 31 + mp / 31 + 1) % 31;
}

struct lte_sss *lte_gen_sss(unsigned n_id_1, unsigned n_id_2)
{
	int m0, m1;
	int ct[31], c0[31], c1[31];
	int st[31], s0_m0[31], s1_m1[31];
	int zt[31], z1_m0[31], z1_m1[31];
	struct lte_sss *sss;

	m0 = gen_m(n_id_1, 0);
	m1 = gen_m(n_id_1, 1);

	gen_tilda(st, SSS_S_TILDA);
	gen_tilda(ct, SSS_C_TILDA);
	gen_tilda(zt, SSS_Z_TILDA);

	for (int n = 0; n < 31; n++) {
		s0_m0[n] = st[(n + m0) % 31];
		s1_m1[n] = st[(n + m1) % 31];

		c0[n] = ct[(n + n_id_2 + 0) % 31];
		c1[n] = ct[(n + n_id_2 + 3) % 31];

		z1_m0[n] = zt[(n + (m0 % 8)) % 31];
		z1_m1[n] = zt[(n + (m1 % 8)) % 31];
	}

	sss = malloc(sizeof *sss);
	if (!sss) {
		return NULL;
	}

	sss->d0 = cxvec_alloc_simple(62);
	if (!sss->d0) {
		free(sss);
		return NULL;
	}

	sss->d5 = cxvec_alloc_simple(62);
	if (!sss->d5) {
		cxvec_free(sss->d0);
		free(sss);
		return NULL;
	}

	for (int n = 0; n < 31; n++) {
		sss->d0->data[2 * n] = (float) (s0_m0[n] * c0[n]);
		sss->d5->data[2 * n] = (float) (s1_m1[n] * c0[n]);

		sss->d0->data[2 * n + 1] = (float) (s1_m1[n] * c1[n] * z1_m0[n]);
		sss->d5->data[2 * n + 1] = (float) (s0_m0[n] * c1[n] * z1_m1[n]);
	}

	return sss;
}
