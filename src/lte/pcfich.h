#ifndef _LTE_PCFICH_
#define _LTE_PCFICH_

#include "ofdm.h"

#define LTE_PCFICH_CFI_LEN	32

struct lte_pcfich_info {
	int cfi;
};

int lte_decode_pcfich(struct lte_pcfich_info *info,
		      struct lte_subframe **subframe,
		      int n_cell_id, signed char *seq, int chans);

#endif /* _LTE_PCFICH_ */
