/*!
* \file   cmbs_chl18msg.h
* \brief  header file for chl18msg.c to use it for parsing with minimum changes
* \Author  podolskyi
*
* @(#) %filespec: cmbs_chl18msg.h~1 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/
#ifndef CMBS_CHL18MSG_H
#define CMBS_CHL18MSG_H
#include "cmbs_api.h"

#define  CMBS_DECT_LOG_BUFF_SIZE   (512)
u8 u8_gDectLogPrintBuffer[CMBS_DECT_LOG_BUFF_SIZE];
u16 u16_gDectLogPrintIndex;

#define printf(...) \
{ \
 if(u16_gDectLogPrintIndex <= CMBS_DECT_LOG_BUFF_SIZE)\
{ \
 u16_gDectLogPrintIndex+=sprintf(u8_gDectLogPrintBuffer+u16_gDectLogPrintIndex,__VA_ARGS__);\
}\
}
//////////////////////////////////////////////////////////////////////////
void app_DectLoggerResetOutputBuffer(void)
{
    u16_gDectLogPrintIndex = 0;
    memset(u8_gDectLogPrintBuffer, 0x0, sizeof(u8_gDectLogPrintBuffer));
}
//////////////////////////////////////////////////////////////////////////
u8* app_DectLoggerGetOutputBuffer(u16* pIndex)
{
    * pIndex = u16_gDectLogPrintIndex;
    return u8_gDectLogPrintBuffer;
}

#endif //CMBS_CHL18MSG_H
