import selectors, signal, socket, sys, time
from aln.parser import Packet
from aln.tcphost import TcpHost
from aln.router import Router
from threading import Lock


def main():
    sel = selectors.DefaultSelector() # application event loop
    router = Router(sel, "python-host-1") # TODO dynamic address allocation protocol
    router.start()

    # TODO register ping service
    PING_SERVICE_ID = 1
    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))

    router.register_service(PING_SERVICE_ID, ping_handler)

    def on_connect(tcpChannel, addr):
        print('accepted connection from', addr)
        router.add_channel(tcpChannel)

    tcpHost = TcpHost()
    tcpHost.listen(sel, 'localhost', 8000, on_connect)
    
    def signal_handler(signal, frame): # listen for ^C
        router.close()
        sel.close()
        lock.release()
    signal.signal(signal.SIGINT, signal_handler)

    lock = Lock()
    lock.acquire()
    lock.acquire()
    lock.release()

if __name__ == "__main__":
    main()
