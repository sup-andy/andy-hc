/*!
*   \file       tcxutil.h
*   \brief
*   \author     Dana Kronfeld
*
*   @(#)        tcxutil.h~1
*
*******************************************************************************/

#if !defined( TCXUTIL_H )
#define TCXUTIL_H


void tcx_UARTConfig(u8 u8_Port);
void tcx_USBConfig(u8 u8_Port);
void tcx_TDMConfig(bool TDM_Type);

#ifdef CMBS_COMA
void tcx_COMAConfig(void);
#endif


u8 tcx_DetectComPort(bool interactiveMode, E_CMBS_DEVTYPE * pu8_type);

int   tcx_getch(void);
void  tcx_appClearScreen(void);
int   tcx_gets(char * buffer, int n_Length);

/******************/
/* File Utilities */
/******************/
E_CMBS_RC tcx_fileOpen(FILE **pf_file, u8 *pu8_fileName, const u8 *pu8_mode);
E_CMBS_RC tcx_fileRead(FILE *pf_file, u8 *pu8_OutBuf, u32 u32_Offset, u32 u32_Size);
E_CMBS_RC tcx_fileWrite(FILE *pf_file, u8* pu8_InBuf, u32 u32_Offset, u32 u32_Size);
E_CMBS_RC tcx_fileClose(FILE *pf_file);
E_CMBS_RC  tcx_fileDelete(char *pFileName);
u32   tcx_fileSize(FILE *pf_file);
E_CMBS_RC tcx_scanf(const char *format, ...);
#endif // TCXUTIL_H