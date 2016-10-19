/*!
* \file   cmbs_dsr.c
* \brief
* \Author  stein
*
* @(#) %filespec: cmbs_dsr.c~17.1.27.1.1 %
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

#include "cmbs_util.h"
#include "cmbs_int.h"
#include "cfr_debug.h"

/*****************************************************************************
* DSR (service) functions
*****************************************************************************/

E_CMBS_RC cmbs_dsr_sys_Start(void * pv_AppRefHandle, ST_IE_SYPO_SPECIFICATION* p_SYPOSpecification)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    if (p_SYPOSpecification != NULL)
    {
        cmbs_api_ie_SYPOSpecificationAdd((void*)p_List, p_SYPOSpecification);
    }

    return cmbs_int_EventSend(CMBS_EV_DSR_SYS_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_sys_Reset(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_SYS_RESET, NULL, 0);
}

E_CMBS_RC cmbs_dsr_sys_SendRawMsg(void * pv_AppRefHandle, u8 u8_Process, u8 u8_Instance,
                                  u8 u8_Event, u16 u16_Info, u8 u16_DataLen, u8 *pu8_Data)
{
    UNUSED_PARAMETER(pv_AppRefHandle);
    UNUSED_PARAMETER(u8_Process);
    UNUSED_PARAMETER(u8_Instance);
    UNUSED_PARAMETER(u8_Event);
    UNUSED_PARAMETER(u16_Info);
    UNUSED_PARAMETER(u16_DataLen);
    UNUSED_PARAMETER(pu8_Data);

    /// \todo: implement cmbs_dsr_sys_SendRawMsg
    CFR_DBG_OUT("cmbs_dsr_sys_SendRawMsg NOT implemented\n");

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_dsr_param_Get(void * pv_AppRefHandle, E_CMBS_PARAM_TYPE e_ParamType, E_CMBS_PARAM e_Param)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_PARAMETER st_Param;

    UNUSED_PARAMETER(pv_AppRefHandle);

    // parameter validation
    if (e_ParamType != CMBS_PARAM_TYPE_EEPROM && e_ParamType != CMBS_PARAM_TYPE_PRODUCTION)
        return CMBS_RC_ERROR_PARAMETER;

    memset(&st_Param, 0, sizeof(st_Param));

    st_Param.e_ParamType = e_ParamType;
    st_Param.e_Param     = e_Param;
    st_Param.pu8_Data    = NULL;
    st_Param.u16_DataLen = 0;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ParameterAdd((void*)p_List, &st_Param);

    return cmbs_int_EventSend(CMBS_EV_DSR_PARAM_GET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_param_Set(void * pv_AppRefHandle, E_CMBS_PARAM_TYPE e_ParamType, E_CMBS_PARAM e_Param, u8 * pu8_Data, u16 u16_DataLen)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_PARAMETER st_Param;
    u8    u8_Ret;

    UNUSED_PARAMETER(pv_AppRefHandle);

    // parameter validation
    if (e_ParamType != CMBS_PARAM_TYPE_EEPROM && e_ParamType != CMBS_PARAM_TYPE_PRODUCTION)
        return CMBS_RC_ERROR_PARAMETER;

    // check if param is valid
    if ((u8_Ret = cmbs_util_ParameterValid(e_Param, u16_DataLen)) != CMBS_RC_OK)
        return u8_Ret;

    // Add parameter to IE list
    st_Param.e_ParamType = e_ParamType;
    st_Param.e_Param     = e_Param;
    st_Param.pu8_Data    = pu8_Data;
    st_Param.u16_DataLen = u16_DataLen;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ParameterAdd((void*)p_List, &st_Param);

    // Send Parameter Set request
    return cmbs_int_EventSend(CMBS_EV_DSR_PARAM_SET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_param_area_Get(void * pv_AppRefHandle, E_CMBS_PARAM_AREA_TYPE e_AreaType,
                                  u32 u32_Offset, u16 u16_DataLen)
{
    PST_CFR_IE_LIST   p_List;
    ST_IE_PARAMETER_AREA st_ParamArea;

    UNUSED_PARAMETER(pv_AppRefHandle);

    // parameter validation
    if (e_AreaType != CMBS_PARAM_AREA_TYPE_EEPROM || u16_DataLen > CMBS_PARAM_AREA_MAX_SIZE)
    {
        // currently only area type EEPROM supported
        // maximum length is 512 bytes
        return CMBS_RC_ERROR_PARAMETER;
    }

    memset(&st_ParamArea, 0, sizeof(st_ParamArea));

    st_ParamArea.e_AreaType  = e_AreaType;
    st_ParamArea.u32_Offset  = u32_Offset;
    st_ParamArea.u16_DataLen = u16_DataLen;
    st_ParamArea.pu8_Data    = NULL;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ParameterAreaAdd((void*)p_List, &st_ParamArea);

    return cmbs_int_EventSend(CMBS_EV_DSR_PARAM_AREA_GET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_param_area_Set(void * pv_AppRefHandle, E_CMBS_PARAM_AREA_TYPE e_AreaType, u32 u32_Offset, u8* pu8_Data, u16 u16_DataLen)
{
    PST_CFR_IE_LIST   p_List;
    ST_IE_PARAMETER_AREA st_ParamArea;

    UNUSED_PARAMETER(pv_AppRefHandle);

    // parameter validation
    if (e_AreaType != CMBS_PARAM_AREA_TYPE_EEPROM || u16_DataLen > CMBS_PARAM_AREA_MAX_SIZE)
    {
        // currently only area type EEPROM supported
        // maximum length is 512 bytes
        return CMBS_RC_ERROR_PARAMETER;
    }

    // Add parameter to IE list
    st_ParamArea.e_AreaType  = e_AreaType;
    st_ParamArea.u32_Offset  = u32_Offset;
    st_ParamArea.u16_DataLen = u16_DataLen;
    st_ParamArea.pu8_Data    = pu8_Data;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ParameterAreaAdd((void*)p_List, &st_ParamArea);

    // Send Parameter Area Set request
    return cmbs_int_EventSend(CMBS_EV_DSR_PARAM_AREA_SET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_cord_OpenRegistration(void * pv_AppRefHandle, u32 u32_timeout)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_IntValueAdd((void*)p_List, u32_timeout);

    return cmbs_int_EventSend(CMBS_EV_DSR_CORD_OPENREG, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_cord_CloseRegistration(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_CORD_CLOSEREG, NULL, 0);
}

E_CMBS_RC cmbs_dsr_cord_DisableEncryption(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_ENCRYPT_DISABLE, NULL, 0);
}

E_CMBS_RC cmbs_dsr_cord_EnableEncryption(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_ENCRYPT_ENABLE, NULL, 0);
}

E_CMBS_RC cmbs_dsr_FixedCarrierSet(void * pv_AppRefHandle, u8 u8_Carrier)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_IntValueAdd((void*)p_List, (u32)u8_Carrier);

    return cmbs_int_EventSend(CMBS_EV_DSR_FIXED_CARRIER, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_hs_Page(void * pv_AppRefHandle, u16 u16_Handsets)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_IntValueAdd((void*)p_List, (u32)u16_Handsets);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_PAGE_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC cmbs_dsr_hs_StopPaging(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);
    return cmbs_int_EventSend(CMBS_EV_DSR_HS_PAGE_STOP, NULL, 0);
}

E_CMBS_RC cmbs_dsr_HsLocProgress(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_PAGE_PROGRESS, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_HsLocAnswer(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_PAGE_ANSWER, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_HsLocRelease(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_PAGE_STOP, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_hs_Range(void * pv_AppRefHandle, u8 u8_Hs, E_CMBS_HS_RANGE_STATUS e_Status)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_IntValueAdd((void*)p_List, (u32)u8_Hs);
    //cmbs_api_ie_IntValueAdd( (void*)p_List, (u32)e_Status );

    if (e_Status == CMBS_HS_RANGE_STATUS_IN_RANGE)
    {
        return cmbs_int_EventSend(CMBS_EV_DSR_HS_IN_RANGE, p_List->pu8_Buffer, p_List->u16_CurSize);
    }

    return CMBS_RC_ERROR_NOT_SUPPORTED;
}

E_CMBS_RC cmbs_dsr_hs_Delete(void * pv_AppRefHandle, u16 u16_Handsets)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_IntValueAdd((void*)p_List, (u32)u16_Handsets);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_DELETE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_fw_UpdateStart(void * pv_AppRefHandle, u8* pu8_Data, u16 u16_DataLen)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_FW_UPD_START, pu8_Data, u16_DataLen);
}

E_CMBS_RC cmbs_dsr_fw_UpdatePacketNext(void * pv_AppRefHandle, u8* pu8_Data, u16 u16_DataLen)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_FW_UPD_PACKETNEXT, pu8_Data, u16_DataLen);
}

E_CMBS_RC cmbs_dsr_fw_UpdateEnd(void * pv_AppRefHandle, u8* pu8_Data, u16 u16_DataLen)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_FW_UPD_END, pu8_Data, u16_DataLen);
}

E_CMBS_RC cmbs_dsr_fw_VersionGet(void * pv_AppRefHandle, E_CMBS_MODULE e_FwModule)
{
    PST_CFR_IE_LIST  p_List;
    ST_IE_FW_VERSION st_FwVersion;

    UNUSED_PARAMETER(pv_AppRefHandle);

    memset(&st_FwVersion, 0, sizeof(ST_IE_FW_VERSION));
    st_FwVersion.e_SwModule = e_FwModule;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_FwVersionAdd((void*)p_List, &st_FwVersion);

    return cmbs_int_EventSend(CMBS_EV_DSR_FW_VERSION_GET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_hw_VersionGet(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_HW_VERSION_GET, NULL, 0);
}

E_CMBS_RC cmbs_dsr_EEPROM_VersionGet(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_EEPROM_VERSION_GET, NULL, 0);
}

E_CMBS_RC cmbs_dsr_RxtunGpioConnect(void * pv_AppRefHandle, u16 u16_GPIO)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_GpioAdd((void*)p_List, u16_GPIO);

    // Send Parameter Set request
    return cmbs_int_EventSend(CMBS_EV_DSR_GPIO_CONNECT, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_RxtunGpioDisconnect(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    // Send Parameter Set request
    return cmbs_int_EventSend(CMBS_EV_DSR_GPIO_DISCONNECT, NULL, 0);
}

E_CMBS_RC cmbs_dsr_ATETestStart(void * pv_AppRefHandle, ST_IE_ATE_SETTINGS * pst_AteSettings)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    if (pst_AteSettings->u8_Instance > 9 ||
            pst_AteSettings->u8_Slot > 11 ||
            pst_AteSettings->u8_Carrier > 94 ||
            pst_AteSettings->u8_Ant > 1 ||
            pst_AteSettings->u8_Pattern > 4 ||
            pst_AteSettings->u8_NormalPreamble > 1 ||
            pst_AteSettings->u8_PowerLevel > 2 ||       /* 0,1 or 2 */

            pst_AteSettings->e_ATESlotType >= CMBS_ATE_SLOT_TYPE_MAX ||
            pst_AteSettings->e_ATEType >= CMBS_ATE_TYPE_MAX)
    {
        return CMBS_RC_ERROR_PARAMETER;
    }

    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    cmbs_api_ie_ATESettingsAdd((void*)p_List, pst_AteSettings);

    // Send Parameter Set request
    return cmbs_int_EventSend(CMBS_EV_DSR_ATE_TEST_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_ATETestLeave(void * pv_AppRefHandle)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Send Parameter Set request
    return cmbs_int_EventSend(CMBS_EV_DSR_ATE_TEST_LEAVE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_LogBufferStart(void * pv_AppRefHandle)
{
    PST_CFR_IE_LIST  p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    return cmbs_int_EventSend(CMBS_EV_DSR_SYS_LOG_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_LogBufferStop(void * pv_AppRefHandle)
{
    PST_CFR_IE_LIST  p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    return cmbs_int_EventSend(CMBS_EV_DSR_SYS_LOG_STOP, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC         cmbs_dsr_LogBufferRead(void * pv_AppRefHandle)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    return cmbs_int_EventSend(CMBS_EV_DSR_SYS_LOG_REQ, p_List->pu8_Buffer, p_List->u16_CurSize);
}

/*****************************************************************************
* CAT-iq 2.0
*****************************************************************************/

E_CMBS_RC cmbs_dsr_gen_SendMWI(void * pv_AppRefHandle,
                               u16 u16_RequestId,
                               u8 u8_Line,
                               u16 u16_Handsets,
                               u16 u16_NumOfMsg,
                               E_CMBS_MWI_TYPE eType)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_GEN_EVENT st_GenEvent;

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_GenEvent.u8_SubType   = (u8)eType;
    st_GenEvent.u16_MultiPlicity = u16_NumOfMsg;
    st_GenEvent.u8_LineId   = u8_Line;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_GenEventAdd(p_List, &st_GenEvent);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);

    return cmbs_int_EventSend(CMBS_EV_DSR_GEN_SEND_MWI, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_gen_SendMissedCalls(void * pv_AppRefHandle,
                                       u16 u16_RequestId,
                                       u8 u8_Line,
                                       u16 u16_Handsets,
                                       u16 u16_NumOfMsg,
                                       bool bNewMissedCall,
                                       u16 u16_TotalNumOfCalls)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_GEN_EVENT st_GenEvent;

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_GenEvent.u8_SubType   = bNewMissedCall ? 0x01 : 0x02;
    st_GenEvent.u16_MultiPlicity = u16_NumOfMsg;
    st_GenEvent.u8_LineId   = u8_Line;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_GenEventAdd(p_List, &st_GenEvent);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);
    cmbs_api_ie_ShortValueAdd(p_List, u16_TotalNumOfCalls, CMBS_IE_LA_NR_OF_ENTRIES);

    return cmbs_int_EventSend(CMBS_EV_DSR_GEN_SEND_MISSED_CALLS, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC cmbs_dsr_gen_SendListChanged(void * pv_AppRefHandle,
                                       u16 u16_RequestId,
                                       u16 u16_Handsets,
                                       u8  u8_ListId,
                                       u16 u16_NumOfEntries,
                                       u8  u8_Line,
                                       u8  u8_LineSubType)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_GEN_EVENT st_GenEvent;

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_GenEvent.u8_SubType   = u8_ListId;
    st_GenEvent.u16_MultiPlicity = u16_NumOfEntries;
    st_GenEvent.u8_LineId   = u8_Line;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_GenEventAdd(p_List, &st_GenEvent);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);
    cmbs_api_ie_ByteValueAdd(p_List, u8_LineSubType, CMBS_IE_LINE_SUBTYPE);

    return cmbs_int_EventSend(CMBS_EV_DSR_GEN_SEND_LIST_CHANGED, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_gen_SendWebContent(void * pv_AppRefHandle,
                                      u16 u16_RequestId,
                                      u16 u16_Handsets,
                                      u8  u8_NumOfWebCont)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_GEN_EVENT st_GenEvent;

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_GenEvent.u8_SubType   = 0x01; // RSS description
    st_GenEvent.u16_MultiPlicity = u8_NumOfWebCont;
    st_GenEvent.u8_LineId   = 0xFF; // no specific line

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_GenEventAdd(p_List, &st_GenEvent);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);

    return cmbs_int_EventSend(CMBS_EV_DSR_GEN_SEND_WEB_CONTENT, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_gen_SendPropEvent(void * pv_AppRefHandle,
                                     u16 u16_RequestId,
                                     u16 u16_PropEvent,
                                     u8 * pu8_Data,
                                     u8  u8_DataLen,
                                     u16 u16_Handsets)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_PROP_EVENT st_PropEvent;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    st_PropEvent.u16_PropEvent = u16_PropEvent;
    st_PropEvent.u8_DataLen = MIN(u8_DataLen, CMBS_PROP_EVENT_LENGTH);
    memcpy(&st_PropEvent.u8_Data, pu8_Data, st_PropEvent.u8_DataLen);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_PropEventAdd(p_List, &st_PropEvent);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);

    return cmbs_int_EventSend(CMBS_EV_DSR_GEN_SEND_PROP_EVENT, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_hs_DataSessionOpenRes(void * pv_AppRefHandle, u16 u16_SessionId, bool bAccept)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_DATA_SESSION_ID);

    st_Response.e_Response = bAccept ? CMBS_RESPONSE_OK : CMBS_RESPONSE_ERROR;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_DATA_SESSION_OPEN_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_hs_DataSessionOpen(void * pv_AppRefHandle,
                                      ST_DATA_SESSION_TYPE * pst_DataSessionType,
                                      u16 u16_Handsets)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_DataSessionTypeAdd(p_List, pst_DataSessionType);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_DATA_SESSION_OPEN, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC cmbs_dsr_hs_DataSessionClose(void * pv_AppRefHandle, u16 u16_SessionId)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_DATA_SESSION_ID);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_DATA_SESSION_CLOSE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_hs_DataSend(void * pv_AppRefHandle,
                               u16 u16_SessionId,
                               u8 * pu8_Data,
                               u16 u16_DataLen)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_DATA  st_Data;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    st_Data.u16_DataLen = u16_DataLen;
    st_Data.pu8_Data    = pu8_Data;

    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_DATA_SESSION_ID);
    cmbs_api_ie_DataAdd(p_List, &st_Data);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_DATA_SEND, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_time_Update(void * pv_AppRefHandle,
                               u16 u16_RequestId,
                               ST_DATE_TIME * pst_DateTime,
                               u16 u16_Handsets)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_DateTimeAdd(p_List, (PST_IE_DATETIME)pst_DateTime);
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handsets, CMBS_IE_HANDSETS);

    return cmbs_int_EventSend(CMBS_EV_DSR_TIME_UPDATE, p_List->pu8_Buffer, p_List->u16_CurSize);
}



E_CMBS_RC cmbs_dsr_time_Indication(void * pv_AppRefHandle,
                                   ST_DATE_TIME * pst_DateTime,
                                   u16 u16_Handset)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_DateTimeAdd(p_List, pst_DateTime);
    cmbs_api_ie_HandsetsAdd(p_List, u16_Handset);

    return cmbs_int_EventSend(CMBS_EV_DSR_TIME_INDICATION, p_List->pu8_Buffer, p_List->u16_CurSize);
}



E_CMBS_RC cmbs_dsr_la_SessionStartRes(void * pv_AppRefHandle,
                                      u16 u16_SessionId,
                                      u16 u16_NumOfEntries,
                                      ST_LA_FIELDS * pst_LASortFields,
                                      E_CMBS_RESPONSE e_Response,
                                      E_CMBS_LA_START_SESSION_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add LA Nr of Entries IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries, CMBS_IE_LA_NR_OF_ENTRIES);

    // Add Sort Fields IE
    cmbs_api_ie_LAFieldsAdd(p_List, pst_LASortFields, CMBS_IE_LA_SORT_FIELDS);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_SESSION_START_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_la_SessionEndRes(void * pv_AppRefHandle,
                                    u16 u16_SessionId,
                                    E_CMBS_RESPONSE e_Response,
                                    E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);


    return cmbs_int_EventSend(CMBS_EV_DSR_LA_SESSION_END_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_QuerySupEntryFieldsRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        ST_LA_FIELDS * pst_LANonEditFields,
        ST_LA_FIELDS * pst_LAEditFields,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add editable fields
    cmbs_api_ie_LAFieldsAdd(p_List, pst_LAEditFields, CMBS_IE_LA_EDIT_FIELDS);

    // Add non-editable fields
    cmbs_api_ie_LAFieldsAdd(p_List, pst_LANonEditFields, CMBS_IE_LA_CONST_FIELDS);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_QUERY_SUPP_ENTRY_FIELDS_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_SearchEntriesRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        u16 u16_EntryStartIndex,
        u16 u16_NumOfEntries,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST
    p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add start index
    cmbs_api_ie_ShortValueAdd(p_List, u16_EntryStartIndex, CMBS_IE_LA_ENTRY_INDEX);

    // Add field LA Nr of Entries
    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries, CMBS_IE_LA_NR_OF_ENTRIES);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_SEARCH_ENTRIES_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_ReadEntriesRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        u16 u16_EntryStartIndex,
        u16 u16_NumOfEntries,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST
    p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add start index
    cmbs_api_ie_ShortValueAdd(p_List, u16_EntryStartIndex, CMBS_IE_LA_ENTRY_INDEX);

    // Add counter
    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries, CMBS_IE_LA_ENTRY_COUNT);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_READ_ENTRIES_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_EditEntryRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST
    p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_EDIT_ENTRY_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_SaveEntryRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        u16 u16_EntryId,
        u16 u16_EntryIndex,
        u16 u16_NumOfEntries,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST
    p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add entry Id
    cmbs_api_ie_ShortValueAdd(p_List, u16_EntryId, CMBS_IE_LA_ENTRY_ID);

    // Add entry index
    cmbs_api_ie_ShortValueAdd(p_List, u16_EntryIndex, CMBS_IE_LA_ENTRY_INDEX);

    // Add field LA Nr of Entries
    cmbs_api_ie_ShortValueAdd(p_List, u16_NumOfEntries, CMBS_IE_LA_NR_OF_ENTRIES);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_SAVE_ENTRY_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_DeleteEntryRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        u16 u16_TotalNoOfEntries,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST
    p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add LA Entry Count IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_TotalNoOfEntries, CMBS_IE_LA_ENTRY_COUNT);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_DELETE_ENTRY_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_DeleteListRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        E_CMBS_RESPONSE e_Response,
        E_CMBS_LA_NACK_ERRORS e_RejectReason)
{
    PST_CFR_IE_LIST
    p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add reject reason if there was an error
    if (e_Response == CMBS_RESPONSE_ERROR)
    {
        cmbs_api_ie_ByteValueAdd(p_List, (u8)e_RejectReason, CMBS_IE_LA_REJECT_REASON);
    }

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_DELETE_LIST_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_SessionEnd(void * pv_AppRefHandle, u16 u16_SessionId)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_SESSION_END_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_DataPacketSend(void * pv_AppRefHandle,
        u16 u16_SessionId,
        u8 u8_IsLast,
        u8 * pu8_Data,
        u16 u16_DataLen)
{
    ST_IE_DATA     st_Data;
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add LA IsLast IE
    cmbs_api_ie_ByteValueAdd(p_List, u8_IsLast, CMBS_IE_LA_IS_LAST);

    // Add Data IE
    st_Data.u16_DataLen = u16_DataLen;
    st_Data.pu8_Data    = pu8_Data;

    cmbs_api_ie_DataAdd(p_List, &st_Data);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_DATA_PACKET_SEND, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC         cmbs_dsr_la_DataPacketReceivedRes(void * pv_AppRefHandle,
        u16 u16_SessionId,
        E_CMBS_RESPONSE e_Response)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA Session Id IE
    cmbs_api_ie_ShortValueAdd(p_List, u16_SessionId, CMBS_IE_LA_SESSION_ID);

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_DATA_PACKET_RECEIVE_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC cmbs_dsr_la_PropCmdRes(void * pv_AppRefHandle, PST_LA_PROP_CMD pst_Cmd)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add Proprietary command Id IE
    cmbs_api_ie_LAPropCmdAdd(p_List, pst_Cmd);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_PROP_CMD_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_la_AddSupportedList(void * pv_AppRefHandle, u16 u16_ListId)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA List Id IE
    cmbs_api_ie_LAListIdAdd(p_List, u16_ListId);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_la_AddSupportedListRes(void * pv_AppRefHandle, u16 u16_ListId, E_CMBS_RESPONSE e_Response)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE st_Response;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    // Add LA List Id IE
    cmbs_api_ie_LAListIdAdd(p_List, u16_ListId);

    // Add response IE
    st_Response.e_Response = e_Response;
    cmbs_api_ie_ResponseAdd(p_List, &st_Response);

    return cmbs_int_EventSend(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC         cmbs_dsr_sys_PowerOff(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_SYS_POWER_OFF, NULL, 0);
}

E_CMBS_RC         cmbs_dsr_sys_RFSuspend(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_RF_SUSPEND, NULL, 0);
}

E_CMBS_RC         cmbs_dsr_sys_RFResume(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_RF_RESUME, NULL, 0);
}

E_CMBS_RC         cmbs_dsr_sys_TurnOnNEMo(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_TURN_ON_NEMO, NULL, 0);
}

E_CMBS_RC         cmbs_dsr_sys_TurnOffNEMo(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_TURN_OFF_NEMO, NULL, 0);
}

E_CMBS_RC         cmbs_dsr_GET_InternalnameList(void * pv_AppRefHandle, u16 u16_HsMask)
{
    PST_CFR_IE_LIST              p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd((void*)p_List, u16_HsMask, CMBS_IE_HANDSETS);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC         cmbs_dsr_SET_InternalnameList(void * pv_AppRefHandle, ST_IE_SUBSCRIBED_HS_LIST * pst_SubscribedHsList)
{
    PST_CFR_IE_LIST p_List;

    if (!pst_SubscribedHsList)
    {
        return CMBS_RC_ERROR_PARAMETER;
    }

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_SubscribedHSListAdd((void*)p_List, pst_SubscribedHsList);

    return cmbs_int_EventSend(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_SET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC         cmbs_dsr_GET_Line_setting_list(void * pv_AppRefHandle, u16 u16_LineMask)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ShortValueAdd((void*)p_List, u16_LineMask, CMBS_IE_REQUEST_ID);

    return cmbs_int_EventSend(CMBS_EV_DSR_LINE_SETTINGS_LIST_GET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_SET_Line_setting_list(void * pv_AppRefHandle, ST_IE_LINE_SETTINGS_LIST* pst_LineSettingsList)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_LineSettingsListAdd((void*)p_List, pst_LineSettingsList);

    return cmbs_int_EventSend(CMBS_EV_DSR_LINE_SETTINGS_LIST_SET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_DectSettingsList_Get(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_DECT_SETTINGS_LIST_GET, NULL, 0);
}

E_CMBS_RC cmbs_dsr_DectSettingsList_Set(void * pv_AppRefHandle, ST_IE_DECT_SETTINGS_LIST* pst_DectSettings)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_DectSettingsListAdd((void*)p_List, pst_DectSettings);

    return cmbs_int_EventSend(CMBS_EV_DSR_DECT_SETTINGS_LIST_SET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_AddNewExtension(void * pv_AppRefHandle, ST_IE_SUBSCRIBED_HS_LIST * pst_ExtensionData)
{
    PST_CFR_IE_LIST  p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_SubscribedHSListAdd((void*)p_List, pst_ExtensionData);

    return cmbs_int_EventSend(CMBS_EV_DSR_ADD_EXTENSION, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_SetBaseName(void * pv_AppRefHandle, ST_IE_BASE_NAME* pst_BaseName)
{
    PST_CFR_IE_LIST  p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_BaseNameAdd((void*)p_List, pst_BaseName);

    return cmbs_int_EventSend(CMBS_EV_DSR_SET_BASE_NAME, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_GetBaseName(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_GET_BASE_NAME, NULL, 0);
}

E_CMBS_RC cmbs_dsr_GetEepromSize(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_EEPROM_SIZE_GET, NULL, 0);
}

E_CMBS_RC cmbs_dsr_ReconnectResp(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_RECONNECT_RES, NULL, 0);
}

E_CMBS_RC cmbs_dsr_StartDectLogger(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_START_DECT_LOGGER, NULL, 0);
}

E_CMBS_RC cmbs_dsr_StopDectLoggerAndRead(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_STOP_AND_READ_DECT_LOGGER, NULL, 0);
}

E_CMBS_RC cmbs_dsr_DectLoggerDataIndRes(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_DECT_DATA_IND_RES, NULL, 0);
}

E_CMBS_RC cmbs_dsr_dc_SessionStart(void * pv_AppRefHandle, u16 u16_HSNumber, u32 u32_CallInstance)
{
    PST_CFR_IE_LIST  p_List = cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_HandsetsAdd((void*)p_List, u16_HSNumber);
    cmbs_api_ie_CallInstanceAdd((void*)p_List, u32_CallInstance);

    return cmbs_int_EventSend(CMBS_EV_DSR_DC_SESSION_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_dc_SessionStop(void * pv_AppRefHandle, u16 u16_HSNumber, u32 u32_CallInstance)
{
    PST_CFR_IE_LIST  p_List = cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_HandsetsAdd((void*)p_List, u16_HSNumber);
    cmbs_api_ie_CallInstanceAdd((void*)p_List, u32_CallInstance);

    return cmbs_int_EventSend(CMBS_EV_DSR_DC_SESSION_STOP, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_dc_DataSend(void * pv_AppRefHandle, u16 u16_HSNumber, u32 u32_CallInstance, u8 *pu8_IWUData, u8 u8_DataLength)
{
    ST_IE_DATA     st_Data;
    PST_CFR_IE_LIST  p_List = cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_Data.u16_DataLen = u8_DataLength;
    st_Data.pu8_Data    = pu8_IWUData;

    cmbs_api_ie_HandsetsAdd((void*)p_List, u16_HSNumber);
    cmbs_api_ie_CallInstanceAdd((void*)p_List, u32_CallInstance);
    cmbs_api_ie_DataAdd(p_List, &st_Data);

    return cmbs_int_EventSend(CMBS_EV_DSR_DC_DATA_SEND,  p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_dc_SessionStartRes(void * pv_AppRefHandle, u16 u16_HSNumber, u32 u32_CallInstance, bool b_Response)
{
    ST_IE_RESPONSE          st_Response;
    PST_CFR_IE_LIST  p_RespList = cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_Response.e_Response = b_Response;
    cmbs_api_ie_ResponseAdd((void*)p_RespList, &st_Response);
    cmbs_api_ie_HandsetsAdd((void*)p_RespList, u16_HSNumber);
    cmbs_api_ie_CallInstanceAdd((void*)p_RespList, u32_CallInstance);

    return cmbs_int_EventSend(CMBS_EV_DSR_DC_SESSION_START_RES, p_RespList->pu8_Buffer, p_RespList->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_dc_SessionStopRes(void * pv_AppRefHandle, u16 u16_HSNumber, u32 u32_CallInstance, bool b_Response)
{
    ST_IE_RESPONSE          st_Response;
    PST_CFR_IE_LIST  p_RespList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_Response.e_Response = b_Response;
    cmbs_api_ie_ResponseAdd((void*)p_RespList, &st_Response);
    cmbs_api_ie_HandsetsAdd((void*)p_RespList, u16_HSNumber);
    cmbs_api_ie_CallInstanceAdd((void*)p_RespList, u32_CallInstance);

    return cmbs_int_EventSend(CMBS_EV_DSR_DC_SESSION_STOP_RES, p_RespList->pu8_Buffer, p_RespList->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_dc_DataSendRes(void * pv_AppRefHandle, u16 u16_HSNumber, u32 u32_CallInstance,  bool b_Response)
{
    ST_IE_RESPONSE          st_Response;
    PST_CFR_IE_LIST  p_RespList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    st_Response.e_Response = b_Response;
    cmbs_api_ie_ResponseAdd((void*)p_RespList, &st_Response);
    cmbs_api_ie_HandsetsAdd((void*)p_RespList, u16_HSNumber);
    cmbs_api_ie_CallInstanceAdd((void*)p_RespList, u32_CallInstance);

    return cmbs_int_EventSend(CMBS_EV_DSR_DC_DATA_SEND_RES, p_RespList->pu8_Buffer, p_RespList->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_Ping(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_PING, NULL, 0);
}

E_CMBS_RC cmbs_dsr_fw_AppInvalidate(void * pv_AppRefHandle)
{
    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_DSR_FW_APP_INVALIDATE, NULL, 0);
}

E_CMBS_RC cmbs_dsr_AFE_EndpointConnect(void * pv_AppRefHandle, ST_IE_AFE_ENDPOINTS_CONNECT * pst_AFEEndpointsConnect)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_AFE_EndpointConnectionAdd((void*)p_List, pst_AFEEndpointsConnect);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_ENDPOINT_CONNECT, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_AFE_EndpointEnable(void * pv_AppRefHandle, ST_IE_AFE_ENDPOINT * pst_AEEndpoint)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_AFE_EndpointAdd((void*)p_List, pst_AEEndpoint);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_ENDPOINT_ENABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC cmbs_dsr_AFE_EndpointDisable(void * pv_AppRefHandle, ST_IE_AFE_ENDPOINT * pst_AEEndpoint)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_AFE_EndpointAdd((void*)p_List, pst_AEEndpoint);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_ENDPOINT_DISABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_AFE_SetEndpointGain(void * pv_AppRefHandle, ST_IE_AFE_ENDPOINT_GAIN * pst_AFEEndpointGain)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_AFE_EndpointGainAdd((void*)p_List, pst_AFEEndpointGain);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_ENDPOINT_GAIN, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC cmbs_dsr_AFE_AUXMeasurement(void * pv_AppRefHandle, ST_IE_AFE_AUX_MEASUREMENT_SETTINGS * pst_AUXMeasurementSettings)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_AFE_AUXMeasureSettingsAdd((void*)p_List, pst_AUXMeasurementSettings);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_AUX_MEASUREMENT, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_AFE_ChannelAllocate(void * pv_AppRefHandle, u8 u8_ResourceType)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_AFE_ResourceTypeAdd(p_List, u8_ResourceType);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_AFE_ChannelDeallocate(void * pv_AppRefHandle, ST_IE_MEDIA_CHANNEL * pst_MediaChannel)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_MediaChannelAdd(p_List, pst_MediaChannel);

    return cmbs_int_EventSend(CMBS_EV_DSR_AFE_CHANNEL_DEALLOCATE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_DHSGValueSend(void * pv_AppRefHandle, u8 u8_DHSGValue)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_DHSGValueAdd(p_List, u8_DHSGValue);
    return cmbs_int_EventSend(CMBS_EV_DSR_DHSG_SEND_BYTE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_DHSGNewValueRcv(void * pv_AppRefHandle, u8 u8_DHSGValue)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_DHSGValueAdd(p_List, u8_DHSGValue);
    return cmbs_int_EventSend(CMBS_EV_DSR_DHSG_NEW_DATA_RCV, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_GPIOEnable(void * pv_AppRefHandle, PST_IE_GPIO_ID st_GPIOId)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_GPIOIDAdd(p_List, st_GPIOId);
    return cmbs_int_EventSend(CMBS_EV_DSR_GPIO_ENABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_GPIODisable(void * pv_AppRefHandle, PST_IE_GPIO_ID st_GPIOId)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_GPIOIDAdd(p_List, st_GPIOId);
    return cmbs_int_EventSend(CMBS_EV_DSR_GPIO_DISABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_GPIOConfigSet(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);
    return cmbs_int_EventSend(CMBS_EV_DSR_GPIO_CONFIG_SET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_GPIOConfigGet(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);
    return cmbs_int_EventSend(CMBS_EV_DSR_GPIO_CONFIG_GET, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_ExtIntConfigure(void * pv_AppRefHandle, PST_IE_GPIO_ID st_GPIOId, PST_IE_INT_CONFIGURATION st_Configuration, u8 u8_IntNumber)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_GPIOIDAdd(p_List, st_GPIOId);
    cmbs_api_ie_ExtIntConfigurationAdd(p_List, st_Configuration);
    cmbs_api_ie_ExtIntNumAdd(p_List, u8_IntNumber);
    return cmbs_int_EventSend(CMBS_EV_DSR_EXT_INT_CONFIG, p_List->pu8_Buffer, p_List->u16_CurSize);
}
E_CMBS_RC cmbs_dsr_ExtIntEnable(void * pv_AppRefHandle, u8 u8_IntNumber)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_ExtIntNumAdd(p_List, u8_IntNumber);
    return cmbs_int_EventSend(CMBS_EV_DSR_EXT_INT_ENABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
E_CMBS_RC cmbs_dsr_ExtIntDisable(void * pv_AppRefHandle, u8 u8_IntNumber)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_ExtIntNumAdd(p_List, u8_IntNumber);
    return cmbs_int_EventSend(CMBS_EV_DSR_EXT_INT_DISABLE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
E_CMBS_RC cmbs_dsr_ExtIntIndication(void * pv_AppRefHandle, u8 u8_IntNumber)
{
    PST_CFR_IE_LIST p_List;
    UNUSED_PARAMETER(pv_AppRefHandle);
    ALLOCATE_IE_LIST(p_List);
    cmbs_api_ie_ExtIntNumAdd(p_List, u8_IntNumber);
    return cmbs_int_EventSend(CMBS_EV_DSR_EXT_INT_INDICATION, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_LocateSuggestReq(void * pv_AppRefHandle, u16 u16_Handsets)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_IntValueAdd((void*)p_List, (u32)u16_Handsets);

    return cmbs_int_EventSend(CMBS_EV_DSR_LOCATE_SUGGEST_REQ, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_TerminalCapabilitiesInd(void * pv_AppRefHandle, PST_IE_TERMINAL_CAPABILITIES pst_TermCapability, u8 u8_HSNum)
{
    PST_CFR_IE_LIST p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_HsNumberAdd((void*)p_List, u8_HSNum);
    cmbs_api_ie_TerminalCapabilitiesAdd((void*)p_List, pst_TermCapability);

    return cmbs_int_EventSend(CMBS_EV_DSR_TERMINAL_CAPABILITIES_IND, p_List->pu8_Buffer, p_List->u16_CurSize);
}
//*/
