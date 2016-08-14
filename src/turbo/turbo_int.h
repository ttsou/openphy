#ifndef _TURBO_INTERLEAVE_
#define _TURBO_INTERLEAVE_

/* Interleaver for initialization - 8-bits */
int turbo_interleave(int k, const uint8_t *input, uint8_t *output);

/* Reverse termination */
void turbo_unterm(int len, uint8_t *d0, uint8_t *d1, uint8_t *d2, uint8_t *d0p);

/* Interleaver for L-values - 16-bits */
int turbo_interleave_lval(int k, const int16_t *in, int16_t *out);
int turbo_deinterleave_lval(int k, const int16_t *in, int16_t *out);

#endif /* _TURBO_INTERLEAVE_ */
