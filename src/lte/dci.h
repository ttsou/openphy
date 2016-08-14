#ifndef _LTE_DCI_
#define _LTE_DCI_

#include <stdint.h>

#define DCI_MAX_FORMAT_FIELDS		16

/* 3GPP TS 36.212 Release 8: 5.3.3.1 DCI formats */
enum lte_dci_format_type {
	LTE_DCI_FORMAT0,
	LTE_DCI_FORMAT1,
	LTE_DCI_FORMAT1A,
	LTE_DCI_FORMAT1B,
	LTE_DCI_FORMAT1C,
	LTE_DCI_FORMAT1D,
	LTE_DCI_FORMAT2,
	LTE_DCI_FORMAT2A,
	LTE_DCI_FORMAT3,
	LTE_DCI_FORMAT3A,
	LTE_DCI_NUM_FORMATS,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.1 Format 0 */
enum lte_dci_format0_fields {
	LTE_DCI_FORMAT0_FORMAT,
	LTE_DCI_FORMAT0_HOPPING,
	LTE_DCI_FORMAT0_RB_ASSIGN,
	LTE_DCI_FORMAT0_MOD,
	LTE_DCI_FORMAT0_NDI,
	LTE_DCI_FORMAT0_TPC,
	LTE_DCI_FORMAT0_CYC_SHIFT,
	LTE_DCI_FORMAT0_DAI,
	LTE_DCI_FORMAT0_CQI_REQ,
	LTE_DCI_FORMAT0_NUM_FIELDS,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.2 Format 1 */
enum lte_dci_format1_fields {
	LTE_DCI_FORMAT1_RA_HDR,
	LTE_DCI_FORMAT1_RB_ASSIGN,
	LTE_DCI_FORMAT1_MOD,
	LTE_DCI_FORMAT1_HARQ,
	LTE_DCI_FORMAT1_NDI,
	LTE_DCI_FORMAT1_RV,
	LTE_DCI_FORMAT1_TPC,
	LTE_DCI_FORMAT1_DAI,
	LTE_DCI_FORMAT1_NUM_FIELDS,

	/* Sub-fields */
	LTE_DCI_FORMAT1_TYPE1_P,
	LTE_DCI_FORMAT1_TYPE1_SHIFT,
	LTE_DCI_FORMAT1_TYPE1_BMP,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3 Format 1A */
enum lte_dci_format1a_fields {
	LTE_DCI_FORMAT1A_FORMAT,
	LTE_DCI_FORMAT1A_LOCAL_VRB,
	LTE_DCI_FORMAT1A_RB_ASSIGN,
	LTE_DCI_FORMAT1A_MOD,
	LTE_DCI_FORMAT1A_HARQ,
	LTE_DCI_FORMAT1A_NDI,
	LTE_DCI_FORMAT1A_RV,
	LTE_DCI_FORMAT1A_TPC,
	LTE_DCI_FORMAT1A_DAI,
	LTE_DCI_FORMAT1A_NUM_FIELDS,

	/* Sub-fields */
	LTE_DCI_FORMAT1A_DIST_RA,
	LTE_DCI_FORMAT1A_DIST_GAP,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.3A Format 1B */
enum lte_dci_format1b_fields {
	LTE_DCI_FORMAT1B_LOCAL_VRB,
	LTE_DCI_FORMAT1B_RB_ASSIGN,
	LTE_DCI_FORMAT1B_MOD,
	LTE_DCI_FORMAT1B_HARQ,
	LTE_DCI_FORMAT1B_NDI,
	LTE_DCI_FORMAT1B_RV,
	LTE_DCI_FORMAT1B_TPC,
	LTE_DCI_FORMAT1B_DAI,
	LTE_DCI_FORMAT1B_TPMI,
	LTE_DCI_FORMAT1B_PMI,
	LTE_DCI_FORMAT1B_NUM_FIELDS,

	/* Sub-fields */
	LTE_DCI_FORMAT1B_DIST_RA,
	LTE_DCI_FORMAT1B_DIST_GAP,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4 Format 1C */
enum lte_dci_format1c_fields {
	LTE_DCI_FORMAT1C_GAP,
	LTE_DCI_FORMAT1C_RB_ASSIGN,
	LTE_DCI_FORMAT1C_BLK_SIZE,
	LTE_DCI_FORMAT1C_NUM_FIELDS,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.4A Format 1D */
enum lte_dci_format1d_fields {
	LTE_DCI_FORMAT1D_LOCAL_VRB,
	LTE_DCI_FORMAT1D_RB_ASSIGN,
	LTE_DCI_FORMAT1D_MOD,
	LTE_DCI_FORMAT1D_HARQ,
	LTE_DCI_FORMAT1D_NDI,
	LTE_DCI_FORMAT1D_RV,
	LTE_DCI_FORMAT1D_TPC,
	LTE_DCI_FORMAT1D_DAI,
	LTE_DCI_FORMAT1D_TPMI,
	LTE_DCI_FORMAT1D_DL_PWR_OFFSET,
	LTE_DCI_FORMAT1D_NUM_FIELDS,

	/* Sub-fields */
	LTE_DCI_FORMAT1D_DIST_RA,
	LTE_DCI_FORMAT1D_DIST_GAP,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.5 Format 2 */
enum lte_dci_format2_fields {
	LTE_DCI_FORMAT2_RA_HDR,
	LTE_DCI_FORMAT2_RB_ASSIGN,
	LTE_DCI_FORMAT2_TPC,
	LTE_DCI_FORMAT2_DAI,
	LTE_DCI_FORMAT2_HARQ,
	LTE_DCI_FORMAT2_SWAP,
	LTE_DCI_FORMAT2_NDI,
	LTE_DCI_FORMAT2_RV,
	LTE_DCI_FORMAT2_PRECODING,
	LTE_DCI_FORMAT2_NUM_FIELDS,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.5A Format 2A */
enum lte_dci_format2a_fields {
	LTE_DCI_FORMAT2A_RA_HDR,
	LTE_DCI_FORMAT2A_RB_ASSIGN,
	LTE_DCI_FORMAT2A_TPC,
	LTE_DCI_FORMAT2A_DAI,
	LTE_DCI_FORMAT2A_HARQ,
	LTE_DCI_FORMAT2A_SWAP,
	LTE_DCI_FORMAT2A_MOD_1,
	LTE_DCI_FORMAT2A_NDI_1,
	LTE_DCI_FORMAT2A_RV_1,
	LTE_DCI_FORMAT2A_MOD_2,
	LTE_DCI_FORMAT2A_NDI_2,
	LTE_DCI_FORMAT2A_RV_2,
	LTE_DCI_FORMAT2A_PRECODING,
	LTE_DCI_FORMAT2A_NUM_FIELDS,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.6 Format 3 */
enum lte_dci_format3_fields {
	LTE_DCI_FORMAT3_NUM_FIELDS,
};

/* 3GPP TS 36.212 Release 8: 5.3.3.1.7 Format 3A */
enum lte_dci_format3a_fields {
	LTE_DCI_FORMAT3A_NUM_FIELDS,
};

enum lte_mode {
	LTE_MODE_FDD,
	LTE_MODE_TDD,
};

/*
 * DCI descriptor
 *
 * type - Format type
 * mode - FDD or TDD
 * rbs  - Number of resource blocks
 * rnti - RNTI value
 * vals - DCI values 
 */
struct lte_dci {
	int type;
	int mode;
	int rbs;
	uint16_t rnti;
	int vals[DCI_MAX_FORMAT_FIELDS];
};

int lte_dci_decode(struct lte_dci *dci, int rbs, int mode,
		   const unsigned char *bits, int len, uint16_t rnti);

int lte_dci_get_val(const struct lte_dci *dci, int field);
int lte_dci_get_mod(const struct lte_dci *dci);
int lte_dci_format_size(int rbs, int mode, int type);
int lte_dci_get_type(int rbs, int mode, int len);
int lte_dci_compare(const struct lte_dci *dci0, const struct lte_dci *dci2);
void lte_dci_print_sizes(int rbs, int mode);
void lte_dci_print(const struct lte_dci *dci);

/* DCI specific calls */
int lte_dci_format1_type0_bmp_size(const struct lte_dci *dci);
int lte_dci_format1_type1_bmp_size(const struct lte_dci *dci);

#endif /* _LTE_DCI_ */
