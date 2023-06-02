
/*
 * Derived from:
 * https://github.com/Uberi/Arduino-HardwareBLESerial
 */

#include "ArduinoBLESerial.h"

ArduinoBLESerial &bleSerial = ArduinoBLESerial::getInstance();
uint8_t buff[20];

void setup() {
  Serial.begin(115200);
  Serial.println("hello world");
  if (!bleSerial.start("echo-box-1")) {
    Serial.begin(115200);
    while (true) {
      Serial.println("failed to initialize ArduinoBLESerial!");
      delay(1000);
    }
  }
}

int t0 = 0;
char hi[] = "hello\n";
bool wasConnected = false;

void loop() {
  // this must be called regularly to perform BLE updates
  bleSerial.poll();

  while (bleSerial.available() > 0) {
    Serial.println(bleSerial.available());
    int n = bleSerial.read(buff, 20);
    Serial.write(buff, n);
    bleSerial.write(buff, n);
  }
  int now = millis();
  if(bleSerial.connected()) {
    if(!wasConnected) {
      Serial.println("connected");
      wasConnected = false;
    }
    if (now-t0 > 5000 || t0 > now) {
      wasConnected = true;
      t0 = millis();
      bleSerial.write((uint8_t*)hi, 6);
      Serial.write((uint8_t*)hi, 6);
    }
  } else if(wasConnected) {
    Serial.println("disconnected");
    wasConnected = false;
  }
}
