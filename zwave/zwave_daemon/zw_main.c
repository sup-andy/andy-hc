/*******************************************************************************
 *   Copyright 2015 WondaLink CO., LTD.
 *   All Rights Reserved. This material can not be duplicated for any
 *   profit-driven enterprise. No portions of this material can be reproduced
 *   in any form without the prior written permission of WondaLink CO., LTD.
 *   Forwarding, transmitting or communicating its contents of this document is
 *   also prohibited.
 *
 *   All titles, proprietaries, trade secrets and copyrights in and related to
 *   information contained in this document are owned by WondaLink CO., LTD.
 *
 *   WondaLink CO., LTD.
 *   23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *   HSIN-CHU, TAIWAN R.O.C.
 *
 ******************************************************************************/
/******************************************************************************
*   Department:
*   Project :
*   Block   :
*   Creator : Mark Yan
*   File   : zw_main.c
*   Abstract:
*   Date   : 12/18/2014
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Rose  06/03/2013 1.0  Initial Dev.
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_common.h"
#include "zwave_api.h"

#include "zw_task.h"
#include "zw_msg.h"
#include "zw_serial.h"
#include "zw_util.h"
#include "zw_queue.h"


/* gloabal variable. */
int keep_looping = 1;
char g_image_file_path[256] = "/tmp/zwave.bin";

static void signal_handle(int signal)
{
    keep_looping = 0;
}

static int device_db_sync(ZW_NODES_INFO *nodeinfo)
{
    int ret = 0;
    int dev_num = 0;
    int i = 0, j = 0;
    HC_DEVICE_INFO_S *devs = NULL;

    ret = db_get_dev_num_by_network_type(HC_NETWORK_TYPE_ZWAVE, &dev_num);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Get ZW devnum from DB failed, ret=%d.\n", ret);
        return -1;
    }

    devs = calloc(dev_num, sizeof(HC_DEVICE_INFO_S));
    if (devs == NULL)
    {
        DEBUG_ERROR("Allocate memory failed.\n");
        return -1;
    }

    ret = db_get_dev_by_network_type(HC_NETWORK_TYPE_ZWAVE, devs, dev_num);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Get ZW devs from DB failed, ret=%d.\n", ret);
        free(devs);
        return -1;
    }

    for (i = 0; i < dev_num; i++)
    {
        for (j = 0; j < nodeinfo->nodes_num; j++)
        {
            if (devs[i].dev_id == nodeinfo->nodes[j].id)
                break;
        }

        /* DB device can't find in ZW Controller, delete it. */
        if (j >= nodeinfo->nodes_num)
        {
            db_del_dev_by_dev_id(devs[i].dev_id);
        }
    }

    free(devs);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    struct sigaction newAction;
    pthread_t pid_task;
    pthread_t pid_serial;
    pthread_t pid_detect;

#ifdef CTRL_LOG_DEBUG
    ctrl_log_init(HC_CTRL_NAME);
#endif

    // initialize serial, ttyS1
    ret = serial_init();
    if (ret == -1)
    {
        DEBUG_ERROR("Initialize serial(tty) failed, ret = %d\n", ret);
        return -1;
    }

    // initialize task handle thread.
    ret = task_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Initialize internal task failed, ret = %d\n", ret);
        serial_uninit();
        return -1;
    }

    // initialize msg handle, comunicate with dispatch.
    ret = msg_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Initialize msg library failed, ret = %d\n", ret);
        task_uninit();
        serial_uninit();
        return -1;
    }

    /* Ignore broken pipes */
    signal(SIGPIPE, SIG_IGN);

    /* catch SIGTERM and SIGINT so we can properly clean up */
    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = signal_handle;
    newAction.sa_flags = SA_RESETHAND;
    ret = sigaction(SIGINT, &newAction, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to install SIGINT handler, ret = %d\n", ret);
        goto EXIT;
    }

    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = signal_handle;
    newAction.sa_flags = SA_RESETHAND;
    ret = sigaction(SIGTERM, &newAction, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to install SIGTERM handler.\n");
        goto EXIT;
    }

    // startup serial thread.
    ret = start_thread(&pid_serial, 0, serial_handle_thread, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread serial_handle_thread failed.\n");
        goto EXIT2;
    }

    // startup task thread.
    ret = start_thread(&pid_task, 0, task_handle_thread, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread task_handle_thread failed.\n");
        goto EXIT2;
    }

    // initialize MID level device info.
    ret = mid_device_init_from_db();
    if (ret != 0)
    {
        DEBUG_ERROR("Initialize MID level device info failed.\n");
        goto EXIT2;
    }

    // startup detect thread.
    ret = start_thread(&pid_detect, 0, detect_handle_thread, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread detect_handle_thread failed.\n");
        goto EXIT2;
    }

    //device_db_sync(&nodeinfo);

    // waiting for thread startup
    sleep(3);

    // initialize zwave protocol
    ret = ZWapi_Init(msg_ZWReportCB, msg_ZWEventCB, NULL);
    if (ret != ZW_STATUS_OK)
    {
        DEBUG_ERROR("Initialize zwave protocol failed, ret is %d.\n", ret);
        goto EXIT2;
    }

    DEBUG_INFO("Going to msg main loop.\n");

    // man loop.
    msg_handle();

EXIT2:
    keep_looping = 0;

    // stop thread.
    stop_thread(pid_detect);
    stop_thread(pid_task);
    stop_thread(pid_serial);

EXIT:
    /* process exit. */
    ZWapi_Uninit();
    msg_uninit();
    task_uninit();
    serial_uninit();

    return 0;
}
