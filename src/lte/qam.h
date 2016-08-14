#ifndef _LTE_QAM_
#define _LTE_QAM_

struct cxvec;

int lte_qpsk_decode(struct cxvec *vec, signed char *bits, int len);
int lte_qpsk_decode2(struct cxvec *vec, signed char *bits, int len);

int lte_qam16_decode(struct cxvec *vec, signed char *bits, int len);
int lte_qam64_decode(struct cxvec *vec, signed char *bits, int len);
int lte_qam256_decode(struct cxvec *vec, signed char *bits, int len);

#endif /* _LTE_QAM_ */
