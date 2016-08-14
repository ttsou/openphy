#ifndef _VRB_H_
#define _VRB_H_

enum {
	RIV_N_GAP1,
	RIV_N_GAP2,
};

int lte_dist_n_vrb(int rbs, int gap);

int lte_dist_vrb_to_prb(int rbs, int ns, int gap,
			int vrbs, int rb_start, int *n_prb);

int lte_local_vrb_to_prb(int rbs, int vrbs, int rb_start, int *n_prb);

#endif /* _VRB_H_ */
