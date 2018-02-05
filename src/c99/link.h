/** ********************************************************************************************************************
 *					   Space Science & Engineering Lab - MSU
 *					     Maia University Nanosat Program
 *
 *							INTERFACE
 * Filename	: link.h 
 * Created : 8, May 2006
 * Description : Manages connection sessions with groundstations.
 **********************************************************************************************************************/
#ifndef  LINK_H
#define  LINK_H
#ifdef   LINK_GLOBALS
#define  LINK_EXT
#else
#define  LINK_EXT  extern
#endif

/**********************************************************************************************************************
 *                  RETURN CODES
 **********************************************************************************************************************/
#define  HAMERR_NOERR          0 /* no errors */
#define  HAMERR_TOOMANYBITS   -1 /* must be less than 12 bits to encode */
#define  HAMERR_TOOMANYERR    -2 /* more bit flips than could be corrected were found */

#define  LINK_NO_ERR           0
#define  LINK_ERR             -3


 /**********************************************************************************************************************
 *                  MODULE  PARAMETER DEFINES
 **********************************************************************************************************************/ 
#define  UPLINK_MAX_MSGS            16   /* Allocated space for the input msg queue */
#define  UPLINK_TASK_STK_SIZE      256

#define  DOWNLINK_MAX_MSGS          16   /* Allocated space for the input msg queue */
#define  DOWNLINK_TASK_STK_SIZE    256

/* CRC Formats */
#define NO_CRC    0x03
#define CRC16     0x1C
#define CRC_CCITT 0xE0
#define CRC32     0xFF

/**********************************************************************************************************************
 *                  MODULE MESSAGE DEFINES
 **********************************************************************************************************************/ 
#define  LINK_NO_MSG            0
#define  LINK_SEND_MSG          1

/**********************************************************************************************************************
 *                  GLOBALOCAL VARIABLES
 **********************************************************************************************************************/
LINK_EXT OS_STK     UpLinkStk[UPLINK_TASK_STK_SIZE];

LINK_EXT OS_EVENT*  DownLinkQueue;
LINK_EXT void*      DownLinkMsgArray[DOWNLINK_MAX_MSGS]; /*pointer buffer for module msg queue*/
LINK_EXT OS_STK     DownLinkStk[DOWNLINK_TASK_STK_SIZE];

/**********************************************************************************************************************
 *                  PROTOTYPES
 **********************************************************************************************************************/

/* DESCRIPTION : Initializes the modules message queue.
 * ARGUMENTS   : none
 * RETURNS     : error message
 */
INT8U LinkInit(void);
  
/* DESCRIPTION : Recieves messages through a message queue from Command Processor. Messages contain data to downlink.
 * ARGUMENTS   : void *pdata - IN/OUT paramter, not used
 * RETURNS     : void
 */
void DownLink(void *pdata);

/* DESCRIPTION : Sends messages through a message queue to Command Processor. Messages contain data from GroundStation.
 * ARGUMENTS   : void *pdata - IN/OUT paramter, not used
 * RETURNS     : void
 */
void UpLink(void *pdata);

/* DESCRIPTION: 
 * ARGUMENTS: format must be one and only one of these CRC16, CRC_CCITT,CRC32, pdata the buffer, size the buffer size.
 * RETURNS: The CRC in the asked format.
 */
INT32U getCRC(INT08U format, INT08U* pdata, INT16U size);

/*
 * This encodes the CF using a modified Hamming (15,11).
 * Returns the encoded 11 bit CF as a 15 bit codeword.
 * Only the 0x07FF bits are  aloud to be on for the input all others will be ignored.
 * Based off of the following Hamming (15,11) matrix...
 * G[16,11] = [[1000,0000,0000,1111],  0x800F
 *             [0100,0000,0000,0111],  0x4007
 *             [0010,0000,0000,1011],  0x200B
 *             [0001,0000,0000,1101],  0x100D
 *             [0000,1000,0000,1110],  0x080E
 *             [0000,0100,0000,0011],  0x0403
 *             [0000,0010,0000,0101],  0x0205
 *             [0000,0001,0000,0110],  0x0106
 *             [0000,0000,1000,1010],  0x008A
 *             [0000,0000,0100,1001],  0x0049
 *             [0000,0000,0010,1100]]; 0x002C
 */
INT16U CFHamEncode(INT16U value);

/*
 * This decodes the CF using a modified Hamming (15,11).
 * It will fix one error, if only one error occures, not very good for burst errors.
 * This is a SEC (single error correction) which means it has no BED (bit error detection) to save on size.
 * The returned value will be, if fixed properly, the same 11 bits that were sent into the encoder.
 * Bits 0xF800 will be zero.
 * Based off of the following Hamming (15,11) matrix...
 * H[16,4]  = [[1011,1000,1110,1000],  0x171D
 *             [1101,1011,0010,0100],  0x24DB
 *             [1110,1101,1000,0010],  0x41B7
 *             [1111,0110,0100,0001]]; 0x826F
 */
INT16U CFHamDecode(INT16U value);

#endif
