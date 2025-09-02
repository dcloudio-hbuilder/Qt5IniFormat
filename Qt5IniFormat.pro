QT -= gui

TEMPLATE = lib
DEFINES += QT5INIFORMAT_LIBRARY

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    qt5iniformat.cpp \
    qt5iniimpl.cpp

HEADERS += \
    Qt5IniFormat_global.h \
    qt5iniformat.h \
    qt5iniimpl.h

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

