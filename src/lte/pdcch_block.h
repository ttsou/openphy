#ifndef _PDCCH_BLOCK_
#define _PDCCH_BLOCK_

#include <stdint.h>

struct lte_pdcch_blk;

/*
 * Allocate and initialize PDCCH block object
 *     A   - Raw transport block size
 *     E   - Rate matched outer size
 */
struct lte_pdcch_blk *lte_pdcch_blk_alloc();
int lte_pdcch_blk_init(struct lte_pdcch_blk *dblk, int A, int E);
void lte_pdcch_blk_free(struct lte_pdcch_blk *dblk);

/* Decode 'e' block of soft bits into 'a' block of bits */
int lte_pdcch_blk_decode(struct lte_pdcch_blk *dblk, uint16_t rnti);

/* Request 'e' buffer - Post code block concatenation */
int8_t *lte_pdcch_blk_ebuf(struct lte_pdcch_blk *dblk, int len);

/* Request 'a' buffer - Decoded control block bits without CRC attachment */
uint8_t *lte_pdcch_blk_abuf(struct lte_pdcch_blk *dblk, int len);

#endif /* _PDCCH_BLOCK_ */
