/*!
*   \file       apprtp.h
*   \brief      RTP API
*   \author     Denis Matiukha
*
*   @(#)        apprtp.h~1
*
*******************************************************************************/

#if !defined( _APPRTP_H )
#define _APPRTP_H

#include "cmbs_platf.h"

#if defined( __cplusplus )
extern "C"
{
#endif

E_CMBS_RC   app_RTPSessionStart(u32 u32_ChannelID, const ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation);
E_CMBS_RC   app_RTPSessionStop(u32 u32_ChannelID);
E_CMBS_RC   app_RTPSessionUpdate(u32 u32_ChannelID, const ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation);
E_CMBS_RC   app_RTCPSessionStart(u32 u32_ChannelID, u32 u32_RTCPInterval);
E_CMBS_RC   app_RTCPSessionStop(u32 u32_ChannelID);
E_CMBS_RC   app_RTPSendDTMF(u32 u32_ChannelID, const ST_IE_RTP_DTMF_EVENT * pst_RTPDTMFEvent, const ST_IE_RTP_DTMF_EVENT_INFO * pst_RTPDTMFEventInfo);
E_CMBS_RC   app_RTPEnableFaxAudioProcessingMode(u32 u32_ChannelID);
E_CMBS_RC   app_RTPDisableFaxAudioProcessingMode(u32 u32_ChannelID);
int         app_RTPEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData);

#if defined( __cplusplus )
}
#endif

#endif // _APPRTP_H
//*/
