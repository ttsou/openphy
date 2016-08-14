#ifndef _CONV_H_
#define _CONV_H_

#include <stdint.h>

enum {
	CONV_TERM_FLUSH,
	CONV_TERM_TAIL_BITING,
};

/*
 * Convolutional code descriptor
 *
 * n    - Rate 2, 3, 4 (1/2, 1/3, 1/4)
 * k    - Constraint length (5 or 7)
 * rgen - Recursive generator polynomial in octal
 * gen  - Generator polynomials in octal
 * punc - Puncturing matrix (-1 terminated)
 * term - Termination type (zero flush default)
 */
struct lte_conv_code {
	int n;
	int k;
	int len;
	unsigned rgen;
	unsigned gen[4];
	int *punc;
	int term;
};

int lte_conv_encode(const struct lte_conv_code *code,
		    const uint8_t *input, uint8_t *output);

int lte_conv_decode(const struct lte_conv_code *code,
		    const int8_t *input, uint8_t *output);

#endif /* _CONV_H_ */
