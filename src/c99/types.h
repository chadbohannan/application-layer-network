#ifndef ELTYPES_H
#define ELTYPES_H

/**********************************************************************************************************************
 *                  PRIMATIVES
 **********************************************************************************************************************/
#define INT08U unsigned char
#define INT16U unsigned short
#define INT32U unsigned long

/**********************************************************************************************************************
 *                  I/O PROTOTYPES
 **********************************************************************************************************************/
INT16U readINT16U(INT08U* buffer);
void writeINT16U(INT08U* buffer, INT16U value);
INT32U readINT32U(INT08U* buffer);
void writeINT32U(INT08U* buffer, INT32U value);

#endif
