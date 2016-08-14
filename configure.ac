AC_INIT([openphy], [0.1], [tom@tsou.cc])
AC_CONFIG_SRCDIR([src/UHDDevice.cpp])
AM_INIT_AUTOMAKE([foreign dist-bzip2 no-define 1.6])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_PROG_INSTALL
LT_INIT([pic-only])

AC_CONFIG_MACRO_DIR([m4])

AC_ARG_WITH(neon, [
    AS_HELP_STRING([--with-neon],
        [enable ARM NEON support])
])

AC_ARG_WITH(neon-vfpv4, [
    AS_HELP_STRING([--with-neon-vfpv4],
        [enable ARM NEON FMA support])
])

AS_IF([test "x$with_neon" = "xyes"], [
    AC_DEFINE(HAVE_NEON, 1, Support ARM NEON)
])

AS_IF([test "x$with_neon_vfpv4" = "xyes"], [
    AC_DEFINE(HAVE_NEON, 1, Support ARM NEON)
    AC_DEFINE(HAVE_NEON_FMA, 1, Support ARM NEON with FMA)
])

AC_CHECK_LIB([m],[sincos])
PKG_CHECK_MODULES(FFTWF, fftw3f)
PKG_CHECK_MODULES(WIRESHARK, wireshark >= 0.0)

PKG_CHECK_MODULES(UHD, uhd >= 003.009.000)

dnl check for processor family
AX_EXT

AM_CONDITIONAL(ARCH_ARM, [test "x$with_neon" = "xyes" || test "x$with_neon_vfpv4" = "xyes"])
AM_CONDITIONAL(ARCH_ARM_A15, [test "x$with_neon_vfpv4" = "xyes"])

AC_OUTPUT(
	src/lte/Makefile
	src/dsp/Makefile
	src/turbo/Makefile
	src/Makefile
	Makefile)
