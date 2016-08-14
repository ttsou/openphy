#ifndef _LTE_SUBFRAME_H_
#define _LTE_SUBFRAME_H_

/* Only support 2 downlink antennas are supported */
#define LTE_DOWNLINK_ANT	2

/* Support up to 10 DCI blocks per subframe */
#define LTE_DCI_MAX		10

#include <stdint.h>
#include "lte.h"
#include "dci.h"

struct lte_ref;
struct lte_ref_map;
struct lte_slot;
struct lte_subframe;
struct fft_hdl;
struct interp_hdl;
struct cxvec;

struct lte_sym {
	int l;
	struct lte_slot *slot;
	struct cxvec *td;
	struct cxvec *fd;
	struct lte_ref *ref;
	struct cxvec **rb;
};

struct lte_ref {
	struct lte_slot *slot;
	struct lte_sym *sym;
	struct cxvec *refs[2];
	struct cxvec *chan[LTE_DOWNLINK_ANT + 1];
	struct cxvec **rb[LTE_DOWNLINK_ANT + 1];
	struct lte_ref_map *map[2];
};

struct lte_slot {
	int rbs;
	int num;
	struct lte_subframe *subframe;
	struct cxvec *td;
	struct cxvec *fd;
	struct lte_sym syms[7];
	struct lte_ref refs[2];
};

struct lte_subframe {
	int rbs;
	int assigned;
	int cell_id;
	int ng;
	int cfi;
	int tx_ants;

	int num_dci;
	struct lte_dci dci[LTE_DCI_MAX];

	struct cxvec *samples;
	struct lte_slot slot[2];
	int ref_indices[4];

	int *reserve;

	struct fft_hdl *fft;
	struct interp_hdl *interp;
};

#endif /* _LTE_SUBFRAME_H_ */
