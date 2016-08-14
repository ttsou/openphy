/*
 * LTE downlink control information (DCI)
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
#include "dci.h"
#include "dci_formats.h"
#include "log.h"

/*
 * DCI sizes for 6 resource block combinations and 2 modes
 *
 * Resource blocks: 6, 15, 25, 50, 75, 100
 * Operating modes: FDD, TDD
 */
static int dci_sizes[2][6][LTE_DCI_NUM_FORMATS];

struct type_string {
	unsigned type;
	const char *str;
};

static const struct type_string dci_type_desc[LTE_DCI_NUM_FORMATS] = {
	{ LTE_DCI_FORMAT0,	"DCI Format 0" },
	{ LTE_DCI_FORMAT1,	"DCI Format 1" },
	{ LTE_DCI_FORMAT1A,	"DCI Format 1A" },
	{ LTE_DCI_FORMAT1B,	"DCI Format 1B" },
	{ LTE_DCI_FORMAT1C,	"DCI Format 1C" },
	{ LTE_DCI_FORMAT1D,	"DCI Format 1D" },
	{ LTE_DCI_FORMAT2,	"DCI Format 2" },
	{ LTE_DCI_FORMAT2A,	"DCI Format 2A" },
	{ LTE_DCI_FORMAT3,	"DCI Format 3" },
	{ LTE_DCI_FORMAT3A,	"DCI Format 3A" },
};

static const struct type_string format0_desc[LTE_DCI_FORMAT0_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT0_FORMAT,	"Format 0/1A differentiation" },
	{ LTE_DCI_FORMAT0_HOPPING,	"Hopping flag" },
	{ LTE_DCI_FORMAT0_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT0_MOD,		"Modulation and coding" },
	{ LTE_DCI_FORMAT0_NDI,		"New data indicator" },
	{ LTE_DCI_FORMAT0_TPC,		"TPC command for scheduled PUSCH" },
	{ LTE_DCI_FORMAT0_CYC_SHIFT,	"Cyclic shift" },
	{ LTE_DCI_FORMAT0_DAI,		"Downlink assignment / UL index" },
	{ LTE_DCI_FORMAT0_CQI_REQ,	"CQI Request" },
};

static const struct type_string format1_desc[LTE_DCI_FORMAT1_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT1_RA_HDR,	"Resource allocation header" },
	{ LTE_DCI_FORMAT1_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT1_MOD,		"Modulation and coding" },
	{ LTE_DCI_FORMAT1_HARQ,		"HARQ process number" },
	{ LTE_DCI_FORMAT1_NDI,		"New data indicator" },
	{ LTE_DCI_FORMAT1_RV,		"Redundancy version" },
	{ LTE_DCI_FORMAT1_TPC,		"TPC command for PUCCH" },
	{ LTE_DCI_FORMAT1_DAI,		"Downlink assignment index" },
};

static const struct type_string format1a_desc[LTE_DCI_FORMAT1A_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT1A_FORMAT,	"Format 0/1A differentiation" },
	{ LTE_DCI_FORMAT1A_LOCAL_VRB,	"Local/distributed VRB assignment" },
	{ LTE_DCI_FORMAT1A_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT1A_MOD,		"Modulation and coding" },
	{ LTE_DCI_FORMAT1A_HARQ,	"HARQ process number" },
	{ LTE_DCI_FORMAT1A_NDI,		"New data indicator" },
	{ LTE_DCI_FORMAT1A_RV,		"Redundancy version" },
	{ LTE_DCI_FORMAT1A_TPC,		"TPC command for PUCCH" },
	{ LTE_DCI_FORMAT1A_DAI,		"Downlink assignment index" },
};

static const struct type_string format1b_desc[LTE_DCI_FORMAT1B_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT1B_LOCAL_VRB,	"Local/distributed VRB assignment" },
	{ LTE_DCI_FORMAT1B_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT1B_MOD,		"Modulation and coding" },
	{ LTE_DCI_FORMAT1B_HARQ,	"HARQ process number" },
	{ LTE_DCI_FORMAT1B_NDI,		"New data indicator" },
	{ LTE_DCI_FORMAT1B_RV,		"Redundancy version" },
	{ LTE_DCI_FORMAT1B_TPC,		"TPC command for PUCCH" },
	{ LTE_DCI_FORMAT1B_DAI,		"Downlink assignment index" },
	{ LTE_DCI_FORMAT1B_TPMI,	"TPMI information for precoding" },
	{ LTE_DCI_FORMAT1B_PMI,		"PMI confirmation for precoding" },
};

static const struct type_string format1c_desc[LTE_DCI_FORMAT1C_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT1C_GAP,		"Gap value" },
	{ LTE_DCI_FORMAT1C_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT1C_BLK_SIZE,	"Transport block size (TBS) index" },
};

static const struct type_string format1d_desc[LTE_DCI_FORMAT1D_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT1D_LOCAL_VRB,	"Local/distributed VRB assignment" },
	{ LTE_DCI_FORMAT1D_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT1D_MOD,		"Modulation and coding" },
	{ LTE_DCI_FORMAT1D_HARQ,	"HARQ process number" },
	{ LTE_DCI_FORMAT1D_NDI,		"New data indicator" },
	{ LTE_DCI_FORMAT1D_RV,		"Redundancy version" },
	{ LTE_DCI_FORMAT1D_TPC,		"TPC command for PUCCH" },
	{ LTE_DCI_FORMAT1D_DAI,		"Downlink assignment index" },
	{ LTE_DCI_FORMAT1D_TPMI,	"TPMI information for precoding" },
	{ LTE_DCI_FORMAT1D_DL_PWR_OFFSET, "Downlink power offset" },
};

static const struct type_string format2_desc[LTE_DCI_FORMAT2_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT2_RA_HDR,	"Resource allocation header" },
	{ LTE_DCI_FORMAT2_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT2_TPC,		"TPC command for PUCCH" },
	{ LTE_DCI_FORMAT2_DAI,		"Downlink assignment index" },
	{ LTE_DCI_FORMAT2_HARQ,		"HARQ process number" },
	{ LTE_DCI_FORMAT2_SWAP,		"Transport block to codeword swap" },
	{ LTE_DCI_FORMAT2_NDI,		"New data indicator" },
	{ LTE_DCI_FORMAT2_RV,		"Redundancy version" },
	{ LTE_DCI_FORMAT2_PRECODING,	"Precoding information" },
};

static const struct type_string format2a_desc[LTE_DCI_FORMAT2A_NUM_FIELDS] = {
	{ LTE_DCI_FORMAT2A_RA_HDR,	"Resource allocation header" },
	{ LTE_DCI_FORMAT2A_RB_ASSIGN,	"Resource block assignment" },
	{ LTE_DCI_FORMAT2A_TPC,		"TPC command for PUCCH" },
	{ LTE_DCI_FORMAT2A_DAI,		"Downlink assignment index" },
	{ LTE_DCI_FORMAT2A_HARQ,	"HARQ process number" },
	{ LTE_DCI_FORMAT2A_SWAP,	"Transport block to codeword swap" },
	{ LTE_DCI_FORMAT2A_MOD_1,	"Block 1: Modulation and coding" },
	{ LTE_DCI_FORMAT2A_NDI_1,	"Block 1: New data indicator" },
	{ LTE_DCI_FORMAT2A_RV_1,	"Block 1: Redundancy version" },
	{ LTE_DCI_FORMAT2A_MOD_2,	"Block 2: Modulation and coding" },
	{ LTE_DCI_FORMAT2A_NDI_2,	"Block 2: New data indicator" },
	{ LTE_DCI_FORMAT2A_RV_2,	"Block 2: Redundancy version" },
	{ LTE_DCI_FORMAT2A_PRECODING,	"Precoding information" },
};

static const struct type_string format3_desc[LTE_DCI_FORMAT3_NUM_FIELDS] = {
};


static const struct type_string format3a_desc[LTE_DCI_FORMAT3A_NUM_FIELDS] = {
};

static int rb_to_index(int rbs)
{
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

	return -1;
}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.2-1: Ambiguous sizes of information bits */
static int chk_ambiguous_bits(int size)
{
	const int ambig[10] = { 12, 14, 16, 20, 24, 26, 32, 40, 44, 56 };

	for (int i = 0; i < 10; i++) {
		if (size == ambig[i])
			return 1;
	}

	return 0;
}

/*
 * Load the DCI format size table for all combinations and
 * adjust sizes to resolve abiguities.
 *
 * 3GPP TS 36.212 Release 8:
 * Section(s) 5.3.3.1.1 DCI Format 0
 *            5.3.3.1.2 DCI Format 1
 *            5.3.3.1.3 DCI Format 1A
 */
static void dci_setup_sizes(int rbs, int mode)
{
	int index = rb_to_index(rbs);

	for (int n = 0; n < LTE_DCI_NUM_FORMATS; n++)
		dci_sizes[mode][index][n] = dci_stored_size(rbs, mode, n);

	int form0 = dci_sizes[mode][index][LTE_DCI_FORMAT0];
	int form1a = dci_sizes[mode][index][LTE_DCI_FORMAT1A];
	int form1 = dci_sizes[mode][index][LTE_DCI_FORMAT1];
	int form2 = dci_sizes[mode][index][LTE_DCI_FORMAT2];

	if (form1a < form0) {
		form1a = form0;
	} else {
		if (chk_ambiguous_bits(form1a))
			form1a++;

		if (form0 < form1a)
			form0 = form1a;
	}

	while (chk_ambiguous_bits(form1) || (form1 == form1a))
		form1++;

	if (chk_ambiguous_bits(form2))
		form2++;

	dci_sizes[mode][index][LTE_DCI_FORMAT0] = form0;
	dci_sizes[mode][index][LTE_DCI_FORMAT1A] = form1a;
	dci_sizes[mode][index][LTE_DCI_FORMAT1] = form1;
	dci_sizes[mode][index][LTE_DCI_FORMAT2] = form2;
}

int lte_dci_format_size(int rbs, int mode, int type)
{
	if ((mode != LTE_MODE_FDD) && (mode != LTE_MODE_TDD))
		return -1;

	if ((type < 0) || (type >= LTE_DCI_NUM_FORMATS))
		return -1;

	int index = rb_to_index(rbs);
	if (index < 0)
		return -1;

	return dci_sizes[mode][index][type];
}

int lte_dci_get_type(int rbs, int mode, int len)
{
	for (int i = 0; i < LTE_DCI_NUM_FORMATS; i++) {
		if (len == lte_dci_format_size(rbs, mode, i))
			return i;
	}

	return -1;
}

static int dci_decode_fdd(struct lte_dci *dci, int rbs,
			  int type, const unsigned char *bits)
{
	switch (type) {
	case LTE_DCI_FORMAT0:
		dci_decode_format0_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1:
		dci_decode_format1_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1A:
		dci_decode_format1a_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1B:
		dci_decode_format1b_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1C:
		dci_decode_format1c_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1D:
		dci_decode_format1d_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT2:
		dci_decode_format2_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT2A:
		dci_decode_format2a_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT3:
		dci_decode_format3_fdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT3A:
		dci_decode_format3a_fdd(rbs, dci->vals, bits);
		break;
	default:
		return -1;
	}

	return 0;
}

static int dci_decode_tdd(struct lte_dci *dci, int rbs,
			  int type, const unsigned char *bits)
{
	switch (type) {
	case LTE_DCI_FORMAT0:
		dci_decode_format0_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1:
		dci_decode_format1_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1A:
		dci_decode_format1a_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1B:
		dci_decode_format1b_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1C:
		dci_decode_format1c_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT1D:
		dci_decode_format1d_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT2:
		dci_decode_format2_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT2A:
		dci_decode_format2a_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT3:
		dci_decode_format3_tdd(rbs, dci->vals, bits);
		break;
	case LTE_DCI_FORMAT3A:
		dci_decode_format3a_tdd(rbs, dci->vals, bits);
		break;
	default:
		return -1;
	}

	return 0;
}

int lte_dci_decode(struct lte_dci *dci, int rbs, int mode,
		   const unsigned char *bits, int len, uint16_t rnti)
{
	if (!bits || !dci)
		return -1;

	int type = lte_dci_get_type(rbs, mode, len);
	if (type < 0) {
		fprintf(stderr, "DCI: No match for length %i\n", len);
		return -1;
	}

	if ((type == LTE_DCI_FORMAT0) & bits[0])
		type = LTE_DCI_FORMAT1A;

	if (mode == LTE_MODE_FDD)
		dci_decode_fdd(dci, rbs, type, bits);
	else
		dci_decode_tdd(dci, rbs, type, bits);

	dci->type = type;
	dci->mode = mode;
	dci->rbs = rbs;
	dci->rnti = rnti;

	return type;
}

int lte_dci_get_mod(const struct lte_dci *dci)
{
	if (!dci)
		return -1;

	switch (dci->type) {
	case LTE_DCI_FORMAT0:
		return lte_dci_get_val(dci, LTE_DCI_FORMAT0_MOD);
	case LTE_DCI_FORMAT1:
		return lte_dci_get_val(dci, LTE_DCI_FORMAT1_MOD);
	case LTE_DCI_FORMAT1A:
		return lte_dci_get_val(dci, LTE_DCI_FORMAT1A_MOD);
	case LTE_DCI_FORMAT1B:
		return lte_dci_get_val(dci, LTE_DCI_FORMAT1B_MOD);
	case LTE_DCI_FORMAT1C:
		return lte_dci_get_val(dci, LTE_DCI_FORMAT1C_BLK_SIZE);
	case LTE_DCI_FORMAT1D:
		return lte_dci_get_val(dci, LTE_DCI_FORMAT1D_MOD);
	case LTE_DCI_FORMAT2:
		break;
	case LTE_DCI_FORMAT2A:
		break;
	}

	fprintf(stderr, "DCI: Unhandled type %i\n", dci->type);
	return -1;
}

void lte_dci_print_sizes(int rbs, int mode)
{
	if ((mode != LTE_MODE_FDD) && (mode != LTE_MODE_TDD)) {
		fprintf(stderr, "Invalid LTE mode %i\n", mode);
		return;
	}

	fprintf(stdout, "LTE DCI Formats for %i resource blocks\n", rbs);
	for (int i = 0; i < LTE_DCI_NUM_FORMATS; i++) {
		fprintf(stdout, "%s\n", dci_type_desc[i].str);
		fprintf(stdout, "    Size......... %i\n",
			lte_dci_format_size(rbs, mode, i));
	}
}

#define DCI_PRINT_FORMAT(N,M) \
	void dci_print_format##N(const struct lte_dci *dci) \
	{ \
		char sbuf[80]; \
		snprintf(sbuf, 80, "PDCCH : %s:", \
			 dci_type_desc[dci->type].str); \
		LOG_CTRL(sbuf); \
		fprintf(stdout, "%s", LOG_COLOR_CTRL); \
		for (int i = 0; i < LTE_DCI_FORMAT##M##_NUM_FIELDS; i++) { \
			if (dci->vals[i] < 0) \
				continue; \
			fprintf(stdout, "                     %-40s %i\n", \
				format##N##_desc[i].str, dci->vals[i]); \
		} \
		fprintf(stdout, "%s", LOG_COLOR_NONE); \
	}

DCI_PRINT_FORMAT(0,0)
DCI_PRINT_FORMAT(1,1)
DCI_PRINT_FORMAT(1a,1A)
DCI_PRINT_FORMAT(1b,1B)
DCI_PRINT_FORMAT(1c,1C)
DCI_PRINT_FORMAT(1d,1D)
DCI_PRINT_FORMAT(2,2)
DCI_PRINT_FORMAT(2a,2A)
DCI_PRINT_FORMAT(3,3)
DCI_PRINT_FORMAT(3a,3A)

static int lte_dci_num_fields(int type)
{
	switch (type) {
	case LTE_DCI_FORMAT0:
		return LTE_DCI_FORMAT0_NUM_FIELDS;
	case LTE_DCI_FORMAT1:
		return LTE_DCI_FORMAT1_NUM_FIELDS;
	case LTE_DCI_FORMAT1A:
		return LTE_DCI_FORMAT1A_NUM_FIELDS;
	case LTE_DCI_FORMAT1B:
		return LTE_DCI_FORMAT1B_NUM_FIELDS;
	case LTE_DCI_FORMAT1C:
		return LTE_DCI_FORMAT1C_NUM_FIELDS;
	case LTE_DCI_FORMAT1D:
		return LTE_DCI_FORMAT1D_NUM_FIELDS;
	case LTE_DCI_FORMAT2:
		return LTE_DCI_FORMAT2_NUM_FIELDS;
	case LTE_DCI_FORMAT2A:
		return LTE_DCI_FORMAT2A_NUM_FIELDS;
	case LTE_DCI_FORMAT3:
		return LTE_DCI_FORMAT3_NUM_FIELDS;
	case LTE_DCI_FORMAT3A:
		return LTE_DCI_FORMAT3A_NUM_FIELDS;
	default:
		break;
	}

	return -1;
}

int lte_dci_compare(const struct lte_dci *dci0, const struct lte_dci *dci1)
{
	if (!dci0 || !dci1)
		return -1;

	if (dci0->type != dci1->type)
		return 1;
	if (dci0->mode != dci1->mode)
		return 1;
	if (dci0->rnti != dci1->rnti)
		return 1;

	for (int i = 0; i < lte_dci_num_fields(dci0->type); i++) {
		if (dci0->vals[i] != dci1->vals[i])
			return 1;
	}

	return 0;
}

void lte_dci_print(const struct lte_dci *dci)
{
	if (!dci)
		return;

	switch (dci->type) {
	case LTE_DCI_FORMAT0:
		return dci_print_format0(dci);
	case LTE_DCI_FORMAT1:
		return dci_print_format1(dci);
	case LTE_DCI_FORMAT1A:
		return dci_print_format1a(dci);
	case LTE_DCI_FORMAT1B:
		return dci_print_format1b(dci);
	case LTE_DCI_FORMAT1C:
		return dci_print_format1c(dci);
	case LTE_DCI_FORMAT1D:
		return dci_print_format1d(dci);
	case LTE_DCI_FORMAT2:
		return dci_print_format2(dci);
	case LTE_DCI_FORMAT2A:
		return dci_print_format2a(dci);
	case LTE_DCI_FORMAT3:
		return dci_print_format3(dci);
	case LTE_DCI_FORMAT3A:
		return dci_print_format3a(dci);
	}

	LOG_PDCCH_ERR("Invalid DCI format");
}

static int format1_type0_ra_size(int rbs)
{
	switch (rbs) {
	case 6:
		return DCI_FORMAT1_TYPE0_RA6;
	case 15:
		return DCI_FORMAT1_TYPE0_RA15;
	case 25:
		return DCI_FORMAT1_TYPE0_RA25;
	case 50:
		return DCI_FORMAT1_TYPE0_RA50;
	case 75:
		return DCI_FORMAT1_TYPE0_RA75;
	case 100:
		return DCI_FORMAT1_TYPE0_RA100;
	}

	return -1;
}

/* Format 1 resource allocation Type 0 bitmap size */
int lte_dci_format1_type0_bmp_size(const struct lte_dci *dci)
{
	if (!dci || (dci->type != LTE_DCI_FORMAT1))
		return -1;
	if ((dci->rbs > 10) && lte_dci_get_val(dci, LTE_DCI_FORMAT1_RA_HDR))
		return -1;

	return format1_type0_ra_size(dci->rbs);
}

/* Format 1 resource allocation Type 1 bitmap size */
int lte_dci_format1_type1_bmp_size(const struct lte_dci *dci)
{
	if (!dci || (dci->type != LTE_DCI_FORMAT1))
		return -1;
	if (dci->rbs <= 10)
		return -1;
	if (lte_dci_get_val(dci, LTE_DCI_FORMAT1_RA_HDR) < 1)
		return -1;

	int ra_size = format1_type0_ra_size(dci->rbs);
	if (dci->rbs <= 26)
		return ra_size - 2;
	else
		return ra_size - 3;
}

/* Format 1 resource allocation Type 1 values */
int lte_dci_format1_type1_subval(const struct lte_dci *dci, int field)
{
	int ra, ra_size, rbs = dci->rbs;

	if (rbs <= 10)
		return -1;
	if (lte_dci_get_val(dci, LTE_DCI_FORMAT1_RA_HDR) < 1)
		return -1;

	ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1_RB_ASSIGN);
	ra_size = format1_type0_ra_size(dci->rbs);

	if (field == LTE_DCI_FORMAT1_TYPE1_P) {
		if (rbs <= 26)
			return (ra >> (ra_size - 1)) & 0x01;
		else
			return (ra >> (ra_size - 2)) & 0x03;
	} else if (field == LTE_DCI_FORMAT1_TYPE1_SHIFT) {
		if (rbs <= 26)
			return (ra >> (ra_size - 2)) & 0x01;
		else
			return (ra >> (ra_size - 3)) & 0x01;
	} else if (field == LTE_DCI_FORMAT1_TYPE1_BMP) {
		unsigned mask;

		switch (rbs) {
		case 15:
			mask = 0x0000003f;
			break;
		case 25:
			mask = 0x000007ff;
			break;
		case 50:
			mask = 0x00003fff;
			break;
		case 75:
			mask = 0x0000ffff;
			break;
		case 100:
			mask = 0x003fffff;
			break;
		default:
			return -1;
		}

		return ra & mask;
	}

	return -1;
}

/* Resource block assignment sizes for Format 1A, 1B, 1D */
static int format1x_local_ra_size(int rbs)
{
	switch (rbs) {
	case 6:
		return DCI_FORMAT1A_LOCAL_RA(6);
	case 15:
		return DCI_FORMAT1A_LOCAL_RA(15);
	case 25:
		return DCI_FORMAT1A_LOCAL_RA(25);
	case 50:
		return DCI_FORMAT1A_LOCAL_RA(50);
	case 75:
		return DCI_FORMAT1A_LOCAL_RA(75);
	case 100:
		return DCI_FORMAT1A_LOCAL_RA(100);
	}

	return -1;
}

/* Resource block assignment sizes for Format 1A, 1B, 1D */
static unsigned format1x_dst_ra_mask(int rbs)
{
	switch (rbs) {
	case 6:
		return 0x000f;
	case 15:
		return 0x003f;
	case 25:
		return 0x00ff;
	case 50:
		return 0x03ff;
	case 75:
		return 0x07ff;
	case 100:
		return 0x0fff;
	}

	return 0;
}

static int lte_dci_format1a_subval(const struct lte_dci *dci, int field)
{
	if (lte_dci_get_val(dci, LTE_DCI_FORMAT1A_LOCAL_VRB) != 1)
		return -1;

	int ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1A_RB_ASSIGN);

	if (ra < 0)
		return -1;

	if (field == LTE_DCI_FORMAT1A_DIST_RA) {
		unsigned mask = format1x_dst_ra_mask(dci->rbs);
		return ra & mask;
	} else if (field == LTE_DCI_FORMAT1A_DIST_GAP) {
		int ra_size = format1x_local_ra_size(dci->rbs);
		return (unsigned) ra >> (ra_size - 1);

	}

	return -1;
};

static int lte_dci_format1b_subval(const struct lte_dci *dci, int field)
{
	if (lte_dci_get_val(dci, LTE_DCI_FORMAT1B_LOCAL_VRB) != 1)
		return -1;

	int ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1B_RB_ASSIGN);

	if (ra < 0)
		return -1;

	if (field == LTE_DCI_FORMAT1B_DIST_RA) {
		unsigned mask = format1x_dst_ra_mask(dci->rbs);
		return ra & mask;
	} else if (field == LTE_DCI_FORMAT1B_DIST_GAP) {
		int ra_size = format1x_local_ra_size(dci->rbs);
		return (unsigned) ra >> (ra_size - 1);

	}

	return -1;
};

static int lte_dci_format1d_subval(const struct lte_dci *dci, int field)
{
	if (lte_dci_get_val(dci, LTE_DCI_FORMAT1D_LOCAL_VRB) != 1)
		return -1;

	int ra = lte_dci_get_val(dci, LTE_DCI_FORMAT1D_RB_ASSIGN);

	if (ra < 0)
		return -1;

	if (field == LTE_DCI_FORMAT1D_DIST_RA) {
		unsigned mask = format1x_dst_ra_mask(dci->rbs);
		return ra & mask;
	} else if (field == LTE_DCI_FORMAT1D_DIST_GAP) {
		int ra_size = format1x_local_ra_size(dci->rbs);
		return (unsigned) ra >> (ra_size - 1);

	}

	return -1;
};

/* Return the value of a DCI field */
int lte_dci_get_val(const struct lte_dci *dci, int field)
{
	if (!dci || (field < 0) || (field >= DCI_MAX_FORMAT_FIELDS)) {
		fprintf(stderr, "DCI: Invalid field %i\n", field);
		return -1;
	}

	switch (dci->type) {
	case LTE_DCI_FORMAT1:
		if (field > LTE_DCI_FORMAT1_NUM_FIELDS)
			return lte_dci_format1_type1_subval(dci, field);
		break;
	case LTE_DCI_FORMAT1A:
		if (field > LTE_DCI_FORMAT1A_NUM_FIELDS)
			return lte_dci_format1a_subval(dci, field);
		break;
	case LTE_DCI_FORMAT1B:
		if (field > LTE_DCI_FORMAT1A_NUM_FIELDS)
			return lte_dci_format1b_subval(dci, field);
		break;
	case LTE_DCI_FORMAT1D:
		if (field > LTE_DCI_FORMAT1A_NUM_FIELDS)
			return lte_dci_format1d_subval(dci, field);
		break;
	}

	return dci->vals[field];
}

static __attribute__((constructor)) void init_dci_sizes()
{
	dci_setup_sizes(6, LTE_MODE_FDD);
	dci_setup_sizes(15, LTE_MODE_FDD);
	dci_setup_sizes(25, LTE_MODE_FDD);
	dci_setup_sizes(50, LTE_MODE_FDD);
	dci_setup_sizes(75, LTE_MODE_FDD);
	dci_setup_sizes(100, LTE_MODE_FDD);

	dci_setup_sizes(6, LTE_MODE_TDD);
	dci_setup_sizes(15, LTE_MODE_TDD);
	dci_setup_sizes(25, LTE_MODE_TDD);
	dci_setup_sizes(50, LTE_MODE_TDD);
	dci_setup_sizes(75, LTE_MODE_TDD);
	dci_setup_sizes(100, LTE_MODE_TDD);
}
