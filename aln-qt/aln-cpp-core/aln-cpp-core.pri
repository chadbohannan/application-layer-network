# ALN Core Library Include File
# Include this in your project with: include(../aln-cpp-core/aln-cpp-core.pri)

QT += core network

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/src

HEADERS += \
    $$PWD/include/aln/alntypes.h \
    $$PWD/include/aln/frame.h \
    $$PWD/include/aln/channel.h \
    $$PWD/include/aln/localchannel.h \
    $$PWD/include/aln/packet.h \
    $$PWD/include/aln/parser.h \
    $$PWD/include/aln/router.h \
    $$PWD/include/aln/tcpchannel.h

SOURCES += \
    $$PWD/src/alntypes.cpp \
    $$PWD/src/frame.cpp \
    $$PWD/src/channel.cpp \
    $$PWD/src/localchannel.cpp \
    $$PWD/src/packet.cpp \
    $$PWD/src/parser.cpp \
    $$PWD/src/router.cpp \
    $$PWD/src/tcpchannel.cpp
