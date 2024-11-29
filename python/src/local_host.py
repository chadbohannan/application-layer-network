import selectors, signal
from alnmeshpy import TcpHost, Router
from threading import Lock


def main():
    sel = selectors.DefaultSelector() # application event loop
    router = Router(sel, "python-host-1") # each Router must have a unique address
    router.start()

    def on_connect(tcpChannel, addr):
        print('accepted connection from', addr)
        tcpChannel.on_close(lambda x: print('closing channel ', addr))
        router.add_channel(tcpChannel)

    tcpHost = TcpHost()
    tcpHost.listen(sel, 'localhost', 8081, on_connect)
    
    def signal_handler(signal, frame): # listen for ^C
        router.close()
        sel.close()
        lock.release() # release the double-lock to exit the app
    signal.signal(signal.SIGINT, signal_handler)

    lock = Lock()
    lock.acquire() # take a lock
    lock.acquire() # double-take the lock (blocks indefinitely)
    lock.release() # politely clean up initial lock

if __name__ == "__main__":
    main()
