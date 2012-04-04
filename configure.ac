AC_PREREQ([2.61])
AC_INIT([twstools], [0.3.4], [http://www.ga-group.nl/])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_SRCDIR([src/])

AM_INIT_AUTOMAKE([foreign parallel-tests color-tests])

## compiler check
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_RANLIB

AC_LANG([C++])
AX_COMPILER_VENDOR

# enable some warnings
AX_CHECK_COMPILE_FLAG([-Wall],[warnflags="-Wall"])
# intel's -Wextra is too verbose
if test "$ax_cv_cxx_compiler_vendor" != "intel"; then
	AX_CHECK_COMPILE_FLAG([-Wextra],[warnflags="$warnflags -Wextra"])
fi
DEFAULT_CXXFLAGS="$warnflags"
AC_SUBST([DEFAULT_CXXFLAGS])

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7])
PKG_CHECK_MODULES([libtwsapi], [libtwsapi >= 0.3.0],
  [AC_DEFINE_UNQUOTED([TWSAPI_VERSION],
    ["$(pkg-config --modversion libtwsapi || echo "unknown")"],
    [Define twsapi version at compile time.])
  if pkg-config --atleast-version="0.4.0" libtwsapi; then
    AC_DEFINE([TWSAPI_IPV6], [1], [Define to 1 if twsapi supports IPv6.])
  fi])


AC_CHECK_HEADERS([winsock2.h])


AC_CHECK_FUNCS(malloc_trim)
AC_CHECK_FUNCS(localtime)
AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(strptime)

AM_MISSING_PROG([HELP2MAN], [help2man], ["${missing_dir}"])

## output
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([sample/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([twstools.spec])
AC_OUTPUT

