#ifndef _LTE_SI_
#define _LTE_SI_

enum lte_phich_dur {
	LTE_PHICH_DUR_NORMAL,
	LTE_PHICH_DUR_EXT,
};

enum lte_phich_grp {
	LTE_PHICH_GRP_SIXTH,
	LTE_PHICH_GRP_HALF,
	LTE_PHICH_GRP_ONE,
	LTE_PHICH_GRP_TWO,
};

struct lte_mib {
	int ant;
	int rbs;
	int fn;
	int phich_dur;
	int phich_ng;
};

struct lte_cell {
	int n_cell_id;
	int n_id_2;
	int n_id_1;

	struct lte_mib mib;
};

#endif /* _LTE_SI_ */
