#ifndef ALNTYPES_H
#define ALNTYPES_H

#include <QBuffer>


#define INT08U unsigned char
#define INT16U unsigned short
#define INT32U unsigned int

INT16U readINT16U(INT08U* buffer);
INT32U readINT32U(INT08U* buffer);

void writeINT16U(INT08U* buffer, INT16U value);
void writeINT32U(INT08U* buffer, INT32U value);

void writeToBuffer(QBuffer* buffer, INT32U value);
void writeToBuffer(QBuffer* buffer, INT16U value);
void writeToBuffer(QBuffer* buffer, QString value);

#endif
