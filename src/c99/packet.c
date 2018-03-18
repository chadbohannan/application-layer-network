#include "packet.h"

int initPacket(Packet* packet)
{
  packet->controlFlags = 0;
  packet->linkState = 0;
  packet->srcAddr = 0;
  packet->destAddr = 0;
  packet->seqNum = 0;
  packet->ackBlock = 0;
  packet->dataSize = 0;
  return 0;
}

int framedCopy(INT08U* frame, INT08U* source, int sourceLength) {
  int delimCount = 0;
  int offset = 0;

  // write frame leader
  for (int i = 0; i < FRAME_LEADER_LENGTH; i++)
    frame[offset++] = FRAME_LEADER;

  for (int i = 0; i < sourceLength; i++)
  {
    INT08U byt = source[i];
    frame[offset++] = byt;
    delimCount = (byt == FRAME_LEADER) ? delimCount + 1 : 0;
    if (delimCount == (FRAME_LEADER_LENGTH - 1) )
    {
      frame[offset++] = FRAME_ESCAPE;
      delimCount = 0;
    }
  }
  return offset;
}

int headerLength(INT16U controlFlags)
{
  int len = 2; // length of control flags
  if(controlFlags & CF_LINKSTATE) len += LINKSTATE_FIELD_SIZE;
  if(controlFlags & CF_SRCADDR) len += SRCADDR_FIELD_SIZE;
  if(controlFlags & CF_DESTADDR) len += DESTADDR_FIELD_SIZE;
  if(controlFlags & CF_SEQNUM) len += SEQNUM_FIELD_SIZE;
  if(controlFlags & CF_ACKBLOCK) len += ACKBLOCK_FIELD_SIZE;
  if(controlFlags & CF_DATA) len += DATALENGTH_FIELD_SIZE;
  return len;
}

int headerFieldOffset(INT16U controlFlags, INT16U field)
{
  int offset = 2;

  if (field == CF_LINKSTATE) return offset;
  if (controlFlags & CF_LINKSTATE) offset += LINKSTATE_FIELD_SIZE;

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

// writes a bytestuffed frame that is ready to stream out
int writePacketToFrameBuffer(Packet* packet, INT08U* frameBuffer, int bufferSize)
{
  INT08U packetBuffer[MAX_PACKET_SIZE];

  // establish packet structure
  packet->controlFlags = CF_CRC;
  if (packet->linkState)   packet->controlFlags |= CF_LINKSTATE;
  if (packet->srcAddr)     packet->controlFlags |= CF_SRCADDR;
  if (packet->destAddr)    packet->controlFlags |= CF_DESTADDR;
  if (packet->seqNum)      packet->controlFlags |= CF_SEQNUM;
  if (packet->ackBlock)    packet->controlFlags |= CF_ACKBLOCK;
  if (packet->dataSize)    packet->controlFlags |= CF_DATA;

  // set EDAC bits
  packet->controlFlags = CFHamEncode(packet->controlFlags);

  writeINT16U(packetBuffer, packet->controlFlags);

  int offset = CF_FIELD_SIZE;

  if (packet->controlFlags & CF_LINKSTATE)
  {
    packetBuffer[offset] = packet->linkState;
    offset += LINKSTATE_FIELD_SIZE;
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

    // copy data block into packetBuffer
    for (int i = 0; i < packet->dataSize; i++)
    {
      packetBuffer[offset++] = packet->data[i];
    }
  }

  // compute CRC and append it to the buffer
  if (packet->controlFlags & CF_CRC)
  {
    packet->crcSum = getCRC(packetBuffer, offset);
    writeINT32U(&packetBuffer[offset], packet->crcSum);
    offset += CRC_FIELD_SIZE;
  }

  // frame the packet with byte-stuffing
  int byteCount = framedCopy(frameBuffer, packetBuffer, offset);

  return byteCount;
}

// populates packet fields from packet contained in packetBuffer
int readPacketFromBuffer(Packet* packet, INT08U* packetBuffer)
{
  int offset = 0;
  packet->controlFlags = readINT16U(&packetBuffer[offset]);
  offset += CF_FIELD_SIZE;

  if (packet->controlFlags & CF_LINKSTATE)
  {
    packet->linkState = packetBuffer[offset];
    offset += LINKSTATE_FIELD_SIZE;
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
