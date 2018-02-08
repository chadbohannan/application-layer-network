/* ELP Packet serialization test/demonstration.
 * Use make to build the elp-serialization executable.
 * This example reads an input file, parses each packet then writes
 * each packet back to an ouput file. The output file should match the
 * input file.
 */

#include "stdio.h"
#include "strings.h"
#include "../../src/c99/link.h"

const char testText1[] = "Bytestuffing is used only when '<<<<' occurs mid-frame.";
const char testText2[] = "The CRC is calculated over both the header and the data.";
const char testText3[] = "Address data only appears in the header when it's non-zero.";
int testText1Len = strlen(testText1);
int testText2Len = strlen(testText2);
int testText3Len = strlen(testText3);

void packet_callback(Packet* packet) {
  printf("read packet with %d data bytes", packet->dataSize);
}

 int main() {
   FILE *f = fopen("packets.input", "a");

   Packet packet;
   memcpy(packet.data, testText1, testText1Len);
   packet.dataSize = testText1Len;

   int packetSize;
   INT08U buffer[MAX_PACKET_SIZE];

   int err = writePacketToBuffer(&packet, buffer, &packetSize, MAX_PACKET_SIZE);
   printf("writePacketToBuffer1 packetSize:%d, offset:%d\n", packetSize, err);
   for (int i = 0; i < packetSize; i++)
   {
     fprintf(f, "%c", buffer[i]);
   }

   memcpy(packet.data, testText2, testText2Len);
   packet.dataSize = testText2Len;
   err = writePacketToBuffer(&packet, buffer, &packetSize, MAX_PACKET_SIZE);
   printf("writePacketToBuffer2 packetSize:%d, offset:%d\n", packetSize, err);
   for (int i = 0; i < packetSize; i++)
   {
     fprintf(f, "%c", buffer[i]);
   }

   packet.srcAddr = 1;
   packet.destAddr = 2;
   memcpy(packet.data, testText3, testText3Len);
   packet.dataSize = testText3Len;
   err = writePacketToBuffer(&packet, buffer, &packetSize, MAX_PACKET_SIZE);
   printf("writePacketToBuffer3 packetSize:%d, offset:%d\n", packetSize, err);
   for (int i = 0; i < packetSize; i++)
   {
     fprintf(f, "%c", buffer[i]);
   }

   fclose(f);


   // Parser parser;
   // parser.packet_callback = packet_callback;
   // parseBytes(&parser, buffer, numBytes);
 }
