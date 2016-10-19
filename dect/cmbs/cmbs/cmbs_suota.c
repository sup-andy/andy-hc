/*!
* \file   cmbs_suota.c
* \brief
* \Author  stein
*
* @(#) %filespec: cmbs_suota.c~7 %
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

E_CMBS_RC cmbs_dsr_suota_SendHSVersionAvail(void *pv_AppRefHandle, ST_SUOTA_UPGRADE_DETAILS  st_HSVerAvail, u16 u16_Handset, ST_VERSION_BUFFER* pst_SwVersion, u16 u16_RequestId)
{
    PST_CFR_IE_LIST    p_List;
    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_Handset, CMBS_IE_HANDSETS);
    cmbs_api_ie_VersionAvailAdd(p_List, &st_HSVerAvail);
    cmbs_api_ie_VersionBufferAdd(p_List, pst_SwVersion);
    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);

    return cmbs_int_EventSend(CMBS_EV_DSR_SUOTA_SEND_VERS_AVAIL, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_suota_SendSWUpdateInd(void *pv_AppRefHandle, u16 u16_Handset, E_SUOTA_SU_SubType enSubType, u16 u16_RequestId)
{
    PST_CFR_IE_LIST    p_List;

    UNUSED_PARAMETER(pv_AppRefHandle);

    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    cmbs_api_ie_ShortValueAdd(p_List, u16_Handset, CMBS_IE_HANDSETS);
    cmbs_api_ie_ByteValueAdd(p_List, enSubType, CMBS_IE_SU_SUBTYPE);
    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    return cmbs_int_EventSend(CMBS_EV_DSR_SUOTA_SEND_SW_UPD_IND, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_suota_SendURL(void *pv_AppRefHandle, u16 u16_Handset, u8   u8_URLToFollow, ST_URL_BUFFER* pst_Url, u16 u16_RequestId)
{
    PST_CFR_IE_LIST    p_List;
    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_Handset, CMBS_IE_HANDSETS);
    cmbs_api_ie_ByteValueAdd(p_List, u8_URLToFollow, CMBS_IE_NUM_OF_URLS);
    cmbs_api_ie_UrlAdd(p_List, pst_Url);
    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);

    return cmbs_int_EventSend(CMBS_EV_DSR_SUOTA_SEND_URL, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_suota_SendNack(void *pv_AppRefHandle, u16 u16_Handset, E_SUOTA_RejectReason RejectReason, u16 u16_RequestId)
{
    PST_CFR_IE_LIST    p_List;
    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_Handset, CMBS_IE_HANDSETS);
    cmbs_api_ie_ByteValueAdd(p_List, RejectReason, CMBS_IE_REJECT_REASON);
    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);

    return cmbs_int_EventSend(CMBS_EV_DSR_SUOTA_SEND_NACK, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC cmbs_dsr_suota_DataSend(void *pv_AppRefHandle, u32 u32_appId, u32 u32_SessionId,
                                  char *pSdu, u32 u32_SduLength, u16 u16_RequestId)
{
    PST_CFR_IE_LIST     p_List;
    ST_IE_DATA       st_Data;
    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    UNUSED_PARAMETER(pv_AppRefHandle);

    cmbs_api_ie_ShortValueAdd(p_List, u16_RequestId, CMBS_IE_REQUEST_ID);
    cmbs_api_ie_u32ValueAdd(p_List, u32_appId, CMBS_IE_SUOTA_APP_ID);
    cmbs_api_ie_u32ValueAdd(p_List, u32_SessionId, CMBS_IE_SUOTA_SESSION_ID);
    st_Data.u16_DataLen = u32_SduLength;
    st_Data.pu8_Data    = (u8*)pSdu;
    cmbs_api_ie_DataAdd(p_List, &st_Data);
    return cmbs_int_EventSend(CMBS_EV_DSR_SUOTA_DATA_SEND, p_List->pu8_Buffer, p_List->u16_CurSize);
}


/*---------[End Of File]---------------------------------------------------------------------------------------------------------------------------*/
