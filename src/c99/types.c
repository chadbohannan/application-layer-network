#include "types.h"


INT16U readINT16U(INT08U* buffer)
{
  INT16U value = ((INT16U)(buffer[0])) << 8;
  value |= buffer[1];
  return value;
}

void writeINT16U(INT08U* buffer, INT16U value)
{
  buffer[0] = value >> 8 & 0xFF;
  buffer[1] = value & 0xFF;
}

INT32U readINT32U(INT08U* buffer)
{
  INT32U value = ((INT32U)(buffer[0])) << 24;
  value |= ((INT32U)(buffer[1])) << 16;
  value |= ((INT32U)(buffer[2])) << 8;
  value |= ((INT32U)(buffer[3]));
  return value;
}

void writeINT32U(INT08U* buffer, INT32U value)
{
  buffer[0] = value >> 24 & 0xFF;
  buffer[1] = value >> 16 & 0xFF;
  buffer[2] = value >> 8 & 0xFF;
  buffer[3] = value & 0xFF;
}
