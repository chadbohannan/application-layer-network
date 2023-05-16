#ifndef ALNTYPES_H
#define ALNTYPES_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

// Packet framing
#define FRAME_CF_LENGTH 2
#define FRAME_END       0xC0
#define FRAME_ESC       0xDB
#define FRAME_END_T     0xDC
#define FRAME_ESC_T     0xDD

// Control Flag bits (Hamming encoding consumes 5 bits, leaving 11)
#define CF_HAMMING1  0x8000
#define CF_HAMMING2  0x4000
#define CF_HAMMING3  0x2000
#define CF_HAMMING4  0x1000
#define CF_HAMMING5  0x0800
#define CF_NETSTATE  0x0400
#define CF_SERVICE   0x0200
#define CF_SRCADDR   0x0100
#define CF_DESTADDR  0x0080
#define CF_NEXTADDR  0x0040
#define CF_SEQNUM    0x0020
#define CF_ACKBLOCK  0x0010
#define CF_CONTEXTID 0x0008
#define CF_DATATYPE  0x0004
#define CF_DATA      0x0002
#define CF_CRC       0x0001

// link state maintenance protocol message types
#define NET_ROUTE   byte(0x01) // packet contains route entry
#define NET_SERVICE byte(0x02) // packet contains service entry
#define NET_QUERY   byte(0x03) // packet is a request for content
#define NET_ERROR   byte(0xFF) // packet is an peer error message


uint16 readUint16(uint8* buffer);
uint32 readUint32(uint8* buffer);

void writeUint16(uint8* buffer, uint16 value);
void writeUint32(uint8* buffer, uint32 value);

#endif
