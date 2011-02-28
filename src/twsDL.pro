TEMPLATE = app

include( ../../src/src.pri )

SOURCES += \
  twsClient.cpp \
  twsUtil.cpp \
  twsWrapper.cpp \
  main.cpp \
  twsDL.cpp \
  tws_meta.cpp

HEADERS += \
  twsClient.h \
  twsUtil.h \
  twsWrapper.h \
  debug.h \
  twsDL.h \
  tws_meta.h

INCLUDEPATH +=

DEFINES += \
  IB_USE_STD_STRING

LIBS += \
  -L$${ibtws_lib_dir} \
  -libtws \
  -lcore

QMAKE_LFLAGS += \
  -Wl,-rpath,\\\$\$ORIGIN/../../lib

TARGETDEPS += \
  $${libdir}/libcore.so
