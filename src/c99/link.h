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

/**********************************************************************************************************************
 *                  PRIMATIVES
 **********************************************************************************************************************/
#define INT08U unsigned char
#define INT16U unsigned short
#define INT32U unsigned long

/**********************************************************************************************************************
 *                  RETURN CODES
 **********************************************************************************************************************/
#define  HAMERR_NOERR          0 /* no errors */
#define  HAMERR_TOOMANYBITS   -1 /* must be less than 12 bits to encode */
#define  HAMERR_TOOMANYERR    -2 /* more bit flips than could be corrected were found */


/**********************************************************************************************************************
 *                  PROTOCOL DEFINITIONS
 **********************************************************************************************************************/
#define FRAME_LEADER_LENGTH 4
#define FRAME_CF_LENGTH 2
#define FRAME_LEADER 0x3C // '<' (ASCII)
#define FRAME_ESCAPE 0xC3 // 'Ã' (Extended ASCII)

// Packet parsing state enumeration
#define STATE_FINDSTART  0
#define STATE_GET_CF     1
#define STATE_GETHEADER  2
#define STATE_GETDATA    3
#define STATE_GETCRC     4

// Control Flag bit definitions
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
#define LINKSTATE_FIELD_SIZE  1 // INT08U enumerated
#define TIMESTAMP_FIELD_SIZE  4 // INT32U seconds epoch
#define CALLSIGN_FIELD_SIZE   8 // ASCII string
#define SRCADDR_FIELD_SIZE    2 // INT16U
#define DESTADDR_FIELD_SIZE   2 // INT16U
#define SEQNUM_FIELD_SIZE     2 // INT16U
#define ACKBLOCK_FIELD_SIZE   2 // INT16U
#define DATALENGTH_FIELD_SIZE 2 // INT16U
#define CRC_FIELD_SIZE        4 // INT32U

// LinkState value enumerations (TODO support source routing)
#define  LINK_CONNECT   0x01
#define  LINK_CONNECTED 0x03
#define  LINK_PING      0x05
#define  LINK_PONG      0x07
#define  LINK_ACKRESEND 0x09
#define  LINK_NOACK     0x0B
#define  LINK_CLOSE     0x0D

#define MAX_HEADER_SIZE 37
#define MAX_DATA_SIZE   4096
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

typedef struct Parser {
  INT08U msg; // next byte being parsed

  INT08U state;
  INT08U delimCount;

  INT16U controlFlags;

  INT08U headerIndex;
  INT08U headerLength;
  INT08U packetLength;
  INT08U packetBuffer[MAX_HEADER_SIZE];

  INT16U dataIndex;
  INT16U dataLength;
  INT08U* data; // references the offset in packetbuffer where data starts

  INT08U crcIndex;
  INT08U crcLength;
  INT08U* crcSum;

  void (*packet_callback)(const Packet*);

} Parser;

int acceptPacket(Parser* parser);
int parseBytes(Parser* parser, INT08U* buffer, int size);

int writePacketToBuffer(Packet* packet, INT08U* outBuffer, int* packetSize, int bufferSize);
int readPacketFromBuffer(Packet* packet, INT08U* packetBuffer);

/* DESCRIPTION:
 * ARGUMENTS: pdata the buffer, size the buffer size.
 * RETURNS: The CRC32 value for the buffer.
 */
INT32U getCRC(INT08U* pdata, INT16U size);

/*
 * This encodes the CF using a modified Hamming (15,11).
 * Returns the encoded 11 bit CF as a 15 bit codeword.
 * Only the 0x07FF bits are  aloud to be on for the input all others will be ignored.
 * Based off of the following Hamming (15,11) matrix...
 * G[16,11] = [[1000,0000,0000,1111],  0x800F
 *             [0100,0000,0000,0111],  0x4007
 *             [0010,0000,0000,1011],  0x200B
 *             [0001,0000,0000,1101],  0x100D
 *             [0000,1000,0000,1110],  0x080E
 *             [0000,0100,0000,0011],  0x0403
 *             [0000,0010,0000,0101],  0x0205
 *             [0000,0001,0000,0110],  0x0106
 *             [0000,0000,1000,1010],  0x008A
 *             [0000,0000,0100,1001],  0x0049
 *             [0000,0000,0010,1100]]; 0x002C
 */
INT16U CFHamEncode(INT16U value);

/*
 * This decodes the CF using a modified Hamming (15,11).
 * It will fix one error, if only one error occures, not very good for burst errors.
 * This is a SEC (single error correction) which means it has no BED (bit error detection) to save on size.
 * The returned value will be, if fixed properly, the same 11 bits that were sent into the encoder.
 * Bits 0xF800 will be zero.
 * Based off of the following Hamming (15,11) matrix...
 * H[16,4]  = [[1011,1000,1110,1000],  0x171D
 *             [1101,1011,0010,0100],  0x24DB
 *             [1110,1101,1000,0010],  0x41B7
 *             [1111,0110,0100,0001]]; 0x826F
 */
INT16U CFHamDecode(INT16U value);


/* DESCRIPTION : This is a modified "Sparse Ones Bit Count".
 *               Instead of counting the ones it just determines
 *               if there was an odd or even count of bits by XORing the int.
 * ARGUMENTS   : This is a 32 bit number to count the ones of.
 * RETURNS     : Return 0x0 or 0x1. */
INT08U IntXOR(INT32U n);

#endif
