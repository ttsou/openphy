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

#include "dci.h"
#include "dci_formats.h"

#ifdef PACK_LE
#define PACK(B,N) pack_le(B,N)
static int pack_le(uint8_t *b, int n)
{
	unsigned v = 0;

	for (int i = 0; i < n; i++)
		v |= (b[i] & 0x01) << i;

	return (int) v;
}
#else
#define PACK(B,N) pack_be(B,N)
static int pack_be(uint8_t *b, int n)
{
	unsigned v = 0;

	for (int i = 0; i < n; i++)
		v |= (b[n - 1 - i] & 0x01) << i;

	return (int) v;
}
#endif

#define DCI_FORMAT_NA	-1

/* 3GPP TS 36.212 Release 8: 5.3.3.1.1 Format 0 */
#define DCI_DECODE_FORMAT0_FDD(N) \
	static void dci_n##N##_decode_format0_fdd(int *v, \
					struct dci_n##N##_format0_fdd *f) \
	{ \
		v[0] = *f->format; \
		v[1] = *f->hopping; \
		v[2] = PACK(f->blk_assign, DCI_FORMAT0_BLK_ASSIGN##N); \
		v[3] = PACK(f->mod, 5); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->tpc, 2); \
		v[6] = PACK(f->cyclic_shift, 3); \
		v[7] = DCI_FORMAT_NA; \
		v[8] = *f->cqi_req; \
	}

#define DCI_DECODE_FORMAT0_TDD(N) \
	static void dci_n##N##_decode_format0_tdd(int *v, \
					struct dci_n##N##_format0_tdd *f) \
	{ \
		v[0] = *f->format; \
		v[1] = *f->hopping; \
		v[2] = PACK(f->blk_assign, DCI_FORMAT0_BLK_ASSIGN##N); \
		v[3] = PACK(f->mod, 5); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->tpc, 2); \
		v[6] = PACK(f->cyclic_shift, 3); \
		v[7] = PACK(f->dai, 2); \
		v[8] = *f->cqi_req; \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.2 Format 1 */
#define DCI_DECODE_FORMAT1_FDD(N) \
	static void dci_n##N##_decode_format1_fdd(int *v, \
					struct dci_n##N##_format1_fdd *f) \
	{ \
		v[0] = PACK(f->ra_header, DCI_FORMAT1_RA_HEADER(N)); \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1_TYPE0_RA(N)); \
		v[2] = PACK(f->mod, 5); \
		v[3] = PACK(f->harq, 3); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->rv, 2); \
		v[6] = PACK(f->tpc, 2); \
		v[7] = DCI_FORMAT_NA; \
	}

#define DCI_DECODE_FORMAT1_TDD(N) \
	static void dci_n##N##_decode_format1_tdd(int *v, \
					struct dci_n##N##_format1_tdd *f) \
	{ \
		v[0] = PACK(f->ra_header, DCI_FORMAT1_RA_HEADER(N)); \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1_TYPE0_RA(N)); \
		v[2] = PACK(f->mod, 5); \
		v[3] = PACK(f->harq, 4); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->rv, 2); \
		v[6] = PACK(f->tpc, 2); \
		v[7] = PACK(f->dai, 2); \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3 Format 1A */
#define DCI_DECODE_FORMAT1A_FDD(N) \
	static void dci_n##N##_decode_format1a_fdd(int *v, \
					struct dci_n##N##_format1a_fdd *f) \
	{ \
		v[0] = *f->format; \
		v[1] = *f->local_vrb; \
		v[2] = PACK(f->blk_assign, DCI_FORMAT1A_LOCAL_RA(N)); \
		v[3] = PACK(f->mod, 5); \
		v[4] = PACK(f->harq, 3); \
		v[5] = *f->ndi; \
		v[6] = PACK(f->rv, 2); \
		v[7] = PACK(f->tpc, 2); \
		v[8] = DCI_FORMAT_NA; \
	}

#define DCI_DECODE_FORMAT1A_TDD(N) \
	static void dci_n##N##_decode_format1a_tdd(int *v, \
					struct dci_n##N##_format1a_tdd *f) \
	{ \
		v[0] = *f->format; \
		v[1] = *f->local_vrb; \
		v[2] = PACK(f->blk_assign, DCI_FORMAT1A_LOCAL_RA(N)); \
		v[3] = PACK(f->mod, 5); \
		v[4] = PACK(f->harq, 4); \
		v[5] = *f->ndi; \
		v[6] = PACK(f->rv, 2); \
		v[7] = PACK(f->tpc, 2); \
		v[8] = PACK(f->dai, 2); \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3A Format 1B */
#define DCI_DECODE_FORMAT1B_FDD(N) \
	static void dci_n##N##_decode_format1b_fdd(int *v, \
					struct dci_n##N##_format1b_fdd *f) \
	{ \
		v[0] = *f->local_vrb; \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1A_LOCAL_RA(N)); \
		v[2] = PACK(f->mod, 5); \
		v[3] = PACK(f->harq, 3); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->rv, 2); \
		v[6] = PACK(f->tpc, 2); \
		v[7] = DCI_FORMAT_NA; \
		v[8] = *f->tpmi; \
		v[9] = *f->pmi; \
	}

#define DCI_DECODE_FORMAT1B_TDD(N) \
	static void dci_n##N##_decode_format1b_tdd(int *v, \
					struct dci_n##N##_format1b_tdd *f) \
	{ \
		v[0] = *f->local_vrb; \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1A_LOCAL_RA(N)); \
		v[2] = PACK(f->mod, 5); \
		v[3] = PACK(f->harq, 4); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->rv, 2); \
		v[6] = PACK(f->tpc, 2); \
		v[7] = PACK(f->dai, 2); \
		v[8] = *f->tpmi; \
		v[9] = *f->pmi; \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4 Format 1C */
#define DCI_DECODE_FORMAT1C_FDD(N) \
	static void dci_n##N##_decode_format1c_fdd(int *v, \
					struct dci_n##N##_format1c_fdd *f) \
	{ \
		v[0] = PACK(f->gap, DCI_FORMAT1C_GAP(N)); \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1C_BLK_ASSIGN(N)); \
		v[2] = PACK(f->blk_size, 5); \
	}

#define DCI_DECODE_FORMAT1C_TDD(N) \
	static void dci_n##N##_decode_format1c_tdd(int *v, \
					struct dci_n##N##_format1c_tdd *f) \
	{ \
		v[0] = PACK(f->gap, DCI_FORMAT1C_GAP(N)); \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1C_BLK_ASSIGN(N)); \
		v[2] = PACK(f->blk_size, 5); \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4A Format 1D */
#define DCI_DECODE_FORMAT1D_FDD(N) \
	static void dci_n##N##_decode_format1d_fdd(int *v, \
					struct dci_n##N##_format1d_fdd *f) \
	{ \
		v[0] = *f->local_vrb; \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1D_LOCAL_RA(N)); \
		v[2] = PACK(f->mod, 5); \
		v[3] = PACK(f->harq, 3); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->rv, 2); \
		v[6] = PACK(f->tpc, 2); \
		v[7] = DCI_FORMAT_NA; \
		v[8] = PACK(f->tpmi, DCI_FORMAT1D_ANT_PORTS); \
		v[9] = *f->dl_pwr_offset; \
	}

#define DCI_DECODE_FORMAT1D_TDD(N) \
	static void dci_n##N##_decode_format1d_tdd(int *v, \
					struct dci_n##N##_format1d_tdd *f) \
	{ \
		v[0] = *f->local_vrb; \
		v[1] = PACK(f->blk_assign, DCI_FORMAT1D_LOCAL_RA(N)); \
		v[2] = PACK(f->mod, 5); \
		v[3] = PACK(f->harq, 4); \
		v[4] = *f->ndi; \
		v[5] = PACK(f->rv, 2); \
		v[6] = PACK(f->tpc, 2); \
		v[7] = PACK(f->dai, 2); \
		v[8] = PACK(f->tpmi, DCI_FORMAT1D_ANT_PORTS); \
		v[9] = *f->dl_pwr_offset; \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.5 Format 2 */
#define DCI_DECODE_FORMAT2_FDD(N) \
	static void dci_n##N##_decode_format2_fdd(int *v, \
					struct dci_n##N##_format2_fdd *f) \
	{ \
		v[0] = *f->ra_header; \
		v[1] = PACK(f->blk_assign, DCI_FORMAT2_TYPE0_RA(N)); \
		v[2] = PACK(f->tpc, 2); \
		v[3] = DCI_FORMAT_NA; \
		v[4] = PACK(f->harq, 3); \
		v[5] = *f->swap; \
		v[6] = PACK(f->mod, 5); \
		v[7] = *f->ndi; \
		v[8] = PACK(f->rv, 2); \
		v[9] = PACK(f->precoding, DCI_FORMAT2_ANT_PORTS_PRECODE); \
	}

#define DCI_DECODE_FORMAT2_TDD(N) \
	static void dci_n##N##_decode_format2_tdd(int *v, \
					struct dci_n##N##_format2_tdd *f) \
	{ \
		v[0] = *f->ra_header; \
		v[1] = PACK(f->blk_assign, DCI_FORMAT2_TYPE0_RA(N)); \
		v[2] = PACK(f->tpc, 2); \
		v[3] = PACK(f->dai, 2); \
		v[4] = PACK(f->harq, 4); \
		v[5] = *f->swap; \
		v[6] = PACK(f->mod, 5); \
		v[7] = *f->ndi; \
		v[8] = PACK(f->rv, 2); \
		v[9] = PACK(f->precoding, DCI_FORMAT2_ANT_PORTS_PRECODE); \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.5A Format 2A */
#define DCI_DECODE_FORMAT2A_FDD(N) \
	static void dci_n##N##_decode_format2a_fdd(int *v, \
					struct dci_n##N##_format2a_fdd *f) \
	{ \
		v[ 0] = PACK(f->ra_header, DCI_FORMAT2A_RA_HEADER(N)); \
		v[ 1] = PACK(f->blk_assign, DCI_FORMAT2A_TYPE0_RA(N)); \
		v[ 2] = PACK(f->tpc, 2); \
		v[ 3] = DCI_FORMAT_NA; \
		v[ 4] = PACK(f->harq, 3); \
		v[ 5] = *f->swap; \
		v[ 6] = PACK(f->mod_1, 5); \
		v[ 7] = *f->ndi_1; \
		v[ 8] = PACK(f->rv_1, 2); \
		v[ 9] = PACK(f->mod_2, 5); \
		v[10] = *f->ndi_2; \
		v[11] = PACK(f->rv_2, 2); \
		v[12] = PACK(f->precoding, DCI_FORMAT2A_ANT_PORTS_PRECODE); \
	}

#define DCI_DECODE_FORMAT2A_TDD(N) \
	static void dci_n##N##_decode_format2a_tdd(int *v, \
					struct dci_n##N##_format2a_tdd *f) \
	{ \
		v[ 0] = *f->ra_header; \
		v[ 1] = PACK(f->blk_assign, DCI_FORMAT2A_TYPE0_RA(N)); \
		v[ 2] = PACK(f->tpc, 2); \
		v[ 3] = PACK(f->dai, 2); \
		v[ 4] = PACK(f->harq, 4); \
		v[ 5] = *f->swap; \
		v[ 6] = PACK(f->mod_1, 5); \
		v[ 7] = *f->ndi_1; \
		v[ 8] = PACK(f->rv_1, 2); \
		v[ 9] = PACK(f->mod_2, 5); \
		v[10] = *f->ndi_2; \
		v[11] = PACK(f->rv_2, 2); \
		v[12] = PACK(f->precoding, DCI_FORMAT2A_ANT_PORTS_PRECODE); \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.6 Format 3 */
#define DCI_DECODE_FORMAT3_FDD(N) \
	static void dci_n##N##_decode_format3_fdd(int *v, \
					struct dci_n##N##_format3_fdd *f) \
	{ \
	}

#define DCI_DECODE_FORMAT3_TDD(N) \
	static void dci_n##N##_decode_format3_tdd(int *v, \
					struct dci_n##N##_format3_tdd *f) \
	{ \
	}

/* 3GPP TS 36.212 Release 8: 5.3.3.1.7 Format 3A */
#define DCI_DECODE_FORMAT3A_FDD(N) \
	static void dci_n##N##_decode_format3a_fdd(int *v, \
					struct dci_n##N##_format3a_fdd *f) \
	{ \
	}

#define DCI_DECODE_FORMAT3A_TDD(N) \
	static void dci_n##N##_decode_format3a_tdd(int *v, \
					struct dci_n##N##_format3a_tdd *f) \
	{ \
	}

#define DCI_DECODE_FORMAT_FDD(N) \
	void dci_decode_format##N##_fdd(int rbs, int *vals, \
					const uint8_t *bits) \
	{ \
		switch (rbs) { \
		case 6: \
			return dci_n6_decode_format##N##_fdd(vals, \
				(struct dci_n6_format##N##_fdd *) bits); \
		case 15: \
			return dci_n15_decode_format##N##_fdd(vals, \
				(struct dci_n15_format##N##_fdd *) bits); \
		case 25: \
			return dci_n25_decode_format##N##_fdd(vals, \
				(struct dci_n25_format##N##_fdd *) bits); \
		case 50: \
			return dci_n50_decode_format##N##_fdd(vals, \
				(struct dci_n50_format##N##_fdd *) bits); \
		case 75: \
			return dci_n75_decode_format##N##_fdd(vals, \
				(struct dci_n75_format##N##_fdd *) bits); \
		case 100: \
			return dci_n100_decode_format##N##_fdd(vals, \
				(struct dci_n100_format##N##_fdd *) bits); \
		} \
	}

#define DCI_DECODE_FORMAT_TDD(N) \
	void dci_decode_format##N##_tdd(int rbs, int *vals, \
					const uint8_t *bits) \
	{ \
		switch (rbs) { \
		case 6: \
			return dci_n6_decode_format##N##_tdd(vals, \
				(struct dci_n6_format##N##_tdd *) bits); \
		case 15: \
			return dci_n15_decode_format##N##_tdd(vals, \
				(struct dci_n15_format##N##_tdd *) bits); \
		case 25: \
			return dci_n25_decode_format##N##_tdd(vals, \
				(struct dci_n25_format##N##_tdd *) bits); \
		case 50: \
			return dci_n50_decode_format##N##_tdd(vals, \
				(struct dci_n50_format##N##_tdd *) bits); \
		case 75: \
			return dci_n75_decode_format##N##_tdd(vals, \
				(struct dci_n75_format##N##_tdd *) bits); \
		case 100: \
			return dci_n100_decode_format##N##_tdd(vals, \
				(struct dci_n100_format##N##_tdd *) bits); \
		} \
	}

#define DCI_FORMAT_FDD_SIZE(N) \
	static int dci_n##N##_format_fdd_size(int type) \
	{ \
		switch (type) { \
		case LTE_DCI_FORMAT0: \
			return sizeof(struct dci_n##N##_format0_fdd); \
		case LTE_DCI_FORMAT1: \
			return sizeof(struct dci_n##N##_format1_fdd); \
		case LTE_DCI_FORMAT1A: \
			return sizeof(struct dci_n##N##_format1a_fdd); \
		case LTE_DCI_FORMAT1B: \
			return sizeof(struct dci_n##N##_format1b_fdd); \
		case LTE_DCI_FORMAT1C: \
			return sizeof(struct dci_n##N##_format1c_fdd); \
		case LTE_DCI_FORMAT1D: \
			return sizeof(struct dci_n##N##_format1d_fdd); \
		case LTE_DCI_FORMAT2: \
			return sizeof(struct dci_n##N##_format2_fdd); \
		case LTE_DCI_FORMAT2A: \
			return sizeof(struct dci_n##N##_format2a_fdd); \
		case LTE_DCI_FORMAT3: \
			return sizeof(struct dci_n##N##_format3_fdd); \
		case LTE_DCI_FORMAT3A: \
			return sizeof(struct dci_n##N##_format3a_fdd); \
		} \
		return -1; \
	}

#define DCI_FORMAT_TDD_SIZE(N) \
	static int dci_n##N##_format_tdd_size(int type) \
	{ \
		switch (type) { \
		case LTE_DCI_FORMAT0: \
			return sizeof(struct dci_n##N##_format0_tdd); \
		case LTE_DCI_FORMAT1: \
			return sizeof(struct dci_n##N##_format1_tdd); \
		case LTE_DCI_FORMAT1A: \
			return sizeof(struct dci_n##N##_format1a_tdd); \
		case LTE_DCI_FORMAT1B: \
			return sizeof(struct dci_n##N##_format1b_tdd); \
		case LTE_DCI_FORMAT1C: \
			return sizeof(struct dci_n##N##_format1c_tdd); \
		case LTE_DCI_FORMAT1D: \
			return sizeof(struct dci_n##N##_format1d_tdd); \
		case LTE_DCI_FORMAT2: \
			return sizeof(struct dci_n##N##_format2_tdd); \
		case LTE_DCI_FORMAT2A: \
			return sizeof(struct dci_n##N##_format2a_tdd); \
		case LTE_DCI_FORMAT3: \
			return sizeof(struct dci_n##N##_format3_tdd); \
		case LTE_DCI_FORMAT3A: \
			return sizeof(struct dci_n##N##_format3a_tdd); \
		} \
		return -1; \
	}

#define SET_ALL_RB_COMBS(DEF) \
	DEF(6) \
	DEF(15) \
	DEF(25) \
	DEF(50) \
	DEF(75) \
	DEF(100)

SET_ALL_RB_COMBS(DCI_FORMAT_FDD_SIZE)
SET_ALL_RB_COMBS(DCI_FORMAT_TDD_SIZE)

int dci_stored_size(int rbs, int mode, int type)
{
	if (mode == LTE_MODE_FDD) {
		switch (rbs) {
		case 6:
			return dci_n6_format_fdd_size(type);
		case 15:
			return dci_n15_format_fdd_size(type);
		case 25:
			return dci_n25_format_fdd_size(type);
		case 50:
			return dci_n50_format_fdd_size(type);
		case 75:
			return dci_n75_format_fdd_size(type);
		case 100:
			return dci_n100_format_fdd_size(type);
		}
	} else {
		switch (rbs) {
		case 6:
			return dci_n6_format_tdd_size(type);
		case 15:
			return dci_n15_format_tdd_size(type);
		case 25:
			return dci_n25_format_tdd_size(type);
		case 50:
			return dci_n50_format_tdd_size(type);
		case 75:
			return dci_n75_format_tdd_size(type);
		case 100:
			return dci_n100_format_tdd_size(type);
		}
	}

	return -1;
}

/* FDD Formats */
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT0_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1A_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1B_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1C_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1D_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT2_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT2A_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT3_FDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT3A_FDD)

DCI_DECODE_FORMAT_FDD(0)
DCI_DECODE_FORMAT_FDD(1)
DCI_DECODE_FORMAT_FDD(1a)
DCI_DECODE_FORMAT_FDD(1b)
DCI_DECODE_FORMAT_FDD(1c)
DCI_DECODE_FORMAT_FDD(1d)
DCI_DECODE_FORMAT_FDD(2)
DCI_DECODE_FORMAT_FDD(2a)
DCI_DECODE_FORMAT_FDD(3)
DCI_DECODE_FORMAT_FDD(3a)

/* TDD Formats */
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT0_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1A_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1B_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1C_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT1D_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT2_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT2A_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT3_TDD)
SET_ALL_RB_COMBS(DCI_DECODE_FORMAT3A_TDD)

DCI_DECODE_FORMAT_TDD(0)
DCI_DECODE_FORMAT_TDD(1)
DCI_DECODE_FORMAT_TDD(1a)
DCI_DECODE_FORMAT_TDD(1b)
DCI_DECODE_FORMAT_TDD(1c)
DCI_DECODE_FORMAT_TDD(1d)
DCI_DECODE_FORMAT_TDD(2)
DCI_DECODE_FORMAT_TDD(2a)
DCI_DECODE_FORMAT_TDD(3)
DCI_DECODE_FORMAT_TDD(3a)
