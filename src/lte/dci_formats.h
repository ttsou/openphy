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

#ifndef _LTE_DCI_FORMAT_
#define _LTE_DCI_FORMAT_

#include <stdint.h>

/* 3GPP TS 36.212 Release 8: 5.3.3.1.1 Format 0 */
#define DCI_FORMAT0_BLK_ASSIGN6			5
#define DCI_FORMAT0_BLK_ASSIGN15		7
#define DCI_FORMAT0_BLK_ASSIGN25		9
#define DCI_FORMAT0_BLK_ASSIGN50		11
#define DCI_FORMAT0_BLK_ASSIGN75		12
#define DCI_FORMAT0_BLK_ASSIGN100		13
#define DCI_FORMAT0_BLK_ASSIGN(N)		DCI_FORMAT0_BLK_ASSIGN##N

#define DCI_FORMAT0_FDD(N) \
	struct dci_n##N##_format0_fdd { \
		uint8_t format[1]; \
		uint8_t hopping[1]; \
		uint8_t blk_assign[DCI_FORMAT0_BLK_ASSIGN(N)]; \
		uint8_t mod[5]; \
		uint8_t ndi[1]; \
		uint8_t tpc[2]; \
		uint8_t cyclic_shift[3]; \
		uint8_t cqi_req[1]; \
	};

#define DCI_FORMAT0_TDD(N) \
	struct dci_n##N##_format0_tdd { \
		uint8_t format[1]; \
		uint8_t hopping[1]; \
		uint8_t blk_assign[DCI_FORMAT0_BLK_ASSIGN(N)]; \
		uint8_t mod[5]; \
		uint8_t ndi[1]; \
		uint8_t tpc[2]; \
		uint8_t cyclic_shift[3]; \
		uint8_t dai[2]; \
		uint8_t cqi_req[1]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.2 Format 1 */
#define DCI_FORMAT1_RA_HEADER(N)		(N <= 10 ? 0 : 1)
#define DCI_FORMAT1_TYPE0_RA6			6
#define DCI_FORMAT1_TYPE0_RA15			8
#define DCI_FORMAT1_TYPE0_RA25			13
#define DCI_FORMAT1_TYPE0_RA50			17
#define DCI_FORMAT1_TYPE0_RA75			19
#define DCI_FORMAT1_TYPE0_RA100			25
#define DCI_FORMAT1_TYPE0_RA(N)			DCI_FORMAT1_TYPE0_RA##N

#define DCI_FORMAT1_FDD(N) \
	struct dci_n##N##_format1_fdd { \
		uint8_t ra_header[DCI_FORMAT1_RA_HEADER(N)]; \
		uint8_t blk_assign[DCI_FORMAT1_TYPE0_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[3]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
	};

#define DCI_FORMAT1_TDD(N) \
	struct dci_n##N##_format1_tdd { \
		uint8_t ra_header[DCI_FORMAT1_RA_HEADER(N)]; \
		uint8_t blk_assign[DCI_FORMAT1_TYPE0_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[4]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
		uint8_t dai[2]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3 Format 1A */
#define DCI_FORMAT1A_LOCAL_RA(N)		DCI_FORMAT0_BLK_ASSIGN##N

#define DCI_FORMAT1A_FDD(N) \
	struct dci_n##N##_format1a_fdd { \
		uint8_t format[1]; \
		uint8_t local_vrb[1]; \
		uint8_t blk_assign[DCI_FORMAT1A_LOCAL_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[3]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
	};

#define DCI_FORMAT1A_TDD(N) \
	struct dci_n##N##_format1a_tdd { \
		uint8_t format[1]; \
		uint8_t local_vrb[1]; \
		uint8_t blk_assign[DCI_FORMAT1A_LOCAL_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[4]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
		uint8_t dai[2]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3A Format 1B */
#define DCI_FORMAT1B_ANT_PORTS2			2
#define DCI_FORMAT1B_ANT_PORTS4			4
#define DCI_FORMAT1B_ANT_PORTS			DCI_FORMAT1B_ANT_PORTS2
#define DCI_FORMAT1B_LOCAL_RA(N)		DCI_FORMAT0_BLK_ASSIGN##N

#define DCI_FORMAT1B_FDD(N) \
	struct dci_n##N##_format1b_fdd { \
		uint8_t local_vrb[1]; \
		uint8_t blk_assign[DCI_FORMAT1B_LOCAL_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[3]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
		uint8_t tpmi[DCI_FORMAT1B_ANT_PORTS]; \
		uint8_t pmi[1]; \
	};

#define DCI_FORMAT1B_TDD(N) \
	struct dci_n##N##_format1b_tdd { \
		uint8_t local_vrb[1]; \
		uint8_t blk_assign[DCI_FORMAT1B_LOCAL_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[4]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
		uint8_t dai[2]; \
		uint8_t tpmi[DCI_FORMAT1B_ANT_PORTS]; \
		uint8_t pmi[1]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4 Format 1C */
#define DCI_FORMAT1C_GAP(N) (N < 50 ? 0 : 1)
#define DCI_FORMAT1C_BLK_ASSIGN6		3
#define DCI_FORMAT1C_BLK_ASSIGN15		5
#define DCI_FORMAT1C_BLK_ASSIGN25		7
#define DCI_FORMAT1C_BLK_ASSIGN50		7
#define DCI_FORMAT1C_BLK_ASSIGN75		8
#define DCI_FORMAT1C_BLK_ASSIGN100		9
#define DCI_FORMAT1C_BLK_ASSIGN(N)		DCI_FORMAT1C_BLK_ASSIGN##N

#define DCI_FORMAT1C_FDD(N) \
	struct dci_n##N##_format1c_fdd { \
		uint8_t gap[DCI_FORMAT1C_GAP(N)]; \
		uint8_t blk_assign[DCI_FORMAT1C_BLK_ASSIGN(N)]; \
		uint8_t blk_size[5]; \
	};

#define DCI_FORMAT1C_TDD(N) \
	struct dci_n##N##_format1c_tdd { \
		uint8_t gap[DCI_FORMAT1C_GAP(N)]; \
		uint8_t blk_assign[DCI_FORMAT1C_BLK_ASSIGN(N)]; \
		uint8_t blk_size[5]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4A Format 1D */
#define DCI_FORMAT1D_ANT_PORTS2			2
#define DCI_FORMAT1D_ANT_PORTS4			4
#define DCI_FORMAT1D_ANT_PORTS			DCI_FORMAT1D_ANT_PORTS2
#define DCI_FORMAT1D_LOCAL_RA(N)		DCI_FORMAT0_BLK_ASSIGN##N

#define DCI_FORMAT1D_FDD(N) \
	struct dci_n##N##_format1d_fdd { \
		uint8_t local_vrb[1]; \
		uint8_t blk_assign[DCI_FORMAT1D_LOCAL_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[3]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
		uint8_t tpmi[DCI_FORMAT1D_ANT_PORTS]; \
		uint8_t dl_pwr_offset[1]; \
	};

#define DCI_FORMAT1D_TDD(N) \
	struct dci_n##N##_format1d_tdd { \
		uint8_t local_vrb[1]; \
		uint8_t blk_assign[DCI_FORMAT1D_LOCAL_RA(N)]; \
		uint8_t mod[5]; \
		uint8_t harq[4]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t tpc[2]; \
		uint8_t dai[2]; \
		uint8_t tpmi[DCI_FORMAT1D_ANT_PORTS]; \
		uint8_t dl_pwr_offset[1]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.5 Format 2 */
#define DCI_FORMAT2_ANT_PORTS2_PRECODE		3
#define DCI_FORMAT2_ANT_PORTS4_PRECODE		6
#define DCI_FORMAT2_ANT_PORTS_PRECODE		DCI_FORMAT2_ANT_PORTS2_PRECODE
#define DCI_FORMAT2_TYPE0_RA(N)			DCI_FORMAT1_TYPE0_RA##N

#define DCI_FORMAT2_FDD(N) \
	struct dci_n##N##_format2_fdd { \
		uint8_t ra_header[1]; \
		uint8_t blk_assign[DCI_FORMAT2_TYPE0_RA(N)]; \
		uint8_t tpc[2]; \
		uint8_t harq[3]; \
		uint8_t swap[1]; \
		uint8_t mod[5]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t precoding[DCI_FORMAT2_ANT_PORTS_PRECODE]; \
	};

#define DCI_FORMAT2_TDD(N) \
	struct dci_n##N##_format2_tdd { \
		uint8_t ra_header[1]; \
		uint8_t blk_assign[DCI_FORMAT2_TYPE0_RA(N)]; \
		uint8_t tpc[2]; \
		uint8_t dai[2]; \
		uint8_t harq[4]; \
		uint8_t swap[1]; \
		uint8_t mod[5]; \
		uint8_t ndi[1]; \
		uint8_t rv[2]; \
		uint8_t precoding[DCI_FORMAT2_ANT_PORTS_PRECODE]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.5A Format 2A */
#define DCI_FORMAT2A_RA_HEADER(N)		(N <= 10 ? 0 : 1)
#define DCI_FORMAT2A_ANT_PORTS2_PRECODE		0
#define DCI_FORMAT2A_ANT_PORTS4_PRECODE		2
#define DCI_FORMAT2A_ANT_PORTS_PRECODE		DCI_FORMAT2A_ANT_PORTS2_PRECODE
#define DCI_FORMAT2A_TYPE0_RA(N)		DCI_FORMAT1_TYPE0_RA##N

#define DCI_FORMAT2A_FDD(N) \
	struct dci_n##N##_format2a_fdd { \
		uint8_t ra_header[DCI_FORMAT2A_RA_HEADER(N)]; \
		uint8_t blk_assign[DCI_FORMAT2A_TYPE0_RA(N)]; \
		uint8_t tpc[2]; \
		uint8_t harq[3]; \
		uint8_t swap[1]; \
		uint8_t mod_1[5]; \
		uint8_t ndi_1[1]; \
		uint8_t rv_1[2]; \
		uint8_t mod_2[5]; \
		uint8_t ndi_2[1]; \
		uint8_t rv_2[2]; \
		uint8_t precoding[DCI_FORMAT2A_ANT_PORTS_PRECODE]; \
	};

#define DCI_FORMAT2A_TDD(N) \
	struct dci_n##N##_format2a_tdd { \
		uint8_t ra_header[DCI_FORMAT2A_RA_HEADER(N)]; \
		uint8_t blk_assign[DCI_FORMAT2A_TYPE0_RA(N)]; \
		uint8_t tpc[2]; \
		uint8_t dai[2]; \
		uint8_t harq[4]; \
		uint8_t swap[1]; \
		uint8_t mod_1[5]; \
		uint8_t ndi_1[1]; \
		uint8_t rv_1[2]; \
		uint8_t mod_2[5]; \
		uint8_t ndi_2[1]; \
		uint8_t rv_2[2]; \
		uint8_t precoding[DCI_FORMAT2A_ANT_PORTS_PRECODE]; \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.6 Format 3 */
#define DCI_FORMAT3_FDD(N) \
	struct dci_n##N##_format3_fdd { \
	};

#define DCI_FORMAT3_TDD(N) \
	struct dci_n##N##_format3_tdd { \
	};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.7 Format 3A */
#define DCI_FORMAT3A_FDD(N) \
	struct dci_n##N##_format3a_fdd { \
	};

#define DCI_FORMAT3A_TDD(N) \
	struct dci_n##N##_format3a_tdd { \
	};

#define SET_ALL_RB_COMBS(DEF) \
	DEF(6) \
	DEF(15) \
	DEF(25) \
	DEF(50) \
	DEF(75) \
	DEF(100)

void dci_decode_format0_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1a_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1b_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1c_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1d_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format2_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format2a_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format3_fdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format3a_fdd(int rbs, int *vals, const uint8_t *bits);

void dci_decode_format0_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1a_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1b_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1c_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format1d_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format2_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format2a_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format3_tdd(int rbs, int *vals, const uint8_t *bits);
void dci_decode_format3a_tdd(int rbs, int *vals, const uint8_t *bits);

int dci_stored_size(int rbs, int mode, int type);

/* FDD Formats */
SET_ALL_RB_COMBS(DCI_FORMAT0_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT1_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT1A_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT1B_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT1C_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT1D_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT2_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT2A_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT3_FDD)
SET_ALL_RB_COMBS(DCI_FORMAT3A_FDD)

/* TDD Formats */
SET_ALL_RB_COMBS(DCI_FORMAT0_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT1_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT1A_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT1B_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT1C_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT1D_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT2_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT2A_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT3_TDD)
SET_ALL_RB_COMBS(DCI_FORMAT3A_TDD)

#endif /* _LTE_DCI_FORMAT_ */
