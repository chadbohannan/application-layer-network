TEMPLATE = app
QT += core network testlib
QT -= gui

CONFIG += c++17 console testcase
CONFIG -= app_bundle

# Include the core library
include(../aln-cpp-core/aln-cpp-core.pri)

SOURCES += \
    main.cpp \
    test_router.cpp \
    test_ping_service.cpp

HEADERS += \
    lambda_packet_handler.h \
    test_router.h \
    test_ping_service.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
