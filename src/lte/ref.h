#ifndef _LTE_REF_
#define _LTE_REF_

#define LTE_N_MAX_DL_RB         110
#define LTE_N_MIN_DL_RB         6
#define LTE_N_MAX_ANT           4

struct sigvec;

struct lte_ref_map {
	struct cxvec *a;
	int rbs;
	int *k;
	int len;
	int l;
	int p;
	int ns;
	int v;
	int v_shift;
};

struct cxvec *lte_gen_ref(struct cxvec *gold);
unsigned int gen_c_init(int N_cp, int N_cell_id, int ns, int l);

struct lte_ref_map *lte_gen_ref_map(int cell_id, int p,
				    int ns, int l, int rbs);

void lte_free_ref_map(struct lte_ref_map *map);

#endif /* LTE_REF_ */
