import asyncio
import selectors, signal, socket
from threading import Lock
from alnmeshpy import TcpChannel, Router,Packet

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-ping-client-1")
    router.start()

    # set up the ping response handler
    def on_pong(packet):
        global pong_count
        print('received:', str(packet.data), 'from:', packet.srcAddr)
        pong_count += 1
        if pong_count == 3:
            # no more packets expected for this context
            print('releasing context')
            router.release_context(packet.contextID)            
        else:
            router.send(Packet(service="ping", contextID=ctxID))

    # get a contextID for the ping handler. packets to this node with ctxID will be
    # routed to the on_pong handler. this ctxID is required for this node to properly
    # receive the ping response. 
    ctxID = router.register_context_handler(on_pong)

    # set up a service discovery handler for the "ping" service
    def on_service_discovery(service, capacity):
        print("on_service_discovery: {0}:{1}".format(service, capacity))
        if service == "ping":
            print("sending ping with ctxID {0}".format(ctxID))
            router.send(Packet(service="ping", contextID=ctxID))
            
    router.set_on_service_capacity_changed_handler(on_service_discovery)

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
