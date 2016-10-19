/*!
*  \file       cmbs_int.c
*  \brief      Internal api functions
*  \author     stein
*
*  @(#)  %filespec: cmbs_int.c~48.1.28 %
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
* 27-Jan-14 tcmc_asa  ---GIT-- solve checksum failure issues
* 21-Jan-14 tcmc_asa  ---GIT-- completed checksum implementation
* 13-Jan-2014 tcmc_asa  -- GIT--  take checksum changes from 3.46.x to 3_x main (3.5x)
* 28-Nov-13 tcmc_asa  ---GIT--  changed u8_Idx to u16_Idx in checksum calculation/Verify
* 22-Nov-13 tcmc_asa  52 GIT  Added cmbs_int_cmd_SendCapablitiesReply
* 07-Nov-13 tcmc_asa  52 GIT  Added CHECKSUM_SUPPORT
*******************************************************************************/

#if defined( __linux__ )
#include <pthread.h>
#endif
#include "cmbs_int.h"
#include "cmbs_dbg.h"
#include "cmbs_util.h"
#include "cfr_uart.h"
#ifdef CMBS_COMA
#include "cfr_coma.h"
#endif // CMBS_COMA
#include "cfr_debug.h"
#include "cmbs_han.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(CMBS_API_TARGET)
#include "csys0reg.h"

#if defined (CSS)
#include "plicu.h"
#include "priorities.h"
#endif

#include "cos00int.h"                // Needed for critical section
#include "tapp_log.h"
#endif // defined(CMBS_API_TARGET)

/* Global variables */

#ifdef CHECKSUM_SUPPORT
extern ST_CAPABLITIES    g_st_CMBSCapabilities;
#endif

/* Local variables */

u8 u8_ChecksumErrorTest = 0;

#if !defined( CMBS_API_TARGET )
#define CMBS_TX_QUEUE_SIZE    (10)

typedef struct
{
    u16 u16_Event;
    u16 u16_Length;
    u8  u8_data[CMBS_BUF_SIZE];
} CMBS_TX_QUEUE_MSG;

typedef struct
{
    u8                  u8_HeadIndex;
    u8                  u8_Size;
    u8                  u8_TxStopped;
    CMBS_TX_QUEUE_MSG   data[CMBS_TX_QUEUE_SIZE];
} CMBS_TX_QUEUE;

CMBS_TX_QUEUE       G_int_TxMessagesQueue;

u8                  cmbs_int_IsTxStopped(void);
E_CMBS_RC           cmbs_int_PushTxQueue(u16 u16_Event, u8 *pBuf, u16 u16_Length);
CMBS_TX_QUEUE_MSG*  cmbs_int_PopQueuedMsg(void);

#endif

/*****************************************************************************
* API Internal functions
*****************************************************************************/

E_CMBS_ENDIAN   cmbs_int_EndiannessGet(void)
{
    int            i = 0x0A0B0C0D;
    char *p = (char *)&i;

    switch (p[0])
    {
        case  0x0A:
            return E_CMBS_ENDIAN_BIG;
        case  0x0B:
            return E_CMBS_ENDIAN_MIXED;
        default:
            return E_CMBS_ENDIAN_LITTLE;
    }
}

/* Convert endianess for 16 bit value */
u16 cmbs_int_EndianCvt16(u16 u16_Value)
{
    return (((u16_Value & 0x00FF) << 8) |
            ((u16_Value & 0xFF00) >> 8));
}

/* Convert endianess for 32 bit value */
u32 cmbs_int_EndianCvt32(u32 u32_Value)
{
    return (((u32_Value & 0x000000FF) << 24) |
            ((u32_Value & 0x0000FF00) << 8) |
            ((u32_Value & 0x00FF0000) >> 8) |
            ((u32_Value & 0xFF000000) >> 24));
}

void    cmbs_int_HdrEndianCvt(ST_CMBS_SER_MSGHDR *pst_Hdr)
{
    pst_Hdr->u16_TotalLength = cmbs_int_EndianCvt16(pst_Hdr->u16_TotalLength);
    pst_Hdr->u16_PacketNr    = cmbs_int_EndianCvt16(pst_Hdr->u16_PacketNr);
    pst_Hdr->u16_EventID     = cmbs_int_EndianCvt16(pst_Hdr->u16_EventID);
    pst_Hdr->u16_ParamLength = cmbs_int_EndianCvt16(pst_Hdr->u16_ParamLength);
}

void*  cmbs_int_RegisterCb(void *pv_AppRef, PFN_CMBS_API_CB pfn_api_Cb, u16 u16_bcdVersion)
{
    PST_CMBS_API_SLOT p_Slot = &g_CMBSInstance.st_ApplSlot;

    if (p_Slot->pFnAppCb)
    {
        CFR_DBG_ERROR("cmbs_int_RegisterCb Error: No free application slot available\n");
        return NULL;
    }

    p_Slot->pFnAppCb = pfn_api_Cb;
    p_Slot->pv_AppRefHandle = pv_AppRef;
    p_Slot->u16_AppAPIVersion = u16_bcdVersion;

    return (void *)p_Slot;
}

void    cmbs_int_RegisterLogBufferCb(void *pv_AppRef, PST_CB_LOG_BUFFER pfn_log_buffer_Cb)
{
    PST_CMBS_API_SLOT p_Slot = &g_CMBSInstance.st_ApplSlot;

    UNUSED_PARAMETER(pv_AppRef);

    memcpy(&p_Slot->pFnCbLogBuffer, pfn_log_buffer_Cb, sizeof(ST_CB_LOG_BUFFER));
}

void    cmbs_int_UnregisterCb(void *pv_AppRefHandle)
{
    PST_CMBS_API_SLOT p_Slot = &g_CMBSInstance.st_ApplSlot;

    if (p_Slot->pv_AppRefHandle == pv_AppRefHandle)
    {
        // clean-up entry
        memset(p_Slot, 0, sizeof(ST_CMBS_API_SLOT));
    }
}

/*!
\brief   returns CMBS API target version
*/
u16 cmbs_int_ModuleVersionGet(void)
{
    /*! \todo target version is received by HELLO_RPLY command */
    return g_CMBSInstance.u16_TargetVersion;
}

/*!
\brief   returns CMBS API target build version
*/
u16 cmbs_int_ModuleVersionBuildGet(void)
{
    /*! \todo target version is received by HELLO_RPLY command */
    return g_CMBSInstance.u16_TargetBuild;
}

/*!
\brief   CMBS event is received, prepare for callback function
\param[in]      pu8_Mssg    pointer to message without sync dword
\param[in]      u16_Size size of message without sync dword
\return         <none>
*/
void    cmbs_int_EventReceive(u8 *pu8_Mssg, u16 u16_Size)
{
    PST_CMBS_SER_MSG p_Mssg = (PST_CMBS_SER_MSG)pu8_Mssg;

#ifdef CHECKSUM_SUPPORT
    u8 u8_CheckSum[2];
#endif

    u16 u16_SizeMinusMsgHdr = u16_Size - sizeof(ST_CMBS_SER_MSGHDR);
    CHECKSUMPRINT(("SizeMinusMsgHdr = %hu \n", u16_SizeMinusMsgHdr));

#ifdef CHECKSUM_SUPPORT
    // Checksum as an IE
    if ((g_st_CMBSCapabilities.u8_Checksum) &&
            ((p_Mssg->st_MsgHdr.u16_EventID & CMBS_CMD_MASK) != CMBS_CMD_MASK) && u16_SizeMinusMsgHdr)
    {
        u16 u16_CheckSumIE;
        cfr_ie_dser_u16(&p_Mssg->u8_Param[p_Mssg->st_MsgHdr.u16_ParamLength - 6], &u16_CheckSumIE);

        // Checksum ID as last IE
        u8_CheckSum[0] = p_Mssg->u8_Param[p_Mssg->st_MsgHdr.u16_ParamLength - 2];
        u8_CheckSum[1] = p_Mssg->u8_Param[p_Mssg->st_MsgHdr.u16_ParamLength - 1];


        if (u16_CheckSumIE == CMBS_IE_CHECKSUM)
        {
            // remove checksum from calculations
            u16_SizeMinusMsgHdr -= 6;

            if (p_cmbs_int_ChecksumVerify(u8_CheckSum, p_Mssg->u8_Param, u16_SizeMinusMsgHdr))
            {
                // Checksum OK
                CHECKSUMPRINT(("%% Checksum OK %%\n"));

                // 'delete' checksum
                p_Mssg->st_MsgHdr.u16_ParamLength -= 6;
                p_Mssg->st_MsgHdr.u16_TotalLength -= 6;
            }
            else
            {
                // message invalid !!!
                CFR_DBG_ERROR("CMBS-EventReceive ERROR: Checksum ERROR \n");

                // send message to the sending side
                cmbs_int_SendChecksumError(CMBS_CHECKSUM_ERROR, p_Mssg->st_MsgHdr.u16_EventID);

                // don't continue, the message is corrupted
                return;
            }
        }
        else
        {
            // Checksum IE is missing, although indicated in u16_PacketNr
            // message invalid !!!
            CFR_DBG_ERROR("CMBS-EventReceive ERROR: Checksum expected, but missing \n");

            CHECKSUMPRINT((" Param = 0x%2hhX \n", p_Mssg->u8_Param[p_Mssg->st_MsgHdr.u16_ParamLength - 6]));

            // send message to the sending side
            cmbs_int_SendChecksumError(CMBS_CHECKSUM_NOT_FOUND, p_Mssg->st_MsgHdr.u16_EventID);

            // don't continue, the message is corrupted
            return;
        }

    }
#endif // CHECKSUM_SUPPORT


    // Command handler
    if ((p_Mssg->st_MsgHdr.u16_EventID & CMBS_CMD_MASK) == CMBS_CMD_MASK)
    {
        //        u16 u16_SizeMinusMsgHdr = u16_Size - sizeof(ST_CMBS_SER_MSGHDR);

        // trace command
#if !defined( CMBS_API_TARGET )
        cmbs_dbg_CmdTrace("<<-", &p_Mssg->st_MsgHdr, p_Mssg->u8_Param);
#endif

        cmbs_int_cmd_Dispatcher((u8)p_Mssg->st_MsgHdr.u16_EventID, (u8 *)p_Mssg->u8_Param, u16_SizeMinusMsgHdr);

#if !defined( CMBS_API_TARGET )
        // notify application about received command
        if (g_CMBSInstance.st_ApplSlot.pFnAppCb)
            g_CMBSInstance.st_ApplSlot.pFnAppCb(g_CMBSInstance.st_ApplSlot.pv_AppRefHandle,
                                                p_Mssg->st_MsgHdr.u16_EventID, p_Mssg->u8_Param);
#endif
    }
    // Event handler
    else
    {
        if (g_CMBSInstance.st_ApplSlot.pFnAppCb)
        {
#if !defined( CMBS_API_TARGET )
            // Host shall leave ‘suspend’ state upon reception of TARGET_UP from target.
            if (p_Mssg->st_MsgHdr.u16_EventID == CMBS_EV_DSR_TARGET_UP && cmbs_int_IsTxStopped())
            {
                CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
                G_int_TxMessagesQueue.u8_TxStopped = 0; // mark tx as resumed
                G_int_TxMessagesQueue.u8_Size = 0;
                G_int_TxMessagesQueue.u8_HeadIndex = 0;
                CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
            }
#endif
            // special case fw update: take whole message
            if (cmbs_util_RawPayloadEvent(p_Mssg->st_MsgHdr.u16_EventID))
            {
                g_CMBSInstance.st_ApplSlot.pFnAppCb(g_CMBSInstance.st_ApplSlot.pv_AppRefHandle,
                                                    p_Mssg->st_MsgHdr.u16_EventID, (void *)pu8_Mssg);
            }
            else                    // standard message with IE list
            {
                ST_CFR_IE_LIST List;
                /*! \todo check if structure fits into calculation work! */
                List.pu8_Buffer = (u8 *)p_Mssg->u8_Param;
                List.u16_CurIE  = 0;
                // List.u16_CurSize = u16_Size - sizeof(ST_CMBS_SER_MSGHDR);
                // List.u16_MaxSize = u16_Size - sizeof(ST_CMBS_SER_MSGHDR);

                List.u16_CurSize = u16_SizeMinusMsgHdr;
                List.u16_MaxSize = u16_SizeMinusMsgHdr;

                // trace command
#if !defined( CMBS_API_TARGET )
                cmbs_dbg_CmdTrace("<<-", &p_Mssg->st_MsgHdr, p_Mssg->u8_Param);
#endif

                g_CMBSInstance.st_ApplSlot.pFnAppCb(g_CMBSInstance.st_ApplSlot.pv_AppRefHandle,
                                                    p_Mssg->st_MsgHdr.u16_EventID, (void *)&List);
            }
        }
        else
        {
            CFR_DBG_ERROR("CMBS-EventReceive ERROR: callback application is not available\n");
        }
    }
}

//  ========== _cmbs_int_Send_Wrp  ===========
/*!
\brief     send packet via communication module
\param[in,out]   u16_Event  event identifier with command indicator, if needed
\param[in,out]   pu8_Buf   pointer to parameter buffer
\param[in,out]   u16_Length  parameter buffer size
\return     <E_CMBS_RC>
*/
E_CMBS_RC   _cmbs_int_Send_Wrp(u16 u16_Event, u8 *pBuf, u16 u16_Length)
{
    ST_CMBS_SER_MSGHDR msgHdr;
    u32 u32_Sync = CMBS_SYNC;
    u16 u16_Size = 0;
    E_CMBS_RC   stResp = CMBS_RC_OK;
#ifdef CHECKSUM_SUPPORT
    u8 u8_CheckSum[6];
#endif

    memset(&msgHdr, 0, sizeof(msgHdr));

    msgHdr.u16_TotalLength = sizeof(ST_CMBS_SER_MSGHDR) + u16_Length;
    msgHdr.u16_PacketNr    = 0;
    msgHdr.u16_EventID     = u16_Event;
    msgHdr.u16_ParamLength = u16_Length;

    u16_Size = sizeof(u32_Sync) + sizeof(msgHdr) + u16_Length;

    // trace packet before endianness conversions
#if !defined( CMBS_API_TARGET )
    cmbs_dbg_CmdTrace("->>", &msgHdr, pBuf);
#endif


#ifdef CHECKSUM_SUPPORT
    if ((g_st_CMBSCapabilities.u8_Checksum) && !((u16_Event & 0xFF00) == 0xFF00))
    {
#if !defined( CMBS_API_TARGET )
        if (u8_ChecksumErrorTest != 2)
#endif
        {
            u16_Size += 6;
            msgHdr.u16_TotalLength += 6;  // 6 bytes Checksum IE
            msgHdr.u16_ParamLength += 6;  // 6 bytes Checksum IE
        }
    }
#endif

#ifdef CMBS_COMA
    1111
    if (CFR_E_RETVAL_OK == cfr_comaPacketPrepare(u16_Size))
#else
    if (CFR_E_RETVAL_OK == cfr_uartPacketPrepare(u16_Size))
#endif
    {
        // perform endianness conversion if needed
        if (g_CMBSInstance.e_Endian != E_CMBS_ENDIAN_LITTLE)
        {
            cmbs_int_HdrEndianCvt(&msgHdr);
        }

        // From now on we need exclusive access to transmission of serial port
        CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);

#ifdef CHECKSUM_SUPPORT
        // calculate checksum
        p_cmbs_int_CalcChecksum(&u8_CheckSum[4], pBuf, u16_Length);
        // add checksum IE header
        u8_CheckSum[0] = (u8)(CMBS_IE_CHECKSUM & 0xFF);
        u8_CheckSum[1] = (u8)(CMBS_IE_CHECKSUM >> 8);
        u8_CheckSum[2] = 2;  // length = 2 Bytes checksum
        u8_CheckSum[3] = 0;  // length = 2 Bytes checksum

#if !defined( CMBS_API_TARGET )
        if (u8_ChecksumErrorTest == 3)
        {
            // wrong IE for checksum, maybe not really useful
            u8_CheckSum[1] += 1;
        }
#endif
#endif  // CHECKSUM_SUPPORT

#ifdef CMBS_COMA
        // writing sync bytes
        cfr_comaPacketPartWrite((u8 *)&u32_Sync, sizeof(u32_Sync));

        // writing header
        cfr_comaPacketPartWrite((u8 *)&msgHdr, sizeof(msgHdr));

        if (pBuf && u16_Length)
        {
            // writing buffer
            cfr_comaPacketPartWrite(pBuf, u16_Length);
        }
#ifdef CHECKSUM_SUPPORT
        if ((g_st_CMBSCapabilities.u8_Checksum) && !((u16_Event & 0xFF00) == 0xFF00)
#if !defined( CMBS_API_TARGET )
                && (u8_ChecksumErrorTest != 2)
#endif
           )
        {
            // Add checksum IE as the last IE
            cfr_comaPacketPartWrite(u8_CheckSum, 6);
            CHECKSUMPRINT(("  Checksum added, Length = %hu\n", msgHdr.u16_ParamLength));
            CHECKSUMPRINT(("  Checksum = 0x%2hhX 0x%2hhX\n", u8_CheckSum[4], u8_CheckSum[5]));
        }
        else
        {
            CHECKSUMPRINT(("  Checksum NOT added! g_st_CMBSCapabilities.u8_Checksum<%hhu>, u16_Event<%hu>\n", g_st_CMBSCapabilities.u8_Checksum, u16_Event));
        }
#endif


        cfr_comaPacketWriteFinish(CFR_BUFFER_UART_TRANS);
        cfr_comaDataTransmitKick();
#else
        // writing sync bytes
        cfr_uartPacketPartWrite((u8 *)&u32_Sync, sizeof(u32_Sync));

        // writing header
        cfr_uartPacketPartWrite((u8 *)&msgHdr, sizeof(msgHdr));

        if (pBuf && u16_Length)
        {
            // writing buffer
            cfr_uartPacketPartWrite(pBuf, u16_Length);
        }
#ifdef CHECKSUM_SUPPORT
        if ((g_st_CMBSCapabilities.u8_Checksum) && !((u16_Event & 0xFF00) == 0xFF00)
#if !defined( CMBS_API_TARGET )
                && (u8_ChecksumErrorTest != 2)
#endif
           )
        {
            // Add checksum IE as the last IE
            cfr_uartPacketPartWrite(u8_CheckSum, 6);
            CHECKSUMPRINT(("  Checksum added, Length = %hu\n", msgHdr.u16_ParamLength));
            CHECKSUMPRINT(("  Checksum = 0x%2hhX 0x%2hhX\n", u8_CheckSum[4], u8_CheckSum[5]));
        }
        else
        {
            CHECKSUMPRINT(("  Checksum NOT added! g_st_CMBSCapabilities.u8_Checksum<%hhu>, u16_Event<%hu>\n", g_st_CMBSCapabilities.u8_Checksum, u16_Event));
        }
#endif


        cfr_uartPacketWriteFinish(CFR_BUFFER_UART_TRANS);
        cfr_uartDataTransmitKick();
#endif

#ifdef CHECKSUM_SUPPORT
        if (u16_Event == CMBS_EV_DSR_FW_UPD_START)
        {
            // boodloader doesn't support checksum, thus disable it before FW upgrade start
            g_st_CMBSCapabilities.u8_Checksum = 0;
        }
#endif
        // We have sent the complete packet, leave critical section
        CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
    }
    else
    {
        stResp = CMBS_RC_ERROR_OUT_OF_MEM;
    }

    if ((g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_outgoing_packet_part_write_cb != NULL)
            && (stResp == CMBS_RC_OK))
    {
        g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.\
        pfn_cmbs_api_log_outgoing_packet_part_write_cb(u16_Event, pBuf, u16_Length);
    }

#if !defined( CMBS_API_TARGET )
    if (stResp != CMBS_RC_OK)
        CFR_DBG_ERROR("cmbs_int_Send: !!!! transmit buffer full\n");
#endif // CMB_API_TARGET

    return stResp;
}
//  ========== _cmbs_int_Send  ===========
/*!
    \brief     send packet via communication module. If it busy, push it to Tx queue
    \param[in,out]   u16_Event  event identifier with command indicator, if needed
    \param[in,out]   pu8_Buf   pointer to parameter buffer
    \param[in,out]   u16_Length  parameter buffer size
    \return     <E_CMBS_RC>
*/
E_CMBS_RC        _cmbs_int_Send(u16 u16_Event, u8 *pBuf, u16 u16_Length)
{
#if !defined( CMBS_API_TARGET )
    if (cmbs_int_IsTxStopped())
    {
        return cmbs_int_PushTxQueue(u16_Event, pBuf, u16_Length);
    }
    else
#endif
    {
        return _cmbs_int_Send_Wrp(u16_Event, pBuf, u16_Length);
    }
}

//  ========== cmbs_int_EventSend ===========
/*!
    \brief     Send complete event message: header + parameter data to communication device
    \param[in,out]   e_EventID   CMBS Event ID
    \param[in,out]   pBuf         pointer parameter buffer
    \param[in,out]   u16_Length   size of parameter buffer
    \return     < E_CMBS_RC >
*/

E_CMBS_RC   cmbs_int_EventSend(E_CMBS_EVENT_ID e_EventID, u8 *pBuf, u16 u16_Length)
{
    u8 ret;
#ifdef CMBS_MSG_DEBUG
    printf("[%d] Host <- CMBS : %s\n", G_u16_os00_SystemTime, cmbs_dbg_GetEventName(e_EventID));
#endif

    if (cmbs_int_cmd_FlowStateGet() == E_CMBS_FLOW_STATE_GO)
    {
        ret = _cmbs_int_Send((u16)e_EventID, pBuf, u16_Length);
        // Some targets require delay before next Msg is sent out.
#if !defined( CMBS_API_TARGET )
        SleepMs(20);
#endif

    }
    else
    {
        ret = CMBS_RC_ERROR_GENERAL;
    }
    return ret;
}

//  ========== cmbs_int_cmd_Send ===========
/*!
    \brief    Send complete internal command message: header + parameter data to communication device
    \param[in]   u8_Cmd         internal command ID
    \param[in]   pBuf         pointer parameter buffer
    \param[in]   u16_Length   size of parameter buffer
    \return    < E_CMBS_RC >
*/
E_CMBS_RC   cmbs_int_cmd_Send(u8 u8_Cmd, u8 *pBuf, u16 u16_Length)
{
    return _cmbs_int_Send((u16)(0xFF00 | u8_Cmd), pBuf, u16_Length);
}



#ifdef CHECKSUM_SUPPORT

/*MPF::p_cmbs_int_CalcChecksum ======================================== */
/*                                                                      */
/* FUNCTIONAL DESCRIPTION:                                              */
/*                                                                      */
/* This routine calculates the checksum for the CMBS API message.       */
/*                                                                      */
/* PARAMETERS                                                           */
/* u16_Length r/- Number of IE bytes in CMBS message                    */
/* pBuf       r/- Pointer to IE buffer                                  */
/*                                                                      */
/* INTERFACE DECLARATION:                                               */

void p_cmbs_int_CalcChecksum(u8 *pCheckSum, u8 *pBuf, u16 u16_Length)
/*EMP================================================================== */

/* == DESIGN ========================================================== */
/*                                                                      */
/* The checksum is calculated according to the following formula        */
/* c0 = Sum{Octet(n)}                                                   */
/* c1 = Sum{(FLEN+1-n)*Octet(n)}                                        */
/* Where n = 1 to FLEN                                                  */
/* Octet(u16_Length-1) = c0 - c1                                              */
/* Octet(u16_Length) = 2*c0 + c1                                              */
/*                                                                      */

/* == STATEMENTS ====================================================== */
{
    s16 s16_C0 = 0;
    s16 s16_C1 = 0;
    u16 u16_Idx;
    u8 u8_Octet1;
    u8 u8_Octet2;

    if (u16_Length)
    {
        for (u16_Idx = 0; u16_Idx < u16_Length; u16_Idx++)
        {
            /*      c0 = Sum{Octet(n)}                                              */
            s16_C0 = ((s16)(s16_C0 + *pBuf++)) % 255;
            /*      c1 = Sum{(FLEN+1-n)*Octet(n)}                                   */
            s16_C1 = ((s16_C1 + s16_C0) % 255);

        }
    }
    /* the 2 checksum octets are also used, but presetting is 0. */

    /*      c0 = Sum{Octet(n)}                                              */
    s16_C0 = ((s16)(s16_C0)) % 255;
    /*      c1 = Sum{(FLEN+1-n)*Octet(n)}                                   */
    s16_C1 = ((s16_C1 + s16_C0) % 255);
    /*      c0 = Sum{Octet(n)}                                              */
    s16_C0 = ((s16)(s16_C0)) % 255;
    /*      c1 = Sum{(FLEN+1-n)*Octet(n)}                                   */
    s16_C1 = ((s16_C1 + s16_C0) % 255);

    /*  Octet(FLEN-1) = c0 - c1                                             */
    u8_Octet1 = (((s16_C0 - s16_C1) + 255) % 255);
    /*  Octet(FLEN) = 2*c0 + c1                                           */
    u8_Octet2 = (((-2 * s16_C0) + s16_C1 + 510) % 255);

    CHECKSUMPRINT(("\n Checksum: Length: %hu, Octed1: 0x%2hhX, Octed2: 0x%2hhX \n", u16_Length, u8_Octet1, u8_Octet2));

#if !defined( CMBS_API_TARGET )
    if (u8_ChecksumErrorTest == 1)
    {
        // Send wrong checksum for testing
        u8_Octet1 += 1;
    }
#endif

    *pCheckSum++ = u8_Octet1;
    *pCheckSum   = u8_Octet2;

}
/* == END OF p_cmbs_int_CalcChecksum ====================================== */


/*MPF::p_cmbs_int_ChecksumVerify ====================================== */
/*                                                                      */
/* FUNCTIONAL DESCRIPTION:                                              */
/*                                                                      */
/* This routine verifies that the checksum calculations in the received */
/* Frame are correct.                                                   */
/*              */
/* PARAMETERS                                                           */
/* return result of checksum, True or False                       */
/* u8_Index r/- Start location in G_u8_hm00_PTPReceiveBuf of  */
/* ptp message.        */
/*                                                                      */
/* INTERFACE DECLARATION:                                               */

u8 p_cmbs_int_ChecksumVerify(u8 u8_Checksum[2],  u8 *pBuf, u16 u16_Length)
/*EMP================================================================== */

/* == DESIGN ========================================================== */
/*                                                                      */
/* Adjust for the link signature                                        */
/* The checksum is verified using the following formula                 */
/* c0 = Sum{Octet(n)}                                                   */
/* c1 = Sum{(FLEN+1-n)*Octet(n)}                                        */
/* Where n = 1 to FLEN                                                  */
/* Octet_1 = c0 - c1                                                    */
/* Octet_2 = 2*c0 + c1                                                  */
/* IF octet_1 == 0ctet_2 == 0x00 then the checksum is correct           */
/*                                                                      */

/* == STATEMENTS ====================================================== */
{
    u8 u8_Octet1;
    u8 u8_Octet2;
    s16 s16_C0 = 0;
    s16 s16_C1 = 0;
    u16 u16_Idx;

    // u8_CSum1 = u8_Checksum[0];
    // u8_CSum2 = u8_Checksum[1];

    if (u16_Length)
    {
        for (u16_Idx = 0; u16_Idx < u16_Length; u16_Idx++)
        {
            /*      c0 = Sum{Octet(n)}                                              */
            s16_C0 = ((s16)(s16_C0 + *pBuf++) % 255);
            /*      c1 = Sum{(length+1-n)*Octet(n)}                                   */
            s16_C1 = ((s16_C1 + s16_C0) % 255);
        }
        // Do the same with the checksum itself
        s16_C0 = ((s16)(s16_C0 + u8_Checksum[0]) % 255);
        s16_C1 = ((s16_C1 + s16_C0) % 255);

        s16_C0 = ((s16)(s16_C0 + u8_Checksum[1]) % 255);
        s16_C1 = ((s16_C1 + s16_C0) % 255);

    }
    /*  Octet_1 = c0 - c1                                                       */
    u8_Octet1 = (((s16_C0 - s16_C1) + 255) % 255);
    /*  Octet_2 = 2*c0 + c1                                                     */
    u8_Octet2 = (((-2 * s16_C0) + s16_C1 + 510) % 255);

    return ((u8_Octet1 == 0x00) && (u8_Octet2 == 0x00));
}
/* == END OF p_cmbs_int_ChecksumVerify ==================================== */

/*MPF::cmbs_int_SendChecksumError ====================================== */
/*                                                                      */
/* FUNCTIONAL DESCRIPTION:                                              */
/*                                                                      */
/* This routine sends a message to the sending side of a message to     */
/* indicate that checksum wasn't received or was incorrect              */
/*              */
/* PARAMETERS                                                           */
/*  ErrorType - see E_CMBS_CHECKSUM_ERROR                               */
/*                                                                      */
/* INTERFACE DECLARATION:                                               */

E_CMBS_RC cmbs_int_SendChecksumError(E_CMBS_CHECKSUM_ERROR e_ErrorType, u16 u16_EventID)
{
    PST_CFR_IE_LIST   p_List;
    ST_IE_CHECKSUM_ERROR st_ChecksumError;

    // Send message, but it can maybe not be sent immidietally, as SW is in
    // procedure of receiving a message. To be verified!!

    // p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    ALLOCATE_IE_LIST(p_List);


    st_ChecksumError.u16_ReceivedEvent = u16_EventID;
    st_ChecksumError.e_CheckSumError = e_ErrorType;

    cmbs_api_ie_ChecksumErrorAdd((void *)p_List, &st_ChecksumError);

    return cmbs_int_EventSend(CMBS_EV_CHECKSUM_FAILURE, p_List->pu8_Buffer, p_List->u16_CurSize);
}

#if !defined( CMBS_API_TARGET )
void cmbs_int_SimulateChecksumError(char u8_ErrorType)
{
    // E_CMBS_CHECKSUM_ERROR e_ErrorType;
    // u16_EventID;

    switch (u8_ErrorType)
    {
        case 0:
            // reset all errors
        case 1:
            // Send wrong checksum
        case 2:
            // Send events without checksum
        case 3:
            // Send events with destroyed CHECKSUM IE
            u8_ChecksumErrorTest = u8_ErrorType;
            break;

        case 5:
            // send CMBS_EV_CHECKSUM_FAILURE with CMBS_CHECKSUM_ERROR
            cmbs_int_SendChecksumError(CMBS_CHECKSUM_ERROR, CMBS_EV_DSR_HS_PAGE_START_RES);
            break;

        case 6:
            // send CMBS_EV_CHECKSUM_FAILURE with CMBS_CHECKSUM_NO_EVENT_ID
            cmbs_int_SendChecksumError(CMBS_CHECKSUM_NO_EVENT_ID, CMBS_EV_DSR_HS_PAGE_START_RES);
            break;
    }
}
#endif

#endif // CHECKSUM_SUPPORT

/*MPF::cmbs_int_cmd_SendCapablities =================================== */
/*                                                                      */
/* FUNCTIONAL DESCRIPTION:                                              */
/*                                                                      */
/* This routine sends a command including the capabilities              */
/*                                              */
/* PARAMETERS                                                           */
/*   none                                                               */
/*                                                                      */
/* INTERFACE DECLARATION:                                               */
void cmbs_int_cmd_SendCapablities(void)
{
    u8 u8_Buffer[5];

    memset(&u8_Buffer, 0, sizeof(u8_Buffer));

    u8_Buffer[0] = 4; // currently 4 Bytes

#ifdef CHECKSUM_SUPPORT
    u8_Buffer[4] |= CMBS_CAPABILITY_MASK;
#endif

#if !defined( CMBS_API_TARGET )
    // preset Capabilities to 0
#ifdef CHECKSUM_SUPPORT
    g_st_CMBSCapabilities.u8_Checksum = 0;
#endif

    // send CMBS_CMD_CAPABILITIES with capabilities
    cmbs_int_cmd_Send(CMBS_CMD_CAPABILITIES, u8_Buffer, 5);
#endif

}

/*MPF::cmbs_int_cmd_SendCapablitiesReply ============================== */
/*                                                                      */
/* FUNCTIONAL DESCRIPTION:                                              */
/*                                                                      */
/* This routine sends a reply command including the capabilities        */
/*                                              */
/* PARAMETERS                                                           */
/*   none                                                               */
/*                                                                      */
/* INTERFACE DECLARATION:                                               */
void cmbs_int_cmd_SendCapablitiesReply(void)
{
    u8 u8_Buffer[5];

    memset(&u8_Buffer, 0, sizeof(u8_Buffer));

    u8_Buffer[0] = 4; // currently 4 Bytes (32 bits)

#ifdef CHECKSUM_SUPPORT
    u8_Buffer[4] |= CMBS_CAPABILITY_MASK;
#endif

#if defined( CMBS_API_TARGET )
    // send CMBS_CMD_CAPABILITIES with capabilities
    cmbs_int_cmd_Send(CMBS_CMD_CAPABILITIES_RPLY, u8_Buffer, 5);
#endif

}

#if !defined( CMBS_API_TARGET )

/**
 * CMBS_CMD_FLASH_START handler: suspend TX activity
*/
void        _cmbs_int_SuspendTxCommands(void)
{
    CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);

    // can't use cmbs_int_cmd_Send, cause it is mutex-dependent
    _cmbs_int_Send_Wrp(0xFF00 | CMBS_CMD_FLASH_START_RES, NULL, 0);

    // mark tx as suspended
    G_int_TxMessagesQueue.u8_TxStopped = 1;

    CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
}

/**
 * CMBS_CMD_FLASH_STOP handler: resume TX activity
*/
void        _cmbs_int_ResumeTxCommands(void)
{
    CMBS_TX_QUEUE_MSG *pMsg = NULL;

    CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);

    // can't use cmbs_int_cmd_Send, cause it is mutex-dependent
    _cmbs_int_Send_Wrp(0xFF00 | CMBS_CMD_FLASH_STOP_RES, NULL, 0);

    while ((pMsg = cmbs_int_PopQueuedMsg()))
    {
        _cmbs_int_Send_Wrp(pMsg->u16_Event, pMsg->u8_data, pMsg->u16_Length);
    }
    G_int_TxMessagesQueue.u8_HeadIndex = 0;

    // mark tx as resumed
    G_int_TxMessagesQueue.u8_TxStopped = 0;

    CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
}

/**
 *  Pop awaiting message from queue
*/
CMBS_TX_QUEUE_MSG*  cmbs_int_PopQueuedMsg(void)
{
    CMBS_TX_QUEUE_MSG *pMsg = NULL;

    if (G_int_TxMessagesQueue.u8_Size)
    {
        pMsg = &(G_int_TxMessagesQueue.data[G_int_TxMessagesQueue.u8_HeadIndex]);
        G_int_TxMessagesQueue.u8_Size--;
        G_int_TxMessagesQueue.u8_HeadIndex++;
    }

    return pMsg;
}

/**
 *  Push message into queue
*/
E_CMBS_RC   cmbs_int_PushTxQueue(u16 u16_Event, u8 *pBuf, u16 u16_Length)
{
    E_CMBS_RC eResp = CMBS_RC_ERROR_OUT_OF_MEM;
    CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
    if ((G_int_TxMessagesQueue.u8_Size < CMBS_TX_QUEUE_SIZE) && (u16_Length < CMBS_BUF_SIZE))
    {
        u8  u8_Index = (G_int_TxMessagesQueue.u8_HeadIndex + G_int_TxMessagesQueue.u8_Size) % CMBS_TX_QUEUE_SIZE;
        CMBS_TX_QUEUE_MSG *pMsg = &(G_int_TxMessagesQueue.data[u8_Index]);
        pMsg->u16_Event = u16_Event;
        pMsg->u16_Length = u16_Length;
        memcpy(pMsg->u8_data, pBuf, u16_Length);
        G_int_TxMessagesQueue.u8_Size++;
        eResp = CMBS_RC_OK;
    }
    else
    {
        CFR_DBG_ERROR("cmbs_int_PushTxQueue: No space in TX queue, message will be skipped !!!! \n");
    }
    CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);

    return eResp;
}

/**
 *  Returns if TX is in suspended state
*/
u8  cmbs_int_IsTxStopped(void)
{
    u8  u8_Resp = 0;
    CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
    u8_Resp = G_int_TxMessagesQueue.u8_TxStopped;
    CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_TxThreadCriticalSection);
    return u8_Resp;
}

#endif

E_CMBS_RC   cmbs_int_ResponseSend(E_CMBS_EVENT_ID e_ID, E_CMBS_RESPONSE e_RSPCode)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE  st_Response;

    st_Response.e_Response = e_RSPCode;

    ALLOCATE_IE_LIST(p_List);

    cmbs_api_ie_ResponseAdd((void *)p_List, &st_Response);

    return cmbs_int_EventSend(e_ID, p_List->pu8_Buffer, p_List->u16_CurSize);
}

E_CMBS_RC   cmbs_int_ResponseWithCallInstanceSend(E_CMBS_EVENT_ID e_ID,
        E_CMBS_RESPONSE e_RSPCode,
        u32 u32CallInstance)
{
    PST_CFR_IE_LIST p_List;
    ST_IE_RESPONSE  st_Response;

    st_Response.e_Response = e_RSPCode;

    ALLOCATE_IE_LIST(p_List);


    cmbs_api_ie_CallInstanceAdd(p_List, u32CallInstance);
    cmbs_api_ie_ResponseAdd((void *)p_List, &st_Response);

    return cmbs_int_EventSend(e_ID, p_List->pu8_Buffer, p_List->u16_CurSize);

}

//  ========== cmbs_int_SendHello ===========
/*!
    \brief    Sends HELLO message
    \param[in]   pst_DevCtl   pointer to communication device configuration
    \param[in]   pst_DevMedia  pointer to media device configuration
    \return    < E_CMBS_RC >
*/
E_CMBS_RC cmbs_int_SendHello(ST_CMBS_DEV *pst_DevCtl, ST_CMBS_DEV *pst_DevMedia)
{
    u8 u8_Buffer[sizeof(ST_CMBS_DEV)] = { 0 };
    u8 u8_BufferIDX = 0;

    UNUSED_PARAMETER(pst_DevCtl);

    // prepare buffer for TDM configuration in Hello command
    if (pst_DevMedia->u_Config.pTdmCfg)
    {
        u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, pst_DevMedia->u_Config.pTdmCfg->e_Type);
        u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, pst_DevMedia->u_Config.pTdmCfg->e_Speed);
        u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, pst_DevMedia->u_Config.pTdmCfg->e_Sync);
        u8_BufferIDX += cfr_ie_ser_u16(u8_Buffer + u8_BufferIDX, pst_DevMedia->u_Config.pTdmCfg->u16_SlotEnable);
    }
    else
    {
        // No TDM (Probabely USB)
        u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, 0);
        u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, 0);
        u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, 0);
        u8_BufferIDX += cfr_ie_ser_u16(u8_Buffer + u8_BufferIDX, 0);
    }

    // flow control
    u8_BufferIDX += cfr_ie_ser_u8(u8_Buffer + u8_BufferIDX, pst_DevMedia->e_FlowCTRL);

    return cmbs_int_cmd_Send(CMBS_CMD_HELLO, u8_Buffer, u8_BufferIDX);
}

/*---------------[End of file]-----------------------------------------------------------------------------------------------------------------*/
