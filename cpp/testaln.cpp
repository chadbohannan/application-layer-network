#include <stdio.h>
#include "./aln/parser.h"

// used to generate test frame feed into a parser and 
// expect a call to our packet handler
int idx = 0;
uint8 frameBuffer[1024];
void bufferWriter(uint8 data) {
    frameBuffer[idx++] = data;
}

bool received = false;
void handler(Packet* p) {
    printf("packet recieved, dst: %d %.*s\n",
        p->dstSz, p->dstSz, p->dst
    );
    received = true;
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

    printf("received %d\n", received);

    // create a parser that gives us packets from a byte stream
    Parser parser(handler);

    // feed the frameBuffer into the parser 
    // and expect the packet handler to be called
    parser.ingestFrameBytes(frameBuffer, idx+1);

    printf("received %d\n", received);

    return 0;
}
