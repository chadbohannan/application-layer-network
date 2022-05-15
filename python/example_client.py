import selectors, signal, socket, sched, time
from threading import Lock
from aln.tcpchannel import TcpChannel
from aln.router import Router
from aln.packet import Packet

def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-client-1") # TODO dynamic address allocation protocol
    router.start()

    # connect to an existing node in the network
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8181))

    # join the network
    router.add_channel(TcpChannel(sock))

    # listen for ^C
    lock = Lock()
    def signal_handler(signal, frame):
        router.close()
        sel.close()
        lock.release() # release the main thread
    signal.signal(signal.SIGINT, signal_handler)

    def on_pong(packet):
        print('received:', str(bytes(packet.data)))
        # no more packets expected for this context
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
