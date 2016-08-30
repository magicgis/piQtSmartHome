QT += dbus
QT -= gui

TEMPLATE = lib
CONFIG += debug

CONFIG(debug, debug|release) {
    message("$${TARGET} - debug mode")
}else {
    message("$${TARGET} - release mode")
}

DEFINES += QT_MESSAGELOGCONTEXT

SOURCES = homeAlarmInfo.cpp \
    smartHomeInfo.cpp

HEADERS = homeAlarmInfo.h \
    smartHomeInfo.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
