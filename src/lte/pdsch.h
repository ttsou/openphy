#ifndef _LTE_PDSCH_
#define _LTE_PDSCH_

struct lte_subframe;
struct lte_pdsch_blk;
struct lte_time;

int lte_decode_pdsch(struct lte_subframe **subframe,
		     int chans, struct lte_pdsch_blk *blk,
		     int cfi, int dci_index,
		     struct lte_time *time);

#endif /* LTE_PDSCH_ */
