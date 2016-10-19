/*!
*  \file       appmsgparser.h
*  \brief      This file contains internal structures and definitions of CMBS
*
*  \author     podolskiy
*
*  @(#)  %filespec: appmsgparser.h~2 %
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
* 02-Nov-10    podolskiy   1       Initial creation                             \n
*******************************************************************************/
#ifndef APP_MSG_PARSER
#define APP_MSG_PARSER

void    app_IEToString(void *pv_IE, u16 u16_IE);
u32 app_PrintIe2Log(char *pOutput, u32 u32_OutputSize, void *pv_IE, u16 u16_IE);
void app_set_msgparserEnabled(u8 value);
u8  app_get_msgparserEnabled();
char * app_GetAFEEndpointString(E_CMBS_AFE_CHANNEL e_Channel);
char * app_GetCodecString(E_CMBS_AUDIO_CODEC e_Codec);
char * app_GetHWCOMString(E_CMBS_HW_COM_TYPE e_HWComType);
char * app_GetHWChipString(E_CMBS_HW_CHIP e_HWChip);
char * app_GetHWChipVersionString(E_CMBS_HW_CHIP_VERSION e_HWChipVersion);

#endif //APP_MSG_PARSER
