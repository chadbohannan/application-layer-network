import selectors, signal, socket, sys, time
from .parser import Parser

class TcpChannel():
    def __init__(self, sock):
        self.sock = sock
        self.sock.setblocking(False)
        self.on_close = None

    def packet_handler(self, packet):
        self.on_packet_callback(self, packet)

    def listen(self, selector, on_packet):
        self.on_packet_callback = on_packet
        self.selector = selector
        self.parser = Parser(self.packet_handler)
        self.selector.register(self.sock, selectors.EVENT_READ, self.recv)

    def close(self):
        self.selector.unregister(self.sock)
        self.sock.close()
        if self.on_close:
            self.on_close(self)

    def send(self, packet):
        frame = packet.toFrameBytes()
        # print("sending frame:"+str(frame))
        try:
            self.sock.send(frame)
        except Exception as e:
            print('send exception', e)
            return False
        return True

    def recv(self, sock, mask):
        data = sock.recv(1024)
        if data:
            self.parser.readBytes(data)
        else:
            self.selector.unregister(sock)
            sock.close()
            if self.on_close:
                self.on_close(self)
