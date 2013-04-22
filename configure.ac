AC_PREREQ([2.61])
AC_INIT([twstools], [0.4.1], [https://github.com/rudimeier/twstools])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/])

AM_INIT_AUTOMAKE([foreign parallel-tests color-tests])

## compiler check
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_CXX_C_O

AC_LANG([C++])
AX_COMPILER_VENDOR
AC_LANG_PUSH([C])
AX_COMPILER_VENDOR
AC_LANG_POP([C])

## enable some warnings for CXX
AX_CHECK_COMPILE_FLAG([-Wall],[warnflags="-Wall"])
# intel's -Wextra is too verbose
if test "$ax_cv_cxx_compiler_vendor" != "intel"; then
	AX_CHECK_COMPILE_FLAG([-Wextra],[warnflags="$warnflags -Wextra"])
fi
DEFAULT_CXXFLAGS="$warnflags"
AC_SUBST([DEFAULT_CXXFLAGS])

## enable some warnings for CC
AC_LANG_PUSH([C])
AX_CHECK_COMPILE_FLAG([-Wall],[warnflags="-Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra],[warnflags="$warnflags -Wextra"])
DEFAULT_CFLAGS="$warnflags"
AC_SUBST([DEFAULT_CFLAGS])
AC_LANG_POP([C])

## libltdl for loadable DSOs
LT_PREREQ([2.1])
LT_INIT([dlopen])
LT_CONFIG_LTDL_DIR([libltdl])
LTDL_INIT([recursive])
# workaround libtool LT_CONFIG_H bug #12262
AC_CONFIG_COMMANDS_PRE([LT_CONFIG_H=`expr "$LT_CONFIG_H" : '.*/\(.*\)'`])
AM_CONDITIONAL([DESCEND_LIBLTDL], [test "${with_included_ltdl}" = "yes"])

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7])
PKG_CHECK_MODULES([twsapi], [twsapi >= 0.4.0],
  [AC_DEFINE_UNQUOTED([TWSAPI_VERSION],
    ["$(pkg-config --modversion twsapi || echo "unknown")"],
    [Define twsapi version at compile time.])
  ])


safe_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${pkg_cv_twsapi_CFLAGS}"
AC_CHECK_HEADERS([twsapi/twsapi_config.h])
CPPFLAGS="${safe_CPPFLAGS}"

AC_CHECK_HEADERS([winsock2.h])


AC_CHECK_FUNCS(malloc_trim)
AC_CHECK_FUNCS(localtime)
AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(strptime)

AM_MISSING_PROG([HELP2MAN], [help2man], ["${missing_dir}"])

## output
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([libltdl/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([sample/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([twstools.pc])
AC_CONFIG_FILES([twstools.spec])
AC_OUTPUT

