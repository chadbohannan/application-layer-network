
/*
 * Derived from:
 * https://github.com/Uberi/Arduino-HardwareBLESerial
 */

#include "ArduinoBLESerial.h"
#define BLE_FRAME_SZ 20
ArduinoBLESerial &bleSerial = ArduinoBLESerial::getInstance();
uint8_t buff[BLE_FRAME_SZ];
bool wasConnected = false;
char echo[] = "echo:";

void setup() {
  Serial.begin(115200);
  Serial.println("Starting echo-box-1...");
  if (!bleSerial.start("echo-box-1")) {
    while (true) {
      Serial.println("failed to initialize ArduinoBLESerial!");
      delay(1000);
    }
  }
}

void loop() {
  bleSerial.poll(); // must be called regularly to perform BLE updates

  while (bleSerial.available() > 0) {
    int n = bleSerial.read(buff, BLE_FRAME_SZ);
    
    // echo received content
    Serial.write("echo:");
    Serial.write(buff, n);
    bleSerial.write((uint8_t*)echo, 5);
    bleSerial.write(buff, n);
  }

  // debugging
  int now = millis();
  if(bleSerial.connected()) {
    if(!wasConnected) {
      Serial.println("connected");
      wasConnected = true;
    }
  } else if(wasConnected) {
    Serial.println("disconnected");
    wasConnected = false;
  }
}
