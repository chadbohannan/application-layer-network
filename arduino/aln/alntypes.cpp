#include "alntypes.h"


uint16 readUint16(uint8* buffer)
{
  uint16 value = ((uint16)(buffer[0])) << 8;
  value |= buffer[1];
  return value;
}

uint32 readUint32(uint8* buffer)
{
  uint32 value = ((uint32)(buffer[0])) << 24;
  value |= ((uint32)(buffer[1])) << 16;
  value |= ((uint32)(buffer[2])) << 8;
  value |= ((uint32)(buffer[3]));
  return value;
}

void writeUint16(uint8* buff, uint16 value)
{
  buff[0] = value >> 8 & 0xFF;
  buff[1] = value & 0xFF;
}

void writeUint32(uint8* buff, uint32 value)
{
  buff[0] = value >> 24 & 0xFF;
  buff[1] = value >> 16 & 0xFF;
  buff[2] = value >> 8 & 0xFF;
  buff[3] = value & 0xFF;
}
