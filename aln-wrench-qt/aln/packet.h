#ifndef PACKET_H
#define PACKET_H

#include "alntypes.h"

#include <QByteArray>
#include <QString>

#define MAX_DATA_SIZE 1024

// Control Flag bits (Hamming encoding consumes 5 bits, leaving 11)
#define CF_NETSTATE  0x0400
#define CF_SERVICE   0x0200
#define CF_SRCADDR   0x0100
#define CF_DESTADDR  0x0080
#define CF_NEXTADDR  0x0040
#define CF_SEQNUM    0x0020
#define CF_ACKBLOCK  0x0010
#define CF_CONTEXTID 0x0008
#define CF_DATATYPE  0x0004
#define CF_DATA      0x0002
#define CF_CRC       0x0001


// Packet header field sizes (static sized fields)
#define CF_FIELD_SIZE         2 // INT16U
#define SEQNUM_FIELD_SIZE     2 // INT16U
#define ACKBLOCK_FIELD_SIZE   4 // INT32U
#define DATALENGTH_FIELD_SIZE 2 // INT16U
#define CRC_FIELD_SIZE        4 // INT32U


class Packet
{
public:
    char net;
    QString srv;
    QString srcAddress;
    QString destAddress;
    QString nxtAddress;
    INT16U seqNum;
    INT32U ackBlock;
    INT16U ctx;
    char type;
    QByteArray data;
    INT32U crc;

    enum NetState {
        ROUTE = 1,
        SERVICE = 2,
        QUERY = 3
    };

public:
    Packet();
    Packet(QByteArray packetBuffer);
    Packet(QString dest, QByteArray content);
    Packet(QString dest, INT16U contextID, QByteArray content);
    Packet(QString dest, QString service, QByteArray content);
    Packet(QString dest, QString service, INT16U contextID, QByteArray content);
    void init(QByteArray);
    void clear();

    INT16U controlField();
    QByteArray toByteArray();

    static Packet parse(QByteArray packetBuffer);

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
    static INT16U CFHamEncode(INT16U value);


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
    static INT16U CFHamDecode(INT16U value);


    /* DESCRIPTION : This is a modified "Sparse Ones Bit Count".
     *               Instead of counting the ones it just determines
     *               if there was an odd or even count of bits by XORing the int.
     * ARGUMENTS   : This is a 32 bit number to count the ones of.
     * RETURNS     : Return 0x0 or 0x1. */
    static INT08U IntXOR(INT32U n);

};

#endif // PACKET_H
