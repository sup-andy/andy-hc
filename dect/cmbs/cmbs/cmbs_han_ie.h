/*!
*  \file       cmbs_han_ie.h
*  \brief      Information Elements List functions for HAN
*  \author     andriig
*
*  @(#)  %filespec: cmbs_han_ie.h~ILD53#3 %
*
*******************************************************************************/

#if   !defined( CMBS_HAN_IE_H )
#define  CMBS_HAN_IE_H

#include "cmbs_han.h"               /* CMBS HAN API definition */

E_CMBS_RC cmbs_api_ie_HanCfgAdd(void * pv_RefIEList, ST_IE_HAN_CONFIG* pst_Cfg);
E_CMBS_RC cmbs_api_ie_HanCfgGet(void * pv_RefIEList, ST_IE_HAN_CONFIG* pst_Cfg);

E_CMBS_RC    cmbs_api_ie_HanUleBaseInfoAdd(void * pv_RefIEList, ST_IE_HAN_BASE_INFO* pst_HanBaseInfo);
E_CMBS_RC    cmbs_api_ie_HanUleBaseInfoGet(void * pv_RefIEList, ST_IE_HAN_BASE_INFO* pst_HanBaseInfo);

E_CMBS_RC  cmbs_ie_HanDeviceTableBriefAdd(void * pv_RefIEList, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8* pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE);
E_CMBS_RC  cmbs_ie_HanDeviceTableExtendedAdd(void * pv_RefIEList, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8* pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE);

E_CMBS_RC cmbs_api_ie_HANDeviceTableBriefAdd(void * pv_RefIEList, ST_IE_HAN_BRIEF_DEVICE_ENTRIES* pst_HanDeviceEntries);
E_CMBS_RC cmbs_api_ie_HANDeviceTableBriefGet(void * pv_RefIE, ST_IE_HAN_BRIEF_DEVICE_ENTRIES * pst_HANDeviceTable);

E_CMBS_RC cmbs_api_ie_HANDeviceTableExtendedAdd(void * pv_RefIEList, ST_IE_HAN_EXTENDED_DEVICE_ENTRIES* pst_HANExtendedDeviceEntries);
E_CMBS_RC cmbs_api_ie_HANDeviceTableExtendedGet(void * pv_RefIE, ST_IE_HAN_EXTENDED_DEVICE_ENTRIES* pst_HANExtendedDeviceEntries);

E_CMBS_RC cmbs_api_ie_HANRegStage1OKResParamsAdd(void * pv_RefIEList, ST_HAN_REG_STAGE_1_STATUS* pst_RegStatus);
E_CMBS_RC cmbs_api_ie_HANRegStage1OKResParamsGet(void * pv_RefIE, ST_HAN_REG_STAGE_1_STATUS* pst_RegStatus);

E_CMBS_RC cmbs_api_ie_HANRegStage2OKResParamsAdd(void * pv_RefIEList, ST_HAN_REG_STAGE_2_STATUS* pst_RegStatus);
E_CMBS_RC cmbs_api_ie_HANRegStage2OKResParamsGet(void * pv_RefIE, ST_HAN_REG_STAGE_2_STATUS* pst_RegStatus);

E_CMBS_RC cmbs_api_ie_HANBindTableAdd(void * pv_RefIEList, ST_IE_HAN_BIND_ENTRIES*  pst_HanBinds);
E_CMBS_RC cmbs_api_ie_HANBindTableGet(void * pv_RefIE, ST_IE_HAN_BIND_TABLE * pst_HANBindTable);

E_CMBS_RC cmbs_api_ie_HANGroupTableAdd(void * pv_RefIEList, ST_IE_HAN_GROUP_ENTRIES*  pst_HanGroups);
E_CMBS_RC cmbs_api_ie_HANGroupTableGet(void * pv_RefIE, ST_IE_HAN_GROUP_TABLE * pst_HANGroupTable);

E_CMBS_RC cmbs_api_ie_HanMsgRegInfoAdd(void * pv_RefIEList, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);
E_CMBS_RC cmbs_api_ie_HanMsgRegInfoGet(void * pv_RefIEList, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);

E_CMBS_RC cmbs_api_ie_HANMsgAdd(void * pv_RefIE, ST_IE_HAN_MSG * pst_HANMsg);
E_CMBS_RC cmbs_api_ie_HANMsgGet(void * pv_RefIE, ST_IE_HAN_MSG * pst_HANMsg);

E_CMBS_RC cmbs_api_ie_HANDeviceAdd(void * pv_RefIE, u16 pu16_HANDevice);
E_CMBS_RC cmbs_api_ie_HANDeviceGet(void * pv_RefIE, u16* pu16_HANDevice);

E_CMBS_RC cmbs_api_ie_HANSendErrorReasonAdd(void * pv_RefIE, u16 u16_Reason);
E_CMBS_RC cmbs_api_ie_HANSendErrorReasonGet(void * pv_RefIE, u16* pu16_Reason);

E_CMBS_RC cmbs_api_ie_HANTxEndedReasonAdd(void * pv_RefIE, u16 u16_Reason);
E_CMBS_RC cmbs_api_ie_HANTxEndedReasonGet(void * pv_RefIE, u16* pu16_Reason);

E_CMBS_RC cmbs_api_ie_HANNumOfEntriesAdd(void * pv_RefIEList, u16 u16_NumOfEntries);
E_CMBS_RC cmbs_api_ie_HANNumOfEntriesGet(void * pv_RefIEList, u16 * pu16_NumOfEntries);

E_CMBS_RC cmbs_api_ie_HANIndex1stEntryAdd(void * pv_RefIEList, u16 u16_IndexOfFirstEntry);
E_CMBS_RC cmbs_api_ie_HANIndex1stEntryGet(void * pv_RefIEList, u16 * pu16_IndexOfFirstEntry);

E_CMBS_RC cmbs_api_ie_HANTableUpdateInfoAdd(void * pv_RefIE, ST_IE_HAN_TABLE_UPDATE_INFO * pst_HANTableUpdateInfo);
E_CMBS_RC cmbs_api_ie_HANTableUpdateInfoGet(void * pv_RefIE, ST_IE_HAN_TABLE_UPDATE_INFO * pst_HANTableUpdateInfo);

E_CMBS_RC cmbs_api_ie_HANGeneralStatusAdd(void * pv_RefIEList, ST_HAN_GENERAL_STATUS* pst_Status);
E_CMBS_RC cmbs_api_ie_HANGeneralStatusGet(void * pv_RefIEList, ST_HAN_GENERAL_STATUS* pst_Staus);

E_CMBS_RC cmbs_api_ie_HANUnknownDeviceContactedParamsAdd(void * pv_RefIEList, ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS* pst_Params);
E_CMBS_RC cmbs_api_ie_HANUnknownDeviceContactedParamsGet(void * pv_RefIEList, ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS* pst_Params);

E_CMBS_RC cmbs_api_ie_HANConnectionStatusAdd(void * pv_RefIE, u16 pu16_ConnectionStatus);
E_CMBS_RC cmbs_api_ie_HANConnectionStatusGet(void * pv_RefIE, u16* pu16_ConnectionStatus);

E_CMBS_RC cmbs_api_ie_HANForcefulDeRegErrorReasonAdd(void * pv_RefIE, u16 u16_Reason);
E_CMBS_RC cmbs_api_ie_HANForcefulDeRegErrorReasonGet(void * pv_RefIE, u16* pu16_Reason);

E_CMBS_RC cmbs_api_ie_HANRegErrorReasonAdd(void * pv_RefIE, u16 u16_Reason);
E_CMBS_RC cmbs_api_ie_HANRegErrorReasonGet(void * pv_RefIE, u16* pu16_Reason);

E_CMBS_RC cmbs_api_ie_HANReqIDAdd(void * pv_RefIE, u16 u16_RequestID);
E_CMBS_RC cmbs_api_ie_HANReqIDGet(void * pv_RefIE, u16* pu16_RequestID);

E_CMBS_RC cmbs_api_ie_HANMsgCtlAdd(void * pv_RefIE, PST_IE_HAN_MSG_CTL pst_MessageControl);
E_CMBS_RC cmbs_api_ie_HANMsgCtlGet(void * pv_RefIE, PST_IE_HAN_MSG_CTL pst_MessageControl);

#endif // CMBS_HAN_IE_H
