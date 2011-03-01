TEMPLATE = app

CONFIG += \
  qt \
  thread \
  warn_on \
  debug

CONFIG -= \
  release

QT += \
  core

QT -= \
  gui

ibtws_prefix = /usr/local
ibtws_include_dir = $${ibtws_prefix}/include/ibtws
ibtws_lib_dir = $${ibtws_prefix}/lib64


SOURCES += \
  twsClient.cpp \
  twsUtil.cpp \
  twsWrapper.cpp \
  properties.cpp \
  main.cpp \
  twsDL.cpp \
  tws_meta.cpp

HEADERS += \
  twsClient.h \
  twsUtil.h \
  twsWrapper.h \
  properties.h \
  debug.h \
  twsDL.h \
  tws_meta.h

INCLUDEPATH +=

DEFINES += \
  IB_USE_STD_STRING

LIBS += \
  -L$${ibtws_lib_dir} \
  -libtws \
  -lconfig++ \

TARGET = twstool

TARGETDEPS +=
