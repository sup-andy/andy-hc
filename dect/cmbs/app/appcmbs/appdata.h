/*!
*  \file       appdata.h
* \brief
* \Author  stein
*
* @(#) %filespec: appdata.h~DMZD53#5 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/

#if !defined( APPCATIQ_H )
#define APPCATIQ_H


#if defined( __cplusplus )
extern "C"
{
#endif

E_CMBS_RC         app_DataSessionOpen(char * psz_Handset);
E_CMBS_RC         app_DataSend(u16 u16_SessionId, ST_IE_DATA * pst_Data);
E_CMBS_RC         app_DataSessionClose(u16 u16_SessionId);

#if defined( __cplusplus )
}
#endif

#endif //APPCATIQ_H
//*/
