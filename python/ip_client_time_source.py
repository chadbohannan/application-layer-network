import selectors, signal, socket, sched, time
from threading import Lock
from aln.tcp_channel import TcpChannel
from aln.router import Router
from aln.packet import Packet
from datetime import datetime # used to generate data for this example

TCP_HOST = "layer7node.net"
TCP_PORT = 8000
ALN_NODE = "6b404c2d-50d1-4007-94af-1b157c64e4e3"

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-time-source-1")
    router.start()

    # assume a localhost connection on port 8081; see ip_ping_host.py
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((TCP_HOST, TCP_PORT))


    # join the network
    ch = TcpChannel(sock)
    ch.send(Packet(destAddr=ALN_NODE))
    router.add_channel(ch)

    while True:
        time.sleep(5.0)
        now = datetime.now().isoformat()
        print("sending " + now + " to log services")
        msg = router.send(Packet(service="log", data=now))
        if msg: print("on send:", msg)

if __name__ == "__main__":
    main()
