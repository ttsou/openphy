#ifndef _LTE_PHICH_
#define _LTE_PHICH_

int lte_phich_num_groups(int rbs, int ng, int dur);
int lte_gen_phich_indices(int *indices, int rbs,
			  int n_cell_id, int ng, int dur);

#endif /* _LTE_PHICH_ */
