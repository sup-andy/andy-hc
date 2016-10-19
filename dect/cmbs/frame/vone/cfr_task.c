/*!
* \file   cfr_task.c
* \brief
* \Author  kelbch
*
* @(#) %filespec: cfr_task.c~12.2.1 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
* 03-Dec-2012 tcmc_asa      9   PR 3652 Handle FIFO full issue by buffer reset
* 11-Dec-11  yanivso  9   Return u8Test to file\n
* 14-feb-09  kelbch  1   Initialize\n
*******************************************************************************/

#include "tclib.h"
#include "embedded.h"
#include "cg0type.h"
#include "bsd09cnf.h"            /* component-globalal, system configuration */
#include "bsd09ddl.h"            /* messages and processes */
#include "csys0reg.h"
#include "csys2vpb.h"
#include "csys5os.h"

#if defined (CSS)
#include "plicu.h"
#include "priorities.h"
#endif

#include "cos00int.h"
#include "cmbs_int.h"            /* internal API structure and defines */
#include "cfr_uart.h"            /* interface of uart packagetizer */
#include "tapp.h"
#include "cmbs_fifo.h"

#if defined( CMBS_CATIQ )
#include "tapp_facility.h"
#endif

#include "tapp_log.h"
#include "tapp_facility.h"
#include "cfr_debug.h"

#ifdef DNA_CFR_TASK_DEBUG
#define DNA_CFR_TASK_DBG(x)     printf( (x) );
#else
#define DNA_CFR_TASK_DBG(x)  ;
#endif

#if defined (CMBS_AUDIO_TEST_SLAVE)
extern void    cmbsAudioSlaveChange(u8 u8_key);
#endif
extern E_CMBS_RC _tapp_dsr_OnHsRegistration(u8 u8_HsNr);

u8                 u8Test0[] = "\xda\xda\xda\xda\x0a\x00\x11\x22\x33\x44\x55\x66\x77\x88";
u8                 u8Test1[] = "\xda\xda\xda\xda\x0b\x00\x99\xaa\x11\x22\x44\x66\x44\x99\x31";

extern ST_CMBS_APP       g_CMBSApp;
extern ST_CFR_UARTHDL    g_UARTHandler;

void    cfr_taskPacketTest(u8 u8_IDX)
{
    if (u8_IDX)
        cfr_uartPacketWrite(u8Test1, 14);
    else
        cfr_uartPacketWrite(u8Test0, 15);
}

//  ==========  cfr_taskPacketReceived ===========
/*!
        \brief     A serialized packet is received, check format and
                         pass it to De-serializer

        \param[in,out]   < none >

        \return    < none >

*/

void    cfr_taskPacketReceived(void)
{
    u16  u16_Length;
    u8* pu8_Buffer;

    pu8_Buffer = cfr_uartPacketRead(CFR_BUFFER_UART_REC, &u16_Length);

    if (pu8_Buffer)
    {
        /*
        CFR_DBG_OUT ( "==================================\n" );
        CFR_DBG_OUT ( "CMBS-Task: Packet len %d received \n",u16_Length );

        cfr_uartPacketTrace ( CFR_BUFFER_UART_REC );
        */

        cmbs_int_EventReceive(pu8_Buffer, u16_Length);

        cfr_uartPacketFree(CFR_BUFFER_UART_REC);
        // re-enable transmission if target is in windows
        if (cfr_uartPacketWindowReached(CFR_BUFFER_UART_REC))
        {
            /*!\todo calculate correct last window */
            cmbs_int_cmd_FlowNOKHandle(0);
        }
        else
        {
            /*!\todo calculate correct last window */
            cmbs_int_cmd_FlowRestartHandle(0);
        }
    }
}

extern void   p_dr18_cmbsTransmit(void);
//extern
//ST_CMBS_APP       g_CMBSApp;

#if defined( CMBS_CATIQ )
extern ST_CMBS_FIFO g_FacilityReqFifo[SD09_MAX_NUM_SUB];
#endif

void    CMBSTASK(void)
{
    switch (G_st_os00_Act.u8_Event)
    {
        case   CMBS_TASK_UART_IND:
        {
            u8 u8_Buffer = (u8)G_st_os00_Act.u16_XInfo;

            CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
            cfr_uartDataReceiveGet(&u8_Buffer, 1);
            CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
        }
        break;

        case   CMBS_TASK_PACK_IND:
            // complete packet received
            cfr_taskPacketReceived();
            break;

        case CMBS_TASK_UART_CFM:
            if (g_UARTHandler.u8_UARTDelay != 0x0)
            {
                //CFR_DBG_OUT("\nCMBS_TASK_UART_CFM with delay = %X \n", g_UARTHandler.u8_UARTDelay);

                // Set timer - once it expires complete the transmission
                p_os10_StartTimer((g_UARTHandler.u8_UARTDelay + 1), CMBS_TASK_UART_TIMER_EXP);
                //CFR_DBG_OUT("\nSet timer\n");
                break;
            }
            else
            {
                //fallthrough next case
            }
        case CMBS_TASK_UART_TIMER_EXP:
            // From now on we need exclusive access to transmission of serial port
            CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);
#if defined (CSS)
#if (CMBS_VIA_COMA)
            cfr_COMAWrite();
#else
            cfr_uartWrite();
#endif
#else
            p_dr18_cmbsTransmit();
#endif
            // To avoid deadlock in case of error, leave critical section
            CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);

            break;

        case CMBS_TASK_PACK_REQ:

            break;

        case CMBS_TASK_UART_CMPL:
            // packet is transmitted, kick next packet if needed
            //      CFR_DBG_OUT ( "Kicked by completed package\n" ),

            // From now on we need exclusive access to transmission of serial port
            CFR_CMBS_ENTER_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);

            cfr_uartDataTransmitKick();

            // To avoid deadlock in case of error, leave critical section
            CFR_CMBS_LEAVE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);

            break;
#ifndef DNA_BOOTING
        case CMBS_TASK_HS_REGISTERED:
            //   CFR_DBG_OUT( "CMBS_TASK_HS_REGISTERED\n" );
            //printf("\nCalling p_dr13_SpeedUpWritesAndNotifyWhenEEPIdle on registration\n");
            p_dr13_SpeedUpWritesAndNotifyWhenEEPIdle(_tapp_dsr_OnHsRegistration, (G_st_os00_Act.u16_XInfo & 0x0F));
            //_tapp_dsr_OnHsRegistration();

            /* PR 3652: Reset Facility buffer for the new registered handset.
             * That's a WA, as the real root cause of the FIFO Full issue is not known.
             */
            cmbs_applFacilityFiFoInitialize(G_st_os00_Act.u16_XInfo - 1);

            break;
#endif

        case CMBS_TASK_USB_LINE_STATUS:
            //   CFR_DBG_OUT( "CMBS_TASK_USB_LINE_STATUS\n" );
            break;

#if defined( CMBS_CATIQ )

        case  CMBS_TASK_FACILITY_FIFO_EXPIRED:
        {
            // Index/Instance starts with 0, HS with 1, _tapp_FacilityCallBack() decrements handset number
            _tapp_FacilityCallBack(G_st_os00_Act.u8_Instance + 1, CMBS_RESPONSE_ERROR);
        }
        break;

        case  CMBS_TASK_FACILITY_REQ:
        {
            PST_CMBS_FACILITY pst_Facility;

            u16 u16_HsNum = G_st_os00_Act.u16_XInfo;

            //CFR_DBG_OUT( "CMBS_TASK_FACILITY_REQ HsNum %d", u16_HsNum );

            pst_Facility = (PST_CMBS_FACILITY)cmbs_util_FifoGet(&g_FacilityReqFifo[u16_HsNum]);

            if (pst_Facility)
            {
                if (pst_Facility->u8_State == CMBS_FACILITY_STATE_NEW)     // new, unused facility request
                {
                    pst_Facility->u8_State = tapp_util_FacilityReq(pst_Facility);

                    if (pst_Facility->u8_State == CMBS_FACILITY_STATE_NEW)
                    {
                        // This status means that the message cannot be sent
                        //CFR_DBG_OUT( "CMBS_TASK_FACILITY_REQ RequestId:%d HsNum %d POP (NOT SENT) \n", pst_Facility->u16_RequestId, u16_HsNum );
                        pst_Facility->u8_State = CMBS_FACILITY_STATE_IN_USE;
                        pst_Facility->u8_Status = CMBS_RESPONSE_ERROR;
                        p_os10_PutMsgInfo(CMBSTASK_ID, 0, CMBS_TASK_FACILITY_CNF, u16_HsNum);
                    }
                    else if (pst_Facility->u8_State == CMBS_FACILITY_STATE_IN_USE)
                    {
                        /* FACILITY will be handled again when Scorpion can handle it */
                        /* restore status to NEW */
                        pst_Facility->u8_State = CMBS_FACILITY_STATE_NEW;

                        //CFR_DBG_OUT( "CMBS_TASK_FACILITY_REQ RequestId:%d HsNum %d SEND POSTPONED \n", pst_Facility->u16_RequestId, u16_HsNum );
                    }
                    else
                    {
                        //CFR_DBG_OUT( "CMBS_TASK_FACILITY_REQ consumed \n" );
                    }
                }
                else if (pst_Facility->u8_State == CMBS_FACILITY_STATE_CONSUMED)
                {
                    //CFR_DBG_OUT( "\nCMBS_TASK_FACILITY_REQ waiting for confirmation for a consumed message (Hs num %d)\n", u16_HsNum );
                }
                else
                {
                    CFR_DBG_OUT("CMBS_TASK_FACILITY_REQ HsNum:%d Error - handling FACILITY that is already being handled\n", u16_HsNum);
                }
            }
            else
            {
                //CFR_DBG_OUT( "CMBS_TASK_FACILITY_REQ FIFO[%d] Empty, nothing to send\n" , u16_HsNum);
            }
        }
        break;

        case  CMBS_TASK_FACILITY_CNF:
        {
            PST_CMBS_FACILITY pst_Facility;

            u16 u16_HsNum = G_st_os00_Act.u16_XInfo;

            pst_Facility = (PST_CMBS_FACILITY)cmbs_util_FifoGet(&g_FacilityReqFifo[u16_HsNum]);

            //CFR_DBG_OUT( "CMBS_TASK_FACILITY_CNF facility state %d HsNum %d\n" , pst_Facility->u8_State, u16_HsNum);

            if (pst_Facility)
            {
                if (pst_Facility->u8_State == CMBS_FACILITY_STATE_CONSUMED || pst_Facility->u8_State == CMBS_FACILITY_STATE_IN_USE)
                {
                    // Send response to Host
                    tapp_util_FacilityCnf(pst_Facility, u16_HsNum);

                    // remove from FIFO
                    cmbs_util_FifoPop(&g_FacilityReqFifo[u16_HsNum]);
                }
                else
                {
                    CFR_DBG_OUT("\n Received CNF for not handled FACILITY (HsNum %d)!!\n", u16_HsNum);
                }
                // meanwhile, there might be another request in the FIFO.
                // retrigger a facility request
                p_os10_PutMsgInfo(CMBSTASK_ID, 0, CMBS_TASK_FACILITY_REQ, u16_HsNum);
            }
            else
            {
                CFR_DBG_OUT("CMBS_TASK_FACILITY_CNF Error - FIFO[%d] empty!\n", u16_HsNum);
            }
        }
        break;
#endif // defined( CMBS_CATIQ )

        case  CMBS_TASK_TIMED_REL_LINE:
        {
            PST_CMBS_APP_LINE
            pst_Line;

            CFR_DBG_ERROR("\nCMBS_TASK_TIMED_REL_LINE: CallRelease timed out !!!!\n");
            pst_Line = tapp_util_LineGet(&g_CMBSApp, TCMBS_APP_LINE_ILLEGAL, (u8)G_st_os00_Act.u16_XInfo);

            if (pst_Line)
            {
                cmbs_dee_CallReleaseComplete(NULL/*pv_AppRefHandle*/, pst_Line->u32_CallInstance);
                tapp_util_LineFree(pst_Line);
            }
            else
            {
                CFR_DBG_ERROR("\nCMBS_TASK_TIMED_REL_LINE: No line found for CallSessionID %u !!!!\n",  G_st_os00_Act.u16_XInfo);
            }
            break;
        }
#ifdef CMBS_USB_NO_HOST  // automated call answer to start USB audio
        case  CMBS_TASK_NOHOST_START_MEDIA:
        {
            PST_CMBS_APP_LINE       pst_Line;
            u32 u32_CallInstance = G_st_os00_Act.u16_XInfo;
            pst_Line = tapp_util_LineGet(&g_CMBSApp, u32_CallInstance, 0);

            // call progress
            {
                ST_IE_MEDIA_DESCRIPTOR st_MediaDesc;
                ST_IE_CALLPROGRESS   st_CallProgress;
                void* IEList = cmbs_api_ie_GetList();

                // Add call Instance IE
                cmbs_api_ie_CallInstanceAdd(IEList, u32_CallInstance);

                // add media descriptor IE
                memset(&st_MediaDesc, 0, sizeof(ST_IE_MEDIA_DESCRIPTOR));
                st_MediaDesc.e_Codec = pst_Line->u8_Codec;
                cmbs_api_ie_MediaDescAdd(IEList, &st_MediaDesc);

                // add progress
                memset(&st_CallProgress, 0, sizeof(ST_IE_CALLPROGRESS));
                st_CallProgress.e_Progress = CMBS_CALL_PROGR_SETUP_ACK;
                cmbs_api_ie_CallProgressAdd(IEList, &st_CallProgress);

                /* Add Line-Id */
                pst_Line->u8_LineId = 255;
                cmbs_api_ie_LineIdAdd(IEList, pst_Line->u8_LineId);

                tapp_dee_CallProgress(&g_CMBSApp, IEList);
            }
            // start media
            {
                ST_IE_MEDIA_CHANNEL  _MediaChannel;
                void* IEList = cmbs_api_ie_GetList();

                _MediaChannel.e_Type = CMBS_MEDIA_TYPE_AUDIO_USB;
                _MediaChannel.u32_ChannelID = 0;
                _MediaChannel.u32_ChannelParameter = 0;
                cmbs_api_ie_MediaChannelAdd(IEList, &_MediaChannel);

                // making USB audio always WB
                pst_Line->u8_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;

                tapp_dem_ChannelStart(&g_CMBSApp, IEList);
            }

            p_os10_PutMsgInfo(CMBSTASK_ID, 0, CMBS_TASK_NOHOST_ANSWER, u32_CallInstance);

            break;
        }
        case  CMBS_TASK_NOHOST_ANSWER:
        {
            u32 u32_CallInstance = G_st_os00_Act.u16_XInfo;

            // call answer
            {
                void* IEList = cmbs_api_ie_GetList();

                // Add call Instance IE
                cmbs_api_ie_CallInstanceAdd(IEList, u32_CallInstance);

                tapp_dee_CallAnswer(&g_CMBSApp, IEList);
            }

            break;
        }
#endif
        /*
           case FTMI_RINGER_ON_EXP:
           {
            PST_CFR_IE_LIST p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

              cmbs_api_ie_IntValueAdd( (void*)p_List, 0 );

              tapp_dsr_HandsetPage( &g_CMBSApp, p_List );
           }
              break;
        */
        //  ========== T ===========
        /*!
        \brief     short description

        \param[in,out]   CMBS_TASK_KEYBOARD_HIT:   description

        \return         case CMBS_TASK_KEYBOARD_HI    description

        */

#if defined (CMBS_AUDIO_TEST_SLAVE)
        case CMBS_TASK_KEYBOARD_HIT:
//      printf ( " KEY %02x\n", (u8)G_st_os00_Act.u16_XInfo);
            cmbsAudioSlaveChange((u8)G_st_os00_Act.u16_XInfo);
            break;

#endif
            // Delayed reset: Used by booter upgrade, or by factory default etc.
        case CMBS_TASK_RESET_AFTER_TIMER:
            CFR_DBG_INFO("CMBS_TASK_RESET_AFTER_TIMER arrived\n");

#if !defined (CSS)
            {
                extern u8 G_u8_os09_ForceWatchDog;
                CFR_DBG_INFO("\nSystem Reset\n");
                // perform an immediate watchdog reset
                VOLATILE(sys0_SCU.rst_del, u32) &= ~SYS0_SCU_RST_SRC_MASK;
                SYS5_DISABLE_ALL;
                G_u8_os09_ForceWatchDog = 1;
                // WD load enable
                VOLATILE(sys2_TIM.wdt_con1, u32) = SYS2_WDT_LENB;
                // set WD interval to minimal value
                VOLATILE(sys2_TIM.wdt_val, u32) = 1;
                // make sure WD is enabled
                VOLATILE(sys2_TIM.wdt_con2, u32) = SYS2_WDT_ACT;
                // wait for WD reset
                while (1);
            }
#else
            // CSS has to request it from host no WDT
            cmbs_int_EventSend(CMBS_EV_DSR_SYS_RESET, NULL, 0);
#endif
            break;


            //host suspend resume
        case CMBS_TASK_SEND_CMD_FLASH_START_REQ:
            CFR_DBG_INFO("CMBS_TASK_SEND_CMD_FLASH_START_REQ  - Start request to host \n");
            p_cmbs14_SuspendTxReq();
            break;

        case CMBS_TASK_SEND_CMD_FLASH_STOP_REQ:
            CFR_DBG_INFO("CMBS_TASK_SEND_CMD_FLASH_STOP_REQ  - Stop req to host \n");
            p_cmbs14_ResumeTxReq();
            break;
        default:
            CFR_DBG_INFO("CMBS-TASK: default ev handler %d\n", G_st_os00_Act.u8_Event);
    }
}

//*/
