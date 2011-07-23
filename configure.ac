AC_PREREQ([2.61])
AC_INIT([twstool], [0.2.1], [http://www.ga-group.nl/])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_SRCDIR([src/])

AM_INIT_AUTOMAKE([foreign])

## compiler check
AC_PROG_CXX
AC_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_RANLIB

DEFAULT_CXXFLAGS="-Wall -W"
AC_SUBST([DEFAULT_CXXFLAGS])

PKG_CHECK_MODULES([libxml2_DEPS], [libxml-2.0 >= 2.7])

## output
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT

