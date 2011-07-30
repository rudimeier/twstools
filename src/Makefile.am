AM_CPPFLAGS =
AM_CPPFLAGS += -DDEBUG -D_GNU_SOURCE
AM_CPPFLAGS += -DIB_USE_STD_STRING
AM_CXXFLAGS =
AM_CXXFLAGS += $(DEFAULT_CXXFLAGS)
AM_LDFLAGS =


bin_PROGRAMS =
bin_PROGRAMS += twsDL twsgen

twsDL_SOURCES =
twsDL_SOURCES += twsDL.cpp
twsDL_SOURCES += tws_meta.cpp
twsDL_SOURCES += tws_xml.cpp
twsDL_SOURCES += twsClient.cpp
twsDL_SOURCES += twsUtil.cpp
twsDL_SOURCES += twsWrapper.cpp
twsDL_CPPFLAGS = ${AM_CPPFLAGS}
twsDL_CPPFLAGS += ${QtCore_DEPS_CFLAGS}
twsDL_LDFLAGS = ${AM_LDFLAGS}
twsDL_LDFLAGS += ${QtCore_DEPS_LIBS}
twsDL_LDFLAGS += ${libxml2_DEPS_LIBS}
twsDL_LDADD =
twsDL_LDADD += -libtws
twsDL_LDADD += -lpopt

twsgen_SOURCES =
twsgen_SOURCES += twsgen.cpp
twsgen_SOURCES += tws_xml.cpp
twsgen_SOURCES += tws_meta.cpp
twsgen_SOURCES += twsUtil.cpp
twsgen_CPPFLAGS = ${AM_CPPFLAGS}
twsgen_LDFLAGS = ${AM_LDFLAGS}
twsgen_LDFLAGS += ${QtCore_DEPS_LIBS}
twsgen_LDFLAGS += ${libxml2_DEPS_LIBS}
twsgen_LDADD =
twsgen_LDADD += -libtws
twsgen_LDADD += -lpopt

noinst_HEADERS =
noinst_HEADERS += twsClient.h
noinst_HEADERS += twsUtil.h
noinst_HEADERS += twsWrapper.h
noinst_HEADERS += debug.h
noinst_HEADERS += twsDL.h
noinst_HEADERS += tws_meta.h
noinst_HEADERS += tws_xml.h




CLEANFILES =
CLEANFILES += *.s
CLEANFILES += *.i

# 
# Help the developers get nice post-processed source files

## Create preprocessor output (debugging purposes only)
.c.i:
	$(COMPILE) -E -o $@ $<
.cpp.i:
	$(CXXCOMPILE) -E -o $@ $<

## Create assembler output (debugging purposes only)
.c.s:
	$(COMPILE) -S -c $(cflags) $<
.cpp.s:
	$(CXXCOMPILE) -S -c $(cflags) $<

