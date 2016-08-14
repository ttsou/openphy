/*
 * LTE Physical Hybrid ARQ Channel (PHICH)
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
#include <math.h>
#include "phich.h"
#include "si.h"
#include "log.h"
#include "slot.h"

#define PHICH_MAX_GRPS		25

static int phich_grp_table[6][4][2];

static int phich_rbs_index(int rbs, int ng, int dur)
{
	if (dur == LTE_PHICH_DUR_EXT) {
		LOG_PHICH_ERR("Extended duration not supported");
		return -1;
	}

	if (ng > LTE_PHICH_GRP_TWO) {
		LOG_PHICH_ERR("Invalid group duration");
		return -1;
	}

	switch (rbs) {
	case 6:
		return 0;
	case 15:
		return 1;
	case 25:
		return 2;
	case 50:
		return 3;
	case 75:
		return 4;
	case 100:
		return 5;
	}

	LOG_PHICH_ERR("Invalid number of resource blocks");
	return -1;
}

/* 3GPP TS 36.211 Release 8: 6.9 Physical hybrid ARQ indicator channel */
int lte_phich_num_groups(int rbs, int ng, int dur)
{
	int index = phich_rbs_index(rbs, ng, dur);
	if (index < 0)
		return -1;

	return phich_grp_table[index][ng][dur];
}

/* 3GPP TS 36.211 Release 8: 6.9.3 Mapping to reource elements */
int lte_gen_phich_indices(int *res, int rbs, int n_cell_id, int ng, int dur)
{
	int n_bar[PHICH_MAX_GRPS];
	int n_base, groups;
	int n_l_prime = rbs * 2 - 4;

	groups = lte_phich_num_groups(rbs, ng, dur);
	if (groups < 0)
		return -1;

	for (int i = 0; i < 3; i++) {
		for (int n = 0; n < groups; n++) {
			n_base = n_cell_id + n;
			n_bar[n] = (n_base + i * n_l_prime / 3) % n_l_prime;

			int cnt = 0;
			int shift = n_cell_id % 3;

			for (int q = 0; q < rbs * 2; q++) {
				if (res[q * 6 + shift + 1] == 1)
					continue;

				if (cnt++ == n_bar[n]) {
					if (shift == 0) {
						res[6 * q + 1] = 2;
						res[6 * q + 2] = 2;
						res[6 * q + 4] = 2;
						res[6 * q + 5] = 2;
					} else if (shift == 1) {
						res[6 * q + 0] = 2;
						res[6 * q + 2] = 2;
						res[6 * q + 3] = 2;
						res[6 * q + 5] = 2;
					} else if (shift == 2) {
						res[6 * q + 0] = 2;
						res[6 * q + 1] = 2;
						res[6 * q + 3] = 2;
						res[6 * q + 4] = 2;
					}
					break;
				}
			}
		}
	}

	return 0;
}

static __attribute__((constructor)) void init_phich_table()
{
	const int rbs[6] = { 6, 15, 25, 50, 75, 100 };
	float num;

	for (int i = 0; i < 6; i++) {
		num = (float) rbs[i] / 8.0f;

		phich_grp_table[i][LTE_PHICH_GRP_SIXTH][LTE_PHICH_DUR_NORMAL] =
			(int) ceilf(num / 6);
		phich_grp_table[i][LTE_PHICH_GRP_HALF][LTE_PHICH_DUR_NORMAL] =
			(int) ceilf(num / 2);
		phich_grp_table[i][LTE_PHICH_GRP_ONE][LTE_PHICH_DUR_NORMAL] =
			(int) ceilf(num * 1);
		phich_grp_table[i][LTE_PHICH_GRP_TWO][LTE_PHICH_DUR_NORMAL] =
			(int) ceilf(num * 2);

		phich_grp_table[i][LTE_PHICH_GRP_SIXTH][LTE_PHICH_DUR_EXT] = -1;
		phich_grp_table[i][LTE_PHICH_GRP_HALF][LTE_PHICH_DUR_EXT] = -1;
		phich_grp_table[i][LTE_PHICH_GRP_ONE][LTE_PHICH_DUR_EXT] = -1;
		phich_grp_table[i][LTE_PHICH_GRP_TWO][LTE_PHICH_DUR_EXT] = -1;
	}
}
