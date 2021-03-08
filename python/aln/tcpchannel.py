import selectors, signal, socket, sys, time
from parser import Parser

class TcpChannel():
    def __init__(self, sock):
        self.sock = sock
        self.sock.setblocking(False)

    def listen(self, selector, onPacket):
        self.selector = selector
        self.parser = Parser(onPacket)
        self.selector.register(self.sock, selectors.EVENT_READ, self.recv)

    def close(self):
        self.selector.unregister(self.sock)
        self.sock.close()

    def send(packet):
        self.sock.send(packet.toFrameBytes())

    def recv(self, sock, mask):
        # TODO parse packets
        data = sock.recv(1024)
        if data:
            # import pdb; pdb.set_trace()
            print('received', repr(data), 'from ', sock.getsockname())
            self.parser.readBytes(data)
        else:
            print('closing', sock)
            self.selector.unregister(sock)
            sock.close()

def packetHandler(p):
    print(p.toJsonString())

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8000))
tcpChannel = TcpChannel(sock)
sel = selectors.DefaultSelector()
tcpChannel.listen(sel, packetHandler)


def signal_handler(signal, frame):
        sock.close()
        sel.close()
        # sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)

# TODO isolate in a thread in Router
while True:
    events = sel.select()
    for key, mask in events:
        callback = key.data
        callback(key.fileobj, mask)