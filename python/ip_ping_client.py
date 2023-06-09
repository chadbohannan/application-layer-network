import selectors, signal, socket, sched, time
from threading import Lock
from aln.tcp_channel import TcpChannel
from aln.router import Router
from aln.packet import Packet

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-localhost-client-1")
    router.start()

    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))

    router.register_service("ping", ping_handler)


    # assume a localhost connection on port 8081; see ip_ping_host.py
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081))

    # join the network
    router.add_channel(TcpChannel(sock))
    time.sleep(0.01) # let the router finish service discovery

    # listen for ^C
    lock = Lock()
    def signal_handler(signal, frame):
        router.close()
        sel.close()
        lock.release() # release the main thread
    signal.signal(signal.SIGINT, signal_handler)

    def on_pong(packet):
        global pong_count
        print('received:', str(packet.data), 'from:', packet.srcAddr)
        pong_count += 1
        if pong_count == 2:
            # no more packets expected for this context
            print('releasing context')
            router.release_context(packet.contextID)            

    ctxID = router.register_context_handler(on_pong)
    msg = router.send(Packet(service="ping", contextID=ctxID))
    if msg: print("on send:", msg)

    # hang until ^C
    lock.acquire() # take the lock
    lock.acquire() # enqueue a lock request to block the application
    lock.release() # pong response recieved; clear the lock and exit

if __name__ == "__main__":
    main()
