"""
This is still a work in progress, but the intent is to map BLE IO with pythons async Stream IO.
It is still a bit patchy.


Dependencies
pip3 install bleak, aiofiles
"""
import os
import asyncio
import aiofiles
from bleak import BleakClient
from bleak import BleakClient
from bleak import _logger as logger
from bleak.uuids import uuid16_dict
from select import select


UART_NU_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
UART_TX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for TX
UART_RX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for RX
BLE_FRAME_SZ = 20

class BLESerial:
    def __init__(self, address, loop):
        self.loop = loop
        self.address = address
        self.r, w = os.pipe()
        self.inbound_reader = os.fdopen(self.r, "rb")
        self.inbound_writer = os.fdopen(w, "wb")
        
        r, w = os.pipe()
        self.outbound_reader = os.fdopen(r, "rb")
        self.outbound_writer = os.fdopen(w, "wb")

        # prevent the file stream from hanging on read
        os.set_blocking(self.inbound_reader.fileno(), False)
        os.set_blocking(self.outbound_reader.fileno(), False)

        self.stop = False


    def notification_handler(self, sender, data):
        """Simple notification handler which prints the data received."""
        # print("{0}: {1}".format(sender, data))
        # print("received:", data)
        self.inbound_writer.write(data)
        self.inbound_writer.flush()

    def get_reader(self):
        return self.inbound_reader

    def read(self):
        return self.inbound_reader.read(block=False)

    def send(self, data):
        self.outbound_writer.write(data)
        self.outbound_writer.flush()

    async def run(self, on_data):
        async with BleakClient(self.address, loop=self.loop) as client:
            #wait for BLE client to be connected
            if not client.is_connected:
                print("Failed to connect to", self.addess)
                return
            print("Connected to", self.address)

            #configure client to listen for data to be sent from client
            await client.start_notify(UART_RX_UUID, self.notification_handler)

            # TODO repace select with loop.add_reader()
            inputs = [self.outbound_reader, self.inbound_reader]
            while not self.stop:                
                files_to_read, files_to_write, exceptions = select(inputs, [], inputs, .1)
                if self.outbound_reader in files_to_read:
                    data = self.outbound_reader.read(BLE_FRAME_SZ)
                    print("sending:", data)
                    await client.write_gatt_char(UART_TX_UUID,data)
                if self.inbound_reader in files_to_read:
                    data = self.inbound_reader.read()
                    on_data(data)
                await asyncio.sleep(0.01)

async def listen(bleSerial):
    async with aiofiles.open(bleSerial.r, mode='rb') as f:
        while True:
            contents = await f.read()
            await asyncio.sleep(0.01)
            if contents:
                print("BLESerial listen:", contents)

