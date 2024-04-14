
/*
 * Derived from:
 * https://github.com/Uberi/Arduino-HardwareBLESerial
 */

#include <parser.h>
#include <framer.h>
#include <packet.h>
#include "ArduinoBLESerial.h"

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

#define BLE_FRAME_SZ 20
ArduinoBLESerial &bleSerial = ArduinoBLESerial::getInstance();
uint8_t buff[BLE_FRAME_SZ];
bool wasConnected = false;
char echo[] = "echo:";

char nodeAddress[] = "ESP32-black-box-a";
uint8 nodeAdressSize = 17;
char nodeRouteData[] = "\x11" "ESP32-black-box-a" "\x00" "\x01";
uint8 nodeRouteDataSz = 20;

char srv[] = "log";
char data[16];
float value;
int dataSz;

void handler(Packet* p);
Parser parser(handler);


// measure elapsed time
int timerMark = 0;
void markTime() {
  timerMark = millis();
}
int elapsed() {
  return millis() - timerMark;
}

void outputWriter(uint8 data) {
  if(bleSerial.connected()) {
    bleSerial.write(data);
  }
}

void sendPacket(Packet* p) {
  Framer f(outputWriter);
  p->write(&f);
  f.end();
}

void handler(Packet* p) {
  if (p->net == NET_QUERY) {
    char buffer[14];

    p->clear();
    p->net = NET_ROUTE;
    p->setSource((uint8*)nodeAddress, nodeAdressSize);
    p->setData((uint8*)nodeRouteData, nodeRouteDataSz);

    sendPacket(p);
  }
}

float readSensor() {
  uint8 sample = temprature_sens_read();
  return (sample - 32) / 1.8;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-black-box-a...");
  if (!bleSerial.start("ESP32-black-box-a")) {
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
    parser.ingestFrameBytes(buff,  n);

    // echo received content
    // Serial.write("received:");
    // Serial.write(buff, n);
    // bleSerial.write((uint8_t*)echo, 5);
    // bleSerial.write(buff, n);
  }

  if(bleSerial.connected()) {
    if(!wasConnected) {
      Serial.println("connected");
      wasConnected = true;
    }

    if (elapsed() > 2000) {
      value = readSensor();
      dataSz = sprintf(data, "%0.2f*F", value);
      // Serial.println(data); delay(10);
      Packet p;
      p.clear();
      p.setService((uint8*)srv, 3);
      p.setSource((uint8*)nodeAddress, nodeAdressSize);
      p.setData((uint8*)data, dataSz);
      sendPacket(&p);
      markTime();
    }
  } else if(wasConnected) {
    Serial.println("disconnected");
    wasConnected = false;
  }
  if (elapsed() < 0) {
    Serial.println("timer rollover detected, reseting timer");
    markTime();
  }
}
