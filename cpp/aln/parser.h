#ifndef ALN_PARSER_H
#define ALN_PARSER_H

#include "packet.h"

#define MAX_PACKET_SZ 1024

const uint8 STATE_BUFFERING = 0;
const uint8 STATE_ESCAPED = 1;

class Parser {
private:
    uint8 packetBuffer[MAX_PACKET_SZ];
    uint8 pBuffIdx;
    Packet packet;
    uint8 state;
    void (*onPacket)(Packet*);

public:
    Parser(void (*handler)(Packet*));
    void ingestBytes(char* in, int sz);
    void acceptPacket();
    void parsePacket();
    void reset();
};

#endif
