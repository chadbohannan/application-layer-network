/*
An Arduino library for Nordic Semiconductors' proprietary "UART/Serial Port Emulation over BLE" protocol, using ArduinoBLE.

* https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v14.0.0/ble_sdk_app_nus_eval.html
* https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service
* https://github.com/sandeepmistry/arduino-BLEPeripheral/tree/master/examples/serial

Tested using UART console feature in [Adafruit Bluefruit LE Connect](https://apps.apple.com/at/app/adafruit-bluefruit-le-connect/id830125974).
*/

#ifndef __BLE_SERIAL_H__
#define __BLE_SERIAL_H__

#include <Arduino.h>
#include <ArduinoBLE.h>

#define BLE_ATTRIBUTE_MAX_VALUE_LENGTH 20
#define BLE_SERIAL_RECEIVE_BUFFER_SIZE 256

template<size_t N> class ByteRingBuffer {
  private:
    uint8_t ringBuffer[N];
    size_t newestIndex = 0;
    size_t length = 0;
  public:
    void add(uint8_t value) {
      ringBuffer[newestIndex] = value;
      newestIndex = (newestIndex + 1) % N;
      length = min(length + 1, N);
    }
    int pop() { // pops the oldest value off the ring buffer
      if (length == 0) {
        return -1;
      }
      uint8_t result = ringBuffer[(N + newestIndex - length) % N];
      length -= 1;
      return result;
    }
    void clear() {
      newestIndex = 0;
      length = 0;
    }
    int get(size_t index) { // this.get(0) is the oldest value, this.get(this.getLength() - 1) is the newest value
      if (index < 0 || index >= length) {
        return -1;
      }
      return ringBuffer[(N + newestIndex - length + index) % N];
    }
    size_t getLength() { return length; }
};

class ArduinoBLESerial {
  public:
    // singleton instance getter
    static ArduinoBLESerial& getInstance() {
      static ArduinoBLESerial instance; // instantiated on first use, guaranteed to be destroyed
      return instance;
    }

    bool start(const char *name);
    void poll();
    void end();
    size_t available();
    int read(uint8_t* buff, int sz);
    size_t write(uint8_t byte);
    size_t write(uint8_t* buff, int sz);
    void flush();
    bool connected();
    operator bool();

  private:
    ArduinoBLESerial();
    ArduinoBLESerial(ArduinoBLESerial const &other) = delete;  // disable copy constructor
    void addService();
    void operator=(ArduinoBLESerial const &other) = delete;  // disable assign constructor

    ByteRingBuffer<BLE_SERIAL_RECEIVE_BUFFER_SIZE> receiveBuffer;
    size_t numAvailableLines;

    unsigned long long lastFlushTime;
    size_t transmitBufferLength;
    uint8_t transmitBuffer[BLE_ATTRIBUTE_MAX_VALUE_LENGTH];

    const char* SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* TX_CHAR_CCCD = "00002902-0000-1000-8000-00805f9b34fb";

    BLEService uartService = BLEService(SERVICE_UUID);
    BLECharacteristic receiveCharacteristic = BLECharacteristic(RX_CHAR_UUID, BLEWriteWithoutResponse, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);
    BLECharacteristic transmitCharacteristic = BLECharacteristic(TX_CHAR_UUID, BLERead | BLENotify, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);
    BLEDescriptor txDescriptor = BLEDescriptor(TX_CHAR_CCCD, "aln:tx");

    void onReceive(const uint8_t* data, size_t size);
    static void onBLEWritten(BLEDevice central, BLECharacteristic characteristic);
};

#endif
