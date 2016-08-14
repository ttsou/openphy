#ifndef _LTE_OFDM_
#define _LTE_OFDM_

/* OFDM demodulator symbol masks */
#define LTE_SYM0_MASK		(1 << 0)
#define LTE_SYM1_MASK		(1 << 1)
#define LTE_SYM2_MASK		(1 << 2)
#define LTE_SYM3_MASK		(1 << 3)
#define LTE_SYM4_MASK		(1 << 4)
#define LTE_SYM5_MASK		(1 << 5)
#define LTE_SYM6_MASK		(1 << 6)
#define LTE_ALL_SYMS_MASK	(LTE_SYM0_MASK | LTE_SYM1_MASK | \
				 LTE_SYM2_MASK | LTE_SYM3_MASK | \
				 LTE_SYM4_MASK | LTE_SYM5_MASK | \
				 LTE_SYM6_MASK)

#define LTE_REF_MASK		(LTE_SYM0_MASK | LTE_SYM4_MASK)

struct cxvec;
struct lte_slot;
struct lte_subframe;
struct lte_ref_map;

struct lte_subframe *lte_subframe_alloc(int rbs, int cell_id, int ant,
					struct lte_ref_map **maps0,
					struct lte_ref_map **maps1);
void lte_subframe_free(struct lte_subframe *slot);

int lte_subframe_reset(struct lte_subframe *subframe,
		       struct lte_ref_map **map0, struct lte_ref_map **map1);

int lte_subframe_convert(struct lte_subframe *subframe);

float lte_ofdm_offset(struct lte_subframe *subframe);

int lte_chk_ref(struct lte_subframe *subframe, int slot, int l, int sc, int p);

#endif /* _LTE_OFDM_ */
