
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


int cnt = 0;
char hi[] = "hello";

void loop() {
  // this must be called regularly to perform BLE updates
  bleSerial.poll();

  while (bleSerial.available() > 0) {
    Serial.println(bleSerial.available());
    int n = bleSerial.read(buff, 20);
    Serial.write(buff, n);
    bleSerial.write(buff, n);
  }
  if(bleSerial.connected() & cnt++ < 1) {
    bleSerial.write((uint8_t*)hi, 5);
  }
}
