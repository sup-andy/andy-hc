/*!
* \file       cmbs_han.c
* \brief
* \Author  CMBS Team
*
*
*******************************************************************************/
#if defined(__arm)
#include "tclib.h"
#include "embedded.h"
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include "cmbs_int.h"
#include "cmbs_han_ie.h"

E_CMBS_RC cmbs_dsr_han_mngr_Init(void *pv_AppRefHandle, ST_HAN_CONFIG * pst_HANConfig)
{
    PST_CFR_IE_LIST    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    ST_IE_HAN_CONFIG   st_HanCfgIe;

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_HanCfgIe.st_HanCfg = *pst_HANConfig;
    cmbs_api_ie_HanCfgAdd(p_List, &st_HanCfgIe);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MNGR_INIT, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_mngr_Start(void *pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MNGR_START, NULL, 0);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_device_ReadTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8 IsBrief)
{
    PST_CFR_IE_LIST    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries,  CMBS_IE_HAN_NUM_OF_ENTRIES);
    cmbs_api_ie_ShortValueAdd(p_List, u16_IndexOfFirstEntry, CMBS_IE_HAN_INDEX_1ST_ENTRY);
    cmbs_api_ie_ByteValueAdd(p_List, IsBrief ? CMBS_HAN_DEVICE_TABLE_ENTRY_TYPE_BRIEF : CMBS_HAN_DEVICE_TABLE_ENTRY_TYPE_EXTENDED, CMBS_IE_HAN_TABLE_ENTRY_TYPE);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_DEVICE_READ_TABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_device_WriteTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_DEVICE_ENTRY * pst_HANDeviceEntriesArray)
{
    PST_CFR_IE_LIST      p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    UNUSED_PARAMETER(u16_NumOfEntries);
    UNUSED_PARAMETER(pv_AppRefHandle);
    UNUSED_PARAMETER(u16_IndexOfFirstEntry);
    UNUSED_PARAMETER(pst_HANDeviceEntriesArray);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_bind_ReadTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries,  CMBS_IE_HAN_NUM_OF_ENTRIES);
    cmbs_api_ie_ShortValueAdd(p_List, u16_IndexOfFirstEntry, CMBS_IE_HAN_INDEX_1ST_ENTRY);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_BIND_READ_TABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_bind_WriteTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_BIND_ENTRY * pst_HANBindEntriesArray)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    ST_IE_HAN_BIND_ENTRIES stIe_Binds;

    UNUSED_PARAMETER(pv_AppRefHandle);

    stIe_Binds.u16_NumOfEntries  = u16_NumOfEntries;
    stIe_Binds.u16_StartEntryIndex = u16_IndexOfFirstEntry;
    stIe_Binds.pst_BindEntries  = pst_HANBindEntriesArray;
    cmbs_api_ie_HANBindTableAdd(p_List, &stIe_Binds);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_BIND_WRITE_TABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_group_ReadTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry)
{
    PST_CFR_IE_LIST    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries,  CMBS_IE_HAN_NUM_OF_ENTRIES);
    cmbs_api_ie_ShortValueAdd(p_List, u16_IndexOfFirstEntry, CMBS_IE_HAN_INDEX_1ST_ENTRY);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_GROUP_READ_TABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_group_WriteTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_GROUP_ENTRY * pst_HANGroupEntriesArray)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    ST_IE_HAN_GROUP_ENTRIES stIe_Groups;

    UNUSED_PARAMETER(pv_AppRefHandle);

    stIe_Groups.u16_NumOfEntries = u16_NumOfEntries;
    stIe_Groups.u16_StartEntryIndex = u16_IndexOfFirstEntry;
    stIe_Groups.pst_GroupEntries = pst_HANGroupEntriesArray;
    cmbs_api_ie_HANGroupTableAdd(p_List, &stIe_Groups);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE, p_List->pu8_Buffer, p_List->u16_CurSize);

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_msg_RecvRegister(void *pv_AppRefHandle, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_HanMsgRegInfoAdd(p_List, pst_HANMsgRegInfo);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MSG_RECV_REGISTER, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_msg_RecvUnregister(void *pv_AppRefHandle, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_HanMsgRegInfoAdd(p_List, pst_HANMsgRegInfo);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_msg_SendTxRequest(void *pv_AppRefHandle, u16 u16_DeviceId)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_DeviceId, CMBS_IE_HAN_DEVICE);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_msg_SendTxEnd(void *pv_AppRefHandle, u16 u16_DeviceId)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_DeviceId, CMBS_IE_HAN_DEVICE);
    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST, p_List->pu8_Buffer, p_List->u16_CurSize);

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_dsr_han_msg_Send(void *pv_AppRefHandle, u16 u16_RequestId, PST_IE_HAN_MSG_CTL pst_MsgCtrl, ST_IE_HAN_MSG * pst_HANMsg)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_HANMsgAdd(p_List, pst_HANMsg);

    cmbs_api_ie_HANMsgCtlAdd(p_List, pst_MsgCtrl);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_MSG_SEND, p_List->pu8_Buffer, p_List->u16_CurSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_dsr_han_device_Delete(void *pv_AppRefHandle, u16 u16_DeviceId)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_HANDeviceAdd(p_List, u16_DeviceId);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE, p_List->pu8_Buffer, p_List->u16_CurSize);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_dsr_han_device_GetConnectionStatus(void *pv_AppRefHandle, u16 u16_DeviceId)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_HANDeviceAdd(p_List, u16_DeviceId);

    return cmbs_int_EventSend(CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS , p_List->pu8_Buffer, p_List->u16_CurSize);

}

/*---------[End Of File]---------------------------------------------------------------------------------------------------------------------------*/
