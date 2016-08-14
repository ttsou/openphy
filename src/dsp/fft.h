#ifndef _FFT_H_
#define _FFT_H_

#include <stddef.h>
#include "sigvec.h"

struct fft_hdl;

struct fft_hdl *init_fft(int reverse, int m, int many,
			 int idist, int odist, int istride, int ostride,
			 struct cxvec *in, struct cxvec *out, int flags);
void *fft_malloc(size_t size);
void fft_free_hdl(struct fft_hdl *hdl);

/* Free the aligned FFT buffer */
void fft_free_buf(void *buf);

void cxvec_fft(struct fft_hdl *hdl, struct cxvec *in, struct cxvec *out);

#endif /* _FFT_H_ */
