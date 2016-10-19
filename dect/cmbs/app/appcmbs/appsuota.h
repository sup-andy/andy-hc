/*!
*  \file       appsuota.h
* \brief
* \Author  podolsky
*
* @(#) %filespec: appsuota.h~ILD53#2 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/

#if !defined( APPSUOTA_H )
#define APPSUOTA_H


#if defined( __cplusplus )
extern "C"
{
#endif

E_CMBS_RC         app_SoutaSendHSVersionAvail(ST_SUOTA_UPGRADE_DETAILS  pHSVerAvail, u16 u16_Handset, ST_VERSION_BUFFER* pst_SwVersion, u16 u16_RequestId);
E_CMBS_RC         app_SoutaSendNewVersionInd(u16 u16_Handset, E_SUOTA_SU_SubType enSubType, u16 u16_RequestId);
E_CMBS_RC         app_SoutaSendURL(u16 u16_Handset, u8   u8_URLToFollow,   ST_URL_BUFFER* pst_Url, u16 u16_RequestId);
E_CMBS_RC         app_SoutaSendNack(u16 u16_Handset, E_SUOTA_RejectReason RejectReason, u16 u16_RequestId);

#if defined( __cplusplus )
}
#endif

#endif //APPSUOTA_H
//*/
