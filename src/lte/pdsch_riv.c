/*
 * LTE Physical Downlink Shared Channel (PDSCH)
 *     Resource Indication Value (RIV)
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
#include <stdlib.h>

#include "dci.h"
#include "pdsch_vrb.h"
#include "pdsch_riv.h"
#include "log.h"

#define SI_RNTI		0xffff

static int compare(const void *a, const void *b)
{
	return (*(int *) a - *(int *) b);
}

static void log_riv(int riv, int start, int step, int crbs)
{
	char sbuf[80];

	snprintf(sbuf, 80, "PDSCH : RIV %i, start %i, N_step %i, L_crbs %i",
		 riv, start, step, crbs);
	LOG_DATA(sbuf);

}

/*
 * 3GPP TS 36.213 Release 8: Table 7.1.6.1-1
 * "Type 0 Resource RBG Size vs. Downlink System Bandwidth"
 */
int lte_ra_type0_p(int rbs)
{
	if (rbs < 0)
		return -1;
	else if (rbs <= 10)
		return 1;
	else if (rbs <= 26)
		return 2;
	else if (rbs <= 63)
		return 3;
	else if (rbs <= 110)
		return 4;
	else
		return -1;
}

/* 3GPP TS 36.213 Release 8: Table 7.1.6.1 "Resource allocation type 0" */
static int decode_ra_type0(int rbs, const struct lte_dci *dci,
			   struct lte_riv *riv)
{
	int i, n, P, bmp, bmp_size, rem = 0;
	int n_vrb = 0;

	P = lte_ra_type0_p(rbs);
	bmp = lte_dci_get_val(dci, LTE_DCI_FORMAT1_RB_ASSIGN);
	bmp_size = lte_dci_format1_type0_bmp_size(dci);

	if ((P < 0) || (bmp < 0) || (bmp_size < 1)) {
		LOG_PDSCH_ERR("Invalid parameter for type 0 allocation");
		return -1;
	}

	if (rbs % P)
		rem = rbs - P * (rbs / P);

	for (i = 0; i < bmp_size; i++) {
		if (((unsigned) bmp >> (bmp_size - i - 1)) & 0x01) {
			for (n = 0; n < P; n++) {
				if (rem && (i == bmp_size - 1) && (n >= rem))
					break;

				riv->prbs0[n_vrb] = P * i + n;
				riv->prbs1[n_vrb] = P * i + n;
				n_vrb++;
			}
		}
	}

	riv->n_vrb = n_vrb;

	return 0;
}

/* 3GPP TS 36.213 Release 8: Table 7.1.6.2 "Resource allocation type 1" */
static int decode_ra_type1(int rbs, const struct lte_dci *dci,
			   struct lte_riv *riv)
{
	int i, p, P, shift, bmp, bmp_size;
	int n_vrb = 0;

	P = lte_ra_type0_p(rbs);
	p = lte_dci_get_val(dci, LTE_DCI_FORMAT1_TYPE1_P);
	shift = lte_dci_get_val(dci, LTE_DCI_FORMAT1_TYPE1_SHIFT);
	bmp = lte_dci_get_val(dci, LTE_DCI_FORMAT1_TYPE1_BMP);
	bmp_size = lte_dci_format1_type1_bmp_size(dci);

	if ((P < 0) || (p < 0) || (shift < 0) || (bmp < 0) || (bmp_size < 0)) {
		LOG_PDSCH_ERR("Invalid parameter for type 1 allocation");
		return -1;
	}

	if (shift) {
		int N_rbg_subset;
		int limit = (rbs - 1 )/ P % P;

		if (p < limit)
			N_rbg_subset = (rbs - 1) / (P * P) * P + P;
		else if (p == limit)
			N_rbg_subset = (rbs - 1) / (P * P) * P + (rbs - 1) % P + 1;
		else
			N_rbg_subset = (rbs - 1) / (P * P) * P;

		shift = N_rbg_subset - bmp_size;
	}

	for (i = 0; i < bmp_size; i++) {
		if (((unsigned) bmp >> (bmp_size - i - 1)) & 0x01) {
			int blk = ((i + shift) / P) * P * P +
				  p * P + (i + shift) % P;

			riv->prbs0[n_vrb] = blk;
			riv->prbs1[n_vrb] = blk;
			n_vrb++;
		}
	}

	riv->n_vrb = n_vrb;

	return 0;
}

/*
 * 3GPP TS 36.213 Release 8: Table 7.1.6.2 "Resource allocation type 2"
 *
 * DCI formats 1A, 1B, and 1D
 */
static int decode_ra_type2(int rbs, int gap, int val,
			   int dist, struct lte_riv *riv)
{
	int i, n;
	int N_dl_rb, N_dl_vrb;
	int RB_start = -1, RB_start_max;
	int L_crbs = -1;

	N_dl_rb = rbs;
	N_dl_vrb = dist ? lte_dist_n_vrb(rbs, gap) : rbs;
	RB_start_max = N_dl_vrb;

	for (n = 0; n <= RB_start_max; n++) {
		for (i = 1; i <= N_dl_vrb - n; i++) {
			if (val == N_dl_rb * (i - 1) + n) {
				RB_start = n;
				L_crbs = i;
				break;
			}
			if (val == N_dl_rb *
				   (N_dl_rb - i + 1) +
				   (N_dl_rb - 1 - n)) {
				RB_start = n;
				L_crbs = i;
				break;
			}
		}
	}

	if ((L_crbs < 0) || (RB_start < 0)) {
		LOG_PDSCH_ERR("RIV value not found");
		return -1;
	}

	riv->offset = RB_start;
	riv->step = 0;
	riv->n_vrb = L_crbs;

	log_riv(val, RB_start, -1, L_crbs);

	return 0;
}

/*
 * 3GPP TS 36.213 Release 8: Table 7.1.6.2 "Resource allocation type 2"
 *
 * DCI format 1C
 */
static int decode_ra_type2_1c(int rbs, int gap, int val, struct lte_riv *riv)
{
	int N_step;
	int N_dl_vrb, N_dl_vrb_pr;
	int RB_start, RB_start_max, RB_start_pr = -1 ;
	int L_crbs, L_crbs_pr = -1;

	N_step = (rbs < 50) ? 2 : 4;
	N_dl_vrb = lte_dist_n_vrb(rbs, gap);
	N_dl_vrb_pr = N_dl_vrb / N_step;
	RB_start_max = (N_dl_vrb  / N_step - 1) * N_step;

	/* Fixme */
	for (int n = 0; n <= RB_start_max; n++) {
		for (int i = 1; i <= N_dl_vrb_pr - n; i++) {
			if (val == N_dl_vrb_pr * (i - 1) + n) {
				RB_start_pr = n;
				L_crbs_pr = i;
				break;
			}
			if (val == N_dl_vrb_pr *
				   (N_dl_vrb_pr - i + 1) +
				   (N_dl_vrb_pr - 1 - n)) {
				RB_start_pr = n;
				L_crbs_pr = i;
				break;
			}
		}
	}

	if ((L_crbs_pr < 0) || (RB_start_pr < 0)) {
		LOG_PDSCH_ERR("RIV value not found");
		return -1;
	}

	L_crbs = L_crbs_pr * N_step;
	RB_start = RB_start_pr * N_step;

	riv->offset = RB_start;
	riv->step = N_step;
	riv->n_vrb = L_crbs;

	log_riv(val, RB_start, N_step, L_crbs);

	return 0;
}

/*
 * 3GPP TS 36.213 Release 8: "Resource allocation for PDCCH DCI Format 0"
 * 
 * Simpler form of downlink resource allocation type 2
 */
static int decode_ra_ul_type0(int rbs, int val, struct lte_riv *riv)
{
	int i, n, N_ul_rb;
	int RB_start = -1, RB_start_max;
	int L_crbs = -1;

	N_ul_rb = rbs;
	RB_start_max = N_ul_rb;

	for (n = 0; n <= RB_start_max; n++) {
		for (i = 1; i <= N_ul_rb - n; i++) {
			if (val == N_ul_rb * (i - 1) + n) {
				RB_start = n;
				L_crbs = i;
				break;
			}
			if (val == N_ul_rb *
				   (N_ul_rb - i + 1) +
				   (N_ul_rb - 1 - n)) {
				RB_start = n;
				L_crbs = i;
				break;
			}
		}
	}

	if ((L_crbs < 0) || (RB_start < 0)) {
		LOG_PDSCH_ERR("RIV value not found");
		return -1;
	}

	riv->offset = RB_start;
	riv->step = -1;
	riv->n_vrb = L_crbs;

	log_riv(val, RB_start, -1, L_crbs);

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.1 "DCI format 0" */
static int lte_decode_riv_0(const struct lte_dci *dci, struct lte_riv *riv)
{
	int rc, val;
	int rbs = dci->rbs;

	if (dci->type != LTE_DCI_FORMAT0) {
		LOG_PDSCH_ERR("Invalid DCI format");
		return -1;
	}

	val = lte_dci_get_val(dci, LTE_DCI_FORMAT0_RB_ASSIGN);
	if (decode_ra_ul_type0(rbs, val, riv) < 0)
		return -1;

	return rc;
}


/* 3GPP TS 36.212 Release 8: 5.3.3.1.2 "DCI format 1" */
static int lte_decode_riv_1(const struct lte_dci *dci, struct lte_riv *riv)
{
	int rc, ra_hdr = 0;
	int rbs = dci->rbs;

	if (dci->type != LTE_DCI_FORMAT1) {
		LOG_PDSCH_ERR("Invalid DCI format");
		return -1;
	}

	if (rbs > 10)
		ra_hdr = lte_dci_get_val(dci, LTE_DCI_FORMAT1_RA_HDR);

	if (!ra_hdr)
		rc = decode_ra_type0(rbs, dci, riv);
	else
		rc = decode_ra_type1(rbs, dci, riv);

	return rc;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4 "DCI format 1C" */
static int lte_decode_riv_1c(const struct lte_dci *dci, struct lte_riv *riv)
{
	int val, N_gap = 0;
	int rbs = dci->rbs;

	val = lte_dci_get_val(dci, LTE_DCI_FORMAT1C_RB_ASSIGN);
	if (val < 0) {
		LOG_PDSCH_ERR("Invalid DCI format");
		return -1;
	}

	N_gap = lte_dci_get_val(dci, LTE_DCI_FORMAT1C_GAP);
	if ((N_gap < 0) && (N_gap > 1)) {
		LOG_PDSCH_ERR("Invalid gap value");
		return -1;
	}

	if (decode_ra_type2_1c(rbs, N_gap, val, riv) < 0)
		return -1;

	lte_dist_vrb_to_prb(rbs, 0, N_gap, riv->n_vrb, riv->offset, riv->prbs0);
	lte_dist_vrb_to_prb(rbs, 1, N_gap, riv->n_vrb, riv->offset, riv->prbs1);

	qsort(riv->prbs0, riv->n_vrb, sizeof(int), compare);
	qsort(riv->prbs1, riv->n_vrb, sizeof(int), compare);

	return 0;
}

/*
 * 3GPP TS 36.212 Release 8: 5.3.3.1 "DCI formats"
 *     Shared type 2 resource allocation handling for Formats 1A, 1B, and 1D
 */
static int lte_decode_riv_1x(const struct lte_dci *dci, struct lte_riv *riv,
			     int dst, int gap, int ra)
{
	int rbs = dci->rbs;

	if (decode_ra_type2(rbs, gap, ra, dst, riv) < 0)
		return -1;

	if (dst) {
		lte_dist_vrb_to_prb(rbs, 0, gap, riv->n_vrb,
				    riv->offset, riv->prbs0);
		lte_dist_vrb_to_prb(rbs, 1, gap, riv->n_vrb,
				    riv->offset, riv->prbs1);
	} else {
		lte_local_vrb_to_prb(rbs, riv->n_vrb, riv->offset, riv->prbs0);
		lte_local_vrb_to_prb(rbs, riv->n_vrb, riv->offset, riv->prbs1);
	}

	qsort(riv->prbs0, riv->n_vrb, sizeof(int), compare);
	qsort(riv->prbs1, riv->n_vrb, sizeof(int), compare);

	return 0;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3 "DCI format 1A" */
static int lte_decode_riv_1a(const struct lte_dci *dci, struct lte_riv *riv)
{
	int ra, dst, gap = 0;

	ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1A_RB_ASSIGN);
	dst = lte_dci_get_val(dci, LTE_DCI_FORMAT1A_LOCAL_VRB);

	if (dst) {
		if ((dci->rbs < 50) || (dci->rnti == SI_RNTI)) {
			gap = lte_dci_get_val(dci, LTE_DCI_FORMAT1A_NDI);
		} else {
			gap = lte_dci_get_val(dci, LTE_DCI_FORMAT1A_DIST_GAP);
			ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1A_DIST_RA);
		}
	}

	return lte_decode_riv_1x(dci, riv, dst, gap, ra);
}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3A "DCI format 1B" */
static int lte_decode_riv_1b(const struct lte_dci *dci, struct lte_riv *riv)
{
	int dst, ra, gap = 0;

	dst = lte_dci_get_val(dci, LTE_DCI_FORMAT1B_LOCAL_VRB);

	if (dst && (dci->rbs >= 50)) {
		ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1B_DIST_RA);
		gap = lte_dci_get_val(dci, LTE_DCI_FORMAT1B_DIST_GAP);
	} else {
		ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1B_RB_ASSIGN);
	}

	return lte_decode_riv_1x(dci, riv, dst, gap, ra);
}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4A "DCI format 1D" */
static int lte_decode_riv_1d(const struct lte_dci *dci, struct lte_riv *riv)
{
	int dst, ra, gap = 0;

	dst = lte_dci_get_val(dci, LTE_DCI_FORMAT1D_LOCAL_VRB);

	if (dst && (dci->rbs >= 50)) {
		ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1D_DIST_RA);
		gap = lte_dci_get_val(dci, LTE_DCI_FORMAT1D_DIST_GAP);
	} else {
		ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1D_RB_ASSIGN);
	}

	return lte_decode_riv_1x(dci, riv, dst, gap, ra);
}

/* 3GPP TS 36.212 Release 8: 5.3.3 "Downlink control information" */
int lte_decode_riv(int rbs, const struct lte_dci *dci, struct lte_riv *riv)
{
	if (!dci || !riv) {
		LOG_PDSCH_ERR("Invalid DCI or RIV parameter");
		return -1;
	}

	switch (dci->type) {
	case LTE_DCI_FORMAT0:
		return lte_decode_riv_0(dci, riv);
	case LTE_DCI_FORMAT1:
		return lte_decode_riv_1(dci, riv);
	case LTE_DCI_FORMAT1A:
		return lte_decode_riv_1a(dci, riv);
	case LTE_DCI_FORMAT1B:
		return lte_decode_riv_1b(dci, riv);
	case LTE_DCI_FORMAT1C:
		return lte_decode_riv_1c(dci, riv);
	case LTE_DCI_FORMAT1D:
		return lte_decode_riv_1d(dci, riv);
	}

	LOG_PDSCH_ERR("Unhandled DCI format");
	return -1;
}
