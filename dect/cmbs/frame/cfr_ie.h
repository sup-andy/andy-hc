/*!
* \file   cfr_ie.h
* \brief
* \Author  kelbch
*
* @(#) %filespec: cfr_ie.h~DMZD53#6 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
* 27-05-14 ronenw  GIT  Added common ie for cmbs and han
*
*******************************************************************************/

#if !defined( CFR_IE_H )
#define CFR_IE_H

#include "cmbs_ie.h"
#include "cmbs_han.h"

#define  CFR_IE_TYPE_POS  0
#define  CFR_IE_SIZE_POS  2
#define  CFR_IE_HEADER_SIZE 4

typedef struct
{
    u16  u16_MaxSize;
    u16  u16_CurSize;
    u16  u16_CurIE;
    u8 *    pu8_Buffer;
} ST_CFR_IE_LIST, * PST_CFR_IE_LIST;

#define TEMP_SIZE (CMBS_PARAM_MAX_LENGTH+100)
typedef union
{
    u8 u8_BufferByteValueAdd[CFR_IE_HEADER_SIZE + sizeof(u8)];
    u8 u8_BufferShortValueAdd[CFR_IE_HEADER_SIZE + sizeof(u16)];
    u8 u8_BufferByteU32ValueAdd[CFR_IE_HEADER_SIZE + sizeof(u32)];
    u8  u8_BufferReqAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALLTRANSFERREQ)];
    u8  u8_BufferInternalReqAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_INTERNAL_TRANSFER)];
    u8  u8_BufferCallerPartyAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALLERPARTY) + CMBS_CALLER_NUM_MAX_LEN];
    u8  u8_BufferCalledPartyAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALLEDPARTY) + CMBS_CALLER_NUM_MAX_LEN];
    u8  u8_BufferCallerNameAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALLINFO) + TEMP_SIZE];
    u8  u8_BufferCallInfoAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALLINFO) + TEMP_SIZE];
    u8  u8_BufferDisplayStringAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_DISPLAY_STRING) + CMBS_DISPLAY_STRING_LENGTH];
    u8  u8_BufferReleaseReasonAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_RELEASE_REASON)];
    u8  u8_BufferCallStateAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALL_STATE)];
    u8  u8_BufferChannelAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_MEDIA_CHANNEL)];
    u8  u8_BufferICAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_MEDIA_INTERNAL_CONNECT)];
    u8  u8_BufferDescAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_MEDIA_DESCRIPTOR)];
    u8  u8_BufferInfoAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_HANDSETINFO)];
    u8  u8_BufferParameterAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_PARAMETER) + TEMP_SIZE];
    u8  u8_BufferHsListAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_SUBSCRIBED_HS_LIST) + TEMP_SIZE];
    u8  u8_BufferSettingsAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_LINE_SETTINGS_LIST)];
    u8  u8_BufferVersionAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_FW_VERSION)];
    u8  u8_BufferLogAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_SYS_LOG)];
    u8  u8_BufferAreaAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_PARAMETER_AREA) + TEMP_SIZE];
    u8  u8_BufferGenEventAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_GEN_EVENT)];
    u8  u8_BufferPropEventAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_PARAMETER_AREA) + TEMP_SIZE];
    u8  u8_BufferDateAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_DATETIME)];
    u8  u8_BufferSessionTypeAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_DATA_SESSION_TYPE)];
    u8  u8_BufferDataAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_DATA) + TEMP_SIZE];
    u8  u8_BufferFieldsAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_LA_FIELDS)];
    u8  u8_BufferCriteriaAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_LA_SEARCH_CRITERIA)];
    u8  u8_BufferATESettingsAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_ATE_SETTINGS)];
    u8  u8_BufferVersionAvailAdd[CFR_IE_HEADER_SIZE + sizeof(ST_SUOTA_UPGRADE_DETAILS) + 1]; // because of bit fields
    u8  u8_BufferVersionBufAdd[CFR_IE_HEADER_SIZE + sizeof(ST_VERSION_BUFFER)];
    u8  u8_BufferVersionIndAdd[CFR_IE_HEADER_SIZE + sizeof(ST_SUOTA_HS_VERSION_IND)];
    u8  u8_BufferURLAdd[CFR_IE_HEADER_SIZE + sizeof(ST_URL_BUFFER)];
    u8  u8_BufferChangeNotifAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_TARGET_LIST_CHANGE_NOTIF)];
    u8  u8_BufferHwVersionAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_HW_VERSION)];
    u8  u8_BufferDectSettingsAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_DECT_SETTINGS_LIST)];
    u8  u8_BufferSessionInfoAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_RTP_SESSION_INFORMATION)];
    u8  u8_BufferDTMFEventAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_RTP_DTMF_EVENT)];
    u8  u8_BufferDTMFEventInfoAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_RTP_DTMF_EVENT_INFO)];
    u8  u8_BufferPropCmdAdd[CFR_IE_HEADER_SIZE + sizeof(ST_LA_PROP_CMD)];
    u8  u8_BufferBaseNameAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_BASE_NAME)];
    u8  u8_BufferHsPropEventAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_HS_PROP_EVENT) + TEMP_SIZE];
    u8  u8_BufferSYPOSpecificationAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_SYPO_SPECIFICATION)];
    u8  u8_BufferAFEEndpointConnectionAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_AFE_ENDPOINTS_CONNECT)];
    u8  u8_BufferAFEEndpointAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_AFE_ENDPOINT)];
    u8  u8_BufferAFEEndpointGainAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_AFE_ENDPOINT_GAIN)];
    u8  u8_BufferAFEEndpointGainDBAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_AFE_ENDPOINT_GAIN)];
    u8  u8_BufferAFEAUXMeasureSettingsAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_AFE_AUX_MEASUREMENT_SETTINGS)];
    u8  u8_BufferAFEAUXMeasureResultAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_AFE_AUX_MEASUREMENT_RESULT)];
    u8  u8_BufferGPIOIDAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_GPIO_ID)];
    u8  u8_BufferExtINTConfigAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_INT_CONFIGURATION)];
    u8  u8_BufferTerminalCapabilitiesAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_TERMINAL_CAPABILITIES)];
    u8  u8_BufferChecksumErrorAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CHECKSUM_ERROR)];
    u8  u8_BuffeCallHoldReasonAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_CALL_HOLD_REASON)];
    u8  u8_BufferCalledNameAdd[CFR_IE_HEADER_SIZE + 3 * sizeof(u8) + 2 * CMBS_CALLED_NAME_MAX_LEN];
    // HAN part
    u8 HanTableAdd[ CFR_IE_HEADER_SIZE + CMBS_HAN_IE_TABLE_SIZE];
    u8 RegInfoAdd[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_MSG_REG_INFO)];
    u8 HanMsgAdd[ CFR_IE_HEADER_SIZE + sizeof(ST_IE_HAN_MSG) + CMBS_HAN_MAX_MSG_LEN];
    u8 UpdateInfoAdd[ CFR_IE_HEADER_SIZE + sizeof(ST_IE_HAN_TABLE_UPDATE_INFO)];
    u8 Stage1ParamsAdd[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_REG_STAGE_1_STATUS)];
    u8 Stage2ParamsAdd[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_REG_STAGE_2_STATUS)];
    u8 HanBaseInfoAdd[CFR_IE_HEADER_SIZE + sizeof(ST_IE_HAN_BASE_INFO)];
    u8 UnknownDeviceParamsAdd[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS)];
}
U_Buffer;

#if defined( __cplusplus )
extern "C"
{
#endif

void *         cfr_ie_ListNew(u8* pu8_Buffer, u16 u16_Size);
int            cfr_ie_ItemAdd(PST_CFR_IE_LIST p_List, u8 * pu8_Buffer, u16 u16_Size);
u8*            cfr_ie_ItemGet(PST_CFR_IE_LIST p_List);
u8*            cfr_ie_ItemNextGet(PST_CFR_IE_LIST p_List);
u8*            cfr_ie_ItemFirstGet(PST_CFR_IE_LIST p_List);

void           cfr_ie_Serialize(PST_CFR_IE_LIST p_List, E_CMBS_IE_TYPE e_IE, void * pv_Data);

u16            cfr_ie_ser_pu8(u8 * pu8_buffer, u8 * pu8_Data, u16 u16_DataSize);
u16            cfr_ie_ser_u8(u8 * pu8_Buffer, u8 u8_Value);
u16            cfr_ie_ser_u16(u8 * pu8_buffer, u16  u16_Value);
u16            cfr_ie_ser_u32(u8 * pu8_buffer, u32  u32_Value);

u16            cfr_ie_dser_pu8(u8 * pu8_Buffer, u8  * pu8_Data, u16 u16_DataSize);
u16            cfr_ie_dser_u8(u8 * pu8_Buffer, u8 * pu8_Value);
u16            cfr_ie_dser_u16(u8 * pu8_Buffer, u16 * pu16_Value);
u16            cfr_ie_dser_u32(u8 * pu8_Buffer, u32 * pu32_Value);


#if !defined ( CMBS_API_TARGET )
u8          cfr_ie_getThreadIdx(void);
u8          cfr_ie_AllocThreadIdx(void);
#endif

#if defined( __cplusplus )
}
#endif

#endif // CFR_IE_H
//*/
