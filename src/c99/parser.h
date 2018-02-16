/** ********************************************************************************************************************
 *					   Space Science & Engineering Lab - MSU
 *					     Maia University Nanosat Program
 *
 *							INTERFACE
 * Filename	   : link.h
 * Created     : 8, May 2006
 * Description :
 **********************************************************************************************************************/
#ifndef  LINK_H
#define  LINK_H

#include "packet.h"

/**********************************************************************************************************************
 *                  RETURN CODES
 **********************************************************************************************************************/
#define  ERROR_NONE          0 // no errors

/**********************************************************************************************************************
 *                  PROTOCOL DEFINITIONS
 **********************************************************************************************************************/

// Packet parsing state enumeration
#define STATE_FINDSTART  0
#define STATE_GET_CF     1
#define STATE_GETHEADER  2
#define STATE_GETDATA    3
#define STATE_GETCRC     4

// LinkState value enumerations (TODO support source routing)
#define  LINK_CONNECT   0x01
#define  LINK_CONNECTED 0x03
#define  LINK_PING      0x05
#define  LINK_PONG      0x07
#define  LINK_ACKRESEND 0x09
#define  LINK_NOACK     0x0B
#define  LINK_CLOSE     0x0D

typedef struct Parser {
  INT08U packetBuffer[MAX_PACKET_SIZE];

  INT08U state;
  INT08U delimCount;
  INT16U controlFlags;

  INT08U headerIndex;
  INT08U headerLength;

  INT16U dataIndex;
  INT16U dataLength;
  INT08U* data; // references the offset in packetbuffer where data starts

  INT08U crcIndex;
  INT08U crcLength;
  INT08U* crcSum;

  void (*packet_callback)(Packet*);
} Parser;

int initParser(Parser* parser, void(Packet*));
int acceptPacket(Parser* parser);
int parseBytes(Parser* parser, INT08U* buffer, int size);

#endif
