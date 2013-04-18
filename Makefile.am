include $(top_builddir)/version.mk

LANG = C
LC_ALL = C

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES =

SUBDIRS =
if DESCEND_LIBLTDL
SUBDIRS += libltdl
endif  DESCEND_LIBLTDL
SUBDIRS += src sample doc test

# do_subst, inspired by the automake project
do_subst = ( sed \
  -e 's,[@]VERSION[@],$(VERSION),g' \
  -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
  -e 's,[@]PACKAGE_STRING[@],$(PACKAGE_STRING),g' \
| $(SHELL) ./config.status --file=- \
  )

doc_DATA =
doc_DATA += README
doc_DATA += ChangeLog

dist_doc_DATA =
dist_doc_DATA += LICENSE

EXTRA_DIST =
EXTRA_DIST += git-version-gen
EXTRA_DIST += $(PACKAGE).spec

DISTCLEANFILES += version.mk

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = twstools.pc

# Rule to build RPM distribution package
rpm: dist-gzip
	rpmbuild -ta $(distdir).tar.gz

dist-hook:
	echo $(VERSION) > $(distdir)/version \
	&& touch -r $(top_builddir)/version.mk $(distdir)/version
