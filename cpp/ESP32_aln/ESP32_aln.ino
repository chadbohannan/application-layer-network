/**************************************************************************
ESP32 demo of streaming internal temperature sensor data to an ALN hub
**************************************************************************/
#include <WiFi.h>
#include <AsyncUDP.h>
#include <parser.h>
#include <framer.h>

// LED is active low on the ESP-01
const int LED_OFF = HIGH;
const int LED_ON  = LOW;

hw_timer_t *My_timer = NULL;
int ledState = LED_OFF;

void toggleLED() {
  ledState = ledState == LED_ON ? LED_OFF : LED_ON;
  digitalWrite(LED_BUILTIN, ledState);
  Serial.println("tick");
}

char nodeAddress[] = "ESP32-07";
char nodeRouteData[] = " ESP32-07  ";

// Replace with your network credentials (STATION)
#define ssid "moonraker"
#define password "nickisadick"

WiFiClient client; // TCP client
AsyncUDP udp;
const short udpBroadcastListenPort = 8082;

#define udpBufferSize 255
char udpBuffer[udpBufferSize]; //buffer to hold incoming packet


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
    // finishing composing router packetdata
    nodeRouteData[0] = 10; // size of address
    nodeRouteData[11] = 0; // first byte of cost
    nodeRouteData[12] = 1; // second byte of cost

    p->clear();
    p->net = NET_ROUTE;
    p->src = (uint8*)nodeAddress;
    p->srcSz = 10;
    p->data = (uint8*)nodeRouteData;  
    p->dataSz = 13;

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

  // init wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println(WiFi.localIP());
  Udp.listen(udpBroadcastListenPort);
  delay(250);
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
    IPAddress remoteIp = urdp.remoteIP();

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

    char* protocol = strtok(udpBuffer, ":");
    char* host = strtok(NULL, ":/");
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
        Serial.println("timer elapsed");
        char srv[] = "log";
        char data[16];
        int dataSz = sprintf(data, "%0.3fF", readSensor());
        Packet p;
        p.clear();
        p.srv = (uint8*)srv;
        p.srvSz = 3;
        p.src = (uint8*)nodeAddress;
        p.srcSz = 10;
        p.data = (uint8*)data;
        p.dataSz = dataSz;
        sendPacket(&p);
        markTime();
      } else if (elapsed() < 0) {
        Serial.println("timer rollover detected, reseting timer");
        markTime();
      }
  } else if (readUdp()) {
    digitalWrite(LED_PIN, HIGH);
    markTime();
  }
  if (elapsed() > 1000) {
    digitalWrite(LED_PIN, LOW);
  }
}
