/*!
*  \file       cfr_uart.c
*  \brief      UART implementation of linux host side
*  \author     stein
*
*  @(#)  %filespec: cfr_uart.cpp~2 %
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
*  14-feb-09   R.Stein    1         Initialize \n
*******************************************************************************/

//#include "stdafx.h"
#include <stdlib.h>
#include <stddef.h>  // for offsetof
#include <stdio.h>
#include "cmbs_platf.h"

#include "cmbs_int.h"   /* internal API structure and defines */
#include "cfr_uart.h"   /* packet handler */
#include "cfr_debug.h"  /* debug handling */

#include "cphysicalport.h"

CPhysicalPort g_CPHYSerial;


//    ========== cfr_uartThread ===========
/*!
      \brief            UART data receive pump. if data is available a message is send
                        to cfr_cmbs task.

      \param[in]        pVoid          pointer to CMBS instance object

      \return           <void *>       return always NULL

* /

void  *           cfr_uartThread( void * pVoid )
{
   PST_CMBS_API_INST
                  pInstance = (PST_CMBS_API_INST)pVoid;
   int            fdDevCtl  = pInstance->fdDevCtl;
   int            msgQId    = pInstance->msgQId;
   fd_set         input_fdset;
   size_t         nMsgSize;

   ST_CMBS_LIN_MSG
                  LinMsg;

//   CFR_DBG_OUT( "UART Thread: ID:%lu running\n", (unsigned long)pthread_self() );

   nMsgSize = sizeof( LinMsg.msgData );

   /* Never ending loop.
      Thread will be exited automatically when parent thread finishes.
   * /
   while( 1 )
   {
      FD_ZERO( &input_fdset );
      FD_SET( fdDevCtl, &input_fdset);

      if( select(fdDevCtl+1, &input_fdset, NULL, NULL, NULL) == -1 )
      {
         CFR_DBG_ERROR( "UartThread Error: select() failed\n" );
       /*!\todo exception handling is needed !* /
      }
      else
      {
         if( FD_ISSET(fdDevCtl, &input_fdset) )
         {
            memset( &LinMsg.msgData, 0, sizeof(LinMsg.msgData) );

            /* Reading available data from serial interface * /
            if( (LinMsg.msgData.nLength = read(fdDevCtl, LinMsg.msgData.u8_Data, sizeof(LinMsg.msgData.u8_Data))) == -1 )
            {
               CFR_DBG_ERROR( "UartThread Error: read() failed\n" );
            }
            else
            {
//               CFR_DBG_OUT( "UartThread: received %d bytes\n", LinMsg.msgData.nLength );

               /* Send what we received to callback thread * /
               if( msgQId >= 0 )
               {
                  LinMsg.msgType = 1;

                  if( msgsnd( msgQId, &LinMsg, nMsgSize, 0 ) == -1 )
                  {
                     CFR_DBG_ERROR( "UartThread: msgsnd ERROR:%d\n", errno );
                  }
               }
               else
               {
                  CFR_DBG_ERROR( "UartThread: invalid msgQId:%d\n", msgQId );
               }
            }
         }
      }
   }

   return NULL;
}
*/

extern "C" int   cfr_uartReceivedDataGet(PBYTE pb_Buffer, int n_MaxLength)
{
    return g_CPHYSerial.read((char*)pb_Buffer, n_MaxLength);
}

//    ==========    cfr_uartPacketPartWrite ===========
/*!
      \brief             write partly the packet into communication device

      \param[in,out]     pu8_Buffer    pointer to packet part

      \param[in,out]     u16_Size      size of packet part

      \return            < int >       currently, alway 0

*/

int               cfr_uartPacketPartWrite(u8* pu8_Buffer, u16 u16_Size)
{
    g_CPHYSerial.write((LPSTR) pu8_Buffer , (DWORD)u16_Size);

    return 0;
}
//    ========== cfr_uartPacketWriteFinish ===========
/*!
      \brief             Currently dummy function is not needed on host side

      \param[in,out]     u8_BufferIDX  buffer index

      \return            < none >

*/

void              cfr_uartPacketWriteFinish(u8 u8_BufferIDX)
{
    // For logging sent data packets
    if (u8_BufferIDX == CFR_BUFFER_UART_TRANS)
    {
        if (g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_outgoing_packet_write_finish_cb != NULL)
        {
            g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_outgoing_packet_write_finish_cb();
        }
    }
}

//    ========== cfr_uartPacketPrepare  ===========
/*!
      \brief             Currently dummy function is not needed on host side

      \param[in,out]     u16_size      size of to be submitted packet for transmission

      \return           < CFR_E_RETVAL >

*/

CFR_E_RETVAL      cfr_uartPacketPrepare(u16 u16_size)
{
    // dummy function
    if (u16_size) {}  // eliminate compiler warnings

    // For logging sent data packets
    if (g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_outgoing_packet_prepare_cb != NULL)
    {
        g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_outgoing_packet_prepare_cb();
    }

    return CFR_E_RETVAL_OK;
}

//    ========== cfr_uartDataTransmitKick  ===========
/*!
      \brief             Currently dummy function is not needed on host side

      \param[in,out]     < none >

      \return           < CFR_E_RETVAL >

*/

void              cfr_uartDataTransmitKick(void)
{
    // dummy function
}

extern "C" void cmbs_int_DataSignal(void);

void cfr_uartReceivedDataNotify(void * pv_ApplRef)
{

    cmbs_int_DataSignal();
}

//    ========== cfr_uartInitialize ===========
/*!
      \brief             open the serial communication interface with relevant parameter sets

      \param[in,out]     pst_Config    pointer to UART configuration

      \return           < int >        if failed returns -1, otherwise 0

*/

int          cfr_uartInitialize(PST_UART_CONFIG pst_Config)
{
    PORTCFG SerialCFG;
    char ch_Port[20];

    // we initialize the COM port in setCMBSSet() with default values
    memset(&SerialCFG, 0, sizeof(PORTCFG));

    g_CPHYSerial.setCMBSSet(SerialCFG);
    g_CPHYSerial.setRXNotifier(cfr_uartReceivedDataNotify);

    sprintf(ch_Port, "\\\\.\\COM%d", pst_Config->u8_Port) ;

    if (! g_CPHYSerial.openPort(ch_Port))
    {
        return -1;
    }

    return 0;
}

extern "C" void  cfr_uartClose(void)
{
    g_CPHYSerial.close();
}
