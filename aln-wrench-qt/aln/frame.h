#ifndef AX25FRAME_H
#define AX25FRAME_H

#include <QByteArray>
#include <QBuffer>

// KIS framing
#define BufferSize 4096;
static const char End = 0xC0;
static const char Esc = 0xDB;
static const char EndT = 0xDC;
static const char EscT = 0xDD;

QByteArray toFrameBuffer(QByteArray content);
QByteArray toFrameBuffer(const char* content, int len);

// TODO move AlnParser to Ax25FrameReader

#endif // AX25FRAME_H
