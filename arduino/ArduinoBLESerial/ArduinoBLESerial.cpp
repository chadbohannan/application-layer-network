#include "ArduinoBLESerial.h"

ArduinoBLESerial::ArduinoBLESerial() {
  this->numAvailableLines = 0;
  this->transmitBufferLength = 0;
  this->lastFlushTime = 0;
}

bool ArduinoBLESerial::beginAndSetupBLE(const char *name) {
  if (!BLE.begin()) { return false; }
  BLE.setLocalName(name);
  BLE.setDeviceName(name);
  this->begin();
  BLE.advertise();
  return true;
}

void ArduinoBLESerial::begin() {
  BLE.setAdvertisedService(uartService);
  uartService.addCharacteristic(receiveCharacteristic);
  uartService.addCharacteristic(transmitCharacteristic);
  receiveCharacteristic.setEventHandler(BLEWritten, ArduinoBLESerial::onBLEWritten);
  BLE.addService(uartService);
}

void ArduinoBLESerial::poll() {
  if (millis() - this->lastFlushTime > 100) {
    flush();
  } else {
    BLE.poll();
  }
}

void ArduinoBLESerial::end() {
  this->receiveCharacteristic.setEventHandler(BLEWritten, NULL);
  this->receiveBuffer.clear();
  flush();
}

size_t ArduinoBLESerial::available() {
  return this->receiveBuffer.getLength();
}

int ArduinoBLESerial::read(uint8_t* buff, int sz) {
  if (available() < sz)
    sz = available();
  for(int i=0; i< sz; i++)
    buff[i] = this->receiveBuffer.pop();
  return sz;
}

size_t ArduinoBLESerial::write(uint8_t byte) {
  if (this->transmitCharacteristic.subscribed() == false) {
    return 0;
  }
  this->transmitBuffer[this->transmitBufferLength] = byte;
  this->transmitBufferLength ++;
  if (this->transmitBufferLength == sizeof(this->transmitBuffer)) {
      flush();
  }
  return 1;
}

size_t ArduinoBLESerial::write(uint8_t* buff, int sz) {
  for(int i =0; i <sz; i++)
    write(buff[i]);
  return sz;
}

void ArduinoBLESerial::flush() {
  if (this->transmitBufferLength > 0) {
    this->transmitCharacteristic.setValue(this->transmitBuffer, this->transmitBufferLength);
    this->transmitBufferLength = 0;
  }
  this->lastFlushTime = millis();
  BLE.poll();
}

bool ArduinoBLESerial::connected() {
  return BLE.connected();
}

ArduinoBLESerial::operator bool() {
  return BLE.connected();
}

void ArduinoBLESerial::onReceive(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < min(size, sizeof(this->receiveBuffer)); i++) {
    this->receiveBuffer.add(data[i]);
    if (data[i] == '\n') {
      this->numAvailableLines ++;
    }
  }
}

void ArduinoBLESerial::onBLEWritten(BLEDevice central, BLECharacteristic characteristic) {
  ArduinoBLESerial::getInstance().onReceive(characteristic.value(), characteristic.valueLength());
}
