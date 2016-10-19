/*!
* \file
* \brief
* \Author  stein
*
* @(#) %filespec: appswup.h~4.1.2 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/

#if !defined( _APPSWUP_H )
#define _APPSWUP_H

#include "cmbs_platf.h"

#if defined( __cplusplus )
extern "C"
{
#endif

typedef enum
{
    FW_UPGRADE_IDLE = 0,
    FW_UPGRADE_FILE_1,
    FW_UPGRADE_FILE_2,
}
E_SWUP_STAGES;

typedef struct
{
    char   file_name1[FILENAME_MAX];
    char   file_name2[FILENAME_MAX];
    u16    u16_Version1;
    u16    u16_Version2;
    u16    u16_PacketSize;
    u16    u16_DurationMinutes;
    char   logFileName[FILENAME_MAX];
    char   booter_name1[FILENAME_MAX];
    char   booter_name2[FILENAME_MAX];
    u16    u16_BooterVersion1;
    u16    u16_BooterVersion2;
}
ST_CMBS_UPGRADE_SETUP;

#define  APP_SWUP_EMPTY_VERSION  (0xFFFF)
#define  APP_SWUP_MAX_PACKETSIZE  (512) // On some USB dongles need to set packet size to 32
#define  APP_SWUP_DEFAULT_PACKETSIZE (32)

E_CMBS_RC app_UpdateAndCheckVersion(const char* pch_FileName, u16 u16_FwVersion);
void  app_SwupStartStressTests(bool isBooter);
void  app_SwupApplicationInvalidate(void);
E_CMBS_RC app_SwupGetImageVersion(char * pFileName, u16* u16_FwVersion);

#if defined( __cplusplus )
}
#endif

#endif // _APPSWUP_H
//*/
