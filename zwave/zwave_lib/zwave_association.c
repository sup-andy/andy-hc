
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

#include "zwave_association.h"

ZW_NODES_ASSOCIATION *zw_nodes_association = NULL;


BYTE EncapCmdAssociationGet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE group)
{
    pBuf->ZW_AssociationGetV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
    pBuf->ZW_AssociationGetV2Frame.cmd = ASSOCIATION_GET;
    pBuf->ZW_AssociationGetV2Frame.groupingIdentifier = group;

    return sizeof(pBuf->ZW_AssociationGetV2Frame);
}

BYTE EncapCmdAssociationSupportGroupingsGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_AssociationGroupingsGetFrame.cmdClass = COMMAND_CLASS_ASSOCIATION;
    pBuf->ZW_AssociationGroupingsGetFrame.cmd = ASSOCIATION_GROUPINGS_GET;

    return sizeof(pBuf->ZW_AssociationGroupingsGetFrame);
}

BYTE EncapCmdAssociationRecordsSupportedGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_CommandRecordsSupportedGetFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION;
    pBuf->ZW_CommandRecordsSupportedGetFrame.cmd = COMMAND_RECORDS_SUPPORTED_GET;

    return sizeof(pBuf->ZW_CommandRecordsSupportedGetFrame);
}


ZW_STATUS ZW_AssociationSupportedGroupingsGet(WORD associatedID, BYTE *pSupportedGroupings)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = bNodeID;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdAssociationSupportGroupingsGet(&appTxBuffer);
    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (pSupportedGroupings)
            *pSupportedGroupings = zw_nodes_association->association[associatedID].supportedGroupings;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    return retVal;
}


ZW_STATUS ZW_AssociationGet(WORD associatedID, BYTE grouping, BYTE *pNodeID, BYTE *pNodeNum)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = bNodeID;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdAssociationGet(&appTxBuffer, grouping);
    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        BYTE i = zw_nodes_association->association[associatedID].grouping[grouping].associatedNodesNum;
        if (pNodeNum)
            *pNodeNum = zw_nodes_association->association[associatedID].grouping[grouping].associatedNodesNum;
        if (pNodeID && i)
        {
            for (i = 0; i < *pNodeNum; i++)
            {
                *(pNodeID + i) = zw_nodes_association->association[associatedID].grouping[grouping].associatedNodes[i].nodeID;
            }
        }
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    return retVal;
}

ZW_STATUS ZW_AssociationSet(WORD associatedID, BYTE grouping, BYTE DesNodeID)
{
    BYTE appTxBuffer[ZW_BUF_SIZE];
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_ASSOCIATION;
    appTxBuffer[len++] = ASSOCIATION_SET;
    appTxBuffer[len++] = grouping;
    appTxBuffer[len++] = DesNodeID;

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    return retVal;
}


ZW_STATUS ZW_AssociationRemove(WORD associatedID, BYTE grouping, BYTE DesNodeID)
{
    BYTE appTxBuffer[ZW_BUF_SIZE];
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_ASSOCIATION;
    appTxBuffer[len++] = ASSOCIATION_REMOVE;
    appTxBuffer[len++] = grouping;
    appTxBuffer[len++] = DesNodeID;

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    return retVal;
}


/* Association command class commands V2*/
ZW_STATUS ZW_AssociationSpecificGroupGet(WORD associatedID, BYTE *pGroup)
{
    BYTE appTxBuffer[ZW_BUF_SIZE];
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = bNodeID;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_ASSOCIATION;
    appTxBuffer[len++] = ASSOCIATION_SPECIFIC_GROUP_GET_V2;
    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (pGroup)
            *pGroup = zw_nodes_association->association[associatedID].specificGroup;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    return retVal;
}


ZW_STATUS ZW_AssociationRecordsSupportedGet(WORD associatedID, BYTE *pMaxCommandLength, BYTE *pVAndC, BYTE *pConfigurableSupported, WORD *pFreeRecords, WORD *pMaxRecords)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = bNodeID;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdAssociationRecordsSupportedGet(&appTxBuffer);

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (pMaxCommandLength)
            *pMaxCommandLength = zw_nodes_association->association[associatedID].maxCommandLength;
        if (pVAndC)
            *pVAndC = zw_nodes_association->association[associatedID].VAndC;
        if (pConfigurableSupported)
            *pConfigurableSupported = zw_nodes_association->association[associatedID].configurableSupported;
        if (pFreeRecords)
            *pFreeRecords = zw_nodes_association->association[associatedID].freeCommandRecords;
        if (pMaxRecords)
            *pMaxRecords = zw_nodes_association->association[associatedID].maxCommandRecords;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    return retVal;
}

ZW_STATUS ZW_AssociationConfigurationGet(WORD associatedID, BYTE grouping, BYTE nodeID)
{
    BYTE appTxBuffer[ZW_BUF_SIZE];
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = bNodeID;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION;
    appTxBuffer[len++] = COMMAND_CONFIGURATION_GET;
    appTxBuffer[len++] = grouping;
    appTxBuffer[len++] = nodeID;
    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        ;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    return retVal;
}


ZW_STATUS ZW_AssociationConfigurationSet(WORD associatedID, BYTE grouping, BYTE nodeID, BYTE cmdLength, BYTE *pCmd)
{
    BYTE appTxBuffer[ZW_BUF_SIZE];
    ZW_STATUS retVal;
    BYTE len = 0, i = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = associatedID & 0xff;
    endPoint = (associatedID >> 8) & 0xff;

    // check whether association node or not
    if (!OwnCommandClass(bNodeID, COMMAND_CLASS_ASSOCIATION))
    {
        // not association node
        return ZW_STATUS_FAIL;
    }

    // check whether free record association configuration or not
    BYTE maxCommandLength = 0, CAndV = 0, configurableSupported = 0;
    WORD freeRecords = 0, maxRecords = 0;
    if (ZW_AssociationRecordsSupportedGet(bNodeID, &maxCommandLength, &CAndV, &configurableSupported, &freeRecords, &maxRecords) != ZW_STATUS_OK)
    {
        return ZW_STATUS_FAIL;
    }
    if (freeRecords == 0)
    {
        // there is no room for new association configuration
        return ZW_STATUS_FAIL;
    }

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION;
    appTxBuffer[len++] = COMMAND_CONFIGURATION_SET;
    appTxBuffer[len++] = grouping;
    appTxBuffer[len++] = nodeID;
    appTxBuffer[len++] = cmdLength;
    for (i = 0; i < cmdLength; i++)
    {
        appTxBuffer[len++] = *(pCmd + i);
    }
    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}


