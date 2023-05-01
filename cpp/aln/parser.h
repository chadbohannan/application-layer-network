#ifndef ALN_PARSER_H
#define ALN_PARSER_H

#include "packet.h"

#define MAX_PACKET_SZ 1024

class Parser {
private:
    char buff[MAX_PACKET_SZ];
    Packet packet;
    void (*onPacket)(*Packet);

public:
    Parser(void (*handler)(*Packet));
    char ingestBytes(char* in, int sz);
    char acceptPacket();
    char parsePacket
}

#endif
