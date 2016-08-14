#ifndef _LTE_CORRELATE_
#define _LTE_CORRELATE_

#include <complex.h>

float complex fautocorr(float complex *a, int len, int offset);
float complex cxvec_mac(struct cxvec *x, struct cxvec *y);
int cxvec_corr(struct cxvec *x, struct cxvec *h, struct cxvec *y, int start, int len);

#endif /* _LTE_CORRELAET_ */
