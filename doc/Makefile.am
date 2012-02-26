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
BUILT_SOURCES += $(built_mans)

## help2man helpers
%.man: $(top_srcdir)/src/%.ggo $(top_srcdir)/configure
	$(HELP2MAN) -S "$(PACKAGE_STRING)" -p "($(PACKAGE))$*" \
		-I "$(srcdir)/$*.h2m" -I "$(builddir)/$*.h2m" \
		-I "$(srcdir)/author.h2m" \
		-o $@ $(top_builddir)/src/$*
