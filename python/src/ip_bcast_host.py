"""
TCP/IP host with UDP advertisement broadcast.
This example broacasts opens a TCP socket and bro URL to 
"""
import selectors, signal, socket, time
from alnmeshpy import Packet, Router, TcpHost
from threading import Lock, Thread

tcp_port = 8081 # TCP server listen port
udp_port = 8082

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('255.255.255.255', 1))
        return s.getsockname()[0]
    except:
        return '127.0.0.1'
    finally:
        s.close()

ip=get_local_ip()
bcast = '.'.join(ip.split('.')[:3]) + '.255'
advert = "tcp+aln://" + ip + ":" + str(tcp_port)

def bcast_advert():
    udpSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    udpSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    udpSocket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    while True:
        time.sleep(3)
        # print("advertising:", advert)
        udpSocket.sendto(advert.encode('utf-8'), (bcast, udp_port))


def main():
    print("using interface:", ip)
    
    sel = selectors.DefaultSelector() # application event loop
    router = Router(sel, "python-host-advertised-1")
    router.start()

    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))

    router.register_service("ping", ping_handler)

    def on_connect(tcpChannel, addr):
        print('accepted connection from', addr)
        tcpChannel.on_close(lambda x: print('closing channel ', addr))
        router.add_channel(tcpChannel)

    tcpHost = TcpHost()
    tcpHost.listen(sel, ip, tcp_port, on_connect)
    
    Thread(target=bcast_advert).run() # advertise the service

    lock = Lock()
    def signal_handler(signal, frame): # listen for ^C
        router.close()
        sel.close()
        lock.release()
    signal.signal(signal.SIGINT, signal_handler)
    lock.acquire()
    lock.acquire()
    lock.release()

if __name__ == "__main__":
    main()
