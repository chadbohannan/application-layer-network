#import "packet.h"

void Packet::clear() {
  cf = 0;
  net = 0;
  srv = 0;
  srvSz = 0;
  src = 0;
  srcSz = 0;
  dst = 0;
  dstSz = 0;
  nxt = 0;
  nxtSz = 0;
  seq = 0;
  ack = 0;
  ctx = 0;
  typ = 0;
  data = 0;
  dataSz = 0;
}

void Packet::write(void (*out)(uint8*,int)) {
  uint16 cf = 0;
}

uint8 intXOR(uint32 n)
{
  uint8 cnt = 0x0;
  while(n)
  {   /* This loop will only execute the number times equal to the number of ones. */
    cnt ^= 0x1;
    n &= (n - 0x1);
  }
  return cnt;
}

uint16 CFHamEncode(uint16 value)
{
  /* perform G matrix */
  return (value & 0x07FF)
    | (intXOR(value & 0x071D) << 12)
    | (intXOR(value & 0x04DB) << 13)
    | (intXOR(value & 0x01B7) << 14)
    | (intXOR(value & 0x026F) << 15);
}

uint16 cfHamDecode(uint16 value)
{
  /* perform H matrix */
  uint8 err = intXOR(value & 0x826F)
          | (intXOR(value & 0x41B7) << 1)
          | (intXOR(value & 0x24DB) << 2)
          | (intXOR(value & 0x171D) << 3);
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
