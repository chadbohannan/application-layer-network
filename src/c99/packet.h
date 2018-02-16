#ifndef ELPACKET_H
#define ELPACKET_H

/**********************************************************************************************************************
 *                  PRIMATIVES
 **********************************************************************************************************************/
#define INT08U unsigned char
#define INT16U unsigned short
#define INT32U unsigned long

// Packet framing
#define FRAME_LEADER_LENGTH 4
#define FRAME_CF_LENGTH 2
#define FRAME_LEADER 0x3C // '<' (ASCII)
#define FRAME_ESCAPE 0xC3 // 'Ã' (Extended ASCII)

// Control Flag bits
#define CF_LINKSTATE  0x0100
#define CF_TIMESTAMP  0x0080
#define CF_CALLSIGN   0x0040
#define CF_SRCADDR    0x0020
#define CF_DESTADDR   0x0010
#define CF_SEQNUM     0x0008
#define CF_ACKBLOCK   0x0004
#define CF_DATA       0x0002
#define CF_CRC      0x0001

// Packet header field sizes
#define CF_FIELD_SIZE         2 // INT16U
#define LINKSTATE_FIELD_SIZE  1 // INT08U enumerated
#define TIMESTAMP_FIELD_SIZE  4 // INT32U seconds epoch
#define CALLSIGN_FIELD_SIZE   8 // ASCII string
#define SRCADDR_FIELD_SIZE    2 // INT16U
#define DESTADDR_FIELD_SIZE   2 // INT16U
#define SEQNUM_FIELD_SIZE     2 // INT16U
#define ACKBLOCK_FIELD_SIZE   4 // INT32U
#define DATALENGTH_FIELD_SIZE 2 // INT16U
#define CRC_FIELD_SIZE        4 // INT32U

#define MAX_HEADER_SIZE CF_FIELD_SIZE + \
  LINKSTATE_FIELD_SIZE + \
  TIMESTAMP_FIELD_SIZE + \
  CALLSIGN_FIELD_SIZE + \
  SRCADDR_FIELD_SIZE + \
  DESTADDR_FIELD_SIZE + \
  SEQNUM_FIELD_SIZE + \
  ACKBLOCK_FIELD_SIZE + \
  DATALENGTH_FIELD_SIZE + \
  CRC_FIELD_SIZE

#define MAX_DATA_SIZE   1024
#define MAX_PACKET_SIZE MAX_HEADER_SIZE + MAX_DATA_SIZE + CRC_FIELD_SIZE

typedef struct Packet {
  INT16U controlFlags;
  INT08U linkState;
  INT32U timeStamp;
  INT08U callSign[CALLSIGN_FIELD_SIZE];
  INT16U srcAddr;
  INT16U destAddr;
  INT16U seqNum;
  INT32U ackBlock;
  INT16U dataSize;
  INT08U data[MAX_DATA_SIZE];
  INT32U crcSum;
} Packet;

#endif
