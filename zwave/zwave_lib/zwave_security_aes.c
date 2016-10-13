

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <openssl/aes.h>
#include <assert.h>

#include "zwave_security_aes.h"


/* Global Cryptography variables in saved NVRAM */
BYTE networkKey[16];                    /* The master key */

/* Global Cryptography variables in SRAM */
BYTE encKey[16];                        /* Encryption/decryption key */
BYTE authKey[16];                       /* Authentication key */

INT_NONCE intNonce[IN_TABLE_SIZE];

// External nonce record
BYTE enNonce[8];    /* External nonce */
BYTE enNodeID = 0;    /* Associated host id */

AUTHDATA authData;

BYTE prngState[16];

BYTE tag[16];

/* Message processing */
BYTE payloadPacket[sizeof(ZW_SECURITY_MESSAGE_ENCAP_FRAME)]; /* Buffer for outgoing packet with payload */

SEQDATA seqData;

#define ZW_AES_ECB(key, inputDat, outputDat) AES128_Encrypt(inputDat, outputDat, key)
#define PAYLOAD_SIZE(p) ((p) - 19)

ZW_STATUS gSecurityStatus = ZW_STATUS_IDLE;


void AES128_Encrypt(BYTE *input, BYTE *output, BYTE *key)
{
    AES_KEY AESEncryptKey;

    AES_set_encrypt_key(key, 128, &AESEncryptKey);
    AES_encrypt(input, output, &AESEncryptKey);
}

void AES128_Decrypt(BYTE *input, BYTE *output, BYTE *key)
{
    AES_KEY AESDecryptKey;

    AES_set_decrypt_key(key, 128, &AESDecryptKey);
    AES_decrypt(input, output, &AESDecryptKey);
}


void AESRaw(BYTE *pKey, BYTE *pSrc, BYTE *pDest)
{
    memcpy(pDest, pSrc, 16);
    ZW_AES_ECB(pKey, pSrc, pDest);
}

void AES_OFB(BYTE *bufdata, BYTE bufdataLength)
{
    BYTE i, j;
    BYTE ivIndex;
    BYTE blockIndex = 0;
    BYTE plaintext16ByteChunk[16];
    BYTE cipherIndex;

    memset((BYTE *)plaintext16ByteChunk, 0, 16);
    for (cipherIndex = 0; cipherIndex < bufdataLength; cipherIndex++)
    {
        plaintext16ByteChunk[blockIndex] = *(bufdata + cipherIndex);
        blockIndex++;
        if (blockIndex == 16)
        {
            ZW_AES_ECB(encKey, authData.iv, authData.iv);
            ivIndex = 0;
            for (i = (cipherIndex - 15); i <= cipherIndex; i++)
            {
                bufdata[i] = (BYTE)(plaintext16ByteChunk[ivIndex] ^ authData.iv[ivIndex]);
                ivIndex++;
            }
            memset((BYTE *)plaintext16ByteChunk, 0, 16);
            blockIndex = 0;
        }
    }

    if (blockIndex != 0)
    {
        ZW_AES_ECB(encKey, authData.iv, authData.iv);
        ivIndex = 0;
        for (j = 0; j < blockIndex; j++)
        {
            bufdata[cipherIndex - blockIndex + j] = (BYTE)(plaintext16ByteChunk[j] ^ authData.iv[j]);
            ivIndex++;
        }
    }
}


void AES_CBCMAC(BYTE *bufdata, BYTE bufdataLength, BYTE *MAC)
{
    BYTE i, j, k;
    BYTE blockIndex = 0;
    BYTE plaintext16ByteChunk[16];
    BYTE cipherIndex;
    BYTE inputData[98];

    // Generate input: [header] . [data]
    memcpy((BYTE *)&inputData[0], (BYTE *) &authData.iv[0], 20);
    memcpy((BYTE *)&inputData[20], bufdata, bufdataLength);
    // Perform initial hashing

    // Build initial input data, pad with 0 if length shorter than 16
    for (i = 0; i < 16; i++)
    {
        if (i >= sizeof(authData) + bufdataLength)
        {
            plaintext16ByteChunk[i] = 0;
        }
        else
        {
            plaintext16ByteChunk[i] = inputData[i];
        }

    }
    ZW_AES_ECB(authKey, &plaintext16ByteChunk[0], MAC);
    memset((BYTE *)plaintext16ByteChunk, 0, 16);

    blockIndex = 0;
    // XOR tempMAC with any left over data and encrypt

    for (cipherIndex = 16; cipherIndex < (sizeof(authData) + bufdataLength); cipherIndex++)
    {
        plaintext16ByteChunk[blockIndex] = inputData[cipherIndex];
        blockIndex++;
        if (blockIndex == 16)
        {
            for (j = 0; j <= 15; j++)
            {
                MAC[j] = (BYTE)(plaintext16ByteChunk[j] ^ MAC[j]);
            }
            memset((BYTE *)plaintext16ByteChunk, 0, 16);
            blockIndex = 0;

            ZW_AES_ECB(authKey, MAC, MAC);
        }
    }

    if (blockIndex != 0)
    {
        for (k = 0; k < 16; k++)
        {
            MAC[k] = (BYTE)(plaintext16ByteChunk[k] ^ MAC[k]);
        }
        ZW_AES_ECB(authKey, MAC, MAC);
    }
}

void EncryptPayload(BYTE *pSrc, BYTE length)
{
    memcpy(payloadPacket + 10, pSrc, length);
    AES_OFB(payloadPacket + 10, length);
}

void DecryptPayload(BYTE *pBufData, BYTE length)
{
    AES_OFB(pBufData, length);
}

void MakeAuthTag(void)
{
    /* AES_CBCMAC calculates 16 byte blocks, we only need 8
        for the auth tag */
    AES_CBCMAC(&payloadPacket[10], authData.payloadLength, tag);
    memcpy(payloadPacket + authData.payloadLength + 11, tag, 8);
}

BOOL VerifyAuthTag(BYTE *pPayload, BYTE payloadLength)
{
    AES_CBCMAC(pPayload, payloadLength, tag);
    return !memcmp(tag, pPayload + payloadLength + 1, 8);
}


void LoadKeys(void)
{
    BYTE pattern[16];

    memset((BYTE *)pattern, 0x55, 16);
    AESRaw(networkKey, pattern, authKey);   /* K_A = AES(K_N, pattern) */
    memset((BYTE *)pattern, 0xAA, 16);
    AESRaw(networkKey, pattern, encKey);    /* K_E = AES(K_N, pattern) */
}

void GetPRNGData(BYTE *pRNDData, BYTE noRNDDataBytes)
{
    BYTE i = 0;

    ZW_SetRFReceiveMode(FALSE);
    i = 0;

    do
    {
        ZW_GetRandomWord((BYTE *)(pRNDData + i), FALSE);
        i += 2;
    } while (--noRNDDataBytes && --noRNDDataBytes);
    /* Do we need to reenable RF? */
}


void PRNGUpdate(void)
{
    BYTE k[16], h[16], ltemp[16], btemp[16], i, j;

    /* H = 0xA5 (repeated x16) */
    memset((BYTE *)h, 0xA5, 16);
    /* The two iterations of the hardware generator */
    for (j = 0; j <= 1; j++)
    {
        /* Random data to K */
        GetPRNGData(k, 16);
        /* ltemp = AES(K, H) */
        AESRaw(k, h, ltemp);
        /* H = AES(K, H) ^ H */
        for (i = 0; i <= 15; i++)
        {
            h[i] ^= ltemp[i];
        }
    }
    /* Update inner state */
    /* S = S ^ H */
    for (i = 0; i <= 15; i++)
    {
        prngState[i] ^= h[i];
    }
    /* ltemp = 0x36 (repeated x16) */
    memset((BYTE *)ltemp, 0x36, 16);
    /* S = AES(S, ltemp) */
    AESRaw(prngState, ltemp, btemp);
    memcpy(prngState, btemp, 16);
    /* Reenable RF */
    ZW_SetRFReceiveMode(TRUE);
}

void PRNGInit(void)
{
    /* Reset PRNG State */
    memset(prngState, 0, 16);
    /* Update PRNG State */
    PRNGUpdate();
}

void PRNGOutput(BYTE *pDest)
{
    BYTE ltemp[16], btemp[16];

    /* Generate output */
    /* ltemp = 0x5C (repeated x16) */
    memset((BYTE *)ltemp, 0x5C/*0xA5*/, 16);
    /* ltemp = AES(PRNGState, ltemp) */
    AESRaw(prngState, ltemp, btemp);
    /* pDest[0..7] = ltemp[0..7] */
    memcpy(pDest, btemp, 8);
    /* Generate next internal state */
    /* ltemp = 0x36 (repeated x16) */
    memset((BYTE *)ltemp, 0x36, 16);
    /* PRNGState = AES(PRNGState, ltemp) */
    AESRaw(prngState, ltemp, btemp);
    memcpy(prngState, btemp, 16);
}


void MakeIN(BYTE tnodeID, BYTE *pDest)
{
    BYTE i;
    BYTE leastLifeLeft, newIndex=0;

    /* Find record in internal nonce table to use */
    leastLifeLeft = 255;
    for (i = 0; i < IN_TABLE_SIZE; i++)
    {
        /* If vacant... */
        if (intNonce[i].nonce[0] == 0)
        {
            newIndex = i;           /* Choose it */
            break;                  /* And we are done */
        }
        /* If less life left... */
        if (intNonce[i].lifeLeft < leastLifeLeft)
        {
            leastLifeLeft = intNonce[i].lifeLeft; /* Store new life left */
            newIndex = i;           /* And safe index as best bet */
        }
    }
    /* Generate nonce */
    /* Avoid collision check vs. old value */
    intNonce[newIndex].nonce[0] = 0;
    do
    {
        /* Generate new nonce */
        PRNGOutput(&authData.iv[0]);
        for (i = 0; i < IN_TABLE_SIZE; i++)
        {
            /* If collision... */
            if (authData.iv[0] == intNonce[i].nonce[0])
            {
                /* Invalidate */
                authData.iv[0] = 0;
                break;
            }
        }
    }
    while (!authData.iv[0]); /* Until valid nonce is found */
    /* Update intNonce[newIndex] and copy to pDest */
    /* Set life left */
    intNonce[newIndex].lifeLeft = INTERNAL_NONCE_LIFE;
    /* Set nodeID */
    intNonce[newIndex].nodeID = tnodeID;
    /* Copy nonce to nonce table */
    memcpy(intNonce[newIndex].nonce, authData.iv, 8);
    /* Copy nonce to destination */
    memcpy(pDest, authData.iv, 8);
}


BOOL GetIN(BYTE ri)
{
    BYTE i, j;

    /* ri = 0 is not allowed */
    if (ri == 0)
    {
        return FALSE;
    }
    /* Find record */
    for (i = 0; i < IN_TABLE_SIZE; i++) /* For all records in the table... */
    {
        /* If ri found... */
        if (intNonce[i].nonce[0] == ri)
        {
            /* Copy to second half of IV */
            memcpy(authData.iv + 8, intNonce[i].nonce, 8);
            /* Mark all records IN TABLE with same nodeID as vacant */
            for (j = 0; j < IN_TABLE_SIZE; j++)
            {
                /* If same node id.. */
                if (intNonce[j].nodeID == intNonce[i].nodeID)
                {
                    /* Mark as vacant */
                    intNonce[j].nonce[0] = 0;
                }
            }
            /* Return with success */
            return TRUE;
        }
    }
    /* Return false (ri not found) */
    return FALSE;
}

ZW_STATUS ZW_SecurityNonceReport(BYTE tnodeID)
{
    BYTE noncePacket[10];

    noncePacket[0] = COMMAND_CLASS_SECURITY;
    noncePacket[1] = SECURITY_NONCE_REPORT;
    MakeIN(tnodeID, noncePacket + 2);
    return ZW_SendData(tnodeID, noncePacket, 10, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
}


ZW_STATUS ZW_SendDataSecure(BYTE tnodeID, BYTE *pBufData, BYTE dataLength, BYTE txSecOptions, VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
    BYTE nonceRequestPacket[2];
    BYTE outPayload[55];
    DWORD timeout;
    ZW_STATUS retVal;

    /* Return error if dataLength > 29 */
    if (dataLength > 29)
    {
        return ZW_STATUS_INVALID_PARAM;
    }
    if (pBufData == NULL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    /* Get external nonce (or request one) */
    /* Get external nonce and RI */
    nonceRequestPacket[0] = COMMAND_CLASS_SECURITY;
    nonceRequestPacket[1] = SECURITY_NONCE_GET;
    /* Send nonce request */
    gSecurityStatus = ZW_STATUS_IDLE;
    retVal = ZW_SendData(tnodeID, nonceRequestPacket, 2, txSecOptions, completedFunc);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }
    timeout = NONCE_TIMER;
    if ((retVal = WaitStatusReady(&gSecurityStatus, ZW_STATUS_SECURITY_NONCE_REPORT_RECEIVED, &timeout)) != ZW_STATUS_OK)
    {
        goto END;
    }

    memset(outPayload, 0, sizeof(outPayload));
    memcpy(outPayload + 1, pBufData, dataLength);
    pBufData = outPayload;
    dataLength++;
    authData.sh = SECURITY_MESSAGE_ENCAPSULATION;
    payloadPacket[0] = COMMAND_CLASS_SECURITY;
    payloadPacket[1] = authData.sh;
    /* Write sender's nonce (SN) */
    MakeIN(tnodeID, payloadPacket + 2);
    /* Encrypt payload if present (EP) */
    EncryptPayload(pBufData, dataLength);
    /* Write receiver's nonce identifier (RI) */
    payloadPacket[dataLength + 10] = enNonce[0];
    /* Generate authentication tag (AT) */
    authData.senderNodeID = 0x01;  // Controller Node
    authData.receiverNodeID = tnodeID;
    authData.payloadLength = dataLength;
    memcpy(&authData.iv[0], &payloadPacket[2] , 8);
    memcpy(&authData.iv[8], enNonce, 8);
    /* Add AT */
    MakeAuthTag();
    /* Send data */
    gSecurityStatus = ZW_STATUS_IDLE;
    retVal =  ZW_SendData(tnodeID, payloadPacket, dataLength + 19, txSecOptions, completedFunc);

END:
    return retVal;
}


void *ProcessIncomingSecure(void *args)
{
    /* Macro to convert packetSize to size of encapsulated payload */
    /* 19 corresponds to security header (1), IV (8), sequencing hdr (1), r-nonce id (1), and MAC (8) */
    BYTE seqHeader;   /* sequencing header */
    BYTE reassembled_frame_size = 0;
    BYTE internal_nonce_found;
    ZW_STATUS retVal = ZW_STATUS_OK;

    SECURITY_COMMAND_DATA securityCommandData;
    BYTE  rxStatus, tnodeID, *pPacket, packetSize;

    if (args)
    {
        memcpy(&securityCommandData, args, sizeof(securityCommandData));
        free(args);
    }
    rxStatus = securityCommandData.rxStatus;
    tnodeID = securityCommandData.rxNodeID;
    pPacket = securityCommandData.cmdData;
    packetSize = securityCommandData.dataLength;

    /* If no packet data... */
    if (packetSize < 1)
    {
        return NULL;
    }

    /* Extract security header */
    authData.sh = pPacket[0];

    if (authData.sh == SECURITY_NONCE_GET)
    {
        gSecurityStatus = ZW_STATUS_SECURITY_NONCE_GET_RECEIVED;
        retVal = ZW_SecurityNonceReport(tnodeID);
        if (retVal != ZW_STATUS_OK)
        {
            return NULL;
        }
        gSecurityStatus = ZW_STATUS_SECURITY_NONCE_REPORT_SEND;
        return NULL;
    }

    if (authData.sh == SECURITY_NONCE_REPORT)
    {
        gSecurityStatus = ZW_STATUS_SECURITY_NONCE_REPORT_RECEIVED;

        enNodeID = tnodeID;
        memcpy(enNonce, pPacket + 1, 8);
        memcpy(authData.iv + 8, enNonce, 8);

        return NULL;
    }

    if (authData.sh == SECURITY_SCHEME_REPORT)
    {
        gSecurityStatus = ZW_STATUS_SECURITY_SCHEME_REPORT_RECEIVED;
        return NULL;
    }

    if (authData.sh == NETWORK_KEY_VERIFY)
    {
        gSecurityStatus = ZW_STATUS_SECURITY_NETWORK_KEY_VERIFY_RECEIVED;
        return NULL;
    }

    if (authData.sh == SECURITY_MESSAGE_ENCAPSULATION
            || authData.sh == SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET)
    {
        if (packetSize < 18)
        {
            /* Discard (incomplete packet) */
            return NULL;
        }
        internal_nonce_found = GetIN(pPacket[packetSize - 9]);
        if (!internal_nonce_found)
        {
            /* Discard (fake or expired RI) */
            return NULL;
        }
        /* Verify authentication */
        memcpy(authData.iv, pPacket + 1, 8);
        authData.senderNodeID = tnodeID;
        authData.receiverNodeID = 0x01;
        authData.payloadLength = packetSize - 18;
        /* If not authentic... */
        if (!VerifyAuthTag(pPacket + 9, packetSize - 18))
        {
            /* Discard (wrong auth. tag) */
            return NULL;
        }

        DecryptPayload(pPacket + 9, packetSize - 18);
        seqHeader = pPacket[9];
        if (!IS_SEQUENCED(seqHeader))
        {
            ApplicationCommandHandler(rxStatus, tnodeID, (ZW_APPLICATION_TX_BUFFER *) & (pPacket[10]), PAYLOAD_SIZE(packetSize));
            //return PAYLOAD_SIZE(packetSize) + 1;
        }
        else
        {
            if (IS_FIRST_FRAME(seqHeader))
            {
                seqData.peerNodeId = tnodeID;
                seqData.sequenceCounter = seqHeader & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCE_COUNTER_MASK;
                seqData.offset = PAYLOAD_SIZE(packetSize);
                memcpy(seqData.buffer, &pPacket[10], seqData.offset); /* 10 includes security header, IV and seq header byte */
            }
            else
            {
                /* this is second part of sequenced frame*/
                if (seqData.peerNodeId == tnodeID
                        && seqData.sequenceCounter == (seqHeader & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCE_COUNTER_MASK))
                {
                    if (seqData.offset < PAYLOAD_SIZE(packetSize))
                    {
                        /* overlapping memcpy regions - abort */
                        seqData.peerNodeId = ILLEGAL_NODE_ID;
                        return NULL;
                    }
                    /* move second frame payload data to make room for first frame payload */
                    memcpy(&pPacket[10 + seqData.offset], &pPacket[10], PAYLOAD_SIZE(packetSize));
                    /* copy back first frame payload */
                    memcpy(&pPacket[10], seqData.buffer, seqData.offset);
                    seqData.peerNodeId = ILLEGAL_NODE_ID;
                    reassembled_frame_size = seqData.offset + PAYLOAD_SIZE(packetSize);

                    ApplicationCommandHandler(rxStatus, tnodeID, (ZW_APPLICATION_TX_BUFFER *) & (pPacket[10]), reassembled_frame_size);
                }
            }
        }

        if (authData.sh == SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET)
        {
            retVal = ZW_SecurityNonceReport(tnodeID);
        }
        return NULL;
    }

    return NULL;
}


void MakeNetworkKey()
{
    memset(networkKey, 0x00, 16);
}

void InitSecurity()
{
    MakeNetworkKey();
    LoadKeys();
    PRNGInit();
    gSecurityStatus = ZW_STATUS_IDLE;
}

ZW_STATUS ZW_SecurityNetworkKeySet(BYTE bNodeID)
{
    ZW_STATUS retVal = ZW_STATUS_OK;
    ZW_APPLICATION_TX_BUFFER appCmdTxBuf;
    BYTE len = 0;
    DWORD timeout;

    // Scheme Get
    memset(&appCmdTxBuf, 0, sizeof(appCmdTxBuf));
    appCmdTxBuf.ZW_SecuritySchemeGetFrame.cmdClass = COMMAND_CLASS_SECURITY;
    appCmdTxBuf.ZW_SecuritySchemeGetFrame.cmd = SECURITY_SCHEME_GET;
    appCmdTxBuf.ZW_SecuritySchemeGetFrame.supportedSecuritySchemes = 0;
    len = sizeof(ZW_SECURITY_SCHEME_GET_FRAME);
    gSecurityStatus = ZW_STATUS_IDLE;
    retVal = ZW_SendData(bNodeID, (BYTE *)&appCmdTxBuf, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }
    // Wait for scheme report
    timeout = INCLUSION_TIMER;
    if ((retVal = WaitStatusReady(&gSecurityStatus, ZW_STATUS_SECURITY_SCHEME_REPORT_RECEIVED, &timeout)) != ZW_STATUS_OK)
    {
        goto END;
    }

    // Enc. Msg. (Key Set)
    BYTE NetworkkeySetPacket[18];
    NetworkkeySetPacket[0] = COMMAND_CLASS_SECURITY;
    NetworkkeySetPacket[1] = NETWORK_KEY_SET;
    memcpy(&(NetworkkeySetPacket[2]), networkKey, 16);
    retVal = ZW_SendDataSecure(bNodeID, (BYTE *)&NetworkkeySetPacket, 18, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    // Wait for Enc. Msg. (Key Verify)
    timeout = NONCE_TIMER;
    if ((retVal = WaitStatusReady(&gSecurityStatus, ZW_STATUS_SECURITY_NETWORK_KEY_VERIFY_RECEIVED, &timeout)) != ZW_STATUS_OK)
    {
        goto END;
    }

END:
    gSecurityStatus = ZW_STATUS_IDLE;
    return retVal;
}


