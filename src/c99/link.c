/*********************************************************************************************************************
 *					   Space Science & Engineering Lab - MSU
 *					     Maia University Nanosat Program
 *
 *							IMPLEMENTATION
 * Filename	 : link.c 
 * Programmer(s) : Chad Bohannan & Grant Nelson
 * Created	 : 15, Nov 2006
 * Description	 : The Link Module is responsible for managing comunications with the GroundStation.
 **********************************************************************************************************************/
#define  LINK_GLOBALS

#include <includes.h>

/**********************************************************************************************************************
 *                  DATA TYPES
 **********************************************************************************************************************/

/*
 *+---------------------------------------
 *| Package Description
 *+---------------------------------------
 *| Header,33:
 *|   [StartDelim,4][CF,2]{TimeStamp,4}{ASCIICallSign,8}{ScrAddr,2}
 *|   {DestAddr,2}{SeqNum,2}{AckBlock,4}[Err,1]{DataLength,2}[CRC,ErrLength]
 *| Body,n+2:
 *|   [Data,n][CRC,ErrLength]
 *| CF (Control Flags):
 *|   0x0001 DataLength
 *|   0x0002 ErrorDetection
 *|   0x0004 AckBlock
 *|   0x0008 SeqNum
 *|   0x0010 DestAddr
 *|   0x0020 ScrAddr
 *|   0x0040 ASCIICallSign
 *|   0x0080 TimeStamp
 *|   0x0100 LinkState
 *| Error Detection Algorithm:
 *|   0x03 No CRC            (0 bytes)
 *|   0x1C CRC16             (2 bytes)
 *|   0xE0 CRC-CCITT(0xFFFF) (2 bytes)
 *|   0xFF CRC32             (4 bytes)
 *+---------------------------------------
 */
  
/**********************************************************************************************************************
 *                  CONSTANTS
 **********************************************************************************************************************/

#define MAX_UPLINKBUFFER_SIZE 255

/*Packet Header control flag bit masks*/
#define CF_DATALENGTH 0x0001
#define CF_ERROR      0x0002
#define CF_ACKBLOCK   0x0004
#define CF_SEQNUM     0x0008
#define CF_DESTADDR   0x0010
#define CF_SCRADDR    0x0020
#define CF_ASCIICALL  0x0040
#define CF_TIMESTAMP  0x0080

/*Packet parsing state definitions*/
#define STATE_FINDSTART  0
#define STATE_GET_CF     1
#define STATE_GETHEADER  2
#define STATE_GETCRC_HDR 3
#define STATE_GETDATA    5
#define STATE_GETCRC_DAT 6


#define MAX_POST_ATTEMPS   16 /* Number of attemps to post the message to the CP */
#define MAX_MEMGET_ATTEMPS 16 /* Number of attemps to get memory from the OS */

#define MAX_HEADER_SIZE          37
#define MAX_DIRECT_BUFFER_SIZE   MESSAGE_BUFFER_SIZE-13
#define MAX_INDIRECT_BUFFER_SIZE EXTENDED_BUFFER_SIZE

 /**********************************************************************************************************************
 *                  LOCAL VARIABLES
 **********************************************************************************************************************/
INT16U BufferFilledSize=0;
INT08U BufferOffset=0;
INT08U UpLinkBuffer[MAX_UPLINKBUFFER_SIZE];

 /**********************************************************************************************************************
 *                  LOCAL PROTOTYPES
 **********************************************************************************************************************/
INT08U PendOnSerial();
INT08U bitCounter(INT32U n);

  
/**********************************************************************************************************************
 *                  GLOBAL FUNCTIONS
 **********************************************************************************************************************/
INT8U LinkInit(void)
{
    DownLinkQueue = OSQCreate( (void**)&DownLinkMsgArray, DOWNLINK_MAX_MSGS );
    /* UpLinkQueue   = OSQCreate( (void**)&UpLinkMsgArray,   UPLINK_MAX_MSGS ); */
    if(DownLinkQueue)/* && UpLinkQueue) */
        return LINK_NO_ERR;
    else
        return LINK_ERR;
}

void UpLink(void *pdata) {
#if OS_CRITICAL_METHOD == 3                               
    OS_CPU_SR  cpu_sr;
#endif
    
    INT08U counter;
	INT08U msg;
	INT16U CFmsg;
    INT08U Errmsg;
    INT16U CRCmsg;
    
    INT08U state;
    INT08U delimCount;
    
    INT08U headerIndex;
    INT08U headerLength;
    INT08U header[MAX_HEADER_SIZE];
    
    INT16U dataIndex;
	INT16U dataLength;
    CP_MSG *CPmsg;
    INT08U *ExtBuf;
	INT08U *Data; /* either points to the data field in CPmsg or ExtBuf */
    
    INT08U error = 0;
	
    pdata = pdata;
    
    GSEDirectMessage("(UPLINK) Link Module Running.\r\n", 31);
    
    state = STATE_FINDSTART;
    headerIndex = 0;
    CPmsg = NULL;
    ExtBuf = NULL;
    delimCount = 0;
    
    /* Start task loop */
    while(1)
    {
        /* get next byte from Serial */
        msg = PendOnSerial();
        
        //printf("printf(\"%s\"<%d>)-- MSG=%04X\r\n",__FILE__,__LINE__,msg);
        
        /* check for escape char */
        if((delimCount==3)&&(msg==0xC3))
        {   /* reset delim count and don't send on char */
            delimCount = 0;
            continue;
        }
        /* check for start delim */
        else if(msg=='<')
        {
            delimCount++;
            if(delimCount==4)
            {   /* reset system, start delim found */
                Data=NULL;
                header[0]='<';
                header[1]='<';
                header[2]='<';
                header[3]='<';
                headerIndex=4;
                state=STATE_GET_CF;
                GSEDirectMessage("(UPLINK) Header Found.\r\n", 24);
                continue;
            }
        }
        else /* reset delim count */
            delimCount = 0;
        
        
        /* get body buffer */
        if(CPmsg==NULL)
        {            
            CPmsg=OSMemGet(queueBuffer, &error);
            /* error getting memory, continue */
            if(error) CPmsg=NULL;
        }

        /* use current char in following state */
        switch(state)
        {
            case STATE_FINDSTART:
                /* Do Nothing, dump char */
                break; /* end STATE_FINDSTART */
                
            case STATE_GET_CF:
                if(headerIndex>=MAX_HEADER_SIZE)
                {   /* Error, write out of bounds, reset */
                    state = STATE_FINDSTART;
                    GSEDirectMessage("(UPLINK) Error: Out of bounds.\r\n", 32);
                    break;
                }
                header[headerIndex] = msg;
                headerIndex++;
                if(headerIndex==6)
                { /* Ham CF */
                    CFmsg = CFHamDecode(*((INT16U*)&header[4]));
                    (*((INT16U*)&header[4])) = CFmsg;
                    headerLength = 6;
					if(CFmsg&0x0100) headerLength+=1; /* LinkState  (1 byte) */
                    if(CFmsg&0x0080) headerLength+=4; /* TimeStamp  (4 bytes)*/
                    if(CFmsg&0x0040) headerLength+=8; /* Call Sign  (8 bytes)*/
                    if(CFmsg&0x0020) headerLength+=2; /* ScrAddr    (2 bytes)*/
                    if(CFmsg&0x0010) headerLength+=2; /* DestAddr   (2 bytes)*/
                    if(CFmsg&0x0008) headerLength+=2; /* SeqNum     (2 bytes)*/
                    if(CFmsg&0x0004) headerLength+=4; /* AckBlock   (4 bytes)*/
                    if(CFmsg&0x0002) headerLength+=1; /* Err        (1 byte) */
                    if(CFmsg&0x0001) headerLength+=2; /* DataLength (2 bytes)*/
                    state = STATE_GETHEADER;
                }
                break; /* end STATE_GET_CF */
                
            case STATE_GETHEADER:
                if(headerIndex>=MAX_HEADER_SIZE)
                {   /* Error, write out of bounds, reset */
                    state = STATE_FINDSTART;
                    GSEDirectMessage("(UPLINK) Error: Out of bounds.\r\n", 32);
                    break;
                }
                header[headerIndex] = msg;
                headerIndex++;
                if(headerIndex>=headerLength)
                {   /* read error type */
                    if(!(CFmsg&0x02)) Errmsg = NO_CRC;
                    else if(CFmsg&0x01) Errmsg = header[headerLength-3];
                    else Errmsg = header[headerLength-1];
                    /* Error check Errmsg */                    
                    if(bitCounter(Errmsg^NO_CRC)<=2)         Errmsg = NO_CRC;
                    else if(bitCounter(Errmsg^CRC16)<=2)     Errmsg = CRC16;
                    else if(bitCounter(Errmsg^CRC_CCITT)<=2) Errmsg = CRC_CCITT ;
                    else if(bitCounter(Errmsg^CRC32)<=2)     Errmsg = CRC32;
                    else /* Bad error code, reset */
                    {
                        state = STATE_FINDSTART;
                        GSEDirectMessage("(UPLINK) Error: Bad error code.\r\n", 33);
                        break;
                    }
                    if(Errmsg&(CRC16|CRC_CCITT)) headerLength+=2;
                    else if(Errmsg&CRC32) headerLength+=4;                    
                    state = STATE_GETCRC_HDR;
                }
                break; /* end STATE_GETHEADER */
                
            case STATE_GETCRC_HDR:
                if(headerIndex>=MAX_HEADER_SIZE)
                {   /* Error, write out of bounds */
                    state = STATE_FINDSTART;
                    GSEDirectMessage("(UPLINK) Error: Out of bounds.\r\n", 32);
                    break;
                }
                header[headerIndex] = msg;
                headerIndex++;
                if(headerIndex>=headerLength)
                {
                    if(!(CFmsg&0x01))
                    {   /* blank header with no body, reset */
                        state = STATE_FINDSTART;
                        GSEDirectMessage("(UPLINK) Header with No Body.\r\n", 30);
                        break;
                    }
                    if(Errmsg==NO_CRC)
                    {
                        dataLength=*((INT16U*)&header[headerIndex-2]);
                        dataIndex=0;
                        state=STATE_GETDATA;
                    }
                    else if((Errmsg==CRC16)||(Errmsg==CRC_CCITT))
                    {
                        CRCmsg=getCRC(Errmsg, &header[0], headerIndex-2);
                        if(CRCmsg==(*((INT16U*)&header[headerIndex-2])))
                        {
                            dataLength=*((INT16U*)&header[headerIndex-4]);
                            dataIndex=0;
                            state=STATE_GETDATA;
                        }
                        else
                        { /* bad CRC, reset */
                            state = STATE_FINDSTART;
                            GSEDirectMessage("(UPLINK) Error: bad crc (1).\r\n", 30);
                            break;
                        }
                    }
                    else if(Errmsg==CRC32)
                    {
                        CRCmsg=getCRC(Errmsg, &header[0], headerIndex-4);
                        if(CRCmsg==(*((INT32U*)&header[headerIndex-4])))
                        {
                            dataLength=*((INT16U*)&header[headerIndex-6]);
                            dataIndex=0;
                            state=STATE_GETDATA;
                        }
                        else
                        { /* bad CRC, reset */
                            state = STATE_FINDSTART;
                            GSEDirectMessage("(UPLINK) Error: bad crc (2).\r\n", 30);
                            break;
                        }
                    }
                    
                    /* full valid header gotten, deal with parts here */
                    
                    /* setting up Package for command processor */
                    if(dataLength<=MAX_DIRECT_BUFFER_SIZE)
                    {   /* direct data */
                        /* check for null memory */
                        counter = 0;
                        while(CPmsg==NULL)
                        {
                            CPmsg=OSMemGet(queueBuffer, &error);
                            if(error)
                            {   /* error getting memory */
                                CPmsg=NULL;
                                counter++;
                                if(counter>=MAX_MEMGET_ATTEMPS)
                                {   /* attemps failed, reset */
                                    GSEDirectMessage("(UPLINK) Error: getting mem (1).\r\n", 34);
                                    state=STATE_FINDSTART;
                                    break;
                                }
                                OSTimeDly(1); /* wait a cycle and try again. */
                            }
                        }
                        if(CPmsg==NULL) break;
                        /* memory gotten, ready to be filled */
                        CPmsg->extData=NULL;
                        Data=CPmsg->data;
                        CPmsg->msg_command=CP_PARSE_UPLINK;
                        CPmsg->isExt = FALSE;
                        CPmsg->msg_size=dataLength;
                        GSEDirectMessage("(UPLINK) Header complete.\r\n", 27);
                        break;
                    }
                    else if(dataLength<=MAX_INDIRECT_BUFFER_SIZE)
                    {   /* indirect data */
                        /* check for null memory */
                        counter = 0;
                        while(CPmsg==NULL)
                        {
                            CPmsg=OSMemGet(queueBuffer, &error);
                            if(error)
                            {   /* error getting memory */
                                CPmsg=NULL;
                                counter++;
                                if(counter>=MAX_MEMGET_ATTEMPS)
                                {   /* attemps failed, reset */
                                    GSEDirectMessage("(UPLINK) Error: getting mem (2).\r\n", 34);
                                    state=STATE_FINDSTART;
                                    break;
                                }
                                OSTimeDly(1); /* wait a cycle and try again. */
                            }
                        }
                        if(CPmsg==NULL) break;
                        counter = 0;
                        while(ExtBuf==NULL)
                        {
                            ExtBuf=OSMemGet(queueBuffer, &error);
                            if(error)
                            {   /* error getting memory */
                                ExtBuf=NULL;
                                counter++;
                                if(counter>=MAX_MEMGET_ATTEMPS)
                                {   /* attemps failed, reset */
                                    GSEDirectMessage("(UPLINK) Error: getting mem (3).\r\n", 34);
                                    state=STATE_FINDSTART;
                                    break;
                                }
                                OSTimeDly(1); /* wait a cycle and try again. */
                            }
                        }
                        if(ExtBuf==NULL) break;
                        CPmsg->extData=ExtBuf;
                        Data=ExtBuf;
                        CPmsg->msg_command=CP_PARSE_UPLINK;
                        CPmsg->isExt = TRUE;
                        CPmsg->msg_size=dataLength;
                        GSEDirectMessage("(UPLINK) Header complete.\r\n", 27);
                        break;
                    }
                    else
                    {   /* data length too large */
                        GSEDirectMessage("(UPLINK) dataLength too large.\r\n", 32);
                        state=STATE_FINDSTART;
                        break;
                    }
                }
                break; /* end STATE_GETCRC_HDR */
                
            case STATE_GETDATA:
                Data[dataIndex] = msg;
                dataIndex++;
                if(dataIndex>=dataLength)
                {   /* Data filled, CRC the data */                
                    if(Errmsg&(CRC16|CRC_CCITT)) dataLength+=2;
                    else if(Errmsg&CRC32) dataLength+=4;                    
                    state = STATE_GETCRC_DAT;
                }                
                break; /* end STATE_GETDATA */
                
            case STATE_GETCRC_DAT:
                Data[dataIndex] = msg;
                dataIndex++;
                if(dataIndex>=dataLength)
                {
                    if(Errmsg==NO_CRC)
                    {
                        /* Do Nothing */
                    }
                    else if((Errmsg==CRC16)||(Errmsg==CRC_CCITT))
                    {
                        CRCmsg=getCRC(Errmsg, Data, dataIndex-2);                        
                        if(CRCmsg!=(*((INT16U*)&Data[dataIndex-2])))
                        { /* bad CRC, reset */
                            state = STATE_FINDSTART;
                            GSEDirectMessage("(UPLINK) Error: 16bit data crc.\r\n", 33);
                            break;
                        }
                    }
                    else if(Errmsg==CRC32)
                    {
                        CRCmsg=getCRC(Errmsg, Data, dataIndex-4);
                        if(CRCmsg!=(*((INT32U*)&Data[dataIndex-4])))
                        { /* bad CRC, reset */
                            state = STATE_FINDSTART;
                            GSEDirectMessage("(UPLINK) Error: 32bit data crc.\r\n", 33);
                            break;
                        }
                    }
                    /* CRC passed Post message */
                    GSEDirectMessage("(UPLINK) Posting message to CP.\r\n", 33);
                    /* attempt to post the message */
                    counter=0;
                    while(1)
                    {
                        error = OSQPost(CPQueue, CPmsg);
                        if(error)
                        {   /* Error posting data */
                            counter++;
                            if(counter>=MAX_MEMGET_ATTEMPS)
                            {   /* attemps failed, reset */
                                GSEDirectMessage("(UPLINK) Failed to posted msg.\r\n", 32);
                                state=STATE_FINDSTART;
                                break;
                            }
                            OSTimeDly(1); /* wait a cycle and try again. */
                        }
                        else
                        {   /* Message sent */
                            /* Must be set to null so that the memory is not recaptured */
                            if(CPmsg!=NULL)
                            {
                                if(CPmsg->extData!=NULL) ExtBuf=NULL;
                                CPmsg=NULL;
                            }
                            Data=NULL;
                            state=STATE_FINDSTART;
                            break;
                        }
                    }
                }
                break; /* end STATE_GETCRC_DAT */
        } /* end switch(state) */
    }/* end while(forever) */
    OSTaskDel(OS_PRIO_SELF);  
}

void DownLink(void *pdata) {
#if OS_CRITICAL_METHOD == 3                               
    OS_CPU_SR  cpu_sr;
#endif
    INT08U header[15];    
    CP_MSG *z;
    INT08U *msg_data;
    INT08U error = 0;
    INT16U dataCRC;
    INT08U i, j, k;
    
    INT08U escape = 0xC3;
    
    pdata = pdata;
    
    GSEDirectMessage("(DOWNLINK) Link Module Running.\r\n", 33);
    while(1)
    {
        z = (CP_MSG*)OSQPend(DownLinkQueue, 0, &error);
        /*branch on msg_type or command*/
        switch(z->msg_type)
        {
            case LINK_NO_MSG:
                break;
    
            case LINK_SEND_MSG:
                /* send Header */
                (*((INT32U*)&header[0]))  = 0x3C3C3C3C;
                (*((INT16U*)&header[4]))  = CFHamEncode(CF_TIMESTAMP|CF_ERROR|CF_DATALENGTH);
                (*((INT32U*)&header[6]))  = OSTimeGet();
                (*((INT08U*)&header[10])) = CRC16;
                (*((INT16U*)&header[11])) = z->msg_size;
                (*((INT16U*)&header[13])) = (INT16U)getCRC(CRC16, &header[0], sizeof(header)-2);
                sciSend(SERIAL0, &header[0], sizeof(header));
                /* send Body and stuff it with the escape char. */
                
                /*select the buffer to grab data from*/
                if(z->isExt == TRUE)
                  msg_data = z->extData;
                else
                  msg_data = z->data;
                
                for(i=j=k=0; i<z->msg_size; i++)
                {
                    if(msg_data[i]=='<')
                    {
                        j++;
                        if(j==3)
                        {
                            sciSend(SERIAL0, &(msg_data[k]), i-k+1);
                            sciSend(SERIAL0, &escape, 1);
                            k=i+1;
                            j=0;
                        }
                    }
                    else j=0;
                }
                if(k<(z->msg_size-1))
                    sciSend(SERIAL0, &(msg_data[k]), i-k);
                /* send Body CRC */
                dataCRC = (INT16U)getCRC(CRC16, msg_data, z->msg_size);
                sciSend(SERIAL0, (INT08U*)&dataCRC, 2);
                
                GSEDirectMessage("(GSE DOWNLINK): ", 16);
                GSEDirectMessage(msg_data, z->msg_size);
                GSEDirectMessage("\r\n", 2);
                break;
          
            default:/*error*/
                break;
        }/*end switch()*/
		postJobComplete(z->msg_jobID, error);
        if(z->isExt == TRUE)
            OSMemPut(extendedBuffer, (void*)z->extData);      /*restore memory to global buffer*/
        OSMemPut(queueBuffer, (void*)z);      /*restore memory to global buffer*/
      
        OSTimeDly(1);
    }/*end while(forever)*/
    OSTaskDel(OS_PRIO_SELF);  
}

INT32U getCRC(INT08U format, INT08U* pdata, INT16U size)
{
    INT08U c;
    INT16U i, j;
    INT32U bit, crc;
    if(format==CRC_CCITT)
    {
        crc=0xFFFF;
        for(i=0; i<size; i++)
        {
            c=pdata[i];
            for(j=0x80; j>0; j>>=1)
            {
                bit=crc&0x8000;
                crc<<=1;
                if((c&j)>0) bit^=0x8000;
                if(bit>0) crc^=0x1021;
            }
        }
        return crc&0xFFFF;
    }
    else if(format==CRC16)
    {
        crc=0x0000;        
        for(i=0; i<size; i++)
        {
            c=pdata[i];                        
            for(j=0x01; j<=0x80; j<<=1)
            {
                bit=crc&0x8000;
                crc<<=1;
                if((c&j)>0) bit^=0x8000;
                if(bit>0) crc^=0x8005;
            }
        }
        /* reverse */
        crc=(((crc&0x5555)<<1)|((crc&0xAAAA)>>1));
        crc=(((crc&0x3333)<<2)|((crc&0xCCCC)>>2));
        crc=(((crc&0x0F0F)<<4)|((crc&0xF0F0)>>4));
        crc=(((crc&0x00FF)<<8)|((crc&0xFF00)>>8));
        return crc&0xFFFF;
    }
    else if(format==CRC32)
    {
        crc=0xFFFFFFFF;
        for(i=0; i<size; i++)
        {
            c=pdata[i];
            for(j=0x01; j<=0x80; j<<=1)
            {
                bit=crc&0x80000000;
                crc<<=1;
                if((c&j)>0) bit^=0x80000000;
                if(bit>0) crc^=0x4C11DB7;
            }
        }
        /* reverse */
        crc=(((crc&0x55555555)<< 1)|((crc&0xAAAAAAAA)>> 1));
        crc=(((crc&0x33333333)<< 2)|((crc&0xCCCCCCCC)>> 2));
        crc=(((crc&0x0F0F0F0F)<< 4)|((crc&0xF0F0F0F0)>> 4));
        crc=(((crc&0x00FF00FF)<< 8)|((crc&0xFF00FF00)>> 8));
        crc=(((crc&0x0000FFFF)<<16)|((crc&0xFFFF0000)>>16));
        return (crc^0xFFFFFFFF)&0xFFFFFFFF;
    }
    return 0x0000;
}

INT16U CFHamEncode(INT16U value)
{
    /* perform G matrix */
    return (value & 0x07FF)
        | (IntXOR(value & 0x071D) << 12)
        | (IntXOR(value & 0x04DB) << 13)
        | (IntXOR(value & 0x01B7) << 14)
        | (IntXOR(value & 0x026F) << 15);
}

INT16U CFHamDecode(INT16U value)
{
    /* perform H matrix */
    INT08U err = IntXOR(value & 0x826F)
              | (IntXOR(value & 0x41B7) << 1)
              | (IntXOR(value & 0x24DB) << 2)
              | (IntXOR(value & 0x171D) << 3);
    /* don't strip control flags, it will mess up the crc */
    switch(err) /* decode error feild */
    {
        case 0x0F: return value ^ 0x0001;
        case 0x07: return value ^ 0x0002;
        case 0x0B: return value ^ 0x0004;
        case 0x0D: return value ^ 0x0008;
        case 0x0E: return value ^ 0x0010;
        case 0x03: return value ^ 0x0020;
        case 0x05: return value ^ 0x0040;
        case 0x06: return value ^ 0x0080;
        case 0x0A: return value ^ 0x0100;
        case 0x09: return value ^ 0x0200;
        case 0x0C: return value ^ 0x0400;
        default: return value;
    }
}


/**********************************************************************************************************************
 *                  LOCAL FUNCTIONS
 **********************************************************************************************************************/

INT08U PendOnSerial()
{
    INT08S err;
    BufferOffset++;
    if(BufferOffset>=BufferFilledSize)
    { /* Buffer Empty, Fill Buffer */
        while(1)
        {
            /* Fill Buffer from SCI0.*/
            err=sciReceive(SERIAL0, &UpLinkBuffer[0], &BufferFilledSize, MAX_UPLINKBUFFER_SIZE);
            if(err)
            {
                #ifdef   GSE_ENABLED
                GSEDirectMessage("(UPLINK) Read Error in SCI0.\r\n", 30);
                #endif
                continue;
            }
            if(BufferFilledSize!=0) break; /* Buffer Filled */
            OSTimeDly(1); /* Buffer Not Filled, Pend until Buffer is Filled */
        }
        /* Reset Buffer Offset to Start of Buffer */
        BufferOffset=0;
    }
    return UpLinkBuffer[BufferOffset];
}

/*
 * Sparse Ones Bit Counter.
 */
INT08U bitCounter(INT32U n)
{
    INT08U cnt=0x0;
    while(n)
    {   /* This loop will only execute the number times equal to the number of ones. */
        cnt++;
        n&=(n-0x1);     
    }
    return cnt;
}

/*
 * This is a modified "Sparse Ones Bit Count", because there are 7 or less ones out of 16 bits it is best to look at the ones.
 * Instead of counting the ones it just determines if there was an odd or even count of bits by XORing the int.
 * Returns 0x0 or 0x1.
 */
#if 0
INT08U IntXOR(INT16U n)
{
    INT08U cnt=0x0;
    while(n)
    {   /* This loop will only execute the number times equal to the number of ones. */
        cnt^=0x1;
        n&=(n-0x1);     
    }
    return cnt;
}
#endif
