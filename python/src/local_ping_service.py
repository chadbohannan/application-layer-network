import selectors, signal, socket, time
from threading import Lock
from alnmeshpy import TcpChannel, Router,Packet

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-ping-service-1")
    router.start()

    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))

    router.register_service("ping", ping_handler)

    # the ALN router is configured and ready for connection(s)
    # this example makes a single connection to a node with a TcpHost configured

    # assume a localhost connection on port 8081; see ip_ping_host.py
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081))

    # join the network
    router.add_channel(TcpChannel(sock))

    # listen for ^C
    lock = Lock()
    lock.acquire() # take the lock
    def signal_handler(signal, frame):
        router.close()
        sel.close()
        lock.release() # release the main thread
    signal.signal(signal.SIGINT, signal_handler)
    lock.acquire() # hang until ^C

if __name__ == "__main__":
    main()
