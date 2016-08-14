#ifndef _LTE_SSS_H_
#define _LTE_SSS_H_

struct cxvec;

#define LTE_SSS_NUM		168

struct lte_sss {
	struct cxvec *d0;
	struct cxvec *d5;
};

struct lte_sss *lte_gen_sss(unsigned n_id_1, unsigned n_id_2);

#endif /* _LTE_SSS_H_ */
