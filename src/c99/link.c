/*********************************************************************************************************************
 *					   Space Science & Engineering Lab - MSU
 *					     Maia University Nanosat Program
 *
 *							IMPLEMENTATION
 * Filename	  : link.c
 * Created	  : 15, Nov 2006
 * Description:
 **********************************************************************************************************************/
#include "link.h"

INT16U readINT16U(INT08U* buffer);
void writeINT16U(INT08U* buffer, INT16U value);
INT16U readINT32U(INT08U* buffer);
void writeINT32U(INT08U* buffer, INT16U value);

int headerLength(INT16U controlFlags)
{
  int headerLength = 6;
  if(controlFlags & CF_LINKSTATE) headerLength += LINKSTATE_FIELD_SIZE;
  if(controlFlags & CF_TIMESTAMP) headerLength += TIMESTAMP_FIELD_SIZE;
  if(controlFlags & CF_CALLSIGN) headerLength += CALLSIGN_FIELD_SIZE;
  if(controlFlags & CF_SRCADDR) headerLength += SRCADDR_FIELD_SIZE;
  if(controlFlags & CF_DESTADDR) headerLength += DESTADDR_FIELD_SIZE;
  if(controlFlags & CF_SEQNUM) headerLength += SEQNUM_FIELD_SIZE;
  if(controlFlags & CF_ACKBLOCK) headerLength += ACKBLOCK_FIELD_SIZE;
  if(controlFlags & CF_DATA) headerLength += DATALENGTH_FIELD_SIZE;
  if(controlFlags & CF_CRC) headerLength += CRC_FIELD_SIZE;
  return headerLength;
}

int headerFieldOffset(INT16U controlFlags, INT16U field)
{
  int offset = 6;

  if (field == CF_LINKSTATE) return offset;
  if (controlFlags & CF_LINKSTATE) offset += LINKSTATE_FIELD_SIZE;

  if (field == CF_TIMESTAMP) return offset;
  if (controlFlags & CF_TIMESTAMP) offset += TIMESTAMP_FIELD_SIZE;

  if (field == CF_CALLSIGN) return offset;
  if (controlFlags & CF_CALLSIGN) offset += CALLSIGN_FIELD_SIZE;

  if (field == CF_SRCADDR) return offset;
  if (controlFlags & CF_SRCADDR) offset += SRCADDR_FIELD_SIZE;

  if (field == CF_DESTADDR) return offset;
  if (controlFlags & CF_DESTADDR) offset += DESTADDR_FIELD_SIZE;

  if (field == CF_SEQNUM) return offset;
  if (controlFlags & CF_SEQNUM) offset += SEQNUM_FIELD_SIZE;

  if (field == CF_ACKBLOCK) return offset;
  if (controlFlags & CF_ACKBLOCK) offset += ACKBLOCK_FIELD_SIZE;

  if (field == CF_DATA) return offset;

  return -4; // TODO enumerate errors
}


int writePacketToBuffer(Packet* packet, INT08U* packetBuffer, int* packetSize, int bufferSize)
{
  int offset;

  // establish packet structure
  packet->controlFlags = CF_CRC;
  if (packet->linkState)   packet->controlFlags |= CF_LINKSTATE;
  if (packet->timeStamp)   packet->controlFlags |= CF_TIMESTAMP;
  if (packet->callSign[0]) packet->controlFlags |= CF_CALLSIGN;
  if (packet->srcAddr)     packet->controlFlags |= CF_SRCADDR;
  if (packet->destAddr)    packet->controlFlags |= CF_DESTADDR;
  if (packet->seqNum)      packet->controlFlags |= CF_SEQNUM;
  if (packet->ackBlock)    packet->controlFlags |= CF_ACKBLOCK;
  if (packet->dataSize)    packet->controlFlags |= CF_DATA;

  *packetSize = headerLength(packet->controlFlags) + packet->dataSize + CRC_FIELD_SIZE;
  if (bufferSize < *packetSize) {
    return -1; // TODO enumerate errors
  }

  offset = 0;
  for (int i = 0; i < FRAME_LEADER_LENGTH; i++)
  packetBuffer[offset++] = FRAME_LEADER;

  writeINT16U(&packetBuffer[offset], CFHamEncode(packet->controlFlags));
  offset += 2;

  if (packet->controlFlags & CF_LINKSTATE)
  {
    packetBuffer[offset] = packet->linkState;
    offset += LINKSTATE_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_TIMESTAMP)
  {
    writeINT32U(&packetBuffer[offset], packet->timeStamp);
    offset += TIMESTAMP_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_CALLSIGN)
  {
    for (int i = 0; i < CALLSIGN_FIELD_SIZE; i++)
    {
      packetBuffer[offset++] = packet->callSign[i];
    }
  }

  if (packet->controlFlags & CF_SRCADDR)
  {
    writeINT16U(&packetBuffer[offset], packet->srcAddr);
    offset += SRCADDR_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_DESTADDR)
  {
    writeINT16U(&packetBuffer[offset], packet->destAddr);
    offset += DESTADDR_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_SEQNUM)
  {
    writeINT16U(&packetBuffer[offset], packet->seqNum);
    offset += SEQNUM_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_DATA)
  {
    writeINT16U(&packetBuffer[offset], packet->dataSize);
    offset += DATALENGTH_FIELD_SIZE;

    // TODO bytestuffing
    for (int i = 0; i < packet->dataSize; i++)
    {
      packetBuffer[offset++] = packet->data[i];
    }
  }

  if (packet->controlFlags & CF_CRC)
  {
    packet->crcSum = getCRC(packetBuffer, offset);
    writeINT32U(&packetBuffer[offset], packet->crcSum);
    offset += CRC_FIELD_SIZE;
  }

  if (offset != *packetSize) {
    return offset; // TODO enumerate errors
  }

  return 0;
}

int readPacketFromBuffer(Packet* packet, INT08U* packetBuffer)
{
  int offset = FRAME_LEADER_LENGTH + FRAME_CF_LENGTH;
  packet->controlFlags = readINT16U(&packetBuffer[offset]);
  if (packet->controlFlags & CF_LINKSTATE)
  {
    packet->linkState = packetBuffer[offset];
    offset += LINKSTATE_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_TIMESTAMP)
  {
    packet->timeStamp = readINT32U(&packetBuffer[offset]);
    offset += TIMESTAMP_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_CALLSIGN)
  {
    for (int i = 0; i < CALLSIGN_FIELD_SIZE; i++)
    {
      packet->callSign[i] = packetBuffer[offset++];
    }
  }

  if (packet->controlFlags & CF_SRCADDR)
  {
    packet->srcAddr = readINT16U(&packetBuffer[offset]);
    offset += SRCADDR_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_DESTADDR)
  {
    packet->destAddr = readINT16U(&packetBuffer[offset]);
    offset += DESTADDR_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_SEQNUM)
  {
    packet->seqNum = readINT16U(&packetBuffer[offset]);
    offset += SEQNUM_FIELD_SIZE;
  }

  if (packet->controlFlags & CF_DATA)
  {
    packet->dataSize = readINT16U(&packetBuffer[offset]);
    offset += DATALENGTH_FIELD_SIZE;
    // TODO remove bytestuffed chars
    for (int i = 0; i < packet->dataSize; i++)
    {
       packet->data[i] = packetBuffer[offset++];
    }
  }

  if (packet->controlFlags & CF_CRC)
  {
    packet->crcSum = readINT32U(&packetBuffer[offset]);
  }
  return 0;
}

int acceptPacket(Parser* parser)
{
  Packet packet; // declare memory on the stack

  // populate a Packet struct from the raw data
  readPacketFromBuffer(&packet, parser->packetBuffer);

  // deliver to application code
  if (parser->packet_callback) {
    parser->packet_callback(&packet);
  } else {
    return -3; // TODO enumerate errors
  }
  parser->state = STATE_FINDSTART;
  parser->headerIndex = 0;
  parser->delimCount = 0;

  return 0;
}

// consumes a byte stream and generates calls to packet_callback
int parseBytes(Parser* parser, INT08U* buffer, int numBytes)
{
  INT08U msg;
  // consume input one byte at a time
  for (int i = 0; i < numBytes; i++)
  {
    msg = buffer[i]; // read next byte in sequence

    // check for escape char (occurs mid-frame)
    if((parser->delimCount == (FRAME_LEADER_LENGTH-1)) && (msg == FRAME_ESCAPE))
    { // reset FRAME_LEADER detection
      parser->delimCount = 0;
      continue;
    }
    else if(msg == FRAME_LEADER)
    {
      parser->delimCount++;
      if(parser->delimCount == FRAME_LEADER_LENGTH)
      {
        for (int i = 0; i < FRAME_LEADER_LENGTH; i++)
          parser->packetBuffer[i] = FRAME_LEADER;
        parser->headerIndex = FRAME_LEADER_LENGTH;
        parser->state = STATE_GET_CF;
        continue;
      }
    }
    else /* reset delim count */
      parser->delimCount = 0;

    /* use current char in following state */
    switch(parser->state)
    {
    case STATE_FINDSTART:
        /* Do Nothing, dump char */
      break; /* end STATE_FINDSTART */

    case STATE_GET_CF:
      if(parser->headerIndex>=MAX_HEADER_SIZE)
      {   // TODO error reporting
        parser->state = STATE_FINDSTART;
        break;
      }
      parser->packetBuffer[parser->headerIndex] = msg;
      parser->headerIndex++;
      if(parser->headerIndex==6)
      { /* Ham CF */
        parser->controlFlags = CFHamDecode(*((INT16U*)&parser->packetBuffer[FRAME_LEADER_LENGTH]));
        (*((INT16U*)&parser->packetBuffer[FRAME_LEADER_LENGTH])) = parser->controlFlags;
        parser->headerLength = headerLength(parser->controlFlags);
        parser->state = STATE_GETHEADER;
      }
      break; /* end STATE_GET_CF */

    case STATE_GETHEADER:
      if(parser->headerIndex>=MAX_HEADER_SIZE)
      { // TODO error reproting
        parser->state = STATE_FINDSTART;
        break;
      }
      parser->packetBuffer[parser->headerIndex] = msg;
      parser->headerIndex++;
      if(parser->headerIndex>=parser->headerLength)
      {
        if (parser->controlFlags & CF_DATA)
        {
          parser->dataIndex = 0;
          parser->dataLength = (*((INT16U*)&parser->packetBuffer[headerFieldOffset(parser->controlFlags, CF_DATA)]));
          parser->data = parser->packetBuffer + parser->headerLength; // set the data pointer to an offset in packetBuffer
          parser->state = STATE_GETDATA;
        }
        else if (parser->controlFlags & CF_CRC)
        {
          parser->crcIndex = 0;
          parser->dataLength = 0;
          parser->crcLength = CRC_FIELD_SIZE;
          parser->crcSum = parser->data + parser->dataLength;
          parser->state = STATE_GETCRC;
        }
        else
        {
          acceptPacket(parser);
        }
      }
      break; /* end STATE_GETHEADER */

    case STATE_GETDATA:
      parser->data[parser->dataIndex] = msg;
      parser->dataIndex++;
      if(parser->dataIndex >= parser->dataLength)
      {
        if (parser->controlFlags & CF_CRC)
        {
          parser->crcIndex = 0;
          parser->crcLength = CRC_FIELD_SIZE;
          parser->crcSum = parser->data + parser->dataLength;
          parser->state = STATE_GETCRC;
        }
        else {
          acceptPacket(parser);
        }
      }
      break; /* end STATE_GETDATA */

    case STATE_GETCRC:
      parser->crcSum[parser->crcIndex] = msg;
      parser->crcIndex++;
      if(parser->crcIndex >= parser->crcLength)
      {
        int checkSumInputLength = parser->headerLength+parser->dataLength;
        if ( readINT32U(parser->crcSum) != getCRC(parser->packetBuffer, checkSumInputLength))
        { // TODO error reporting
          parser->state = STATE_FINDSTART;
        } else {
          acceptPacket(parser);
        }
      }
      break; /* end STATE_GETCRC */
    } /* end switch(state) */
  }/* end while(forever) */
  return numBytes;
}


INT32U getCRC(INT08U* pdata, INT16U size)
{
  INT08U c;
  INT16U i, j;
  INT32U bit;
  INT32U crc=0xFFFFFFFF;
  for(i=0; i<size; i++)
  {
    c=pdata[i];
    for(j=0x01; j<=0x80; j<<=1)
    {
      bit=crc&0x80000000;
      crc<<=1;
      if((c&j)>0) bit^=0x80000000;
      if(bit>0) crc^=0x4C11DB7;
    }
  }
  /* reverse */
  crc=(((crc&0x55555555)<< 1)|((crc&0xAAAAAAAA)>> 1));
  crc=(((crc&0x33333333)<< 2)|((crc&0xCCCCCCCC)>> 2));
  crc=(((crc&0x0F0F0F0F)<< 4)|((crc&0xF0F0F0F0)>> 4));
  crc=(((crc&0x00FF00FF)<< 8)|((crc&0xFF00FF00)>> 8));
  crc=(((crc&0x0000FFFF)<<16)|((crc&0xFFFF0000)>>16));
  return (crc^0xFFFFFFFF)&0xFFFFFFFF;
}

INT16U CFHamEncode(INT16U value)
{
  /* perform G matrix */
  return (value & 0x07FF)
    | (IntXOR(value & 0x071D) << 12)
    | (IntXOR(value & 0x04DB) << 13)
    | (IntXOR(value & 0x01B7) << 14)
    | (IntXOR(value & 0x026F) << 15);
}

INT16U CFHamDecode(INT16U value)
{
  /* perform H matrix */
  INT08U err = IntXOR(value & 0x826F)
          | (IntXOR(value & 0x41B7) << 1)
          | (IntXOR(value & 0x24DB) << 2)
          | (IntXOR(value & 0x171D) << 3);
  /* don't strip control flags, it will mess up the crc */
  switch(err) /* decode error feild */
  {
    case 0x0F: return value ^ 0x0001;
    case 0x07: return value ^ 0x0002;
    case 0x0B: return value ^ 0x0004;
    case 0x0D: return value ^ 0x0008;
    case 0x0E: return value ^ 0x0010;
    case 0x03: return value ^ 0x0020;
    case 0x05: return value ^ 0x0040;
    case 0x06: return value ^ 0x0080;
    case 0x0A: return value ^ 0x0100;
    case 0x09: return value ^ 0x0200;
    case 0x0C: return value ^ 0x0400;
    default: return value;
  }
}

INT08U IntXOR(INT32U n)
{
  INT08U cnt = 0x0;
  while(n)
  {   /* This loop will only execute the number times equal to the number of ones. */
    cnt ^= 0x1;
    n &= (n - 0x1);
  }
  return cnt;
}


INT16U readINT16U(INT08U* buffer)
{
    INT16U value = buffer[0] << 8;
    value |= buffer[1];
    return value;
}

void writeINT16U(INT08U* buffer, INT16U value)
{
  buffer[0] = value >> 8 & 0xFF;
  buffer[1] = value & 0xFF;
}

INT16U readINT32U(INT08U* buffer)
{
    INT32U value = buffer[0] << 24;
    value |= buffer[1] << 16;
    value |= buffer[2] << 8;
    value |= buffer[3];
    return value;
}

void writeINT32U(INT08U* buffer, INT16U value)
{
  buffer[0] = value >> 24 & 0xFF;
  buffer[0] = value >> 16 & 0xFF;
  buffer[0] = value >> 8 & 0xFF;
  buffer[1] = value & 0xFF;
}
