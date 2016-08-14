#ifndef _PBCH_BLOCK_
#define _PBCH_BLOCK_

#include <stdint.h>

struct lte_pbch_blk;

/*
 * 3GPP TS 36.212 Release 8 "Transport block CRC attachment"
 *     L_CRC - Number of parity bits
 *     A     - Size of transport block
 *     K     - Size of transport block with CRC attached
 */
#define L_CRC16		16
#define PBCH_A		24
#define PBCH_K		(PBCH_A + L_CRC16)

/*
 * Allocate and initialize PBCH block object
 *     E   - Block size after convolutional rate matching 
 */
struct lte_pbch_blk *lte_pbch_blk_alloc();
int lte_pbch_blk_init(struct lte_pbch_blk *cblk, int E);
void lte_pbch_blk_free(struct lte_pbch_blk *cblk);

/* Decode 'e' block of soft bits into 'a' block of bits */
int lte_pbch_blk_decode(struct lte_pbch_blk *cblk);

/* Request 'e' buffer - Post code block concatenation */
int8_t *lte_pbch_blk_ebuf(struct lte_pbch_blk *cblk, int len);

/* Request 'a' buffer - Raw broadcast block without CRC attachment */
uint8_t *lte_pbch_blk_abuf(struct lte_pbch_blk *cblk, int len);

#endif /* _PBCH_BLOCK_ */
