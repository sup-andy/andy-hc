
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

#include "zwave_serialapi.h"
#include "zwave_association.h"
#include "zwave_uart.h"

pthread_mutex_t g_cmd_internal_mutex = PTHREAD_MUTEX_INITIALIZER;

ZW_STATUS exe_status = ZW_STATUS_IDLE;
ZW_STATUS response_status = ZW_STATUS_IDLE;
ZW_STATUS ack_status = ZW_STATUS_IDLE;


int add_remove_node_flag = 0;

ZW_NODES_INFO *zw_nodes;
BYTE gResponseBuffer[ZW_BUF_SIZE];
BYTE gRequestBuffer[ZW_BUF_SIZE];
BYTE gResponseBufferLength = 0;
BYTE gRequestBufferLength = 0;
ZW_TIME timer_array[TIMER_MAX];
WORD gGettingStatusNodeID = ILLEGAL_NODE_ID;
ZW_STATUS gGettingNodeStatus = ZW_STATUS_IDLE;
BYTE gNodeRequest = ILLEGAL_NODE_ID;

void (*cbFuncReportHandle)(ZW_DEVICE_INFO *);
void (*cbFuncEventHandler)(ZW_EVENT_REPORT *);
void (*cbFuncLearnModeHandler)(LEARN_INFO *);
void (*cbFuncAddRemoveNodeSuccess)(ZW_DEVICE_INFO *);
void (*cbFuncAddRemoveNodeFail)(ZW_STATUS);
void (*cbFuncSendData)(BYTE);
void (*cbFuncRemoveFailedNodeHandler)(BYTE);
void (*cbFuncSetDefaultHandler)(void);
void (*cbFuncMemoryPutBuffer)(void);
void (*cbFuncAssignReturnRouteHandler)(BYTE);
void (*cbFuncDeleteReturnRouteHandler)(BYTE);

/*****************************************************************************
*Function Name: CalculateChecksum
*Description: Calculate the data send to (or received from) Serial.
*                  Include the length, type, command id and command specific data
*Input:
*Output:   checksum
******************************************************************************/
BYTE CalculateChecksum(BYTE *pData, int len)
{
    BYTE checksum = 0xff;

    for (; len; len--)
    {
        checksum ^= *pData++;
    }

    return checksum;
}

BYTE OwnCommandClass(WORD nodeID, BYTE cmdClass)
{
    int i = 0, j = 0;

    for (i = 0; i < MAX_CMDCLASS_BITMASK_LEN; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (zw_nodes->nodes[nodeID].cmdclass_bitmask[i] & (0x01 << j))
            {
                BYTE cmdClass1 = i * 8 + j + 1;
                if (cmdClass1 == cmdClass)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

ZW_STATUS WriteCom(BYTE *data, BYTE len)
{
    assert(data != NULL);
    assert(len > 0);

    int ret = 0;

#if (ZWAVE_DRIVER_DEBUG == 1)
    ret = ZW_UART_tx(data, len);
    if (ret != len)
    {
        DEBUG_ERROR("[ZWave Driver] Serial send data error! length[%u] \n", len);
        return ZW_STATUS_FAIL;
    }
#else
    ret = serial_send(data, len);
    if (ret != len)
    {
#ifdef ZWAVE_DRIVER_DEBUG
    	DEBUG_ERROR("[ZWave Driver] Serial send data error! length[%u] \n", len);
#endif
        return ZW_STATUS_FAIL;
    }
#endif


    return ZW_STATUS_OK;
}

ZW_STATUS  Request(BYTE func, BYTE *pRequest, BYTE reqLen)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE idx = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    buffer[0] = SOF;
    buffer[1] = 3 + reqLen;
    buffer[2] = REQUEST;
    buffer[3] = func;
    while (idx < reqLen)
    {
        buffer[4 + idx] = pRequest[idx];
        idx++;
    }
    buffer[4 + reqLen] = CalculateChecksum(&(buffer[1]), buffer[1]);

    ack_status = ZW_STATUS_IDLE;
    response_status = ZW_STATUS_IDLE;

    // write serial com
    WriteCom(buffer, buffer[1] + 2);

    timeout = ACK_TIMER;
    if ((retVal = WaitStatusReady(&ack_status, ZW_STATUS_ACK_RECEIVED, &timeout)) != ZW_STATUS_OK)
    {
        return retVal;
    }

    return retVal;
}


ZW_STATUS SendRequestAndWaitForResponse(BYTE func, BYTE *pRequest, BYTE reqLen, BYTE *pResponse, BYTE *pResLen)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    buffer[0] = SOF;
    buffer[1] = 3 + reqLen;
    buffer[2] = REQUEST;
    buffer[3] = func;
    while (bLen < reqLen)
    {
        buffer[4 + bLen] = pRequest[bLen];
        bLen++;
    }
    buffer[4 + reqLen] = CalculateChecksum(&(buffer[1]), buffer[1]);

    ack_status = ZW_STATUS_IDLE;
    response_status = ZW_STATUS_IDLE;

    // write serial com
    retVal = WriteCom(buffer, buffer[1] + 2);
    if (retVal != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] write com failed, retVal=%d, errno=%d. \n", retVal, errno);
#endif
        return retVal;
    }

    timeout = ACK_TIMER;
    if ((retVal = WaitStatusReady(&ack_status, ZW_STATUS_ACK_RECEIVED, &timeout)) != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] wait for ACK timeout. timeout[%u] \n", ACK_TIMER/1000);
#endif
        return retVal;
    }

    timeout = RESPONSE_TIMER;
    do
    {
        if ((retVal = WaitStatusReady(&response_status, ZW_STATUS_RESPONSE_RECEIVED, &timeout)) != ZW_STATUS_OK)
        {
            return retVal;
        }

        GetResponseBuffer(buffer, &bLen);

    } while (buffer[0] != func);

    *pResLen = bLen;
    memcpy(pResponse, buffer, bLen);

    return retVal;
}


void GetResponseBuffer(BYTE *pResponse, BYTE *pLen)
{
    memcpy(pResponse, gResponseBuffer, gResponseBufferLength);
    *pLen = gResponseBufferLength;

    response_status = ZW_STATUS_IDLE;
}

BYTE DoAck(void)
{
    // write ACK to serial com
    BYTE data = ACK;
    WriteCom(&data, 1);
    return TRUE;
}

void MsecSleep(DWORD Msec)
{
    struct timeval tv;

    if (Msec > 0)
    {
        tv.tv_sec = Msec / 1000;
        tv.tv_usec = (Msec % 1000) * 1000;
    }
    else
    {
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
    }

    select(1, NULL, NULL, NULL, &tv);
}

ZW_STATUS WaitStatusReady(ZW_STATUS *pWatchStatus, ZW_STATUS expect_status, DWORD *pMsec)
{
    while (*pMsec > 0)
    {
        if ((*pWatchStatus) == expect_status)
        {
            return ZW_STATUS_OK;
        }

        MsecSleep(1);
        (*pMsec) --;
    }

    return ZW_STATUS_TIMEOUT;
}

ZW_STATUS CheckDataValid(BYTE *pData, BYTE len)
{
    BYTE checksum;

    assert(pData != NULL);
    assert(len > 0);

    if (len == 1)
    {
        if (*pData == ACK
                || *pData == NAK
                || *pData == CAN)
        {
            return ZW_STATUS_OK;
        }
        else
        {
            return ZW_STATUS_FAIL;
        }
    }
    else
    {
        if (*pData != SOF)
        {
            return ZW_STATUS_FAIL;
        }

        if (len != *(pData + 1) + 2)
        {
            return ZW_STATUS_FAIL;
        }

        checksum = CalculateChecksum(pData + 1, *(pData + 1));
        if (checksum != *(pData + * (pData + 1) + 1))
        {
            // write serial com NAK

            return ZW_STATUS_FAIL;
        }
    }

    return ZW_STATUS_OK;
}



ZW_STATUS ZW_AddNodeToNetwork(BYTE bMode, VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO *))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen  = 0;
    ZW_STATUS ret = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    cbFuncLearnModeHandler = completedFunc;

    buffer[bLen++] = bMode;
    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);

    ret = Request(FUNC_ID_ZW_ADD_NODE_TO_NETWORK, buffer, bLen);

    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return ret;

}


void AddNodeLearnHandler(LEARN_INFO *learn_info)
{
    assert(learn_info != NULL);
    static NODEINFO nodeInfo;
    int i = 0;

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] Add node step %u. \n", learn_info->bStatus);
#endif

    if(learn_info->bLen >= 3)
    {
        memset(&nodeInfo, 0, sizeof(nodeInfo));
        nodeInfo.nodeType.basic = learn_info->pCmd[0];
        nodeInfo.nodeType.generic = learn_info->pCmd[1];
        nodeInfo.nodeType.specific = learn_info->pCmd[2];
        
        zw_nodes->nodes[learn_info->bSource].id = learn_info->bSource;
        zw_nodes->nodes[learn_info->bSource].type = Analysis_DeviceType(&nodeInfo);
        zw_nodes->nodes[learn_info->bSource].phy_id = learn_info->bSource;
        for(i = 3; i < learn_info->bLen; i++)
        {
            zw_nodes->nodes[learn_info->bSource].cmdclass_bitmask[(learn_info->pCmd[i] - 1) >> 3] |= 0x01 << ((learn_info->pCmd[i] - 1) & 0x7);
        }
        zw_nodes->nodes[learn_info->bSource].dev_num = 1;
    }

    switch (learn_info->bStatus)
    {
        case ADD_NODE_STATUS_LEARN_READY:
            break;
        case ADD_NODE_STATUS_NODE_FOUND:
            break;
        case ADD_NODE_STATUS_ADDING_SLAVE:
            break;
        case ADD_NODE_STATUS_ADDING_CONTROLLER:
            break;
        case ADD_NODE_STATUS_PROTOCOL_DONE:
            {
                add_remove_node_flag = 0;
                ZW_timerCancel(TIMER_ADD_DELETE_NODES);
                ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);

#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_INFO("[ZWave Driver] New Node.  id[0x%x] type[%u]\n", zw_nodes->nodes[learn_info->bSource].id, zw_nodes->nodes[learn_info->bSource].type);
#endif

                if(zw_nodes->nodes[learn_info->bSource].id
                    &&zw_nodes->nodes[learn_info->bSource].type != ZW_DEVICE_UNKNOW)
                {
                    ZW_DATA zw_data;
                    memset(&zw_data, 0, sizeof(zw_data));
                    zw_data.type = ZW_DATA_TYPE_ADD_NODE_RESPONSE;
                    memcpy(&zw_data.data.addRemoveNodeRes.deviceInfo, &(zw_nodes->nodes[learn_info->bSource]), sizeof(ZW_DEVICE_INFO));
                    ZW_sendMsgToProcessor((BYTE *)&zw_data, sizeof(zw_data));
                }

                pthread_mutex_unlock(&g_cmd_mutex);
            }
            break;
        case ADD_NODE_STATUS_DONE:
            //break;
        case ADD_NODE_STATUS_FAILED:
            ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
            break;
        default:
            ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
            break;
    }

}

ZW_STATUS ZW_RemoveNodeFromNetwork(BYTE bMode, VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO *))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen  = 0;
    ZW_STATUS ret = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    cbFuncLearnModeHandler = completedFunc;

    buffer[bLen++] = bMode;
    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);

    ret = Request(FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK, buffer, bLen);

    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return ret;

}


void RemoveNodeLearnHandler(LEARN_INFO *learn_info)
{
    assert(learn_info != NULL);
    static NODEINFO nodeInfo;

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] Remove node step %u. \n", learn_info->bStatus);
#endif

    if(learn_info->bLen >= 3)
    {
        memset(&nodeInfo, 0, sizeof(nodeInfo));
        nodeInfo.nodeType.basic = learn_info->pCmd[0];
        nodeInfo.nodeType.generic = learn_info->pCmd[1];
        nodeInfo.nodeType.specific = learn_info->pCmd[2];
    }

    switch (learn_info->bStatus)
    {
        case REMOVE_NODE_STATUS_LEARN_READY:
            break;
        case REMOVE_NODE_STATUS_NODE_FOUND:
            break;
        case REMOVE_NODE_STATUS_REMOVING_SLAVE:
            break;
        case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
            break;
        case REMOVE_NODE_STATUS_DONE:
            {
                add_remove_node_flag = 0;
                ZW_timerCancel(TIMER_ADD_DELETE_NODES);
                ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);

                ZW_DATA zw_data;
                ZW_DEVICE_INFO deviceInfo;
                memset(&deviceInfo, 0, sizeof(deviceInfo));
                deviceInfo.type = Analysis_DeviceType(&nodeInfo);

#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_INFO("[ZWave Driver] Remove Node. id[0x%x] type[%u] basic[%u] generic[%u] specific[%u] \n", learn_info->bSource, deviceInfo.type,
                nodeInfo.nodeType.basic, nodeInfo.nodeType.generic, nodeInfo.nodeType.specific);
#endif
                if(deviceInfo.type != ZW_DEVICE_UNKNOW)
                {
                    deviceInfo.id = learn_info->bSource;
                    memset(&zw_data, 0, sizeof(zw_data));
                    zw_data.type = ZW_DATA_TYPE_REMOVE_NODE_RESPONSE;
                    memcpy(&zw_data.data.addRemoveNodeRes.deviceInfo, &deviceInfo, sizeof(ZW_DEVICE_INFO));
                    ZW_sendMsgToProcessor((BYTE *)&zw_data, sizeof(zw_data));
                }

                pthread_mutex_unlock(&g_cmd_mutex);
            }
            break;
        case REMOVE_NODE_STATUS_FAILED:
            ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);
            break;
        default:
            ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);
            break;
    }

}


ZW_STATUS ZW_GetRandomWord(BYTE *randomWord, BOOL bResetRadio)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    BYTE randomGenerationSuccess = 0, noRandomBytesGenerated = 0;
    BYTE *noRandomGenerated = NULL;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_GET_RANDOM, NULL, 0, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen < 4)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }

    randomGenerationSuccess = buffer[1];
    noRandomBytesGenerated = buffer[2];
    noRandomGenerated = &(buffer[3]);
    if (randomGenerationSuccess == TRUE)
        memcpy(randomWord, noRandomGenerated, noRandomBytesGenerated);

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}


ZW_STATUS ZW_SetRFReceiveMode(BYTE mode)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    buffer[bLen++] = mode;
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_SET_RF_RECEIVE_MODE, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen != 2)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }

    if (buffer[1] != TRUE)
    {
        retVal = ZW_STATUS_FAIL;
    }

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

void SetDefaultHandler(void)
{
    memset(zw_nodes, 0, sizeof(ZW_NODES_INFO));
    memset(zw_nodes_association, 0, sizeof(ZW_NODES_ASSOCIATION));
    memset(zw_nodes_configuration, 0, sizeof(ZW_NODES_CONFIGURATION));
    memset(StatusUpdateTimer, 0, sizeof(ZW_NODE_STATUS_UPDATE_TIMER));
    memset(timer_array, 0, sizeof(timer_array));

    add_remove_node_flag = 0;

    zw_nodes->nodes_num = 1;
    zw_nodes->nodes[1].id = 1;
    zw_nodes->nodes[1].phy_id = 1;
    zw_nodes->nodes[1].type = ZW_DEVICE_STATIC_CONTROLLER;
    SaveNodesInfo(zw_nodes);

    exe_status = ZW_STATUS_SET_DEFAULT_DONE;
}

ZW_STATUS ZW_SetDefault(VOID_CALLBACKFUNC(completedFunc)(void))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen  = 0;
    DWORD timeout = 0;
    ZW_STATUS ret = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_IDLE;

    cbFuncSetDefaultHandler = completedFunc;

    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);
    ret =  Request(FUNC_ID_ZW_SET_DEFAULT, buffer, bLen);
    if (ret != ZW_STATUS_OK)
    {
        pthread_mutex_unlock(&g_cmd_internal_mutex);
        return ZW_STATUS_FAIL;
    }

    timeout = REQUEST_TIMER;
    if ((ret = WaitStatusReady(&exe_status, ZW_STATUS_SET_DEFAULT_DONE, &timeout)) != ZW_STATUS_OK)
    {
    }

    exe_status = ZW_STATUS_IDLE;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return ret;

}

ZW_STATUS ZW_SoftReset(void)
{
    ZW_STATUS ret = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    ret =  Request(FUNC_ID_SERIAL_API_SOFT_RESET, NULL, 0);

    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return ret;

}

ZW_STATUS ZW_Version(BYTE *lib_version, BYTE *lib_type)
{
    assert(lib_version != NULL);
    assert(lib_type != NULL);

    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen  = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_GET_VERSION, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen != 14)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }
    memcpy(lib_version, &(buffer[1]), 12);
    *lib_type = buffer[13];

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;

}

ZW_STATUS ZW_MemoryGetID(BYTE *pHomeID, BYTE *pNodeID)
{
    assert(pHomeID != NULL);
    assert(pNodeID != NULL);

    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen  = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_MEMORY_GET_ID, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen != 6)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }
    memcpy(pHomeID, &(buffer[1]), 4);
    *pNodeID = buffer[5];

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

ZW_STATUS MemoryGetBuffer(WORD offset, BYTE *pBuffer, BYTE length)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    buffer[bLen++] = (offset >> 8) & 0xff;
    buffer[bLen++] = offset & 0xff;
    buffer[bLen++] = length;
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_MEMORY_GET_BUFFER, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    memcpy(pBuffer, buffer, bLen);

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

void CB_MemoryPutBuffer(void)
{
    exe_status = ZW_STATUS_PUT_BUFFER_REQUEST_RECEIVED;
}

ZW_STATUS MemoryPutBuffer(WORD offset, BYTE *pBuffer, WORD length, VOID_CALLBACKFUNC(func)(void))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    BYTE i = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_IDLE;

    cbFuncMemoryPutBuffer = func;

    buffer[bLen++] = (offset >> 8) & 0xff;
    buffer[bLen++] = offset & 0xff;
    buffer[bLen++] = (length >> 8) & 0xff;
    buffer[bLen++] = length & 0xff;
    while (i != length)
    {
        buffer[bLen++] = pBuffer[i++];
    }
    buffer[bLen++] = (func == NULL ? 0 : 0x03);
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_MEMORY_PUT_BUFFER, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (cbFuncMemoryPutBuffer)
    {
        timeout = REQUEST_TIMER;
        if ((retVal = WaitStatusReady(&exe_status, ZW_STATUS_PUT_BUFFER_REQUEST_RECEIVED, &timeout)) != ZW_STATUS_OK)
        {
            goto END;
        }
    }

END:
    exe_status = ZW_STATUS_IDLE;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

/*
 * HOST->ZW: REQ | 0x02
 * (Controller)  ZW->HOST:  RES | 0x02 | ver | capabilities | 29 | nodes[29] | chip_type | chip_version 
 * (Slave)  ZW->HOST: RES | 0x02 | ver | capabilities | 0 | chip_type | chip_version 
 */
ZW_STATUS ZW_GetInitData(BYTE *version, BYTE *capabilities, BYTE *len, BYTE *nodes, BYTE *chip_type, BYTE *chip_version)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_SERIAL_API_GET_INIT_DATA, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen != 35)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }

    if (version)
        *version = buffer[1];
    if (capabilities)
        *capabilities = buffer[2];
    if (len)
        *len = buffer[3];
    if (nodes)
        memcpy(nodes, &(buffer[4]), 29);
    if (chip_type)
        *chip_type = buffer[33];
    if (chip_version)
        *chip_version = buffer[34];

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

/*
 * Return the Node Information Frame without command classes from the NVM for a given node ID:
 *              |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 * Capability:           
 * Security:             
 * Reserved:
 * Basic:       |       Basic Device Class (Z-Wave Protocol Specific Part)
 * Generic:     |       Generic Device Class (Z-Wave Appl. Specific Part)
 * Specific:    |       Specific Device Class (Z-Wave Appl. Specific Part)
 *
 *
 * Note: Node Information frame structure without command classes.
 *
 */
ZW_STATUS ZW_GetNodeProtocolInfo(BYTE bNodeID, NODEINFO *nodeInfo)
{
    assert(nodeInfo != NULL);

    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    buffer[bLen++] = bNodeID;
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen != 7)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }
    nodeInfo->capability = buffer[1];
    nodeInfo->security = buffer[2];
    nodeInfo->reserved = buffer[3];
    nodeInfo->nodeType.basic = buffer[4];
    nodeInfo->nodeType.generic = buffer[5];
    nodeInfo->nodeType.specific = buffer[6];

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

ZW_STATUS ZW_RequestNodeInfo(BYTE nodeID, void (*completedFunc)(BYTE txStatus))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_REQUEST_NODEINFO;
    gNodeRequest = nodeID;

    buffer[bLen++] = nodeID;
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_REQUEST_NODE_INFO, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen == 2 &&buffer[1] == TRUE)
    {
        // Z-Wave chip transmit success
        // call completedFunc if necessary
         timeout = NODEINFO_REQUEST_TIMER;
         while(timeout)
         {
             if(exe_status == ZW_STATUS_NODEINFO_RECEIVED)
             {
                 retVal = ZW_STATUS_OK;
                 goto END;
             }
             else if(exe_status == ZW_STATUS_NODEINFO_RECEIVED_FAIL)
             {
                 retVal = ZW_STATUS_FAIL;
                 goto END;
             }
             
             MsecSleep(1);
             timeout--;
             if(timeout == 0)
             {
                 retVal = ZW_STATUS_TIMEOUT;
                 goto END;
             }
             
         }

    }
    else
    {
        retVal = ZW_STATUS_FAIL;
    }

END:
    exe_status = ZW_STATUS_IDLE;
    gNodeRequest = ILLEGAL_NODE_ID;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

ZW_STATUS ZW_isFailedNode(WORD nodeID)
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE bNodeID = 0, endPoint = 0;

    pthread_mutex_lock(&g_cmd_internal_mutex);

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;


    buffer[bLen++] = bNodeID;
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_IS_FAILED_NODE_ID, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen != 2)
    {
        retVal = ZW_STATUS_FAIL;
        goto END;
    }

    if (buffer[1] == TRUE)
    {
        retVal = ZW_STATUS_IS_FAIL_NODE;
    }
    else
    {
        retVal = ZW_STATUS_NOT_FAIL_NODE;
    }

END:
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

void CB_RemoveFailedNodeID(BYTE txStatus)
{
    exe_status = ZW_STATUS_FAIL_NODE_REMOVED_FAIL;
    switch (txStatus)
    {
        case ZW_NODE_OK:
            break;
        case ZW_FAILED_NODE_REMOVED:
            exe_status = ZW_STATUS_FAIL_NODE_REMOVED;
            break;
        case ZW_FAILED_NODE_NOT_REMOVED:
            break;
        default:
            break;
    }
}

ZW_STATUS ZW_RemoveFailedNodeID(WORD nodeID, VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE bNodeID = 0, endPoint = 0;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_IDLE;

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;

    cbFuncRemoveFailedNodeHandler = completedFunc;

    buffer[bLen++] = bNodeID;
    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_REMOVE_FAILED_NODE_ID, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if(bLen == 2)
    {
        switch (buffer[1])
        {
            case ZW_FAILED_NODE_REMOVE_STARTED:
            {
                timeout = REQUEST_TIMER;
                while(timeout)
                {
                    if(exe_status == ZW_STATUS_FAIL_NODE_REMOVED)
                    {
                        retVal = ZW_STATUS_OK;
                        goto END;
                    }
                    else if(exe_status == ZW_STATUS_FAIL_NODE_REMOVED_FAIL)
                    {
                        retVal = ZW_STATUS_FAIL;
                        goto END;
                    }
                    MsecSleep(1);
                    timeout--;
                    if(timeout == 0)
                    {
                        retVal = ZW_STATUS_TIMEOUT;
                        goto END;
                    }
                    
                }
            }
                break;
            case ZW_NOT_PRIMARY_CONTROLLER:
                break;
            case ZW_NO_CALLBACK_FUNCTION:
                break;
            case ZW_FAILED_NODE_NOT_FOUND:
                break;
            case ZW_FAILED_NODE_REMOVE_PROCESS_BUSY:
                break;
            case ZW_FAILED_NODE_REMOVE_FAIL:
                break;
            default:
                break;
        }
    }
    retVal = ZW_STATUS_FAIL;

END:
    if(retVal != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] Send request and wait response fail. nodeID[0x%x] ret[%u] rvalue[0x%x]\n", nodeID, retVal, buffer[1]);
#endif
    }
    exe_status = ZW_STATUS_IDLE;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

void CB_SendDataComplete(BYTE txStatus)
{
    exe_status = ZW_STATUS_SENDDATA_FAIL;
    switch (txStatus)
    {
        case TRANSMIT_COMPLETE_OK:
            exe_status = ZW_STATUS_SENDDATA_SUCCESSFUL;
            break;
        case TRANSMIT_COMPLETE_NO_ACK:
        {
            ZW_SendDataAbout();
        }
        break;
        case TRANSMIT_COMPLETE_FAIL:
            break;
        case TRANSMIT_COMPLETE_NOROUTE:
            break;
        default:
            break;
    }

}

ZW_STATUS ZW_SendData(BYTE nodeID, BYTE *pData, BYTE dataLen, BYTE options, VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    DWORD timeout = 0;
    BYTE i = 0;
    BYTE buffer1[ZW_BUF_SIZE] = {0};
    BYTE bLen1 = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_IDLE;

    cbFuncSendData = completedFunc;

    buffer[bLen++] = nodeID;
    buffer[bLen++] = dataLen;
    while (i < dataLen)
    {
        buffer[bLen++] = pData[i++];
    }
    buffer[bLen++] = options;
    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);

    int txCount = 0;
    while (txCount < 1)
    {
    	retVal = ZW_STATUS_FAIL;
        memset(buffer1, 0, sizeof(buffer1));
        if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_SEND_DATA, buffer, bLen, buffer1, &bLen1)) == ZW_STATUS_OK)
        {
            if (bLen1 == 2 && buffer1[1] == TRUE)
            {
                timeout = REQUEST_TIMER;
                while(timeout)
                {
                    if(exe_status == ZW_STATUS_SENDDATA_SUCCESSFUL)
                    {
                        retVal = ZW_STATUS_OK;
                        goto END;
                    }
                    else if(exe_status == ZW_STATUS_SENDDATA_FAIL)
                    {
                        retVal = ZW_STATUS_FAIL;
                        goto END;
                    }
                    MsecSleep(1);
                    timeout--;
                    if(timeout == 0)
                    {
                        retVal = ZW_STATUS_TIMEOUT;
                        goto END;
                    }
					
                }
                
            }
            else
            {
                retVal = ZW_STATUS_FAIL;
            }
        }

        txCount++;
        usleep(100 * 1000 + txCount * 1000 * 1000); //sleep 100 ms + n * 1000 ms
    }

END:
    if(retVal != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] chip send data fail. nodeID[%x] ret[%u]\n", nodeID, retVal);
#endif
    }

    exe_status = ZW_STATUS_IDLE;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}


void ZW_SendDataAbout(void)
{
    pthread_mutex_lock(&g_cmd_internal_mutex);
    Request(FUNC_ID_ZW_SEND_DATA_ABORT, NULL , 0);
    pthread_mutex_unlock(&g_cmd_internal_mutex);
}

void CB_AssignReturnRoute(BYTE txStatus)
{
    exe_status = ZW_STATUS_ASSIGN_RETURN_ROUTE_FAIL;
    switch (txStatus)
    {
        case TRANSMIT_COMPLETE_OK:
            exe_status = ZW_STATUS_ASSIGN_RETURN_ROUTE_OK;
            break;
        case TRANSMIT_COMPLETE_NO_ACK:
        break;
        case TRANSMIT_COMPLETE_FAIL:
            break;
        default:
            break;
    }

}

ZW_STATUS ZW_AssignReturnRoute(BYTE bSrcNodeID, BYTE bDstNodeID, VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_IDLE;

    cbFuncAssignReturnRouteHandler = completedFunc;

    buffer[bLen++] = bSrcNodeID;
    buffer[bLen++] = bDstNodeID;
    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen == 2 && buffer[1] == TRUE)
    {
        timeout = REQUEST_TIMER;
        while(timeout)
        {
            if(exe_status == ZW_STATUS_ASSIGN_RETURN_ROUTE_OK)
            {
                retVal = ZW_STATUS_OK;
                goto END;
            }
            else if(exe_status == ZW_STATUS_ASSIGN_RETURN_ROUTE_FAIL)
            {
                retVal = ZW_STATUS_FAIL;
                goto END;
            }
            MsecSleep(1);
            timeout--;
            if(timeout == 0)
            {
                retVal = ZW_STATUS_TIMEOUT;
                goto END;
            }
            
        }
        
    }
    else
    {
        retVal = ZW_STATUS_FAIL;
    }

END:
    if(retVal != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] Send request and wait response fail. nodeID[0x%x] ret[%u]\n", bSrcNodeID, retVal);
#endif
    }
    exe_status = ZW_STATUS_IDLE;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}

void CB_DeleteReturnRoute(BYTE txStatus)
{
    exe_status = ZW_STATUS_DELETE_RETURN_ROUTE_FAIL;
    switch (txStatus)
    {
        case TRANSMIT_COMPLETE_OK:
            exe_status = ZW_STATUS_DELETE_RETURN_ROUTE_OK;
            break;
        case TRANSMIT_COMPLETE_NO_ACK:
        break;
        case TRANSMIT_COMPLETE_FAIL:
            break;
        default:
            break;
    }

}

ZW_STATUS ZW_DeleteReturnRoute(BYTE nodeID, VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus))
{
    BYTE buffer[ZW_BUF_SIZE] = {0};
    BYTE bLen = 0;
    DWORD timeout = 0;
    ZW_STATUS retVal = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_internal_mutex);
    exe_status = ZW_STATUS_IDLE;

    cbFuncDeleteReturnRouteHandler = completedFunc;

    buffer[bLen++] = nodeID;
    buffer[bLen++] = (completedFunc == NULL ? 0 : 0x03);
    if ((retVal = SendRequestAndWaitForResponse(FUNC_ID_ZW_DELETE_RETURN_ROUTE, buffer, bLen, buffer, &bLen)) != ZW_STATUS_OK)
    {
        goto END;
    }

    if (bLen == 2 && buffer[1] == TRUE)
    {
        timeout = REQUEST_TIMER;
        while(timeout)
        {
            if(exe_status == ZW_STATUS_DELETE_RETURN_ROUTE_OK)
            {
                retVal = ZW_STATUS_OK;
                goto END;
            }
            else if(exe_status == ZW_STATUS_DELETE_RETURN_ROUTE_FAIL)
            {
                retVal = ZW_STATUS_FAIL;
                goto END;
            }
            MsecSleep(1);
            timeout--;
            if(timeout == 0)
            {
                retVal = ZW_STATUS_TIMEOUT;
                goto END;
            }
            
        }
        
    }
    else
    {
        retVal = ZW_STATUS_FAIL;
    }

END:
    if(retVal != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] Send request and wait response fail. nodeID[0x%x] ret[%u]\n", nodeID, retVal);
#endif
    }
    exe_status = ZW_STATUS_IDLE;
    pthread_mutex_unlock(&g_cmd_internal_mutex);
    return retVal;
}



