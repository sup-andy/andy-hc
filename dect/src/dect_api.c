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
*   File   : dect_api.c
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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include "dect_api.h"
#include "ctrl_common_lib.h"
#include "dect_device.h"
#include "dect_task.h"
#include "dect_msg.h"
#include "dect_util.h"
#include "tcm_cmbs.h"


/* bit 25 - 32: global type,  bit 24: handset, ule type, bit 0 - 22: cmbs type */
#define GET_DECT_DEVICE_TYPE(id) ((id >> 16) & 0x0080)
#define DECT_DEVICE_ID_TO_CMBS(id)      (id & 0x007FFFFF)
#define CMBS_DEVICE_ID_DECT(type,id)      ((HC_NETWORK_TYPE_ULE << 24) | (type << 23) | id)

static pthread_t g_hanDeviceKeepAliveThreadId;
static pthread_t g_hanDeviceReportThreadId;
static pthread_t g_dectTask;

extern int g_dect_controller_mode;

DECT_STATUS dect_daemon_init(void)
{
    if (0 == tcm_cmbs_init())
    {
        return DECT_STATUS_OK;
    }
    else
    {
        return DECT_STATUS_FAIL;
    }

}

DECT_STATUS dect_callline_init(void)
{
    dect_voice_init();
    return DECT_STATUS_OK;
}

DECT_STATUS dect_ule_device_mgr_init(void)
{
    DECT_STATUS ret = 0;
    ret = tcm_han_mgr_start();
    tcm_HanRegisterInterface();

    return ret;

}

DECT_STATUS dect_thread_int(void)
{
    int ret = 0;
    // startup keepalive thread.
    ret = start_thread(&g_hanDeviceKeepAliveThreadId, 0, tcm_DeviceKeepAliveThread, NULL);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Start thread tcm_DeviceKeepAliveThread failed.\n");
        return DECT_STATUS_FAIL;
    }

    // startup dect report thread.
    ret = start_thread(&g_hanDeviceReportThreadId, 0, tcm_DeviceReportThread, NULL);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Start thread tcm_DeviceReportThread failed.\n");
        return DECT_STATUS_FAIL;
    }

    // startup task thread.
    ret = start_thread(&g_dectTask, 0, task_handle_thread, NULL);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Start thread task_handle_thread failed.\n");
        return DECT_STATUS_FAIL;
    }

    return DECT_STATUS_OK;

}

DECT_STATUS dect_device_set(DECT_HAN_DEVICE_INF *device_info)
{
    DECT_STATUS ret = DECT_STATUS_OK;
    DECT_HAN_DEVICE_INF dev;
    int i = 0;

    memset(&dev, 0, sizeof(dev));

    switch (device_info->type)
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
        {
            break;
        }
        case DECT_TYPE_DETECTOR:
        case DECT_TYPE_DOOR_OPEN_CLOSE_DETECTOR:
        case DECT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR:
        case DECT_TYPE_MOTION_DETECTOR:
        case DECT_TYPE_SMOKE_DETECTOR:
        case DECT_TYPE_GAS_DETECTOR:
        case DECT_TYPE_FLOOD_DETECTOR:
        case DECT_TYPE_GLASS_BREAK_DETECTOR:
        case DECT_TYPE_VIBRATION_DETECTOR:
        {
            unsigned int id = DECT_DEVICE_ID_TO_CMBS(device_info->id);
            if (0 == tcm_dect_device_on_off(device_info->device.binary_sensor.value, id))
            {
                ret = DECT_STATUS_OK;
                for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
                {
                    if (device_info->id == g_Device_info.device[i].id)
                    {
                        memcpy(&dev, &g_Device_info.device[i], sizeof(DECT_HAN_DEVICE_INF));
                        dev.device.binary_sensor.value = device_info->device.binary_sensor.value;
                        memcpy(device_info, &dev, sizeof(DECT_HAN_DEVICE_INF));
                        g_Device_info.device[i].device.binary_sensor.value = dev.device.binary_sensor.value;
                        break;
                    }
                }
            }
            else
            {
                ret = DECT_STATUS_FAIL;
            }

            break;
        }
        case DECT_TYPE_HANDSET:
        {
            if (device_info->device.handset.value)
            {
                ret = dect_handset_ring(device_info->id);
            }
            else
            {
                ret = dect_handset_stop_ring();
            }
            break;
        }
        default:
        {
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Unsupported Dect Device type %d.\n", device_info->type);
            return DECT_STATUS_FAIL;
        }
    }

    return ret;
}

DECT_STATUS dect_device_add(unsigned int timeout)
{
    DECT_STATUS ret = DECT_STATUS_OK;
    if (0 == tcm_add_device(timeout))
    {
        ret = DECT_STATUS_OK;
    }
    else
    {
        ret = DECT_STATUS_FAIL;
    }
    return ret;
}
DECT_STATUS dect_device_delete(unsigned int id)
{
    DECT_STATUS ret = DECT_STATUS_OK;
    unsigned int dev_id = DECT_DEVICE_ID_TO_CMBS(id);

    if (GET_DECT_DEVICE_TYPE(id))
    {
        char hs[16] = {0};
        sprintf(hs, "%u", dev_id);
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Handset delete [%u]...\n", dev_id);
        tcm_handset_delete(hs);
    }
    else
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Ule delete [%u]...\n", dev_id);
        if (0 == tcm_delete_device(dev_id))
        {
            ret = DECT_STATUS_OK;
        }
        else
        {
            ret = DECT_STATUS_FAIL;
        }
    }
    return ret;
}

DECT_STATUS dect_stop_register(void)
{
    if (tcm_close_register() == 0)
    {
        return DECT_STATUS_OK;
    }
    else
    {
        return DECT_STATUS_FAIL;
    }

}

static char* dect_get_register_status_str(TCM_DEVICE_REGISTER_STUTAS_E result)
{
    switch (result)
    {
        case TCM_DEVICE_REGISTER_SUCCESS:
            return "register success";
        case TCM_DEVICE_REGISTER_FAILURE:
            return "register failture";
        case TCM_DEVICE_REGISTER_UNKNOW:
            return "register result unknow";
        default:
            return "nothing";
    }
}
unsigned int dect_convert_cmbs_id_to_dect(unsigned int id, bool isHandset)
{
    if (id)
    {
        return CMBS_DEVICE_ID_DECT(isHandset, id);
    }
    return 0;
}
void dect_device_register_complete(TCM_DEVICE_REGISTER_STUTAS_E result, unsigned int id)
{
    if (g_dect_controller_mode != DECT_CONTROLLER_MODE_ADDING)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT control isnot in ADDING mode, ignore it...\n");
        return;
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT register complete, result: %s,id [%d]\n", dect_get_register_status_str(result), id);

    if (TCM_DEVICE_REGISTER_FAILURE != result && id > 0)
    {
        DECT_HAN_DEVICE_INF device;
        DECT_HAN_DEVICES list;
        int i = 0, ret = 0;;

        memset(&device, 0, sizeof(device));
        memset(&list, 0, sizeof(list));

        tcm_close_register();

        device.id = id;
        strcpy(device.name, "");
        strcpy(device.location, "");
        device.keepalive = time(NULL);
        device.connection = DECT_DEVICE_STATUS_CONNECTION;

        /* get device type and ipei info */
        tcm_GetDeviceList(&list);
        for (i = 0; i < list.device_num; i++)
        {
            if (list.device[i].id == device.id)
            {
                device.type = list.device[i].type;
                strcpy(device.ipei, list.device[i].ipei);
                if (0 == strcmp(device.name, "") || 0 == strcmp(device.name, "Unspecified"))
                {
                    strcpy(device.name, tcm_GetDeviceName(device.type));
                }
                break;
            }
        }
        if (i >= list.device_num)
        {
            ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Invalide device id [%d],cannot find it in realtime devices table\n", device.id);
            ret = 1;

            dect_device_delete(DECT_DEVICE_ID_TO_CMBS(id));
            msg_DECTAddFail(0);
        }
        else
        {
            /* update device */
            for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
            {
                if (device.id == g_Device_info.device[i].id)
                {
                    /* same id, replace old now */
                    //msg_DECTDeleteDeviceFromDb(&device);
                    memcpy(&g_Device_info.device[i], &device, sizeof(DECT_HAN_DEVICE_INF));
                    g_Device_info.index += 1;
                    ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Error:Same device id [%d]\n", device.id);
                    break;
                }

            }
            /* insert a new device */
            if (i >= DECT_CMBS_DEVICES_MAX)
            {
                for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
                {
                    if (0 == g_Device_info.device[i].id)
                    {
                        /* new device */
                        memcpy(&g_Device_info.device[i], &device, sizeof(DECT_HAN_DEVICE_INF));
                        g_Device_info.device_num += 1;
                        g_Device_info.index += 1;
                        break;
                    }
                }
            }
            if (i >= DECT_CMBS_DEVICES_MAX)
            {
                ret = 1;
                ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Dect only support [%d] devices\n", DECT_CMBS_DEVICES_MAX);
                msg_DECTAddFail(0);
            }
            else
            {
                msg_DECTAddSuccess(&device);
            }
        }


    }
    else
    {
        msg_DECTAddFail(0);
    }
}

void dect_handset_register_complete(TCM_DEVICE_REGISTER_STUTAS_E result, DECT_HAN_DEVICE_INF *device)
{
    int i = 0;
    if (g_dect_controller_mode != DECT_CONTROLLER_MODE_ADDING)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT control isnot in ADDING mode, ignore it...\n");
        return;
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DECT register complete, result: %s\n", dect_get_register_status_str(result));

    if (TCM_DEVICE_REGISTER_FAILURE != result)
    {
        /* update device */
        for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
        {
            if (device->id == g_Device_info.device[i].id)
            {
                /* same id, replace old now */
                //msg_DECTDeleteDeviceFromDb(&device);
                memcpy(&g_Device_info.device[i], device, sizeof(DECT_HAN_DEVICE_INF));
                g_Device_info.index += 1;
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Error:Same device id [%0x]\n", device->id);
                break;
            }

        }
        /* insert a new device */
        if (i >= DECT_CMBS_DEVICES_MAX)
        {
            for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
            {
                if (0 == g_Device_info.device[i].id)
                {
                    /* new device */
                    memcpy(&g_Device_info.device[i], device, sizeof(DECT_HAN_DEVICE_INF));
                    g_Device_info.device_num += 1;
                    g_Device_info.index += 1;
                    break;
                }
            }
        }
        if (i >= DECT_CMBS_DEVICES_MAX)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Dect only support [%d] devices\n", DECT_CMBS_DEVICES_MAX);
            msg_DECTAddFail(0);
        }
        else
        {
            msg_DECTAddSuccess(device);
        }

    }
    else
    {
        msg_DECTAddFail(0);
    }
}

static DECT_STATUS  dect_handle_upgrade_before()
{
    // The firmware update starts only if no cordless connection is active.
    // So, we need stop all service
    tcm_HanRegisterUnterface();
    printf("Stop DeviceReportThread.\n");
    if (stop_thread(g_hanDeviceReportThreadId) != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "stop device report thread Error...");
    }
    if (stop_thread(g_hanDeviceKeepAliveThreadId) != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "stop keepalive thread Error...");
    }

    return DECT_STATUS_OK;

}
static DECT_STATUS  dect_handle_upgrade_after()
{
    int ret = 0;
    // start all service
    dect_ule_device_mgr_init();
    //tcm_createDeviceReportThread();
    //tcm_createDeviceKeepAliveThread();
    ret = start_thread(&g_hanDeviceKeepAliveThreadId, 0, tcm_DeviceKeepAliveThread, NULL);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Start thread tcm_DeviceKeepAliveThread failed.\n");
    }

    // startup task thread.
    ret = start_thread(&g_hanDeviceReportThreadId, 0, tcm_DeviceReportThread, NULL);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Start thread tcm_DeviceReportThread failed.\n");
    }

    return DECT_STATUS_OK;
}

DECT_STATUS dect_handle_upgrade(char *fw_file_path)
{
    DECT_STATUS ret = 0;

    // Stop All Service
    dect_handle_upgrade_before();
    // wait 0.2 seconds
    usleep(200000);

    ret = tcm_device_image_upgrade(fw_file_path);

    dect_handle_upgrade_after();

    return ret;
}


void dect_device_data_init()
{
    DECT_HAN_DEVICES handset;
    int i = 0, j = 0;

    memset(&g_Device_info, 0, sizeof(g_Device_info));
    memset(&handset, 0, sizeof(handset));


    /* get devices from db */
    //msg_DECTDataGet(&g_Device_info);

    /* get realtime devices table from base station */
    tcm_GetDeviceList(&g_Device_info);
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Get ule realtime list data, ule device num:%d\n", g_Device_info.device_num);
    j = g_Device_info.device_num;


    tcm_handset_list(&handset);
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Get handset realtime list data, handset num:%d.\n", handset.device_num);
    for (i = 0; i < handset.device_num; i++)
    {
        memcpy(&g_Device_info.device[j + i], &handset.device[i], sizeof(DECT_HAN_DEVICE_INF));
    }
    g_Device_info.device_num += handset.device_num;

    g_Device_info.index = 1;
    return ;
}

void dect_device_status_change(DECT_HAN_DEVICE_INF *devinfo)
{

    if (devinfo == NULL)
        return;

    msg_DECTReport(devinfo);

}

DECT_STATUS dect_handle_voip_call(DECT_HAN_DEVICE_INF *devinfo)
{
    dect_Handle_voip_Message(devinfo->device.voice.action);
    return DECT_STATUS_OK;
}


DECT_STATUS  dect_handset_add(int op)
{
    int ret = 0;
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dect_handset_add op = %d\n", op);
    if (op == 0) {//open
        ret = tcm_add_device(0);
    } else if (op == 1) { //close
        ret = tcm_close_register();
    } else {
        ret = -1;
    }

    if (ret != 0)
    {
        return DECT_STATUS_FAIL;
    }

    return DECT_STATUS_OK;

}
static DECT_STATUS dect_handset_delete(unsigned int id)
{

    int ret = 0;
    char buffer[32] = {0};
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "dect_handset_delete id = %u\n", id);
    sprintf(buffer, "%u", id);
    ret = tcm_handset_delete(buffer);
    if (ret)
    {
        return DECT_STATUS_FAIL;
    }
    else
    {
        return DECT_STATUS_OK;
    }


}

DECT_STATUS dect_handset_ring(unsigned int id)
{
    if (tcm_handset_page(id))
    {
        return DECT_STATUS_FAIL;
    }
    else
    {
        return DECT_STATUS_OK;
    }
}
DECT_STATUS dect_handset_stop_ring(void)
{
    if (tcm_handset_stop_page())
    {
        return DECT_STATUS_FAIL;
    }
    else
    {
        return DECT_STATUS_OK;
    }
}
DECT_STATUS dect_system_reboot(void)
{
    if (tcm_dect_system_reboot())
    {
        return DECT_STATUS_FAIL;
    }
    else
    {
        return DECT_STATUS_OK;
    }
}
DECT_STATUS dect_set_dect_type(int type)
{
    if (tcm_dect_type_set(type))
    {
        return DECT_STATUS_FAIL;
    }
    else
    {
        return DECT_STATUS_OK;
    }
}
DECT_STATUS dect_get_dect_type(int *type)
{
    if (tcm_dect_type_get(type))
    {
        return DECT_STATUS_FAIL;
    }
    else
    {
        return DECT_STATUS_OK;
    }
}
void dect_device_connection_status_change(int bconnection, DECT_HAN_DEVICE_INF *devinfo)
{
    if (NULL == devinfo)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "devinfo = null\n");
        return;
    }
    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Device [%u] connection status change, bconnection [%d]\n", devinfo->id, bconnection);

    if (bconnection)
    {
        msg_DECTReply(0, HC_EVENT_RESP_DEVICE_CONNECTED, devinfo);
    }
    else
    {

        msg_DECTReply(0, HC_EVENT_RESP_DEVICE_DISCONNECTED, devinfo);
    }
}
