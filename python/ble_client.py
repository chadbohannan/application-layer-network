import asyncio
import selectors, signal, socket, sched, time
from threading import Lock
from aln.tcp_channel import TcpChannel
from aln.router import Router
from aln.packet import Packet


from ble_scan import BLEScanner
from ble_serial import BLESerial, UART_NU_UUID
from ble_channel import BLEChannel

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    # router = Router(sel, "python-localhost-client-1")
    # router.start()

    # def ping_handler(packet):
    #     print("ping from [{0}]".format(packet.srcAddr))
    #     router.send(Packet(
    #         destAddr=packet.srcAddr,
    #         contextID=packet.contextID,
    #         data="pong"
    #     ))

    # router.register_service("ping", ping_handler)

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



    # TODO this is set up in the router in add_channel
    def on_packet(packet):
        print("packet from [{0}]".format(packet.srcAddr))


    bleSerial = BLESerial(ble_address, loop)
    bleChannel = BLEChannel(bleSerial)
    bleChannel.listen(sel, on_packet)

    
    # loop.create_task(bleSerial.run(ble_address, loop))
    # loop.create_task(listen(bleSerial))

    print("tasks started, sending message")
    # bleSerial.send("hakuna matata".encode("utf-8"))
    # import pdb; pdb.set_trace()
    bleChannel.send(Packet(
        data="test"
    ))

    print("message sent")    
    loop.run_forever()
    print("Exiting...")


if __name__ == "__main__":
    main()
