#ifndef _PDCCH_CONV_H_
#define _PDCCH_CONV_H_

#include <complex.h>

typedef struct {
	float complex val[4];
} cquadf;

struct lte_pdcch_deinterlv {
	int len;
	int *seq;
};

int pdcch_deinterlv(struct lte_pdcch_deinterlv *d, cquadf *in, cquadf *out);
struct lte_pdcch_deinterlv *lte_alloc_pdcch_deinterlv(int len, int n_cell_id);
void lte_free_pdcch_deinterlv(struct lte_pdcch_deinterlv *d);

#endif /* _PDCCH_CONV_H_ */
