#include <stdio.h>
#include "./aln/parser.h"

// used to generate test frame feed into a parser and 
// expect a call to our packet handler
int idx = 0;
uint8 frameBuffer[1024];
void bufferWriter(uint8 data) {
    frameBuffer[idx++] = data;
}

void handler(Packet* p) {
    printf("packet recieved, dst: %d %.*s\n",
        p->dstSz, p->dstSz, p->dst
    );
}


int main() {
    uint8 destAddr[] = "test";

    // compose a ticket
    Packet packet;
    packet.clear();
    packet.dst = destAddr;
    packet.dstSz = 4;

    // KIS frame the serialized packet to a buffer
    Framer f(bufferWriter);
    packet.write(&f);
    f.end();

    Parser parser(handler);
    parser.ingestFrameBytes(frameBuffer, idx+1);

    return 0;
}
