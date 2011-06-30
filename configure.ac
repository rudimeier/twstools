AC_PREREQ([2.61])
AC_INIT([twstool], [0.2.0], [http://www.ga-group.nl/])
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

PKG_CHECK_MODULES([QtCore_DEPS], [QtCore >= 4.3])
## use rpath if qt4 is not the global installed one
QtCore_DEPS_LIBS=$( echo ${QtCore_DEPS_LIBS} \
	| sed 's/\(^\| \|\t\)-L\([[^\t ]]*\)/\1-Wl,-rpath,\2 -L\2/g' )

PKG_CHECK_MODULES([libxml2_DEPS], [libxml-2.0 >= 2.7])

PKG_CHECK_MODULES([libconfig_DEPS], [libconfig++ >= 1.3])

## output
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT

