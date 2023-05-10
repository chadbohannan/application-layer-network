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


// Replace with your network credentials (STATION)
#define ssid "moonraker"
#define password "nickisadick"

WiFiClient client; // TCP client
WiFiUDP Udp;       // UDP client
const short udpBroadcastListenPort = 8082;

#define udpBufferSize 255
char udpBuffer[udpBufferSize]; //buffer to hold incoming packet

char pinOut = 0;

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
char data;


// Mode Resolution SampleTime
//  0    0.5°C       30 ms
//  1    0.25°C      65 ms
//  2    0.125°C     130 ms
//  3    0.0625°C    250 ms
const int sensorResolution = 3;


void setup() {
  //  init serial
  Serial.begin(115200);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards. 
  while(Serial.available() > 0) {
    data = Serial.read();     // Clear the buffer, we can proceed.
  }
  delay(1);
  Serial.println("");
  
  // init sensor
  if (!tempsensor.begin(0x18)) {
    Serial.println("Could not find MCP9808");
  } else {
    Serial.println("Found MCP9808");
    tempsensor.setResolution(sensorResolution);
  }
  delay(1);

  // init wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  delay(250);
  Serial.println(WiFi.localIP());
  delay(250);
  Udp.begin(udpBroadcastListenPort);
  delay(250);

  pinMode(GPIO_D6,OUTPUT);
}

void readSensor() {
  Serial.println("wake up MCP9808.... "); // wake up MCP9808 - power consumption ~200 micro Ampere
  tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, also shows the resolution mode used for reading.
//  Serial.print("Resolution in mode: ");
  Serial.println (tempsensor.getResolution());
  float c = tempsensor.readTempC();
  float f = tempsensor.readTempF();
  Serial.print("Temp: "); 
//  Serial.print(c, 4); Serial.print("*C\t and ");
  Serial.print(f, 4); Serial.println("*F");
  
//  Serial.println("Shutdown MCP9808.... \n");
  tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 micro Ampere, stops temperature sampling 
}

int readUdp(){
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    IPAddress remoteIp = Udp.remoteIP();

    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    int len = Udp.read(udpBuffer, udpBufferSize);
    if (len > 0) {
      udpBuffer[len] = 0;
    }
    Serial.print("Contents:");
    Serial.println(udpBuffer);

    char* protocol = strtok(udpBuffer, "://");
    char* rest = strtok(NULL, "//");
    char* host = strtok(rest, ":");
    String port = strtok(NULL, ":");
    
    Serial.println(protocol);
    Serial.println(host);
    Serial.println(port);
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
    delay(5000);
    return 1;
  }
  Serial.println("connection successful"); 
  return 0;
}

void loop() {
  if (readUdp()) {
    readSensor();
    // TODO pull LOW, then set a timer to release LED
    digitalWrite(GPIO_D6, !digitalRead(GPIO_D6));
    
   
  }
  if(client.connected()) {
      while (client.available()) {
        char ch = static_cast<char>(client.read());
        Serial.print(ch);
      }
  }
}
