AC_PREREQ([2.61])
AC_INIT([twstools], [0.3.3], [http://www.ga-group.nl/])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_SRCDIR([src/])

AM_INIT_AUTOMAKE([foreign parallel-tests color-tests])

## compiler check
AC_PROG_CXX
AC_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_RANLIB

DEFAULT_CXXFLAGS="-Wall -W"
AC_SUBST([DEFAULT_CXXFLAGS])

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7])
PKG_CHECK_MODULES([libtwsapi], [libtwsapi >= 0.3.0])


AC_CHECK_HEADERS([winsock2.h])


AC_CHECK_FUNCS(localtime)
AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(strptime)


## output
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([sample/Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([twstools.spec])
AC_OUTPUT

