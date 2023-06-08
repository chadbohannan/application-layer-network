"""
Bluetooth Low Energy (BLE) device scanner.
Provide a service_uuid to filter devices advertised services.
"""
import asyncio
from bleak import BleakScanner

class BLEScanner():
    def __init__(self, service_uuid=None):
        self.service_uuid = service_uuid
        self.uart_devices = None

    def scan(self):
        self.uart_devices = dict()
        return asyncio.run(self._scan())

    def _detection_callback(self, device, advertisement_data):
        if not self.service_uuid or self.service_uuid in advertisement_data.service_uuids:
            self.uart_devices[device.address] = advertisement_data.local_name
    
    async def _scan(self, duration=3.0):
        scanner = BleakScanner(detection_callback=self._detection_callback)
        await scanner.start()
        await asyncio.sleep(duration)
        await scanner.stop()
        return self.uart_devices


if __name__ == "__main__":
    print("Scanning for BLE services...")
    scanner = BLEScanner()
    devices = scanner.scan()
    for key in devices.keys():
        print(key, "-", devices[key])
    print("Exiting...")
