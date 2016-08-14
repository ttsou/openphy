#ifndef _LTE_SYNC_
#define _LTE_SYNC_

struct lte_rx;
struct lte_sync;
struct cxvec;

int lte_pss_search(struct lte_rx *rx, struct cxvec **subframe,
		   int chans, struct lte_sync *sync);

int lte_pss_sync(struct lte_rx *rx, struct cxvec **subframe,
		 int chans, struct lte_sync *sync, int n_id_2);

int lte_pss_fine_sync(struct lte_rx *rx, struct cxvec **subframe,
		      int chans, struct lte_sync *sync, int n_id_2);

#endif /* _LTE_SYNC_ */
