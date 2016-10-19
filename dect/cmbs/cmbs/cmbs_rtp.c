/*!
*   \file           cmbs_rtp.c
*   \brief          CMBS RTP Extension
*   \Author         Denis Matiukha
*
*   @(#)            cmbs_rtp.c~1
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

E_CMBS_RC   cmbs_rtp_SessionStart(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTP_SESSION_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtp_SessionStop(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTP_SESSION_STOP, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtp_SessionUpdate(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTP_SESSION_UPDATE, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtcp_SessionStart(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTCP_SESSION_START, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtcp_SessionStop(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTCP_SESSION_STOP, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtp_SendDTMF(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTP_SEND_DTMF, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtp_EnableFaxAudioProcessingMode(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTP_ENABLE_FAX_AUDIO_PROCESSING_MODE, p_List->pu8_Buffer, p_List->u16_CurSize);
}


E_CMBS_RC   cmbs_rtp_DisableFaxAudioProcessingMode(void * pv_AppRefHandle, void * pv_RefIEList)
{
    PST_CFR_IE_LIST  p_List = (PST_CFR_IE_LIST)pv_RefIEList;

    UNUSED_PARAMETER(pv_AppRefHandle);

    return cmbs_int_EventSend(CMBS_EV_RTP_DISABLE_FAX_AUDIO_PROCESSING_MODE, p_List->pu8_Buffer, p_List->u16_CurSize);
}
