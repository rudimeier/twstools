# Help the Developers and yourself.  Just use the C locale and settings
# for the compilation. They can still be overriden by make LANG=<whatever>
# but that is general a not very good idea
LANG=C
LC_ALL=C

BUILT_SOURCES =
EXTRA_DIST = $(BUILT_SOURCES)

man1_MANS =
man1_MANS += $(built_mans)

EXTRA_DIST += author.h2m
EXTRA_DIST += twsdo.h2m
EXTRA_DIST += twsgen.h2m
BUILT_SOURCES += $(built_mans)

built_mans =
built_mans += twsdo.1
built_mans += twsgen.1

## non generic deps per executable
twsdo.1: $(top_srcdir)/src/twsdo_main.cpp
twsgen.1: $(top_srcdir)/src/twsgen.cpp

## help2man helpers
%.1: $(top_srcdir)/src/%.ggo $(top_srcdir)/doc/%.h2m $(top_srcdir)/configure
	$(HELP2MAN) -S "$(PACKAGE_STRING)" -N \
		-I "$(srcdir)/$*.h2m" -I "$(builddir)/$*.h2m" \
		-I "$(srcdir)/author.h2m" \
		-o $@ $(top_builddir)/src/$*
