import selectors
from .parser import Parser

class TcpChannel():
    def __init__(self, sock):
        self.sock = sock
        self.sock.setblocking(False)
        self.on_close_callbacks = []

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
        for callback in self.on_close_callbacks:
            try:
                callback(self)
            except Exception as e:
                print(e)

    def on_close(self, callback):
        self.on_close_callbacks.append(callback)

    def send(self, packet):
        frame = packet.toFrameBytes()
        try:
            self.sock.send(frame)
        except Exception as e:
            print('send exception', e)
            return False
        return True

    def recv(self, sock, mask):
        data = sock.recv(1024)
        self.parser.readBytes(data) if data else self.close()
