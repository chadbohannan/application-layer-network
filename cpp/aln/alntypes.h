#ifndef ALNTYPES_H
#define ALNTYPES_H

#define uint8  unsigned char
#define uint16 unsigned short
#define uint32 unsigned int

uint16 readUint16(uint8* buffer);
uint32 readUint32(uint8* buffer);

void writeUint16(uint8* buffer, uint16 value);
void writeUint32(uint8* buffer, uint32 value);

#endif
