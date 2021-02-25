# Parser class can compose and decompose packets for frame-link transit
from packet import Packet, crc, CFHamDecode, readINT16U, writeINT16U, readINT32U, writeINT32U

class Parser:

    # Packet parsing state enumeration
    STATE_FINDSTART = 0
    STATE_GET_CF    = 1
    STATE_GETHEADER = 2
    STATE_GETDATA   = 3
    STATE_GETCRC    = 4

    # LinkState value enumerations (TODO support source routing)
    LINK_CONNECT   = 0x01
    LINK_CONNECTED = 0x03
    LINK_PING      = 0x05
    LINK_PONG      = 0x07
    LINK_ACKRESEND = 0x09
    LINK_NOACK     = 0x0B
    LINK_CLOSE     = 0x0D

    # requires a callback function that will be called when a packet is parsed
    # from incremental data
    def __init__(self, packet_callback):
        self.packet_callback = packet_callback
        self.clear()

    def clear(self):
        self.state = Parser.STATE_FINDSTART
        self.delimCount = 0
        self.controlFlags = 0x0000
        self.headerIndex = 0
        self.headerLength = 0
        self.dataIndex = 0
        self.dataLength = 0
        self.crcIndex = 0

    def acceptPacket(self):
        packet = Packet(self.frameBuffer)

        # deliver to application code
        if (self.packet_callback):
            self.packet_callback(packet)
        else:
            return -3 # TODO enumerate errors

        self.clear()

        return None

    # consumes data incrementally and emits calls to packet_callback when full
    # packets are parsed from the incoming stream
    def readBytes(self, buffer):
        for msg in buffer:

            # check for escape char (occurs mid-frame)
            if (msg == Packet.FRAME_ESCAPE):
                if (self.delimCount >= (Packet.FRAME_LEADER_LENGTH-1)):
                    self.delimCount = 0 # reset FRAME_LEADER detection
                    continue # drop the char from the stream
            elif(msg == Packet.FRAME_LEADER):
                self.delimCount += 1
                if (self.delimCount >= Packet.FRAME_LEADER_LENGTH):
                    self.delimCount = 0
                    self.headerIndex = 0
                    self.frameBuffer = []
                    self.state = Parser.STATE_GET_CF
                    continue
            else: # not a framing byte; reset delim count
                self.delimCount = 0


            # use current char in following state
            if (self.state == Parser.STATE_FINDSTART):
                # Do Nothing, dump char
                continue # end STATE_FINDSTART

            if (self.state == Parser.STATE_GET_CF):
                if(self.headerIndex > Packet.MAX_HEADER_SIZE):
                    self.state = Parser.STATE_FINDSTART
                else:
                    self.frameBuffer.append(msg)
                    self.headerIndex += 1
                    if (self.headerIndex == 2):
                        cf = readINT16U(self.frameBuffer)
                        self.controlFlags = CFHamDecode(cf)
                        self.headerLength = Packet.headerLength(self.controlFlags)
                        self.state = Parser.STATE_GETHEADER

                continue # end STATE_GET_CF

            if (self.state == Parser.STATE_GETHEADER):
                if(self.headerIndex >= Packet.MAX_HEADER_SIZE):
                    self.state = Parser.STATE_FINDSTART
                else:
                    self.frameBuffer.append(msg)
                    self.headerIndex += 1
                    if(self.headerIndex >= self.headerLength):
                        if (self.controlFlags & Packet.CF_DATA):
                            self.dataIndex = 0
                            dataOffset = Packet.headerFieldOffset(self.controlFlags, Packet.CF_DATA)
                            dataBytes = self.frameBuffer[dataOffset:dataOffset+2]
                            self.dataLength = readINT16U(dataBytes)
                            self.state = Parser.STATE_GETDATA
                        elif (self.controlFlags & Packet.CF_CRC):
                            self.crcIndex = 0
                            self.dataLength = 0
                            self.state = Parser.STATE_GETCRC
                        else:
                            self.acceptPacket()
                continue # end STATE_GETHEADER

            if (self.state == Parser.STATE_GETDATA):
                self.frameBuffer.append(msg)
                self.dataIndex += 1
                if(self.dataIndex >= self.dataLength):

                    if (self.controlFlags & Packet.CF_CRC):
                        self.crcIndex = 0
                        self.state = Parser.STATE_GETCRC
                    else:
                        self.acceptPacket()
                continue # end STATE_GETDATA

            if (self.state == Parser.STATE_GETCRC):
                self.frameBuffer.append(msg)
                self.crcIndex += 1
                if(self.crcIndex >= Packet.CRC_FIELD_SIZE):
                    sz = len(self.frameBuffer)
                    subframeBytes = self.frameBuffer[0:sz-Packet.CRC_FIELD_SIZE]
                    computedCRC = crc(subframeBytes)
                    crcBytes = self.frameBuffer[sz-Packet.CRC_FIELD_SIZE:sz]
                    expectedCRC = readINT32U(crcBytes)
                    if (computedCRC != expectedCRC):
                        # TODO error reporting
                        self.clear()
                    else:
                      self.acceptPacket()
                continue # end STATE_GETCRC
