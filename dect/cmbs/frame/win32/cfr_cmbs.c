/*!
*  \file       cfr_cmbs.c
*  \brief      Win32 implementation of cmbs internal functions
*  \author     CMBS Team
*
*  @(#)  %filespec: cfr_cmbs.c~DMZD53#11 %
*
*******************************************************************************/

#include "cmbs_int.h"   /* internal API structure and defines */
#include "cfr_uart.h"   /* packet handler */
#include "cfr_debug.h"  /* debug handling */

DWORD WINAPI _cmbs_int_CbThread(LPVOID pvoid);

/* GLOBALS */
ST_CMBS_API_INST  g_CMBSInstance;                   // global CMBS instance object

//  ========== _cmbs_int_StartupBlockSignal ===========
/*!
    \brief   signal to block statement that CMBS is available
    \param[in]  pst_CMBSInst   pointer to CMBS instance object
    \return   <none>
*/
void              _cmbs_int_StartupBlockSignal(PST_CMBS_API_INST pst_CMBSInst)
{
    // delay reply for 1 sec, if it occurs before we start to wait for it
    if (pst_CMBSInst->h_InitBlock == NULL)
    {
        Sleep(1000);
    }

    if (pst_CMBSInst->h_InitBlock)
    {
        SetEvent(pst_CMBSInst->h_InitBlock);
    }
}

//  ========== _cmbs_int_MsgQCreate ===========
/*!
    \brief    Create a message queue
    \param[in,out]  < none >
    \return    < int > return identifier of queue. If there was an error, a value of -1 is returned.
    \note    win32 implementation not needed
*/
int               _cmbs_int_MsgQCreate(void)
{
    return 1;
}

//  ========== _cmbs_int_MsgQDestroy ===========
/*!
    \brief    Destroy message queue
    \param[in]    nMsgQId     message queue identifier
    \return    < none >
    \note    win32 implementation not needed
*/
void              _cmbs_int_MsgQDestroy(int nMsgQId)
{
    if (nMsgQId)
    {};
}

//  ========== cmbs_int_EnvCreate ===========
/*!
    \brief    build up the environment of CMBS-API. Open the relevant devices and starts the pumps.
    \param[in,out]  e_Mode          to be used CMBS mode, currently only CMBS Multiline is supported
    \param[in,out]  pst_DevCtl  pointer to device call control properties
    \param[in,out]  pst_DevMedia pointer to device media control properties
    \return    < E_CMBS_RC >
*/
E_CMBS_RC cmbs_int_EnvCreate(E_CMBS_API_MODE e_Mode, ST_CMBS_DEV *pst_DevCtl, ST_CMBS_DEV *pst_DevMedia)
{
    memset(&g_CMBSInstance, 0, sizeof(g_CMBSInstance));

    // initialize the device control
    if (!pst_DevCtl)
    {
        CFR_DBG_ERROR("[ERROR] cmbs_int_EnvCreate: Device type is not specified\n");
        return CMBS_RC_ERROR_PARAMETER;
    }

    CFR_CMBS_INIT_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
    CFR_CMBS_INIT_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);

    g_CMBSInstance.u32_CallInstanceCount = 0x80000000;
    g_CMBSInstance.e_Mode  = e_Mode;  // useful later, if the API is connected to target side.
    g_CMBSInstance.e_Endian  = cmbs_int_EndiannessGet();
    g_CMBSInstance.eDevCtlType = pst_DevCtl->e_DevType;
    g_CMBSInstance.eDevMediaType = pst_DevMedia->e_DevType;
    g_CMBSInstance.h_RecPath = CreateEvent(NULL, FALSE, FALSE, "CMBS_REC Path");
    g_CMBSInstance.bo_Run  = TRUE;
    g_CMBSInstance.h_RecThread = CreateThread(NULL, 0, _cmbs_int_CbThread, (DWORD *)&g_CMBSInstance, 0, &g_CMBSInstance.dw_ThreadID);

    switch (pst_DevCtl->e_DevType)
    {
        case CMBS_DEVTYPE_UART:
        case CMBS_DEVTYPE_USB:      // cfr_uart is used also for USB
            if (cfr_uartInitialize(pst_DevCtl->u_Config.pUartCfg) == -1)
            {
                CFR_DBG_ERROR("[ERROR] cmbs_int_EnvCreate: UART initialization failed\n");
            }
            else
            {
                // everything looks ok
                return CMBS_RC_OK;
            }
            break;

        default:
            CFR_DBG_ERROR("[ERROR] cmbs_int_EnvCreate: Specified device type is not supported now\n");
            break;
    }

    // clean up
    CFR_CMBS_DELETE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
    CFR_CMBS_DELETE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);

    return CMBS_RC_ERROR_GENERAL;
}

//  ========== cmbs_int_WaitForResponse ===========
/*!
    \brief    Waits for target response using timeout in ms
    \param[in,out]  u32_TimeoutMs   waiting timeout in ms
    \return    < E_CMBS_RC >
*/
E_CMBS_RC cmbs_int_WaitForResponse(u32 u32_TimeoutMs)
{
    // wait until CMBS is connected, value is in ms
    g_CMBSInstance.h_InitBlock = CreateEvent(NULL, FALSE, FALSE, "CMBS Init Block");
    if (WaitForSingleObject(g_CMBSInstance.h_InitBlock, u32_TimeoutMs) == WAIT_TIMEOUT)
    {
        CFR_DBG_ERROR("[ERROR] cmbs_int_WaitForResponse: There is no response from target\n");
        return CMBS_RC_ERROR_OPERATION_TIMEOUT;
    }

    return CMBS_RC_OK;
}

//  ========== cmbs_int_EnvDestroy ===========
/*!
    \brief    clean up the CMBS environment
    \param[in,out]  < none >
    \return    < E_CMBS_RC >
*/
E_CMBS_RC         cmbs_int_EnvDestroy(void)
{
    PST_CMBS_API_INST pst_CMBSInst = &g_CMBSInstance;

    cfr_uartClose();

    CFR_CMBS_DELETE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
    CFR_CMBS_DELETE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);

    if (pst_CMBSInst->h_InitBlock)
    {
        CloseHandle(pst_CMBSInst->h_InitBlock);
        pst_CMBSInst->h_InitBlock = NULL;
    }

    if (pst_CMBSInst->h_RecPath)
    {
        CloseHandle(pst_CMBSInst->h_RecPath);
        pst_CMBSInst->h_RecPath = NULL;

    }

    // stop pipe thread
    g_CMBSInstance.bo_Run = FALSE;

    while (pst_CMBSInst->dw_ThreadID != 0)
    {
        Sleep(100);
    }

    return   CMBS_RC_OK;
}

int               _cmbs_int_PackageCollector(U_CMBS_SER_DATA *pst_Package, int nDataIndex)
{
    u16 u16_Total;

    do
    {
        // validate syc dword
        if (nDataIndex >= sizeof(u32))
        {
            u32 u32_Sync = CMBS_SYNC;

            if (memcmp(pst_Package->serialBuf, &u32_Sync, sizeof(u32)) != 0)
            {
                CFR_DBG_ERROR("[ERROR]CB Thread: !!!! msgrcv ERROR: NO sync word detected\n");

                // Skip first byte and do perhaps a new loop
                memcpy(pst_Package->serialBuf, pst_Package->serialBuf + 1, nDataIndex - 1);

                nDataIndex--;

                continue;
            }
        }

        // check cmbs message length
        if (nDataIndex < sizeof(u32) + sizeof(u16))   // sizeof(u32_Sync) + sizeof(u16_TotalLength)
        {
            // Too less data to calculate message length, leave loop and collect more data
            break;
        }

        if (g_CMBSInstance.e_Endian == E_CMBS_ENDIAN_BIG)
        {
            u16_Total = cmbs_int_EndianCvt16(pst_Package->st_Data.st_Msg.st_MsgHdr.u16_TotalLength);
        }
        else
        {
            u16_Total = pst_Package->st_Data.st_Msg.st_MsgHdr.u16_TotalLength;
        }

        if (nDataIndex < (int)sizeof(pst_Package->st_Data.u32_Sync) + u16_Total)
        {
            // not enough data for complete cmbs message, leave loop and collect more data
            break;
        }

        // now: at least one complete message in receive buffer

        // we assume that cmbs message is complete

        if (g_CMBSInstance.e_Endian == E_CMBS_ENDIAN_BIG)
        {
            cmbs_int_HdrEndianCvt(&pst_Package->st_Data.st_Msg.st_MsgHdr);
        }

        if (g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_incoming_packet_write_finish_cb != NULL)
        {
            g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_incoming_packet_write_finish_cb((u8 *)&pst_Package->st_Data.st_Msg, pst_Package->st_Data.st_Msg.st_MsgHdr.u16_TotalLength);
        }

        cmbs_int_EventReceive((u8 *)&pst_Package->st_Data.st_Msg, pst_Package->st_Data.st_Msg.st_MsgHdr.u16_TotalLength);

        // check, if we might have received more than one cmbs message
        if (nDataIndex == (int)sizeof(pst_Package->st_Data.u32_Sync) + u16_Total)
        {
            // we received no further data
            nDataIndex = 0;
            break;
        }

        memcpy(pst_Package->serialBuf, pst_Package->serialBuf + (int)sizeof(pst_Package->st_Data.u32_Sync) + u16_Total,
               nDataIndex - ((int)sizeof(pst_Package->st_Data.u32_Sync) + u16_Total));

        nDataIndex -= (int)sizeof(pst_Package->st_Data.u32_Sync) + u16_Total;
    } while (nDataIndex > 0);

    return nDataIndex;
}

//  ========== _cmbs_int_CbThread ===========
/*!
    \brief    callback pump to receive and call application call-back
    \param[in,out]  pVoid  pointer to CMBS instance object
    \return    < void * >  always NULL
*/
DWORD WINAPI      _cmbs_int_CbThread(LPVOID pvoid)
{
    PST_CMBS_API_INST pst_CMBS = &g_CMBSInstance;

    U_CMBS_SER_DATA   st_CMBSPackage;
    u32               n_DataIndex = 0;

    //CFR_DBG_OUT( "CbThread Start \n");
    while (pst_CMBS->bo_Run)
    {
        WaitForSingleObject(pst_CMBS->h_RecPath, 500);

        // get the serial data and pass to collector function
        n_DataIndex += cfr_uartReceivedDataGet((PBYTE)st_CMBSPackage.serialBuf + n_DataIndex,
                                               sizeof(st_CMBSPackage.serialBuf) - n_DataIndex);

        n_DataIndex = _cmbs_int_PackageCollector(&st_CMBSPackage, n_DataIndex);
    }

    pst_CMBS->dw_ThreadID = 0;
    //CFR_DBG_OUT( "CbThread End\n");
    return 0;
}

void               cmbs_int_DataSignal(void)
{
    SetEvent(g_CMBSInstance.h_RecPath);
}

//*/
