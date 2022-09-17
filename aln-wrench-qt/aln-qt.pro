QT       += core gui network testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 testcase no_testcase_installs

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addchanneldialog.cpp \
    aln/alntypes.cpp \
    aln/ax25frame.cpp \
    aln/packet.cpp \
    aln/parser.cpp \
    aln/router.cpp \
    aln/tcpchannel.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    addchanneldialog.h \
    aln/alntypes.h \
    aln/ax25frame.h \
    aln/channel.h \
    aln/packet.h \
    aln/parser.h \
    aln/router.h \
    aln/tcpchannel.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    aln-qt_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
