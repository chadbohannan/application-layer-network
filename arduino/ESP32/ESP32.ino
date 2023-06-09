/**************************************************************************
ESP32 demo of streaming internal temperature sensor data to an ALN hub
**************************************************************************/
#include <WiFi.h>
#include <AsyncUDP.h>
#include "parser.h"
#include "framer.h"

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

// Replace with your network credentials (STATION)
#define ssid ""
#define password ""

#define udpBufferSize 255
char udpBuffer[udpBufferSize]; //buffer to hold incoming packet
const short udpBroadcastListenPort = 8082;

WiFiClient client; // TCP client
AsyncUDP udp;

char nodeAddress[] = "ESP32-black-box";
uint8 nodeAdressSize = 15;
char nodeRouteData[] = "\x08" "ESP32-black-box\x0001";
uint8 nodeRouteDataSz = 18;

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
    p->setSource((uint8*)nodeAddress, nodeAdressSize);
    p->setData((uint8*)nodeRouteData, nodeRouteDataSz);

    sendPacket(p);
  }
}

float readSensor() {
  uint8 sample = temprature_sens_read();
  return (sample - 32) / 1.8;
}

int connectTCP(char* host, int port) {
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return 1;
  }
  Serial.println("connection successful"); 
  return 0;
}

void setup() {
  //  init serial
  Serial.begin(115200);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards. 
  char data;
  while(Serial.available() > 0) {
    data = Serial.read();     // Clear the buffer, we can proceed.
  }
  delay(250);
  Serial.println("\n");

  // init wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println(WiFi.localIP());
  udp.listen(udpBroadcastListenPort);
  udp.onPacket([](AsyncUDPPacket packet) {
    if (client.connected()){
      Serial.println("Ignoring UDP; TCP is connected");
    } else {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length()); //dlzka packetu
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();

      strcpy(udpBuffer, (char*)packet.data());

      if (packet.length()) {
        char* protocol = strtok(udpBuffer, ":");
        char* host = strtok(NULL, ":/");
        String port = strtok(NULL, ":");
        
        Serial.println(protocol);
        Serial.println(host);
        Serial.println(port);
        if (!client.connected()) {
          connectTCP(host, port.toInt());
        }
      }
    }
  });
}


void loop() {
  if(client.connected()) {
      while (client.available()) {
        uint8 ch = static_cast<uint8>(client.read());
        parser.ingestFrameBytes(&ch, 1);
      }
      if (elapsed() > 2000) {
        value = readSensor();
        dataSz = sprintf(data, "%0.2f*F", value);
        Serial.println(data); delay(10);
        Packet p;
        p.clear();
        p.setService((uint8*)srv, 3);
        p.setSource((uint8*)nodeAddress, nodeAdressSize);
        p.setData((uint8*)data, dataSz);
        sendPacket(&p);
        markTime();
      }
  }
  if (elapsed() < 0) {
    Serial.println("timer rollover detected, reseting timer");
    markTime();
  }
}
