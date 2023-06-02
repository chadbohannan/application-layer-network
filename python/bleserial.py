"""
Dependencies Bleak
pip3 install bleak
"""
import os
import platform
import logging
import asyncio
import time
from bleak import BleakClient
from bleak import BleakClient
from bleak import _logger as logger
from bleak.uuids import uuid16_dict

UART_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
UART_TX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for TX
UART_RX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e" #Nordic NUS characteristic for RX

dataFlag = False #global flag to check for new data

class BLESerial:
    def __init__(self, address, loop):
        self.address = address
        self.loop = loop

        # TODO wrap a Bleak instance
        # self.bleak_client = BleakClient(address, loop=loop)

        r, w = os.pipe()

        self.inbound_reader = os.fdopen(r, "rb")
        self.inbound_writer = os.fdopen(w, "wb")

        self.stop = False
        
        
        # TODO outbound pipe

    def notification_handler(self, sender, data):
        """Simple notification handler which prints the data received."""
        print("{0}: {1}".format(sender, data))
        print(data)
        self.inbound_writer.write(data)
        self.inbound_writer.flush()

    def get_reader(self):
        return self.inbound_reader

    
    async def run(self, address, loop):

        async with BleakClient(address, loop=loop) as client:

            #wait for BLE client to be connected
            x = client.is_connected
            print("Connected: {0}".format(x))

            #wait for data to be sent from client
            await client.start_notify(UART_RX_UUID, self.notification_handler)

            while not self.stop:
                await asyncio.sleep(0.01)

                # TODO configure selector to handle read an input pipe

                # #check if we received data
                # global dataFlag
                # if dataFlag :
                #     dataFlag = False

                #     #echo our received data back to the BLE device
                #     data = await client.read_gatt_char(UART_RX_UUID)
                #     self.inbound_writer.write(datat)
                #     self.inbound_writer.flush()
                #     # await client.write_gatt_char(UART_TX_UUID,data)


if __name__ == "__main__":

    #this is MAC of our BLE device
    address = (
        "C8:F0:9E:F6:DD:EA"
    )

    def on_data(data):
        print("data:", data)

    loop = asyncio.get_event_loop()

    bleSerial = BLESerial(address, loop)

    try:
        loop.run_until_complete(bleSerial.run(address, loop))
    finally:
        bleSerial.stop = True
        time.sleep(0.5);
    