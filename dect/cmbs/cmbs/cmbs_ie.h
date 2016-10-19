/*!
*  \file       cmbs_ie.h
*  \brief      Information Elements List functions
*  \author     andriig
*
*  @(#)  %filespec: cmbs_ie.h~7 %
*
*******************************************************************************/

#if   !defined( CMBS_IE_H )
#define  CMBS_IE_H
#include "cmbs_api.h"               /* CMBS API definition */

/* Allocate IE list and verify it */
#define ALLOCATE_IE_LIST(p_List)      \
 p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList(); \
 if(!p_List)           \
 return CMBS_RC_ERROR_OUT_OF_MEM;

/* Get IE type and check it */
#define CHECK_IE_TYPE(pu8_Buffer, IEType)       \
 {                \
  u16  u16_Type = 0;          \
  cfr_ie_dser_u16( pu8_Buffer + CFR_IE_TYPE_POS, &u16_Type ); \
  if ( IEType != u16_Type )         \
   return CMBS_RC_ERROR_PARAMETER;       \
 }

void * cmbs_api_ie_GetList(void);
E_CMBS_RC cfr_ie_DeregisterThread(u32 u32_ThreadId);

E_CMBS_RC cmbs_api_ie_GetFirst(void * pv_RefIEList, void ** ppv_RefIE, u16 * pu16_IEType);
E_CMBS_RC cmbs_api_ie_GetNext(void * pv_RefIEList, void ** ppv_RefIE, u16 * pu16_IEType);
E_CMBS_RC cmbs_api_ie_GetIE(void * pv_RefIEList, void ** ppv_RefIE, u16 pu16_IEType);

E_CMBS_RC cmbs_api_ie_ByteValueAdd(void * pv_RefIEList, u8 u8_Value, E_CMBS_IE_TYPE e_IETYPE);
E_CMBS_RC cmbs_api_ie_ByteValueGet(void * pv_RefIE, u8 * pu8_Value, E_CMBS_IE_TYPE e_IETYPE);
E_CMBS_RC cmbs_api_ie_ShortValueAdd(void * pv_RefIEList, u16 u16_Value, E_CMBS_IE_TYPE e_IETYPE);
E_CMBS_RC cmbs_api_ie_ShortValueGet(void * pv_RefIE, u16 * pu16_Value, E_CMBS_IE_TYPE e_IETYPE);
E_CMBS_RC cmbs_api_ie_u32ValueAdd(void * pv_RefIEList, u32 u32_Value, E_CMBS_IE_TYPE e_IEType);
E_CMBS_RC cmbs_api_ie_u32ValueGet(void * pv_RefIE, u32 * pu32_Value, E_CMBS_IE_TYPE e_IEType);

E_CMBS_RC cmbs_api_ie_IntValueAdd(void * pv_RefIEList, u32 u32_Value);
E_CMBS_RC cmbs_api_ie_IntValueGet(void * pv_RefIE, u32 * pu32_Value);
E_CMBS_RC cmbs_api_ie_LineIdAdd(void * pv_RefIEList, u8 u8_LineId);
E_CMBS_RC cmbs_api_ie_LineIdGet(void * pv_RefIE, u8 * pu8_LineId);
E_CMBS_RC cmbs_api_ie_MelodyAdd(void * pv_RefIEList, u8 u8_Melody);
E_CMBS_RC cmbs_api_ie_MelodyGet(void * pv_RefIE, u8 * pu8_Melody);
E_CMBS_RC cmbs_api_ie_CallInstanceAdd(void * pv_RefIEList, u32 u32_CallInstance);
E_CMBS_RC cmbs_api_ie_CallInstanceGet(void * pv_RefIE, u32 * pu32_CallInstance);
E_CMBS_RC cmbs_api_ie_RequestIdAdd(void * pv_RefIE, u16 pu16_RequestId);
E_CMBS_RC cmbs_api_ie_RequestIdGet(void * pv_RefIE, u16 * pu16_RequestId);
E_CMBS_RC cmbs_api_ie_HandsetsAdd(void * pv_RefIE, u16  u16_Handsets);
E_CMBS_RC cmbs_api_ie_HandsetsGet(void * pv_RefIE, u16 * pu16_Handsets);
E_CMBS_RC cmbs_api_ie_HsNumberAdd(void * pv_RefIEList, u8 u8_HsNumber);
E_CMBS_RC cmbs_api_ie_HsNumberGet(void * pv_RefIE, u8 * pu8_HsNumber);
E_CMBS_RC cmbs_api_ie_GpioAdd(void * pv_RefIEList, u16 u16_Gpio);
E_CMBS_RC cmbs_api_ie_GpioGet(void * pv_RefIE, u16 * pu16_Gpio);

E_CMBS_RC cmbs_api_ie_CallTransferReqAdd(void * pv_RefIEList, PST_IE_CALLTRANSFERREQ pst_CallTrf);
E_CMBS_RC cmbs_api_ie_CallTransferReqGet(void * pv_RefIE, PST_IE_CALLTRANSFERREQ pst_CallTrf);
E_CMBS_RC cmbs_api_ie_InternalCallTransferReqAdd(void * pv_RefIEList, PST_IE_INTERNAL_TRANSFER pst_InternalTransfer);
E_CMBS_RC cmbs_api_ie_InternalCallTransferReqGet(void * pv_RefIE, PST_IE_INTERNAL_TRANSFER pst_InternalTransfer);
E_CMBS_RC cmbs_api_ie_CallerPartyAdd(void * pv_RefIEList, ST_IE_CALLERPARTY * pst_CallerParty);
E_CMBS_RC cmbs_api_ie_CallerPartyGet(void * pv_RefIE, ST_IE_CALLERPARTY * pst_CallerParty);
E_CMBS_RC cmbs_api_ie_CalledPartyAdd(void * pv_RefIEList, ST_IE_CALLEDPARTY * pst_CalledParty);
E_CMBS_RC cmbs_api_ie_CalledPartyGet(void * pv_RefIE, ST_IE_CALLEDPARTY * pst_CalledParty);
E_CMBS_RC cmbs_api_ie_CallerNameAdd(void * pv_RefIEList, ST_IE_CALLERNAME * pst_CallerName);
E_CMBS_RC cmbs_api_ie_CallerNameGet(void * pv_RefIE, ST_IE_CALLERNAME * pst_CallerName);
E_CMBS_RC cmbs_api_ie_CallProgressAdd(void * pv_RefIEList, ST_IE_CALLPROGRESS * pst_CallProgress);
E_CMBS_RC cmbs_api_ie_CallProgressGet(void * pv_RefIE, ST_IE_CALLPROGRESS * pst_CallProgress);
E_CMBS_RC cmbs_api_ie_CallInfoAdd(void * pv_RefIEList, ST_IE_CALLINFO * pst_CallInfo);
E_CMBS_RC cmbs_api_ie_CallInfoGet(void * pv_RefIE, ST_IE_CALLINFO * pst_CallInfo);
E_CMBS_RC cmbs_api_ie_DisplayStringAdd(void * pv_RefIEList, ST_IE_DISPLAY_STRING * pst_DisplayString);
E_CMBS_RC cmbs_api_ie_CallReleaseReasonAdd(void * pv_RefIEList, ST_IE_RELEASE_REASON * pst_RelReason);
E_CMBS_RC cmbs_api_ie_CallReleaseReasonGet(void * pv_RefIE, ST_IE_RELEASE_REASON * pst_RelReason);
E_CMBS_RC cmbs_api_ie_CallStateGet(void * pv_RefIE, ST_IE_CALL_STATE * pst_CallState);
E_CMBS_RC cmbs_api_ie_MediaChannelAdd(void * pv_RefIEList, ST_IE_MEDIA_CHANNEL * pst_MediaChannel);
E_CMBS_RC cmbs_api_ie_MediaChannelGet(void * pv_RefIE, ST_IE_MEDIA_CHANNEL * pst_MediaChannel);
E_CMBS_RC cmbs_api_ie_MediaICAdd(void * pv_RefIEList, ST_IE_MEDIA_INTERNAL_CONNECT * pst_MediaIC);
E_CMBS_RC cmbs_api_ie_MediaICGet(void * pv_RefIE, ST_IE_MEDIA_INTERNAL_CONNECT * pst_MediaIC);
E_CMBS_RC cmbs_api_ie_MediaDescAdd(void * pv_RefIEList, ST_IE_MEDIA_DESCRIPTOR * pst_MediaDesc);
E_CMBS_RC cmbs_api_ie_MediaDescGet(void * pv_RefIE, ST_IE_MEDIA_DESCRIPTOR * pst_MediaDesc);
E_CMBS_RC cmbs_api_ie_ToneAdd(void * pv_RefIEList, ST_IE_TONE * pst_Tone);
E_CMBS_RC cmbs_api_ie_TimeAdd(void * pv_RefIEList, ST_IE_TIMEOFDAY * pst_TimeOfDay);
E_CMBS_RC cmbs_api_ie_TimeGet(void * pv_RefIE, ST_IE_TIMEOFDAY * pst_TimeOfDay);
E_CMBS_RC cmbs_api_ie_HandsetInfoGet(void * pv_RefIE, ST_IE_HANDSETINFO * pst_HandsetInfo);
E_CMBS_RC cmbs_api_ie_ParameterGet(void * pv_RefIE, ST_IE_PARAMETER * pst_Parameter);
E_CMBS_RC cmbs_api_ie_SubscribedHSListAdd(void * pv_RefIE, ST_IE_SUBSCRIBED_HS_LIST * pst_SubscribedHsList);
E_CMBS_RC cmbs_api_ie_SubscribedHSListGet(void * pv_RefIE, ST_IE_SUBSCRIBED_HS_LIST * pst_SubscribedHsList);
E_CMBS_RC cmbs_api_ie_LAPropCmdAdd(void * pv_RefIE, ST_LA_PROP_CMD * pst_Cmd);
E_CMBS_RC cmbs_api_ie_LAPropCmdGet(void * pv_RefIE, ST_LA_PROP_CMD * pst_Cmd);
E_CMBS_RC cmbs_api_ie_LineSettingsListAdd(void * pv_RefIE, ST_IE_LINE_SETTINGS_LIST * pst_LineSettingsList);
E_CMBS_RC cmbs_api_ie_LineSettingsListGet(void * pv_RefIE, ST_IE_LINE_SETTINGS_LIST * pst_LineSettingsList);
E_CMBS_RC cmbs_api_ie_ParameterAreaGet(void * pv_RefIE, ST_IE_PARAMETER_AREA * pst_ParameterArea);
E_CMBS_RC cmbs_api_ie_FwVersionGet(void * pv_RefIE, ST_IE_FW_VERSION * pst_FwVersion);
E_CMBS_RC cmbs_api_ie_HwVersionGet(void * pv_RefIE, ST_IE_HW_VERSION * pst_HwVersion);
E_CMBS_RC cmbs_api_ie_EEPROMVersionGet(void * pv_RefIE, ST_IE_EEPROM_VERSION * pst_EEPROMVersion);
E_CMBS_RC cmbs_api_ie_SysLogGet(void * pv_RefIE, ST_IE_SYS_LOG * pst_SysLog);
E_CMBS_RC cmbs_api_ie_SysStatusGet(void * pv_RefIE, ST_IE_SYS_STATUS * pst_SysStatus);
E_CMBS_RC cmbs_api_ie_ResponseAdd(void * pv_RefIE, ST_IE_RESPONSE * pst_Response);
E_CMBS_RC cmbs_api_ie_ResponseGet(void * pv_RefIE, ST_IE_RESPONSE * pst_Response);
E_CMBS_RC cmbs_api_ie_DateTimeAdd(void * pv_RefIEList, ST_IE_DATETIME * pst_DateTime);
E_CMBS_RC cmbs_api_ie_DateTimeGet(void * pv_RefIE, ST_IE_DATETIME * pst_DateTime);
E_CMBS_RC cmbs_api_ie_DataGet(void * pv_RefIE, ST_IE_DATA * pst_Data);
E_CMBS_RC cmbs_api_ie_DataSessionIdAdd(void * pv_RefIE, u16 pu16_DataSessionId);
E_CMBS_RC cmbs_api_ie_DataSessionIdGet(void * pv_RefIE, u16 * pu16_DataSessionId);
E_CMBS_RC cmbs_api_ie_DataSessionTypeGet(void * pv_RefIE, ST_IE_DATA_SESSION_TYPE * pst_DataSessionType);
E_CMBS_RC cmbs_api_ie_LASessionIdAdd(void * pv_RefIE, u16 pu16_LASessionId);
E_CMBS_RC cmbs_api_ie_LASessionIdGet(void * pv_RefIE, u16 * pu16_LASessionId);
E_CMBS_RC cmbs_api_ie_LAListIdGet(void * pv_RefIE, u16 * pu16_LAListId);
E_CMBS_RC cmbs_api_ie_LAListIdAdd(void * pv_RefIE, u16 u16_LAListId);
E_CMBS_RC cmbs_api_ie_LAFieldsGet(void * pv_RefIE, ST_IE_LA_FIELDS * pst_LAFields);
E_CMBS_RC cmbs_api_ie_LASearchCriteriaGet(void * pv_RefIE, ST_IE_LA_SEARCH_CRITERIA * pst_LASearchCriteria);
E_CMBS_RC cmbs_api_ie_LAEntryIdGet(void * pv_RefIE, u16 * pu16_LAEntryId);
E_CMBS_RC cmbs_api_ie_LAEntryIndexGet(void * pv_RefIE, u16 * pu16_LAEntryIndex);
E_CMBS_RC cmbs_api_ie_LAEntryCountGet(void * pv_RefIE, u16 * pu16_LAEntryCount);
E_CMBS_RC cmbs_api_ie_LAIsLastGet(void * pv_RefIE, u8 * pu8_LAIsLast);
E_CMBS_RC cmbs_api_ie_ATESettingsGet(void * pv_RefIE, ST_IE_ATE_SETTINGS * pst_AteSettings);
E_CMBS_RC cmbs_api_ie_ATESettingsAdd(void * pv_RefIEList, ST_IE_ATE_SETTINGS * pst_AteSettings);
E_CMBS_RC cmbs_api_ie_ReadDirectionAdd(void * pv_RefIEList, ST_IE_READ_DIRECTION * pst_ReadDirection);
E_CMBS_RC cmbs_api_ie_ReadDirectionGet(void * pv_RefIE, ST_IE_READ_DIRECTION * pst_ReadDirection);
E_CMBS_RC cmbs_api_ie_MarkRequestAdd(void * pv_RefIEList, ST_IE_MARK_REQUEST * pst_MarkRequest);
E_CMBS_RC cmbs_api_ie_MarkRequestGet(void * pv_RefIE, ST_IE_MARK_REQUEST * pst_MarkRequest);
E_CMBS_RC cmbs_api_ie_VersionAvailAdd(void * pv_RefIEList, ST_SUOTA_UPGRADE_DETAILS* st_HSVerAvail);
E_CMBS_RC cmbs_api_ie_VersionAvailGet(void * pv_RefIEList, ST_SUOTA_UPGRADE_DETAILS* pst_HSVerAvail);
E_CMBS_RC cmbs_api_ie_VersionBufferAdd(void * pv_RefIEList, ST_VERSION_BUFFER* pst_SwVersion);
E_CMBS_RC cmbs_api_ie_VersionBufferGet(void * pv_RefIEList, ST_VERSION_BUFFER* pst_SwVersion);
E_CMBS_RC cmbs_api_ie_VersionIndGet(void * pv_RefIEList, ST_SUOTA_HS_VERSION_IND* pst_HSVerInd);
E_CMBS_RC cmbs_api_ie_VersionIndAdd(void * pv_RefIEList, ST_SUOTA_HS_VERSION_IND* st_HSVerInd);
E_CMBS_RC cmbs_api_ie_UrlAdd(void * pv_RefIEList, ST_URL_BUFFER* pst_Url);
E_CMBS_RC cmbs_api_ie_UrlGet(void * pv_RefIEList, ST_URL_BUFFER* pst_Url);
E_CMBS_RC cmbs_api_ie_NBOTACodecAdd(void *pv_RefIEList, PST_IE_NB_CODEC_OTA pst_Codec);
E_CMBS_RC cmbs_api_ie_NBOTACodecGet(void *pv_RefIE, PST_IE_NB_CODEC_OTA pst_Codec);
E_CMBS_RC cmbs_api_ie_TargetListChangeNotifAdd(void *pv_RefIEList, PST_IE_TARGET_LIST_CHANGE_NOTIF pst_Notif);
E_CMBS_RC cmbs_api_ie_TargetListChangeNotifGet(void * pv_RefIE, PST_IE_TARGET_LIST_CHANGE_NOTIF pst_Notif);
E_CMBS_RC cmbs_api_ie_DectSettingsListAdd(void * pv_RefIEList, ST_IE_DECT_SETTINGS_LIST * pst_DectSettings);
E_CMBS_RC cmbs_api_ie_DectSettingsListGet(void * pv_RefIE, ST_IE_DECT_SETTINGS_LIST * pst_DectSettings);
E_CMBS_RC cmbs_api_ie_RTPSessionInformationAdd(void * pv_RefIEList, const ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation);
E_CMBS_RC cmbs_api_ie_RTPSessionInformationGet(void * pv_RefIE, ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation);
E_CMBS_RC cmbs_api_ie_RTCPIntervalAdd(void * pv_RefIEList, u32 u32_RTCPInterval);
E_CMBS_RC cmbs_api_ie_RTCPIntervalGet(void * pv_RefIE, u32 * pu32_RTCPInterval);
E_CMBS_RC cmbs_api_ie_RTPDTMFEventAdd(void * pv_RefIEList, const ST_IE_RTP_DTMF_EVENT * pst_RTPDTMFEvent);
E_CMBS_RC cmbs_api_ie_RTPDTMFEventGet(void * pv_RefIE, ST_IE_RTP_DTMF_EVENT * pst_RTPDTMFEvent);
E_CMBS_RC cmbs_api_ie_RTPDTMFEventInfoAdd(void * pv_RefIEList, const ST_IE_RTP_DTMF_EVENT_INFO * pst_RTPDTMFEventInfo);
E_CMBS_RC cmbs_api_ie_RTPDTMFEventInfoGet(void * pv_RefIE, ST_IE_RTP_DTMF_EVENT_INFO * pst_RTPDTMFEventInfo);
E_CMBS_RC cmbs_api_ie_RTPFaxToneTypeAdd(void * pv_RefIEList, E_CMBS_FAX_TONE_TYPE e_FaxToneType);
E_CMBS_RC cmbs_api_ie_RTPFaxToneTypeGet(void * pv_RefIE, E_CMBS_FAX_TONE_TYPE * pe_FaxToneType);
E_CMBS_RC cmbs_api_ie_BaseNameAdd(void * pv_RefIE, ST_IE_BASE_NAME* pst_BaseName);
E_CMBS_RC cmbs_api_ie_BaseNameGet(void * pv_RefIE, ST_IE_BASE_NAME* pst_BaseName);
E_CMBS_RC cmbs_api_ie_RegCloseReasonGet(void * pv_RefIE, ST_IE_REG_CLOSE_REASON* st_Reg_Close_Reason);
E_CMBS_RC cmbs_api_ie_DCRejectReasonGet(void * pv_RefIE, ST_IE_DC_REJECT_REASON* st_DC_Reject_Reason);
E_CMBS_RC cmbs_api_ie_DCRejectReasonAdd(void * pv_RefIE, ST_IE_DC_REJECT_REASON* st_DC_Reject_Reason);
E_CMBS_RC cmbs_api_ie_ParameterAdd(void * pv_RefIEList, ST_IE_PARAMETER * pst_Param);
E_CMBS_RC cmbs_api_ie_ParameterAreaAdd(void * pv_RefIEList, ST_IE_PARAMETER_AREA * pst_ParamArea);
E_CMBS_RC cmbs_api_ie_HandsetInfoAdd(void * pv_RefIEList, ST_IE_HANDSETINFO * pst_HandsetInfo);
E_CMBS_RC cmbs_api_ie_DisplayStringGet(void * pv_RefIE, ST_IE_DISPLAY_STRING * pst_DisplayString);
E_CMBS_RC cmbs_api_ie_ToneGet(void * pv_RefIE, ST_IE_TONE * pst_Tone);
E_CMBS_RC cmbs_api_ie_FwVersionAdd(void * pv_RefIEList, ST_IE_FW_VERSION * pst_FwVersion);
E_CMBS_RC cmbs_api_ie_EEPROMVersionAdd(void * pv_RefIEList, ST_IE_EEPROM_VERSION * pst_EEPROMVersion);
E_CMBS_RC cmbs_api_ie_HwVersionAdd(void * pv_RefIEList, ST_IE_HW_VERSION * pst_HwVersion);
E_CMBS_RC cmbs_api_ie_SysLogAdd(void * pv_RefIEList, ST_IE_SYS_LOG * pst_SysLog);
E_CMBS_RC cmbs_api_ie_SysStatusAdd(void * pv_RefIEList, ST_IE_SYS_STATUS * pst_SysStatus);
E_CMBS_RC cmbs_api_ie_GenEventAdd(void * pv_RefIEList, ST_IE_GEN_EVENT * pst_GenEvent);
E_CMBS_RC cmbs_api_ie_GenEventGet(void * pv_RefIE, ST_IE_GEN_EVENT * pst_GenEvent);
E_CMBS_RC cmbs_api_ie_PropEventAdd(void * pv_RefIEList, ST_IE_PROP_EVENT * pst_PropEvent);
E_CMBS_RC cmbs_api_ie_PropEventGet(void * pv_RefIE, ST_IE_PROP_EVENT * pst_PropEvent);

E_CMBS_RC cmbs_api_ie_DataSessionTypeAdd(void * pv_RefIEList, ST_IE_DATA_SESSION_TYPE * pst_DataSessionType);
E_CMBS_RC cmbs_api_ie_DataAdd(void * pv_RefIEList, ST_IE_DATA * pst_Data);
E_CMBS_RC cmbs_api_ie_CallStateAdd(void * pv_RefIEList, ST_IE_CALL_STATE * pst_CallState);
E_CMBS_RC cmbs_api_ie_LAFieldsAdd(void * pv_RefIEList, ST_IE_LA_FIELDS * pst_LAFields, E_CMBS_IE_TYPE e_IEType);
E_CMBS_RC cmbs_api_ie_LASearchCriteriaAdd(void * pv_RefIEList, ST_IE_LA_SEARCH_CRITERIA * pst_LASearchCriteria);
E_CMBS_RC cmbs_api_ie_RegCloseReasonAdd(void * pv_RefIEList, ST_IE_REG_CLOSE_REASON * pst_Reg_Close_Reason);

E_CMBS_RC cmbs_api_ie_LAEntryIdAdd(void * pv_RefIE, u16 u16_LAEntryId);
E_CMBS_RC cmbs_api_ie_LAEntryIndexAdd(void * pv_RefIE, u16 u16_LAEntryIndex);
E_CMBS_RC cmbs_api_ie_LAEntryCountAdd(void * pv_RefIE, u16 u16_LAEntryCount);
E_CMBS_RC cmbs_api_ie_LAIsLastAdd(void * pv_RefIE, u8 u8_LAIsLast);
E_CMBS_RC cmbs_api_ie_LARejectReasonAdd(void * pv_RefIE, u8 u8_LARejectReason);
E_CMBS_RC cmbs_api_ie_LARejectReasonGet(void * pv_RefIE, u8 * pu8_LARejectReason);
E_CMBS_RC cmbs_api_ie_LANrOfEntriesAdd(void * pv_RefIE, u16 u16_LANrOfEntries);
E_CMBS_RC cmbs_api_ie_LANrOfEntriesGet(void * pv_RefIE, u16 * pu16_LANrOfEntries);
E_CMBS_RC cmbs_api_ie_LineSubtypeAdd(void * pv_RefIEList, u8 value);
E_CMBS_RC cmbs_api_ie_LineSubtypeGet(void * pv_RefIE, u8 * value);
E_CMBS_RC cmbs_api_ie_SuSubtypeAdd(void * pv_RefIEList, u8 value);
E_CMBS_RC cmbs_api_ie_SuSubtypeGet(void * pv_RefIE, u8 * value);
E_CMBS_RC cmbs_api_ie_NumOfUrlsAdd(void * pv_RefIEList, u8 value);
E_CMBS_RC cmbs_api_ie_NumOfUrlsGet(void * pv_RefIE, u8 * value);
E_CMBS_RC cmbs_api_ie_RejectReasonAdd(void * pv_RefIEList, u8 value);
E_CMBS_RC cmbs_api_ie_RejectReasonGet(void * pv_RefIE, u8 * value);
E_CMBS_RC cmbs_api_ie_SuotaAppIdAdd(void * pv_RefIE, u32 u32_SuotaAppId);
E_CMBS_RC cmbs_api_ie_SuotaAppIdGet(void * pv_RefIE, u32 * u32_SuotaAppId);
E_CMBS_RC cmbs_api_ie_SuotaSessionIdAdd(void * pv_RefIE, u32 u32_SuotaSessionId);
E_CMBS_RC cmbs_api_ie_SuotaSessionIdGet(void * pv_RefIE, u32 * u32_SuotaSessionId);

E_CMBS_RC cmbs_api_ie_HsPropEventAdd(void * pv_RefIEList, ST_IE_HS_PROP_EVENT * pst_Param);
E_CMBS_RC cmbs_api_ie_HsPropEventGet(void * pv_RefIE, ST_IE_HS_PROP_EVENT * pst_Parameter);

E_CMBS_RC cmbs_api_ie_SYPOSpecificationGet(void * pv_RefIE, ST_IE_SYPO_SPECIFICATION* pst_Parameter);
E_CMBS_RC cmbs_api_ie_SYPOSpecificationAdd(void * pv_RefIE, ST_IE_SYPO_SPECIFICATION * pst_Parameter);

E_CMBS_RC cmbs_api_ie_AFE_EndpointConnectionGet(void *pv_RefIE, ST_IE_AFE_ENDPOINTS_CONNECT *pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointConnectionAdd(void *pv_RefIE, ST_IE_AFE_ENDPOINTS_CONNECT *pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointGet(void *pv_RefIE, ST_IE_AFE_ENDPOINT *pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointAdd(void * pv_RefIE, ST_IE_AFE_ENDPOINT * pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointGainGet(void * pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN *pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointGainAdd(void * pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN * pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointGainDBGet(void * pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN_DB * pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_EndpointGainDBAdd(void * pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN_DB * pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureSettingsGet(void * pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_SETTINGS* pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureSettingsAdd(void * pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_SETTINGS* pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureResultGet(void * pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_RESULT* pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureResultAdd(void * pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_RESULT* pst_Parameter);
E_CMBS_RC cmbs_api_ie_AFE_ResourceTypeAdd(void * pv_RefIE, u8 u8_ResourceType);
E_CMBS_RC cmbs_api_ie_AFE_ResourceTypeGet(void * pv_RefIE, u8 * u8_ResourceType);
E_CMBS_RC cmbs_api_ie_AFE_InstanceNumAdd(void * pv_RefIE, u8 u8_InstanceNum);
E_CMBS_RC cmbs_api_ie_AFE_InstanceNumGet(void * pv_RefIE, u8 * u8_InstanceNum);
E_CMBS_RC cmbs_api_ie_DHSGValueAdd(void * pv_RefIE, u8 u8_DHSGValue);
E_CMBS_RC cmbs_api_ie_DHSGValueGet(void * pv_RefIE, u8 *u8_DHSGValue);
E_CMBS_RC cmbs_api_ie_GPIOIDGet(void * pv_RefIE, PST_IE_GPIO_ID pst_GPIOID);
E_CMBS_RC cmbs_api_ie_GPIOIDAdd(void * pv_RefIE, PST_IE_GPIO_ID st_GPIOID);
E_CMBS_RC cmbs_api_ie_GPIOModeGet(void * pv_RefIE, u8 * u8_Mode);
E_CMBS_RC cmbs_api_ie_GPIOModeAdd(void * pv_RefIE, u8  u8_Mode);
E_CMBS_RC cmbs_api_ie_GPIOValueGet(void * pv_RefIE, u8 * u8_Value);
E_CMBS_RC cmbs_api_ie_GPIOValueAdd(void * pv_RefIE, u8  u8_Value);
E_CMBS_RC cmbs_api_ie_GPIOPullTypeGet(void * pv_RefIE, u8 * u8_PullType);
E_CMBS_RC cmbs_api_ie_GPIOPullTypeAdd(void * pv_RefIE, u8 u8_PullType);
E_CMBS_RC cmbs_api_ie_GPIOPullEnaGet(void * pv_RefIE, u8 * u8_PullEna);
E_CMBS_RC cmbs_api_ie_GPIOPullEnaAdd(void * pv_RefIE, u8 u8_PullEna);
E_CMBS_RC cmbs_api_ie_GPIOEnaGet(void * pv_RefIE, u8 * u8_Ena);
E_CMBS_RC cmbs_api_ie_GPIOEnaAdd(void * pv_RefIE, u8 u8_Ena);
E_CMBS_RC cmbs_api_ie_ExtIntNumGet(void * pv_RefIE, u8 * u8_IntNum);
E_CMBS_RC cmbs_api_ie_ExtIntNumAdd(void * pv_RefIE, u8 u8_IntNum);
E_CMBS_RC cmbs_api_ie_ExtIntConfigurationAdd(void * pv_RefIE, PST_IE_INT_CONFIGURATION st_INTConfiguration);
E_CMBS_RC cmbs_api_ie_ExtIntConfigurationGet(void * pv_RefIE, PST_IE_INT_CONFIGURATION st_INTConfiguration);
E_CMBS_RC cmbs_api_ie_TerminalCapabilitiesAdd(void * pv_RefIE, PST_IE_TERMINAL_CAPABILITIES pst_TermCapability);
E_CMBS_RC cmbs_api_ie_TerminalCapabilitiesGet(void * pv_RefIE, PST_IE_TERMINAL_CAPABILITIES pst_TermCapability);
E_CMBS_RC cmbs_api_ie_ChecksumErrorAdd(void * pv_RefIEList, PST_IE_CHECKSUM_ERROR  pst_CheckSumError);
E_CMBS_RC cmbs_api_ie_ChecksumErrorGet(void * pv_RefIE, PST_IE_CHECKSUM_ERROR  pst_CheckSumError);
E_CMBS_RC cmbs_api_ie_CallHoldReasonGet(void * pv_RefIE, PST_IE_CALL_HOLD_REASON pst_Reason);
E_CMBS_RC cmbs_api_ie_CallHoldReasonAdd(void * pv_RefIE, PST_IE_CALL_HOLD_REASON pst_Reason);

#endif
