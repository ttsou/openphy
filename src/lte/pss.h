#ifndef _LTE_PSS_H_
#define _LTE_PSS_H_

#define LTE_PSS_LEN		62
#define LTE_PSS_NUM		3

struct cxvec;

struct cxvec *lte_gen_pss(unsigned n_id_2);

#endif /* _LTE_PSS_H_ */
