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
*   File   : zw_msg.c
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
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "zwave_api.h"

#include "hc_common.h"
#include "zw_msg.h"
#include "zw_task.h"
#include "zw_util.h"
#include "zw_queue.h"

extern int keep_looping;
extern int g_zw_controller_mode;
extern int g_zw_add_delete_appid;
extern int g_fw_upgrade_flag;
extern int zwave_has_device_added;


static int g_msg_fd = -1;

int compare_hcdev(HC_DEVICE_INFO_S *dest, HC_DEVICE_INFO_S *src);

void dump_mem(unsigned char *mem, int size)
{
    int idx = 0;
    unsigned char buffer[512] = {0};

    if (mem == NULL || size <= 0)
    {
        DEBUG_ERROR("Error parameters, mem is NULL or size(%d) <= 0.\n", size);
        return;
    }

    if (size > sizeof(buffer))
        size = sizeof(buffer);

    memset(buffer, 0, sizeof(buffer));
    for (idx = 0; idx < 20; idx++)
    {
        sprintf(buffer, "%s%02X, ", buffer, mem[idx]);
    }
    DEBUG_ERROR("============ dump buffer ==========\n[%s]\n", buffer);
}

static int update_mid(HC_MSG_S *msg, ZW_DEVICE_INFO *devinfo)
{
    MID_DEVICE_INFO_S *middev = NULL;
    int idx = 0;

    if (NULL == devinfo)
        return -1;

    /*
     * For multi-level type device, it would be generate several virtual devices.
     * 1. every vritual device will report ADD/DELETE success by CB function.
     * 2. when device remove by user on UI, call HC_EVENT_REQ_DEVICE_NA_DELETE,
     * we also send out DELETE success for per virtual device.
     */
    if (HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == msg->head.type)
    {
        middev = get_mid_device_by_dev_id(devinfo->id);
        if (NULL == middev)
        {
            middev = get_unused_mid_device();
            if (NULL == middev)
            {
                DEBUG_ERROR("Get unused MID device resource failed.\n");
            }
            else
            {
                set_mid_device_info(middev, devinfo);
            }
        }
    }
    else if (HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == msg->head.type)
    {
        middev = get_mid_device_by_dev_id(devinfo->id);
        if (middev)
        {
            reset_mid_device_info(middev);
        }
    }
    else if (HC_EVENT_STATUS_DEVICE_STATUS_CHANGED == msg->head.type
        ||HC_EVENT_RESP_DEVICE_CONNECTED == msg->head.type)
    {
        if (ZW_DEVICE_BATTERY == devinfo->type ||
                ZW_DEVICE_BINARYSENSOR == devinfo->type ||
                ZW_DEVICE_MULTILEVEL_SENSOR == devinfo->type)
        {
            DEBUG_INFO("Receive Sensor event, id:%d, phy_id:%d, type:%d.\n",
                       devinfo->id, devinfo->phy_id, devinfo->type);

            for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
            {
                middev = &g_mid_device_info[idx];

                if (devinfo->phy_id > 0 && devinfo->phy_id == middev->phy_id)
                {
                    middev->time_count = 0;
                }
            }
        }
    }

    return 0;
}

int update_device_in_db(HC_DEVICE_INFO_S *hcdev, ZW_DEVICE_INFO *zwdev)
{
    int ret = 0;
    HC_DEVICE_INFO_S hcdev_tmp;
    char connect_status[32] = {0};
    
    if(hcdev == NULL)
    {
        return HC_DB_ACT_FAIL;
    }

    if(HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == hcdev->event_type)
    {
        ret = hcapi_get_dev_by_dev_id(hcdev);
        if(ret == HC_RET_SUCCESS)
        {
            return HC_DB_ACT_OK;
        }
        else
        {
            ret = hcapi_add_dev(hcdev);
            if(ret != HC_RET_SUCCESS)
            {
                DEBUG_ERROR("Add HC DEV id(%x) failed.\n", hcdev->dev_id);
                return HC_DB_ACT_FAIL;
            }
            else
            {
                ret = hcapi_db_add_zw_dev(zwdev);
                if(ret != HC_RET_SUCCESS)
                {
                    DEBUG_ERROR("Add ZW DEV id(%x) phy_id(%x) failed.\n", zwdev->id, zwdev->phy_id);
                    ret = hcapi_del_dev(hcdev);
                    if(ret != HC_RET_SUCCESS)
                    {
                        DEBUG_ERROR("Remove HC DEV id(%x) failed.\n", hcdev->dev_id);
                    }
                    return HC_DB_ACT_FAIL;
                }
                return HC_DB_ACT_OK;
            }
        }
    }
    else if(HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == hcdev->event_type)
    {
        ret = hcapi_db_del_zw_dev(zwdev);
        if(ret != HC_RET_SUCCESS)
        {
            DEBUG_ERROR("Remove ZW DEV id(%x) phy_id(%x) failed.\n", zwdev->id, zwdev->phy_id);
            //return HC_DB_ACT_FAIL;
        }
    
        ret = hcapi_del_dev(hcdev);
        if(ret != HC_RET_SUCCESS)
        {
            DEBUG_ERROR("Remove HC DEV id(%x) failed.\n", hcdev->dev_id);
            return HC_DB_ACT_FAIL;
        }

        return HC_DB_ACT_OK;
    }
    else if(HC_EVENT_RESP_DEVICE_CONNECTED == hcdev->event_type
        ||HC_EVENT_RESP_DEVICE_DISCONNECTED == hcdev->event_type)
    {
        memset(connect_status, 0, sizeof(connect_status));
        ret = hcapi_get_attr(hcdev->dev_id, "connection", connect_status);
        if(ret != HC_RET_SUCCESS)
        {
            return HC_DB_ACT_FAIL;
        }

        if(HC_EVENT_RESP_DEVICE_CONNECTED == hcdev->event_type)
        {
            if(strcmp(connect_status, "online") == 0)
            {
                return HC_DB_ACT_NO_CHANGE;
            }
            else
            {
                ret = hcapi_set_attr(hcdev->dev_id, "connection", "online");
                if(ret != HC_RET_SUCCESS)
                {
                    return HC_DB_ACT_FAIL;
                }
                else
                {
                    return HC_DB_ACT_OK;
                }
            }
        }
        
        if(HC_EVENT_RESP_DEVICE_DISCONNECTED == hcdev->event_type)
        {
            if(strcmp(connect_status, "offline") == 0)
            {
                return HC_DB_ACT_NO_CHANGE;
            }
            else
            {
                ret = hcapi_set_attr(hcdev->dev_id, "connection", "offline");
                if(ret != HC_RET_SUCCESS)
                {
                    return HC_DB_ACT_FAIL;
                }
                else
                {
                    return HC_DB_ACT_OK;
                }
            }
        }

    }
    else
    {
        memset(&hcdev_tmp, 0, sizeof(hcdev_tmp));
        hcdev_tmp.dev_id = hcdev->dev_id;
        ret = hcapi_get_dev_by_dev_id(&hcdev_tmp);
        if(ret == HC_RET_SUCCESS)
        {
            // binary sensor status change does not report sub-type
            if(hcdev_tmp.dev_type == HC_DEVICE_TYPE_BINARY_SENSOR)
            {
                hcdev->device.binary_sensor.sensor_type = hcdev_tmp.device.binary_sensor.sensor_type;
            }
            else if(hcdev_tmp.dev_type == HC_DEVICE_TYPE_BINARYSWITCH)
            {
                hcdev->device.binaryswitch.type = hcdev_tmp.device.binaryswitch.type;
            }
            else if (hcdev_tmp.dev_type == HC_DEVICE_TYPE_THERMOSTAT)
            {
                if (hcdev->device.thermostat.heat_value == ILLEGAL_TEMPERATURE_VALUE)
                    hcdev->device.thermostat.heat_value = hcdev_tmp.device.thermostat.heat_value;
                
                if (hcdev->device.thermostat.cool_value == ILLEGAL_TEMPERATURE_VALUE)
                    hcdev->device.thermostat.cool_value = hcdev_tmp.device.thermostat.cool_value;
                
                if (hcdev->device.thermostat.energe_save_heat_value == ILLEGAL_TEMPERATURE_VALUE)
                    hcdev->device.thermostat.energe_save_heat_value = hcdev_tmp.device.thermostat.energe_save_heat_value;

                if (hcdev->device.thermostat.energe_save_cool_value == ILLEGAL_TEMPERATURE_VALUE)
                    hcdev->device.thermostat.energe_save_cool_value = hcdev_tmp.device.thermostat.energe_save_cool_value;
            }
            // electric meter just display uint W
            else if (hcdev_tmp.dev_type == HC_DEVICE_TYPE_METER)
            {
                if(hcdev->device.meter.meter_type == HC_METER_TYPE_SINGLE_E_ELECTRIC
                    &&hcdev->device.meter.scale != HC_SCALE_ELECTRIC_W)
                {
                    return HC_DB_ACT_NO_CHANGE;
                }
            }

            if (0 == compare_hcdev(hcdev, &hcdev_tmp))
            {
                //DEBUG_INFO("No changed.\n");
                return HC_DB_ACT_NO_CHANGE;
            }
            else
            {
                //DEBUG_INFO("Device changed, update it.\n");
                ret = hcapi_set_dev(hcdev);
                if(ret != HC_RET_SUCCESS)
                {
                    return HC_DB_ACT_FAIL;
                }
                else
                {
                    return HC_DB_ACT_OK;
                }
            }
        }
        else
        {
            return HC_DB_ACT_FAIL;
        }
    }
    
}

static int send_msg_to_app(HC_MSG_S *msg, ZW_DEVICE_INFO *devinfo)
{
    int ret = 0;
    char *str = NULL;
    HC_DEVICE_INFO_S *hcdev = NULL;

    if (msg == NULL)
        return -1;

    msg->head.src = DAEMON_ZWAVE;
    msg->head.dst = APPLICATION;

    hcdev = &msg->body.device_info;
    hcdev->network_type = HC_NETWORK_TYPE_ZWAVE;
    hcdev->event_type = msg->head.type;

    if (devinfo)
    {
        msg->head.dev_id = devinfo->id;

        ret = convert_zwdev_to_hcdev(devinfo, hcdev);
        if (ret != 0)
        {
            DEBUG_ERROR("Convert ZW device to MSG failed.\n");
            return -1;
        }

        if (msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS)
        {
            if (HC_DEVICE_TYPE_BINARY_SENSOR == hcdev->dev_type)
            {
                snprintf(hcdev->dev_name, MAX_DEVICE_NAME_SIZE - 1, "%s %d", 
                        hc_map_bin_sensor_txt(hcdev->device.binary_sensor.sensor_type), 
                        hcdev->dev_id & 0xFF);
            }
            else if (HC_DEVICE_TYPE_MULTILEVEL_SENSOR == hcdev->dev_type)
            {
                snprintf(hcdev->dev_name, MAX_DEVICE_NAME_SIZE - 1, "%s %d", 
                        hc_map_multi_sensor_txt(hcdev->device.multilevel_sensor.sensor_type),
                        hcdev->dev_id & 0xFF);
            }
            else if (HC_DEVICE_TYPE_METER == hcdev->dev_type)
            {
                snprintf(hcdev->dev_name, MAX_DEVICE_NAME_SIZE - 1, "%s %d", 
                        hc_map_meter_txt(hcdev->device.meter.meter_type), 
                        hcdev->dev_id & 0xFF);
            }
            else if(HC_DEVICE_TYPE_BINARYSWITCH == hcdev->dev_type)
            {
                snprintf(hcdev->dev_name, MAX_DEVICE_NAME_SIZE - 1, "%s %d", 
                        hc_map_binary_switch_txt(hcdev->device.binaryswitch.type), 
                        hcdev->dev_id & 0xFF);
            }
            else
            {
                snprintf(hcdev->dev_name, MAX_DEVICE_NAME_SIZE - 1, "%s %d", 
                        hc_map_hcdev_txt(hcdev->dev_type),
                        hcdev->dev_id & 0xFF);
            }
        }
    }

    /*
     * When GET success, do not reply HC_EVENT_RESP_DEVICE_GET_SUCCESS, use
     * HC_EVENT_STATUS_DEVICE_STATUS_CHANGED instead.
     */
    if (HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == msg->head.type ||
            HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == msg->head.type ||
            HC_EVENT_RESP_DEVICE_SET_SUCCESS == msg->head.type ||
            //HC_EVENT_RESP_DEVICE_GET_SUCCESS == pMsg->head.type ||
            HC_EVENT_STATUS_DEVICE_STATUS_CHANGED == msg->head.type ||
            HC_EVENT_RESP_DEVICE_CONNECTED == msg->head.type ||
            HC_EVENT_RESP_DEVICE_DISCONNECTED == msg->head.type)
    {
        if (g_fw_upgrade_flag == 1)
        {
            DEBUG_INFO("FW upgrading, ignore msg %s[%d] appid[%d] devid[%x]", 
                    hc_map_msg_txt(msg->head.type), msg->head.type, msg->head.appid, msg->head.dev_id);   
            return 0;
        }
    
        update_mid(msg, devinfo);

        ret = update_device_in_db(hcdev, devinfo);
        
        if (HC_DB_ACT_FAIL == ret)
        {
            DEBUG_ERROR("Update DB id:%08X, phy_id:%d, dev_type:%d failed, ret:%d.\n",
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
                    if (hcdev->dev_type != HC_DEVICE_TYPE_SIREN)
                    {
                        DEBUG_INFO("Status changed but DB no change, just return.\n");
                        return 0;
                    }
                }
            }
        }

        msg->head.dst = APPLICATION;
    }

    if (HC_EVENT_RESP_DEVICE_DEFAULT_SUCCESS == msg->head.type)
    {
        memset(g_mid_device_info, 0, sizeof(g_mid_device_info));
    }

    ret = msg_send(msg);
    if (ret <= 0)
    {
        DEBUG_ERROR("Call msg_send failed, ret is %d.\n", ret);
        return -1;
    }

    return 0;
}

static int send_cfg_to_app(HC_MSG_S *msg, ZW_DEVICE_CONFIGURATION *zwcfg)
{
    int ret = 0;
    char *str = NULL;
    HC_DEVICE_INFO_S *hcdev = NULL;

    if (msg == NULL)
        return -1;

    msg->head.src = DAEMON_ZWAVE;
    msg->head.dst = APPLICATION;

    hcdev = &msg->body.device_info;
    hcdev->event_type = msg->head.type;
    hcdev->network_type = HC_NETWORK_TYPE_ZWAVE;


    if (zwcfg)
    {
        msg->head.dev_id = zwcfg->deviceID;

        hcdev->dev_id = zwcfg->deviceID;


        if (zwcfg->type == ZW_DEVICE_DOORLOCK)
        {
            hcdev->dev_type = HC_DEVICE_TYPE_DOORLOCK_CONFIG;
            hcdev->device.doorlock_config.operationType = zwcfg->configuration.doorlock_configuration.operationType;
            hcdev->device.doorlock_config.doorHandlesMode = zwcfg->configuration.doorlock_configuration.doorHandlesMode;
            hcdev->device.doorlock_config.lockTimeoutMinutes = zwcfg->configuration.doorlock_configuration.lockTimeoutMinutes;
            hcdev->device.doorlock_config.lockTimeoutSeconds = zwcfg->configuration.doorlock_configuration.lockTimeoutSeconds;
        }
        else if (zwcfg->type == ZW_DEVICE_MULTILEVEL_SENSOR)
        {
            hcdev->dev_type = HC_DEVICE_TYPE_HSM100_CONFIG;
            hcdev->device.hsm100_config.parameterNumber = zwcfg->configuration.HSM100_configuration.parameterNumber;
            hcdev->device.hsm100_config.bDefault = zwcfg->configuration.HSM100_configuration.bDefault;
            hcdev->device.hsm100_config.value = zwcfg->configuration.HSM100_configuration.value;
        }
        else
        {
            DEBUG_ERROR("Unsupported ZW Config type %d.\n", zwcfg->type);
        }


    }

    ret = msg_send(msg);
    if (ret <= 0)
    {
        DEBUG_ERROR("Call msg_send failed, ret is %d.\n", ret);
        return -1;
    }

    return 0;
}

static int filter_msg(HC_MSG_S *pMsg)
{
    if (pMsg == NULL)
    {
        return -1;
    }

    if (pMsg->head.type == HC_EVENT_EXT_IP_CHANGED ||
            pMsg->head.type == HC_EVENT_WLAN_CONFIGURE_CHANGED ||
            pMsg->head.type == HC_EVENT_FACTORY_DEFAULT)
    {
        DEBUG_INFO("Filter msg %s(%d), appid %d.\n",
                   hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.appid);
        return 1;
    }

    return 0;
}

int compare_hcdev(HC_DEVICE_INFO_S *dest, HC_DEVICE_INFO_S *src)
{
    if (dest == NULL || src == NULL)
    {
        return -1;
    }

    switch (src->dev_type)
    {
        case HC_DEVICE_TYPE_BINARYSWITCH:
            if (dest->device.binaryswitch.value != src->device.binaryswitch.value)
                return -1;
            break;
        case HC_DEVICE_TYPE_DIMMER:
            if (dest->device.dimmer.value != src->device.dimmer.value)
                return -1;
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            if (dest->device.curtain.value != src->device.curtain.value)
                return -1;
            break;
        case HC_DEVICE_TYPE_BINARY_SENSOR:
            if (dest->device.binary_sensor.value != src->device.binary_sensor.value)
                return -1;
            break;
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            if (dest->device.multilevel_sensor.scale != src->device.multilevel_sensor.scale)
                return -1;
            if (0 == double_equal(dest->device.multilevel_sensor.value, src->device.multilevel_sensor.value))
                return -1;
            break;
        case HC_DEVICE_TYPE_BATTERY:
            if (dest->device.battery.battery_level != src->device.battery.battery_level)
                return -1;
            if (dest->device.battery.interval_time != src->device.battery.interval_time)
                return -1;
            break;
        case HC_DEVICE_TYPE_DOORLOCK:
            if (dest->device.doorlock.doorLockMode != src->device.doorlock.doorLockMode)
                return -1;
            break;
        case HC_DEVICE_TYPE_THERMOSTAT: 
            if (dest->device.thermostat.mode != src->device.thermostat.mode)
                return -1;
            if (0 == double_equal(dest->device.thermostat.value, src->device.thermostat.value))
                return -1;
            if (0 == double_equal(dest->device.thermostat.heat_value,src->device.thermostat.heat_value))
                return -1;
            if (0 == double_equal(dest->device.thermostat.cool_value, src->device.thermostat.cool_value))
                return -1;
            if (0 == double_equal(dest->device.thermostat.energe_save_heat_value, src->device.thermostat.energe_save_heat_value))
                return -1;
            if (0 == double_equal(dest->device.thermostat.energe_save_cool_value, src->device.thermostat.energe_save_cool_value))
                return -1;
            break;
        case HC_DEVICE_TYPE_METER:
            if (dest->device.meter.scale != src->device.meter.scale)
                return -1;
            if (dest->device.meter.rate_type != src->device.meter.rate_type)
                return -1;
            if (0 == double_equal(dest->device.meter.value, src->device.meter.value))
                return -1;
            if (dest->device.meter.delta_time != src->device.meter.delta_time)
                return -1;
            if (0 == double_equal(dest->device.meter.previous_value, src->device.meter.previous_value))
                return -1;
            break;
        case HC_DEVICE_TYPE_KEYFOB:
            DEBUG_INFO("Keyfob dest:%s - src:%s", dest->device.keyfob.item_word, src->device.keyfob.item_word);
            return -1;
            break;
        case HC_DEVICE_TYPE_SIREN:
            DEBUG_INFO("Siren dest:%d - src:%d", dest->device.siren.warning_mode, src->device.siren.warning_mode);
            if (dest->device.siren.warning_mode != src->device.siren.warning_mode)
                return -1;
            break;
            
        default:
            DEBUG_ERROR("Unsupported HC Device type %d.\n", src->dev_type);
            return -1;
    }

    return 0;
}

int convert_hcdev_to_zwdev(HC_DEVICE_INFO_S *hcdev, ZW_DEVICE_INFO *zwdev)
{
    int tmp_type = 0;

    if (zwdev == NULL || hcdev == NULL)
    {
        return -1;
    }

    //zwdev->id = hcdev->dev_id;
    zwdev->id = GLOBAL_ID_TO_DEV_ID(hcdev->dev_id);
    zwdev->phy_id = hcdev->phy_id;

    DEBUG_INFO("Global ID %08X, Dev ID %x, Phy ID %x \n",
               hcdev->dev_id, zwdev->id, hcdev->phy_id);

    if (hcdev->dev_type == 0)
    {
        return 0;
    }

    switch (hcdev->dev_type)
    {
        case HC_DEVICE_TYPE_BINARYSWITCH:
            zwdev->type = ZW_DEVICE_BINARYSWITCH;
            tmp_type = map_binary_switch_zwtype(hcdev->device.binaryswitch.type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported HC Binary Switch type %d.\n", hcdev->device.binaryswitch.type);
                tmp_type = 0;
            }
            zwdev->status.binaryswitch_status.value = hcdev->device.binaryswitch.value;
            zwdev->status.binaryswitch_status.type = tmp_type;
            break;
        case HC_DEVICE_TYPE_DIMMER:
            zwdev->type = ZW_DEVICE_DIMMER;
            zwdev->status.dimmer_status.value = hcdev->device.dimmer.value;
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            zwdev->type = ZW_DEVICE_CURTAIN;
            zwdev->status.curtain_status.value = hcdev->device.curtain.value;
            break;
        case HC_DEVICE_TYPE_BINARY_SENSOR:
            zwdev->type = ZW_DEVICE_BINARYSENSOR;
            tmp_type = map_bin_sensor_zwtype(hcdev->device.binary_sensor.sensor_type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported HC Binary Sensor type %d.\n", hcdev->device.binary_sensor.sensor_type);
                tmp_type = 0;
            }
            zwdev->status.binarysensor_status.sensor_type = tmp_type;
            zwdev->status.binarysensor_status.value = hcdev->device.binary_sensor.value;
            break;
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            zwdev->type = ZW_DEVICE_MULTILEVEL_SENSOR;
            tmp_type = map_multi_sensor_zwtype(hcdev->device.multilevel_sensor.sensor_type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported HC Multilevel Sensor type %d.\n", hcdev->device.multilevel_sensor.sensor_type);
                tmp_type = 0;
            }
            zwdev->status.multilevelsensor_status.sensor_type = tmp_type;
            zwdev->status.multilevelsensor_status.scale = hcdev->device.multilevel_sensor.scale;
            zwdev->status.multilevelsensor_status.value = hcdev->device.multilevel_sensor.value;
            break;
        case HC_DEVICE_TYPE_BATTERY:
            zwdev->type = ZW_DEVICE_BATTERY;
            zwdev->status.battery_status.battery_level = hcdev->device.battery.battery_level;
            zwdev->status.battery_status.interval_time = hcdev->device.battery.interval_time;
            break;
        case HC_DEVICE_TYPE_DOORLOCK:
            zwdev->type = ZW_DEVICE_DOORLOCK;
            zwdev->status.doorlock_status.doorLockMode = hcdev->device.doorlock.doorLockMode;

            break;
        case HC_DEVICE_TYPE_DOORLOCK_CONFIG:
            zwdev->type = ZW_DEVICE_DOORLOCK;
            zwdev->status.doorlock_status.doorLockMode = hcdev->device.doorlock.doorLockMode;

            break;
        case HC_DEVICE_TYPE_HSM100_CONFIG:
            zwdev->type = ZW_DEVICE_DOORLOCK;
            zwdev->status.doorlock_status.doorLockMode = hcdev->device.doorlock.doorLockMode;

            break;
        case HC_DEVICE_TYPE_THERMOSTAT:
            zwdev->type = ZW_DEVICE_THERMOSTAT;
            zwdev->status.thermostat_status.mode = hcdev->device.thermostat.mode;
            zwdev->status.thermostat_status.value = hcdev->device.thermostat.value;
            zwdev->status.thermostat_status.heat_value = hcdev->device.thermostat.heat_value;
            zwdev->status.thermostat_status.cool_value = hcdev->device.thermostat.cool_value;
            zwdev->status.thermostat_status.energe_save_heat_value = hcdev->device.thermostat.energe_save_heat_value;
            zwdev->status.thermostat_status.energe_save_cool_value = hcdev->device.thermostat.energe_save_cool_value;
            break;
        case HC_DEVICE_TYPE_METER:
            zwdev->type = ZW_DEVICE_METER;
            tmp_type = map_meter_zwtype(hcdev->device.meter.meter_type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported HC Meter type %d.\n", hcdev->device.meter.meter_type);
                tmp_type = 0;
            }
            zwdev->status.meter_status.meter_type = tmp_type;
            zwdev->status.meter_status.scale = hcdev->device.meter.scale;
            zwdev->status.meter_status.rate_type = hcdev->device.meter.rate_type;
            zwdev->status.meter_status.value = hcdev->device.meter.value;
            zwdev->status.meter_status.delta_time = hcdev->device.meter.delta_time;
            zwdev->status.meter_status.previous_value = hcdev->device.meter.previous_value;
            break;
        case HC_DEVICE_TYPE_KEYFOB:
            zwdev->type = ZW_DEVICE_KEYFOB;
            if (0 == strcasecmp(hcdev->device.keyfob.item_word, "Away"))
                zwdev->status.keyfob_status.value = 0xff;
            else if (0 == strcasecmp(hcdev->device.keyfob.item_word, "Disarm"))
                zwdev->status.keyfob_status.value = 0x0;
            else
                DEBUG_ERROR("Unknown HC Keyfob item_word %d.\n", hcdev->device.keyfob.item_word);
            break;
        case HC_DEVICE_TYPE_SIREN:
            zwdev->type = ZW_DEVICE_SIREN;
            zwdev->status.siren_status.value = hcdev->device.siren.warning_mode;
            break;
            
        default:
            DEBUG_ERROR("Unsupported HC Device type %d.\n", hcdev->dev_type);
            dump_mem(hcdev, sizeof(HC_DEVICE_INFO_S));
            return -1;
    }

    return 0;
}

int convert_zwdev_to_hcdev(ZW_DEVICE_INFO *zwdev, HC_DEVICE_INFO_S *hcdev)
{
    int tmp_type = 0;

    if (zwdev == NULL || hcdev == NULL)
    {
        return -1;
    }

    //hcdev->dev_id = zwdev->id;
    hcdev->dev_id = COMBINE_GLOBAL_ID(HC_NETWORK_TYPE_ZWAVE, zwdev->id);
    hcdev->phy_id = zwdev->phy_id;
    hcdev->network_type = HC_NETWORK_TYPE_ZWAVE;

    DEBUG_INFO("Global ID %08X, Dev ID %x, Phy ID %x \n",
               hcdev->dev_id, zwdev->id, hcdev->phy_id);

    /* Protocol do not fill device structure, and HCAPI also
       do not care these info. */
    if (zwdev->type == 0)
    {
        return 0;
    }

    switch (zwdev->type)
    {
        case ZW_DEVICE_BINARYSWITCH:
            hcdev->dev_type = HC_DEVICE_TYPE_BINARYSWITCH;
            hcdev->device.binaryswitch.value = zwdev->status.binaryswitch_status.value;
            hcdev->device.binaryswitch.type = zwdev->status.binaryswitch_status.type;
            break;
        case ZW_DEVICE_DIMMER:
            hcdev->dev_type = HC_DEVICE_TYPE_DIMMER;
            hcdev->device.dimmer.value = zwdev->status.dimmer_status.value;
            break;
        case ZW_DEVICE_CURTAIN:
            hcdev->dev_type = HC_DEVICE_TYPE_CURTAIN;
            hcdev->device.curtain.value = zwdev->status.curtain_status.value;
            break;
        case ZW_DEVICE_BINARYSENSOR:
            hcdev->dev_type = HC_DEVICE_TYPE_BINARY_SENSOR;
            tmp_type = map_bin_sensor_hctype(zwdev->status.binarysensor_status.sensor_type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported ZW Binary Sensor type %d.\n", zwdev->status.binarysensor_status.sensor_type);
                //tmp_type = 0;
            }
            hcdev->device.binary_sensor.sensor_type = tmp_type;
            hcdev->device.binary_sensor.value = zwdev->status.binarysensor_status.value;
            break;
        case ZW_DEVICE_MULTILEVEL_SENSOR:
            hcdev->dev_type = HC_DEVICE_TYPE_MULTILEVEL_SENSOR;
            tmp_type = map_multi_sensor_hctype(zwdev->status.multilevelsensor_status.sensor_type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported ZW Multilevel Sensor type %d.\n", zwdev->status.multilevelsensor_status.sensor_type);
                //tmp_type = 0;
            }
            hcdev->device.multilevel_sensor.sensor_type = tmp_type;
            hcdev->device.multilevel_sensor.scale = zwdev->status.multilevelsensor_status.scale;
            hcdev->device.multilevel_sensor.value = zwdev->status.multilevelsensor_status.value;
            break;

        case ZW_DEVICE_BATTERY:
            hcdev->dev_type = HC_DEVICE_TYPE_BATTERY;
            hcdev->device.battery.battery_level = zwdev->status.battery_status.battery_level;
            hcdev->device.battery.interval_time = zwdev->status.battery_status.interval_time;
            break;
        case ZW_DEVICE_DOORLOCK:
            hcdev->dev_type = HC_DEVICE_TYPE_DOORLOCK;
            hcdev->device.doorlock.doorLockMode = zwdev->status.doorlock_status.doorLockMode;
            hcdev->device.doorlock.doorHandlesMode = zwdev->status.doorlock_status.doorHandlesMode;
            hcdev->device.doorlock.doorCondition = zwdev->status.doorlock_status.doorCondition;
            hcdev->device.doorlock.lockTimeoutMinutes = zwdev->status.doorlock_status.lockTimeoutMinutes;
            hcdev->device.doorlock.lockTimeoutSeconds = zwdev->status.doorlock_status.lockTimeoutSeconds;
            break;
        case ZW_DEVICE_THERMOSTAT:
            hcdev->dev_type = HC_DEVICE_TYPE_THERMOSTAT;
            hcdev->device.thermostat.mode = zwdev->status.thermostat_status.mode;
            hcdev->device.thermostat.value = zwdev->status.thermostat_status.value;
            hcdev->device.thermostat.heat_value = zwdev->status.thermostat_status.heat_value;
            hcdev->device.thermostat.cool_value = zwdev->status.thermostat_status.cool_value;
            hcdev->device.thermostat.energe_save_heat_value = zwdev->status.thermostat_status.energe_save_heat_value;
            hcdev->device.thermostat.energe_save_cool_value = zwdev->status.thermostat_status.energe_save_cool_value;
            break;
        case ZW_DEVICE_METER:
            hcdev->dev_type = HC_DEVICE_TYPE_METER;
            tmp_type = map_meter_hctype(zwdev->status.meter_status.meter_type);
            if (-1 == tmp_type)
            {
                DEBUG_ERROR("Unsupported ZW Meter type %d.\n", zwdev->status.meter_status.meter_type);
                //tmp_type = 0;
            }
            hcdev->device.meter.meter_type = tmp_type;
            hcdev->device.meter.scale = zwdev->status.meter_status.scale;
            hcdev->device.meter.rate_type = zwdev->status.meter_status.rate_type;
            hcdev->device.meter.value = zwdev->status.meter_status.value;
            hcdev->device.meter.delta_time = zwdev->status.meter_status.delta_time;
            hcdev->device.meter.previous_value = zwdev->status.meter_status.previous_value;
            break;
        case ZW_DEVICE_KEYFOB:
            hcdev->dev_type = HC_DEVICE_TYPE_KEYFOB;
            if (zwdev->status.keyfob_status.value == 0xff)
                strcpy(hcdev->device.keyfob.item_word, "Away");
            else if (zwdev->status.keyfob_status.value == 0x0)
                strcpy(hcdev->device.keyfob.item_word, "Disarm");
            else
                DEBUG_ERROR("Unknown ZW Keyfob value %d.\n", zwdev->status.keyfob_status.value);
            break;
        case ZW_DEVICE_SIREN:
            hcdev->dev_type = HC_DEVICE_TYPE_SIREN;
            hcdev->device.siren.warning_mode = zwdev->status.siren_status.value;
            break;
            
        default:
            DEBUG_ERROR("Unsupported ZW Device type %d.\n", zwdev->type);
            dump_mem(zwdev, sizeof(ZW_DEVICE_INFO));
            return -1;
    }

    return 0;
}

static int process_msg(HC_MSG_S *pMsg)
{
    int ret = 0;
    MID_DEVICE_U devinfo;
    HC_DEVICE_INFO_S *hcdev = NULL;
    HC_UPGRADE_INFO_S *upgradeInfo = NULL;
    MID_DEVICE_INFO_S *middev = NULL;
    int idx = 0;
    

    if (pMsg == NULL)
    {
        return -1;
    }

    // when zwave lock success, do not accept any msg except below.
    if (g_fw_upgrade_flag == 1)
    {
        if (pMsg->head.type != HC_EVENT_EXT_FW_UPGRADE_LOCK
                && pMsg->head.type != HC_EVENT_EXT_FW_UPGRADE_UNLOCK
                && pMsg->head.type != HC_EVENT_EXT_MODULE_UPGRADE)
        {
            DEBUG_INFO("Ignore msg %s(%d), appid %d.\n",
                      hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.appid);
            return 0;
        }
    }

    // parse msg.
    DEBUG_INFO("Process msg %s(%d), appid %d.\n",
               hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.appid);

    memset(&devinfo, 0, sizeof(MID_DEVICE_U));

    hcdev = &pMsg->body.device_info;

    if (pMsg->head.type == HC_EVENT_REQ_DEVICE_CFG_GET ||
            pMsg->head.type == HC_EVENT_REQ_DEVICE_CFG_SET)
    {
        devinfo.config.deviceID = hcdev->dev_id;

        if (hcdev->dev_type == HC_DEVICE_TYPE_DOORLOCK_CONFIG)
        {
            devinfo.config.type = ZW_DEVICE_DOORLOCK;

            devinfo.config.configuration.doorlock_configuration.operationType = hcdev->device.doorlock_config.operationType;
            devinfo.config.configuration.doorlock_configuration.doorHandlesMode = hcdev->device.doorlock_config.doorHandlesMode;
            devinfo.config.configuration.doorlock_configuration.lockTimeoutMinutes = hcdev->device.doorlock_config.lockTimeoutMinutes;
            devinfo.config.configuration.doorlock_configuration.lockTimeoutSeconds = hcdev->device.doorlock_config.lockTimeoutSeconds;
        }
        else if (hcdev->dev_type == HC_DEVICE_TYPE_HSM100_CONFIG)
        {
            devinfo.config.type = ZW_DEVICE_MULTILEVEL_SENSOR;

            devinfo.config.configuration.HSM100_configuration.parameterNumber = hcdev->device.hsm100_config.parameterNumber;
            devinfo.config.configuration.HSM100_configuration.bDefault = hcdev->device.hsm100_config.bDefault;
            devinfo.config.configuration.HSM100_configuration.value = hcdev->device.hsm100_config.value;
        }
        else
        {
            DEBUG_ERROR("Unsupported HC Config type %d.\n", hcdev->dev_type);
            return -1;
        }
    }
    else if (pMsg->head.type == HC_EVENT_REQ_ASSOICATION_SET ||
             pMsg->head.type == HC_EVENT_REQ_ASSOICATION_REMOVE)
    {
        devinfo.association.srcPhyId = hcdev->device.association.srcPhyId;
        devinfo.association.dstPhyId = hcdev->device.association.dstPhyId;
    }
    else if (pMsg->head.type == HC_EVENT_REQ_DEVICE_ADD_STOP)
    {
        if (g_zw_controller_mode == ZW_CONTROLLER_MODE_ADDING)
        {
            ret = ZWapi_StopAddNodeToNetwork();
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_ERROR("Call ZWapi_StopAddNodeToNetwork failed, ret=%d.\n", ret);

                msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_STOP_FAILURE, NULL);
            }
            else
            {
                msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_STOP_SUCCESS, NULL);
            }
        }
        else
        {
            DEBUG_ERROR("ZW Controller was not in ADDING mode, mode=%d.\n", g_zw_controller_mode);
            msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_STOP_FAILURE, NULL);
        }

        return 0;
    }
    else if (pMsg->head.type == HC_EVENT_REQ_DEVICE_DELETE_STOP)
    {
        if (g_zw_controller_mode == ZW_CONTROLLER_MODE_DELETING)
        {
            ret = ZWapi_StopRemoveNodeFromNetwork();
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_ERROR("Call ZWapi_StopRemoveNodeFromNetwork failed, ret=%d.\n", ret);

                msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETE_STOP_FAILURE, NULL);
            }
            else
            {
                msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETE_STOP_SUCCESS, NULL);
            }
        }
        else
        {
            DEBUG_ERROR("ZW Controller was not in DELETING mode, mode=%d.\n", g_zw_controller_mode);
            msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETE_STOP_FAILURE, NULL);
        }

        return 0;
    }
    else if (pMsg->head.type == HC_EVENT_REQ_DRIVER_STATUS_QUERY)
    {
        DEBUG_ERROR("ZW Controller mode is %d.\n", g_zw_controller_mode);
        if (g_zw_controller_mode == ZW_CONTROLLER_MODE_ADDING)
        {
            msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_MODE, NULL);
        }
        else if (g_zw_controller_mode == ZW_CONTROLLER_MODE_DELETING)
        {
            msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETE_MODE, NULL);
        }
        else
        {
            msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_NORMAL_MODE, NULL);
        }

        return 0;
    }
    else if (pMsg->head.type == HC_EVNET_EXT_ARMMODE_CHANGED)
    {
        if (pMsg->body.device_info.device.ext_u.armmode.value == HC_DISARM)
        {
            for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
            {
                middev = &g_mid_device_info[idx];
                if (middev->dev_type == ZW_DEVICE_SIREN)
                {
                    devinfo.devinfo.id = middev->dev_id;
                    devinfo.devinfo.phy_id = middev->phy_id;
                    devinfo.devinfo.type = middev->dev_type;
                    devinfo.devinfo.status.siren_status.value = 0x0;

                    DEBUG_INFO("[ARM Set] Add task msgtype:%s(%d), appid:%d, pri:%d.\n",
                            hc_map_msg_txt(HC_EVENT_REQ_DEVICE_SET), HC_EVENT_REQ_DEVICE_SET, 0, TASK_PRIORITY_UI);

                    ret = task_add(HC_EVENT_REQ_DEVICE_SET, 0, TASK_PRIORITY_UI, (void *)&devinfo);
                    if (ret != 0)
                    {
                        DEBUG_ERROR("Add task to queue failed.\n");
                        return -1;
                    }

                    return 0;
                }
            }
        }

        return 0;
    }
    else if (pMsg->head.type == HC_EVENT_EXT_FW_UPGRADE_LOCK)
    {
        if (g_zw_controller_mode == ZW_CONTROLLER_MODE_ADDING
                || g_zw_controller_mode == ZW_CONTROLLER_MODE_DELETING)
        {
            msg_ZWReply(pMsg->head.appid, HC_EVENT_EXT_FW_UPGRADE_LOCK_FAILED, NULL);
            return 0;
        }
        else
        {
            DEBUG_INFO("FW upgrade ZWAVE lock.\n");
            g_fw_upgrade_flag = 1;
            msg_ZWReply(pMsg->head.appid, HC_EVENT_EXT_FW_UPGRADE_LOCK_DONE, NULL);
            return 0;
        }
                
    }
    else if (pMsg->head.type == HC_EVENT_EXT_FW_UPGRADE_UNLOCK)
    {
        DEBUG_INFO("FW upgrade ZWAVE unlock.\n");
        g_fw_upgrade_flag = 0;
        return 0;
    }
    else if (pMsg->head.type == HC_EVENT_EXT_MODULE_UPGRADE)
    {
        DEBUG_INFO("ZWAVE module upgrade.\n");

        upgradeInfo = &pMsg->body.upgrade_info;
        strncpy(&g_image_file_path, &upgradeInfo->image_file_path, sizeof(g_image_file_path));
        g_image_file_path[255] = '\0';
    }
    else
    {
        if (pMsg->head.type == HC_EVENT_REQ_DEVICE_ADD || 
                pMsg->head.type == HC_EVENT_REQ_DEVICE_DELETE)
        {
            if (g_zw_controller_mode == ZW_CONTROLLER_MODE_ADDING)
            {
                msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_ADD_MODE, NULL);
                return 0;
            }
            else if (g_zw_controller_mode == ZW_CONTROLLER_MODE_DELETING)
            {
                msg_ZWReply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETE_MODE, NULL);
                return 0;
            }
            
        }
    
        memset(&devinfo, 0, sizeof(ZW_DEVICE_INFO));
        ret = convert_hcdev_to_zwdev(&pMsg->body.device_info, &devinfo.devinfo);
        if (ret != 0)
        {
            DEBUG_ERROR("Convert HC device %s(%d)to ZW device info failed.\n",
                        hc_map_hcdev_txt(pMsg->head.dev_type), pMsg->head.dev_type);
            return -1;
        }
    }

    // add task.
    DEBUG_INFO("Add task msgtype:%s(%d), appid:%d, pri:%d.\n",
               hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.appid, TASK_PRIORITY_UI);

    ret = task_add(pMsg->head.type, pMsg->head.appid, TASK_PRIORITY_UI, (void *)&devinfo);
    if (ret != 0)
    {
        DEBUG_ERROR("Add task to queue failed.\n");
        return -1;
    }

    return 0;
}

int msg_init(void)
{
    // invoke library MSG provided APIs to create socket.
    g_msg_fd = hc_client_msg_init(DAEMON_ZWAVE, SOCKET_DEFAULT);
    if (g_msg_fd == -1)
    {
        DEBUG_ERROR("Call hc_client_msg_init failed, errno is '%d'.\n", errno);
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
    int i = 0;
    HC_MSG_S *hcmsg = NULL;

    while (keep_looping)
    {
        tv.tv_sec = 1;
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
            DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is %d.\n", errno);
            hc_msg_free(hcmsg);
            //return -1;
            msg_uninit();
            msg_init();
            DEBUG_ERROR("new g_msg_fd = '%d'.\n", g_msg_fd);
            continue;
        }

        if (0 == filter_msg(hcmsg))
        {
            process_msg(hcmsg);
        }

        hc_msg_free(hcmsg);

    }

    return 0;
}

void msg_ZWReportCB(ZW_DEVICE_INFO *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    if (devinfo == NULL)
        return;

    show_device_information(__FUNCTION__, __LINE__, devinfo);

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;

    ret = send_msg_to_app(&msg, devinfo);
    if (ret != 0)
    {
        DEBUG_ERROR("Send msg to app failed.\n");
    }

    DEBUG_INFO("Return from msg_ZWReportCB \n");
}

void msg_ZWEventCB(ZW_EVENT_REPORT *event)
{
    int idx = 0;
    MID_DEVICE_INFO_S *middev = NULL;
    MID_DEVICE_U dev_u;

    if (event == NULL)
        return;

    if (event->phy_id <= 0)
    {
        DEBUG_ERROR("eventCB wrong phy_id:%x\n", event->phy_id);
        return;
    }

    if (event->type == ZW_EVENT_TYPE_NODEINFO)
    {
        for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
        {
            middev = &g_mid_device_info[idx];

            if (event->phy_id > 0 && event->phy_id == middev->phy_id)
            {
                memset(&dev_u, 0, sizeof(MID_DEVICE_U));
                dev_u.devinfo.id = middev->dev_id;
                dev_u.devinfo.phy_id = middev->phy_id;
                dev_u.devinfo.type = middev->dev_type;

                DEBUG_INFO("eventCB NODEINFO, add task DEVICE_GET id:%x, phy_id:%x, type:%d.\n",
                           dev_u.devinfo.id, dev_u.devinfo.phy_id, dev_u.devinfo.type);

                task_add(HC_EVENT_REQ_DEVICE_GET, 0, TASK_PRIORITY_PROTOCOL, &dev_u);
            }
        }
    }
    else if (event->type == ZW_EVENT_TYPE_WAKEUP)
    {
        // 1. device connected.
        for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
        {
            middev = &g_mid_device_info[idx];

            if (event->phy_id > 0 && event->phy_id == middev->phy_id)
            {
                memset(&dev_u, 0, sizeof(MID_DEVICE_U));
                dev_u.devinfo.id = middev->dev_id;
                dev_u.devinfo.phy_id = middev->phy_id;
                dev_u.devinfo.type = middev->dev_type;
                msg_ZWReply(0, HC_EVENT_RESP_DEVICE_CONNECTED, &dev_u.devinfo);
            }
        }

        // 2. get device info.
        for (idx = 0; idx < MAX_MID_DEVICE_NUMBER; idx++)
        {
            middev = &g_mid_device_info[idx];

            if (event->phy_id > 0 && event->phy_id == middev->phy_id)
            {
                memset(&dev_u, 0, sizeof(MID_DEVICE_U));
                dev_u.devinfo.id = middev->dev_id;
                dev_u.devinfo.phy_id = middev->phy_id;
                dev_u.devinfo.type = middev->dev_type;

                DEBUG_INFO("eventCB WAKEUP, add task DEVICE_GET id:%x, phy_id:%x, type:%d.\n",
                           dev_u.devinfo.id, dev_u.devinfo.phy_id, dev_u.devinfo.type);

                task_add(HC_EVENT_REQ_DEVICE_GET, 0, TASK_PRIORITY_PROTOCOL, &dev_u);
            }
        }
    }
    else
    {
        DEBUG_ERROR("eventCB Unknown event type:%d, phy_id:%x \n", event->type, event->phy_id);
    }

}

void msg_ZWReply(int appid, HC_EVENT_TYPE_E type, ZW_DEVICE_INFO *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    if (devinfo)
    {
        DEBUG_INFO("Reply msg %s(%d) to appid %d with devid %d.\n",
                   hc_map_msg_txt(type), type, appid, devinfo->id);
    }
    else
    {
        DEBUG_INFO("Reply msg %s(%d) to appid %d.\n",
                   hc_map_msg_txt(type), type, appid);
    }

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = type;
    msg.head.appid = appid;

    ret = send_msg_to_app(&msg, devinfo);
    if (ret != 0)
    {
        DEBUG_ERROR("Send msg to app failed.\n");
    }

}

void msg_ZWReplyCfg(int appid, HC_EVENT_TYPE_E type, ZW_DEVICE_CONFIGURATION *zwcfg)
{
    int ret = 0;
    HC_MSG_S msg;

    if (zwcfg)
    {
        DEBUG_INFO("ReplyCfg msg %s(%d) to appid %d with devid %d.\n",
                   hc_map_msg_txt(type), type, appid, zwcfg->deviceID);
    }
    else
    {
        DEBUG_INFO("ReplyCfg msg %s(%d) to appid %d.\n",
                   hc_map_msg_txt(type), type, appid);
    }

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = type;
    msg.head.appid = appid;

    ret = send_cfg_to_app(&msg, zwcfg);
    if (ret != 0)
    {
        DEBUG_ERROR("Send msg to app failed.\n");
    }

}

void msg_ZWAddSuccess(ZW_DEVICE_INFO *devinfo)
{
    int ret = 0;
    ZW_DEVICE_INFO zwdev;
    MID_DEVICE_INFO_S *middev = NULL;
    int retry_count = 0;


    if (devinfo == NULL)
        return;

    show_device_information(__FUNCTION__, __LINE__, devinfo);

    /* here ignore the failure case, if get failure the value was not changed. */
    if (devinfo->type == ZW_DEVICE_BINARYSWITCH
            || devinfo->type == ZW_DEVICE_DOORLOCK
            || devinfo->type == ZW_DEVICE_THERMOSTAT
            || devinfo->type == ZW_DEVICE_BATTERY
            /*|| devinfo->type == ZW_DEVICE_SIREN*/)
    {
        if (devinfo->type == ZW_DEVICE_DOORLOCK)
            devinfo->status.doorlock_status.doorLockMode = DOORLOCK_LOCKMODE_SECURED; /* use the YALE doorlock initial status locked */
        else
            ZWapi_GetDeviceStatus(devinfo);
    }
    else if (devinfo->type == ZW_DEVICE_DIMMER
             || devinfo->type == ZW_DEVICE_CURTAIN
             || devinfo->type == ZW_DEVICE_METER)
    {
        if (devinfo->type == ZW_DEVICE_METER)
            devinfo->status.meter_status.scale = METER_ELECTRIC_SCALE_POWER;
        ZWapi_GetDeviceStatus(devinfo);
        memcpy(&zwdev, devinfo, sizeof(ZW_DEVICE_INFO));
        while (retry_count++ < 10)
        {
            sleep(1); // 1000ms

            ZWapi_GetDeviceStatus(devinfo);

            ret = compare_zwdev(&zwdev, devinfo);
            if (ret == 0)
            {
                break;
            }
            memcpy(&zwdev, devinfo, sizeof(ZW_DEVICE_INFO));
        }
    }
    else
    {
        // have get the device status in driver
    }
	zwave_has_device_added++;

    msg_ZWReply(0, HC_EVENT_RESP_DEVICE_ADDED_SUCCESS, devinfo);

}

void msg_ZWAddFail(ZW_STATUS status)
{

    DEBUG_INFO("Add device failed, reason is %d.\n", status);
    msg_ZWReply(0, HC_EVENT_RESP_DEVICE_ADDED_FAILURE, NULL);
}


void msg_ZWRemoveSuccess(ZW_DEVICE_INFO *devinfo)
{
    int ret = 0;
    MID_DEVICE_INFO_S *middev = NULL;

    if (devinfo == NULL)
        return;

    DEBUG_INFO("ID:%x, PhyID:%x, Type:%u \n", devinfo->id, devinfo->phy_id, devinfo->type);

    if (0 == devinfo->id)
    {
        msg_ZWReply(0, HC_EVENT_RESP_DEVICE_HAS_BEEN_REMOVED, devinfo);
    }
    else
    {
        msg_ZWReply(0, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, devinfo);
    }

}

void msg_ZWRemoveFail(ZW_STATUS status)
{

    DEBUG_INFO("Delete device failed, reason is %d.\n", status);
    msg_ZWReply(0, HC_EVENT_RESP_DEVICE_DELETED_FAILURE, NULL);
}


