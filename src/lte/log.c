#include "log.h"
#include "lte.h"

void lte_log_time(struct lte_time *ltime)
{
	char sbuf[64];

	snprintf(sbuf, 64, "FRAME : SFN %04i, Ns %02i",
		 ltime->frame, ltime->subframe);
	LOG_APP(sbuf);
}
