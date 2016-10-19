/**********************************************************************
 *    Copyright 2015 WondaLink CO., LTD.
 *    All Rights Reserved. This material can not be duplicated for any
 *    profit-driven enterprise. No portions of this material can be reproduced
 *    in any form without the prior written permission of WondaLink CO., LTD.
 *    Forwarding, transmitting or communicating its contents of this document is
 *    also prohibited.
 *
 *    All titles, proprietaries, trade secrets and copyrights in and related to
 *    information contained in this document are owned by WondaLink CO., LTD.
 *
 *    WondaLink CO., LTD.
 *    23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *    HSIN-CHU, TAIWAN R.O.C.
 *
 ******************************************************************************/
/******************************************************************************
 *    Department:
 *    Project :
 *    Block   :
 *    Creator :
 *    File    :
 *    Abstract:
 *    Date    :
 *    $Id:$
 *
 *    Modification History:
 *    By           Date       Ver.   Modification Description
 *    -----------  ---------- -----  -----------------------------
 ******************************************************************************/
 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "ctrl_common_lib.h"
#include "tcm_call.h"
#include "dect_device.h"
#include "dect_msg.h"

#define DECT_CALL_MAX_NUM 2
/* ---- Static Variables ------------------------------------------------- */


#define DECT_DATA_TITLE_ACTION "action"
#define DECT_DATA_TITLE_ID "id"
#define DECT_DATA_TITLE_TYPE "type"
#define DECT_DATA_TITLE_VALUE "value"
#define DECT_DATA_TITLE_CALLER "caller"
#define DECT_DATA_TITLE_BAND "band"

#define DECT_SUCCESS "success"
#define DECT_FAIL "fail"
#define DECT_VOICE_REPONSE_FORMAT "action=%s&value=%s"
#define DECT_VOICE_REQUEST_FORMAT "action=%s&lineid=%d"

typedef enum
{
    EP_ACTION_NULL,                        /* internal use: Null event */
    EP_ACTION_PLAYTONE,
    EP_ACTION_STOPTONE,
    EP_ACTION_OPENCHANNEL,
    EP_ACTION_STOPCHANNEL,
    EP_ACTION_STARTRING,
    EP_ACTION_DECTBAND,
    EP_ACTION_STOPRING,
    EP_ACTION_RELEASE,
    EP_ACTION_PROGRESS,
    EP_ACTION_ANSWER,
    EP_ACTION_LAST
} EP_ACTION;


typedef struct dect_endpoint_msg_t {
    EP_ACTION type;
    int lineid;
    int band;
    union
    {
        unsigned int id;
        char type[32];
        char caller[32];
    } msg_body;
} DECT_ENDPOINT_MSG;

static void dect_send_reponse(char *pMsg);
static void dect_send_request(char *pMsg);
static int dect_parse_mesage(char *pMsg, DECT_ENDPOINT_MSG *pVoip_msg);


//example:IN action=list&id=1111&type=switch&value=0&name=""
void dect_call_request_offhook(int lineid)
{
    char buf[256];

    snprintf(buf, sizeof(buf), DECT_VOICE_REQUEST_FORMAT, "offhook", lineid);
    dect_send_request(buf);
}

void dect_call_request_dial(int digit, int lineid)
{
    char buf[256];

    snprintf(buf, sizeof(buf), DECT_VOICE_REQUEST_FORMAT"&value=%d", "dial", lineid, digit);
    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "dial [%d]\n", digit);
    dect_send_request(buf);
}

void dect_call_request_onhook(int lineid)
{
    char buf[256];

    snprintf(buf, sizeof(buf), DECT_VOICE_REQUEST_FORMAT, "onhook", lineid);
    dect_send_request(buf);
}

void dect_call_request_flash(int lineid)
{
    char buf[256];

    snprintf(buf, sizeof(buf), DECT_VOICE_REQUEST_FORMAT, "flash", lineid);
    dect_send_request(buf);
}


void dect_Handle_voip_Message(char *pMsg)
{
    DECT_ENDPOINT_MSG msg;
    int ret = 0;
    char buf[256];

    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "rcv voip meeage [%s]\n", pMsg);

    memset(&msg, 0, sizeof(msg));
    dect_parse_mesage(pMsg, &msg);

    switch (msg.type)
    {
        case EP_ACTION_PLAYTONE:
        {
            ret = tcm_call_progress(msg.lineid, msg.msg_body.type);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "playtoneres", ret ? DECT_FAIL : DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_STOPTONE:
        {
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "stoptoneres", ret ? DECT_FAIL : DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_OPENCHANNEL:
        {
            ret = tcm_call_answer(msg.lineid);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "openchannelres", ret ? DECT_FAIL : DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_STARTRING:
        {
            ret = tcm_call_establish(msg.lineid, msg.msg_body.caller, msg.msg_body.caller);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "startringres", ret ? DECT_FAIL : DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_DECTBAND:
        {
            ret = tcm_call_update_media(msg.lineid, msg.band);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "dectbandres", ret ? DECT_FAIL : DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }

        case EP_ACTION_STOPRING:
        {
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "stopringres", DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_RELEASE:
        {
            tcm_call_release(msg.lineid, msg.msg_body.type);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "releaseres", DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_PROGRESS:
        {
            tcm_call_progress(msg.lineid, msg.msg_body.type);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "progressres", DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        case EP_ACTION_ANSWER:
        {
            ret = tcm_call_answer(msg.lineid);
            snprintf(buf, sizeof(buf), DECT_VOICE_REPONSE_FORMAT, "answerres", ret ? DECT_FAIL : DECT_SUCCESS);
            dect_send_reponse(buf);
            break;
        }
        default:
            break;
    }
}

static void dect_send_reponse(char *pMsg)
{
    DECT_HAN_DEVICE_INF devinfo;
    memset(&devinfo, 0, sizeof(devinfo));

    devinfo.type = DECT_TYPE_VOICE;
    strncpy(devinfo.device.voice.action, pMsg, sizeof(devinfo.device.voice.action) - 1);
    //msg_DECTReply(g_dect_voip_appid, HC_EVENT_RESP_VOIP_CALL, &devinfo);
    msg_DECTVoiceReply(&devinfo);
    /*    DECT_FIFO_MSG msg;

        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "FIFO_TYPE_ULE_VOIP_RESPONSE [%s]\n", pMsg);

        memset(&msg, 0, sizeof(msg));
        msg.type = FIFO_TYPE_ULE_VOIP_RESPONSE;

        strncpy(msg.msg_body.voip_info.msg, pMsg, sizeof(msg.msg_body.voip_info.msg));

        if (send_fifo_message_to_mgr(&msg) <= 0)
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "send_fifo_message_to_mgrssage failed, [%s]\n", pMsg);
        }*/
}

static void dect_send_request(char *pMsg)
{
    DECT_HAN_DEVICE_INF devinfo;
    memset(&devinfo, 0, sizeof(devinfo));

    devinfo.type = DECT_TYPE_VOICE;
    strncpy(devinfo.device.voice.action, pMsg, sizeof(devinfo.device.voice.action) - 1);
    //msg_DECTReply(g_dect_voip_appid, HC_EVENT_REQ_VOIP_CALL, &devinfo);
    msg_DECTVoiceReq(&devinfo);
}

static int dect_parse_mesage(char *pMsg, DECT_ENDPOINT_MSG *pVoip_msg)
{
    char *p = NULL;

    char action[32] = {0};
    char id[32] = {0};

    char value[32] = {0};
    char type[32] = {0};
    char caller[32] = {0};
    char band[32] = {0};

    if (NULL == pMsg || NULL == pVoip_msg)
    {
        return -1;
    }

    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "message:%s \n", pMsg);

    p = strtok(pMsg, "&");
    while (p)
    {
        /* parse action */
        if (0 == strncasecmp(p, DECT_DATA_TITLE_ACTION, strlen(DECT_DATA_TITLE_ACTION)))
        {
            memset(action, 0, sizeof(action));
            strncpy(action, p + strlen(DECT_DATA_TITLE_ACTION) + 1, sizeof(action) - 1);
            if (0 == strncasecmp(action, "playtone", strlen("playtone")))
            {
                pVoip_msg->type = EP_ACTION_PLAYTONE;
            }
            else if (0 == strncasecmp(action, "stoptone", strlen("stoptone")))
            {
                pVoip_msg->type = EP_ACTION_STOPTONE;
            }
            else if (0 == strncasecmp(action, "openchannel", strlen("openchannel")))
            {
                pVoip_msg->type = EP_ACTION_OPENCHANNEL;
            }
            else if (0 == strncasecmp(action, "startring", strlen("startring")))
            {
                pVoip_msg->type = EP_ACTION_STARTRING;
            }

            else if (0 == strncasecmp(action, "dectband", strlen("dectband")))
            {
                pVoip_msg->type = EP_ACTION_DECTBAND;
            }
            else if (0 == strncasecmp(action, "stopring", strlen("stopring")))
            {
                pVoip_msg->type = EP_ACTION_STOPRING;
            }
            else if (0 == strncasecmp(action, "release", strlen("release")))
            {
                pVoip_msg->type = EP_ACTION_RELEASE;
            }
            else if (0 == strncasecmp(action, "progress", strlen("progress")))
            {
                pVoip_msg->type = EP_ACTION_PROGRESS;
            }
            else if (0 == strncasecmp(action, "answer", strlen("answer")))
            {
                pVoip_msg->type = EP_ACTION_ANSWER;
            }
            else
            {
                printf("fun:%s line:%d Invalid action [%s]...\n", __FUNCTION__, __LINE__, action);
                return 1;
            }
        }
        else if (0 == strncasecmp(p, DECT_DATA_TITLE_ID, strlen(DECT_DATA_TITLE_ID))) /* parse id */
        {
            /* parse id */
            memset(id, 0, sizeof(id));
            strncpy(id, p + strlen(DECT_DATA_TITLE_ID) + 1, sizeof(id) - 1);
        }
        else if (0 == strncasecmp(p, DECT_DATA_TITLE_VALUE, strlen(DECT_DATA_TITLE_VALUE)))
        {
            /* parse value */
            memset(value, 0, sizeof(value));
            strncpy(value, p + strlen(DECT_DATA_TITLE_VALUE) + 1, sizeof(value) - 1);
            pVoip_msg->lineid = atoi(value);
        }
        else if (0 == strncasecmp(p, DECT_DATA_TITLE_TYPE, strlen(DECT_DATA_TITLE_TYPE)))
        {
            /* parse type */
            memset(type, 0, sizeof(type));
            strncpy(type, p + strlen(DECT_DATA_TITLE_TYPE) + 1, sizeof(type) - 1);
        }
        else if (0 == strncasecmp(p, DECT_DATA_TITLE_CALLER, strlen(DECT_DATA_TITLE_CALLER)))
        {
            /* parse caller */
            memset(caller, 0, sizeof(caller));
            strncpy(caller, p + strlen(DECT_DATA_TITLE_CALLER) + 1, sizeof(caller) - 1);
        }
        else if (0 == strncasecmp(p, DECT_DATA_TITLE_BAND, strlen(DECT_DATA_TITLE_BAND)))
        {
            /* parse band */
            memset(band, 0, sizeof(band));
            strncpy(band, p + strlen(DECT_DATA_TITLE_BAND) + 1, sizeof(band) - 1);
            pVoip_msg->band = atoi(band);
        }
        p = strtok(NULL, "&");
    }

    switch (pVoip_msg->type)
    {
        case EP_ACTION_PLAYTONE:
        case EP_ACTION_PROGRESS:
        case EP_ACTION_RELEASE:
        {
            if (strlen(type))
            {
                memset(pVoip_msg->msg_body.type, 0, sizeof(pVoip_msg->msg_body.type));
                strncpy(pVoip_msg->msg_body.type, type, sizeof(pVoip_msg->msg_body.type) - 1);
                ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "action:%d type = %s\n",  pVoip_msg->type, pVoip_msg->msg_body.type);
            }
            break;
        }
        case EP_ACTION_STARTRING:
        {
            memset(pVoip_msg->msg_body.caller, 0, sizeof(pVoip_msg->msg_body.caller));
            strncpy(pVoip_msg->msg_body.caller, caller, sizeof(pVoip_msg->msg_body.caller) - 1);
            ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "start ring, caller = %s\n", pVoip_msg->msg_body.caller);
            break;
        }

        default:
            break;
    }


    return 0;
}

void dect_voice_init()
{
    int i = 0;
    for (i = 0; i < DECT_CALL_MAX_NUM; i++)
    {
        dect_call_request_onhook(i);
    }
}
