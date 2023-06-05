import asyncio
from bleak import BleakScanner

UART_NU_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS service ID
UART_TX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for TX
UART_RX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for RX

uart_devices = dict()

def detection_callback(device, advertisement_data):
    if UART_NU_UUID in advertisement_data.service_uuids:
        uart_devices[device.address] = advertisement_data

async def scan():
    scanner = BleakScanner(
        filters={"UUIDs":[UART_NU_UUID], "DuplicateData":False},
        detection_callback=detection_callback
    )
    await scanner.start()
    await asyncio.sleep(3.0)
    await scanner.stop()
    print(uart_devices)

asyncio.run(scan())
