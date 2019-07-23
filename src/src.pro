QT += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fc2mppatcher
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += main.cpp \
        widget.cpp \
        fc2mppatcher.cpp

HEADERS += widget.h \
        fc2mppatcher.h \
        constants.h

FORMS += widget.ui

# Including 3rd party PeLib library.
INCLUDEPATH += $$PWD/../lib/pelib/include
DEPENDPATH += $$PWD/../lib/pelib/include

LIBS += -L$$PWD/../lib/pelib/build/src/pelib -lpelib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
