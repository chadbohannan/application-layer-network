# Parser class can compose and decompose packets for frame-link transit
from .packet import *
from .frame import FRAME_END, FRAME_ESC, FRAME_END_T, FRAME_ESC_T

class Parser:
    STATE_BUFFERING = 0
    STATE_ESCAPED = 1

    # requires a callback function that will be called when a packet is parsed
    # from incremental data
    def __init__(self, packet_callback):
        self.packet_callback = packet_callback
        self.clear()

    def clear(self):
        self.state = Parser.STATE_BUFFERING
        self.controlFlags = 0x0000
        self.frameBuffer = []
        
    def acceptPacket(self):
        packet = Packet(self.frameBuffer)
        err = None
        if (self.packet_callback):
            err = self.packet_callback(packet) # deliver to application code
        self.clear()
        return err

    # consumes data incrementally and emits calls to packet_callback when full
    # packets are parsed from the incoming stream
    def readBytes(self, buffer):
        for msg in buffer:
            if msg == FRAME_END:
                self.acceptPacket()
            elif self.state == Parser.STATE_ESCAPED:
                if msg == FRAME_END_T:
                    self.frameBuffer.append(FRAME_END)
                elif msg == FRAME_ESC_T:
                    self.frameBuffer.append(FRAME_ESC)                    
                self.state = Parser.STATE_BUFFERING
            elif (msg == FRAME_ESC):
                self.state = Parser.STATE_ESCAPED
            else:
                self.frameBuffer.append(msg)
