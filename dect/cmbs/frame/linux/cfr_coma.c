/*!
*  \file       cfr_coma.c
*  \brief      COMA implementation of host side
*  \author
*
*******************************************************************************/

#include <stdlib.h>
#include <stddef.h>  // for offsetof
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h> // we need <sys/select.h>; should be included in <sys/types.h> ???
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "cmbs_int.h"   /* internal API structure and defines */
#include "cfr_coma.h"   /* packet handler */
#include "cfr_debug.h"  /* debug handling */
#include "coma_service_api.h" /* coma api */


#define CFR_COMA_TIMEOUT 1000

static HCOMA_ENDPOINT init_endpoint(const char* ip, const char* port);
static void delete_endpoint(HCOMA_ENDPOINT cmbs_endpoint);

static int s_coma_stop = 0;

//    ========== cfr_cmbsThread ===========
/*!
      \brief            COMA data receive pump. if data is available a message is send
                        to cfr_cmbs task.

      \param[in]        pVoid          pointer to CMBS instance object

      \return           <void *>       return always NULL

*/

void  *           cfr_comaThread(void * pVoid)
{
    PST_CMBS_API_INST pInstance = (PST_CMBS_API_INST)pVoid;
    HCOMA_ENDPOINT fdDevCtl = (HCOMA_ENDPOINT)pInstance->fdDevCtl;
    int msgQId;
    size_t nMsgSize;
    ST_CMBS_LIN_MSG LinMsg;

    msgQId = pInstance->msgQId;

//   CFR_DBG_OUT( "CMBS Thread: ID:%lu running\n", (unsigned long)pthread_self() );

    nMsgSize = sizeof(LinMsg.msgData);

    /* Never ending loop.
       Thread will be exited automatically when parent thread finishes.
    */
    while (1)
    {
        memset(&LinMsg.msgData, 0, sizeof(LinMsg.msgData));

        LinMsg.msgData.nLength = COMA_Service_GetMsg(fdDevCtl, LinMsg.msgData.u8_Data, sizeof(LinMsg.msgData.u8_Data), COMA_TIMEOUT_INFINITE);

        /* Reading available data */
        if (LinMsg.msgData.nLength <= 0)
        {
            if (s_coma_stop)
            {
                CFR_DBG_ERROR("CMBS Thread: Stop COMA...\n");
                delete_endpoint(g_CMBSInstance.fdDevCtl);
                return 0;
            }

            CFR_DBG_ERROR("CMBS Thread: Error read() failed\n");
        }
        else
        {
//               CFR_DBG_OUT( "CMBSThread: received %d bytes\n", LinMsg.msgData.nLength );

            /* Send what we received to callback thread */
            if (msgQId >= 0)
            {
                LinMsg.msgType = 1;

                if (msgsnd(msgQId, &LinMsg, nMsgSize, 0) == -1)
                {
                    CFR_DBG_ERROR("CMBS Thread: msgsnd ERROR:%d\n", errno);
                }
            }
            else
            {
                CFR_DBG_ERROR("CMBS Thread: invalid msgQId:%d\n", msgQId);
            }
        }
    }

    return NULL;
}


//    ==========    cfr_wait_till_characters_transmitted ===========
/*!
      \brief             Wait till characters are transmitted

      \param[in,out]     fd    pointer to packet part

      \return            < none >

*/

void cfr_comaWaitPacketPartTransmitFinished(int fd)
{
    UNUSED_PARAMETER(fd);
}

//    ==========    cfr_comaPacketPartWrite ===========
/*!
      \brief             write partly the packet into communication device

      \param[in,out]     pu8_Buffer    pointer to packet part

      \param[in,out]     u16_Size      size of packet part

      \return            < int >       currently, alway 0

*/

int               cfr_comaPacketPartWrite(u8* pu8_Buffer, u16 u16_Size)
{
    // int    i;

    // CFR_DBG_OUT( "PacketPartWrite: " );
    // for (i=0; i < u16_Size; i++ )
    // {
    //   CFR_DBG_OUT( "%02x ",pu8_Buffer[i] );
    //   write( g_CMBSInstance.fdDevCtl, pu8_Buffer + i, 1 );
    // }
    // CFR_DBG_OUT( "\n" );

    if (COMA_Service_SendMsg((HCOMA_ENDPOINT)g_CMBSInstance.fdDevCtl, pu8_Buffer, u16_Size, CFR_COMA_TIMEOUT) != COMA_SUCCESS)
    {
        printf("COMA: Error sending\n");
        return 0;
    }

// cfr_comaWaitPacketPartTransmitFinished(g_CMBSInstance.fdDevCtl);

    return 0;
}
//    ========== cfr_comaPacketWriteFinish ===========
/*!
      \brief             Currently dummy function is not needed on host side

      \param[in,out]     u8_BufferIDX  buffer index

      \return            < none >

*/

void              cfr_comaPacketWriteFinish(u8 u8_BufferIDX)
{
    // For logging sent data packets
    UNUSED_PARAMETER(u8_BufferIDX);
}

//    ========== cfr_comaPacketPrepare  ===========
/*!
      \brief             Currently dummy function is not needed on host side

      \param[in,out]     u16_size      size of to be submitted packet for transmission

      \return           < CFR_E_RETVAL >

*/

CFR_E_RETVAL      cfr_comaPacketPrepare(u16 u16_size)
{
    // dummy function
    UNUSED_PARAMETER(u16_size);

    return CFR_E_RETVAL_OK;
}

//    ========== cfr_comaDataTransmitKick  ===========
/*!
      \brief             Currently dummy function is not needed on host side

      \param[in,out]     < none >

      \return           < CFR_E_RETVAL >

*/

void              cfr_comaDataTransmitKick(void)
{
    // dummy function
}


//    ========== cfr_comaInitialize ===========
/*!
      \brief             open CMBS COMA link

      \return           < int >        if failed returns -1

*/


int cfr_comaInitialize(void *param)
{
    HCOMA_ENDPOINT cmbs_endpoint = init_endpoint(0, 0);

    if (cmbs_endpoint == (HCOMA_ENDPOINT)0)
    {
        CFR_DBG_ERROR("Error: Can't open COMA link: \"cmbs\"\n");
        return -1;
    }

    UNUSED_PARAMETER(param);

    return (int)cmbs_endpoint;
}

static HCOMA_ENDPOINT init_endpoint(const char* ip, const char* port)
{
    HCOMA_ENDPOINT cmbs_endpoint;

    LINK_DESCRIPTOR LinkDesc;

/// Should come from linux-android/include/linux/netlink.h
#ifndef NETLINK_COMA_CMBS
#define NETLINK_COMA_CMBS 29
#endif

#ifdef USE_UDP_SOCK

    if (!ip || ip[0] == 0) {
        ip = "127.0.0.1";
    }

    if (!port || port[0] == 0)
    {
        printf("init_endpoint() : required port parameter is missing\n");
        return (HCOMA_ENDPOINT)0;
    }

    LinkDesc.LinkInfo.UDP.szIpAddr = ip;
    LinkDesc.LinkInfo.UDP.Port = atoi(port);
#elif defined(USE_COMA_SOCK)
    LinkDesc.LinkInfo.Coma.name = "cmbs";
#else
    LinkDesc.LinkInfo.Netlink.Type = NETLINK_COMA_CMBS;
#endif

    cmbs_endpoint = COMA_Service_CreateEndpoint(DEBUG_SERVICE, LinkDesc);

    if (cmbs_endpoint == 0)
    {
        printf("ERROR: COMA_Service_CreateEndpoint\n");
        return (HCOMA_ENDPOINT)0;
    }

    UNUSED_PARAMETER2(ip, port);

    return cmbs_endpoint;
}

static void delete_endpoint(HCOMA_ENDPOINT cmbs_endpoint)
{
    if (cmbs_endpoint != 0)
        COMA_Service_DeleteEndpoint(cmbs_endpoint);
}

void cfr_comaStop(void)
{
    s_coma_stop = 1;
}
