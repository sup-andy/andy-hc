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
*   File   : zw_task.c
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
#include <errno.h>

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "zwave_api.h"

#include "hc_common.h"
#include "zw_task.h"
#include "zw_queue.h"
#include "zw_msg.h"

extern int keep_looping;

int g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
int g_zw_add_delete_appid = 0;
int g_fw_upgrade_flag = 0;
int zwave_has_device_added = 0;
QUEUE_UNIT_S g_running_task;

static pthread_cond_t task_cond;
static pthread_mutex_t task_mutex;



#define MAX_GET_DEV_RETRY_NUM   10
#define MAX_GET_DEV_FAIL_NUM    2
#define MAX_ADD_DELETE_DEV_FAIL_NUM    3

/* define MACRO */
#if (HC_DEVICE_SYNC_FREQUENCY == 1) // WKSSYS DEMO
 // unit: second(s)
#define DEFAULT_CONNECT_DETECT_INTERVAL      15
#define DOORLOCK_CONNECT_DETECT_INTERVAL     0
#define DOORLOCK_DISCONNECT_DETECT_INTERVAL  0
#define THERMOSTAT_CONNECT_DETECT_INTERVAL   15
#define SENSOR_NO_SIGNAL_TIMEOUT              3605
#else
#define DEFAULT_CONNECT_DETECT_INTERVAL      180 // seconds
#define DOORLOCK_CONNECT_DETECT_INTERVAL     0
#define DOORLOCK_DISCONNECT_DETECT_INTERVAL  0
#define THERMOSTAT_CONNECT_DETECT_INTERVAL   3600*6
#define SENSOR_NO_SIGNAL_TIMEOUT              3605 // 1 hour + 5 seconds
#endif

int compare_zwdev(ZW_DEVICE_INFO *dest, ZW_DEVICE_INFO *src)
{
    if (dest == NULL || src == NULL)
    {
        return -1;
    }

    switch (src->type)
    {
        case ZW_DEVICE_BINARYSWITCH:
            if ((dest->status.binaryswitch_status.value == 0 && src->status.binaryswitch_status.value > 0) ||
                    (dest->status.binaryswitch_status.value > 0 && src->status.binaryswitch_status.value == 0))
                return -1;
            break;
        case ZW_DEVICE_DIMMER:
            DEBUG_INFO("Dimmer get:%d - set:%d\n", dest->status.dimmer_status.value, src->status.dimmer_status.value);
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
            if (dest->status.multilevelsensor_status.scale != src->status.multilevelsensor_status.scale)
                return -1;
            if (0 == double_equal(dest->status.multilevelsensor_status.value, src->status.multilevelsensor_status.value))
                return -1;
            break;
        case ZW_DEVICE_BATTERY:
            if (dest->status.battery_status.battery_level != src->status.battery_status.battery_level)
                return -1;
            if (dest->status.battery_status.interval_time != src->status.battery_status.interval_time)
                return -1;
            break;
        case ZW_DEVICE_DOORLOCK:
            /*
             * GET doorlock status, 0xFF is lock. but not only 0 is unlock, maybe it's 1,
             * so get status is not 0xFF, we think it's unlock.
             * 1. SET 0xFF(lock), GET NOT 0xFF(unlock)
             * 2. SET 0x0(unlock), GET 0xFF(lock), since 0, 1 ... all are unlock.
             */
            DEBUG_INFO("Doorlock get:%d - set:%d\n", dest->status.doorlock_status.doorLockMode, src->status.doorlock_status.doorLockMode);
            if ((dest->status.doorlock_status.doorLockMode != 0xFF && src->status.doorlock_status.doorLockMode == 0xFF) ||
                    (dest->status.doorlock_status.doorLockMode == 0xFF && src->status.doorlock_status.doorLockMode == 0))
                return -1;
            break;
        case ZW_DEVICE_THERMOSTAT:
            /*
            if (dest->status.thermostat_status.mode != src->status.thermostat_status.mode)
                return -1;
            if (src->status.thermostat_status.value > 0 &&
                    dest->status.thermostat_status.value != src->status.thermostat_status.value)
                return -1;
                */
            break;
        case ZW_DEVICE_METER:
            if (dest->status.meter_status.scale != src->status.meter_status.scale)
                return -1;
            if (dest->status.meter_status.rate_type != src->status.meter_status.rate_type)
                return -1;
            if (0 == double_equal(dest->status.meter_status.value, src->status.meter_status.value))
                return -1;
            if (dest->status.meter_status.delta_time != src->status.meter_status.delta_time)
                return -1;
            if (0 == double_equal(dest->status.meter_status.previous_value, src->status.meter_status.previous_value))
                return -1;

            break;
        case ZW_DEVICE_KEYFOB:
            DEBUG_INFO("Keyfob get:%d - set:%d\n", dest->status.keyfob_status.value, src->status.keyfob_status.value);
            if (dest->status.keyfob_status.value != src->status.keyfob_status.value)
                return -1;
            break;
        case ZW_DEVICE_SIREN:
            DEBUG_INFO("Siren get:%d - set:%d\n", dest->status.siren_status.value, src->status.siren_status.value);
            if ((dest->status.siren_status.value == 0 && src->status.siren_status.value > 0) ||
                    (dest->status.siren_status.value > 0 && src->status.siren_status.value == 0))
                return -1;
            break;
            
        default:
            DEBUG_ERROR("%s: Unsupported ZW Device type %d.\n", __FUNCTION__, src->type);
            return -1;
    }

    return 0;
}



static int process_task(QUEUE_UNIT_S *task)
{
    int ret = 0;
    int retry_count = 0;
    int fail_count = 0;
    ZW_DEVICE_INFO zwdev;
    MID_DEVICE_INFO_S *mid_dev = NULL;
    int idx = 0;

    if (task == NULL)
        return -1;

    if (g_zw_controller_mode == ZW_CONTROLLER_MODE_ADDING)
    {
        if (HC_EVENT_REQ_DEVICE_DETECT != task->msgtype)
        {
            msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_ADD_MODE, NULL);
        }
        return 0;
    }
    else if (g_zw_controller_mode == ZW_CONTROLLER_MODE_DELETING)
    {
        if (HC_EVENT_REQ_DEVICE_DETECT != task->msgtype)
        {
            msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DELETE_MODE, NULL);
        }
        return 0;
    }

    memset(&zwdev, 0, sizeof(ZW_DEVICE_INFO));

    switch (task->msgtype)
    {
        case HC_EVENT_REQ_DEVICE_ADD:
            DEBUG_INFO("=========== Process Add Task ===========\n");
            while (fail_count++ < MAX_ADD_DELETE_DEV_FAIL_NUM)
            {
                ret = ZWapi_AddNodeToNetwork(msg_ZWAddSuccess, msg_ZWAddFail);
                if (ret != ZW_STATUS_OK)
                {
                    DEBUG_ERROR("Call ZWapi_AddNodeToNetwork failed, ret=%d.\n", ret);
                    ret = ZWapi_StopAddNodeToNetwork();
                    if (ret != ZW_STATUS_OK)
                    {
                        DEBUG_ERROR("Call ZWapi_StopAddNodeToNetwork failed, ret=%d.\n", ret);
                        return -1;
                    }
                    sleep(1);
                    continue;
                }
                g_zw_controller_mode = ZW_CONTROLLER_MODE_ADDING;
                g_zw_add_delete_appid = task->appid;
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_ADD_MODE, NULL);

                DEBUG_INFO("Now, Let's wait for adding device completed \n");
                ZWapi_WaitAddRemoveCompleted(NULL);
                DEBUG_INFO("Add device completed \n");

                // for auto scan
                if(zwave_has_device_added)
                {
                    msg_ZWReply(0, HC_EVENT_RESP_DEVICE_ADD_COMPLETE, NULL);
                    zwave_has_device_added = 0;
                }

                g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                g_zw_add_delete_appid = 0;
                return 0;
            }
            // Add task do not directly return failure, just let UI wait timeout.
            break;
        case HC_EVENT_REQ_DEVICE_DELETE:
            DEBUG_INFO("=========== Process Delete Task ===========\n");
            while (fail_count++ < MAX_ADD_DELETE_DEV_FAIL_NUM)
            {
                ret = ZWapi_RemoveNodeFromNetwork(msg_ZWRemoveSuccess, msg_ZWRemoveFail);
                if (ret != ZW_STATUS_OK)
                {
                    DEBUG_ERROR("Call ZWapi_RemoveNodeFromNetwork failed, ret=%d.\n", ret);
                    ret = ZWapi_StopRemoveNodeFromNetwork();
                    if (ret != ZW_STATUS_OK)
                    {
                        DEBUG_ERROR("Call ZWapi_StopRemoveNodeFromNetwork failed, ret=%d.\n", ret);
                        return -1;
                    }
                    sleep(1);
                    continue;
                }
                g_zw_controller_mode = ZW_CONTROLLER_MODE_DELETING;
                g_zw_add_delete_appid = task->appid;
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DELETE_MODE, NULL);

                DEBUG_INFO("Now, Let's wait for deleting device completed \n");
                ZWapi_WaitAddRemoveCompleted(NULL);
                DEBUG_INFO("Delete device completed \n");

                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DELETE_COMPLETE, NULL);

                g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                g_zw_add_delete_appid = 0;
                return 0;
            }
            // Delete task do not directly return failure, just let UI wait timeout.
            break;
        case HC_EVENT_REQ_DEVICE_SET:
            DEBUG_INFO("=========== Process Set Task, id:%x ===========\n", task->device.devinfo.id);
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            while (fail_count++ < MAX_GET_DEV_FAIL_NUM)
            {
                ret = ZWapi_SetDeviceStatus(&task->device.devinfo);
                if (ret != ZW_STATUS_OK)
                {
                    DEBUG_ERROR("Call ZWapi_SetDeviceStatus failed retry, ret:%d, id=%x, phy_id:%x, type:%d. \n",
                                ret, task->device.devinfo.id, task->device.devinfo.phy_id, task->device.devinfo.type);
                    sleep(1);
                    continue;
                }
                fail_count = 0;
                break;
            }

            if (fail_count >= MAX_GET_DEV_FAIL_NUM)
            {
                DEBUG_ERROR("Call ZWapi_SetDeviceStatus failed return, ret:%d, id=%x, phy_id:%x, type:%d. \n",
                            ret, task->device.devinfo.id, task->device.devinfo.phy_id, task->device.devinfo.type);
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_SET_FAILURE, &task->device.devinfo);
                g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                return -1;
            }

            /* After SET success, wait several seconds then GET status. */
            if (task->device.devinfo.type == ZW_DEVICE_DIMMER ||
                    task->device.devinfo.type == ZW_DEVICE_DOORLOCK)
                sleep(3);
            else
                sleep(1);

            /* Get device info. */
            retry_count = 0;
            fail_count = 0;
            zwdev.id = task->device.devinfo.id;
            while (retry_count++ < MAX_GET_DEV_RETRY_NUM)
            {
                while (fail_count++ < MAX_GET_DEV_FAIL_NUM)
                {
                    ret = ZWapi_GetDeviceStatus(&zwdev);
                    if (ret != ZW_STATUS_OK)
                    {
                        DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed retry, ret:%d, id=%x, phy_id:%x, type:%d. \n",
                                    ret, zwdev.id, zwdev.phy_id, zwdev.type);
                        sleep(1);
                        continue;
                    }
                    fail_count = 0;
                    break;
                }

                if (fail_count >= MAX_GET_DEV_FAIL_NUM)
                {
                    DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed return, ret:%d, id=%x, phy_id:%x, type:%d. \n",
                                ret, zwdev.id, zwdev.phy_id, zwdev.type);
                    msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_SET_FAILURE, &zwdev);
                    g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                    return -1;
                }

                ret = compare_zwdev(&zwdev, &task->device.devinfo);
                if (ret == 0)
                {
                    msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_SET_SUCCESS, &zwdev);
                    g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                    return 0;
                }
                // compare failed, sleep awhile and try again.
                sleep(1);
            }
            DEBUG_ERROR("In SET task, Call ZWapi_GetDeviceStatus failed, ret=%d, retry %d times, return failure. \n", ret, MAX_GET_DEV_RETRY_NUM);
            msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_SET_FAILURE, &zwdev);
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_DEVICE_GET:
            DEBUG_INFO("=========== Process Get Task, id:%x ===========\n", task->device.devinfo.id);
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            while (fail_count++ < MAX_GET_DEV_FAIL_NUM)
            {
                ret = ZWapi_GetDeviceStatus(&task->device.devinfo);
                if (ret != ZW_STATUS_OK)
                {
                    DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, fail_count=%d \n", ret, fail_count);
                    sleep(1);
                    continue;
                }
                msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                return 0;
            }
            DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, retry %d times, return failure. \n", ret, MAX_GET_DEV_FAIL_NUM);
            msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_GET_FAILURE, &task->device.devinfo);
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_DEVICE_CFG_SET:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            ret = ZWapi_SetDeviceConfiguration(&task->device.config);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_ERROR("Call ZWapi_SetDeviceConfiguration failed, ret=%d.\n", ret);
                msg_ZWReplyCfg(task->appid, HC_EVENT_RESP_DEVICE_SET_CFG_FAILURE, NULL);
            }
            else
            {
                msg_ZWReplyCfg(task->appid, HC_EVENT_RESP_DEVICE_SET_CFG_SUCCESS, &task->device.config);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_DEVICE_CFG_GET:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            ret = ZWapi_GetDeviceConfiguration(&task->device.config);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_ERROR("Call ZWapi_GetDeviceConfiguration failed, ret=%d.\n", ret);
                msg_ZWReplyCfg(task->appid, HC_EVENT_RESP_DEVICE_GET_CFG_FAILURE, &task->device.config);
            }
            else
            {
                msg_ZWReplyCfg(task->appid, HC_EVENT_RESP_DEVICE_GET_CFG_SUCCESS, &task->device.config);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_DEVICE_RESTART:
            break;
        case HC_EVENT_REQ_DEVICE_DEFAULT:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            ret = ZWapi_SetDefault();
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_INFO("Call ZWapi_SetDefault failed, ret is %d.\n", ret);
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DEFAULT_FAILURE, NULL);
            }
            else
            {
                DEBUG_INFO("Call ZWapi_SetDefault success.\n");
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DEFAULT_SUCCESS, NULL);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_DEVICE_DETECT:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            DEBUG_INFO("Detect Phy ID %x.\n", task->device.devinfo.phy_id);
            ret = ZWapi_isFailedNode(task->device.devinfo.phy_id);
            // NODE does not exist in network
            if (ret == ZW_STATUS_IS_FAIL_NODE)
            {
                DEBUG_INFO("Detect ID %x DISCONNECTED.\n", task->device.devinfo.phy_id);
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DISCONNECTED, &task->device.devinfo);

                mid_dev = get_mid_device_by_dev_id(task->device.devinfo.id);
                if (mid_dev && ZW_DEVICE_DOORLOCK == mid_dev->dev_type)
                {
                    mid_dev->time_count = 0;
                    mid_dev->detect_interval = DOORLOCK_DISCONNECT_DETECT_INTERVAL;
                }
            }
            // NODE in network
            else if (ret == ZW_STATUS_NOT_FAIL_NODE)
            {
                DEBUG_INFO("Detect ID %x CONNECTED.\n", task->device.devinfo.phy_id);
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_CONNECTED, &task->device.devinfo);

                mid_dev = get_mid_device_by_dev_id(task->device.devinfo.id);
                if (mid_dev)
                {
                    if (ZW_DEVICE_DOORLOCK == mid_dev->dev_type)
                    {
                        DEBUG_INFO("Device is DOORLOCK, id:%x, phy_id:%x.\n", mid_dev->dev_id, mid_dev->phy_id);
                        mid_dev->time_count = 0;
                        mid_dev->detect_interval = DOORLOCK_CONNECT_DETECT_INTERVAL;

                        while (fail_count++ < MAX_GET_DEV_FAIL_NUM)
                        {
                            ret = ZWapi_GetDeviceStatus(&task->device.devinfo);
                            if (ret != ZW_STATUS_OK)
                            {
                                DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, fail_count=%d \n", ret, fail_count);
                                continue;
                            }

                            msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                            return 0;
                        }
                        DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, retry %d times, return failure. \n", ret, MAX_GET_DEV_FAIL_NUM);
                        //msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                    }
                    else if (ZW_DEVICE_METER == mid_dev->dev_type)
                    {
                        DEBUG_INFO("Device is METER, id:%x, phy_id:%x.\n", mid_dev->dev_id, mid_dev->phy_id);
                        mid_dev->time_count = 0;
                        task->device.devinfo.status.meter_status.scale = METER_ELECTRIC_SCALE_POWER;

                        while (fail_count++ < MAX_GET_DEV_FAIL_NUM)
                        {
                            ret = ZWapi_GetDeviceStatus(&task->device.devinfo);
                            if (ret != ZW_STATUS_OK)
                            {
                                DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, fail_count=%d \n", ret, fail_count);
                                continue;
                            }

                            msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                            return 0;
                        }
                        DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, retry %d times, return failure. \n", ret, MAX_GET_DEV_FAIL_NUM);
                        //msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                    }
                    else if (ZW_DEVICE_THERMOSTAT == mid_dev->dev_type)
                    {
                        DEBUG_INFO("Device is thermostat, id:%x, phy_id:%x.\n", mid_dev->dev_id, mid_dev->phy_id);
                        /*
                        int num = 0;
                        int i = 0;
                        HC_DEVICE_INFO_S *hcdev = NULL;
                        ZW_DEVICE_INFO zwdev;
                        
                        ret = hcapi_get_devnum_by_network_type(HC_NETWORK_TYPE_ZWAVE, &num);
                        if(ret != HC_RET_SUCCESS &&num == 0)
                        {
                            DEBUG_ERROR("Get device num from DB fail. ret [%d] num [%d] \n", ret, num);
                            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                            return -1;
                        }
                        hcdev = calloc(num, sizeof(HC_DEVICE_INFO_S));
                        if (hcdev == NULL)
                        {
                            DEBUG_ERROR("Allocate memory failed.\n");
                            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                            return -1;
                        }
                        
                        ret = hcapi_get_devs_by_network_type(HC_NETWORK_TYPE_ZWAVE, hcdev, num);
                        if(ret != HC_RET_SUCCESS)
                        {
                            DEBUG_ERROR("Get device from DB fail. ret [%d] num [%d] \n", ret, num);
                            free(hcdev);
                            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                            return -1;
                        }

                        for(i = 0; i < num; i++)
                        {
                            DEBUG_INFO("Ready to get status, id:%x, phy_id:%x, task phy id [%x].\n", hcdev[i].dev_id, hcdev[i].phy_id, task->device.devinfo.phy_id);
                            if(hcdev[i].phy_id != task->device.devinfo.phy_id)
                                continue;
                            
                            memset(&zwdev, 0, sizeof(ZW_DEVICE_INFO));
                            ret = convert_hcdev_to_zwdev(&hcdev[i], &zwdev);
                            if(ret != HC_RET_SUCCESS)
                            {
                                DEBUG_ERROR("Convert HC device to ZW device fail. ret [%d] device id[%x]\n", ret, hcdev[i].dev_id);
                                continue;
                            }
                            ret = ZWapi_GetDeviceStatus(&zwdev);
                            if (ret != ZW_STATUS_OK)
                            {
                                DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, dev_id=%x \n", ret, zwdev.id);
                                continue;
                            }
                            
                            msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &zwdev);
                        }

                        free(hcdev);
                        */
                        
                        for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
                        {
                            mid_dev = &g_mid_device_info[idx];
                            if (task->device.devinfo.phy_id == mid_dev->phy_id)
                            {
                                task->device.devinfo.id = mid_dev->dev_id;
                                task->device.devinfo.type = mid_dev->dev_type;
                            
                                ret = ZWapi_GetDeviceStatus(&task->device.devinfo);
                                if (ret != ZW_STATUS_OK)
                                {
                                    DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, dev_id=%x \n", ret, task->device.devinfo.id);
                                    continue;
                                }
                                
                                msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                            }
                        }
                        
                        g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                        return 0;
                        
                    }
                    else if(ZW_DEVICE_BINARYSWITCH == mid_dev->dev_type)
                    {
                        DEBUG_INFO("Device is binarySwitch, id:%x, phy_id:%x.\n", mid_dev->dev_id, mid_dev->phy_id);
                        for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
                        {
                            mid_dev = &g_mid_device_info[idx];
                            if (task->device.devinfo.phy_id == mid_dev->phy_id)
                            {
                                if(mid_dev->dev_type != ZW_DEVICE_METER)
                                    continue;
                                
                                task->device.devinfo.id = mid_dev->dev_id;
                                task->device.devinfo.type = mid_dev->dev_type;
                                task->device.devinfo.status.meter_status.scale = SCALE_ELECTRIC_W;
                            
                                ret = ZWapi_GetDeviceStatus(&task->device.devinfo);
                                if (ret != ZW_STATUS_OK)
                                {
                                    DEBUG_ERROR("Call ZWapi_GetDeviceStatus failed, ret=%d, dev_id=%x \n", ret, task->device.devinfo.id);
                                    continue;
                                }
                                
                                msg_ZWReply(task->appid, HC_EVENT_STATUS_DEVICE_STATUS_CHANGED, &task->device.devinfo);
                            }
                        }
                        
                        g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
                        return 0;
                    }
                }
            }
            else
            {
                DEBUG_INFO("Detect device failed, ret is %d.\n", ret);
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DETECT_FAILURE, &task->device.devinfo);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_DEVICE_NA_DELETE:
            DEBUG_INFO("=========== Process NA Delete Task ===========\n");
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            ret = ZWapi_isFailedNode(task->device.devinfo.phy_id);
            if (ret != ZW_STATUS_IS_FAIL_NODE)
            {
                DEBUG_INFO("It's an online device, phy_id:%x, ret:%d\n", task->device.devinfo.phy_id, ret);
                msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DELETED_FAILURE, &task->device.devinfo);
            }
            else
            {
                // Just device DISCONNECTED, we can remove it.
                ret = ZWapi_RemoveFailedNodeID(task->device.devinfo.phy_id);
                if (ret != ZW_STATUS_OK)
                {
                    DEBUG_INFO("Remove N/A device failure, ret is %d.\n", ret);
                    msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DELETED_FAILURE, &task->device.devinfo);
                }
                else
                {
                    DEBUG_INFO("Remove N/A device success.\n");
                    msg_ZWReply(task->appid, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &task->device.devinfo);

                    //if (task->device.devinfo.id != task->device.devinfo.phy_id)
                    //{
                     //   DEBUG_INFO("Remove N/A device is multi type.\n");
                        for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
                        {
                            mid_dev = &g_mid_device_info[idx];
                            if (task->device.devinfo.phy_id == mid_dev->phy_id &&
                                    task->device.devinfo.id != mid_dev->dev_id)
                            {
                                task->device.devinfo.id = mid_dev->dev_id;
                                task->device.devinfo.type = mid_dev->dev_type;

                                DEBUG_INFO("Remove N/A device, id:%d, phy_id:%d, type:%d.\n",
                                           task->device.devinfo.id, task->device.devinfo.phy_id,
                                           task->device.devinfo.type);
                                msg_ZWReply(0, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &task->device.devinfo);
                            }
                        }
                    //}
                }
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_ASSOICATION_SET:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            ret = ZWapi_AssociationSet(task->device.association.srcPhyId, task->device.association.dstPhyId);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_INFO("Call ZWapi_AssociationSet failure, ret is %d.\n", ret);
                msg_ZWReply(task->appid, HC_EVENT_RESP_ASSOICATION_SET_FAILURE, NULL);
            }
            else
            {
                DEBUG_INFO("Association Set srcPhyId:%x, srcPhyId:%x success.\n",
                           task->device.association.srcPhyId, task->device.association.dstPhyId);
                msg_ZWReply(task->appid, HC_EVENT_RESP_ASSOICATION_SET_SUCCESS, NULL);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_REQ_ASSOICATION_REMOVE:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            ret = ZWapi_AssociationRemove(task->device.association.srcPhyId, task->device.association.dstPhyId);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_INFO("Call ZWapi_AssociationRemove failure, ret is %d.\n", ret);
                msg_ZWReply(task->appid, HC_EVENT_RESP_ASSOICATION_REMOVE_FAILURE, NULL);
            }
            else
            {
                DEBUG_INFO("Association Remove srcPhyId:%x, srcPhyId:%x success.\n",
                           task->device.association.srcPhyId, task->device.association.dstPhyId);
                msg_ZWReply(task->appid, HC_EVENT_RESP_ASSOICATION_REMOVE_SUCCESS, NULL);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        case HC_EVENT_EXT_MODULE_UPGRADE:
            g_zw_controller_mode = ZW_CONTROLLER_MODE_RUNNING;
            if(ZWapi_UpgradeModule(g_image_file_path) == ZW_STATUS_OK) {
                msg_ZWReply(task->appid, HC_EVENT_EXT_MODULE_UPGRADE_SUCCESS, NULL);
            } else {
                msg_ZWReply(task->appid, HC_EVENT_EXT_MODULE_UPGRADE_FAILURE, NULL);
            }
            g_zw_controller_mode = ZW_CONTROLLER_MODE_IDLE;
            break;
        default:
            DEBUG_ERROR("Unsupported msg type %s(%d).\n", hc_map_msg_txt(task->msgtype), task->msgtype);
            return -1;
    }

    return 0;
}

int task_init(void)
{
    if (queue_init() != 0)
    {
        DEBUG_ERROR("Initial queue error, exit");
        return -1;
    }

    memset(&g_running_task, 0, sizeof(QUEUE_UNIT_S));

    pthread_cond_init(&task_cond, NULL);
    pthread_mutex_init(&task_mutex, NULL);

    return 0;
}

void task_uninit(void)
{
    pthread_cond_destroy(&task_cond);
    pthread_mutex_destroy(&task_mutex);
}

int task_add(int msgtype, int appid, int priority, void *devinfo)
{
    QUEUE_UNIT_S *task = NULL;

    if (devinfo == NULL)
        return -1;

    task = calloc(1, sizeof(QUEUE_UNIT_S));
    if (task == NULL)
    {
        DEBUG_ERROR("Allocate QUEUE_UNIT_S memory failed.\n");
        return -1;
    }

    // set task's parameters.
    task->msgtype = msgtype;
    task->appid = appid;
    task->priority = priority;
    memcpy(&task->device, devinfo, sizeof(MID_DEVICE_U));

    // if there is a same task is running, do not add it.
    if (g_zw_controller_mode != ZW_CONTROLLER_MODE_IDLE &&
            memcmp(&g_running_task, task, sizeof(QUEUE_UNIT_S)) == 0)
    {
        DEBUG_INFO("Task %s(%d) appid:%d, pri:%d is running, no need enqueue.\n",
                   hc_map_msg_txt(msgtype), msgtype, appid, priority);
        free(task);
        return 0;
    }

    // if there is a same task in the queue, do not add it.
    if (queue_found(task))
    {
        DEBUG_INFO("Task %s(%d) appid:%d, pri:%d has been added, no need enqueue.\n",
                   hc_map_msg_txt(msgtype), msgtype, appid, priority);
        free(task);
        return 0;
    }

    // push task to queue.
    queue_insert_priority(task);

    queue_dump();

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
            DEBUG_ERROR("=== Bug fix me. queue_popup() return NULL. ===\n");
            return NULL;
        }

        memcpy(&g_running_task, task, sizeof(QUEUE_UNIT_S));

        // process task
        process_task(task);

        free(task);

    }

    return NULL;
}

MID_DEVICE_INFO_S * get_unused_mid_device(void)
{
    int i = 0;

    for (i = 0; i < MAX_MID_DEVICE_NUMBER; i++)
    {
        if (0 == g_mid_device_info[i].dev_id)
        {
            return &g_mid_device_info[i];
        }
    }
    return NULL;
}

MID_DEVICE_INFO_S * get_mid_device_by_dev_id(int dev_id)
{
    int i = 0;

    for (i = 0; i < MAX_MID_DEVICE_NUMBER; i++)
    {
        if (dev_id == g_mid_device_info[i].dev_id)
        {
            return &g_mid_device_info[i];
        }
    }
    return NULL;
}

int set_mid_device_info(MID_DEVICE_INFO_S *mid_dev, ZW_DEVICE_INFO *zw_dev)
{
    MID_DEVICE_INFO_S *middev = NULL;
    int idx = 0;
    int found = 0;
    unsigned int interval_time = 0;

    if (NULL == mid_dev || NULL == zw_dev)
        return -1;

    memset(mid_dev, 0, sizeof(MID_DEVICE_INFO_S));

    mid_dev->dev_id = zw_dev->id;
    mid_dev->phy_id = zw_dev->phy_id;
    mid_dev->dev_type = zw_dev->type;
    mid_dev->time_count = 0;

    for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
    {
        middev = &g_mid_device_info[idx];

        if (zw_dev->phy_id == middev->phy_id &&
                middev->detect_interval > 0)
        {
            found = 1;
            break;
        }
    }

    /*
     * The battery is a special virtual device, it includes wakeup interval setting.
     * if without wakeup interval value just use the default SENSOR_NO_SIGNAL_TIMEOUT.
     */
    if (ZW_DEVICE_BATTERY == zw_dev->type)
    {
        DEBUG_INFO("Battery battery_level:%u, interval_time:%u, found:%d\n",
                   zw_dev->status.battery_status.battery_level, zw_dev->status.battery_status.interval_time, found);

        if (zw_dev->status.battery_status.interval_time > 0)
        {
            interval_time = zw_dev->status.battery_status.interval_time + 10;
        }
        else
        {
            interval_time = SENSOR_NO_SIGNAL_TIMEOUT;
        }

        if (SENSOR_NO_SIGNAL_TIMEOUT == 0)
            interval_time = 0;

        if (found)
        {
            middev->detect_interval = 0;

            DEBUG_INFO("Re-Set MID id:%d, phy_id:%d, type:%d, detect_interval:%d\n",
                       middev->dev_id, middev->phy_id, middev->dev_type, middev->detect_interval);
        }

        mid_dev->detect_interval = interval_time;

    }
    else if (ZW_DEVICE_THERMOSTAT == zw_dev->type)
    {
        DEBUG_INFO("Thermostat\n");

        if (found)
        {
            middev->detect_interval = 0;

            DEBUG_INFO("Re-Set MID id:%d, phy_id:%d, type:%d, detect_interval:%d\n",
                       middev->dev_id, middev->phy_id, middev->dev_type, middev->detect_interval);
        }

        mid_dev->detect_interval = THERMOSTAT_CONNECT_DETECT_INTERVAL;
    }
    else if (ZW_DEVICE_BINARYSENSOR == zw_dev->type ||
             ZW_DEVICE_MULTILEVEL_SENSOR == zw_dev->type)
    {
        /*
         * Since BinarySensor and Multi-Sensor always present with other
         * virtual devices, battery or thermostat, so we can skip them.
         */
        /*
        if (0 == found)
        {
            mid_dev->detect_interval = SENSOR_NO_SIGNAL_TIMEOUT;
        }
        */
    }
    else if (ZW_DEVICE_DOORLOCK == zw_dev->type)
    {
        mid_dev->detect_interval = DOORLOCK_CONNECT_DETECT_INTERVAL;
    }
    else
    {
        if (0 == found)
        {
            mid_dev->detect_interval = DEFAULT_CONNECT_DETECT_INTERVAL;
        }
    }

    DEBUG_INFO("Set MID id:%d, phy_id:%d, type:%d, detect_interval:%d\n",
               mid_dev->dev_id, mid_dev->phy_id, mid_dev->dev_type, mid_dev->detect_interval);

    return 0;
}

void reset_mid_device_info(MID_DEVICE_INFO_S *mid_dev)
{
    if (mid_dev)
    {
        DEBUG_INFO("Reset MID id:%d, phy_id:%d, type:%d, detect_interval:%d\n",
                   mid_dev->dev_id, mid_dev->phy_id, mid_dev->dev_type, mid_dev->detect_interval);

        memset(mid_dev, 0, sizeof(MID_DEVICE_INFO_S));
    }
}

int mid_device_init(ZW_NODES_INFO *zw_nodes)
{
    int i = 0;
    MID_DEVICE_INFO_S *mid_dev = NULL;

    for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
    {
        if (zw_nodes->nodes[i].id > 1)
        {
            mid_dev = get_unused_mid_device();
            if (NULL == mid_dev)
            {
                DEBUG_ERROR("Get unused MID device failed.\n");
                return -1;
            }

            set_mid_device_info(mid_dev, &zw_nodes->nodes[i]);
        }
    }

    return 0;
}

int mid_device_init_from_db(void)
{
    int ret = 0;
    HC_DEVICE_INFO_S *hcdev = NULL;
    ZW_DEVICE_INFO zwdev;
    MID_DEVICE_INFO_S *mid_dev = NULL;
    int devnum = 0;
    int idx = 0;

    ret = hcapi_get_devnum_by_network_type(HC_NETWORK_TYPE_ZWAVE, &devnum);
    if (ret != 0)
    {
        DEBUG_ERROR("Get device number failed.\n");
        return -1;
    }

    DEBUG_INFO("There are %d ZW devices in DB.\n", devnum);

    if (0 == devnum)
    {
        return 0;
    }

    hcdev = calloc(devnum, sizeof(HC_DEVICE_INFO_S));
    if (hcdev == NULL)
    {
        DEBUG_ERROR("Allocate memory failed.\n");
        return -1;
    }

    ret = hcapi_get_devs_by_network_type(HC_NETWORK_TYPE_ZWAVE, hcdev, devnum);
    if (ret != 0)
    {
        DEBUG_ERROR("Get device all failed.\n");
        free(hcdev);
        return -1;
    }

    for (idx = 0; idx < devnum; idx++)
    {
        memset(&zwdev, 0, sizeof(ZW_DEVICE_INFO));
        ret = convert_hcdev_to_zwdev(&hcdev[idx], &zwdev);
        if (ret != 0)
        {
            DEBUG_ERROR("Convert HC to ZW failed, id:%08X, phy_id:%d, type:%d.\n",
                        hcdev[idx].dev_id, hcdev[idx].phy_id, hcdev[idx].dev_type);
            continue;
        }

        mid_dev = get_unused_mid_device();
        if (NULL == mid_dev)
        {
            DEBUG_ERROR("Get unused MID device failed.\n");
            free(hcdev);
            return -1;
        }

        set_mid_device_info(mid_dev, &zwdev);
    }

    free(hcdev);
    return 0;
}

void * detect_handle_thread(void *arg)
{
    int n = 0;
    struct timeval tv;
    MID_DEVICE_U middev;
    int timeout = 0;
    int idx = 0;

    while (keep_looping)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        n = select(1, NULL, NULL, NULL, &tv);

        if (n == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            DEBUG_ERROR("Call select failed, errno is '%d'.\n", errno);
            return -1;
        }
        else if (n == 0)
        {
            if (ZW_CONTROLLER_MODE_IDLE != g_zw_controller_mode
                    || g_fw_upgrade_flag == 1)
            {
                continue;
            }

            for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
            {
                if (0 == g_mid_device_info[idx].dev_id ||
                        0 == g_mid_device_info[idx].detect_interval)
                    continue;

                if (++g_mid_device_info[idx].time_count >= g_mid_device_info[idx].detect_interval)
                {
                    if (ZW_DEVICE_BINARYSENSOR == g_mid_device_info[idx].dev_type ||
                            ZW_DEVICE_MULTILEVEL_SENSOR == g_mid_device_info[idx].dev_type)
                    {
                        g_mid_device_info[idx].time_count = 0;

                        memset(&middev, 0, sizeof(MID_DEVICE_U));
                        middev.devinfo.id = g_mid_device_info[idx].dev_id;
                        middev.devinfo.phy_id = g_mid_device_info[idx].phy_id;
                        middev.devinfo.type = g_mid_device_info[idx].dev_type;
                        msg_ZWReply(0, HC_EVENT_RESP_DEVICE_DISCONNECTED, &middev.devinfo);
                    }
                    else
                    {
                        if (queue_empty())
                        {
                            g_mid_device_info[idx].time_count = 0;

                            memset(&middev, 0, sizeof(MID_DEVICE_U));
                            middev.devinfo.id = g_mid_device_info[idx].dev_id;
                            middev.devinfo.phy_id = g_mid_device_info[idx].phy_id;
                            middev.devinfo.type = g_mid_device_info[idx].dev_type;

                            DEBUG_INFO("Add Detect ZW type:%d to task dev_id:%d, phy_id:%d.\n",
                                       middev.devinfo.type,
                                       middev.devinfo.id,
                                       middev.devinfo.phy_id);

                            task_add(HC_EVENT_REQ_DEVICE_DETECT, 0, TASK_PRIORITY_DETECT, &middev);
                        }
                    }
                }
            }

        }
    }

    return 0;
}

