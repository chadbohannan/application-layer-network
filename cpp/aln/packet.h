#ifndef ALN_PACKET_H
#define ALN_PACKET_H

#include "alntypes.h"

// Packet framing
#define FRAME_CF_LENGTH = 2
#define FRAME_END       = 0xC0;
#define FRAME_ESC       = 0xDB;
#define FRAME_END_T     = 0xDC;
#define FRAME_ESC_T     = 0xDD;

// Control Flag bits (Hamming encoding consumes 5 bits, leaving 11)
#define CF_HAMMING1  = 0x8000
#define CF_HAMMING2  = 0x4000
#define CF_HAMMING3  = 0x2000
#define CF_HAMMING4  = 0x1000
#define CF_HAMMING5  = 0x0800
#define CF_NETSTATE  = 0x0400
#define CF_SERVICE   = 0x0200
#define CF_SRCADDR   = 0x0100
#define CF_DESTADDR  = 0x0080
#define CF_NEXTADDR  = 0x0040
#define CF_SEQNUM    = 0x0020
#define CF_ACKBLOCK  = 0x0010
#define CF_CONTEXTID = 0x0008
#define CF_DATATYPE  = 0x0004
#define CF_DATA      = 0x0002
#define CF_CRC       = 0x0001


// Packet header field sizes
#define CF_FIELD_SIZE           = 2   // uint16
#define NETSTATE_FIELD_SIZE     = 1   // uint8 enumerated
#define SERVICE_FIELD_SIZE_MAX  = 256 // string
#define SRCADDR_FIELD_SIZE_MAX  = 256 // string
#define DESTADDR_FIELD_SIZE_MAX = 256 // string
#define NEXTADDR_FIELD_SIZE_MAX = 256 // string
#define SEQNUM_FIELD_SIZE       = 2   // uint16
#define ACKBLOCK_FIELD_SIZE     = 4   // uint32
#define CONTEXTID_FIELD_SIZE    = 2   // uint16
#define DATATYPE_FIELD_SIZE     = 1   // uint8
#define DATALENGTH_FIELD_SIZE   = 2   // uint16
#define CRC_FIELD_SIZE          = 4   // uint32

// LinkState value enumerations (TODO support mesh routing)
#define NET_ROUTE   = byte(0x01) // packet contains route entry
#define NET_SERVICE = byte(0x02) // packet contains service entry
#define NET_QUERY   = byte(0x03) // packet is a request for content
#define NET_ERROR   = byte(0xFF) // packet is an peer error message


struct Packet {
    uint16 cf;
    uint8 net;
    char* srv;
    uint8 srvSz;
    char* src;
    uint8 srcSz;
    char* dst;
    uint8 dstSz;
    char* nxt;
    uint8 nxtSz;
    uint16 seq;
    uint32 ack;
    uint16 ctx;
    uint8 typ;
    char* data;
    uint8 dataSz;
};


uint8 intXOR(uint32 n);
uint16 CFHamEncode(uint16 value);
uint16 cfHamDecode(uint16 value);

#endif
