import asyncio
import selectors, signal, socket, sched, time
from threading import Lock
from aln.tcp_channel import TcpChannel
from aln.router import Router
from aln.packet import Packet


from aln.ble_scan import BLEScanner
from aln.ble_serial import BLESerial, UART_NU_UUID
from aln.ble_channel import BLEChannel

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-localhost-client-1")
    router.start()

    def log_handler(packet):
        print("log from {0}: {1}".format(packet.srcAddr, packet.data.decode("utf-8")))
    router.register_service("log", log_handler)

    # need to grap the event loop before scanning because get_event_loop() fails if called later
    loop = asyncio.get_event_loop()

    scanner = BLEScanner(service_uuid=UART_NU_UUID)
    device_map = scanner.scan()
    if len(device_map) == 0:
        print("No Nordic UART Serial devices found")
        exit(0)
    elif len(device_map) == 1:
        for key in device_map.keys():
            ble_address = key
    else:
        print(len(device_map), "devices found, TODO: device selection")
        exit(0)

    bleSerial = BLESerial(ble_address, loop)
    bleChannel = BLEChannel(bleSerial)
    router.add_channel(bleChannel)
    
    print("Running...")
    try :
        loop.run_forever()
    except Exception as e:
        print(e)
    
    print("Exiting...")


if __name__ == "__main__":
    main()
