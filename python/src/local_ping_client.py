import selectors, signal, socket, time
from alnmeshpy import TcpChannel, Router,Packet

def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-ping-client-1")
    router.start()

    def on_pong(packet): # define the ping response handler
        print('received:', str(packet.data), 'from:', packet.srcAddr)
        time.sleep(1) # wait a second then ping again
        router.send(Packet(service="ping", contextID=packet.contextID))
    ctxID = router.register_context_handler(on_pong) # set response handler

    # send a packet when a 'ping' service is discovered
    def on_service_discovery(service, capacity, address):
        print("on_service_discovery: {0}:{1}:{2}".format(service, capacity, address))
        if service == "ping":
            print("sending ping with ctxID {0}".format(ctxID))
            router.send(Packet(destAddr=address, service="ping", contextID=ctxID))      
    router.set_on_service_capacity_changed_handler(on_service_discovery)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081)) # hard-coded address for demonstration

    router.add_channel(TcpChannel(sock)) # let the router operate on this channel

    def signal_handler(signal, frame):
        router.close()
        sel.close()
    signal.signal(signal.SIGINT, signal_handler)   
    signal.pause()

if __name__ == "__main__":
    main()
