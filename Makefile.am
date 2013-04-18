LANG = C
LC_ALL = C

ACLOCAL_AMFLAGS = -I m4

SUBDIRS =
if DESCEND_LIBLTDL
SUBDIRS += libltdl
endif  DESCEND_LIBLTDL
SUBDIRS += src sample doc test

doc_DATA =
doc_DATA += README
doc_DATA += ChangeLog

dist_doc_DATA =
dist_doc_DATA += LICENSE

EXTRA_DIST =
EXTRA_DIST += git-version-gen
EXTRA_DIST += $(PACKAGE).spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = twstools.pc

# Rule to build RPM distribution package
rpm: dist-gzip
	rpmbuild -ta $(distdir).tar.gz
