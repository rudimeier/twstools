TEMPLATE = app

include( ../../src/src.pri )

SOURCES += \
  main.cpp \
  twsDL.cpp \
  tws_meta.cpp

HEADERS += \
  twsDL.h \
  tws_meta.h

INCLUDEPATH += \
  $${ibtws_include_dir}

DEFINES += \
  IB_USE_STD_STRING

LIBS += \
  -ltwsapi \
  -lcore

QMAKE_LFLAGS += \
  -Wl,-rpath,\\\$\$ORIGIN/../../lib

TARGETDEPS += \
  $${libdir}/libcore.so \
  $${libdir}/libtwsapi.so
