#ifndef _LTE_TURBO_
#define _LTE_TURBO_

#include <stdint.h>

struct tdecoder;

/* Min and max code block sizes */
#define TURBO_MIN_K		40
#define TURBO_MAX_K		6144

/*
 * Convolutional encoder descriptor
 *
 * Rate and constraint lengths refer to constituent encoders. Only
 * rate 1/2 contraint length 4 codes supported. Uses fixed LTE turbo
 * interleaver.
 *
 * n    - Rate 2 (1/2)
 * k    - Constraint length 4
 * len  - Length of code block to be encoded
 * rgen - Recursive generator polynomial in octal
 * gen  - Generator polynomial in octal
 */
struct lte_turbo_code {
	int n;
	int k;
	int len;
	unsigned rgen;
	unsigned gen;
};

struct tdecoder *alloc_tdec();
void free_tdec(struct tdecoder *dec);

int lte_turbo_encode(const struct lte_turbo_code *code,
		   const uint8_t *input, uint8_t *d0, uint8_t *d1, uint8_t *d2);

/* Packed output */
int lte_turbo_decode(struct tdecoder *dec, int len, int iter, uint8_t *output,
		     const int8_t *d0, const int8_t *d1, const int8_t *d2);

/* Unpacked output */
int lte_turbo_decode_unpack(struct tdecoder *dec, int len, int iter,
			    uint8_t *output, const int8_t *d0,
			    const int8_t *d1, const int8_t *d2);

#endif /* _LTE_TURBO_ */
