LANG = C
LC_ALL = C

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src sample doc test

doc_DATA =
doc_DATA += README
doc_DATA += ChangeLog

dist_doc_DATA =
dist_doc_DATA += LICENSE

EXTRA_DIST =
EXTRA_DIST += $(PACKAGE).spec

# Rule to build RPM distribution package
rpm: dist-gzip
	rpmbuild -ta $(distdir).tar.gz
