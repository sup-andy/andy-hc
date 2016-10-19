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
*   File   : dect_msg.c
*   Abstract:
*   Date   : 03/11/2015
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
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "ctrl_common_lib.h"
#include "dect_device.h"
#include "hcapi.h"
#include "hc_msg.h"

#include "dect_msg.h"
#include "dect_task.h"
#include "dect_util.h"

typedef enum {
    DB_OPT_TYPE_ADD = 1,
    DB_OPT_TYPE_DELETE,
    DB_OPT_TYPE_UPDATE,
} DB_OPT_TYPE_E;

extern int keep_looping;
extern int g_dect_controller_mode;
extern int g_dect_add_delete_appid;
extern int g_fw_upgrade_flag;

static int g_msg_fd = -1;

#define DB_OPERATION_RETRY_TIMES    2
#define DB_OPERATION_TIMEOUT_SEC    2

int msg_send(HC_MSG_S *pMsg);

int convert_dectdev_to_hcdev(DECT_HAN_DEVICE_INF *uledev, HC_DEVICE_INFO_S *hcdev)
{
    if (uledev == NULL || hcdev == NULL)
    {
        return -1;
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Dect Device type %d.\n", uledev->type);

    hcdev->dev_id = uledev->id;
    hcdev->network_type = HC_NETWORK_TYPE_ULE;

    /* Protocol do not fill device structure, and HCAPI also
       do not care these info. */
    if (uledev->type == 0)
    {
        return 0;
    }

    switch (uledev->type)
    {
        case DECT_TYPE_ONOFF_SWITCHABLE:
        {
            break;
        }
        case DECT_TYPE_ONOFF_SWITCH:
        case DECT_TYPE_AC_OUTLET:
        case DECT_TYPE_LIGHT:
        case DECT_TYPE_DOOR_LOCK:
        case DECT_TYPE_DOOR_BELL:

        case DECT_TYPE_LEVEL_CONTROLABLE:
        case DECT_TYPE_LEVEL_CONTROL:
        case DECT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE:
        case DECT_TYPE_LEVEL_CONTROL_SWITCH:
        case DECT_TYPE_DIMMABLE_LIGHT:
        case DECT_TYPE_DIMMER_SWITCH:

        case DECT_TYPE_AC_OUTLET_WITH_POWER_METERING:
        case DECT_TYPE_POWER_METER:

        case DECT_TYPE_DETECTOR:
        case DECT_TYPE_DOOR_OPEN_CLOSE_DETECTOR:
        case DECT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR:
        case DECT_TYPE_MOTION_DETECTOR:
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Dect Device type %d.\n", uledev->type);
            break;
        case DECT_TYPE_SMOKE_DETECTOR:
        {
            hcdev->dev_type = HC_DEVICE_TYPE_BINARY_SENSOR;
            hcdev->device.binary_sensor.sensor_type = HC_BINARY_SENSOR_TYPE_SMOKE;
            hcdev->device.binary_sensor.value = uledev->device.binary_sensor.value;
            memset(hcdev->dev_name, 0, sizeof(hcdev->dev_name));
            strncpy(hcdev->dev_name, uledev->name, sizeof(hcdev->dev_name) - 1);
            break;
        }
        case DECT_TYPE_GAS_DETECTOR:
        {
            hcdev->dev_type = HC_DEVICE_TYPE_BINARY_SENSOR;
            hcdev->device.binary_sensor.sensor_type = HC_BINARY_SENSOR_TYPE_CO;
            hcdev->device.binary_sensor.value = uledev->device.binary_sensor.value;
            memset(hcdev->dev_name, 0, sizeof(hcdev->dev_name));
            strncpy(hcdev->dev_name, uledev->name, sizeof(hcdev->dev_name) - 1);
            break;
        }
        case DECT_TYPE_FLOOD_DETECTOR:
        case DECT_TYPE_GLASS_BREAK_DETECTOR:
        case DECT_TYPE_VIBRATION_DETECTOR:
            break;
        case DECT_TYPE_HANDSET:
        {
            hcdev->dev_type = HC_DEVICE_TYPE_HANDSET;
            hcdev->device.handset.u.value = 0;
            memset(hcdev->dev_name, 0, sizeof(hcdev->dev_name));
            strncpy(hcdev->dev_name, uledev->name, sizeof(hcdev->dev_name) - 1);
            break;
        }
        case DECT_TYPE_EXT_PARAMETER:
        {
            hcdev->dev_type = HC_DEVICE_TYPE_EXT_DECT_STATION;
            switch (uledev->device.station.type)
            {
                case DECT_PARAMETER_TYPE_DECTTYPE:
                {
                    hcdev->device.ext_dect_station.type = HC_DECT_PARAMETER_TYPE_DECTTYPE;
                    hcdev->device.ext_dect_station.u.dect_type = uledev->device.station.u.dect_type;
                    break;
                }
                case DECT_PARAMETER_TYPE_BASENAME:
                {
                    hcdev->device.ext_dect_station.type = HC_DECT_PARAMETER_TYPE_BASENAME;
                    memset(hcdev->device.ext_dect_station.u.base_station_name, 0, sizeof(hcdev->device.ext_dect_station.u.base_station_name));
                    strncpy(hcdev->device.ext_dect_station.u.base_station_name, uledev->device.station.u.base_station_name,
                            sizeof(hcdev->device.ext_dect_station.u.base_station_name) - 1);
                    break;
                }
                case DECT_PARAMETER_TYPE_PINCODE:
                {
                    hcdev->device.ext_dect_station.type = HC_DECT_PARAMETER_TYPE_PINCODE;
                    memset(hcdev->device.ext_dect_station.u.pin_code, 0, sizeof(hcdev->device.ext_dect_station.u.pin_code));
                    strncpy(hcdev->device.ext_dect_station.u.pin_code, uledev->device.station.u.pin_code,
                            sizeof(hcdev->device.ext_dect_station.u.pin_code) - 1);
                    break;
                }
                default:
                {
                    ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported dect parameter type %d.\n", uledev->device.station.type);
                    return -1;
                    break;
                }
            }
            break;
        }
        default:
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Dect Device type %d.\n", uledev->type);
            return -1;
        }
    }

    return 0;
}

int convert_dectdev_to_msg(DECT_HAN_DEVICE_INF *devinfo, HC_MSG_S *msg)
{
    if (devinfo == NULL || msg == NULL)
    {
        return -1;
    }


    switch (devinfo->type)
    {
        case DECT_TYPE_ONOFF_SWITCHABLE:
        {
            break;
        }
        case DECT_TYPE_ONOFF_SWITCH:
        case DECT_TYPE_AC_OUTLET:
        case DECT_TYPE_LIGHT:
        case DECT_TYPE_DOOR_LOCK:
        case DECT_TYPE_DOOR_BELL:

        case DECT_TYPE_LEVEL_CONTROLABLE:
        case DECT_TYPE_LEVEL_CONTROL:
        case DECT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE:
        case DECT_TYPE_LEVEL_CONTROL_SWITCH:
        case DECT_TYPE_DIMMABLE_LIGHT:
        case DECT_TYPE_DIMMER_SWITCH:

        case DECT_TYPE_AC_OUTLET_WITH_POWER_METERING:
        case DECT_TYPE_POWER_METER:
        case DECT_TYPE_DETECTOR:
        case DECT_TYPE_DOOR_OPEN_CLOSE_DETECTOR:
        case DECT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR:
        case DECT_TYPE_MOTION_DETECTOR:
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Dect Device type %d.\n", devinfo->type);
            break;
        case DECT_TYPE_SMOKE_DETECTOR:
        {
            msg->head.dev_type = HC_DEVICE_TYPE_BINARY_SENSOR;
            msg->body.device_info.device.binary_sensor.sensor_type = HC_BINARY_SENSOR_TYPE_SMOKE;
            msg->body.device_info.device.binary_sensor.value = devinfo->device.binary_sensor.value;
            break;
        }
        case DECT_TYPE_GAS_DETECTOR:
        {
            msg->head.dev_type = HC_DEVICE_TYPE_BINARY_SENSOR;
            msg->body.device_info.device.binary_sensor.sensor_type = HC_BINARY_SENSOR_TYPE_CO;
            msg->body.device_info.device.binary_sensor.value = devinfo->device.binary_sensor.value;
            break;
        }
        case DECT_TYPE_FLOOD_DETECTOR:
        case DECT_TYPE_GLASS_BREAK_DETECTOR:
        case DECT_TYPE_VIBRATION_DETECTOR:
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Dect Device type %d.\n", devinfo->type);
            break;
        }
        case DECT_TYPE_HANDSET:
        {
            msg->head.dev_type = HC_DEVICE_TYPE_HANDSET;
            memset(&msg->body.device_info.device.handset, 0, sizeof(msg->body.device_info.device.handset));
            msg->body.device_info.device.handset.status = devinfo->device.handset.status;
            msg->body.device_info.device.handset.u.value = devinfo->device.handset.value;

            break;
        }
        case DECT_TYPE_VOICE:
        {
            msg->head.dev_type = HC_DEVICE_TYPE_HANDSET;
            memset(msg->body.device_info.device.handset.u.action, 0, sizeof(msg->body.device_info.device.handset.u.action));
            strncpy(msg->body.device_info.device.handset.u.action, devinfo->device.voice.action, sizeof(msg->body.device_info.device.handset.u.action) - 1);

            break;
        }
        default:
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Dect Device type %d.\n", devinfo->type);
            return -1;
    }

    return 0;
}

int convert_msg_to_dectdev(HC_MSG_S *msg, DECT_HAN_DEVICE_INF *devinfo)
{
    if (devinfo == NULL || msg == NULL)
    {
        return -1;
    }

    devinfo->id = msg->head.dev_id;

    if (msg->head.type == HC_EVENT_REQ_DEVICE_SET
            || msg->head.type == HC_EVENT_REQ_DEVICE_DETECT)
    {
        switch (msg->head.dev_type)
        {
            case HC_DEVICE_TYPE_BINARYSWITCH:
                break;
            case HC_DEVICE_TYPE_DOUBLESWITCH:
                break;
            case HC_DEVICE_TYPE_DIMMER:
                break;
            case HC_DEVICE_TYPE_CURTAIN:
                break;
            case HC_DEVICE_TYPE_BINARY_SENSOR:
                devinfo->type = DECT_TYPE_DETECTOR;
                switch (msg->body.device_info.device.binary_sensor.sensor_type)
                {
                    case HC_BINARY_SENSOR_TYPE_SMOKE:
                    {
                        devinfo->type = DECT_TYPE_SMOKE_DETECTOR;
                        break;
                    }
                    case HC_BINARY_SENSOR_TYPE_CO:
                    {
                        devinfo->type = DECT_TYPE_GAS_DETECTOR;
                        break;
                    }
                    default:
                        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Binary Sensor type %d.\n",
                                       msg->body.device_info.device.binary_sensor.sensor_type);
                        break;
                }

                devinfo->device.binary_sensor.value = msg->body.device_info.device.binary_sensor.value;
                break;
                /*case HC_DEVICE_TYPE_SMOKE_SENSOR:
                    devinfo->type = DECT_TYPE_SMOKE_DETECTOR;
                    devinfo->device.binary_sensor.value = msg->body.device_info.device.binary_sensor.value;
                    break;
                case HC_DEVICE_TYPE_GAS_SENSOR:
                    devinfo->type = DECT_TYPE_GAS_DETECTOR;
                    devinfo->device.binary_sensor.value = msg->body.device_info.device.binary_sensor.value;
                    break;*/
            case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
                break;

            case HC_DEVICE_TYPE_BATTERY:
                break;
            case HC_DEVICE_TYPE_DOORLOCK:
                break;
            case HC_DEVICE_TYPE_THERMOSTAT:
                break;
            case HC_DEVICE_TYPE_METER:
                break;

            case HC_DEVICE_TYPE_HANDSET:
            {
                devinfo->type = DECT_TYPE_HANDSET;
                memset(&devinfo->device.handset, 0, sizeof(devinfo->device.handset));
                devinfo->device.handset.value = msg->body.device_info.device.handset.u.value;
                devinfo->device.handset.status = msg->body.device_info.device.handset.status;
                break;
            }
            default:
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported HC Device type %d.\n", devinfo->type);
                return -1;
        }
    }
    else if (msg->head.type == HC_EVENT_REQ_VOIP_CALL)
    {
        devinfo->type = DECT_TYPE_VOICE;
        memset(devinfo->device.voice.action, 0, sizeof(devinfo->device.voice.action));
        strncpy(devinfo->device.voice.action, msg->body.device_info.device.handset.u.action, sizeof(devinfo->device.voice.action) - 1);

    }
    if (msg->head.type == HC_EVENT_REQ_DECT_PARAMETER_SET ||
            msg->head.type == HC_EVENT_REQ_DECT_PARAMETER_GET)
    {
        devinfo->type = DECT_TYPE_EXT_PARAMETER;
        switch (msg->body.device_info.device.ext_dect_station.type)
        {
            case HC_DECT_PARAMETER_TYPE_DECTTYPE:
            {
                devinfo->device.station.type = DECT_PARAMETER_TYPE_DECTTYPE;
                devinfo->device.station.u.dect_type = msg->body.device_info.device.ext_dect_station.u.dect_type;
                break;
            }
            case HC_DECT_PARAMETER_TYPE_BASENAME:
            {
                devinfo->device.station.type = DECT_PARAMETER_TYPE_BASENAME;
                memset(devinfo->device.station.u.base_station_name, 0, sizeof(devinfo->device.station.u.base_station_name));
                strncpy(devinfo->device.station.u.base_station_name, msg->body.device_info.device.ext_dect_station.u.base_station_name,
                        sizeof(devinfo->device.station.u.base_station_name) - 1);
                break;
            }
            case HC_DECT_PARAMETER_TYPE_PINCODE:
            {
                devinfo->device.station.type = DECT_PARAMETER_TYPE_PINCODE;
                memset(devinfo->device.station.u.pin_code, 0, sizeof(devinfo->device.station.u.pin_code));
                strncpy(devinfo->device.station.u.pin_code, msg->body.device_info.device.ext_dect_station.u.pin_code,
                        sizeof(devinfo->device.station.u.pin_code) - 1);
                break;
            }
            default:
            {
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported HC dect parameter type %d.\n", msg->body.device_info.device.ext_dect_station.type);
                return -1;
            }
        }
    }
    else if (msg->head.type == HC_EVENT_REQ_DEVICE_DELETE)
    {
        devinfo->id = msg->body.option_info.u.id;

    }

    return 0;
}

int convert_hcdev_to_dectdev(HC_DEVICE_INFO_S *hcdev, DECT_HAN_DEVICE_INF *devinfo)
{
    if (devinfo == NULL || hcdev == NULL)
    {
        return -1;
    }

    devinfo->id = hcdev->dev_id;

    switch (hcdev->dev_type)
    {
        case HC_DEVICE_TYPE_BINARYSWITCH:
            break;
        case HC_DEVICE_TYPE_DOUBLESWITCH:
            break;
        case HC_DEVICE_TYPE_DIMMER:
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            break;
        case HC_DEVICE_TYPE_BINARY_SENSOR:
            devinfo->type = DECT_TYPE_SMOKE_DETECTOR;
            devinfo->device.binary_sensor.value = hcdev->device.binary_sensor.value;
            memset(devinfo->name, 0, sizeof(devinfo->name));
            strncpy(devinfo->name,  hcdev->dev_name, sizeof(devinfo->name) - 1);
            break;
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            break;

        case HC_DEVICE_TYPE_BATTERY:
            break;
        case HC_DEVICE_TYPE_DOORLOCK:
            break;
        case HC_DEVICE_TYPE_THERMOSTAT:
            break;
        case HC_DEVICE_TYPE_METER:
            break;

        default:
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported HC Device type %d.\n", devinfo->type);
            return -1;
    }

    return 0;
}

static int send_msg_to_app(HC_MSG_S *msg, DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;
    char *str = NULL;
    HC_DEVICE_INFO_S *hcdev = NULL;
    HC_DEVICE_INFO_S hcdev_tmp;

    memset(&hcdev_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    if (msg == NULL)
        return -1;

    msg->head.src = DAEMON_ULE;
    msg->head.dst = APPLICATION;

    hcdev = &msg->body.device_info;
    hcdev->network_type = HC_NETWORK_TYPE_ULE;
    hcdev->event_type = msg->head.type;

    if (devinfo)
    {
        msg->head.dev_id = devinfo->id;

        ret = convert_dectdev_to_hcdev(devinfo, hcdev);
        if (ret != 0)
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Convert DECT device to MSG failed.\n");
            return -1;
        }

        if (msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS)
        {
            if (HC_DEVICE_TYPE_BINARY_SENSOR == hcdev->dev_type)
            {
                strncpy(hcdev->dev_name, hc_map_bin_sensor_txt(hcdev->device.binary_sensor.sensor_type), sizeof(hcdev->dev_name) - 1);
            }
            else if (HC_DEVICE_TYPE_MULTILEVEL_SENSOR == hcdev->dev_type)
            {
                strncpy(hcdev->dev_name, hc_map_multi_sensor_txt(hcdev->device.multilevel_sensor.sensor_type), sizeof(hcdev->dev_name) - 1);
            }
            else if (HC_DEVICE_TYPE_METER == hcdev->dev_type)
            {
                strncpy(hcdev->dev_name, hc_map_meter_txt(hcdev->device.meter.meter_type), sizeof(hcdev->dev_name) - 1);
            }
            else
            {
                strncpy(hcdev->dev_name, hc_map_hcdev_txt(hcdev->dev_type), sizeof(hcdev->dev_name) - 1);
            }
        }
    }
    switch(msg->head.type)
    {
        case HC_EVENT_RESP_DEVICE_ADDED_SUCCESS:
        {
            ret = hcapi_get_dev_by_dev_id(hcdev);
            if (ret == HC_RET_SUCCESS)
            {
                ret = HC_DB_ACT_OK;
            }
            else
            {
                memcpy(&hcdev_tmp, hcdev, sizeof(HC_DEVICE_INFO_S));
                //ret = hcapi_add_dev(hcdev);
                ret = hcapi_add_dev(&hcdev_tmp);
                
                if (ret != HC_RET_SUCCESS)
                {
                    ret = HC_DB_ACT_FAIL;
                }
                else
                {
                    ret = HC_DB_ACT_OK;
                }
            }
        }
        break;

        case HC_EVENT_RESP_DEVICE_DELETED_SUCCESS:
        {
            ret = hcapi_del_dev(hcdev);
            if (ret != HC_RET_SUCCESS)
            {
                ret = HC_DB_ACT_FAIL;
            }
            else
            {
                ret = HC_DB_ACT_OK;
            }
        }
        break;

        case HC_EVENT_RESP_DEVICE_SET_SUCCESS:
        case HC_EVENT_STATUS_DEVICE_STATUS_CHANGED:
        {
            ret = hcapi_set_dev(hcdev);
            if (ret != HC_RET_SUCCESS)
            { 
                ret = HC_DB_ACT_FAIL;
            }
            else
            {
                ret =  HC_DB_ACT_OK;
            }
        }
        break;

        case HC_EVENT_RESP_DEVICE_CONNECTED:
        {
            ret = hcapi_set_attr(hcdev->dev_id,  "connection", "online");
            if (ret != HC_RET_SUCCESS)
            {
                ret =  HC_DB_ACT_FAIL;
            }
            else
            {
                ret =  HC_DB_ACT_OK;
            }
        }
        break;

        case HC_EVENT_RESP_DEVICE_DISCONNECTED:
        {
            ret = hcapi_set_attr(hcdev->dev_id, "connection", "offline");
            if (ret != HC_RET_SUCCESS)
            {
                ret =  HC_DB_ACT_FAIL;
            }
            else
            {
                ret =  HC_DB_ACT_OK;
            }
        }
        break;

        default:
        break;
    }

    if (HC_DB_ACT_FAIL == ret)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Update DB id:%08X, phy_id:%d, dev_type:%d failed, ret:%d.\n",
            hcdev->dev_id, hcdev->phy_id, msg->head.dev_type, ret);
        msg->head.type = HC_EVENT_RESP_DB_OPERATION_FAILURE;
    }
    else if (HC_DB_ACT_NO_CHANGE == ret)
    {
        if (msg->head.appid == 0)
        {
            if (HC_EVENT_STATUS_DEVICE_STATUS_CHANGED == msg->head.type ||
                HC_EVENT_RESP_DEVICE_CONNECTED == msg->head.type ||
                HC_EVENT_RESP_DEVICE_DISCONNECTED == msg->head.type)
            {
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Status changed but DB no change, just return.\n");
                return 0;
            }
        }
    }

    ret = msg_send(msg);
    if (ret <= 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Call msg_send failed, ret is %d.\n", ret);
        return -1;
    }

    return 0;
}
static int send_msg_to_voip(HC_MSG_S *msg, DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;

    if (msg == NULL)
        return -1;

    msg->head.src = DAEMON_ULE;
    msg->head.dst = APPLICATION;
    msg->head.appid = 0;

    if (devinfo)
    {
        msg->head.dev_id = devinfo->id;

        ret = convert_dectdev_to_msg(devinfo, msg);
        if (ret != 0)
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Convert dect device to MSG failed.\n");
            return -1;
        }
    }

    ret = msg_send(msg);
    if (ret <= 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Call msg_send failed, ret is %d.\n", ret);
        return -1;
    }

    return 0;
}
static int process_msg(HC_MSG_S *pMsg)
{
    int ret = 0;
    DECT_HAN_DEVICE_INF devinfo;
    TASK_HEAD_S head;

    if (pMsg == NULL)
    {
        return -1;
    }

    // when dect lock success, do not accept any msg except below.
    if (g_fw_upgrade_flag == 1)
    {
        if (pMsg->head.type != HC_EVENT_EXT_FW_UPGRADE_LOCK
                && pMsg->head.type != HC_EVENT_EXT_FW_UPGRADE_UNLOCK)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, 
                        "Ignore msg %s(%d). device id[%x], type[%d]\n", 
                        hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.dev_id, pMsg->head.dev_type);
            return 0;
        }
    }

    // parse msg.
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Process msg %s(%d). device id[%x], type[%d]\n", hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.dev_id, pMsg->head.dev_type);

    memset(&devinfo, 0, sizeof(devinfo));
    ret = convert_msg_to_dectdev(pMsg, &devinfo);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Convert HC device %s(%d)to dect device info failed.\n",
                       hc_map_hcdev_txt(pMsg->head.dev_type), pMsg->head.dev_type);
        return -1;
    }

    if (pMsg->head.type == HC_EVENT_REQ_DEVICE_ADD_STOP)
    {
        g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;
        ret = dect_stop_register();
        if (ret == 0)
        {
            msg_DECTReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_STOP_SUCCESS, NULL);
            return 0;
        }
        else
        {
            msg_DECTReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_STOP_FAILURE, NULL);
            return -1;
        }
    }
    else if (pMsg->head.type == HC_EVENT_EXT_FW_UPGRADE_LOCK)
    {
        if (g_dect_controller_mode == DECT_CONTROLLER_MODE_ADDING
                || g_dect_controller_mode == DECT_CONTROLLER_MODE_DELETING
                || g_dect_controller_mode == DECT_CONTROLLER_MODE_UPGRADING)
        {
            msg_DECTReply(pMsg->head.appid, HC_EVENT_EXT_FW_UPGRADE_LOCK_FAILED, NULL);
            return 0;
        }
        else
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "FW upgrade DECT lock.\n");
            g_fw_upgrade_flag = 1;
            msg_DECTReply(pMsg->head.appid, HC_EVENT_EXT_FW_UPGRADE_LOCK_DONE, NULL);
            return 0;
        }
                
    }
    else if (pMsg->head.type == HC_EVENT_EXT_FW_UPGRADE_UNLOCK)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "FW upgrade DECT unlock.\n");
        g_fw_upgrade_flag = 0;
        return 0;
    }

    // add task.
    memset(&head, 0, sizeof(TASK_HEAD_S));
    head.type = pMsg->head.type;
    head.appid = pMsg->head.appid;
    head.timeout = pMsg->body.option_info.u.timeout;
    ret = task_add(&head, (void *)&devinfo);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Add task to queue failed.\n");
        return -1;
    }

    return 0;
}

int msg_init(void)
{
    // invoke library MSG provided APIs to create socket.
    g_msg_fd = hc_client_msg_init(DAEMON_ULE, SOCKET_DEFAULT);
    if (g_msg_fd == -1)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Call hc_client_msg_init failed, errno is '%d'.\n", errno);
        return -1;
    }

    return 0;
}

void msg_uninit(void)
{
    if (g_msg_fd !=  -1)
    {
        hc_client_unint(g_msg_fd);
    }
}

int msg_send(HC_MSG_S *pMsg)
{
    return hc_client_send_msg_to_dispacther(g_msg_fd, pMsg);
}


int msg_handle(void)
{
    int ret = 0;
    struct timeval tv;
    //int i = 0;
    HC_MSG_S *hcmsg = NULL;

    while (keep_looping)
    {
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        hcmsg = NULL;
        ret = hc_client_wait_for_msg(g_msg_fd, &tv, &hcmsg);

        if (ret == 0)
        {
            hc_msg_free(hcmsg);
            continue;
        }
        else if (ret == -1)
        {
            hc_msg_free(hcmsg);
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
            msg_uninit();
            msg_init;
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "new g_msg_fd =  '%d'.\n", g_msg_fd);
            continue;
        }

        process_msg(hcmsg);

        hc_msg_free(hcmsg);

    }

    return 0;
}

void msg_DECTReport(DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    if (devinfo == NULL)
        return;

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "id %d.\n", devinfo->id);

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;

    ret = send_msg_to_app(&msg, devinfo);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Send msg to app failed.\n");
    }

}

void msg_DECTVoiceReq(DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    if (devinfo == NULL)
        return;

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "voice reqestion [%s].\n", devinfo->device.voice.action);

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = HC_EVENT_REQ_VOIP_CALL;

    ret = send_msg_to_voip(&msg, devinfo);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Send msg to app failed.\n");
    }

}
void msg_DECTVoiceReply(DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    if (devinfo == NULL)
        return;

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "voice reqestion [%s].\n", devinfo->device.voice.action);

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = HC_EVENT_RESP_VOIP_CALL;

    ret = send_msg_to_voip(&msg, devinfo);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Send msg to app failed.\n");
    }

}
void msg_DECTReply(int appid, HC_EVENT_TYPE_E type, DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    if (devinfo)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "%s(%d), id[%x], type[%d].\n", hc_map_msg_txt(type), type, devinfo->id, devinfo->type);
    }
    else
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "%s(%d).\n", hc_map_msg_txt(type), type);
    }

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = type;
    msg.head.appid = appid;

    ret = send_msg_to_app(&msg, devinfo);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Send msg to app failed.\n");
    }

    if (HC_EVENT_RESP_DEVICE_SET_SUCCESS == type)
    {
        msg_DECTReport(devinfo);
    }

}

void msg_DECTAddSuccess(DECT_HAN_DEVICE_INF *devinfo)
{
    int ret = 0;

    if (devinfo == NULL)
        return;

    g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;

    //msg_DECTReply(g_dect_add_delete_appid, HC_EVENT_RESP_DEVICE_ADDED_SUCCESS, devinfo);

    msg_DECTReply(0, HC_EVENT_RESP_DEVICE_ADDED_SUCCESS, devinfo);

}

void msg_DECTAddFail(int status)
{
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "\n");

    g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;

    //msg_DECTReply(g_dect_add_delete_appid, HC_EVENT_RESP_DEVICE_ADDED_FAILURE, NULL);
    msg_DECTReply(0, HC_EVENT_RESP_DEVICE_ADDED_FAILURE, NULL);

}


void msg_DECTRemoveSuccess(unsigned int id)
{
    int ret = 0;
    int i = 0;

    if (0 == id)
    {
        memset(&g_Device_info, 0, sizeof(g_Device_info));
    }
    else
    {
        for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
        {
            if (id == g_Device_info.device[i].id)
            {
                /* new device */
                break;
            }
        }
        if (i >= DECT_CMBS_DEVICES_MAX)
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unknow device id [%d]...\n", id);
            return;
        }
    }

    g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;

    msg_DECTReply(g_dect_add_delete_appid, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &g_Device_info.device[i]);

    //msg_DECTReport(&g_Device_info.device[i]);

    //msg_DECTReply(0, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &g_Device_info.device[i]);

    memset(&g_Device_info.device[i], 0, sizeof(DECT_HAN_DEVICE_INF));

}

void msg_DECTRemoveFail(int status)
{
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "%s.\n", __FUNCTION__);

    g_dect_controller_mode = DECT_CONTROLLER_MODE_IDLE;

    msg_DECTReply(g_dect_add_delete_appid, HC_EVENT_RESP_DEVICE_DELETED_FAILURE, NULL);
}
