#ifndef LTE_LOG_H
#define LTE_LOG_H

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#define LOG_COLOR_RED		"\033[1;31m"
#define LOG_COLOR_CYAN		"\033[0;36m"
#define LOG_COLOR_GREEN		"\033[0;32m"
#define LOG_COLOR_BLUE		"\033[0;34m"
#define LOG_COLOR_BLACK		"\033[0;30m"
#define LOG_COLOR_BROWN		"\033[0;33m"
#define LOG_COLOR_MAGENTA	"\033[1;35m"
#define LOG_COLOR_WHITE		"\033[0;37m"
#define LOG_COLOR_YELLOW	"\033[0;33m"
#define LOG_COLOR_NONE		"\033[0m"

#define LOG_COLOR_ERR		LOG_COLOR_RED
#define LOG_COLOR_APP		LOG_COLOR_YELLOW
#define LOG_COLOR_DEV		LOG_COLOR_BLUE
#define LOG_COLOR_DATA		LOG_COLOR_MAGENTA
#define LOG_COLOR_SYNC		LOG_COLOR_GREEN
#define LOG_COLOR_CTRL		LOG_COLOR_CYAN

#define LOG_BASE(V,C) \
{ \
	struct timeval tv; \
	gettimeofday(&tv, 0); \
	time_t curr = tv.tv_sec; \
	struct tm *t = localtime(&curr); \
	fprintf(stdout, "%s%02d:%02d:%02d:%03d %s%s\n", C, \
		t->tm_hour, t->tm_min, t->tm_sec, (int) tv.tv_usec / 1000, \
		V, LOG_COLOR_NONE); \
}

#define LOG_ERR(V)		LOG_BASE(V, LOG_COLOR_ERR)
#define LOG_APP(V)		LOG_BASE(V, LOG_COLOR_APP)
#define LOG_DEV(V)		LOG_BASE(V, LOG_COLOR_DEV)
#define LOG_DATA(V)		LOG_BASE(V, LOG_COLOR_DATA)
#define LOG_SYNC(V)		LOG_BASE(V, LOG_COLOR_SYNC)
#define LOG_CTRL(V)		LOG_BASE(V, LOG_COLOR_CYAN)

#define LOG_DEV_ERR(V)		LOG_ERR("DEV   : " V)
#define LOG_PSS_ERR(V)		LOG_ERR("PSS   : " V)
#define LOG_SSS_ERR(V)		LOG_ERR("SSS   : " V)
#define LOG_DSP_ERR(V)		LOG_ERR("DSP   : " V)
#define LOG_PBCH_ERR(V)		LOG_ERR("PBCH  : " V)
#define LOG_PCFICH_ERR(V)	LOG_ERR("PCFICH: " V)
#define LOG_PHICH_ERR(V)	LOG_ERR("PHICH : " V)
#define LOG_PDSCH_ERR(V)	LOG_ERR("PDSCH : " V)
#define LOG_PDCCH_ERR(V)	LOG_ERR("PDCCH : " V)

#define LOG_PSS(V)		LOG_SYNC("PSS   : " V)
#define LOG_SSS(V)		LOG_SYNC("SSS   : " V)
#define LOG_DSP(V)		LOG_ERR("DSP   : " V)
#define LOG_PBCH(V)		LOG_CTRL("PBCH  : " V)
#define LOG_PCFICH(V)		LOG_CTRL("PCFICH: " V)
#define LOG_PHICH(V)		LOG_CTRL("PHICH : " V)
#define LOG_PDCCH(V)		LOG_CTRL("PDCCH : " V)
#define LOG_PDSCH(V)		LOG_DATA("PDSCH : " V)

#define LOG_ARG(PREFIX,TYPE,STR,INT) \
{ \
	char sbuf[80]; \
	snprintf(sbuf, 80, "%s%s%i", PREFIX, STR, INT); \
	LOG_##TYPE(sbuf); \
}

#define LOG_PSS_ARG(STR,INT)	LOG_ARG("PSS   : ",SYNC,STR,INT)
#define LOG_SSS_ARG(STR,INT)	LOG_ARG("SSS   : ",SYNC,STR,INT)
#define LOG_DSP_ARG(STR,INT)	LOG_ARG("DSP   : ",CTRL,STR,INT)
#define LOG_PBCH_ARG(STR,INT)	LOG_ARG("PBCH  : ",CTRL,STR,INT)
#define LOG_PCFICH_ARG(STR,INT)	LOG_ARG("PCFICH: ",CTRL,STR,INT)
#define LOG_PHICH_ARG(STR,INT)	LOG_ARG("PHICH : ",CTRL,STR,INT)
#define LOG_PDCCH_ARG(STR,INT)	LOG_ARG("PDCCH : ",CTRL,STR,INT)
#define LOG_PDSCH_ARG(STR,INT)	LOG_ARG("PDSCH : ",DATA,STR,INT)

struct lte_time;

void lte_log_time(struct lte_time *ltime);

#endif /* LTE_LOG_H */
