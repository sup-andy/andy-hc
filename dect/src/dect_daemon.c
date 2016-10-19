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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>
#include <fcntl.h>
#include <util_mq.h>
#include <signal.h>
#include <ctype.h>
#include "dect_daemon.h"
//#include "dect_message.h"
#include "tcm_cmbs.h"
#include "dect_device.h"
#include "dect_voice.h"
#include "dect_util.h"
#include "dect_task.h"
#include "dect_msg.h"
#include "dect_api.h"

/* flag for tunnel loop*/
int keep_looping = 1;


static void dect_handset_list(unsigned int key)
{
#if 0
    DECT_FIFO_MSG msg;
    memset(&msg, 0, sizeof(msg));
    int ret = 0;
    unsigned i = 1, j = 1;
    char str[32] = {0};
    char num[32] = {0};
    strcpy(str, "h");
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dect_handset_list");

    ret = tcm_handset_list();

    /* send response */
    msg.type = FIFO_TYPE_ULE_RESPONSE;
    msg.msg_body.ule_opr_info.msg_index = key;

    if (ret == 0) {
        for (i = 1; i <= 0x0800; i = i << 1) {

            if ((i & g_reg_hs) != 0) {
                //sprintf(str, "%s%d", str, j);
                memset(num , 0, sizeof(num));
                sprintf(num, "%x", j);
                strncat(str, num, 1);
            }
            j++;
        }
        strcpy(msg.msg_body.ule_opr_info.msg, str);
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "handset list g_reg_hs = 0x%x, str = %s\n", g_reg_hs, str);
    } else if (ret == 1) {
        strcpy(msg.msg_body.ule_opr_info.msg, "operate failure");
    } else {
        strcpy(msg.msg_body.ule_opr_info.msg, "operate invalid");
    }

    send_fifo_message_to_mgr(&msg);
#endif
}




static void dect_show_status(int key)
{
#if 0
    /* get devices data */

    DECT_FIFO_MSG msg;
    DECT_CMBS_INFO info;

    memset(&info, 0, sizeof(info));
    memset(&msg, 0, sizeof(msg));
    tcm_get_cmbs_info(&info);


    generate_cmbs_status_json(&info, msg.msg_body.ule_opr_info.msg, sizeof(msg.msg_body.ule_opr_info.msg));
    /* send response */
    msg.type = FIFO_TYPE_ULE_RESPONSE;
    msg.msg_body.ule_opr_info.msg_index = key;
    send_fifo_message_to_mgr(&msg);

    dect_add_log("showStatus", NULL, 1);
#endif
}




static void dect_refresh_device(int key, unsigned int id)
{
#if 0
    /* get devices data */
    char cid[32];
    DECT_FIFO_MSG msg;

    memset(&msg, 0, sizeof(msg));

    sprintf(cid, "%d", id);

    /* send response */
    msg.type = FIFO_TYPE_ULE_RESPONSE;
    msg.msg_body.ule_opr_info.msg_index = key;

    strcpy(msg.msg_body.ule_opr_info.msg, "success");
    send_fifo_message_to_mgr(&msg);

    dect_add_log("showStatus", cid, 1);
#endif
}

int main(int argc, char *argv[])
{

    int ret;

    ctrl_log_init(DECT_CTRL_NAME);

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "init cmbs.\n");
    if (DECT_STATUS_OK != dect_daemon_init())
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "cmbs initailize fail!!!,dect_daemon exit...\n");
        return 0;
    }

    //han_mgr_start();
    //GetDeviceList();

    //tcm_HanRegisterInterface();
    /* init han manger and register interface for ule device */
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "init ule mgr.\n");
    dect_ule_device_mgr_init();

    // initialize task handle thread.
    ret = task_init();
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "task_init failed.\n");
        return -1;
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "init msg.\n");
    ret = msg_init();
    if (ret != 0)
    {
        task_uninit();
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "msg_init failed.\n");
        return -1;
    }

    //tcm_createDeviceReportThread();

    //tcm_createDeviceKeepAliveThread();

    //dect_device_data_init();
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "init call line.\n");
    dect_callline_init();

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "init thread.\n");
    if (dect_thread_int() != DECT_STATUS_OK)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "dect_thread_int failed.\n");
        goto EXIT;
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "main loop.\n");
    msg_handle();

    ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "dect_daemon is going to exit\n");


EXIT:
    /* process exit. */
    task_uninit();
    msg_uninit();

    return 0;
}




