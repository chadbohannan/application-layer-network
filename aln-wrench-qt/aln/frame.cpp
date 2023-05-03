#include "ax25frame.h"

QByteArray toFrameBuffer(QByteArray content) {
    return toFrameBuffer(content.data(), content.size());
}

QByteArray toFrameBuffer(const char* content, int len) {
    QByteArray ary;
    QBuffer buffer(&ary);
    buffer.open(QIODevice::Append);
    for (int i = 0; i < len; i++) {
        char b = content[i];
        if (b == End){
            buffer.write(&Esc, 1);
            buffer.write(&EndT, 1);
        } else if (b == Esc) {
            buffer.write(&Esc, 1);
            buffer.write(&EscT, 1);
        } else {
            buffer.write(&b, 1);
        }
    }
    buffer.write(&End, 1);
    buffer.close();
    return ary;
}
