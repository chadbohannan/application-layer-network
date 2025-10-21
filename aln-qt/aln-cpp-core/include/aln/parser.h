#ifndef PARSER_H
#define PARSER_H

#include <QBuffer>
#include <QObject>
#include "packet.h"


class Parser : public QObject
{
    Q_OBJECT;

    static const char STATE_BUFFERING = 0;
    static const char STATE_ESCAPED = 1;
    char state;
    QByteArray bytes;
    QBuffer buffer;

public:
    Parser();
    void reset();

protected:
    void acceptPacket();

public slots:
    void read(QByteArray);

signals:
    void onPacket(Packet*);
};

#endif // PARSER_H
