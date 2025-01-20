import os, selectors, signal
from threading import Lock
from alnmeshpy import LocalChannel, Router,Packet

# this is an integration test that configures 3 nodes
# node 1 advertises a ping service that responds with a 'pong'
# node 2 is an intermediate node that demonstrates multi-hop routing
# node 3 listens for the ping service advertisment and sends a pack to it
# finally node 3 prints the content of the ping packet

def main():
    
    r1a, w1a = os.pipe()
    r2a, w2a = os.pipe()
    ch1a = LocalChannel(r1a, w2a)
    ch2a = LocalChannel(r2a, w1a)

    sel = selectors.DefaultSelector()

    # configure the ping handler on node 1
    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router1.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))

    router1 = Router(sel, "aln-node-1")
    router1.start()
    router1.register_service("ping", ping_handler)
    
    # node 2 exists to test mesh networking
    router2 = Router(sel, "aln-node-2")
    router2.start()
    
    router1.add_channel(ch1a)
    router2.add_channel(ch2a)
    
    r1b, w1b = os.pipe()
    r2b, w2b = os.pipe()
    ch1b = LocalChannel(r1b, w2b)
    ch2b = LocalChannel(r2b, w1b)

    # node 3 will:
    #  listen for the ping service advertisment advertisement
    #  send a 'ping' packet to the advertising node
    #  print the response to the console
    router3 = Router(sel, "aln-node-3")
    router3.start()

    # set up listener for ping response
    def on_packet(packet):
        print(packet.toJsonString())
        print('unpacked response data:', packet.data)
        print('ctrl-C to quit')
    ctxID = router3.register_context_handler(on_packet)

    # set up a service discovery handler for the "ping" service
    def on_service_discovery(service, capacity, address):
        print("on_service_discovery at aln-node-3: {0}:{1}:{2}".format(service, capacity, address))
        if service == "ping":
            print("sending ping with ctxID {0}".format(ctxID))
            text = "hello world".encode(encoding="utf-8")
            router3.send(Packet(service="ping", contextID=ctxID, data=text))
            
    router3.set_on_service_capacity_changed_handler(on_service_discovery)
    router3.add_channel(ch2b)
    router2.add_channel(ch1b)

    def signal_handler(signal, frame):
        sel.close()
    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()

if __name__ == "__main__":
    main()
