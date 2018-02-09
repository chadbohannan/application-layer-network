/* ELP Packet serialization test/demonstration.
 * Use make to build the elp-serialization executable.
 * This example creates a few packets and appends them to a file.
 * Then parses that file back into packets, then back to a second file.
 * the two files should be identical.
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

FILE* f2; // c doesn't have closures, so we make the output file handle global
void packet_callback(Packet* packet) {
  INT08U buffer[MAX_PACKET_SIZE];
  int packetSize = writePacketToBuffer(packet, buffer, MAX_PACKET_SIZE);
  fwrite(buffer, 1, packetSize, f2);

  printf("parsed a packet %d byte packet with %d data bytes\n", packetSize, packet->dataSize);
}

 int main()
 {
   FILE *f1 = fopen("packets.input", "a");

   Packet packet;
   initPacket(&packet);
   INT08U buffer[MAX_PACKET_SIZE];

   // first packet
   memcpy(packet.data, testText1, testText1Len);
   packet.dataSize = testText1Len;
   int packetSize = writePacketToBuffer(&packet, buffer, MAX_PACKET_SIZE);
   printf("writePacketToBuffer1 packetSize:%d\n", packetSize);
   fwrite(buffer, packetSize, 1, f1);

   // second packet
   memcpy(packet.data, testText2, testText2Len);
   packet.dataSize = testText2Len;
   packetSize = writePacketToBuffer(&packet, buffer, MAX_PACKET_SIZE);
   printf("writePacketToBuffer2 packetSize:%d\n", packetSize);
   fwrite(buffer, packetSize, 1, f1);

   // third pcket
   packet.srcAddr = 1;
   packet.destAddr = 2;
   memcpy(packet.data, testText3, testText3Len);
   packet.dataSize = testText3Len;
   packetSize = writePacketToBuffer(&packet, buffer, MAX_PACKET_SIZE);
   printf("writePacketToBuffer3 packetSize:%d\n", packetSize);
   fwrite(buffer, packetSize, 1, f1);

   // finished phase 1
   fclose(f1);

   // open the input file, parse packets, rewrite them to a second file
   Parser parser;
   initParser(&parser, packet_callback);

   f1 = fopen("packets.input", "r");
   f2 = fopen("packets.output", "w");
   if (f1 && f2) {
     int chunkSize = 5;// read input 5 bytes at a time
     INT08U buff[chunkSize];
     int size = fread(buff, 1, chunkSize, f1);
     while (size)
     {
       parseBytes(&parser, buff, size);
       size = fread(buff, 1, chunkSize, f1);
     }
   }
   fclose(f1);
   fclose(f2);

   if (parser.state != STATE_FINDSTART) {
     printf("Parser in ended in a bad state:%d\n", parser.state);
   }
   else
   {
     printf("completed successfully\n");
   }
 }
