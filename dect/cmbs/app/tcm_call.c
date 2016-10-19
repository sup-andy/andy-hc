#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "cmbs_platf.h"


#include "cmbs_int.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "appswup.h"
#include "tcx_util.h"
#include "cmbs_voipline.h"
#include "hanfun_protocol_defs.h"
#include "ctrl_common_lib.h"
#include "tcm_call.h"

//u16 g_hs_callid[CMBS_HS_SUBSCRIBED_MAX_NUM];

/* Endpoint state machine states */
typedef enum
{
    CMST_ONHOOK = 0,
    CMST_DIALING,
    CMST_WAITANSWER,
    CMST_RINGING,
    CMST_TALK,
    CMST_WAITONHOOK,
    CMST_CWTALK,                   /* talk while call-waiting (not call-hold) */
    CMST_CWXFER,                /*  Received Refer request in CW state*/
    CMST_EPMAX
} CMEP_STATE;

typedef struct
{
    int list_length;
    int call_list[CMBS_HS_SUBSCRIBED_MAX_NUM];
} TCM_CALL_OBJ;

static CMEP_STATE g_EndptState = CMST_ONHOOK;
static ST_IE_CALLEDPARTY oldDigit;
//static u16 cmbsCallID = 0;
static char oldDigitbuf[128];


static TCM_CALL_OBJ g_tcm_call[TCM_CALL_MAX_NUM];

#define CC_MAXDSTR      64  /* Max length of the dial string / user name */


extern const char * cmbs_dbg_GetCallStateName(E_CMBS_CALL_STATE_STATUS  e_CallStatus);

int tcm_HandleEndPointCallEvent(ST_APPCMBS_LINUX_CONTAINER *pContainer)
{
    //int endpt = 0;
    //u16 cmbs_callID = 0;
    int event = pContainer->Content.n_Event;
    int i = 0;

    oldDigit.pu8_Address = (u8*)oldDigitbuf;

    if (event == CMBS_EV_DEE_CALL_ESTABLISH
            || event == CMBS_EV_DCM_CALL_STATE
            || event == CMBS_EV_DEE_CALL_INBANDINFO
            || event == CMBS_EV_DEE_CALL_PROGRESS
            || event == CMBS_EV_DEE_CALL_ANSWER
            || event == CMBS_EV_DEE_CALL_RELEASE
            || event == CMBS_EV_DEE_CALL_RELEASECOMPLETE
            || event == CMBS_EV_DEE_CALL_HOLD
            || event == CMBS_EV_DEE_CALL_RESUME)
    {
        printf("Event: %d\n", pContainer->Content.n_Event);
        //cmbsCallID = pContainer->Content.n_Info;
    }
    else
    {
        return -1;
    }
    if (event == CMBS_EV_DEE_CALL_ESTABLISH)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_ESTABLISH callID:%d\n", pContainer->Content.n_Info);
        //gNotifyCb(endpt, AC_EPEVT_OFFHOOK);
#if 0
        g_tcm_call[0].list_length = 1;
        g_tcm_call[0].call_list[0] = pContainer->Content.n_Info;
        dect_call_request_offhook(0);
#endif
        oldDigit.u8_AddressLen = 0;
        g_EndptState = CMST_DIALING;
    }
    else if (event == CMBS_EV_DCM_CALL_STATE)
    {
        ST_IE_CALL_STATE * pState = (ST_IE_CALL_STATE *)pContainer->Content.ch_Info;
        PST_CALL_OBJ pstCall = _appcall_CallObjGetById(pContainer->Content.n_Info);

        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "CMBS_EV_DCM_CALL_STATE, %s \n", cmbs_dbg_GetCallStateName(pState->e_CallStatus));

        if (!pstCall->b_Incoming && pState->e_CallStatus == CMBS_CALL_STATE_STATUS_CALL_SETUP_ACK)
        {
            int line = pstCall->u8_LineId;
            if (line >= TCM_CALL_MAX_NUM)
            {
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error: invalid lineid:%d \n", line);
                line = 0;
            }
            g_tcm_call[line].list_length = 1;
            g_tcm_call[line].call_list[0] = pContainer->Content.n_Info;
            dect_call_request_offhook(line);
        }
    }
    else if (event == CMBS_EV_DEE_CALL_INBANDINFO)
    {
        PST_CALL_OBJ pstCall = _appcall_CallObjGetById(pContainer->Content.n_Info);
        if (pstCall == NULL)
            return -1;

        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "CMBS_EV_DEE_CALL_INBANDINFO, length:%d, old length:%d,lineid:%d \n", pstCall->st_CalledParty.u8_AddressLen, oldDigit.u8_AddressLen, pstCall->u8_LineId);
        if (!pstCall->b_Incoming)
        {
            /* send offhook here */
            int line = pstCall->u8_LineId;
            if (line >= TCM_CALL_MAX_NUM)
            {
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error: invalid lineid:%d \n", line);
                line = 0;
            }
            //g_tcm_call[line].list_length = 1;
            //g_tcm_call[line].call_list[0] = pContainer->Content.n_Info;
            //dect_call_request_offhook(line);


            if ((oldDigit.u8_AddressLen > 0)
                    //&& ((oldDigit.u8_AddressLen + 1) ==  pstCall->st_CalledParty.u8_AddressLen)
                    && (memcmp(oldDigit.pu8_Address, pstCall->st_CalledParty.pu8_Address, oldDigit.u8_AddressLen) == 0))
            {
                for (i = oldDigit.u8_AddressLen; i < pstCall->st_CalledParty.u8_AddressLen; i ++)
                {
                    //printf("----new %c\n", pstCall->st_CalledParty.pu8_Address[i]);
                    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dialed number:%c \n", pstCall->st_CalledParty.pu8_Address[i]);
                    //gNotifyCb(endpt, epDigit2EpEvent(pstCall->st_CalledParty.pu8_Address[i] - '0'));
                    dect_call_request_dial(pstCall->st_CalledParty.pu8_Address[i] - '0', line);
                    //usleep(3000);
                }
            }
            else
            {
                for (i = 0; i < pstCall->st_CalledParty.u8_AddressLen; i ++)
                {
                    //printf("---- %c\n", pstCall->st_CalledParty.pu8_Address[i]);
                    //gNotifyCb(endpt, epDigit2EpEvent(pstCall->st_CalledParty.pu8_Address[i] - '0'));
                    dect_call_request_dial(pstCall->st_CalledParty.pu8_Address[i] - '0', line);
                    //usleep(3000);
                }
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dialed number:%s \n", pstCall->st_CalledParty.pu8_Address);

            }
            oldDigit.u8_AddressLen = pstCall->st_CalledParty.u8_AddressLen;
            memcpy(oldDigit.pu8_Address, pstCall->st_CalledParty.pu8_Address, pstCall->st_CalledParty.u8_AddressLen);
        }
        g_EndptState = CMST_WAITANSWER;

    }
    else if (event == CMBS_EV_DEE_CALL_PROGRESS)
    {
        if (CMST_ONHOOK == g_EndptState)
        {
            g_EndptState = CMST_RINGING;
        }
        oldDigit.u8_AddressLen = 0;
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_PROGRESS\n");
    }
    else if (event == CMBS_EV_DEE_CALL_ANSWER)
    {
        oldDigit.u8_AddressLen = 0;
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_ANSWER callid:%d\n",  pContainer->Content.n_Info);
        //gNotifyCb(endpt, AC_EPEVT_OFFHOOK);
        PST_CALL_OBJ pstCall = _appcall_CallObjGetById(pContainer->Content.n_Info);
        PST_CALL_OBJ p = NULL;
        int line = pstCall->u8_LineId;
        int i = 0;

        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "pstCall->ch_CallerID:%s u8_LineId:%u\n", pstCall->ch_CalledID, line);
        if (line >= TCM_CALL_MAX_NUM)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error: invalid lineid:%d \n", line);
            line = 0;
        }

        //int handset = atoi(pstCall->ch_CalledID);
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "list_length:%d\n", g_tcm_call[line].list_length);
        for (i = 0; i < g_tcm_call[line].list_length; i++)
        {
            if (pContainer->Content.n_Info == g_tcm_call[line].call_list[i])
            {
                continue;
            }
            ST_APPCALL_PROPERTIES st_Properties;
            st_Properties.e_IE  = CMBS_IE_CALLRELEASE_REASON;
            st_Properties.psz_Value = "";
            p = _appcall_CallObjGetById(g_tcm_call[line].call_list[i]);
            p->stopring = 1;
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "call answered, stop other hansets ring, callid[%d] lineid [%d]\n", g_tcm_call[line].call_list[i], line);
            appcall_ReleaseCall(&st_Properties, 1, g_tcm_call[line].call_list[i], NULL);
        }
        g_tcm_call[line].list_length = 1;
        g_tcm_call[line].call_list[0] = pContainer->Content.n_Info;

        dect_call_request_offhook(line);
        g_EndptState = CMST_TALK;
    }
    else if (event == CMBS_EV_DEE_CALL_RELEASE)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_RELEASE lineid:%d\n",  pContainer->Content.n_Info);
        PST_CALL_OBJ p = NULL;
        //gNotifyCb(endpt, AC_EPEVT_ONHOOK);
        int line = pContainer->Content.n_Info;
        if (line >= TCM_CALL_MAX_NUM)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error: invalid lineid:%d \n", line);
            line = 0;
        }
        for (i = 0; i < g_tcm_call[line].list_length; i++)
        {
            if (*(u16*)(&pContainer->Content.ch_Info) == g_tcm_call[line].call_list[i])
            {
                continue;
            }
            ST_APPCALL_PROPERTIES st_Properties;
            st_Properties.e_IE  = CMBS_IE_CALLRELEASE_REASON;
            st_Properties.psz_Value = "";
            p = _appcall_CallObjGetById(g_tcm_call[line].call_list[i]);
            p->stopring = 1;

            appcall_ReleaseCall(&st_Properties, 1, g_tcm_call[line].call_list[i], NULL);
        }
        memset(&g_tcm_call[line], 0, sizeof(g_tcm_call[line]));
        dect_call_request_onhook(line);
        g_EndptState = CMST_ONHOOK;
    }
    else if (event == CMBS_EV_DEE_CALL_RELEASECOMPLETE)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_RELEASECOMPLETE lineid:%d\n",  pContainer->Content.n_Info);
        //gNotifyCb(endpt, AC_EPEVT_ONHOOK);
        int line = pContainer->Content.n_Info;
        if (line >= TCM_CALL_MAX_NUM)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error: invalid lineid:%d \n", line);
            line = 0;
        }
        memset(&g_tcm_call[line], 0, sizeof(g_tcm_call[line]));
        dect_call_request_onhook(line);
        g_EndptState = CMST_ONHOOK;
    }
    else if (event == CMBS_EV_DEE_CALL_HOLD)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_HOLD\n");

        PST_CALL_OBJ pstCall = _appcall_CallObjGetById(pContainer->Content.n_Info);
        PST_CALL_OBJ p = NULL;
        int line = pstCall->u8_LineId;
        int i = 0;

        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "pstCall->ch_CallerID:%s u8_LineId:%u\n", pstCall->ch_CalledID, line);

        if (line >= TCM_CALL_MAX_NUM)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error: invalid lineid:%d \n", line);
            line = 0;
        }

        //int handset = atoi(pstCall->ch_CalledID);
        for (i = 0; i < g_tcm_call[line].list_length; i++)
        {
            if (pContainer->Content.n_Info == g_tcm_call[line].call_list[i])
            {
                continue;
            }
            ST_APPCALL_PROPERTIES st_Properties;
            st_Properties.e_IE  = CMBS_IE_CALLRELEASE_REASON;
            st_Properties.psz_Value = "";
            p = _appcall_CallObjGetById(g_tcm_call[line].call_list[i]);
            p->stopring = 1;

            appcall_ReleaseCall(&st_Properties, 1, g_tcm_call[line].call_list[i], NULL);
        }
        g_tcm_call[line].list_length = 1;
        g_tcm_call[line].call_list[0] = pContainer->Content.n_Info;
        dect_call_request_flash(0);

    }
    else if (event == CMBS_EV_DEE_CALL_RESUME)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT CMBS_EV_DEE_CALL_RESUME\n");
        //gNotifyCb(endpt, AC_EPEVT_FLASH);
        dect_call_request_flash(0);
    }
    return 0;
}



int tcm_call_establish(int line, char *pCalernum, char *pCallerName)
{
    static char c_callerName[CC_MAXDSTR] = {0};
    static char c_callerNumber[CC_MAXDSTR] = {0};
    static char c_lineid[3] = {0};
    static char c_hs[16] = {0};
    static char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
    static char s_codecsOTA[2] = {0};
    char hs[32] = {"123456789A"};
    u32 hsmask = 0, a = 0;;

    int  n_Prop = 0;
    int ret = 0, i = 0, j = 0;
    ST_APPCALL_PROPERTIES st_Properties[6];
    //send CMBS_EV_DEE_CALL_ESTABLISH
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_establish, line [%d], callernum [%s], callername[%s] \n", line, pCalernum, pCallerName);
    //strncpy(c_callerName, call->remoteUser.disPlayName, CC_MAXDSTR - 1);
    //strncpy(c_callerNumber, call->remoteUser.userName, CC_MAXDSTR - 1);

    sprintf(c_lineid, "%d", line);

    /* all hadset ring */
    //strcpy(c_hs, "h123456789abc");
    //strcpy(c_hs, ALL_HS_STRING);
	strcpy(c_hs, "h1\0");


    for (i = 0; i < 6; i++)
        st_Properties[i].psz_Value = 0;


    //sprintf(s_codecs, "%d,%d", (), (CMBS_AUDIO_CODEC_PCM_LINEAR_WB));
    //sprintf(s_codecs, "%d,%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_WB), (CMBS_AUDIO_CODEC_PCMU));
   sprintf(s_codecs, "%d", CMBS_AUDIO_CODEC_PCM_LINEAR_WB);
   //sprintf(s_codecs, "%d", CMBS_AUDIO_CODEC_PCM_LINEAR_WB);
  // sprintf(s_codecs, "%d", CMBS_AUDIO_CODEC_PCMU);

    memset(c_callerName, 0, sizeof(c_callerName));
    memset(c_callerNumber, 0, sizeof(c_callerNumber));
    strncpy(c_callerName, pCallerName, sizeof(c_callerName) - 1);
    strncpy(c_callerNumber, pCalernum, sizeof(c_callerNumber) - 1);
	

    st_Properties[0].e_IE    = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = c_callerNumber;//pCalernum;
    st_Properties[1].e_IE    = CMBS_IE_CALLEDPARTY;
    //st_Properties[1].psz_Value = c_hs;
    st_Properties[2].e_IE    = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE    = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = c_callerName;//pCallerName,
    st_Properties[4].e_IE    = CMBS_IE_LINE_ID ;
    st_Properties[4].psz_Value = c_lineid;
    st_Properties[5].e_IE      = CMBS_IE_MELODY;
    st_Properties[5].psz_Value = "1\0";
    n_Prop = 6;

    memset(&g_tcm_call[line], 0, sizeof(g_tcm_call[line]));

    if (line == 0)
    {
        hsmask = VOIP_LINE1_HSMASK;
    }
    else
    {
        hsmask = VOIP_LINE2_HSMASK;
    }

    for (i = 0; i < CMBS_HS_SUBSCRIBED_MAX_NUM; i++)
    {
        a = hsmask >> i;
        if (a & 1)
        {
            sprintf(c_hs, "h%c", hs[i]);
            //sprintf(c_lineid, "%d", j);
            st_Properties[1].psz_Value = c_hs;
            //st_Properties[4].psz_Value = c_lineid;
            g_tcm_call[line].call_list[j] = appcall_EstablishCall(st_Properties, n_Prop);
            g_tcm_call[line].list_length++;
            if (g_tcm_call[line].call_list[j] < APPCALL_CALLOBJ_MAX)
            {
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_establish, callid:%d hs:%s\n", g_tcm_call[line].call_list[j], c_hs);
            }
            else
            {
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Error:tcm_call_establish, invalid callid:%d hs:%s\n", g_tcm_call[line].call_list[j], c_hs);
            }
            j++;
        }

    }
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_establish, call_list_length [%d] \n", g_tcm_call[line].list_length);
    return ret;
}

int tcm_call_release(int line, char *pType)
{
    int ret = 0;
    ST_APPCALL_PROPERTIES st_Properties;
    int i = 0;
    char s_reason[5] = {0};
    PST_CALL_OBJ pstCall = NULL;

    sprintf(s_reason, "%s", pType);

    // disconnecting call
    st_Properties.e_IE  = CMBS_IE_CALLRELEASE_REASON;
    st_Properties.psz_Value = s_reason;

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_release, line [%d], call_list_length [%d] \n", line, g_tcm_call[line].list_length);

    for (i = 0; i < g_tcm_call[line].list_length; i++)
    {
        appcall_ReleaseCall(&st_Properties, 1, g_tcm_call[line].call_list[i], NULL);
        /* more than one callid, only last need release reponse */
        if (i != g_tcm_call[line].list_length - 1)
        {
            pstCall = _appcall_CallObjGetById(g_tcm_call[line].call_list[i]);
            pstCall->stopring = 1;
        }
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_release, callid:%d \n", g_tcm_call[line].call_list[i]);
    }
    memset(& g_tcm_call[line], 0, sizeof(g_tcm_call[line]));
    return ret;
}
int tcm_call_update_media(int line, int band)
{
    int ret = 0;
	u16 u16_CallId = line;
	if (band == 1) {
	    appmedia_CallObjMediaOffer(g_tcm_call[line].call_list[0], 'u');
	    // wait for CMBS_EV_DEM_CHANNEL_OFFER_RES
	   // SleepMs(100);          

	    appmedia_CallObjMediaStart(0, g_tcm_call[line].call_list[0], 0xFF, NULL);
	} else if (band == 4) {
	    appmedia_CallObjMediaOffer(g_tcm_call[line].call_list[0], 'w');
	    // wait for CMBS_EV_DEM_CHANNEL_OFFER_RES
	    //SleepMs(100);          

	    appmedia_CallObjMediaStart(0, g_tcm_call[line].call_list[0], 0xFF, NULL);
	} else {
		ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_update_media, line:%d, band:%d, ERROR, can't start media!\n", line, band);
	}
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_update_media, line:%d, band:%d \n", line, band);
    return ret;
}
int tcm_call_answer(int line)
{
    int ret = 0;
    ST_APPCALL_PROPERTIES st_Properties;
    appcall_AnswerCall(&st_Properties, 0, g_tcm_call[line].call_list[0], NULL);
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_answer, line:%d, callid:%d \n", line, g_tcm_call[line].call_list[0]);
    return ret;
}

int tcm_call_progress(int line, char * pType)
{
    int ret = 0;
    int rc = 0;
    ST_APPCALL_PROPERTIES st_Properties;
    st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
    //st_Properties.psz_Value = "CMBS_CALL_PROGR_PROCEEDING\0";
    if (0 == strcmp(pType, "dial"))
    {
        st_Properties.psz_Value = "CMBS_CALL_PROGR_RINGING\0";
    }
    else if (0 == strcmp(pType, "busy"))
    {
        st_Properties.psz_Value = "CMBS_CALL_PROGR_BUSY\0";
    }
    else
    {
        st_Properties.psz_Value = "CMBS_CALL_PROGR_RINGING\0";
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "tcm_call_progress, line:%d, %s \n", line, st_Properties.psz_Value);

    rc = appcall_ProgressCall(&st_Properties, 1, g_tcm_call[line].call_list[0], NULL);
    return ret;
}
int tcm_channel_start(int line)
{
    int ret = 0;

    return ret;
}
