AM_CFLAGS = -std=gnu99 $(UHD_CFLAGS) $(WIRESHARK_CFLAGS)
AM_CPPFLAGS = $(all_includes)
AM_CXXFLAGS = $(UHD_CFLAGS) $(WIRESHARK_CFLAGS) -std=c++14

DSP_LA = $(top_builddir)/src/dsp/libdsp.la
LTE_LA = $(top_builddir)/src/lte/liblte.la
TURBO_LA = $(top_builddir)/src/turbo/libturbo.la

SUBDIRS = dsp lte turbo

noinst_LTLIBRARIES = libopenphy.la

libopenphy_la_SOURCES = \
	BufferQueue.cpp \
	Converter.cpp \
	DecoderPDSCH.cpp \
	IOInterface.cpp \
	Resampler.cpp \
	SignalVector.cpp \
	Synchronizer.cpp \
	SynchronizerPBCH.cpp \
	SynchronizerPDSCH.cpp \
	TimestampBuffer.cpp \
	UHDDevice.cpp \
	FreqAverager.cpp \
	DecoderASN1.cpp \
	LteBuffer.cpp

bin_PROGRAMS = ltedecode

ltedecode_SOURCES = ltedecode.cpp
ltedecode_LDADD = libopenphy.la $(LTE_LA) $(TURBO_LA) $(DSP_LA) $(FFTWF_LIBS) $(UHD_LIBS)
ltedecode_LDFLAGS = -pthread

noinst_HEADERS = \
	BufferQueue.h \
	Converter.h \
	DecoderASN1.h \
	DecoderPDSCH.h \
	FreqAverager.h \
	IOInterface.h \
	LteBuffer.h \
	Resampler.h \
	SignalVector.h \
	Synchronizer.h \
	SynchronizerPBCH.h \
	SynchronizerPDSCH.h \
	TimestampBuffer.h \
	Device.h \
	UHDDevice.h
