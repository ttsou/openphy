#ifndef _LTE_PRECODE_
#define _LTE_PRECODE_

struct lte_sym;
struct cxvec;

int lte_unprecode(struct lte_sym *sym0, struct lte_sym *sym1,
		  int tx_ants, int rx_ants, int rb, int k0, int k1,
		  struct cxvec *data, int index);

int lte_unprecode_1x1(struct lte_sym *sym,
		      int rb, int k0, int k1,
		      struct cxvec *data, int idx);

int lte_unprecode_1x2(struct lte_sym *sym, struct lte_sym *sym2,
		      int rb, int k0, int k1, struct cxvec *data, int idx);

int lte_unprecode_2x1(struct lte_sym *sym,
		      int rb, int k0, int k1, struct cxvec *data, int idx);

int lte_unprecode_2x2(struct lte_sym *sym, struct lte_sym *sym2,
		      int rb, int k0, int k1, struct cxvec *data, int idx);

#endif /* _LTE_PRECODE_ */
