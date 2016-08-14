#ifndef _LTE_H_
#define _LTE_H_

#define LTE_PSS_NUM		3
#define LTE_SSS_NUM		168

struct cxvec;

enum lte_state {
	LTE_STATE_PSS_SYNC,
	LTE_STATE_PSS_SYNC2,
	LTE_STATE_SSS_SYNC,
	LTE_STATE_PBCH_SYNC,
	LTE_STATE_PBCH,
	LTE_STATE_PDSCH_SYNC,
	LTE_STATE_PDSCH,
	LTE_NUM_STATES,
};

struct lte_sync {
	int coarse;
	int fine;
	int sym;
	int slot;
	int n_id_1;
	int n_id_2;
	int n_id_cell;
	int dn;
	float mag;
	float f_dist;
	float f_offset;
};

struct lte_time {
	int frame;
	int subframe;
	int slot;
};

struct lte_rx {
	int rbs;
	enum lte_state state;
	enum lte_state last_state;
	struct lte_sync sync;
	struct cxvec *pss_f[LTE_PSS_NUM];
	struct cxvec *pss_fc[LTE_PSS_NUM];
	struct cxvec *pss_t[LTE_PSS_NUM];
	int pss_i[LTE_PSS_NUM][64 * 2];
	unsigned long long pss[LTE_PSS_NUM][2];
	struct cxvec *pss_chan;
	struct cxvec *pss_chan1;
	unsigned long long sss[LTE_PSS_NUM][LTE_SSS_NUM][2];
	struct lte_time time;

	signed char *pbch_scram_seq;
};

struct lte_rx *lte_init();
void lte_free(struct lte_rx *rx);

int lte_pss_detect(struct lte_rx *rx, struct cxvec **slot, int chans);
int lte_pss_detect2(struct lte_rx *rx, struct cxvec **slot, int chans);
int lte_pss_detect3(struct lte_rx *rx, struct cxvec **slot, int chans);

int lte_sss_detect(struct lte_rx *rx, int n_id_2,
		   struct cxvec **slot, int chans,
		   struct lte_sync *sync);

#endif /* _LTE_H_ */
