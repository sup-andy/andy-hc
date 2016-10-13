
#ifndef _ZWAVE_SECURITY_AES_H_
#define _ZWAVE_SECURITY_AES_H_
//#pragma pack(1)
#include "zwave_api.h"
#include "zwave_appl.h"
#include "zwave_serialapi.h"
#include "zwave_association.h"

/* Global Nonce variables */
#define IN_TABLE_SIZE 8                      /*  Internal nonce table size */
/* (variable; max 128) */
#define INTERNAL_NONCE_LIFE 200/*30*/               /* Internal nonce life is 3 sec */
#define NONCE_TIMER                   (20*1000)      /* Nonce Timer, min. 3 sec, rec.10, max 20 */
#define INCLUSION_TIMER       (10*1000)

#define IS_FIRST_FRAME(h) (((h) & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK) \
            && !((h) & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SECOND_FRAME_BIT_MASK))

#define IS_SEQUENCED(h) ((h) && SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK)



#define NETWORK_KEY_LENGTH  16
#define EEOFFS_NETWORK_SECURITY             0
#define EEOFFS_NETWORK_SECURITY_SIZE        1
#define EEOFFS_NETWORK_KEY_START            EEOFFS_NETWORK_SECURITY + EEOFFS_NETWORK_SECURITY_SIZE
#define EEOFFS_NETWORK_KEY_SIZE             NETWORK_KEY_LENGTH

/*
The internal nonce table "intNonce[]" is used to store internal nonces. A record in the table is vacant if
nonce[0] is zero. lifeLeft is decreased by one every 100 ms. When it has reached zero, nonce[0]
is set to zero.
*/
/* Internal nonce table */
typedef struct _INT_NONCE_
{
    /* First byte=0 -> vacant record */
    BYTE nonce[8];
    /* Life left (unit: 100 ms) */
    BYTE lifeLeft;
    /* ID of the node it was sent to */
    BYTE nodeID;
} INT_NONCE;


/* Auxiliary authentication data */
typedef struct _AUTHDATA_
{
    BYTE iv[16];          /* Initialization vector for enc, dec,& auth */
    BYTE sh;              /* Security Header for authentication */
    BYTE senderNodeID;    /* Sender ID for authentication */
    BYTE receiverNodeID;  /* Receiver ID for authentication */
    BYTE payloadLength;   /* Length of authenticated payload */
} AUTHDATA;


#define SEQ_BUF_SIZE 100

typedef struct _SEQDATA_
{
    BYTE peerNodeId;
    BYTE sequenceCounter;
    BYTE buffer[SEQ_BUF_SIZE];
    BYTE offset;     /* data size stored in buffer */
} SEQDATA;

typedef struct _SECURITY_COMMAND_DATA_
{
    BYTE rxStatus;
    BYTE rxNodeID;
    BYTE cmdData[ZW_BUF_SIZE];
    BYTE dataLength;
} SECURITY_COMMAND_DATA;

extern ZW_STATUS ZW_SendDataSecure(BYTE tnodeID, BYTE *pBufData, BYTE dataLength, BYTE txSecOptions, VOID_CALLBACKFUNC(completedFunc)(BYTE));
extern void *ProcessIncomingSecure(void *args);
extern void InitSecurity();
extern ZW_STATUS ZW_SecurityNetworkKeySet(BYTE bNodeID);

#endif
