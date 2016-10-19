/*!
*  \file       applog.h
* \brief
* \Author    podolskyi
*
* @(#) %filespec: applog.h~1 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/

#if !defined( APPLOG_H )
#define APPLOG_H

#if defined( __cplusplus )
extern "C"
{
#endif
E_CMBS_RC app_LogStartDectLogger(void);
E_CMBS_RC app_LogStoptDectLoggerAndRead(void);
int   app_LogEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData);


#if defined( __cplusplus )
}
#endif

#endif