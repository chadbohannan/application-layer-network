TEMPLATE = subdirs

SUBDIRS = \
    aln-wrench-qt \
    aln-cli-qt \
    aln-core-tests

# Specify the exact .pro file paths (needed when .pro name doesn't match directory)
aln-wrench-qt.file = aln-wrench-qt/aln-wrench-qt.pro
aln-cli-qt.file = aln-cli-qt/aln-cli-qt.pro
aln-core-tests.file = aln-core-tests/aln-core-tests.pro

# Note: With .pri approach, no build dependencies needed
# Each project compiles the core independently

# Make core library files visible in Qt Creator (doesn't affect build)
OTHER_FILES += \
    aln-cpp-core/aln-cpp-core.pri \
    aln-cpp-core/include/aln/alntypes.h \
    aln-cpp-core/include/aln/frame.h \
    aln-cpp-core/include/aln/channel.h \
    aln-cpp-core/include/aln/localchannel.h \
    aln-cpp-core/include/aln/packet.h \
    aln-cpp-core/include/aln/parser.h \
    aln-cpp-core/include/aln/router.h \
    aln-cpp-core/include/aln/tcpchannel.h \
    aln-cpp-core/src/alntypes.cpp \
    aln-cpp-core/src/frame.cpp \
    aln-cpp-core/src/channel.cpp \
    aln-cpp-core/src/localchannel.cpp \
    aln-cpp-core/src/packet.cpp \
    aln-cpp-core/src/parser.cpp \
    aln-cpp-core/src/router.cpp \
    aln-cpp-core/src/tcpchannel.cpp
