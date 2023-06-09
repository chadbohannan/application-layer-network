import selectors
from aln.parser import Parser


class BLEChannel():
    def __init__(self, bleSerial):
        self.bleSerial = bleSerial
        self.on_close_callbacks = []

    def packet_handler(self, packet):
        self.on_packet_callback(self, packet)

    def on_inbound_data(self, data):
        self.parser.readBytes(data)

    def listen(self, selector, on_packet):
        self.on_packet_callback = on_packet
        self.selector = selector
        self.parser = Parser(self.packet_handler)
        self.selector.register(self.bleSerial.get_reader(), selectors.EVENT_READ, self.recv)
        self.bleSerial.loop.create_task(self.bleSerial.run(self.on_inbound_data))

    def snd(self, packet):
        self.bleSerial.send(packet)

    def close(self):
        fd = self.bleSerial.get_reader()
        self.selector.unregister(fd)
        fd.close()
        for callback in self.on_close_callbacks:
            try:
                callback(self)
            except Exception as e:
                print("BLEChannel close exeption:", e)

    def on_close(self, callback):
        self.on_close_callbacks.append(callback)

    def send(self, packet):
        frame = packet.toFrameBytes()
        try:
            self.bleSerial.send(frame)
        except Exception as e:
            print('send exception', e)
            return False
        return True

    def recv(self, fd, mask):
        # data = sock.recv(1024)
        data = fd.read(block=False)
        print("ble_channel recv:", data)
        self.parser.readBytes(data) if data else self.close()

