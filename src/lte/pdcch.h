#ifndef _LTE_PDCCH_
#define _LTE_PDCCH_

#include <stdint.h>

struct lte_subframe;
struct lte_dci;

int lte_decode_pdcch(struct lte_subframe **subframe, int chans,
		     int cfi, int n_cell_id, int ng, uint32_t rnti,
		     signed char *pdcch_seq);

#endif /* _LTE_PDCCH_ */
