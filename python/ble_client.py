# SPDX-FileCopyrightText: 2020 Dan Halbert for Adafruit Industries
#
# SPDX-License-Identifier: MIT

# Connect to an "eval()" service over BLE UART.

# dependencies:
"""
pip3 install adafruit-circuitpython-ble
"""
import time

from adafruit_ble import BLERadio
from adafruit_ble.advertising.standard import ProvideServicesAdvertisement
from adafruit_ble.services.nordic import UARTService

ble = BLERadio()

uart_connection = None
was_connected = False
while True:
    if not uart_connection:
        print("Trying to connect...")
        for adv in ble.start_scan(ProvideServicesAdvertisement):
            if UARTService in adv.services:
                uart_connection = ble.connect(adv)
                print("Connected", uart_connection)
                break
        ble.stop_scan()

    if uart_connection and uart_connection.connected:
        was_connected = True
        uart_service = uart_connection[UARTService]
        while uart_connection.connected:
            s = input("Tx: ")
            uart_service.write(s.encode("utf-8"))
            uart_service.write(b'\n')
            time.sleep(0.5)
            print(uart_service.in_waiting)       
            print("Rx:", uart_service.read().decode("utf-8"))
            print(uart_service.in_waiting)
    elif was_connected:
        uart_connection = None


print("exiting\n")
