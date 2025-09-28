import os, selectors
from .parser import Parser

# LocalChannel is a testing channel that uses pipes to transmit data
class LocalChannel():
    def __init__(self, r, w):
        self.r = r
        self.w = w
        self.on_close_callbacks = []


    def packet_handler(self, packet):
        self.on_packet_callback(self, packet)

    def listen(self, selector, on_packet):
        self.on_packet_callback = on_packet
        self.selector = selector
        self.parser = Parser(self.packet_handler)
        self.selector.register(self.r, selectors.EVENT_READ, self.recv)

    def close(self):
        try:
            self.selector.unregister(self.r)
        except (KeyError, ValueError):
            # File descriptor already unregistered or selector closed
            pass
        try:
            os.close(self.r)
        except OSError:
            # File descriptor already closed
            pass
        try:
            os.close(self.w)
        except OSError:
            # File descriptor already closed
            pass
        for callback in self.on_close_callbacks:
            try:
                callback(self)
            except Exception as e:
                print(e)

    def on_close(self, *callbacks):
        self.on_close_callbacks.extend(callbacks)

    def send(self, packet):
        frame = packet.toFrameBytes()
        try:
            os.write(self.w, frame)
        except Exception as e:
            print('send exception:', e)
            return False
        return True

    def recv(self, r, mask):
        data = []
        try:
            data = os.read(r, 1024)
            # print('read data:' + str(data))
        except:
            self.close()
        self.parser.readBytes(data) if data else self.close()
