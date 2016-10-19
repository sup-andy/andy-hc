/*!
*  \file       tcx_log.h
*  \brief
*  \Author     maurer
*
*  @(#)  %filespec: tcx_log.h~DMZD53#6 %
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
*******************************************************************************/

#if   !defined( TCX_LOG_H )
#define  TCX_LOG_H


#if defined( __cplusplus )
extern "C"
{
#endif

int tcx_LogOpenLogfile(char * psz_LogFile);

void tcx_LogOutgoingPacketPrepare(void);

void tcx_LogOutgoingPacketPartWrite(u16 u16_Event, u8* pu8_Buffer, u16 u16_Size);

void tcx_LogOutgoingPacketWriteFinish(void);

void tcx_LogIncomingPacketWriteFinish(u8* pu8_Buffer, u16 u16_Size);

void tcx_LogWriteLogBuffer(u8* pu8_Buffer, u16 u16_Size);

void tcx_LogCloseLogfile(void);

int tcx_LogOpenTracefile(char * psz_TraceFile);

void tcx_LogCloseTracefile(void);

int  tcx_IsLoggerEnabled(void);

E_CMBS_RC tcx_LogOutputCreate(void);

void tcx_LogOutputDestroy(void);

void tcx_ForcePrintIe(void);

void tcx_PushToPrintQueue(char * pMessage, u16 nLength);

#ifdef WIN32
#define CMBS_TCX_PRINT_MESSAGE  (WM_USER + 1)
#else
#define CMBS_TCX_PRINT_MESSAGE  (0x500)
#endif
#if defined( __cplusplus )
}
#endif

#endif   // TCX_LOG_H
//*/
