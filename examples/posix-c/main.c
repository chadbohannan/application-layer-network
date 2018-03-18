/* ELP Packet serialization test/demonstration.
 * Use make to build the elp-serialization executable.
 * This example creates a few packets and appends them to a file.
 * Then parses that file back into packets, then back to a second file.
 * the two files should be identical.
 */

#include "stdio.h"
#include "strings.h"
#include "../../src/c99/parser.h"

const char testText1[] = "Bytestuffing is used only when '<<<<' occurs mid-frame.";
const char testText2[] = "The CRC is calculated over both the header and the data.";
const char testText3[] = "Address data only appears in the header when it's non-zero.";
int testText1Len = strlen(testText1);
int testText2Len = strlen(testText2);
int testText3Len = strlen(testText3);

FILE* outFile; // c doesn't have closures, so we make the output file handle global
void packet_callback(Packet* packet) {
  INT08U buffer[MAX_PACKET_SIZE];
  int packetSize = writePacketToFrameBuffer(packet, buffer, MAX_PACKET_SIZE);
  fwrite(buffer, 1, packetSize, outFile);
  // printf("parsed a packet %d byte packet with %d data bytes\n", packetSize, packet->dataSize);
}

int writeNewPacketsToFile();
int streamPacketsToNewFile();

int main()
{
  writeNewPacketsToFile();
  streamPacketsToNewFile();
}

int writeNewPacketsToFile()
{
   FILE *f = fopen("packetstream.input", "a");

   Packet packet; // define a packet on the stack that will be used several times
   initPacket(&packet); // zero out the packet
   INT08U buffer[MAX_PACKET_SIZE];

   // first packet
   memcpy(packet.data, testText1, testText1Len);
   packet.dataSize = testText1Len;
   int packetSize = writePacketToFrameBuffer(&packet, buffer, MAX_PACKET_SIZE);
   // printf("writePacketToFrameBuffer1 packetSize:%d\n", packetSize);
   fwrite(buffer, packetSize, 1, f);

   // second packet
   memcpy(packet.data, testText2, testText2Len);
   packet.dataSize = testText2Len;
   packetSize = writePacketToFrameBuffer(&packet, buffer, MAX_PACKET_SIZE);
   // printf("writePacketToFrameBuffer2 packetSize:%d\n", packetSize);
   fwrite(buffer, packetSize, 1, f);

   // third pcket
   packet.srcAddr = 1;
   packet.destAddr = 2;
   memcpy(packet.data, testText3, testText3Len);
   packet.dataSize = testText3Len;
   packetSize = writePacketToFrameBuffer(&packet, buffer, MAX_PACKET_SIZE);
   // printf("writePacketToFrameBuffer3 packetSize:%d\n", packetSize);
   fwrite(buffer, packetSize, 1, f);

   // finished phase 1
   fclose(f);
   return 0;
}

// open the input file, parse packets, rewrite them to a second file
int streamPacketsToNewFile()
{
   Parser parser;
   initParser(&parser, packet_callback);

   FILE* inFile = fopen("packetstream.input", "r");
   outFile = fopen("packetstream.output", "w");
   if (inFile && outFile) {
     int chunkSize = 5;// read input 5 bytes at a time
     INT08U buff[chunkSize];
     int size = fread(buff, 1, chunkSize, inFile);
     while (size)
     {
       parseBytes(&parser, buff, size);
       size = fread(buff, 1, chunkSize, inFile);
     }
   }
   fclose(inFile);
   fclose(outFile);

   if (parser.state != STATE_FINDSTART) {
     printf("Parser in ended in a bad state:%d\n", parser.state);
   }
   else
   {
     printf("completed successfully\n");
   }
   return 0;
 }
