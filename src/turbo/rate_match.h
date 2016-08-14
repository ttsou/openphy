#ifndef _TURBO_RATE_MATCH_
#define _TURBO_RATE_MATCH_

struct lte_rate_matcher {
	int E;
	int D;
	int V;
	int rows;
	int *w_null;
	int w_null_cnt;
	int rv;
	signed char *z[3];
	signed char *v[3];
	int *pi;
};

struct lte_rate_matcher_io {
	int D;
	int E;
	signed char *d[3];
	signed char *e;
};

/*
 * Allocate rate matcher object. Same object type is used for convolutional
 * and turbo rate processing paths.
 */
struct lte_rate_matcher *lte_rate_matcher_alloc();
void lte_rate_matcher_free(struct lte_rate_matcher *match);

/* LTE reverse turbo path rate matching */
int lte_rate_match_rv(struct lte_rate_matcher *match,
		      struct lte_rate_matcher_io *io, int rv);

/* LTE forward turbo path rate matching */
int lte_rate_match_fw(struct lte_rate_matcher *match,
		      struct lte_rate_matcher_io *io, int rv);

/* LTE reverse convolutional path rate matching */
int lte_conv_rate_match_rv(struct lte_rate_matcher *match,
			   struct lte_rate_matcher_io *io);

/* LTE forward convolutional path rate matching */
int lte_conv_rate_match_fw(struct lte_rate_matcher *match,
			   struct lte_rate_matcher_io *io);

#endif /* _TURBO_RATE_MATCH_ */
