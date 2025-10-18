QT       += core gui network bluetooth testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 testcase no_testcase_installs

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addchanneldialog.cpp \
    advertisementitemmodel.cpp \
    advertiserthread.cpp \
    aln/alntypes.cpp \
    aln/frame.cpp \
    aln/channel.cpp \
    aln/localchannel.cpp \
    aln/packet.cpp \
    aln/parser.cpp \
    aln/router.cpp \
    aln/tcpchannel.cpp \
    discoverylisteneritemmodel.cpp \
    knownconnectionitemmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    networkinterfacesitemmodel.cpp \
    openportdialog.cpp \
    packetsenddialog.cpp \
    startadvertisementdialog.cpp \
    startlistendialog.cpp

HEADERS += \
    addchanneldialog.h \
    advertisementitemmodel.h \
    advertiserthread.h \
    aln/alntypes.h \
    aln/frame.h \
    aln/channel.h \
    aln/localchannel.h \
    aln/packet.h \
    aln/parser.h \
    aln/router.h \
    aln/tcpchannel.h \
    discoverylisteneritemmodel.h \
    knownconnectionitemmodel.h \
    mainwindow.h \
    networkinterfacesitemmodel.h \
    openportdialog.h \
    packetsenddialog.h \
    startadvertisementdialog.h \
    startlistendialog.h

FORMS += \
    mainwindow.ui

RESOURCES = aln-qt.qrc

TRANSLATIONS += \
    aln-qt_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
