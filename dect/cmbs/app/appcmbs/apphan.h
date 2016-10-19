/*!
*   \file       apphan.h
*   \brief      HAN API
*   \author     CMBS Team
*
*
*******************************************************************************/

#if !defined( _APPHAN_H )
#define _APPHAN_H

#include "cmbs_han.h"
#include "hanfun_protocol_defs.h"

int app_HANEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData);
E_CMBS_RC app_DsrHanMngrInit(ST_HAN_CONFIG * pst_HANConfig);
E_CMBS_RC app_DsrHanMngrStart(void);
E_CMBS_RC app_DsrHanDeviceReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8 isBrief);
E_CMBS_RC app_DsrHanDeviceWriteTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_DEVICE_ENTRY * pst_HANDeviceEntriesArray);
E_CMBS_RC app_DsrHanBindReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);
E_CMBS_RC app_DsrHanBindWriteTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_BIND_ENTRY * pst_HANBindEntriesArray);
E_CMBS_RC app_DsrHanGroupReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);
E_CMBS_RC app_DsrHanGroupWriteTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_GROUP_ENTRY * pst_HANGroupEntriesArray);
E_CMBS_RC app_DsrHanMsgRecvRegister(ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);
E_CMBS_RC app_DsrHanMsgRecvUnregister(ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);
E_CMBS_RC app_DsrHanMsgSendTxRequest(u16 u16_DeviceId);
E_CMBS_RC app_DsrHanMsgSendTxEnd(u16 u16_DeviceId);
E_CMBS_RC app_DsrHanMsgSend(u16 u16_RequestId, u16 u16_DestDeviceId, ST_IE_HAN_MSG_CTL* pst_HANMsgCtl , ST_IE_HAN_MSG * pst_HANMsg);
E_CMBS_RC app_DsrHanDeleteDevice(u16 u16_DeviceId);
E_CMBS_RC app_DsrHanGetDeviceConnectionStatus(u16 u16_DeviceId);

#endif // _APPHAN_H

/**********************[End Of File]**********************************************************************************************************/
