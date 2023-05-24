# ArduinoBLESerial

Original work:
https://github.com/Uberi/Arduino-HardwareBLESerial
Changes are made to replace string-helpers with data-helpers.

Implements the Nordic BLE UART protocol. Provides a serial byte stream to enable packet streaming to support for higher level data protocols.
BLE data is limited to 20 bytes per packet. Outbound data is fragmented. Inbound data may be fragments, consumer is expected to parse frame delimiters.
