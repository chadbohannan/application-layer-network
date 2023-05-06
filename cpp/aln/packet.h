#ifndef ALN_PACKET_H
#define ALN_PACKET_H

#include "alntypes.h"
#include "framer.h"

// Packet header field sizes
#define CF_FIELD_SIZE           2   // uint16
#define NETSTATE_FIELD_SIZE     1   // uint8 enumerated
#define SERVICE_FIELD_SIZE_MAX  256 // string
#define SRCADDR_FIELD_SIZE_MAX  256 // string
#define DESTADDR_FIELD_SIZE_MAX 256 // string
#define NEXTADDR_FIELD_SIZE_MAX 256 // string
#define SEQNUM_FIELD_SIZE       2   // uint16
#define ACKBLOCK_FIELD_SIZE     4   // uint32
#define CONTEXTID_FIELD_SIZE    2   // uint16
#define DATATYPE_FIELD_SIZE     1   // uint8
#define DATALENGTH_FIELD_SIZE   2   // uint16
#define CRC_FIELD_SIZE          4   // uint32


struct Packet {
    uint16 cf;
    uint8 net;
    uint8* srv;
    uint8 srvSz;
    uint8* src;
    uint8 srcSz;
    uint8* dst;
    uint8 dstSz;
    uint8* nxt;
    uint8 nxtSz;
    uint16 seq;
    uint32 ack;
    uint16 ctx;
    uint8 typ;
    uint8* data;
    uint16 dataSz;
    void clear();
    void write(Framer*);
    void evalCF();
};


void writeOut(Framer*, uint8* buff, int len);

uint8 intXOR(uint32 n);
uint16 cfHamEncode(uint16 value);
uint16 cfHamDecode(uint16 value);

#endif
