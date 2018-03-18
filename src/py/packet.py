# Packet class can compose and decompose packets for frame-link transit


def readINT16U(buffer):
    if (len(buffer) != 2):
        raise ValueError('readINT16U cannot read buffer of size:%d' % len(buffer))
    value = buffer[0]  << 8
    value |= buffer[1]
    return value

def writeINT16U(value):
    buffer = []
    buffer.append(value >> 8 & 0xFF)
    buffer.append(value & 0xFF)
    return buffer

def readINT32U(buffer):
    if (len(buffer) != 4):
        raise ValueError('readINT32U cannot read buffer of size: %d' % len(buffer))
    value = buffer[0] << 24
    value |= buffer[1] << 16
    value |= buffer[2] << 8
    value |= buffer[3]
    return value


def writeINT32U(value):
    buffer = []
    buffer.append(value >> 24 & 0xFF)
    buffer.append(value >> 16 & 0xFF)
    buffer.append(value >> 8 & 0xFF)
    buffer.append(value & 0xFF)
    return buffer

def crc(pdata):
    c = 0
    i = 0
    j = 0
    bit = 0
    crc = 0xFFFFFFFF;
    for c in pdata:
        j = 0x0001
        while (j <= 0x0080):
            bit = crc & 0x80000000;
            crc <<= 1
            if((c&j)>0): bit ^= 0x80000000
            if(bit>0): crc ^= 0x4C11DB7
            j <<= 1

    # reverse
    crc=(((crc& 0x55555555)<< 1)|((crc& 0xAAAAAAAA)>> 1))
    crc=(((crc& 0x33333333)<< 2)|((crc& 0xCCCCCCCC)>> 2))
    crc=(((crc& 0x0F0F0F0F)<< 4)|((crc& 0xF0F0F0F0)>> 4))
    crc=(((crc& 0x00FF00FF)<< 8)|((crc& 0xFF00FF00)>> 8))
    crc=(((crc& 0x0000FFFF)<<16)|((crc& 0xFFFF0000)>>16))
    return (crc^ 0xFFFFFFFF) & 0xFFFFFFFF


def CFHamEncode(value):   # perform G matrix
    return (value &  0x07FF) |\
        (IntXOR(value &  0x071D) << 12) |\
        (IntXOR(value &  0x04DB) << 13) |\
        (IntXOR(value &  0x01B7) << 14) |\
        (IntXOR(value &  0x026F) << 15);

hamDecodMap = {
    0x0F: 0x0001,
    0x07: 0x0002,
    0x0B: 0x0004,
    0x0D: 0x0008,
    0x0E: 0x0010,
    0x03: 0x0020,
    0x05: 0x0040,
    0x06: 0x0080,
    0x0A: 0x0100,
    0x09: 0x0200,
    0x0C: 0x0400
}

def CFHamDecode(value): # perform H matrix
    err = IntXOR(value & 0x826F) |\
        (IntXOR(value & 0x41B7) << 1) |\
        (IntXOR(value & 0x24DB) << 2) |\
        (IntXOR(value & 0x171D) << 3) &\
        0xFF; # mask into single byte

    value ^= hamDecodMap.get(err, 0)
    return value


def IntXOR(n):
  cnt = 0;
  while(n): # This loop will only execute the number times equal to the number of ones. */
    cnt ^= 0x1;
    n &= (n - 0x1);
  return cnt


class Packet:

    # Packet framing
    FRAME_LEADER_LENGTH = 4
    FRAME_CF_LENGTH     = 2
    FRAME_LEADER        = 0x3C
    FRAME_ESCAPE        = 0xC3

    # Control Flag bits (Hamming encoding consumes 5 bits, leaving 11)
    CF_UNUSED4   = 0x0400
    CF_UNUSED3   = 0x0200
    CF_UNUSED2   = 0x0100
    CF_UNUSED1   = 0x0080
    CF_LINKSTATE = 0x0040
    CF_SRCADDR   = 0x0020
    CF_DESTADDR  = 0x0010
    CF_SEQNUM    = 0x0008
    CF_ACKBLOCK  = 0x0004
    CF_DATA      = 0x0002
    CF_CRC       = 0x0001

    # Packet header field sizes
    CF_FIELD_SIZE         = 2 # INT16U
    LINKSTATE_FIELD_SIZE  = 1 # INT08U enumerated
    SRCADDR_FIELD_SIZE    = 2 # INT16U
    DESTADDR_FIELD_SIZE   = 2 # INT16U
    SEQNUM_FIELD_SIZE     = 2 # INT16U
    ACKBLOCK_FIELD_SIZE   = 4 # INT32U
    DATALENGTH_FIELD_SIZE = 2 # INT16U
    CRC_FIELD_SIZE        = 4 # INT32U

    MAX_HEADER_SIZE = CF_FIELD_SIZE + \
      LINKSTATE_FIELD_SIZE + \
      SRCADDR_FIELD_SIZE + \
      DESTADDR_FIELD_SIZE + \
      SEQNUM_FIELD_SIZE + \
      ACKBLOCK_FIELD_SIZE + \
      DATALENGTH_FIELD_SIZE

    MAX_DATA_SIZE = 1024 # small number to prevent constrained device stack overflow
    MAX_PACKET_SIZE = MAX_HEADER_SIZE + MAX_DATA_SIZE + CRC_FIELD_SIZE


    # parse packet from incoming data, if available
    def __init__(self, body):

        if (body != None):
            self.parsePacketBytes(body)

    @staticmethod
    def headerLength(controlFlags):
        len = 2 # length of controlFlags
        if(controlFlags & Packet.CF_LINKSTATE):
            len += Packet.LINKSTATE_FIELD_SIZE
        if(controlFlags & Packet.CF_SRCADDR):
            len += Packet.SRCADDR_FIELD_SIZE
        if(controlFlags & Packet.CF_DESTADDR):
            len += Packet.DESTADDR_FIELD_SIZE
        if(controlFlags & Packet.CF_SEQNUM):
            len += Packet.SEQNUM_FIELD_SIZE
        if(controlFlags & Packet.CF_ACKBLOCK):
            len += Packet.ACKBLOCK_FIELD_SIZE
        if(controlFlags & Packet.CF_DATA):
            len += Packet.DATALENGTH_FIELD_SIZE
        return len

    @staticmethod
    def headerFieldOffset(controlFlags, field):
      offset = 2

      if (field == Packet.CF_LINKSTATE):
          return offset
      if (controlFlags & Packet.CF_LINKSTATE):
          offset += Packet.LINKSTATE_FIELD_SIZE

      if (field == Packet.CF_SRCADDR):
          return offset
      if (controlFlags & Packet.CF_SRCADDR):
          offset += Packet.SRCADDR_FIELD_SIZE

      if (field == Packet.CF_DESTADDR):
          return offset
      if (controlFlags & Packet.CF_DESTADDR):
          offset += Packet.DESTADDR_FIELD_SIZE

      if (field == Packet.CF_SEQNUM):
          return offset;
      if (controlFlags & Packet.CF_SEQNUM):
          offset += Packet.SEQNUM_FIELD_SIZE

      if (field == Packet.CF_ACKBLOCK):
          return offset
      if (controlFlags & Packet.CF_ACKBLOCK):
          offset += Packet.ACKBLOCK_FIELD_SIZE

      if (field == Packet.CF_DATA):
          return offset
      return None # TODO enumerate errors

    # returns a link-frame ready block of bytes to be transmitted
    def toFramedBuffer(self): # returns a byte-stuffed array
        packetBuffer = self.toPacketBuffer()
        frameBuffer = []

        # write frame leader
        for i in range(0, 4):
            frameBuffer.append(Packet.FRAME_LEADER_LENGTH)

        # write byte-stuffed packet body
        delimCount = 0
        for byt in packetBuffer:
            frameBuffer.append(byt)
            delimCount = (0, delimCount + 1)[byt == Packet.FRAME_LEADER]
            if (delimCount == (Packet.FRAME_LEADER_LENGTH - 1) ):
                frameBuffer.append(Packet.FRAME_ESCAPE)
                delimCount = 0

        return frameBuffer

    def toPacketBuffer(self): # returns a un-framed array
        # establish packet structure
        self.controlFlags = Packet.CF_CRC
        if (self.linkState):   self.controlFlags |= Packet.CF_LINKSTATE
        if (self.srcAddr):     self.controlFlags |= Packet.CF_SRCADDR
        if (self.destAddr):    self.controlFlags |= Packet.CF_DESTADDR
        if (self.seqNum):      self.controlFlags |= Packet.CF_SEQNUM
        if (self.ackBlock):    self.controlFlags |= Packet.CF_ACKBLOCK
        if (self.dataSize):    self.controlFlags |= Packet.CF_DATA

        # set EDAC bits
        self.controlFlags = CFHamEncode(self.controlFlags);

        packetBuffer = []
        packetBuffer.extend(writeINT16U(self.controlFlags))


        if (self.controlFlags & Packet.CF_LINKSTATE):
            packetBuffer.append(self.linkState);

        if (self.controlFlags & Packet.CF_SRCADDR):
            packetBuffer.extend(writeINT16U(self.srcAddr))

        if (self.controlFlags & Packet.CF_DESTADDR):
            packetBuffer.extend(writeINT16U(self.destAddr))

        if (self.controlFlags & Packet.CF_SEQNUM):
            packetBuffer.extend(writeINT16U(self.seqNum))

        if (self.controlFlags & Packet.CF_DATA):
            packetBuffer.extend(writeINT16U(self.dataSize))
            packetBuffer.extend(self.data)

        # compute CRC and append it to the buffer
        if (self.controlFlags & Packet.CF_CRC):
            self.crcSum = crc(packetBuffer);
            crcBytes = writeINT32U(self.crcSum)
            packetBuffer.extend(crcBytes)

        return packetBuffer

    # unpacks a recieved packet into local variables
    def parsePacketBytes(self, packetBuffer):
        # initialize fields
        self.controlFlags = 0x0000
        self.linkState = 0x00
        self.srcAddr   = 0x0000
        self.destAddr  = 0x0000
        self.seqNum    = 0x0000
        self.ackBlock  = 0x00000000
        self.dataSize  = 0
        self.data      = []
        self.crcSum    = 0x00000000

        if (len(packetBuffer) < 2):
            self.error = "packet < 2 bytes long"
            return None

        cfBytes = packetBuffer[0:Packet.CF_FIELD_SIZE]
        self.controlFlags = readINT16U(cfBytes)
        offset = Packet.CF_FIELD_SIZE # length of controlFlags
        # self.controlFlags = CFHamDecode(self.controlFlags)

        if(self.controlFlags & Packet.CF_LINKSTATE):
            self.linkState = packetBuffer[offset]
            offset += Packet.LINKSTATE_FIELD_SIZE # 1

        if(self.controlFlags & Packet.CF_SRCADDR):
            srcBytes = packetBuffer[offset:offset+Packet.SRCADDR_FIELD_SIZE]
            self.srcAddr = readINT16U(srcBytes)
            offset += Packet.SRCADDR_FIELD_SIZE

        if(self.controlFlags & Packet.CF_DESTADDR):
            destBytes = packetBuffer[offset:offset+Packet.DESTADDR_FIELD_SIZE]
            self.destAddr = readINT16U(destBytes)
            offset += Packet.DESTADDR_FIELD_SIZE

        if(self.controlFlags & Packet.CF_SEQNUM):
            seqBytes = packetBuffer[offset:offset+Packet.SEQNUM_FIELD_SIZE]
            self.seqNum = readINT16U(seqBytes)
            offset += Packet.SEQNUM_FIELD_SIZE

        if(self.controlFlags & Packet.CF_ACKBLOCK):
            ackBytes = packetBuffer[offset:offset+Packet.ACKBLOCK_FIELD_SIZE]
            self.ackBlock = readINT16U(ackBytes)
            offset += Packet.ACKBLOCK_FIELD_SIZE

        if(self.controlFlags & Packet.CF_DATA):
            lengthBytes = packetBuffer[offset:offset+Packet.DATALENGTH_FIELD_SIZE]
            self.dataSize = readINT16U(lengthBytes)
            offset += Packet.DATALENGTH_FIELD_SIZE
            self.data = packetBuffer[offset:offset+self.dataSize]
            # print 'parsed %d data bytes: %s' % (self.dataSize, "".join(map(chr, self.data)))


        # print 'parsePacketBytes, cf: %x body size:%d' % (self.controlFlags, len(packetBuffer))

        return None
