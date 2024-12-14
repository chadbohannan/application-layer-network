import os, selectors, signal, time
from threading import Lock
from alnmeshpy import LocalChannel, TcpChannel, Router,Packet


def main():
    
    r1a, w1a = os.pipe()
    r2a, w2a = os.pipe()
    ch1a = LocalChannel(r1a, w2a)
    ch2a = LocalChannel(r2a, w1a)

    sel = selectors.DefaultSelector()

    def on_packet(packet):
        print(packet.toJsonString())
    # ch1.listen(sel, on_packet)
    # ch2.send(Packet(data="ping"))

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
    
    
    router2 = Router(sel, "aln-node-2")
    router2.start()
    
    router1.add_channel(ch1a)
    router2.add_channel(ch2a)
    
    r1b, w1b = os.pipe()
    r2b, w2b = os.pipe()
    ch1b = LocalChannel(r1b, w2b)
    ch2b = LocalChannel(r2b, w1b)

    router3 = Router(sel, "aln-node-3")
    router3.start()
    ctxID = router3.register_context_handler(on_packet)

    # set up a service discovery handler for the "ping" service
    def on_service_discovery(service, capacity):
        print("on_service_discovery: {0}:{1}".format(service, capacity))
        if service == "ping":
            print("sending ping with ctxID {0}".format(ctxID))
            text = "hello world".encode(encoding="utf-8")
            router3.send(Packet(service="ping", contextID=ctxID, data=text))
            
    router3.set_on_service_capacity_changed_handler(on_service_discovery)

    router3.add_channel(ch2b)
    router2.add_channel(ch1b)

    # text = "hello world".encode(encoding="utf-8")
    # router3.send(Packet(destAddr='aln-node-1', contextID=ctxID, data=text))

    # text = "hello world".encode(encoding="utf-8")
    # router2.send(Packet(destAddr="aln-node-1", data=text))

    # def recv(r):
    #     data = os.read(r, 1024)
    #     print('read data:' + str(data))

    # sel.register(r1, selectors.EVENT_READ, recv)
    
    # text = "hello world".encode(encoding="utf-8")  
    # os.write(w1, text)

    # while True:
    #     events = sel.select()
    #     for key, mask in events:
    #         callback = key.data  # read_from_pipe in this case
    #         callback(key.fileobj, mask)  # r_fd in this case


    # listen for ^C
    lock = Lock()
    lock.acquire() # take the lock
    def signal_handler(signal, frame):
        # router.close()
        sel.close()
        lock.release() # release the main thread
    signal.signal(signal.SIGINT, signal_handler)   
    lock.acquire() # hang until ^C

if __name__ == "__main__":
    main()
