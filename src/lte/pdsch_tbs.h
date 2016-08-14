#ifndef _PDSCH_TBS_
#define _PDSCH_TBS_

struct lte_dci;

int lte_tbs_get_mod_order(struct lte_dci *dci);
int lte_tbs_get(struct lte_dci *dci, int n_prb, unsigned rnti);

struct lte_tbs_desc *lte_tbs_segment(uint8_t *b, int len);
void lte_tbs_desc_free(struct lte_tbs_desc *desc);

#endif /* _PDSCH_TBS_ */
