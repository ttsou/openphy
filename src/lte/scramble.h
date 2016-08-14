#ifndef _LTE_SCRAMBLE_
#define _LTE_SCRAMBLE_

void lte_pbch_gen_scrambler(unsigned init, signed char *seq, int len);
void lte_pdcch_gen_scrambler(unsigned init, signed char *seq, int len);

void lte_scramble(signed char *bits, signed char *c, int len);
void lte_descramble(signed char *bits, signed char *c, int len);

void lte_scramble2(signed char *bits, signed char *c, int len);

#endif /* _LTE_SCRAMBLE_ */
