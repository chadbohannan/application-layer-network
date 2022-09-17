#include "ax25frame.h"

QByteArray toAx25Buffer(QByteArray content) {
    return toAx25Buffer(content.data(), content.size());
}

QByteArray toAx25Buffer(const char* content, int len) {
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
