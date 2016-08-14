#ifndef _PDSCH_BLOCK_
#define _PDSCH_BLOCK_

#include <stdint.h>

struct lte_pdsch_blk;

/*
 * Allocate and initialize PDSCH transport block object
 *     A   - Raw transport block size
 *     G   - Total number of available bits
 *     N_l - Transmission layer mapping (1 or 2)
 *     Q_m - Modulation density (2, 4, or 6)
 */
struct lte_pdsch_blk *lte_pdsch_blk_alloc();
int lte_pdsch_blk_init(struct lte_pdsch_blk *tblk,
		       int A, int G, int N_l, int Q_m);
void lte_pdsch_blk_free(struct lte_pdsch_blk *tblk);

/* Decode 'f' block of soft bits into 'a' block of bits */
int lte_pdsch_blk_decode(struct lte_pdsch_blk *tblk, int rv);

/* Request 'f' buffer - Post code block concatenation */
int8_t *lte_pdsch_blk_fbuf(struct lte_pdsch_blk *tblk, int *len);

/* Request 'a' buffer - Original data block before CRC attachment */
uint8_t *lte_pdsch_blk_abuf(struct lte_pdsch_blk *tblk, int *len);

#endif /* _PDSCH_BLOCK_ */
