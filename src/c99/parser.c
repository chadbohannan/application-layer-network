/*********************************************************************************************************************
 *					   Space Science & Engineering Lab - MSU
 *					     Maia University Nanosat Program
 *
 *							IMPLEMENTATION
 * Filename	  : link.c
 * Created	  : 15, Nov 2006
 * Description:
 **********************************************************************************************************************/
#include "parser.h"

int initParser(Parser* parser, void (*packet_callback)(Packet*))
{
  parser->state = STATE_FINDSTART;
  parser->delimCount = 0;
  parser->controlFlags = 0;
  parser->headerIndex = 0;
  parser->headerLength = 0;
  parser->dataIndex = 0;
  parser->dataLength = 0;
  parser->crcIndex = 0;
  parser->crcLength = 0;
  parser->packet_callback = packet_callback;
  return 0;
}

int acceptPacket(Parser* parser)
{
  Packet packet; // declare memory on the stack
  initPacket(&packet);

  // populate a Packet struct from the raw data
  readPacketFromBuffer(&packet, parser->packetBuffer);

  // deliver to application code
  if (parser->packet_callback) {
    parser->packet_callback(&packet);
  } else {
    return -3; // TODO enumerate errors
  }
  parser->state = STATE_FINDSTART;
  parser->headerIndex = 0;
  parser->delimCount = 0;

  return 0;
}

// consumes a byte stream and generates calls to packet_callback
int parseBytes(Parser* parser, INT08U* buffer, int numBytes)
{
  INT08U msg;
  // consume input one byte at a time
  for (int i = 0; i < numBytes; i++)
  {
    msg = buffer[i]; // read next byte in sequence

    // check for escape char (occurs mid-frame)
    if((parser->delimCount == (FRAME_LEADER_LENGTH-1)) && (msg == FRAME_ESCAPE))
    { // reset FRAME_LEADER detection
      parser->delimCount = 0;
      continue;
    }
    else if(msg == FRAME_LEADER)
    {
      parser->delimCount++;
      if(parser->delimCount == FRAME_LEADER_LENGTH)
      {
        for (int i = 0; i < FRAME_LEADER_LENGTH; i++)
          parser->packetBuffer[i] = FRAME_LEADER;
        parser->headerIndex = FRAME_LEADER_LENGTH;
        parser->state = STATE_GET_CF;
        continue;
      }
    }
    else /* reset delim count */
      parser->delimCount = 0;

    /* use current char in following state */
    switch(parser->state)
    {
    case STATE_FINDSTART:
        /* Do Nothing, dump char */
      break; /* end STATE_FINDSTART */

    case STATE_GET_CF:
      if(parser->headerIndex>=MAX_HEADER_SIZE)
      {   // TODO error reporting
        parser->state = STATE_FINDSTART;
        break;
      }
      parser->packetBuffer[parser->headerIndex] = msg;
      parser->headerIndex++;
      if(parser->headerIndex==6)
      { /* Ham CF */
        INT16U cf = readINT16U(&parser->packetBuffer[FRAME_LEADER_LENGTH]);
        parser->controlFlags = CFHamDecode(cf);
        writeINT16U(&parser->packetBuffer[FRAME_LEADER_LENGTH], parser->controlFlags);
        parser->headerLength = headerLength(parser->controlFlags);
        parser->state = STATE_GETHEADER;
      }
      break; /* end STATE_GET_CF */

    case STATE_GETHEADER:
      if(parser->headerIndex>=MAX_HEADER_SIZE)
      { // TODO error reproting
        parser->state = STATE_FINDSTART;
        break;
      }
      parser->packetBuffer[parser->headerIndex] = msg;
      parser->headerIndex++;
      if(parser->headerIndex>=parser->headerLength)
      {
        if (parser->controlFlags & CF_DATA)
        {
          parser->dataIndex = 0;
          int dataOffset = headerFieldOffset(parser->controlFlags, CF_DATA);
          parser->dataLength = readINT16U(&parser->packetBuffer[dataOffset]);
          parser->data = parser->packetBuffer + parser->headerLength; // set the data pointer to an offset in packetBuffer
          parser->state = STATE_GETDATA;
        }
        else if (parser->controlFlags & CF_CRC)
        {
          parser->crcIndex = 0;
          parser->dataLength = 0;
          parser->crcLength = CRC_FIELD_SIZE;
          parser->crcSum = parser->data + parser->dataLength;
          parser->state = STATE_GETCRC;
        }
        else
        {
          acceptPacket(parser);
        }
      }
      break; /* end STATE_GETHEADER */

    case STATE_GETDATA:
      parser->data[parser->dataIndex] = msg;
      parser->dataIndex++;

      if(parser->dataIndex >= parser->dataLength)
      {
        if (parser->controlFlags & CF_CRC)
        {
          parser->crcIndex = 0;
          parser->crcLength = CRC_FIELD_SIZE;
          parser->crcSum = parser->data + parser->dataLength;
          parser->state = STATE_GETCRC;
        }
        else {
          acceptPacket(parser);
        }
      }
      break; /* end STATE_GETDATA */

    case STATE_GETCRC:
      parser->crcSum[parser->crcIndex] = msg;
      parser->crcIndex++;
      if(parser->crcIndex >= parser->crcLength)
      {
        int checkSumInputLength = parser->headerLength+parser->dataLength;
        INT32U expectedCRC = readINT32U(parser->crcSum);
        INT32U computedCRC = getCRC(parser->packetBuffer, checkSumInputLength);
        if ( expectedCRC != computedCRC)
        { // TODO error reporting
          parser->state = STATE_FINDSTART;
        } else {
          acceptPacket(parser);
        }
      }
      break; /* end STATE_GETCRC */
    } /* end switch(state) */
  }/* end while(forever) */
  return numBytes;
}
