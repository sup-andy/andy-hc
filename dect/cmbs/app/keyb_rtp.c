/*!
*   \file       keyb_rtp.c
*   \brief      RTP Interface Tests
*   \author     Denis Matiukha
*
*   @(#)        keyb_rtp.c~1
*
*******************************************************************************
*   \par    History
* \n==== History ===============================================================\n
*   date        name        version     action                                  \n
*   ----------------------------------------------------------------------------\n
*   13-Mar-11   denism      01          Initial revision                        \n
*   19-Apr-11   denism      02          Added test cases with predefined parameters\n
*   07-May-11   denism      03          Added FAX API tests\n
*   15-May-11   denism      04          Used app-layer abstraction interface    \n
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if ! defined ( WIN32 )
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/msg.h>
#else
#include <conio.h>
#include <io.h>
#endif

#include <fcntl.h>
#include <errno.h>

#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "tcx_keyb.h"
#include "appmsgparser.h"
#include "apprtp.h"
#include <tcx_util.h>

bool _keyb_input_d(const char * sz_Prompt, int * pi_Value)
{
    char sz_Buffer[256];
    const size_t cb_BufferSize = sizeof(sz_Buffer) / sizeof(sz_Buffer[0]);
    char t;

    puts(sz_Prompt);

    tcx_gets(sz_Buffer, cb_BufferSize);
    sz_Buffer[cb_BufferSize - 1] = '\0';

    return sscanf(sz_Buffer, "%d%c", pi_Value, &t) == 1;
}

bool _keyb_input_lu(const char * sz_Prompt, long unsigned * plu_Value)
{
    char sz_Buffer[256];
    const size_t cb_BufferSize = sizeof(sz_Buffer) / sizeof(sz_Buffer[0]);
    char t;

    puts(sz_Prompt);

    tcx_gets(sz_Buffer, cb_BufferSize);
    sz_Buffer[cb_BufferSize - 1] = '\0';

    return sscanf(sz_Buffer, "%lu%c", plu_Value, &t) == 1;
}

bool _keyb_input_u8(const char * sz_Prompt, u8 * pu8_Value)
{
    int i_Value = 0;
    if (_keyb_input_d(sz_Prompt, &i_Value) && i_Value >= 0 && i_Value <= 255)
    {
        *pu8_Value = (u8)i_Value;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

u8 _keyb_input_u8_loop(const char * sz_Prompt)
{
    u8 u8_Value;
    while (!_keyb_input_u8(sz_Prompt, &u8_Value))
    {
        puts("The value should be in rage of 0-255. Please input the correct value.");
    }
    return u8_Value;
}

bool _keyb_input_u16(const char * sz_Prompt, u16 * pu16_Value)
{
    int i_Value = 0;
    if (_keyb_input_d(sz_Prompt, &i_Value) && i_Value >= 0 && i_Value <= 65535)
    {
        *pu16_Value = (u16)i_Value;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

u16 _keyb_input_u16_loop(const char * sz_Prompt)
{
    u16 u16_Value;
    while (!_keyb_input_u16(sz_Prompt, &u16_Value))
    {
        puts("The value should be in rage of 0-65535. Please input the correct value.");
    }
    return u16_Value;
}

bool _keyb_input_u32(const char * sz_Prompt, u32 * pu32_Value)
{
    long unsigned lu_Value = 0;
    if (_keyb_input_lu(sz_Prompt, &lu_Value) && lu_Value <= 0xFFFFFFFFlu)
    {
        *pu32_Value = (u32)lu_Value;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

u32 _keyb_input_u32_loop(const char * sz_Prompt)
{
    u32 u32_Value;
    while (!_keyb_input_u32(sz_Prompt, &u32_Value))
    {
        puts("The value should be in rage of 0-4294967295. Please input the correct value.");
    }
    return u32_Value;
}

void _keyb_input_str_loop(const char * sz_Prompt, char * p_Buffer, unsigned long cb_BufferSize)
{
    char sz_Buffer[256] = {0};

    for (;;)
    {
        puts(sz_Prompt);

        tcx_gets(sz_Buffer, sizeof(sz_Buffer));

        if (strlen(sz_Buffer) >= cb_BufferSize)
        {
            printf("The input string should be no longer than %lu chars. Please input a shorter string.\n", cb_BufferSize - 1);
            continue;
        }

        strcpy(p_Buffer, sz_Buffer);
        return;
    }
}

void _keyb_input_rtp_session_info(ST_IE_RTP_SESSION_INFORMATION * pst_RTPSessionInformation)
{
    int i;
    for (i = 0; i < CMBS_MAX_NUM_OF_CODECS; ++i)
    {
        printf("*** RX codec %d ***\n", i);
        pst_RTPSessionInformation->ast_RxCodecList[i].u8_CodecPt = _keyb_input_u8_loop("Payload Type: ");
        _keyb_input_str_loop("Name (optional): ",  pst_RTPSessionInformation->ast_RxCodecList[i].sz_CodecStr, CMBS_MAX_DYNAMIC_CODEC_LEN);
    }
    pst_RTPSessionInformation->u8_RxCodecEventPt        = _keyb_input_u8_loop("RX Event Payload Type: ");
    pst_RTPSessionInformation->st_TxCodec.u8_CodecPt    = _keyb_input_u8_loop("TX Codec Payload Type: ");
    _keyb_input_str_loop("TX Codec Name (optional): ",  pst_RTPSessionInformation->st_TxCodec.sz_CodecStr, CMBS_MAX_DYNAMIC_CODEC_LEN);
    pst_RTPSessionInformation->u8_TxCodecEventPt        = _keyb_input_u8_loop("TX Event Payload Type: ");
    pst_RTPSessionInformation->u32_Capabilities         = _keyb_input_u32_loop("Capabilities: ");
    _keyb_input_str_loop("SDes Name: ", pst_RTPSessionInformation->sz_SDesName, CMBS_RTCP_MAX_SDES);
    pst_RTPSessionInformation->u16_Duration             = _keyb_input_u16_loop("Duration: ");
    pst_RTPSessionInformation->u32_CurrentTime          = _keyb_input_u32_loop("Current Time: ");
    pst_RTPSessionInformation->u32_Timestamp            = _keyb_input_u32_loop("Timestamp: ");
    pst_RTPSessionInformation->u32_SSRC                 = _keyb_input_u32_loop("SSRC: ");
    pst_RTPSessionInformation->u8_JBMinLen              = _keyb_input_u8_loop("Jitter Buffer Min Length: ");
    pst_RTPSessionInformation->u8_JBMaxLen              = _keyb_input_u8_loop("Jitter Buffer Max Length: ");
    for (;;)
    {
        puts("Jitter Buffer Mode (F - Fixed, A - Adaptive):");
        switch (tcx_getch())
        {
            case 'f':
            case 'F':
                pst_RTPSessionInformation->e_JBMode = CMBS_RTP_JB_MODE_FIXED;
                break;
            case 'a':
            case 'A':
                pst_RTPSessionInformation->e_JBMode = CMBS_RTP_JB_MODE_ADAPTIVE;
                break;
            default:
                printf("\nWrong input!\n");
                continue;
        }
        break;
    }
    pst_RTPSessionInformation->u32_DTMFEndPackets       = _keyb_input_u32_loop("\nDTMF End Packets: ");
    for (;;)
    {
        puts("Media Loop Mode (N - None, D - DSP Level, R - RTP Level):");
        switch (tcx_getch())
        {
            case 'n':
            case 'N':
                pst_RTPSessionInformation->e_MediaLoopLevel = CMBS_VOICE_LOOP_NONE;
                break;
            case 'd':
            case 'D':
                pst_RTPSessionInformation->e_MediaLoopLevel = CMBS_VOICE_LOOP_DSP_LEVEL;
                break;
            case 'r':
            case 'R':
                pst_RTPSessionInformation->e_MediaLoopLevel = CMBS_VOICE_LOOP_RTP_LEVEL;
                break;
            default:
                printf("\nWrong input!\n");
                continue;
        }
        break;
    }
    pst_RTPSessionInformation->u16_T38LsRedundancy      = _keyb_input_u16_loop("\nT38 LS Redundancy: ");
    pst_RTPSessionInformation->u16_T38HsRedundancy      = _keyb_input_u16_loop("T38 LS Redundancy: ");
    pst_RTPSessionInformation->u8_T38EcnOn              = _keyb_input_u8_loop("T38 Ecn On: ");
    for (;;)
    {
        puts("Audio Mode (A - Active, R - RecvOnly, S - SendOnly, I - Inactive):");
        switch (tcx_getch())
        {
            case 'a':
            case 'A':
                pst_RTPSessionInformation->e_AudioMode = CMBS_AUDIO_MODE_ACTIVE;
                break;
            case 'r':
            case 'R':
                pst_RTPSessionInformation->e_AudioMode = CMBS_AUDIO_MODE_REC_ONLY;
                break;
            case 's':
            case 'S':
                pst_RTPSessionInformation->e_AudioMode = CMBS_AUDIO_MODE_SEND_ONLY;
                break;
            case 'i':
            case 'I':
                pst_RTPSessionInformation->e_AudioMode = CMBS_AUDIO_MODE_INACTIVE;
                break;
            default:
                printf("\nWrong input!\n");
                continue;
        }
        break;
    }
    puts("");
}

void              keyb_RTPSessionStart(void)
{
    u32                             u32_ChannelID;
    ST_IE_RTP_SESSION_INFORMATION   st_RTPSessionInformation;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    _keyb_input_rtp_session_info(&st_RTPSessionInformation);

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSessionStart(u32_ChannelID, &st_RTPSessionInformation);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SESSION_START_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSessionStop(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSessionStop(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SESSION_STOP_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSessionUpdate(void)
{
    u32                             u32_ChannelID;
    ST_IE_RTP_SESSION_INFORMATION   st_RTPSessionInformation;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    _keyb_input_rtp_session_info(&st_RTPSessionInformation);

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSessionUpdate(u32_ChannelID, &st_RTPSessionInformation);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SESSION_UPDATE_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTCPSessionStart(void)
{
    u32                             u32_ChannelID;
    u32                             u32_RTCPInterval;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    u32_RTCPInterval = _keyb_input_u32_loop("RTCP Interval: ");

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTCPSessionStart(u32_ChannelID, u32_RTCPInterval);

    appcmbs_WaitForContainer(CMBS_EV_RTCP_SESSION_START_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTCPSessionStop(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTCPSessionStop(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTCP_SESSION_STOP_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSendDTMF(void)
{
    u32                             u32_ChannelID;
    ST_IE_RTP_DTMF_EVENT            st_RTPDTMFEvent;
    ST_IE_RTP_DTMF_EVENT_INFO       st_RTPDTMFEventInfo;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    st_RTPDTMFEvent.u8_Event        = _keyb_input_u8_loop("Event: ");
    st_RTPDTMFEvent.u16_Volume      = _keyb_input_u16_loop("Volume: ");
    st_RTPDTMFEvent.u16_Duration    = _keyb_input_u16_loop("Duration: ");

    st_RTPDTMFEventInfo.u16_EventDuration    = _keyb_input_u16_loop("Event Duration: ");
    st_RTPDTMFEventInfo.u16_MaxEventDuration = _keyb_input_u16_loop("Max Event Duration: ");

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSendDTMF(u32_ChannelID, &st_RTPDTMFEvent, &st_RTPDTMFEventInfo);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SEND_DTMF_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPEnableFaxAudioProcessingMode(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPEnableFaxAudioProcessingMode(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTP_ENABLE_FAX_AUDIO_PROCESSING_MODE_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPDisableFaxAudioProcessingMode(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = _keyb_input_u16_loop("\nChannel ID: ");

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPDisableFaxAudioProcessingMode(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTP_DISABLE_FAX_AUDIO_PROCESSING_MODE_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSessionStart2(void)
{
    u32                             u32_ChannelID;
    ST_IE_RTP_SESSION_INFORMATION   st_RTPSessionInformation;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;
    int                             i;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    for (i = 0; i < CMBS_MAX_NUM_OF_CODECS; ++i)
    {
        st_RTPSessionInformation.ast_RxCodecList[i].u8_CodecPt = i + 1;
        sprintf(st_RTPSessionInformation.ast_RxCodecList[i].sz_CodecStr, "Codec%d", i);
    }
    st_RTPSessionInformation.u8_RxCodecEventPt        = 1;
    st_RTPSessionInformation.st_TxCodec.u8_CodecPt    = 1;
    strcpy(st_RTPSessionInformation.st_TxCodec.sz_CodecStr, "Codec1");
    st_RTPSessionInformation.u8_TxCodecEventPt        = 1;
    st_RTPSessionInformation.u32_Capabilities         = 1;
    strcpy(st_RTPSessionInformation.sz_SDesName, "SDES");
    st_RTPSessionInformation.u16_Duration             = 1;
    st_RTPSessionInformation.u32_CurrentTime          = 1;
    st_RTPSessionInformation.u32_Timestamp            = 1;
    st_RTPSessionInformation.u32_SSRC                 = 1;
    st_RTPSessionInformation.u8_JBMinLen              = 1;
    st_RTPSessionInformation.u8_JBMaxLen              = 1;
    st_RTPSessionInformation.e_JBMode = CMBS_RTP_JB_MODE_FIXED;
    st_RTPSessionInformation.u32_DTMFEndPackets       = 1;
    st_RTPSessionInformation.e_MediaLoopLevel = CMBS_VOICE_LOOP_DSP_LEVEL;
    st_RTPSessionInformation.u16_T38LsRedundancy      = 1;
    st_RTPSessionInformation.u16_T38HsRedundancy      = 1;
    st_RTPSessionInformation.u8_T38EcnOn              = 1;
    st_RTPSessionInformation.e_AudioMode = CMBS_AUDIO_MODE_ACTIVE;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSessionStart(u32_ChannelID, &st_RTPSessionInformation);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SESSION_START_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSessionStop2(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSessionStop(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SESSION_STOP_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSessionUpdate2(void)
{
    u32                             u32_ChannelID;
    ST_IE_RTP_SESSION_INFORMATION   st_RTPSessionInformation;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;
    int                             i;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    for (i = 0; i < CMBS_MAX_NUM_OF_CODECS; ++i)
    {
        st_RTPSessionInformation.ast_RxCodecList[i].u8_CodecPt = i + 1;
        sprintf(st_RTPSessionInformation.ast_RxCodecList[i].sz_CodecStr, "Codec%d", i);
    }
    st_RTPSessionInformation.u8_RxCodecEventPt        = 1;
    st_RTPSessionInformation.st_TxCodec.u8_CodecPt    = 1;
    strcpy(st_RTPSessionInformation.st_TxCodec.sz_CodecStr, "Codec1");
    st_RTPSessionInformation.u8_TxCodecEventPt        = 1;
    st_RTPSessionInformation.u32_Capabilities         = 1;
    strcpy(st_RTPSessionInformation.sz_SDesName, "SDES");
    st_RTPSessionInformation.u16_Duration             = 1;
    st_RTPSessionInformation.u32_CurrentTime          = 1;
    st_RTPSessionInformation.u32_Timestamp            = 1;
    st_RTPSessionInformation.u32_SSRC                 = 1;
    st_RTPSessionInformation.u8_JBMinLen              = 1;
    st_RTPSessionInformation.u8_JBMaxLen              = 1;
    st_RTPSessionInformation.e_JBMode = CMBS_RTP_JB_MODE_FIXED;
    st_RTPSessionInformation.u32_DTMFEndPackets       = 1;
    st_RTPSessionInformation.e_MediaLoopLevel = CMBS_VOICE_LOOP_DSP_LEVEL;
    st_RTPSessionInformation.u16_T38LsRedundancy      = 1;
    st_RTPSessionInformation.u16_T38HsRedundancy      = 1;
    st_RTPSessionInformation.u8_T38EcnOn              = 1;
    st_RTPSessionInformation.e_AudioMode = CMBS_AUDIO_MODE_ACTIVE;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSessionUpdate(u32_ChannelID, &st_RTPSessionInformation);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SESSION_UPDATE_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTCPSessionStart2(void)
{
    u32                             u32_ChannelID;
    u32                             u32_RTCPInterval;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    u32_RTCPInterval = 1;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTCPSessionStart(u32_ChannelID, u32_RTCPInterval);

    appcmbs_WaitForContainer(CMBS_EV_RTCP_SESSION_START_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTCPSessionStop2(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTCPSessionStop(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTCP_SESSION_STOP_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPSendDTMF2(void)
{
    u32                             u32_ChannelID;
    ST_IE_RTP_DTMF_EVENT            st_RTPDTMFEvent;
    ST_IE_RTP_DTMF_EVENT_INFO       st_RTPDTMFEventInfo;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    st_RTPDTMFEvent.u8_Event        = 1;
    st_RTPDTMFEvent.u16_Volume      = 2;
    st_RTPDTMFEvent.u16_Duration    = 3;

    st_RTPDTMFEventInfo.u16_EventDuration    = 1;
    st_RTPDTMFEventInfo.u16_MaxEventDuration = 2;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPSendDTMF(u32_ChannelID, &st_RTPDTMFEvent, &st_RTPDTMFEventInfo);

    appcmbs_WaitForContainer(CMBS_EV_RTP_SEND_DTMF_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPEnableFaxAudioProcessingMode2(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPEnableFaxAudioProcessingMode(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTP_ENABLE_FAX_AUDIO_PROCESSING_MODE_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPDisableFaxAudioProcessingMode2(void)
{
    u32                             u32_ChannelID;
    ST_APPCMBS_CONTAINER            st_Container;
    u8                              msgparserEnabled;

    //
    // Input arguments.
    //

    u32_ChannelID = 0;

    //
    // Send the request and wait for the response.
    //

    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);
    appcmbs_PrepareRecvAdd(1);

    app_RTPDisableFaxAudioProcessingMode(u32_ChannelID);

    appcmbs_WaitForContainer(CMBS_EV_RTP_DISABLE_FAX_AUDIO_PROCESSING_MODE_RES, &st_Container);

    printf("Done!");

    app_set_msgparserEnabled(msgparserEnabled);
}

void              keyb_RTPTestLoop(void)
{
    int n_Keep = TRUE;

    while (n_Keep)
    {
        // tcx_appClearScreen();
        printf("\n------------------------------------\n");
        printf("1 => Start a RTP session\n");
        printf("2 => Stop an ongoing RTP session\n");
        printf("3 => Update a RTP session\n");
        printf("4 => Start a RTCP session\n");
        printf("5 => Stop an ongoing RTCP session\n");
        printf("6 => Send RTP DTMF\n");
        printf("7 => Enable Fax Audio Processing\n");
        printf("8 => Disable Fax Audio Processing\n");
        printf("a => Start a RTP session (with predefined parameters)\n");
        printf("s => Stop an ongoing RTP session (with predefined parameters)\n");
        printf("d => Update a RTP session (with predefined parameters)\n");
        printf("f => Start a RTCP session (with predefined parameters)\n");
        printf("g => Stop an ongoing RTCP session (with predefined parameters)\n");
        printf("h => Send RTP DTMF (with predefined parameters)\n");
        printf("j => Enable Fax Audio Processing (with predefined parameters)\n");
        printf("k => Disable Fax Audio Processing (with predefined parameters)\n");
        printf("- - - - - - - - - - - - - - -    \n");
        printf("q => Return to Interface Menu\n");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case '1':
                keyb_RTPSessionStart();
                break;

            case '2':
                keyb_RTPSessionStop();
                break;

            case '3':
                keyb_RTPSessionUpdate();
                break;

            case '4':
                keyb_RTCPSessionStart();
                break;

            case '5':
                keyb_RTCPSessionStop();
                break;

            case '6':
                keyb_RTPSendDTMF();
                break;

            case '7':
                keyb_RTPEnableFaxAudioProcessingMode();
                break;

            case '8':
                keyb_RTPDisableFaxAudioProcessingMode();
                break;

            case 'a':
            case 'A':
                keyb_RTPSessionStart2();
                break;

            case 's':
            case 'S':
                keyb_RTPSessionStop2();
                break;

            case 'd':
            case 'D':
                keyb_RTPSessionUpdate2();
                break;

            case 'f':
            case 'F':
                keyb_RTCPSessionStart2();
                break;

            case 'g':
            case 'G':
                keyb_RTCPSessionStop2();
                break;

            case 'h':
            case 'H':
                keyb_RTPSendDTMF2();
                break;

            case 'j':
            case 'J':
                keyb_RTPEnableFaxAudioProcessingMode2();
                break;

            case 'k':
            case 'K':
                keyb_RTPDisableFaxAudioProcessingMode2();
                break;

            case 'q':
            case 'Q':
                n_Keep = FALSE;
                break;
        }
    }
}
