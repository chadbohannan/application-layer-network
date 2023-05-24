/*
 * https://github.com/Uberi/Arduino-HardwareBLESerial
 */


#include <HardwareBLESerial.h>

HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();

void setup() {
  Serial.begin(115200);
  Serial.println("hello world");
  if (!bleSerial.beginAndSetupBLE("echo-bb-1")) {
    Serial.begin(115200);
    while (true) {
      Serial.println("failed to initialize HardwareBLESerial!");
      delay(1000);
    }
  }
}

void loop() {
  // this must be called regularly to perform BLE updates
  bleSerial.poll();

  while (bleSerial.available() > 0) {
    char byte = bleSerial.read();
    bleSerial.print(byte);
//    Serial.print(byte);
  }
//  delay();
}
