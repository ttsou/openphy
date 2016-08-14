#ifndef _SIGPROC_INTERP_
#define _SIGPROC_INTERP_

struct interp_hdl {
	struct cxvec *h;
	int p;
};

struct interp_hdl *init_interp(int len, float p);
void free_interp(struct interp_hdl *hdl);
int cxvec_interp(struct interp_hdl *hdl, struct cxvec *x, struct cxvec *y);

#endif /* _SIGPROC_INTERP_ */
