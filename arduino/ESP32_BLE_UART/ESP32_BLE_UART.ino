/*
 * https://github.com/Uberi/Arduino-HardwareBLESerial
 */

#include <ArduinoBLESerial.h>

ArduinoBLESerial &bleSerial = ArduinoBLESerial::getInstance();

void setup() {
  Serial.begin(115200);
  Serial.println("hello world");
  if (!bleSerial.beginAndSetupBLE("echo-bb-1")) {
    Serial.begin(115200);
    while (true) {
      Serial.println("failed to initialize ArduinoBLESerial!");
      delay(1000);
    }
  }
}

void loop() {
  // this must be called regularly to perform BLE updates
  bleSerial.poll();

  while (bleSerial.available() > 0) {
    Serial.println(bleSerial.available());
    int n = bleSerial.read(buff, 20);
    Serial.write(buff, n);
    bleSerial.write(buff, n);
  }
}
