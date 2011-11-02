# Help the Developers and yourself. Just use the C locale and settings
# for the compilation. They can still be overriden by make LANG=<whatever>
# but that is general a not very good idea
LANG = C
LC_ALL = C

EXTRA_DIST = $(TESTS)
EXTRA_DIST += $(TWST_LOG_COMPILER)
TESTS =

TEST_EXTENSIONS = .twst
TWST_LOG_COMPILER = $(srcdir)/tws-test.sh
AM_TWST_LOG_FLAGS = --builddir $(top_builddir)/src --hash sha1sum
LOG_COMPILER = echo

TESTS += defaults.01.twst
TESTS += defaults.02.twst

## Makefile.am ends here
