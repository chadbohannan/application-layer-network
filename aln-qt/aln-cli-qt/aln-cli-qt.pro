TEMPLATE = app
QT += core network
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# Include the core library
include(../aln-cpp-core/aln-cpp-core.pri)

SOURCES += \
    main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
