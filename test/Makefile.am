# Help the Developers and yourself. Just use the C locale and settings
# for the compilation. They can still be overriden by make LANG=<whatever>
# but that is general a not very good idea
LANG = C
LC_ALL = C

EXTRA_DIST = $(TESTS)
EXTRA_DIST += $(TWST_LOG_COMPILER)
TESTS =
dist_noinst_DATA =

TEST_EXTENSIONS = .twst
TWST_LOG_COMPILER = $(srcdir)/tws-test.sh
AM_TWST_LOG_FLAGS = --builddir $(top_builddir)/src --hash sha1sum
LOG_COMPILER = echo

TESTS += defaults.01.twst
TESTS += defaults.02.twst
TESTS += defaults.03.twst
TESTS += defaults.04.twst
TESTS += twsgen_hist.01.twst
TESTS += twsgen_hist.02.twst
TESTS += twsgen_hist.03.twst
TESTS += twsgen_hist.04.twst
TESTS += twsgen_hist.05.twst

dist_noinst_DATA += con_fut.xml
dist_noinst_DATA += hist_data_no_defaults.xml
dist_noinst_DATA += hist_data_with_defaults.xml
dist_noinst_DATA += work_hist_fut_01.xml
dist_noinst_DATA += work_hist_fut_02.xml
dist_noinst_DATA += work_hist_cash_01.xml

## Makefile.am ends here
