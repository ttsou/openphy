#ifndef _RIV_H_
#define _RIV_H_

struct lte_dci;

struct lte_riv {
	int offset;
	int step;
	int n_vrb;
	int prbs0[110];
	int prbs1[110];
};

int lte_ra_type0_p(int rbs);
int lte_decode_riv(int rbs, const struct lte_dci *dci, struct lte_riv *riv);

#endif /* _RIV_H_ */
