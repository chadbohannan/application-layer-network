"""
TCP/IP client with UDP advertisement listener.
This example listens for UDP broadcasts containing connection URLs
"""

import selectors, signal, socket, time
from urllib.parse import urlparse
from socket import AF_INET, SOCK_DGRAM
from threading import Lock
from aln.tcp_channel import TcpChannel
from aln.router import Router
from aln.packet import Packet

def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-listening-client-1") # TODO dynamic address allocation protocol
    router.start()

    def on_log(packet):
        print('log: ' + packet.data.decode('utf-8'))
    router.register_service("log", on_log)

    s=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('', 8082))
    while True:
        m=s.recvfrom(4096)
        print("UDP:", m[0])
        url = urlparse(m[0])
        protocol = url.scheme.decode("utf-8")
        if protocol in ["tcp+aln", "tcp+maln"]:
            break
    s.close()
    
    print('parsed host:', url.scheme, url.hostname, url.port, url.path)

    # connect to an existing node in the network
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((url.hostname, url.port))

    # join the network
    ch = TcpChannel(sock)
    if "maln" in protocol: # support multiplexed links
        ch.send(Packet(destAddr=url.path))
    router.add_channel(TcpChannel(sock))

    # listen for ^C
    lock = Lock()
    def signal_handler(signal, frame):
        router.close()
        sel.close()
        lock.release() # release the main thread
    signal.signal(signal.SIGINT, signal_handler)

    # also exit is the client socket fails
    ch.on_close(signal_handler)

    # hang until ^C
    lock.acquire() # take the lock
    lock.acquire() # enqueue a lock request to block the application
    lock.release() # pong response recieved; clear the lock and exit

if __name__ == "__main__":
    main()