AM_CFLAGS = -Wall -std=c99 -fvisibility=hidden $(OPENFEC_CFLAGS) $(UHD_CFLAGS) -pthread -I$(top_srcdir)/src/dsp $(WIRESHARK_CFLAGS)  -march=native
AM_CPPFLAGS = $(UHD_CFLAGS)

noinst_LTLIBRARIES = liblte.la

liblte_la_SOURCES = \
	lte.cc \
	pss.c \
	sss.c \
	ref.c \
	crc.c \
	pbch.c \
	pbch_block.c \
	pcfich.c \
	phich.c \
	pdcch.c \
	pdcch_block.c \
	pdcch_interleave.c \
	qam.c \
	precode.c \
	ofdm.c \
	slot.c \
	gold.c \
	sync.c \
	sync_pss.c \
	dci.c \
	dci_formats.c \
	scramble.c \
	expand.c \
	pdsch.c \
	pdsch_vrb.c \
	pdsch_riv.c \
	pdsch_tbs.c \
	pdsch_block.c \
	log.c

noinst_HEADERS = \
	crc.h \
	gold.h \
	pbch_block.h \
	pdcch.h \
	pdsch_riv.h \
	precode.h \
	scramble.h \
	sss.h \
	dci_formats.h \
	log.h \
	pbch.h \
	pdcch_interleave.h \
	pdsch_tbs.h \
	pss.h \
	si.h \
	subframe.h \
	dci.h \
	lte.h \
	pcfich.h \
	pdsch_block.h \
	pdsch_vrb.h \
	qam.h \
	slot.h \
	sync.h \
	expand.h \
	ofdm.h \
	pdcch_block.h \
	pdsch.h \
	phich.h \
	ref.h
