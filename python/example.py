import selectors, signal, socket, sys, time
from aln.tcpchannel import TcpChannel
from aln.router import Router

def packet_handler(p):
    print(p.toJsonString())


def main():
    print('connecting')
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8000))
    print('connected')

    tcpChannel = TcpChannel(sock)
    
    # TODO router.add_channel(tcpChannel)
    sel = selectors.DefaultSelector()
    router = Router(sel, 2) # TODO address-request protocol
    router.add_channel(tcpChannel)

    #tcpChannel.listen(sel, packet_handler)


    def signal_handler(signal, frame): # listen for ^C
            tcpChannel.close()
            sel.close()
    signal.signal(signal.SIGINT, signal_handler)

    while True: # enter event loop
        events = sel.select()
        for key, mask in events:
            callback = key.data
            callback(key.fileobj, mask)

print("B")
if __name__ == "__main__":
    print("starting")
    main()