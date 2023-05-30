import asyncio
from bleak import BleakScanner

UART_NU_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS service ID
UART_TX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for TX
UART_RX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for RX


async def main():
    devices = await BleakScanner.discover()
    # import pdb; pdb.set_trace()
    for dev in devices:
        ids = dev.details['props']['UUIDs']
        if UART_NU_UUID in ids:
            address = dev.details['props']['Address']
            name = dev.details['props']['Name']
            print("found", name, address)

asyncio.run(main())
