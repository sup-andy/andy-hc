/*!
*   \file       apphan.c
*   \brief      HAN API
*   \author     CMBS Team
*
*
*******************************************************************************/

#include <string.h>
#include "cmbs_han.h"
#include "cmbs_han_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appmsgparser.h"
#include "cmbs_api.h"
#include "cmbs_int.h"
#include "cmbs_dbg.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "cfr_debug.h"
#include "hanfun_protocol_defs.h"
#include "cmbs_fifo.h"
#include "ctrl_common_lib.h"

//////////////////////////////////////////////////////////////////////////
extern void app_HanDemoPrintHanMessage(void * pv_IE);
//////////////////////////////////////////////////////////////////////////
void app_HanOnReadDeviceTableRes(void* pv_EventData);
void app_HanOnInitStartRes(void* pv_EventData, E_CMBS_EVENT_ID e_EventID);
void app_HanOnDsrMsgSendRes(void* pv_EventData);

//////////////////////////////////////////////////////////////////////////

void app_HanServerOnRegClosed(E_CMBS_EVENT_ID e_EventID, void * pv_EventData);
void app_HanServerOnReadDeviceTableRes(void* pv_EventData);
int app_HanServerOnMsgRecv(void *pv_EventData);
void app_OnHandsetRegistered(void *pv_List);
int app_HanServerOnTableUpdate(void *pv_EventData);

void app_HanOnFunMsgReceived(void* pv_EventData);
void app_HanOnSendTxReady(void* pv_EventData);
void app_HanOnReadDeviceTableRes(void *pv_EventData);
extern u16 u16_Han_Server;

void app_HanSendDsrMsgFromFifo(u16 u16_deviceId);

////////////////////////////////////////////////////

// Message Queue per ULE device -- definitions

typedef struct        // Msg Struct to be saved in message Queue
{
    ST_IE_HAN_MSG_CTL  st_HANMsgCtl;
    ST_IE_HAN_MSG   st_HANMsg;
    u8       Payload[CMBS_HAN_MAX_MSG_DATA_LEN];
} ST_FUN_MSG, * PST_FUN_MSG;

#define   FUN_MSG_FIFO_SIZE 3  // up to 3 messages per HAN device

ST_FUN_MSG  g_CMBS_UleMsgBuffer[CMBS_HAN_MAX_DEVICES][FUN_MSG_FIFO_SIZE];
ST_CMBS_FIFO g_UleMsgFifo[CMBS_HAN_MAX_DEVICES];

bool   WaitForReady[CMBS_HAN_MAX_DEVICES];

CFR_CMBS_CRITICALSECTION G_g_UleMsgFifoPriCriticalSection[CMBS_HAN_MAX_DEVICES];

//////////////////////////////////////////////////


int app_HANEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    void *         pv_IE = NULL;
    u16            u16_IE;
    E_CMBS_HAN_EVENT_ID e_HanEventId = (E_CMBS_HAN_EVENT_ID)e_EventID;

    UNUSED_PARAMETER(pv_AppRef);

#ifdef HAN_SERVER_DEMO
    if (((E_CMBS_EVENT_ID)e_HanEventId == CMBS_EV_DSR_CORD_CLOSEREG))
    {
        app_HanServerOnRegClosed(e_EventID, pv_EventData);
        return TRUE;
    }
    else if ((u16_Han_Server) && (e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES))
    {
        app_HanServerOnReadDeviceTableRes(pv_EventData);
        return TRUE;
    }
    else if (e_HanEventId == CMBS_EV_DSR_HAN_MSG_RECV)
    {
        app_HanServerOnMsgRecv(pv_EventData);
        app_HanOnFunMsgReceived(pv_EventData);

        return TRUE;
    }

    else if (e_HanEventId == CMBS_EV_DSR_HAN_MSG_SEND_TX_READY)
    {
        app_HanOnSendTxReady(pv_EventData);

        return TRUE;
    }
    else if ((E_CMBS_EVENT_ID)e_HanEventId == CMBS_EV_DSR_HS_REGISTERED)
    {
        app_OnHandsetRegistered(pv_EventData);
        return TRUE;
    }
#else
    if (e_HanEventId == CMBS_EV_DSR_HAN_MSG_RECV)
    {
        app_HanOnFunMsgReceived(pv_EventData);
        return TRUE;
    }
#endif
    if (e_HanEventId == CMBS_EV_DSR_HAN_MSG_SEND_RES)
    {
        app_HanOnDsrMsgSendRes(pv_EventData);
        return TRUE;
    }
    if ((e_HanEventId == CMBS_EV_DSR_HAN_MNGR_INIT_RES) ||
            (e_HanEventId == CMBS_EV_DSR_HAN_MNGR_START_RES))
    {
        app_HanOnInitStartRes(pv_EventData, e_EventID);
        return TRUE;
    }
    if (e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES)
    {
        app_HanOnReadDeviceTableRes(pv_EventData);
        return TRUE;
    }
    if (e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_1_NOTIFICATION  ||
            e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_2_NOTIFICATION  ||
            e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_3_NOTIFICATION)
    {
        app_HanOnRegNotification(pv_EventData, e_EventID);
        return TRUE;
    }
    if (e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE_RES)
    {
        app_HanOnDeleteRes(pv_EventData, e_EventID);
        return TRUE;
    }
    if (e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_REG_DELETED)
    {
        appcmbs_ObjectReport(NULL, 0, 0, e_EventID);
        return TRUE;
    }
    else if (//e_HanEventId == CMBS_EV_DSR_HAN_MNGR_INIT_RES              ||
        //e_HanEventId == CMBS_EV_DSR_HAN_MNGR_START_RES             ||
        e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE_RES     ||
        e_HanEventId == CMBS_EV_DSR_HAN_BIND_READ_TABLE_RES        ||
        e_HanEventId == CMBS_EV_DSR_HAN_BIND_WRITE_TABLE_RES       ||
        e_HanEventId == CMBS_EV_DSR_HAN_GROUP_READ_TABLE_RES       ||
        e_HanEventId == CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE_RES      ||
        e_HanEventId == CMBS_EV_DSR_HAN_MSG_RECV_REGISTER_RES      ||
        e_HanEventId == CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER_RES    ||
        e_HanEventId == CMBS_EV_DSR_HAN_MSG_RECV                   ||
        e_HanEventId == CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST_RES    ||
        e_HanEventId == CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST_RES        ||
        e_HanEventId == CMBS_EV_DSR_HAN_MSG_SEND_TX_ENDED ||
        e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_REG_DELETED ||
        e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS_RES ||
        e_HanEventId == CMBS_EV_DSR_HAN_DEVICE_UNKNOWN_DEV_CONTACT
    )
    {
        cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (u16_IE == CMBS_IE_HAN_MSG)
            {
                app_HanDemoPrintHanMessage(pv_IE);
            }
            else
            {
                app_IEToString(pv_IE, u16_IE);
            }


            cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
        }


        if (g_cmbsappl.n_Token)
        {
            appcmbs_ObjectSignal(NULL, 0, 1, e_EventID);
        }

        return TRUE;
    }

    return FALSE;
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMngrInit(ST_HAN_CONFIG * pst_HANConfig)
{
    return cmbs_dsr_han_mngr_Init(g_cmbsappl.pv_CMBSRef, pst_HANConfig);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMngrStart(void)
{
    return cmbs_dsr_han_mngr_Start(g_cmbsappl.pv_CMBSRef);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanDeviceReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8 isBrief)
{
    return cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, isBrief);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanDeviceReadTableExtended(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry)
{
    return cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, (u8)FALSE);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanDeviceWriteTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_DEVICE_ENTRY * pst_HANDeviceEntriesArray)
{
    return cmbs_dsr_han_device_WriteTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, pst_HANDeviceEntriesArray);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanBindReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry)
{
    return cmbs_dsr_han_bind_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanBindWriteTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_BIND_ENTRY * pst_HANBindEntriesArray)
{
    return cmbs_dsr_han_bind_WriteTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, pst_HANBindEntriesArray);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanGroupReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry)
{
    return cmbs_dsr_han_group_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanGroupWriteTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_GROUP_ENTRY * pst_HANGroupEntriesArray)
{
    return cmbs_dsr_han_group_WriteTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, pst_HANGroupEntriesArray);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMsgRecvRegister(ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo)
{
    return cmbs_dsr_han_msg_RecvRegister(g_cmbsappl.pv_CMBSRef, pst_HANMsgRegInfo);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMsgRecvUnregister(ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo)
{
    return cmbs_dsr_han_msg_RecvUnregister(g_cmbsappl.pv_CMBSRef, pst_HANMsgRegInfo);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMsgSendTxRequest(u16 u16_DeviceId)
{
    return cmbs_dsr_han_msg_SendTxRequest(g_cmbsappl.pv_CMBSRef, u16_DeviceId);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMsgSendTxEnd(u16 u16_DeviceId)
{
    return cmbs_dsr_han_msg_SendTxEnd(g_cmbsappl.pv_CMBSRef, u16_DeviceId);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanMsgSend(u16 u16_RequestId, u16 u16_DestDeviceId, ST_IE_HAN_MSG_CTL* pst_HANMsgCtl , ST_IE_HAN_MSG * pst_HANMsg)
{
    bool  queue_is_empty = TRUE;

    ST_FUN_MSG st_fun_msg;

#ifdef CMBS_DEBUG
    printf(">>>>>>hanServer: callInterface Received from Remote Client \n");
    printf("callInterface ====>:\n %s", pst_HANMsg);
    printf("callInterface <====:\n");
    printf("size of ST_IE_HAN_MSG_CTL= %d  size of ST_IE_HAN_MSG = %d\n", sizeof(ST_IE_HAN_MSG_CTL), sizeof(ST_IE_HAN_MSG));
#endif

    // copy message into local queue type element
    memcpy(&st_fun_msg.st_HANMsgCtl, pst_HANMsgCtl , sizeof(ST_IE_HAN_MSG_CTL));
    memcpy(&st_fun_msg.st_HANMsg, pst_HANMsg , sizeof(ST_IE_HAN_MSG));

    memset(&st_fun_msg.Payload, 0, sizeof(st_fun_msg.Payload));

    if ((pst_HANMsg->u16_DataLen) && (pst_HANMsg->u16_DataLen < CMBS_HAN_MAX_MSG_DATA_LEN))
    {
        memcpy(&st_fun_msg.Payload, pst_HANMsg->pu8_Data, pst_HANMsg->u16_DataLen);
        //printf("Payload exists\n");
    }
    else
    {
        //printf("Payload does not exist\n");
    }

    // Push to device queue and send Tx Reuest. If Queue was not empty (in process), no need to ask again for Tx Request
    queue_is_empty = !((PST_FUN_MSG)cmbs_util_FifoGet(&g_UleMsgFifo[u16_DestDeviceId]));

    if (cmbs_util_FifoPush(&g_UleMsgFifo[u16_DestDeviceId], &st_fun_msg))
    {
        printf("Fun Message Pushed!\n");
        if (queue_is_empty)
        {
            // Start with TxEnd response, to ensure TxRequest will return TxReady (and not be ignored or send Error)
            app_DsrHanMsgSendTxEnd(u16_DestDeviceId);

            app_DsrHanMsgSendTxRequest(u16_DestDeviceId);
            WaitForReady[u16_DestDeviceId] = TRUE;
        }
    }
    else
    {

        CFR_DBG_WARN("\nUle_msg_not InsertToFIFOs FIFO for Device %d is full\n", u16_DestDeviceId);

        // IF a READY response was lost, try here to resend it, to release the queue
        // Start with TxEnd response, to ensure TxRequest will return TxReady (and not be ignored or send Error)
        app_DsrHanMsgSendTxEnd(u16_DestDeviceId);

        app_DsrHanMsgSendTxRequest(u16_DestDeviceId);

        // send error response to Host
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanDeleteDevice(u16 u16_DeviceId)
{
    return cmbs_dsr_han_device_Delete(g_cmbsappl.pv_CMBSRef, u16_DeviceId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_DsrHanGetDeviceConnectionStatus(u16 u16_DeviceId)
{
    return cmbs_dsr_han_device_GetConnectionStatus(g_cmbsappl.pv_CMBSRef, u16_DeviceId);
}


//////////////////////////////////////////////////////////////////////////

void app_HanOnReadDeviceTableRes(void *pv_EventData)
{
    void *pv_IE = NULL;
    u16       u16_IE, u16_Length = 0;
    u8       temp_buffer[2000];
    u8                          *pTmpBufPtr;
    //#define MAX_DEVICES_ARR 5
    u8            u8_Length = 0;
    u8            u8_ArrEntry = 0;
    u8         Units;
    u16        Interfaces;

    printf("\nhanServer: app_HanServerOnReadDeviceTableRes--> \n");
    memset(temp_buffer, 0x0, sizeof(temp_buffer));
    pTmpBufPtr = temp_buffer;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES || u16_IE == CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES)
        {
            if (u16_IE == CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES)
            {
                ST_HAN_BRIEF_DEVICE_INFO  arr_Devices[CMBS_HAN_MAX_DEVICES];
                ST_IE_HAN_BRIEF_DEVICE_ENTRIES st_HANDeviceEntries;
                st_HANDeviceEntries.pst_DeviceEntries = arr_Devices;

                cmbs_api_ie_HANDeviceTableBriefGet(pv_IE, &st_HANDeviceEntries);
                u16_Length = st_HANDeviceEntries.u16_NumOfEntries * sizeof(ST_HAN_DEVICE_ENTRY);
                memcpy(&arr_Devices, (u8 *)st_HANDeviceEntries.pst_DeviceEntries, u16_Length);

                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal(&st_HANDeviceEntries, sizeof(ST_IE_HAN_BRIEF_DEVICE_ENTRIES), 1, CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES);
                }
#if 0
                for (u8_ArrEntry = 0; u8_ArrEntry < st_HANDeviceEntries.u16_NumOfEntries && u8_ArrEntry < NUMBER_OF_REG_DEV_ENTRIES; u8_ArrEntry++)
                {
                    {

                        sprintf((char*)pTmpBufPtr, "DEV_REG_STATUS\r\n%s %d\r\n",
                                g_fieldID[HAN_SERVER_SRC_DEV_ID],
                                arr_Devices[u8_ArrEntry].u16_DeviceId);

                        pTmpBufPtr += strlen((char*)pTmpBufPtr);

                        for (Units = 0; Units < arr_Devices[u8_ArrEntry].u8_NumberOfUnits; Units++)
                        {
                            sprintf((char*)pTmpBufPtr, "%s %d\r\nTYPE: %d\r\n\r\n",
                                    g_fieldID[HAN_SERVER_SRC_UNIT_ID],
                                    arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u8_UnitId,
                                    arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_UnitType);
                            pTmpBufPtr += strlen((char*)pTmpBufPtr);
                        }

                    }
                }
#endif
                //send2InternalSocket(temp_buffer);
            }

            if (u16_IE == CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES)
            {
                ST_HAN_EXTENDED_DEVICE_INFO   arr_Devices[CMBS_HAN_MAX_DEVICES];
                ST_IE_HAN_EXTENDED_DEVICE_ENTRIES  st_HANDeviceEntries;
                st_HANDeviceEntries.pst_DeviceEntries = arr_Devices;

                cmbs_api_ie_HANDeviceTableExtendedGet(pv_IE, &st_HANDeviceEntries);
                u16_Length = st_HANDeviceEntries.u16_NumOfEntries * sizeof(ST_HAN_EXTENDED_DEVICE_INFO);
                memcpy(&arr_Devices, (u8 *)st_HANDeviceEntries.pst_DeviceEntries, u16_Length);

                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal(&st_HANDeviceEntries, sizeof(ST_IE_HAN_EXTENDED_DEVICE_ENTRIES), 1, CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES);
                }

#if 0
                for (u8_ArrEntry = 0; u8_ArrEntry < st_HANDeviceEntries.u16_NumOfEntries && u8_ArrEntry < NUMBER_OF_REG_DEV_ENTRIES; u8_ArrEntry++)
                {
                    sprintf((char*)pTmpBufPtr, "DEV_REG_STATUS\r\n%s %d\r\n",
                            g_fieldID[HAN_SERVER_SRC_DEV_ID],
                            arr_Devices[u8_ArrEntry].u16_DeviceId);

                    pTmpBufPtr += strlen((char*)pTmpBufPtr);

                    sprintf((char*)pTmpBufPtr, "DEV_IPUI\r\n%s %d %d %d %d %d\r\n",
                            g_fieldID[HAN_SERVER_DEVICE_IPUI],
                            arr_Devices[u8_ArrEntry].u8_IPUI[0], arr_Devices[u8_ArrEntry].u8_IPUI[1], arr_Devices[u8_ArrEntry].u8_IPUI[2], arr_Devices[u8_ArrEntry].u8_IPUI[3], arr_Devices[u8_ArrEntry].u8_IPUI[4]);

                    pTmpBufPtr += strlen((char*)pTmpBufPtr);

                    for (Units = 0; Units < arr_Devices[u8_ArrEntry].u8_NumberOfUnits; Units++)
                    {
                        sprintf((char*)pTmpBufPtr, "%s %d\r\nTYPE: %d\r\n\r\n",
                                g_fieldID[HAN_SERVER_SRC_UNIT_ID],
                                arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u8_UnitId,
                                arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_UnitType);
                        pTmpBufPtr += strlen((char*)pTmpBufPtr);

                        for (Interfaces = 0; Interfaces < arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_NumberOfOptionalInterfaces; Interfaces++)
                        {
                            sprintf((char*)pTmpBufPtr, "%s %d\r\nINTERFACE: %d\r\n\r\n",
                                    g_fieldID[HAN_SERVER_DEVICE_UNIT_INTERFACE],
                                    Interfaces,
                                    arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_OptionalInterfaces[Interfaces]);
                            pTmpBufPtr += strlen((char*)pTmpBufPtr);
                        }
                    }
                }
#endif
            }

            //send2InternalSocket(temp_buffer);

        }
        app_IEToString(pv_IE, u16_IE);
        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);

    }
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void app_HanOnInitStartRes(void* pv_EventData, E_CMBS_EVENT_ID e_EventID)
{
    char chRetCode =  app_ResponseCheck(pv_EventData) ? CMBS_RC_ERROR_GENERAL : CMBS_RC_OK;
    appcmbs_ObjectSignal(&chRetCode, 1, 0, e_EventID);
}

void app_HanOnRegNotification(void* pv_EventData, E_CMBS_EVENT_ID e_EventID)
{
    void *pv_IE = NULL;
    u16  u16_IE = 0, u16_deviceId = 0;
    ST_HAN_GENERAL_STATUS status;

    status.u16_Status = 0;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_HAN_DEVICE)   // CMBS_EV_DSR_HAN_MSG_RECV
        {
            cmbs_api_ie_HANDeviceGet(pv_IE, &u16_deviceId);
        }
        else if (u16_IE == CMBS_IE_HAN_GENERAL_STATUS)
        {
            cmbs_api_ie_HANGeneralStatusGet(pv_IE, &status);
            if (status.u16_Status  == CMBS_HAN_GENERAL_STATUS_OK)
            {
                ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, " Status = %s ", "SUCCESS (0)");
            }
            else if (status.u16_Status == CMBS_HAN_GENERAL_STATUS_ERROR)
            {
                ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Status = %s ", "ERROR (1)");
            }
            else
            {
                ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Status = %s(%d) ", "UNKNOWN", status.u16_Status);
            }
        }

        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }

    appcmbs_ObjectReport(&u16_deviceId, sizeof(u16_deviceId), status.u16_Status, e_EventID);
}

void app_HanOnDeleteRes(void* pv_EventData, E_CMBS_EVENT_ID e_EventID)
{
    void *pv_IE = NULL;
    u16  u16_IE = 0, u16_deviceId = 0;
    u16 u16_Reason = 0;
    ST_IE_RESPONSE st_Response;
    char error_reason[64];

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_HAN_DEVICE)   // CMBS_EV_DSR_HAN_MSG_RECV
        {
            cmbs_api_ie_HANDeviceGet(pv_IE, &u16_deviceId);
            printf("\n device id:%d\n", u16_deviceId);
        }
        else if (u16_IE == CMBS_IE_HAN_DEVICE_FORCEFUL_DELETE_ERROR_STATUS_REASON)
        {
            memset(error_reason, 0, sizeof(error_reason));
            cmbs_api_ie_HANForcefulDeRegErrorReasonGet(pv_IE, &u16_Reason);
            switch (u16_Reason)
            {
                case CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_INVALID_ID:
                    //APPCMBS_LOG("DeReg Failure Reason : Ivalid or Unregistered Device ID\n");
                    strncpy(error_reason, "Invalid or Unregistered Device ID", sizeof(error_reason) - 1);
                    break;
                case CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_DEV_IN_MIDDLE_OF_REG:
                    //APPCMBS_LOG("DeReg Failure Reason : Device Id is valid but the device is in the middle of registration\n");
                    strncpy(error_reason, "Device Id is valid but the device is in the middle of registration\n", sizeof(error_reason) - 1);
                    break;
                case CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_BUSY:
                    //APPCMBS_LOG("DeReg Failure Reason : Engine is busy with previous task. Try soon\n");
                    strncpy(error_reason, "Engine is busy with previous task. Try soon\n", sizeof(error_reason) - 1);
                    break;
                default:
                case CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_UNEXPECTED:
                    //APPCMBS_LOG("DeReg Failure Reason : Unknown Error\n");
                    strncpy(error_reason, "Unknown Error\n", sizeof(error_reason) - 1);
                    break;
            }
        }
        else if (u16_IE == CMBS_IE_RESPONSE)
        {
            cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
            printf("\n CMBS_IE_RESPONSE:%d\n", st_Response.e_Response);
        }

        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }

    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal(&u16_Reason, sizeof(u16_Reason), st_Response.e_Response, e_EventID);
    }
}

typedef struct
{
    u8   SizeInBytes;
    int  NumOfElements;
    char*  Name;
} keyb_han_AttributeDefStruct;

#define STRING_WITH_SIZE_ELEMENT 0 // 0 is a special case where the first byte is the size and the rest is a string
#define ONE_BYTE_ELEMENT   1
#define TWO_BYTES_ELEMENT   2
#define FOUR_BYTES_ELEMENT  4

keyb_han_AttributeDefStruct g_DeviceIFAttributes[] =

{
    {ONE_BYTE_ELEMENT,   1, "HF Core Release version "},     // DEV_INF_IF_ATTR_HF_RELEASE_VER
    {ONE_BYTE_ELEMENT,   1, "Profile Release version "},     // DEV_INF_IF_ATTR_PROF_RELEASE_VER
    {ONE_BYTE_ELEMENT,   1, "Interface Release version "},     // DEV_INF_IF_ATTR_IF_RELEASE_VER
    {ONE_BYTE_ELEMENT,   1, "Paging Capabilities "},      // DEV_INF_IF_ATTR_PAGING_CAPS
    {ONE_BYTE_ELEMENT,   4, "Minimum Sleep Time "},       // DEV_INF_IF_ATTR_MIN_SLEEP_TIME
    {ONE_BYTE_ELEMENT,   4, "Actual Response Time (Paging Interval)  "}, // DEV_INF_IF_ATTR_ACT_RESP_TIME
    {STRING_WITH_SIZE_ELEMENT, 1, "Application version "},      // DEV_INF_IF_ATTR_APPL_VER
    {STRING_WITH_SIZE_ELEMENT, 1, "Hardware version "},       // DEV_INF_IF_ATTR_HW_VER
    {ONE_BYTE_ELEMENT,   2, "EMC "},          // DEV_INF_IF_ATTR_EMC
    {ONE_BYTE_ELEMENT,   5, "IPUI "},          // DEV_INF_IF_ATTR_IPUE
    {STRING_WITH_SIZE_ELEMENT, 1, "Manufacture Name "},       // DEV_INF_IF_ATTR_MANUF_NAME
    {STRING_WITH_SIZE_ELEMENT, 1, "Location "},         // DEV_INF_IF_ATTR_LOCATION
    {ONE_BYTE_ELEMENT,   1, "Device Enable "},        // DEV_INF_IF_ATTR_DEV_ENABLE
    {STRING_WITH_SIZE_ELEMENT, 0, "Friendly Name "}        // DEV_INF_IF_ATTR_FRIENDLY_NAME
};

char* g_GetPackResponceDataValues[] =
{
    "OK",      // CMBS_HAN_DEV_INFO_RESP_VAL_OK
    "Fail:Not Supported",  // CMBS_HAN_DEV_INFO_RESP_VAL_FAIL_NOT_SUPPORTED
    "Fail:Unknown Reason",  // CMBS_HAN_DEV_INFO_RESP_VAL_FAIL_UNKNOWN
};

#define KEYB_HAN_GET_PACK_RESP_RESP_VALUE_INDEX  0
#define KEYB_HAN_GET_PACK_RESP_NUM_OF_ATTR_INDEX  1
#define KEYB_HAN_GET_PACK_RESP_ATTR_START_INDEX  2

#define KEYB_HAN_GET_ATTR_RESP_ATTR_START_INDEX  1

int app_msg_parser_Parse_Interface_DeviceInfo_AttrGetResParse(u8* AttrData, u16 AttrId)
{

    u8   u8_AttrVal;
    u16  u16_AttrVal;
    u32  u32_AttrVal;
    int  DataElement;
    int  count = 0;

    printf("\t\t%s : ", g_DeviceIFAttributes[AttrId].Name);

    switch (g_DeviceIFAttributes[AttrId].SizeInBytes)
    {
        case STRING_WITH_SIZE_ELEMENT:
            // First element (size)
            cfr_ie_dser_u8(AttrData + count, &u8_AttrVal);
            count++;
            // Now read the data
            printf("%.*s", u8_AttrVal, AttrData + count);
            count += u8_AttrVal;
            break;
        case ONE_BYTE_ELEMENT:
            for (DataElement = 0;  DataElement <  g_DeviceIFAttributes[AttrId].NumOfElements; DataElement++)
            {
                // we are going to deserialize N elements of u8
                cfr_ie_dser_u8(AttrData + count, &u8_AttrVal);
                printf("%d  ", u8_AttrVal);
                count++;
            }
            break;
        case TWO_BYTES_ELEMENT:
            for (DataElement = 0;  DataElement <  g_DeviceIFAttributes[AttrId].NumOfElements; DataElement++)
            {
                // we are going to deserialize N elements of u16
                cfr_ie_dser_u16(AttrData + count, &u16_AttrVal);
                printf("%d  ", u16_AttrVal);
                count += 2;
            }
            break;
        case FOUR_BYTES_ELEMENT:
            for (DataElement = 0;  DataElement <  g_DeviceIFAttributes[AttrId].NumOfElements; DataElement++)
            {
                // we are going to deserialize N elements of u32
                cfr_ie_dser_u32(AttrData + count, &u32_AttrVal);
                printf("%d  ", u32_AttrVal);
                count += 4;
            }

            break;
        default:
            break;
    }
    printf("\n");
    return count;

}

void app_msg_parser_Parse_Interface_DeviceInfo_AttrGetRes(ST_IE_HAN_MSG* stIe_Msg)
{
    u8*  AttrData;
    u8*  Payload = stIe_Msg->pu8_Data;


    printf("DEVICE INFO GET ATTR RES) === : \n");
    printf("\tDevice Id: %u\n", stIe_Msg->u16_SrcDeviceId);
    printf("\tUnit Id: %u\n", stIe_Msg->u8_SrcUnitId);
    printf("\tInterface Id: %u\n", stIe_Msg->u16_InterfaceId);
    printf("\tAttribute Id: %u\n", stIe_Msg->u8_InterfaceMember);

    printf("\tResponse Value : %s\n", g_GetPackResponceDataValues[Payload[KEYB_HAN_GET_PACK_RESP_RESP_VALUE_INDEX]]);


    AttrData = &Payload[KEYB_HAN_GET_ATTR_RESP_ATTR_START_INDEX];
    AttrData += app_msg_parser_Parse_Interface_DeviceInfo_AttrGetResParse(AttrData,
                stIe_Msg->u8_InterfaceMember - 1);
}

void app_msg_parser_Parse_Interface_DeviceInfo_PackGetRes(ST_IE_HAN_MSG* stIe_Msg)
{
    int  Attr;
    u8*  AttrData;
    u8*  Payload = stIe_Msg->pu8_Data;
    int  NumOfAttributes;
    u8  AttrId;


    printf("DEVICE INFO PACK RES) === : \n");
    printf("\tDevice Id: %u\n", stIe_Msg->u16_SrcDeviceId);
    printf("\tUnit Id: %u\n", stIe_Msg->u8_SrcUnitId);
    printf("\tInterface Id: %u\n", stIe_Msg->u16_InterfaceId);

    printf("\tResponse Value : %s\n", g_GetPackResponceDataValues[Payload[KEYB_HAN_GET_PACK_RESP_RESP_VALUE_INDEX]]);
    printf("\tNumber of Attributes : %u\n", NumOfAttributes = Payload[KEYB_HAN_GET_PACK_RESP_NUM_OF_ATTR_INDEX]);


    AttrData = &Payload[KEYB_HAN_GET_PACK_RESP_ATTR_START_INDEX];
    for (Attr = 0;  Attr < NumOfAttributes; Attr++)
    {
        AttrId = (*AttrData - 1); // Get The AttribId and advance the pointer. One Based
        AttrData++;
        AttrData += app_msg_parser_Parse_Interface_DeviceInfo_AttrGetResParse(AttrData,
                    AttrId);


    }

}

void app_msg_parser_Parse_Interface_AttrRep_Report(ST_IE_HAN_MSG* stIe_Msg)
{
    int Location = 0;
    int NumOfElements;
    int i;
    u16  temp;
    printf("=== ATTRIBUTE REPORT INTERFACE REPORT RECEIVED ===\n");

    if (stIe_Msg->u8_InterfaceMember == CMBS_HAN_ATTR_REPORT_S2C_CMD_ID_PERIODIC_REPORT_NOTIFICATION)
    {
        printf(" Report Type : Periodic \n");
    }
    else if (stIe_Msg->u8_InterfaceMember == CMBS_HAN_ATTR_REPORT_S2C_CMD_ID_EVENT_REPORT_NOTIFICATION)
    {
        printf(" Report Type : Event \n");
    }
    else
    {
        printf(" UNKNOWN REPORT TYPE \n");
    }

    printf("Report ID   (hex): 0x%x \n", stIe_Msg->pu8_Data[Location++]);
    printf("Unit ID     (dec): %d \n", stIe_Msg->pu8_Data[Location++]);
    temp = stIe_Msg->pu8_Data[Location++] << 8;
    printf("Interface ID  (hex): 0x%x \n", ((temp) | stIe_Msg->pu8_Data[Location++]) & 0x7FFF);
    printf("Number of Attributes : %d \n", NumOfElements = stIe_Msg->pu8_Data[Location++]);

    for (i = 0;  i < NumOfElements; i++)
    {
        printf("Element #%d :\n", i);
        printf("Attribute ID (dec) : %d \n", stIe_Msg->pu8_Data[Location++]);
        printf("Type of reporting : ");
        temp = stIe_Msg->pu8_Data[Location++];
        switch (temp)
        {
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_COV:
                printf(" COV \n");
                break;
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_HT:
                printf(" HT \n");
                break;
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_LH:
                printf(" LH \n");
                break;
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_EQUAL:
                printf(" EQUAL \n");
                break;
            default:
                printf(" UNKNOWN - Error \n");
                break;
        }
        temp = (stIe_Msg->pu8_Data[Location++] << 8);
        printf("Attribute Value (hex) : 0x%x \n", temp || stIe_Msg->pu8_Data[Location++]);
    }

}

void app_msg_parser_Parse_Interface_AttrRep_Res(ST_IE_HAN_MSG* stIe_Msg)
{
    printf("=== ATTRIBUTE REPORT INTERFACE RESPONCE ===\n");
    switch (stIe_Msg->u8_InterfaceMember)
    {
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_CREATE_PERIODIC_REP:
            printf("\tOperation : Create Periodic Report\n");
            break;
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_CREATE_EVENT_REP:
            printf("\tOperation : Create Event Report\n");
            break;
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_DELETE_REP:
            printf("\tOperation : Delete Report\n");
            break;
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_PERIODIC_REP:
            printf("\tOperation : Add Entry to Periodic Report\n");
            break;
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_EVENT_REP:
            printf("\tOperation : Add Entry to Event Report\n");
            break;
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_GET_PERIODIC_REP_ENTRIES:
            printf("\tOperation : Get Periodic Report Entries\n");
            break;
        case CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_GET_EVENT_REP_ENTRIES:
            printf("\tOperation : Get Event Report Entries\n");
            break;

        default:
            printf("\tWrong Operation Responce ( u8_InterfaceMember=%d ) field \n", stIe_Msg->u8_InterfaceMember);
            break;
    }
    switch (stIe_Msg->pu8_Data[0])
    {
        case CMBS_HAN_ATTR_REPORT_RESP_VAL_OK:
            printf("\tResponce Value : OK\n");
            printf("\t** REPORT ID : 0x%x **\n", stIe_Msg->pu8_Data[1]);
            break;
        case CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_NOT_AUTHORIZED:
            printf("\tResponce Value : Fail -- Not Autorized \n");
            break;
        case CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_INV_ARG:
            printf("\tResponce Value : Fail -- Invalid Argument\n");
            break;
        case CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_NOT_ENOUGH_RES:
            printf("\tResponce Value : Fail -- Not Enough Resources\n");
            break;
        case CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_UNKNOWN:
            printf("\tResponce Value : Fail -- Unknown\n");
            break;
        default:
            printf("\tResponce Value : Error in field\n");
            break;
    }
}

void app_msg_parser_Parse_DevInfo_Res(ST_IE_HAN_MSG* stIe_Msg)
{

    if ((stIe_Msg->e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_SET_RES) ||
            (stIe_Msg->e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_GET_RES) ||
            (stIe_Msg->e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_GET_PACK_RES) ||
            (stIe_Msg->e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_RES))
    {

        switch (stIe_Msg->e_MsgType)
        {
            case CMBS_HAN_MSG_TYPE_ATTR_SET_RES:
                printf("\n=== DEVICE INTERFACE ATTRIBUTE SET RESPONSE (");
                break;
            case CMBS_HAN_MSG_TYPE_ATTR_GET_RES:
                printf("\n=== DEVICE INTERFACE ATTRIBUTE GET RESPONSE (");
                app_msg_parser_Parse_Interface_DeviceInfo_AttrGetRes(stIe_Msg);
                break;
            case CMBS_HAN_MSG_TYPE_ATTR_GET_PACK_RES:
                printf("\n=== DEVICE INTERFACE ATTRIBUTE GET PACK RESPONSE (");
                app_msg_parser_Parse_Interface_DeviceInfo_PackGetRes(stIe_Msg);
                break;
            case CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_RES:
                printf("\n=== DEVICE INTERFACE ATTRIBUTE SET PACK RESPONSE (");
                break;
            default:
                printf("\n=== DEVICE INTERFACE ERROR IN RES TYPE FIELD ( e_MsgType )\n");
                break;
        }


    }

}
void app_HanOnFunMsgReceived(void* pv_EventData)
{
    void *pv_IE = NULL;
    u16  u16_IE, u16_deviceId = 0;

    u8 u8_Buffer[CMBS_HAN_MAX_MSG_LEN * 2];
    ST_IE_HAN_MSG stIe_Msg;


    stIe_Msg.pu8_Data = u8_Buffer;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_HAN_MSG)   // CMBS_EV_DSR_HAN_MSG_RECV
        {
            //app_HanDemoPrintHanMessage(pv_IE);
            cmbs_api_ie_HANMsgGet(pv_IE, &stIe_Msg);
            u16_deviceId = stIe_Msg.u16_SrcDeviceId;

            switch (stIe_Msg.u16_InterfaceId)
            {
                case ATTRIBUTE_REPORTING_INTERFACE_ID:
                    if (stIe_Msg.u8_InterfaceType == 0)   // client
                    {
                        app_msg_parser_Parse_Interface_AttrRep_Report(&stIe_Msg);
                    }
                    else
                    {
                        app_msg_parser_Parse_Interface_AttrRep_Res(&stIe_Msg);
                    }
                    break;
                case DEVICE_INFORMATION_INTERFACE_ID:
                    app_msg_parser_Parse_DevInfo_Res(&stIe_Msg);
                    break;
                default:

                    break;
            }

            printf("Raw Data : \n\n");
            app_IEToString(pv_IE, u16_IE);

        }
        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }

    // If we are waiting for TxReady and device had sent a FUN message, try to send TxReady again

    if ((u16_deviceId < CMBS_HAN_MAX_DEVICES) && (WaitForReady[u16_deviceId]))
    {
        app_DsrHanMsgSendTxRequest(u16_deviceId);
    }
}

void app_HanOnSendTxReady(void* pv_EventData)
{
    void * pv_IE = NULL;
    u16  u16_IE, u16_deviceId;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    cmbs_api_ie_HANDeviceGet(pv_IE, &u16_deviceId);

    WaitForReady[u16_deviceId] = FALSE;

    app_HanSendDsrMsgFromFifo(u16_deviceId);

}

void app_HanOnDsrMsgSendRes(void* pv_EventData)
{
    void * pv_IE = NULL;
    u16  u16_IE, u16_deviceId;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    cmbs_api_ie_HANDeviceGet(pv_IE, &u16_deviceId);

    // Previous message was sent, check if another message waiting in queue
    app_HanSendDsrMsgFromFifo(u16_deviceId);
}

void app_HanSendDsrMsgFromFifo(u16 u16_deviceId)
{

    u16  u16_RequestId;

    u8  u8_Buffer[CMBS_HAN_MAX_MSG_DATA_LEN]; // buffer for payload

    ST_IE_HAN_MSG   st_HANMsg_temp;
    ST_IE_HAN_MSG_CTL  st_HANMsgCtl_temp = { 0, 0, 0 };

    PST_FUN_MSG pst_fun_msg;

    st_HANMsg_temp.pu8_Data = u8_Buffer;

    // Since we can send only one message per device, we use its Device ID as the cookie for the message.
    // In future, we may add the request ID as part of the data saved in queue per message

    u16_RequestId = u16_deviceId;

    // Pop a message from FIFO of the device id

    pst_fun_msg = (PST_FUN_MSG)cmbs_util_FifoPop(&g_UleMsgFifo[u16_deviceId]);

    if (pst_fun_msg)
    {
        memcpy(&st_HANMsgCtl_temp, &(pst_fun_msg->st_HANMsgCtl) , sizeof(ST_IE_HAN_MSG_CTL));
        memcpy(&st_HANMsg_temp, &(pst_fun_msg->st_HANMsg) , sizeof(ST_IE_HAN_MSG));
        if ((st_HANMsg_temp.u16_DataLen) && (st_HANMsg_temp.u16_DataLen < CMBS_HAN_MAX_MSG_DATA_LEN))
        {
            memcpy(st_HANMsg_temp.pu8_Data, pst_fun_msg->Payload, st_HANMsg_temp.u16_DataLen);
        }

        // Send to device

        cmbs_dsr_han_msg_Send(g_cmbsappl.pv_CMBSRef, u16_RequestId, &st_HANMsgCtl_temp, &st_HANMsg_temp);
    }
    else
    {
        //printf("ULE Device queue is empty\n");
    }

}

void     han_applUleMsgFiFoInitialize()
{
    u8 u8_Index;

    for (u8_Index = 0; u8_Index < CMBS_HAN_MAX_DEVICES; ++u8_Index)
    {
        cmbs_util_FifoInit(&g_UleMsgFifo[u8_Index],
                           g_CMBS_UleMsgBuffer[u8_Index],
                           sizeof(ST_FUN_MSG),
                           FUN_MSG_FIFO_SIZE,
                           G_g_UleMsgFifoPriCriticalSection[u8_Index]);

        WaitForReady[u8_Index] = FALSE;
    }
}

