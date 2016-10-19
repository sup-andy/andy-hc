/*!
*  \file       tcx_log.c
*  \brief      Logging functionality for the test application
*  \Author     maurer
*
*  @(#)  %filespec: tcx_log.c~12.2.1.1 %
*
*******************************************************************************
*  \par  History
*  \n==== History =============================================================\n
*  date        name     version   action                                       \n
*  ----------------------------------------------------------------------------\n
*   10-jul-09  maurer     1.0       Initial version                            \n
*   14-dec-09  sergiym     ?        Add OsTrace to CMBS log                            \n
*******************************************************************************/
#include <stdio.h>
#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cmbs_dbg.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "tcx_log.h"
#include "cmbs_int.h"
#include "appmsgparser.h"
#include "ctrl_common_lib.h"

/* Trace features */
#define OS08_TRACE
#include "cos08trc.h"   // ..\core\ext\dspg\vegaone\scorpion4\vdsw-ftvone
#define OS_DEBUG 5
#define VDSW_SMS 1
#define VDSW_FEATURES 1
#define cos19_c
/* State names, event names */
#include "bsd09ddl.h"   // ..\core\ext\dspg\vegaone\scorpion4\vdsw-ftvone
#undef cos19_c
#undef VDSW_FEATURES
#undef VDSW_SMS
#undef OS_DEBUG
/* OS Interface */
#include "cos00int.h"   // ..\core\ext\dspg\vegaone\scorpion4\vdsw-ftvone

#ifdef __linux__
#include <pthread.h>
#endif

/* Effi: adding these extern so i can print the Host Version/BUild in the beginning of the Log file */
/* Note: the functions should be in a header file. Moreover, they should be in CMBS API and not in tcx */
extern u16   tcx_ApplVersionGet(void);
extern int   tcx_ApplVersionBuildGet(void);


/* Dummy functions as workaround for result of ddl2c tool */
void FTCSM(void) { }
void FTDSR(void) { }
void FTSS(void) { }
void FTMM(void) { }
void FTMLP(void) { }
void p_hl17_LUXProcess(void) { }
void FTHE(void) { }
void FTCH(void) { }
void FTCC(void) { }
void FTRMC(void) { }
void TRACE_PROCESS(void) { }
void FTMI(void) { }
void CSA(void) { }
void FTMMS(void) { }
void FTCLI(void) { }
void FTLA(void) { }
void TERMINAL(void) { }
void FTTTS(void) { }
void CMBSTASK(void) { }
void FWUP_TASK(void) { }
void FTFWUP(void) { }
void SWUPTASK(void) { }
void SUOTATESTAPP(void) { }
void GMEP(void) { }


FILE *g_fpLogFile = NULL;
FILE *g_fpTraceFile = NULL;

u8  g_u8_tcx_LogOutgoingPacket[1024];
u32 g_u32_tcx_LogOutgoingPacket_WriteCounter;

u8 G_u8_tcx_TraceBuffer_InUse = 0;
u8 G_u8_tcx_TraceBuffer_Pos = 0;
u8 G_u8_tcx_TraceBuffer_Size = 0;
u8 G_u8_tcx_TraceBuffer[255];

u32 G_u32_tcx_TraceTime = 0;
u16 G_u16_tcx_TraceTickPrev = 0;

typedef enum
{
    CMBS_LOGGER_INCOMING_MSG,
    CMBS_LOGGER_OUTGOING_MSG,
}
CMBS_LOGGER_DIRECTION_e;

// Effi change from 10 entries to 100
#define    CMBS_PRINT_FIFO_SIZE  100

CFR_CMBS_CRITICALSECTION G_hPrintingCriticalSection;
#ifdef WIN32
HANDLE      G_tcx_hLoggerThread   = NULL;
ULONG      G_tcx_LoggerThreadId  = 0;
DWORD txc_LoggerPrintThread(LPVOID lpThreadParameter);
#elif __linux__
pthread_t     G_tcx_hLoggerThread = 0;
void *print_message_function(void *ptr);
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define CMBS_MAX_LOG_MESSAGE   4000

typedef struct
{
    // Effi - Log of a complete message can be bigger than IE
    // char message[CMBS_MAX_IE_PRINT_SIZE];
    char message[CMBS_MAX_LOG_MESSAGE];
    u16  length;
}
CMBS_IE_print_msg;
//////////////////////////////////////////////////////////////////////////
typedef struct
{
    CMBS_IE_print_msg messages[CMBS_PRINT_FIFO_SIZE];
    u8 nReadMsg;
    u8 nWriteMsg;
    u8 nCount;
}
CMBS_G_tcx_ie_print_queue;
//////////////////////////////////////////////////////////////////////////
CMBS_G_tcx_ie_print_queue G_tcx_ie_print_queue;
//////////////////////////////////////////////////////////////////////////

void tcx_PushToPrintQueue(char *pMessage, u16 nLength)
{
    CFR_CMBS_ENTER_CRITICALSECTION(G_hPrintingCriticalSection);
    // Effi, there is a check for the length, giong to use CMBS_MAX_LOG_MESSAGE instead of CMBS_MAX_IE_PRINT_SIZE
    if ((G_tcx_ie_print_queue.nCount < CMBS_PRINT_FIFO_SIZE) && (nLength < CMBS_MAX_LOG_MESSAGE))
    {
        CMBS_IE_print_msg *pMsg = &G_tcx_ie_print_queue.messages[G_tcx_ie_print_queue.nWriteMsg];
        memcpy(pMsg->message, pMessage, nLength);
        pMsg->length = nLength;
        G_tcx_ie_print_queue.nCount++;

        // increment write index
        G_tcx_ie_print_queue.nWriteMsg++;
        if (G_tcx_ie_print_queue.nWriteMsg >= CMBS_PRINT_FIFO_SIZE)
            G_tcx_ie_print_queue.nWriteMsg = 0;

    }
    CFR_CMBS_LEAVE_CRITICALSECTION(G_hPrintingCriticalSection);
}
//////////////////////////////////////////////////////////////////////////
u16 tcx_PopFromPrintQueue(char **pMessage)
{
    CMBS_IE_print_msg *pMsg;
    CFR_CMBS_ENTER_CRITICALSECTION(G_hPrintingCriticalSection);
    pMsg = &G_tcx_ie_print_queue.messages[G_tcx_ie_print_queue.nReadMsg];
    if (G_tcx_ie_print_queue.nCount)
    {
        *pMessage = pMsg->message;
        G_tcx_ie_print_queue.nCount--;
        // increment read index
        G_tcx_ie_print_queue.nReadMsg++;
        if (G_tcx_ie_print_queue.nReadMsg >= CMBS_PRINT_FIFO_SIZE)
            G_tcx_ie_print_queue.nReadMsg = 0;
    }
    else
    {
        *pMessage = NULL;
        pMsg->length = 0;
    }
    CFR_CMBS_LEAVE_CRITICALSECTION(G_hPrintingCriticalSection);

    return pMsg->length;
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC tcx_LogOutputCreate(void)
{
    CFR_CMBS_INIT_CRITICALSECTION(G_hPrintingCriticalSection);
#ifdef WIN32
    G_tcx_hLoggerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)txc_LoggerPrintThread, 0, 0, &G_tcx_LoggerThreadId);
    if (NULL == G_tcx_hLoggerThread)
    {
        return CMBS_RC_ERROR_GENERAL;
    }
#elif __linux__
    {
        int i_result = pthread_create(&G_tcx_hLoggerThread, NULL, print_message_function, (void*) NULL);
        if (0 != i_result)
        {
            return CMBS_RC_ERROR_GENERAL;
        }
    }
#endif

    return CMBS_RC_OK;
}
//////////////////////////////////////////////////////////////////////////
void tcx_LogOutputDestroy(void)
{
#ifdef WIN32
    if (G_tcx_hLoggerThread)
    {
        TerminateThread(G_tcx_hLoggerThread, 0);
        G_tcx_hLoggerThread = NULL;
    }
#endif
    CFR_CMBS_DELETE_CRITICALSECTION(G_hPrintingCriticalSection);
}
//////////////////////////////////////////////////////////////////////////
void tcx_ForcePrintIe(void)
{
#ifdef WIN32
    if (G_tcx_LoggerThreadId != 0)
    {
        PostThreadMessage(G_tcx_LoggerThreadId, CMBS_TCX_PRINT_MESSAGE, 0, 0);
    }
#elif __linux__
    pthread_mutex_lock(&condition_mutex);
    pthread_cond_signal(&condition_cond);
    pthread_mutex_unlock(&condition_mutex);
#endif

}

//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
DWORD txc_LoggerPrintThread(LPVOID lpThreadParameter)
{
    MSG msg;

    while (1)
    {
        if (GetMessage(&msg, NULL, 0, 0))
        {
            char *pMessage = NULL;
            u16 nLength;
            nLength = tcx_PopFromPrintQueue(&pMessage);
            if (pMessage)
            {
                pMessage[nLength] = 0;
                //////////////////////////////////////////////////
                // Effi :
                //   if the message starts with "FILE:" write it to the log file
                //   else continue writing to the console
                if (!strncmp(pMessage, "FILE:", 5) && g_fpLogFile)
                {
                    // write to log file without the "FILE:"
                    fwrite(pMessage + 5, 1, nLength - 5, g_fpLogFile);
                    fflush(g_fpLogFile);
                }
                else // console
                    printf("%s", pMessage);
            }
        }

    }
}
#elif __linux__
void *print_message_function(void *ptr)
{
    UNUSED_PARAMETER(ptr);
    while (1)
    {
        char *pMessage;
        u16 nLength;

        pthread_mutex_lock(&condition_mutex);
        pthread_cond_wait(&condition_cond, &condition_mutex);
        pthread_mutex_unlock(&condition_mutex);

        do
        {
            nLength = tcx_PopFromPrintQueue(&pMessage);
            if (pMessage)
            {
                pMessage[nLength] = 0;
                if (!strncmp(pMessage, "FILE:", 5) && g_fpLogFile)
                {
                    // write to log file without the "FILE:"
                    fwrite(pMessage + 5, 1, nLength - 5, g_fpLogFile);
                    fflush(g_fpLogFile);
                }
                else
                {
                    printf("%s", pMessage);
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "%s",pMessage);
                }
            }
        } while (nLength != 0);
    }
}
#endif

//    ========== tcx_LogOpenLogfile ===========
/*!
      \brief             Open the logfile
      \param[in]         pszLogFile The name of the logfile
      \return            1 if successful, 0 if there was an error

*/

int tcx_LogOpenLogfile(char *psz_LogFile)
{
#ifdef WIN32
    SYSTEMTIME stSystemTime;
    u16 u16_Pos;
    char strOutBuffer[100];
#endif

    g_fpLogFile = fopen(psz_LogFile, "w");

    if (g_fpLogFile == NULL)
    {
        return 0;
    }

    // get local time
#ifdef WIN32
    GetLocalTime(&stSystemTime);
    u16_Pos = sprintf(strOutBuffer, "DSP Group-Logfile\nLog started at: %02d-%02d-%d  %02d:%02d\n", stSystemTime.wDay,
                      stSystemTime.wMonth, stSystemTime.wYear, stSystemTime.wHour, stSystemTime.wMinute);
    fwrite(strOutBuffer, 1, u16_Pos, g_fpLogFile);
    /* Effi - print the version as well */
    u16_Pos = sprintf(strOutBuffer, "Version %02x.%02x - Build %d\n", (tcx_ApplVersionGet() >> 8), (tcx_ApplVersionGet() & 0xFF), tcx_ApplVersionBuildGet());
    fwrite(strOutBuffer, 1, u16_Pos, g_fpLogFile);

#endif
    fflush(g_fpLogFile);

    return 1;
}


//    ========== tcx_LogOpenTracefile ===========
/*!
      \brief             Open the tracefile
      \param[in]         pszTraceFile The name of the tracefile
      \return            1 if successful, 0 if there was an error

*/
int tcx_LogOpenTracefile(char *psz_TraceFile)
{
    g_fpTraceFile = fopen(psz_TraceFile, "wb");

    if (g_fpTraceFile == NULL)
    {
        return 0;
    }
    return 1;
}


//    ========== tcx_LogutgoingPacketPrepare ===========
/*!
      \brief             Prepare to write an outgoing packet
      \return            none

*/
void tcx_LogOutgoingPacketPrepare(void)
{
    g_u32_tcx_LogOutgoingPacket_WriteCounter = 0;
}

// Effi added timestamp for windows only
#ifdef WIN32
#include <time.h>
#endif

void tcx_LogWriteEvent(u16 u16_Event, ST_CFR_IE_LIST *pList, u8 *pu8_Buffer,
                       u16 u16_Length, CMBS_LOGGER_DIRECTION_e direction)
{
    //EFfi use a define that is also used for the message queue
    char   tcx_LogPacketString[CMBS_MAX_LOG_MESSAGE];
    //
    u32    u32_Pos = 0, u32_i = 0;
    void   *pv_IE = NULL;
    u16    u16_IE;
    // Effi added timestamp for windows only
#ifdef WIN32
    SYSTEMTIME stSystemTime;
    time_t curTime;
    int curTimeInteger;
#endif
    // check that log file is valid
    if (NULL == g_fpLogFile)
        return;

    // Effi - adding prefix FILE: so the thread of log knows to print to file and not console
    u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "FILE:");

    // Effi Added Timestamp for windows only
#ifdef WIN32
    curTime = time(NULL);
    curTimeInteger = (int)curTime;
    GetLocalTime(&stSystemTime);
    u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "%02d-%02d-%d  %02d:%02d:%02d:%03d (%d%d)\n", stSystemTime.wDay, stSystemTime.wMonth,
                       stSystemTime.wYear, stSystemTime.wHour, stSystemTime.wMinute, stSystemTime.wSecond, stSystemTime.wMilliseconds, curTimeInteger, stSystemTime.wMilliseconds);
#endif

    if (direction == CMBS_LOGGER_INCOMING_MSG)
        u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "Target ---> Host:  ");
    else
        u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "Host ---> Target:  ");


    if ((NULL == pu8_Buffer) || (0 == u16_Length))
    {
        // effi - different treatment for commands
        if ((u16_Event & CMBS_CMD_MASK) == CMBS_CMD_MASK)
            u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "\n {%s(%d)}\n\n", cmbs_dbg_GetCommandName(u16_Event & ~CMBS_CMD_MASK), u16_Event);
        else
            u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "\n {%s(%d)}\n\n", cmbs_dbg_GetEventName(u16_Event), u16_Event);
    }
    else
    {
        for (u32_i = 0; u32_i < u16_Length; u32_i++)
        {
            u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, " %02X", pu8_Buffer[u32_i]);
        }
        u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "\n");

        // effi - different treatment for commands
        if ((u16_Event & CMBS_CMD_MASK) == CMBS_CMD_MASK)
            u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "{%s(%d)}\n\n", cmbs_dbg_GetCommandName(u16_Event & ~CMBS_CMD_MASK), u16_Event);
        else
        {
            u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "{%s(%d)}\n", cmbs_dbg_GetEventName(u16_Event), u16_Event);

            // IE only in events, not in commands
            cmbs_api_ie_GetFirst((void *)pList, &pv_IE, &u16_IE);
            while (pv_IE != NULL)
            {
                u32_Pos += app_PrintIe2Log(tcx_LogPacketString + u32_Pos, sizeof(tcx_LogPacketString) - u32_Pos, pv_IE, u16_IE);
                cmbs_api_ie_GetNext((void *)pList, &pv_IE, &u16_IE);
            }

            u32_Pos += sprintf(tcx_LogPacketString + u32_Pos, "\n");
        }
    }

    // Effi - push to queue instead of writing to file
    tcx_PushToPrintQueue(tcx_LogPacketString, u32_Pos);
    tcx_ForcePrintIe(); // this causes to send message to thread
    //fwrite(tcx_LogPacketString, 1, u32_Pos, g_fpLogFile);
    //fflush(g_fpLogFile);
}


//    ========== tcx_LogOutgoingPacketPartWrite ===========
/*!
      \brief             Collect part of the packet, to write them in to the logfile later
      \param[in]         Pointer to part of packet
      \param[in]         Length of the part of the packet
      \return            none

*/

void tcx_LogOutgoingPacketPartWrite(u16 u16_Event, u8 *pu8_Buffer, u16 u16_Length)
{
    ST_CFR_IE_LIST  List;
    List.pu8_Buffer = pu8_Buffer;
    List.u16_CurSize = u16_Length;
    List.u16_MaxSize = CMBS_BUF_SIZE;
    List.u16_CurIE = 0;

    tcx_LogWriteEvent(u16_Event, &List, pu8_Buffer, u16_Length, CMBS_LOGGER_OUTGOING_MSG);

}


//    ========== tcx_LogOutgoingPacketWriteFinish ===========
/*!
      \brief             Write collected outgoing data to logfile
      \return            none

*/
void tcx_LogOutgoingPacketWriteFinish(void)
{
    char tcx_LogOutgoingPacketString[CMBS_MAX_LOG_MESSAGE];
    u32  u32_pos;
    u32  u32_i;

    if (g_fpLogFile != NULL)
    {
        // Effi - add the FILE: and send to queue
        u32_pos = sprintf(tcx_LogOutgoingPacketString, "FILE:");

        for (u32_i = 0; u32_i < g_u32_tcx_LogOutgoingPacket_WriteCounter; u32_i++)
        {
            u32_pos += sprintf(tcx_LogOutgoingPacketString + u32_pos, " %02X", g_u8_tcx_LogOutgoingPacket[u32_i]);
        }

        u32_pos += sprintf(tcx_LogOutgoingPacketString + u32_pos, "\n");

        // Effi - push to queue instead of printing to file now
        tcx_PushToPrintQueue(tcx_LogOutgoingPacketString, u32_pos);
        tcx_ForcePrintIe();
        //fwrite(tcx_LogOutgoingPacketString, 1, u32_pos, g_fpLogFile);
        //fflush(g_fpLogFile);
    }

}


//    ========== tcx_LogIncomingPacketWriteFinish ===========
/*!
      \brief             Write complete incoming data packet to logfile
      \return            none

*/

void tcx_LogIncomingPacketWriteFinish(u8 *pu8_Buffer, u16 u16_Size)
{
    PST_CMBS_SER_MSG p_Mssg = (PST_CMBS_SER_MSG)pu8_Buffer;
    ST_CFR_IE_LIST List;

    List.pu8_Buffer = (u8 *)p_Mssg->u8_Param;
    List.u16_CurIE  = 0;
    List.u16_CurSize = u16_Size - sizeof(ST_CMBS_SER_MSGHDR);
    List.u16_MaxSize = u16_Size - sizeof(ST_CMBS_SER_MSGHDR);

    tcx_LogWriteEvent(p_Mssg->st_MsgHdr.u16_EventID, &List, pu8_Buffer, u16_Size, CMBS_LOGGER_INCOMING_MSG);
}


//    ========== tcx_TraceTimeUpdate ===========
/*!
      \brief             Update trace time
      \return            none

*/
void tcx_TraceTimeUpdate(u16 u16_tick)
{
    if (G_u16_tcx_TraceTickPrev <= u16_tick)
    {
        G_u32_tcx_TraceTime += u16_tick - G_u16_tcx_TraceTickPrev;
    }
    else
    {
        G_u32_tcx_TraceTime += 0x00010000 + u16_tick - G_u16_tcx_TraceTickPrev;
    }
    G_u16_tcx_TraceTickPrev = u16_tick;
}


//    ========== tcx_TraceTimeUpdate_u8 ===========
/*!
      \brief             Update trace time
      \return            none

*/
void tcx_TraceTimeUpdate_u8(u8 u8_tick)
{
    u16 u16_ticks = (G_u16_tcx_TraceTickPrev & ~0x00FF) | u8_tick;
    if (u16_ticks < G_u16_tcx_TraceTickPrev)
    {
        u16_ticks += 0x0100;
    }
    tcx_TraceTimeUpdate(u16_ticks);
}


//    ========== tcx_TraceTimeToStr ===========
/*!
      \brief             Write trace time to string
      \return            none

*/
u32 tcx_TraceTimeToStr(char *pc_str)
{
    u8 u8_ms, u8_sec, u8_min;
    u32 u32_time = G_u32_tcx_TraceTime;
    u8_ms = u32_time % 100;
    u32_time /= 100;
    u8_sec = u32_time % 60;
    u32_time /= 60;
    u8_min = u32_time % 60;
    u32_time /= 60;
    return sprintf(pc_str, "%2d:%02d:%02d.%02d ", u32_time, u8_min, u8_sec, u8_ms);
}


//    ========== tcx_TraceBufferToStr ===========
/*!
      \brief             Write trace buffer entries to string
      \return            none

*/
u32 tcx_TraceBufferToStr(char *pc_str, u8 *pu8_Buffer, u8 u8_Size)
{
    t_st_os08_TraceBuffer *p_buf;
    u32  u32_pos = 0;
    u8   u8_i;

    p_buf = (t_st_os08_TraceBuffer *)pu8_Buffer;
    switch (p_buf->u8_TraceType)
    {
        case OS08_IF:
        {
            break;
        }
        case OS08_CASE:
        {
            break;
        }
        case OS08_TASK:
        {
            break;
        }
        case OS08_OUTPUT:
        {
            break;
        }
        case OS08_OUTIPC:
        {
            break;
        }
        case OS08_TIMER:
        {
            break;
        }
        case OS08_SDL_CALL:
        {
            break;
        }
        case OS08_SDL_RET:
        {
            break;
        }
        case OS08_PSI:
        {
            if (OS08_SIZEOF_TRACE(p_buf->un_Payload.st_PSI) == u8_Size)
            {
                tcx_TraceTimeUpdate(p_buf->un_Payload.st_PSI.u16_SystemTime);
                u32_pos += tcx_TraceTimeToStr(pc_str + u32_pos);
                u32_pos += sprintf(pc_str + u32_pos, "RCV-%c   p-%d/%-16s  s-%-21s  ev-%-30s  info-%04X",
                                   (p_buf->u8_Process < OS09_1ST_LOW_PRIO_PROCESS) ? 'H' : 'L',
                                   p_buf->un_Payload.st_PSI.u8_Instance,
                                   G_pu8_os09_ProcessNames[p_buf->u8_Process],
                                   G_pu8_os09_StateNames[p_buf->u8_Process][p_buf->un_Payload.st_PSI.u8_State],
                                   G_pu8_os09_EventNames[p_buf->u8_Process][p_buf->un_Payload.st_PSI.u8_Event],
                                   p_buf->un_Payload.st_PSI.u16_XInfo);
            }
            break;
        }
        case OS08_EXIT:
        {
            if (OS08_SIZEOF_TRACE(p_buf->un_Payload.st_Exit) == u8_Size)
            {
                tcx_TraceTimeUpdate(p_buf->un_Payload.st_Exit.u16_SystemTime);
                u32_pos += tcx_TraceTimeToStr(pc_str + u32_pos);
                if (p_buf->u8_Process == OS08_IDLE_PROCESS_ID)
                {
                    u32_pos += sprintf(pc_str + u32_pos, "EXIT    *idle*");
                }
                else
                {
                    u32_pos += sprintf(pc_str + u32_pos, "EXIT-%c  p-%-18s  s-%s",
                                       (p_buf->u8_Process < OS09_1ST_LOW_PRIO_PROCESS) ? 'H' : 'L',
                                       G_pu8_os09_ProcessNames[p_buf->u8_Process],
                                       G_pu8_os09_StateNames[p_buf->u8_Process][p_buf->un_Payload.st_Exit.u8_State]);
                }
            }
            break;
        }
        case OS08_EXCEPTION:
        {
            break;
        }
        case OS08_MSG_CONTENTS:
        {
            break;
        }
        case OS08_DUMP:
        {
            break;
        }
        case OS08_FUNCTION_CALL:
        {
            if (OS08_SIZEOF_TRACE(p_buf->un_Payload.st_FuncCall) <= u8_Size)
            {
                tcx_TraceTimeUpdate_u8(p_buf->un_Payload.st_FuncCall.u8_Time);  // func sequence completes within 256 ticks (because of WatchDog)
                u32_pos += tcx_TraceTimeToStr(pc_str + u32_pos);
                u32_pos += sprintf(pc_str + u32_pos, "func                          s-%-21s  ev-%-30s ",
                                   G_pu8_os09_StateNames[p_buf->u8_Process][p_buf->un_Payload.st_FuncCall.u8_State],
                                   G_pu8_os09_EventNames[p_buf->u8_Process][p_buf->un_Payload.st_FuncCall.u8_Event]);
                if (p_buf->un_Payload.st_FuncCall.u8_NrOfBytes + OS08_SIZEOF_TRACE(p_buf->un_Payload.st_FuncCall) == u8_Size)
                {
                    for (u8_i = OS08_SIZEOF_TRACE(p_buf->un_Payload.st_FuncCall); u8_i < u8_Size; ++u8_i)
                    {
                        u32_pos += sprintf(pc_str + u32_pos, " %02X", pu8_Buffer[u8_i]);
                    }
                }
            }
            break;
        }
        case OS08_POINT:
        {
            if (OS08_SIZEOF_TRACE(p_buf->un_Payload.st_TracePoint) == u8_Size)
            {
                tcx_TraceTimeUpdate(p_buf->un_Payload.st_TracePoint.u16_SystemTime);
                u32_pos += tcx_TraceTimeToStr(pc_str + u32_pos);
                u32_pos += sprintf(pc_str + u32_pos, "point                                                  ev-%-30s  info-%04X",
                                   G_pu8_os09_EventNames[p_buf->u8_Process][p_buf->un_Payload.st_TracePoint.u8_Event],
                                   p_buf->un_Payload.st_TracePoint.u16_XInfo);
            }
            break;
        }
        case OS08_MSG_PUT:
        {
            if (OS08_SIZEOF_TRACE(p_buf->un_Payload.st_MsgPut) == u8_Size)
            {
                tcx_TraceTimeUpdate(p_buf->un_Payload.st_MsgPut.u16_SystemTime);
                u32_pos += tcx_TraceTimeToStr(pc_str + u32_pos);
                if (p_buf->un_Payload.st_MsgPut.u8_SrcProc == OS00_ILLEGAL_PROCESS)
                {
                    u32_pos += sprintf(pc_str + u32_pos, "SEND-%c  *ISR*/%-14d --> d-%d/%-16s  ev-%-30s  info-%04X",
                                       (p_buf->un_Payload.st_MsgPut.u8_Instance & OS00_LIFO_MSG) ? 'H' : 'L',
                                       p_buf->un_Payload.st_MsgPut.u8_SrcInst, //todo: isr number
                                       p_buf->un_Payload.st_MsgPut.u8_Instance & ~OS00_LIFO_MSG,
                                       G_pu8_os09_ProcessNames[p_buf->u8_Process],
                                       G_pu8_os09_EventNames[p_buf->u8_Process][p_buf->un_Payload.st_MsgPut.u8_Event],
                                       p_buf->un_Payload.st_MsgPut.u16_XInfo);
                }
                else
                {
                    u32_pos += sprintf(pc_str + u32_pos, "SEND-%c  p-%d/%-16s --> d-%d/%-16s  ev-%-30s  info-%04X",
                                       (p_buf->un_Payload.st_MsgPut.u8_Instance & OS00_LIFO_MSG) ? 'H' : 'L',
                                       p_buf->un_Payload.st_MsgPut.u8_SrcInst,
                                       G_pu8_os09_ProcessNames[p_buf->un_Payload.st_MsgPut.u8_SrcProc],
                                       p_buf->un_Payload.st_MsgPut.u8_Instance & ~OS00_LIFO_MSG,
                                       G_pu8_os09_ProcessNames[p_buf->u8_Process],
                                       G_pu8_os09_EventNames[p_buf->u8_Process][p_buf->un_Payload.st_MsgPut.u8_Event],
                                       p_buf->un_Payload.st_MsgPut.u16_XInfo);
                }
            }
            break;
        }
    }
    return u32_pos;
}


//    ========== tcx_LogWriteLogBuffer ===========
/*!
      \brief             Write log buffer entries as string into logfile
      \return            none

*/
void tcx_LogWriteLogBuffer(u8 *pu8_Buffer, u16 u16_Size)
{
    char tcx_LogBufferString[16 + 1024 * 3];
    u32  u32_pos;
    u32  u32_i;
    u8   c;
    // for debug
    //u8   u8_ii;

    u32_pos = sprintf(tcx_LogBufferString, "LOGBUFFER> ");

    for (u32_i = 0; u32_i < u16_Size; ++u32_i)
    {
        c = pu8_Buffer[u32_i];

        if (G_u8_tcx_TraceBuffer_InUse)
        {
            if (G_u8_tcx_TraceBuffer_Size == 0)
            {
                if (c == 0)
                {
                    u32_pos += sprintf(tcx_LogBufferString + u32_pos, "\nLOGBUFFER> * * * * * BUFFER OVERFLOW * * * * *");
                }
                else
                {
                    G_u8_tcx_TraceBuffer_Size = c;
                }
            }
            else
            {
                G_u8_tcx_TraceBuffer[G_u8_tcx_TraceBuffer_Pos] = c;
                if (++G_u8_tcx_TraceBuffer_Pos == G_u8_tcx_TraceBuffer_Size)
                {
                    // for debug
                    //u32_pos += sprintf(tcx_LogBufferString + u32_pos, "\nTrcBuf> ");
                    //for (u8_ii = 0; u8_ii < G_u8_tcx_TraceBuffer_Size; ++u8_ii)
                    //{
                    //   c = G_u8_tcx_TraceBuffer[u8_ii];
                    //   u32_pos += sprintf(tcx_LogBufferString + u32_pos, "(%02X)", c);
                    //}

                    u32_pos += sprintf(tcx_LogBufferString + u32_pos, "\nOsTrace> ");
                    u32_pos += tcx_TraceBufferToStr(tcx_LogBufferString + u32_pos, G_u8_tcx_TraceBuffer, G_u8_tcx_TraceBuffer_Size);

                    if (g_fpTraceFile != NULL)
                    {
                        fwrite(G_u8_tcx_TraceBuffer, 1, G_u8_tcx_TraceBuffer_Size, g_fpTraceFile);
                    }

                    G_u8_tcx_TraceBuffer_InUse = 0;
                    G_u8_tcx_TraceBuffer_Pos = 0;
                    G_u8_tcx_TraceBuffer_Size = 0;
                }
            }
        }
        else
        {
            if (c == 0)
                G_u8_tcx_TraceBuffer_InUse = 1;
            else if (c >= 0x20 && c <= 0x7e)
                u32_pos += sprintf(tcx_LogBufferString + u32_pos, "%c", c);
            else if (c == 0x0a)
                u32_pos += sprintf(tcx_LogBufferString + u32_pos, "\nLOGBUFFER> ");
            else
                u32_pos += sprintf(tcx_LogBufferString + u32_pos, "(%02X)", c);
        }

    }

    u32_pos += sprintf(tcx_LogBufferString + u32_pos, "\n");
    /*if(g_fpLogFile != NULL)
    {
       fwrite(tcx_LogBufferString, 1, u32_pos, g_fpLogFile);
       fflush(g_fpLogFile);
    }*/
    if (g_fpTraceFile != NULL)
    {
        fflush(g_fpTraceFile);
    }
}


//    ========== tcx_LogCloseLogfile ===========
/*!
      \brief             Close log file
      \return            none

*/
void tcx_LogCloseLogfile(void)
{
    if (g_fpLogFile != NULL)
    {
        fclose(g_fpLogFile);
        g_fpLogFile = NULL;
    }
}

//    ========== tcx_LogCloseTracefile ===========
/*!
      \brief             Close trace file
      \return            none

*/
void tcx_LogCloseTracefile(void)
{
    if (g_fpTraceFile != NULL)
    {
        fclose(g_fpTraceFile);
        g_fpTraceFile = NULL;
    }
}
//////////////////////////////////////////////////////////////////////////
int tcx_IsLoggerEnabled(void)
{
    return (g_fpLogFile != NULL);
}
//////////////////////////////////////////////////////////////////////////
//*/
