#ifndef _LTE_PDSCH_
#define _LTE_PDSCH_

struct lte_pdsch_decoder;
struct lte_subframe;
struct lte_dci;
struct lte_pdsch_blk;
struct lte_time;

int lte_decode_pdsch(struct lte_subframe **subframe,
		     int chans, struct lte_pdsch_blk *blk,
		     int cfi, int dci_index,
		     struct lte_time *time);

struct lte_pdsch_decoder *lte_pdsch_alloc_decoder(int d_len, int e_len);

void lte_pdsch_free_decoder(struct lte_pdsch_decoder *dec);

#endif /* LTE_PDSCH_ */
