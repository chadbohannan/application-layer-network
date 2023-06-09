import signal
import sys

import selectors
import socket

from .tcp_channel import TcpChannel

# TODO encapsulate into TcpHost class
sel = selectors.DefaultSelector()
class TcpHost:
    def __init__(self):
        pass

    def accept(self, sock, mask):
        conn, addr = sock.accept()  # Should be ready
        self.on_connect(TcpChannel(conn), addr)

    def listen(self, sel, interface, port, on_connect):
        self.sel = sel
        self.on_connect = on_connect
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((interface, port))
        sock.setblocking(False)
        sel.register(sock, selectors.EVENT_READ, self.accept)
        sock.listen()
