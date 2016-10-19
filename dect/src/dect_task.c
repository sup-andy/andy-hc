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
*   Creator : Harvey Hua
*   File   : dect_task.c
*   Abstract:
*   Date   : 03/10/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "ctrl_common_lib.h"
#include "dect_device.h"

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"

#include "dect_task.h"
#include "dect_queue.h"
#include "dect_msg.h"
#include "dect_api.h"


extern int keep_looping;

int g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;
int g_dect_add_delete_appid = -1;
int g_fw_upgrade_flag = 0;

static pthread_cond_t task_cond;
static pthread_mutex_t task_mutex;


#define MAX_GET_DEV_RETRY_NUM   10

#if 0
int compare_zwdev(DECT_HAN_DEVICE_INF *dest, DECT_HAN_DEVICE_INF *src)
{
    if (dest == NULL || src == NULL)
    {
        return -1;
    }

    switch (src->type)
    {
            //case ZW_DEVICE_PORTABLE_CONTROLLER:
            //    break;
            //case ZW_DEVICE_STATIC_CONTROLLER:
            //    break;
        case ZW_DEVICE_BINARYSWITCH:
            if (dest->status.binaryswitch_status.value != src->status.binaryswitch_status.value)
                return -1;
            break;
        case ZW_DEVICE_DOUBLESWITCH:
            if (dest->status.binaryswitch_status.value != src->status.binaryswitch_status.value)
                return -1;
            break;
        case ZW_DEVICE_DIMMER:
            printf("%d - %d\n", dest->status.dimmer_status.value, src->status.dimmer_status.value);
            if (dest->status.dimmer_status.value != src->status.dimmer_status.value)
                return -1;
            break;
        case ZW_DEVICE_CURTAIN:
            if (dest->status.curtain_status.value != src->status.curtain_status.value)
                return -1;
            break;
        case ZW_DEVICE_BINARYSENSOR:
            if (dest->status.binarysensor_status.value != src->status.binarysensor_status.value)
                return -1;
            break;
        case ZW_DEVICE_MULTILEVEL_SENSOR:
            DEBUG_ERROR("%s: ZW_DEVICE_MULTILEVEL_SENSOR, can't come here\n", __FUNCTION__);
            break;
        case ZW_DEVICE_MOTIONSENSOR:
            if (dest->status.motionsensor_status.value != src->status.motionsensor_status.value)
                return -1;
            break;
        case ZW_DEVICE_WINDOWSENSOR:
            if (dest->status.windowsensor_status.value != src->status.windowsensor_status.value)
                return -1;
            break;
        case ZW_DEVICE_TEMPERATURE:
            if (dest->status.temperaturesensor_status.value != src->status.temperaturesensor_status.value)
                return -1;
            break;
        case ZW_DEVICE_LUMINANCE:
            if (dest->status.luminancesensor_status.value != src->status.luminancesensor_status.value)
                return -1;
            break;
        case ZW_DEVICE_BATTERY:
            if (dest->status.battery_status.battery_level != src->status.battery_status.battery_level)
                return -1;
            if (dest->status.battery_status.interval_time != src->status.battery_status.interval_time)
                return -1;
            break;
        case ZW_DEVICE_DOORLOCK:
            // TODO:
            DEBUG_ERROR("%s: ZW_DEVICE_DOORLOCK, not implement\n", __FUNCTION__);
            break;
        case ZW_DEVICE_THERMOSTAT:
            if (dest->status.thermostat_status.mode != src->status.thermostat_status.mode)
                return -1;
            if (dest->status.thermostat_status.value != src->status.thermostat_status.value)
                return -1;
            break;
        case ZW_DEVICE_METER:
            DEBUG_ERROR("%s: ZW_DEVICE_METER, can't come here\n", __FUNCTION__);
            break;
        case ZW_DEVICE_ELECTRIC_METER:
            if (dest->status.meter_status.power_meter_status.power != src->status.meter_status.power_meter_status.power)
                return -1;
            if (dest->status.meter_status.power_meter_status.voltage != src->status.meter_status.power_meter_status.voltage)
                return -1;
            if (dest->status.meter_status.power_meter_status.current != src->status.meter_status.power_meter_status.current)
                return -1;
            if (dest->status.meter_status.power_meter_status.delta_time != src->status.meter_status.power_meter_status.delta_time)
                return -1;
            if (dest->status.meter_status.power_meter_status.previous_power != src->status.meter_status.power_meter_status.previous_power)
                return -1;
            if (dest->status.meter_status.power_meter_status.previous_voltage != src->status.meter_status.power_meter_status.previous_voltage)
                return -1;
            if (dest->status.meter_status.power_meter_status.previous_current != src->status.meter_status.power_meter_status.previous_current)
                return -1;
            break;
        case ZW_DEVICE_GAS_METER:
            if (dest->status.meter_status.gas_meter_status.value != src->status.meter_status.gas_meter_status.value)
                return -1;
            if (dest->status.meter_status.gas_meter_status.delta_time != src->status.meter_status.gas_meter_status.delta_time)
                return -1;
            if (dest->status.meter_status.gas_meter_status.previous_value != src->status.meter_status.gas_meter_status.previous_value)
                return -1;
            break;
        case ZW_DEVICE_WATER_METER:
            if (dest->status.meter_status.water_meter_status.value != src->status.meter_status.water_meter_status.value)
                return -1;
            if (dest->status.meter_status.water_meter_status.delta_time != src->status.meter_status.water_meter_status.delta_time)
                return -1;
            if (dest->status.meter_status.water_meter_status.previous_value != src->status.meter_status.water_meter_status.previous_value)
                return -1;
            break;
        default:
            DEBUG_ERROR("%s: Unsupported ZW Device type %d.\n", __FUNCTION__, src->type);
            return -1;
    }

    return 0;
}
#endif

static int process_task(QUEUE_UNIT_S *task)
{
    DECT_STATUS ret = 0;
    int retry_count = 0;
    DECT_HAN_DEVICE_INF uledev;

    //DEBUG_INFO("Process task ...\n");

    if (task == NULL)
        return -1;

    if (g_dect_controller_mode == DECT_CONTROLLER_MODE_ADDING)
    {
        msg_DECTReply(task->appid, HC_EVENT_RESP_DEVICE_ADD_MODE, NULL);
        return 0;
    }
    else if (g_dect_controller_mode == DECT_CONTROLLER_MODE_DELETING)
    {
        msg_DECTReply(task->appid, HC_EVENT_RESP_DEVICE_DELETE_MODE, NULL);
        return 0;
    }
    else if (g_dect_controller_mode == DECT_CONTROLLER_MODE_UPGRADING)
    {
        //msg_DECTReply(task->appid, HC_EVENT_RESP_DECT_UPGRADE, NULL);
    }

    memset(&uledev, 0, sizeof(DECT_HAN_DEVICE_INF));

    switch (task->msgtype)
    {
        case HC_EVENT_REQ_DECT_PARAMETER_SET:
        {
            int reboot = 0;
            switch (task->devinfo.device.station.type)
            {
                case HC_DECT_PARAMETER_TYPE_DECTTYPE:
                {
                    ret = dect_set_dect_type(task->devinfo.device.station.u.dect_type);
                    if (ret != DECT_STATUS_OK)
                    {
                        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "dect_set_dect_type failed.\n");
                    }
                    else
                    {
                        reboot = 1;
                        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dect_set_dect_type success.\n");
                    }
                    break;
                }
                case HC_DECT_PARAMETER_TYPE_BASENAME:
                case HC_DECT_PARAMETER_TYPE_PINCODE:
                default:
                {
                    ret = DECT_STATUS_FAIL;
                    ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported dect parameter type %d.\n", task->devinfo.device.station.type);
                    break;
                }
            }
            if (ret != DECT_STATUS_OK)
            {
                msg_DECTReply(task->appid, HC_EVENT_RESP_DECT_PARAMETER_SET_FAILURE, &task->devinfo);
            }
            else
            {
                msg_DECTReply(task->appid, HC_EVENT_RESP_DECT_PARAMETER_SET_SUCCESS, &task->devinfo);
            }
            if (reboot)
            {
                dect_system_reboot();
            }
            break;
        }
        case HC_EVENT_REQ_DECT_PARAMETER_GET:
        {
            switch (task->devinfo.device.station.type)
            {
                case HC_DECT_PARAMETER_TYPE_DECTTYPE:
                {
                    ret = dect_get_dect_type(&task->devinfo.device.station.u.dect_type);
                    if (ret != DECT_STATUS_OK)
                    {
                        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "dect_get_dect_type failed.\n");
                    }
                    else
                    {
                        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dect_get_dect_type success.\n");
                    }
                    break;
                }
                case HC_DECT_PARAMETER_TYPE_BASENAME:
                case HC_DECT_PARAMETER_TYPE_PINCODE:
                default:
                {
                    ret = DECT_STATUS_FAIL;
                    ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported dect parameter type %d.\n", task->devinfo.device.station.type);
                    break;
                }
            }
            if (ret != DECT_STATUS_OK)
            {
                msg_DECTReply(task->appid, HC_EVENT_RESP_DECT_PARAMETER_GET_FAILURE, &task->devinfo);
            }
            else
            {
                msg_DECTReply(task->appid, HC_EVENT_RESP_DECT_PARAMETER_GET_SUCCESS, &task->devinfo);
            }
            break;
        }
        case HC_EVENT_REQ_DEVICE_ADD:
        {
            g_dect_controller_mode = DECT_CONTROLLER_MODE_ADDING;
            g_dect_add_delete_appid = task->appid;
            ret = dect_device_add(task->timeout);
            if (ret != 0)
            {
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Call tcm_add_device failed, ret=%d.\n", ret);
                msg_DECTAddFail(0);
                return -1;
            }
            msg_DECTReply(task->appid, HC_EVENT_RESP_DEVICE_ADD_MODE, NULL);
            break;
        }

        case HC_EVENT_REQ_DEVICE_DELETE:
        {
            g_dect_controller_mode = DECT_CONTROLLER_MODE_DELETING;
            g_dect_add_delete_appid = task->appid;
            ret = dect_device_delete(task->devinfo.id);
            if (ret != DECT_STATUS_OK)
            {
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Delete device [%d] failed.\n", task->devinfo.id);
                msg_DECTRemoveFail(0);
            }
            else
            {
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Delete device [%d] success.\n", task->devinfo.id);
                msg_DECTRemoveSuccess(task->devinfo.id);
            }
            g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;

            break;
        }
        case HC_EVENT_REQ_DEVICE_SET:
        {
            ret = dect_device_set(&task->devinfo);
            if (ret != DECT_STATUS_OK)
            {
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "set device [%d] failed.\n", task->devinfo.id);
                msg_DECTReply(task->appid, HC_EVENT_RESP_DEVICE_SET_FAILURE, &task->devinfo);
            }
            else
            {
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "set device [%d] success.\n", task->devinfo.id);
                msg_DECTReply(task->appid, HC_EVENT_RESP_DEVICE_SET_SUCCESS, &task->devinfo);
            }

            break;
        }
#if 0
        case HC_EVENT_REQ_DEVICE_GET:
            ret = ZWapi_GetDeviceStatus(&task->devinfo);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d.\n", ret);
                msg_ZWReply(HC_EVENT_DEVICE_GET_FAILURE, &task->devinfo);
                return -1;
            }
            msg_ZWReply(HC_EVENT_DEVICE_GET_SUCCESS, &task->devinfo);
            break;
            //case HC_EVENT_DEVICE_RESET:
            //    break;

        case HC_EVENT_REQ_DEVICE_DEFAULT:
            ret = ZWapi_SetDefault();
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_INFO("Set default failure, ret is %d.\n", ret);
                msg_ZWReply(HC_EVENT_DEVICE_DEFAULT_FAILURE, NULL);
            }
            else
            {
                DEBUG_INFO("Set default success.\n");
                msg_ZWReply(HC_EVENT_DEVICE_DEFAULT_SUCCESS, NULL);
            }
            break;
        case HC_EVENT_REQ_DEVICE_DETECT:
            ret = ZWapi_isFailedNode(&task->devinfo);
            // NODE does not exist in network
            if (ret == ZW_STATUS_IS_FAIL_NODE)
            {
                DEBUG_INFO("Detect device is N/A.\n");
                msg_ZWReply(HC_EVENT_DEVICE_DETECT_NOT_AVAILABLE, &task->devinfo);
            }
            // NODE in network
            else if (ret == ZW_STATUS_NOT_FAIL_NODE)
            {
                DEBUG_INFO("Detect device is OK.\n");
                msg_ZWReply(HC_EVENT_DEVICE_DETECT_AVAILABLE, &task->devinfo);
            }
            else
            {
                DEBUG_INFO("Detect device failed, ret is %d.\n", ret);
                msg_ZWReply(HC_EVENT_DEVICE_DETECT_FAILURE, &task->devinfo);
                return -1;
            }

            break;

        case HC_EVENT_DEVICE_NA_DELETE:
            ret = ZWapi_RemoveFailedNodeID(&task->devinfo);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_INFO("Remove N/A device failure, ret is %d.\n", ret);
                msg_ZWReply(HC_EVENT_DEVICE_NA_DELETE_FAILURE, &task->devinfo);
            }
            else
            {
                DEBUG_INFO("Remove N/A device success.\n");
                msg_ZWReply(HC_EVENT_DEVICE_NA_DELETE_SUCCESS, &task->devinfo);
            }
            break;
#endif
        case HC_EVENT_REQ_VOIP_CALL:
        {
            dect_handle_voip_call(&task->devinfo);
            break;
        }
        case HC_EVENT_REQ_DECT_UPGRADE:  /* dect upgrade */
        {
            g_dect_controller_mode = DECT_CONTROLLER_MODE_UPGRADING;
#if 0
            memset(&fifo_msg, 0, sizeof(DECT_FIFO_MSG));
            if (CMBS_RC_OK == dect_handle_upgrade(p_fifo_msg->msg_body.fw_file_path))
            {
                ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE,  "firmware upgrade success.");
                fifo_msg.msg_body.fw_upgrade_res = DECT_UPGRADE_SUCCESS;
            }
            else
            {
                ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "firmware upgrade failed.");
                fifo_msg.msg_body.fw_upgrade_res = DECT_UPGRADE_FAILURE;
            }
            fifo_msg.type = FIFO_TYPE_ULE_UPGRADE_DONE;
            send_fifo_message_to_mgr(&fifo_msg);
#endif
            g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;
            break;
        }
        default:
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported msg type %s(%d).\n",
                           hc_map_msg_txt(task->msgtype), task->msgtype);
            msg_DECTReply(task->appid, HC_EVENT_RESP_FUNCTION_NOT_SUPPORT, &task->devinfo);
            return -1;
    }

    return 0;
}

int task_init(void)
{
    if (queue_init() != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Initial queue error, exit");
        return -1;
    }

    pthread_cond_init(&task_cond, NULL);
    pthread_mutex_init(&task_mutex, NULL);

    return 0;
}

void task_uninit(void)
{
    pthread_cond_destroy(&task_cond);
    pthread_mutex_destroy(&task_mutex);
}

int task_add(TASK_HEAD_S *head , void *devinfo)
{
    QUEUE_UNIT_S *task = NULL;

    if (devinfo == NULL)
        return -1;

    task = calloc(1, sizeof(QUEUE_UNIT_S));
    if (task == NULL)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Allocate QUEUE_UNIT_S memory failed.\n");
        return -1;
    }

    // set task's parameters.
    task->msgtype = head->type;
    task->appid = head->appid;
    task->timeout = head->timeout;
    memcpy(&task->devinfo, devinfo, sizeof(DECT_HAN_DEVICE_INF));

    // push task to queue.
    queue_push(task);
    pthread_cond_signal(&task_cond);

    return 0;
}

void * task_handle_thread(void *arg)
{
    QUEUE_UNIT_S *task = NULL;

    while (keep_looping)
    {
        pthread_mutex_lock(&task_mutex);

        while (queue_empty())
        {
            pthread_cond_wait(&task_cond, &task_mutex);
        }

        task = queue_popup();

        pthread_mutex_unlock(&task_mutex);

        if (task == NULL)
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "=== Bug fix me. queue_popup() return NULL. ===\n");
            return NULL;
        }

        // process task
        process_task(task);


        free(task);

    }

    return NULL;
}


