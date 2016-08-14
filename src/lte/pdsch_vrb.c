/*
 * LTE Physical Downlink Shared Channel (PDSCH)
 *     Virtual Resource Block (VRB)
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

#include "pdsch_riv.h"
#include "pdsch_vrb.h"
#include "log.h"

#define MIN(a,b) ((a) < (b) ? a : b)

/* 3GPP TS 36.211 Release 8: Table 6.2.3.2-1: RB gap values */
static int lte_dist_vrb_gap(int rbs, int gap)
{
	if ((gap != RIV_N_GAP1) && (gap != RIV_N_GAP2))
		return -1;

	if ((rbs >= 6) && (rbs <= 10))
		return (gap == RIV_N_GAP1) ? (int) ceilf((float) rbs / 2.0f) : -1;
	else if (rbs == 11)
		return (gap == RIV_N_GAP1) ? 4 : -1;
	else if ((rbs >= 12) && (rbs <= 19))
		return (gap == RIV_N_GAP1) ? 8 : -1;
	else if ((rbs >= 20) && (rbs <= 26))
		return (gap == RIV_N_GAP1) ? 12 : -1;
	else if ((rbs >= 27) && (rbs <= 44))
		return (gap == RIV_N_GAP1) ? 18 : -1;
	else if ((rbs >= 45) && (rbs <= 49))
		return (gap == RIV_N_GAP1) ? 27 : -1;
	else if ((rbs >= 50) && (rbs <= 63))
		return (gap == RIV_N_GAP1) ? 27 : 9;
	else if ((rbs >= 64) && (rbs <= 79))
		return (gap == RIV_N_GAP1) ? 32 : 16;
	else if ((rbs >= 80) && (rbs <= 110))
		return (gap == RIV_N_GAP1) ? 48 : 16;
	else
		return -1;
}

/*
 * 3GPP TS 36.211 Release 8: 6.2.3.2 "Virtual resource blocks of distributed type"
 */
int lte_dist_n_vrb(int rbs, int gap)
{
	int N_gap;

	N_gap = lte_dist_vrb_gap(rbs, gap);
	if (N_gap < 0)
		return -1;

	if (gap == RIV_N_GAP1)
		return 2 * MIN(N_gap, rbs - N_gap);
	else if (gap == RIV_N_GAP2)
		return rbs / (2 * N_gap) * 2 * N_gap;
	else
		return -1;
}

/*
 * 3GPP TS 36.211 Release 8: 6.2.3.2
 * "Virtual resource blocks of distributed type"
 */
int lte_dist_vrb_to_prb(int rbs, int ns, int gap,
			int vrbs, int rb_start, int *n_prb)
{
	char sbuf[80];
	int n_prb_tild[vrbs], n_vrb, n_vrb_tild;
	int P, N_dl_vrb, N_null, N_gap, N_dl_vrb_tild;
	int N_row, n_prb_tild_p, n_prb_tild_pp;

	N_gap = lte_dist_vrb_gap(rbs, gap);
	if (N_gap < 0) {
		LOG_PDSCH_ERR("Invalid gap value");
		return -1;
	}

	N_dl_vrb = lte_dist_n_vrb(rbs, gap);
	if (N_dl_vrb < 0) {
		LOG_PDSCH_ERR("Invalid gap value");
		return -1;
	}

	if ((rb_start < 0) || (rb_start >= N_dl_vrb)) {
		fprintf(stderr, "Resource: Invalid start index %i\n", rb_start);
		return -1;
	}

	snprintf(sbuf, 80, "PDSCH : rbs %i, gap %i, Ngap %i, nvrb %i, start %i",
		 rbs, gap, N_gap, N_dl_vrb, rb_start);
	LOG_DATA(sbuf);

	P = lte_ra_type0_p(rbs);
	if (P < 0) {
		LOG_PDSCH_ERR("Failed to obtain resource block group size (P)");
		return -1;
	}

	N_dl_vrb_tild = (gap == RIV_N_GAP1) ? N_dl_vrb : 2 * N_gap;
	N_row = (int) (ceilf((float) N_dl_vrb_tild /
		      (4.0f * (float) P)) * (float) P);
	N_null = 4 * N_row - N_dl_vrb_tild;

	for (int i = 0; i < vrbs; i++) {
		n_vrb = i + rb_start;
		n_vrb_tild = n_vrb % N_dl_vrb_tild;

		n_prb_tild_p = 2 * N_row * (n_vrb_tild % 2) + n_vrb_tild / 2 +
			       N_dl_vrb_tild * (n_vrb / N_dl_vrb_tild);

		n_prb_tild_pp = N_row * (n_vrb_tild % 4) + n_vrb_tild / 4 +
				N_dl_vrb_tild * (n_vrb / N_dl_vrb_tild);

		/* Even slot number */
		if (N_null && (n_vrb_tild >= N_dl_vrb_tild - N_null) &&
		    ((n_vrb_tild % 2) == 1)) {
			n_prb_tild[i] = n_prb_tild_p - N_row;
		} else if (N_null && (n_vrb_tild >= N_dl_vrb_tild - N_null) &&
			   ((n_vrb_tild % 2) == 0)) {
			n_prb_tild[i] = n_prb_tild_p - N_row + N_null / 2;
		} else if (N_null && (n_vrb_tild < N_dl_vrb_tild - N_null) &&
			   ((n_vrb_tild % 4) >= 2)) {
			n_prb_tild[i] = n_prb_tild_pp - N_null / 2;
		} else {
			n_prb_tild[i] = n_prb_tild_pp;
		}

		/* Odd slot number */
		if (ns % 2) {
			n_prb_tild[i] = (n_prb_tild[i] + N_dl_vrb_tild / 2) %
					N_dl_vrb_tild +
					N_dl_vrb_tild * (n_vrb / N_dl_vrb_tild);
		}

		if (n_prb_tild[i] < N_dl_vrb_tild / 2)
			n_prb[i] = n_prb_tild[i];
		else
			n_prb[i] = n_prb_tild[i] + N_gap - N_dl_vrb_tild / 2;
	}

	return 0;
}

/*
 * 3GPP TS 36.211 Release 8: 6.2.3.2 "Virtual resource blocks of local type"
 */
int lte_local_vrb_to_prb(int rbs, int vrbs, int rb_start, int *n_prb)
{
	int N_dl_vrb;

	N_dl_vrb = rbs;
	if ((rb_start < 0) || (rb_start >= N_dl_vrb)) {
		LOG_PDSCH_ERR("Invalid resource block start index");
		return -1;
	}

	for (int i = 0; i < vrbs; i++)
		n_prb[i] = (rb_start + i) % N_dl_vrb;

	return 0;
}
