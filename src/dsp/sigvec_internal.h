#ifndef _SIGVEC_INTERNAL_H_
#define _SIGVEC_INTERNAL_H_

#include <complex.h>

struct cxvec {
	int len;
	int buf_len;
	int flags;
	int start_idx;
	float complex *buf;
	float complex *data;
	float complex _buf[0];
};

int cxvec_max_idx(struct cxvec *vec);

#endif /* _SIGVEC_INTERNAL_H_ */

