#ifndef _GOLD_H_
#define _GOLD_H_

struct cxvec;

struct cxvec *lte_gen_gold_cxvec(unsigned init, int len);
void lte_gen_gold_seq(unsigned init, signed char *seq, int len);
void lte_gen_gold(unsigned init, unsigned char *seq, int len);

#endif /* _GOLD_H_ */
