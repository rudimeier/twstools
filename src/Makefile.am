AM_CPPFLAGS =
AM_CPPFLAGS += -DDEBUG -D_GNU_SOURCE
AM_CPPFLAGS += -DIB_USE_STD_STRING
AM_CPPFLAGS += -D_REENTRANT -DQT_CORE_LIB -DQT_SHARED
AM_CPPFLAGS += -I/usr/share/qt4/mkspecs/default
AM_CXXFLAGS =
AM_CXXFLAGS += $(DEFAULT_CXXFLAGS)
AM_LDFLAGS =
AM_LDFLAGS += -L/usr/local/lib64
AM_LDFLAGS += -lconfig++
AM_LDFLAGS += -lQtCore


bin_PROGRAMS =
bin_PROGRAMS += twstool
twstool_SOURCES =
twstool_SOURCES += main.cpp
twstool_SOURCES += twsDL.cpp
twstool_SOURCES += tws_meta.cpp
twstool_SOURCES += twsClient.cpp
twstool_SOURCES += twsUtil.cpp
twstool_SOURCES += twsWrapper.cpp
twstool_SOURCES += properties.cpp
twstool_LDADD =
twstool_LDADD += -libtws


noinst_HEADERS =
noinst_HEADERS += twsClient.h
noinst_HEADERS += twsUtil.h
noinst_HEADERS += twsWrapper.h
noinst_HEADERS += properties.h
noinst_HEADERS += debug.h
noinst_HEADERS += twsDL.h
noinst_HEADERS += tws_meta.h




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

