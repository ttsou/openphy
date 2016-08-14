#ifndef _SIGVEC_H_
#define _SIGVEC_H_

#define CXVEC_FLG_REAL_ONLY		(1 << 0)
#define CXVEC_FLG_MEM_ALIGN		(1 << 1)
#define CXVEC_FLG_FFT_ALIGN		(1 << 2)
#define CXVEC_FLG_MEM_CHILD		(1 << 3)
#define CXVEC_FLG_MEM_INIT		(1 << 4)

/* Disallow sub-vectors that break memory alignment */
#define CXVEC_FLG_SUB_MEM_ALIGN		(1 << 5)

enum cxvec_conv_type {
        CONV_FULL_SPAN,
        CONV_OVERLAP_ONLY,
        CONV_NO_DELAY,
};

/* Complex vectors */
struct cxvec *cxvec_alloc(int len, int head, int tail,
			  void *buf, int flags);
struct cxvec *cxvec_subvec(struct cxvec *vec, int start,
			   int head, int tail, int len);
struct cxvec *cxvec_alias(struct cxvec *vec);
struct cxvec *cxvec_alloc_simple(int len);
void *cxvec_data(struct cxvec *vec);
void *cxvec_get_buffer(struct cxvec *vec);
int cxvec_len(struct cxvec *vec);
int cxvec_get_buffer_size(struct cxvec *vec);

void cxvec_free(struct cxvec *vec);
void cxvec_reset(struct cxvec *vec);

struct cxvec *cxvec_fftshift(struct cxvec *vec);
float cxvec_avgpow(struct cxvec *vec);
float cxvec_min(struct cxvec *vec);

int cxvec_cp(struct cxvec *dst, struct cxvec *src,
	     int dst_idx, int src_idx, int len);
int cxvec_rvrs(struct cxvec *in, struct cxvec *out);
int cxvec_sub(struct cxvec *a, struct cxvec *b, struct cxvec *out);
int cxvec_decim(struct cxvec *in, struct cxvec *out, int idx, int decim);
int cxvec_shft(struct cxvec *vec, struct cxvec *h, enum cxvec_conv_type type);

/* Vector set operations */
int cxvecs_set_len(struct cxvec **vecs, int m, int len);
int cxvecs_set_idx(struct cxvec **vecs, int m, int idx);

/* Interleavers */
int cxvec_interlv(struct cxvec **in, struct cxvec *out, int chan_m);
int cxvec_deinterlv_fw(struct cxvec *in, struct cxvec **out, int chan_m);
int cxvec_deinterlv_rv(struct cxvec *in, struct cxvec **out, int chan_m);

/* Cyclic shift */
int cxvec_cyc_shft(struct cxvec *in, struct cxvec *out, int shift);

/* Max / Min */
//complex float cxvec_max_val(struct cxvec *vec);
float cxvec_par(struct cxvec *vec);
int cxvec_max_idx(struct cxvec *vec);
void cxvec_conj(struct cxvec *vec);

/* Floating point utilities */
float cxvec_sinc(float x);
#if 0
//int cxvec_mac(float complex *a, float complex *b, float complex *out, int len);
float complex cxvec_mac4(float complex *a, float complex *b, float complex *out, int len);

void cxvec_mac_conj(float complex *a, float complex *b, float complex *out, int len);
void cxvec_mul_const(float complex *a, float complex *b, float complex *out, int len);
void cxvec_norm(float complex *a, float complex *out, int len);

void cxvec_mac4_conj(float complex *a, float complex *b, float complex *out, int len);
void cxvec_mul4_const(float complex *a, float complex *b, float complex *out, int len);
void cxvec_norm4(float complex *a, float complex *out, int len);
#endif

#endif /* _SIGVEC_H_ */
