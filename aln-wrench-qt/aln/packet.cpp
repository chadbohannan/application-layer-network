#include "packet.h"
#include "alntypes.h"

#include <QByteArray>
#include <QBuffer>

Packet::Packet() {
    clear();
}

Packet::Packet(QByteArray packetBuffer) {
    clear();
    init(packetBuffer);
}

Packet::Packet(QString dest, QByteArray content) {
    clear();
    destAddress = dest;
    data = content;
}

Packet::Packet(QString dest, INT16U contextID, QByteArray content) {
    clear();
    destAddress = dest;
    ctx = contextID;
    data = content;
}

Packet::Packet(QString dest, QString service, QByteArray content) {
    clear();
    destAddress = dest;
    srv = service;
    data = content;
}

void Packet::init(QByteArray data) {
    INT08U* pData = (INT08U*)data.data();
    INT16U controlFlags = readINT16U(pData);
    controlFlags = CFHamDecode(controlFlags);
    int offset = 2;
    if (controlFlags & CF_NETSTATE) {
        net = pData[offset];
        offset += 1;
    }
    if (controlFlags & CF_SERVICE) {
        int srvSize = pData[offset];
        offset += 1;
        srv = QString(data.mid(offset, offset+srvSize));
        offset += srvSize;
    }
    if (controlFlags & CF_SRCADDR) {
        int sz = pData[offset];
        offset += 1;
        srcAddress = QString(data.mid(offset, sz));
        offset += sz;
    }
    if (controlFlags & CF_DESTADDR) {
        int sz = pData[offset];
        offset += 1;
        destAddress = QString(data.mid(offset, sz));
        offset += sz;
    }
    if (controlFlags & CF_NEXTADDR) {
        int sz = pData[offset];
        offset += 1;
        nxtAddress = QString(data.mid(offset, sz));
        offset += sz;
    }
    if (controlFlags & CF_SEQNUM) {
        seqNum = readINT16U(pData+offset);
        offset += 2;
    }
    if (controlFlags & CF_ACKBLOCK) {
        ackBlock = readINT32U(pData+offset);
        offset += 4;
    }
    if (controlFlags & CF_CONTEXTID) {
        ctx = readINT16U(pData+offset);
        offset += 2;
    }
    if (controlFlags & CF_DATATYPE) {
        type = pData[offset];
        offset += 1;
    }
    if (controlFlags & CF_DATA) {
        int dataLength = readINT16U(pData+offset);
        offset += 2;
        this->data = data.mid(offset, dataLength);
        offset += dataLength;
    }
    if (controlFlags & CF_CRC) {
//        int crcPacket = readINT32U(pData, offset);
//        int crcCalc = CRC32(pData, 0, offset);
//        if (crcPacket != crcCalc) {
//            // TODO error handling
//        }
    }
}

INT16U Packet::controlField() {
    INT16U controlField = 0;
    if (net != 0) controlField |= CF_NETSTATE;
    if (srv.length()) controlField |= CF_SERVICE;
    if (srcAddress.length()) controlField |= CF_SRCADDR;
    if (destAddress.length()) controlField |= CF_DESTADDR;
    if (nxtAddress.length()) controlField |= CF_NEXTADDR;
    if (seqNum) controlField |= CF_SEQNUM;
    if (ackBlock) controlField |= CF_ACKBLOCK;
    if (ctx != 0) controlField |= CF_CONTEXTID;
    if (type != 0) controlField |= CF_DATATYPE;
    if (data.length()) controlField |= CF_DATA;
    if (crc) controlField |= CF_CRC;
    controlField = CFHamEncode(controlField);
    return controlField;
}

QByteArray Packet::toByteArray() {
    QByteArray ary;
    QBuffer buffer(&ary);
    buffer.open(QIODevice::Append);
    INT16U controlField = this->controlField();
    writeToBuffer(&buffer, controlField);
    if ((controlField & CF_NETSTATE) != 0) {
        buffer.write(&net, 1);
    }
    if ((controlField & CF_SERVICE) != 0) {
        char sz = srv.length();
        buffer.write(&sz, 1);
        writeToBuffer(&buffer, srv);
    }
    if ((controlField & CF_SRCADDR) != 0) {
        char sz = srcAddress.size();
        buffer.write(&sz, 1);
        writeToBuffer(&buffer, srcAddress);
    }
    if((controlField & CF_DESTADDR)!=0) {
        char sz = destAddress.size();
        buffer.write(&sz, 1);
        writeToBuffer(&buffer, destAddress);
    }
    if((controlField & CF_NEXTADDR)!=0) {
        char sz = nxtAddress.size();
        buffer.write(&sz, 1);
        writeToBuffer(&buffer, nxtAddress);
    }
    if((controlField & CF_SEQNUM)!=0) {
        writeToBuffer(&buffer, seqNum);
    }
    if((controlField & CF_ACKBLOCK)!=0) {
        writeToBuffer(&buffer, ackBlock);
    }
    if((controlField & CF_CONTEXTID)!=0) {
        writeToBuffer(&buffer, ctx);
    }
    if((controlField & CF_DATATYPE)!=0) {
        buffer.write(&type, 1);
    }
    if((controlField &  CF_DATA)!=0) {
        INT16U sz = data.length();
        writeToBuffer(&buffer, sz);
        buffer.write(data);
    }
    if((controlField &  CF_CRC)!=0) {
        writeToBuffer(&buffer, crc);
    }
    buffer.close();
    return ary;
}

void Packet::clear() {
    net = 0;
    srv.clear();
    srcAddress.clear();
    destAddress.clear();
    nxtAddress.clear();
    seqNum = 0;
    ackBlock = 0;
    ctx = 0;
    type = 0;
    data.clear();
    crc = 0;
}

INT16U Packet::CFHamEncode(INT16U value)
{
  /* perform G matrix */
  return (value & 0x07FF)
    | (IntXOR(value & 0x071D) << 12)
    | (IntXOR(value & 0x04DB) << 13)
    | (IntXOR(value & 0x01B7) << 14)
    | (IntXOR(value & 0x026F) << 15);
}

INT16U Packet::CFHamDecode(INT16U value)
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

INT08U Packet::IntXOR(INT32U n)
{
  INT08U cnt = 0x0;
  while(n)
  {   /* This loop will only execute the number times equal to the number of ones. */
    cnt ^= 0x1;
    n &= (n - 0x1);
  }
  return cnt;
}
