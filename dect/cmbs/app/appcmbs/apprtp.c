/*!
*   \file       apprtp.c
*   \brief      RTP API
*   \author     Denis Matiukha
*
*   @(#)        apprtp.c~1
*
*******************************************************************************/

#include <stdio.h>

#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "cmbs_event.h"
#include "appcmbs.h"
#include "appmsgparser.h"
#include "apprtp.h"


E_CMBS_RC       app_RTPSessionStart(u32 u32_ChannelID, const ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    cmbs_api_ie_RTPSessionInformationAdd(p_RefIEList, pst_RTPSessionInformation);

    //
    // Call CMBS API function.
    //

    return cmbs_rtp_SessionStart(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTPSessionStop(u32 u32_ChannelID)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    //
    // Call CMBS API function.
    //

    return cmbs_rtp_SessionStop(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTPSessionUpdate(u32 u32_ChannelID, const ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    cmbs_api_ie_RTPSessionInformationAdd(p_RefIEList, pst_RTPSessionInformation);

    //
    // Call CMBS API function.
    //

    return cmbs_rtp_SessionUpdate(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTCPSessionStart(u32 u32_ChannelID, u32 u32_RTCPInterval)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    cmbs_api_ie_RTCPIntervalAdd(p_RefIEList, u32_RTCPInterval);

    //
    // Call CMBS API function.
    //

    return cmbs_rtcp_SessionStart(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTCPSessionStop(u32 u32_ChannelID)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    //
    // Call CMBS API function.
    //

    return cmbs_rtcp_SessionStop(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTPSendDTMF(u32 u32_ChannelID, const ST_IE_RTP_DTMF_EVENT * pst_RTPDTMFEvent, const ST_IE_RTP_DTMF_EVENT_INFO * pst_RTPDTMFEventInfo)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    cmbs_api_ie_RTPDTMFEventAdd(p_RefIEList, pst_RTPDTMFEvent);

    cmbs_api_ie_RTPDTMFEventInfoAdd(p_RefIEList, pst_RTPDTMFEventInfo);

    //
    // Call CMBS API function.
    //

    return cmbs_rtp_SendDTMF(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTPEnableFaxAudioProcessingMode(u32 u32_ChannelID)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    //
    // Call CMBS API function.
    //

    return cmbs_rtp_EnableFaxAudioProcessingMode(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

E_CMBS_RC       app_RTPDisableFaxAudioProcessingMode(u32 u32_ChannelID)
{
    PST_CFR_IE_LIST         p_RefIEList;
    ST_IE_MEDIA_CHANNEL     st_MediaChannel;

    //
    // Allocate new IE list.
    //

    p_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    if (p_RefIEList == 0)
    {
        puts("Can not allocate new IE List.\n");
        return CMBS_RC_ERROR_OUT_OF_MEM;
    }

    //
    // Add relevant IEs to the list.
    //

    st_MediaChannel.u32_ChannelID           = u32_ChannelID;
    st_MediaChannel.u32_ChannelParameter    = 0;
    st_MediaChannel.e_Type                  = CMBS_MEDIA_TYPE_RTP;
    cmbs_api_ie_MediaChannelAdd(p_RefIEList, &st_MediaChannel);

    //
    // Call CMBS API function.
    //

    return cmbs_rtp_DisableFaxAudioProcessingMode(g_cmbsappl.pv_CMBSRef, p_RefIEList);
}

int             app_RTPEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    UNUSED_PARAMETER(pv_AppRef);
    UNUSED_PARAMETER(pv_EventData);

    if (e_EventID == CMBS_EV_RTP_SESSION_START_RES  ||
            e_EventID == CMBS_EV_RTP_SESSION_STOP_RES   ||
            e_EventID == CMBS_EV_RTP_SESSION_UPDATE_RES ||
            e_EventID == CMBS_EV_RTCP_SESSION_START_RES ||
            e_EventID == CMBS_EV_RTCP_SESSION_STOP_RES  ||
            e_EventID == CMBS_EV_RTP_SEND_DTMF_RES      ||
            e_EventID == CMBS_EV_RTP_ENABLE_FAX_AUDIO_PROCESSING_MODE_RES   ||
            e_EventID == CMBS_EV_RTP_DISABLE_FAX_AUDIO_PROCESSING_MODE_RES
       )
    {
        if (g_cmbsappl.n_Token)
        {
            appcmbs_ObjectSignal(NULL, 0, 1, e_EventID);
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
