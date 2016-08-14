#ifndef _LTE_PBCH_
#define _LTE_PBCH_

struct lte_subframe;
struct lte_mib;

int lte_decode_pbch(struct lte_mib *mib,
		    struct lte_subframe **subframe, int chans);

#endif /* _LTE_PBCH_ */
