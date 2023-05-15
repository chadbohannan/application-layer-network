/**************************************************************************/
/*!
This is a demo for the Adafruit MCP9808 breakout
----> http://www.adafruit.com/products/1782
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
*/
/**************************************************************************/
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include "parser.h"
#include "framer.h"

#include <WiFiUdp.h>

#define ONBOARD_LED 2

#define GPIO_D0   16
#define GPIO_D1   5
#define GPIO_D2   4
#define GPIO_D3   0
#define GPIO_D4   2
#define GPIO_D5   14
#define GPIO_D6   12
#define GPIO_D7   13
#define GPIO_D8   15
#define GPIO_D9   3  // RX
#define GPIO_D10  1  // TX
#define GPIO_D11  9  // SD2
#define GPIO_D12  10 // SD3

#define LED_PIN GPIO_D6



// Replace with your network credentials (STATION)
#define ssid "moonraker"
#define password "nickisadick"


WiFiClient client; // TCP client
WiFiUDP Udp;       // UDP client
const short udpBroadcastListenPort = 8082;

#define udpBufferSize 255
char udpBuffer[udpBufferSize]; //buffer to hold incoming packet

char pinOut = 0;

char nodeAddress[] = "ESP8266-09";
char nodeRouteData[] = "\x0A" "ESP8266-09\x0001";


// Create the MCP9808 temperature sensor object
char data;

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();


// Mode Resolution SampleTime
//  0    0.5°C       30 ms
//  1    0.25°C      65 ms
//  2    0.125°C     130 ms
//  3    0.0625°C    250 ms
const int sensorResolution = 3;

// measure elapsed time
int timerMark = 0;
void markTime() {
  timerMark = millis();
}
int elapsed() {
  return millis() - timerMark;
}

void outputWriter(uint8 data) {
  if(client.connected()) {
    client.write(data);
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
    p->setSource((uint8*)nodeAddress, 10);
    p->setData((uint8*)nodeRouteData, 13);

    sendPacket(p);
  }
}

Parser parser(handler);

void setup() {
  //  init serial
  Serial.begin(115200);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards. 
  while(Serial.available() > 0) {
    data = Serial.read();     // Clear the buffer, we can proceed.
  }
  delay(250);
  Serial.println("\n");
  
  // init sensor
  if (!tempsensor.begin(0x18)) {
    Serial.println("Could not find MCP9808");
  } else {
    Serial.println("Found MCP9808");
    tempsensor.setResolution(sensorResolution);
  }
  delay(250);

  // init wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println(WiFi.localIP());
  Udp.begin(udpBroadcastListenPort);
}

float readSensor() {
  // Serial.println("wake up MCP9808.... "); // wake up MCP9808 - power consumption ~200 micro Ampere
  tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, also shows the resolution mode used for reading.
  // Serial.println (tempsensor.getResolution());

  float tempF = tempsensor.readTempF();
  tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 micro Ampere, stops temperature sampling 

  char buff[20];
  sprintf(buff, "Measured: %f*F", tempF);
  Serial.println(buff);
  return tempF;
}

int readUdp(){
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    IPAddress remoteIp = Udp.remoteIP();

    // read the packet into packetBufffer
    int len = Udp.read(udpBuffer, udpBufferSize);
    if (len > 0) {
      udpBuffer[len] = 0;
    }

    char* protocol = strtok(udpBuffer, ":");
    char* host = strtok(NULL, ":/");
    String port = strtok(NULL, ":");

    if (!client.connected()) {
      connectTCP(host, port.toInt());
    }
    return 1;
  }
  return 0;
}


int connectTCP(char* host, int port) {
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return 1;
  }
  Serial.println("connection successful"); 
  return 0;
}


void loop() {
  if(client.connected()) {
      while (client.available()) {
        uint8 ch = static_cast<uint8>(client.read());
        parser.ingestFrameBytes(&ch, 1);
      }
      if (elapsed() > 5000) {
        markTime();

        char data[16];
        int dataSz = sprintf(data, "%0.3fF", readSensor());
        Serial.println(data);
        
        Packet p;
        p.clear();
        char srv[] = "log";
        p.setService((uint8*)srv, 3);
        p.setSource((uint8*)nodeAddress, 10);
        p.setData((uint8*)data, dataSz);
        sendPacket(&p);        
      } else if (elapsed() < 0) {
        Serial.println("timer rollover detected, reseting timer");
        markTime();
      }
  } else {
    readUdp();
  }
}