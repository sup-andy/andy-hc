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
*   File   : hcapi.c
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
#include "hc_common.h"

#include "sql_api.h"

HC_RETVAL_E hcapi_db_reset_zw_dev(void);
HC_RETVAL_E hcapi_db_reset_zb_dev(void);

#define HC_ARMMODE_CONFIG_FILE "/storage/config/homectrl/armmode"

#define DEFAULT_TIMEOUT_SEC     30

/** Number of micro-seconds in 1 second. */
#define USECS_IN_SEC  1000000

/** Number of micro-seconds in a milli-second. */
#define USECS_IN_MSEC 1000

/** Number of milliseconds in 1 second */
#define MSECS_IN_SEC  1000

/** Number of seconds in 1 minute */
#define SECS_IN_MINUTE  60

#define DB_SET_TIMEOUT  (10 * 1000)
#define DB_GET_TIMEOUT  (3000)

static int g_event_handle = -1;
static int g_command_handle = -1;
static int iadding = 0;
static time_t ttm;

/*************************** Local Function ***************************/
static HC_RETVAL_E convert_network_to_dst(int network, CLIENT_TYPE_E *dst)
{
    switch (network)
    {
        case HC_NETWORK_TYPE_APP:
            *dst = APPLICATION;
            break;
        case HC_NETWORK_TYPE_ZWAVE:
            *dst = DAEMON_ZWAVE;
            break;
        case HC_NETWORK_TYPE_ZIGBEE:
            *dst = DAEMON_ZIGBEE;
            break;
        case HC_NETWORK_TYPE_ULE:
            *dst = DAEMON_ULE;
            break;
        case HC_NETWORK_TYPE_LPRF:
            *dst = DAEMON_LPRF;
            break;
        case HC_NETWORK_TYPE_CAMERA:
            *dst = DAEMON_CAMERA;
            break;
        case HC_NETWORK_TYPE_ALLJOYN:
            *dst = DAEMON_ALLJOYN;
            break;
        case HC_NETWORK_TYPE_ALL:
            *dst = DAEMON_ALL;
            break;
        default:
            DEBUG_ERROR("Unsupported network type '%d'.\n", network);
            return HC_RET_INVALID_ARGUMENTS;
    }

    return HC_RET_SUCCESS;
}

static HC_RETVAL_E convert_network_to_src(int network, CLIENT_TYPE_E *src)
{
    switch (network)
    {
        case HC_NETWORK_TYPE_APP:
            *src = APPLICATION;
            break;
        case HC_NETWORK_TYPE_ZWAVE:
            *src = DAEMON_ZWAVE;
            break;
        case HC_NETWORK_TYPE_ZIGBEE:
            *src = DAEMON_ZIGBEE;
            break;
        case HC_NETWORK_TYPE_ULE:
            *src = DAEMON_ULE;
            break;
        case HC_NETWORK_TYPE_LPRF:
            *src = DAEMON_LPRF;
            break;
        case HC_NETWORK_TYPE_CAMERA:
            *src = DAEMON_CAMERA;
            break;
        case HC_NETWORK_TYPE_ALLJOYN:
            *src = DAEMON_ALLJOYN;
        case HC_NETWORK_TYPE_ALL:
            *src = DAEMON_ALL;
            break;
        default:
            DEBUG_ERROR("Unsupported network type '%d'.\n", network);
            return HC_RET_INVALID_ARGUMENTS;
    }

    return HC_RET_SUCCESS;
}

static HC_RETVAL_E check_hcdev(HC_DEVICE_INFO_S *dev_info)
{
    if (dev_info == NULL)
        return HC_RET_INVALID_ARGUMENTS;

    switch (dev_info->dev_type)
    {
        case HC_DEVICE_TYPE_DIMMER:
            if ((dev_info->device.dimmer.value >= 0x0 && dev_info->device.dimmer.value <= 0x63) ||
                    dev_info->device.dimmer.value == 0xff)
            {
                break;
            }
            else
            {
                DEBUG_ERROR("The value(%d) of dimmer is error.\n", dev_info->device.dimmer.value);
                return HC_RET_INVALID_ARGUMENTS;
            }
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            if ((dev_info->device.curtain.value >= 0x0 && dev_info->device.curtain.value <= 0x63) ||
                    dev_info->device.curtain.value == 0xfe || dev_info->device.curtain.value == 0xff)
            {
                break;
            }
            else
            {
                DEBUG_ERROR("The value(%d) of curtain is error.\n", dev_info->device.curtain.value);
                return HC_RET_INVALID_ARGUMENTS;
            }
            break;
        case HC_DEVICE_TYPE_HSM100_CONFIG:
            if (dev_info->device.hsm100_config.parameterNumber == 1 &&
                    (dev_info->device.hsm100_config.value >= 0 &&
                     dev_info->device.hsm100_config.value <= 255))
            {
                break;
            }
            else if (dev_info->device.hsm100_config.parameterNumber == 2 &&
                     (dev_info->device.hsm100_config.value >= 1 &&
                      dev_info->device.hsm100_config.value <= 127))
            {
                break;
            }
            else if (dev_info->device.hsm100_config.parameterNumber == 5 &&
                     (dev_info->device.hsm100_config.value == 0 ||
                      dev_info->device.hsm100_config.value == -1))
            {
                break;
            }
            else
            {
                DEBUG_ERROR("The battery_level(%d) of battery is error.\n", dev_info->device.battery.battery_level);
                return HC_RET_INVALID_ARGUMENTS;
            }
            break;
        case HC_DEVICE_TYPE_THERMOSTAT:
            if (dev_info->device.thermostat.mode >= HC_THERMOSTAT_MODE_OFF &&
                    dev_info->device.thermostat.mode <= HC_THERMOSTAT_MODE_AWAY_HEATING)
            {
                break;
            }
            else
            {
                DEBUG_ERROR("The mode (%d) of thermostat is error.\n", dev_info->device.thermostat.mode);
                return HC_RET_INVALID_ARGUMENTS;
            }
        default:
            break;
    }

    return HC_RET_SUCCESS;
}

static HC_RETVAL_E send_command(HC_MSG_S *msg, HC_NETWORK_TYPE_E network_type)
{
    int ret = 0;
    struct timeval tv;
    HC_MSG_S *hcmsg = NULL;
    time_t tm;

    if (msg == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }
    if (iadding != 0)
    {
        tm = time(NULL);
        if (tm - ttm <= 3)
        {
            DEBUG_ERROR("HC_EVENT_DEVICE_ADD_DELETE_DOING\n");
            return HC_RET_UNEXPECTED_EVENT;
        } else {
            iadding = 0;
        }
    }

    while (1)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        hcmsg = NULL;
        ret = hc_client_wait_for_msg(g_command_handle, &tv, &hcmsg);
        if (ret < 0)
        {
            hc_msg_free(hcmsg);
            DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
            return HC_RET_RECV_FAILURE;
        }
        hc_msg_free(hcmsg);

        if (ret == 0)
            break;
    }

    ret = convert_network_to_dst(network_type, &(msg->head.dst));
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Convert network(%d) to msg destination failed.\n", network_type);
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = hc_client_send_msg_to_dispacther(g_command_handle, msg);
    if (ret <= 0)
    {
        DEBUG_ERROR("Send msg to dispacher failed. [%d][%d][%d]\n", g_command_handle, msg->head.dev_id, msg->head.db_act);
        return HC_RET_SEND_FAILURE;
    }

    return HC_RET_SUCCESS;
}

static HC_RETVAL_E receive_reply(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    int ret = 0;
    struct timeval tv;
    HC_MSG_S *hcmsg = NULL;
    time_t tm;

    if (iadding != 0 && dev_info->event_type != HC_EVENT_DEVICE_ADD_DELETE_DOING && dev_info->network_type == HC_NETWORK_TYPE_ZWAVE)
    {
        tm = time(NULL);
        if (tm - ttm <= 3)
        {
            DEBUG_ERROR("HC_EVENT_DEVICE_ADD_DELETE_DOING\n");
            return HC_RET_UNEXPECTED_EVENT;
        } else {
            iadding = 0;
        }
    }
    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (timeout_msec > 0)
    {
        tv.tv_sec = timeout_msec / MSECS_IN_SEC;
        tv.tv_usec = (timeout_msec % MSECS_IN_SEC) * USECS_IN_MSEC;
    }
    else
    {
        tv.tv_sec = DEFAULT_TIMEOUT_SEC;
        tv.tv_usec = 0;
    }

    ret = hc_client_wait_for_msg(g_command_handle, &tv, &hcmsg);
    iadding = 0;
    if (ret == 0)
    {
        hc_msg_free(hcmsg);
        return HC_RET_TIMEOUT;
    }
    else if (ret < 0)
    {
        hc_msg_free(hcmsg);
        DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
        return HC_RET_RECV_FAILURE;
    }

    memcpy(dev_info, &hcmsg->body.device_info, sizeof(HC_DEVICE_INFO_S));

    hc_msg_free(hcmsg);

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_init(void)
{
    // initialize msg.
    g_event_handle = hc_client_msg_init(APPLICATION, SOCKET_EVENT);
    if (g_event_handle == -1)
    {
        return HC_RET_FAILURE;
    }

    g_command_handle = hc_client_msg_init(APPLICATION, SOCKET_COMMAND);
    if (g_command_handle == -1)
    {
        hc_client_unint(g_event_handle);
        g_event_handle = -1;
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

void hcapi_uninit(void)
{
    if (g_event_handle != -1)
    {
        hc_client_unint(g_event_handle);
        g_event_handle = -1;
    }

    if (g_command_handle != -1)
    {
        hc_client_unint(g_command_handle);
        g_command_handle = -1;
    }
}

HC_RETVAL_E hcapi_host_module_restart(HC_NETWORK_TYPE_E network_type, unsigned int timeout_msec)
{
    int ret = 0;
    HC_MSG_S msg;
    HC_DEVICE_INFO_S hcdev;

    memset(&msg, 0, sizeof(msg));
    msg.head.type = HC_EVENT_REQ_DEVICE_RESTART;
    msg.head.src = APPLICATION;

    ret = send_command(&msg, network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_FAILURE;
    }

    memset(&hcdev, 0, sizeof(hcdev));
    ret = receive_reply(&hcdev, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_host_module_default(unsigned int timeout_msec)
{
    int ret = 0;
    HC_MSG_S msg;
    HC_DEVICE_INFO_S hcdev;

    memset(&msg, 0, sizeof(msg));
    msg.head.type = HC_EVENT_REQ_DEVICE_DEFAULT;
    msg.head.src = APPLICATION;

    ret = send_command(&msg, HC_NETWORK_TYPE_ALL);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_FAILURE;
    }

    memset(&hcdev, 0, sizeof(hcdev));
    ret = receive_reply(&hcdev, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_FAILURE;
    }
    
    // delete EXT table, but except user log table
    hcapi_db_del_attr_all();
    hcapi_db_del_scene_all();
    hcapi_db_del_location_all();

    // delete device specific table
    hcapi_db_reset_zw_dev();
    hcapi_db_reset_zb_dev();
    hcapi_db_reset_lprf_dev();

    // delete device table
    hcapi_db_del_dev_all();

    return HC_RET_SUCCESS;
}
HC_RETVAL_E hcapi_dev_add(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.type = HC_EVENT_REQ_DEVICE_ADD;
    msg.head.src = APPLICATION;
    msg.body.option_info.u.timeout = timeout_msec / 1000;

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADDED_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        //iadding = 1;
        //ttm = time(NULL);
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_COMPLETE == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_TIMEOUT == dev_info->event_type)
    {
        return HC_RET_TIMEOUT;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_add_stop(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_ADD_STOP;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }


    if (HC_EVENT_RESP_DEVICE_ADD_STOP_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_STOP_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_delete(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.type = HC_EVENT_REQ_DEVICE_DELETE;
    msg.head.dev_type = dev_info->dev_type;
    msg.head.dev_id = dev_info->dev_id;
    msg.head.src = APPLICATION;
    msg.body.option_info.u.id = dev_info->dev_id;
    memcpy (&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETED_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        //iadding = 2;
        //ttm = time(NULL);
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_COMPLETE == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_TIMEOUT == dev_info->event_type)
    {
        return HC_RET_TIMEOUT;
    }
    else if (HC_EVENT_RESP_DEVICE_HAS_BEEN_REMOVED == dev_info->event_type)
    {
        return HC_RET_DEVICE_HAS_REMOVED;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_delete_stop(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_DELETE_STOP;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }


    if (HC_EVENT_RESP_DEVICE_DELETE_STOP_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_STOP_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_drv_status_query(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DRIVER_STATUS_QUERY;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }


    if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_NORMAL_MODE == dev_info->event_type)
    {
        return HC_RET_NORMAL_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_result_handle(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }
    if (HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADDED_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_COMPLETE == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_TIMEOUT == dev_info->event_type)
    {
        return HC_RET_TIMEOUT;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_COMPLETE == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_TIMEOUT == dev_info->event_type)
    {
        return HC_RET_TIMEOUT;
    }
    else if (HC_EVENT_RESP_DEVICE_HAS_BEEN_REMOVED == dev_info->event_type)
    {
        return HC_RET_DEVICE_HAS_REMOVED;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = check_hcdev(dev_info);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_SET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DEVICE_SET_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_SET_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_get(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_GET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DEVICE_GET_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_STATUS_DEVICE_STATUS_CHANGED == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_GET_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_cfg_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = check_hcdev(dev_info);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_CFG_SET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DEVICE_SET_CFG_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_SET_CFG_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_cfg_get(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_CFG_GET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DEVICE_GET_CFG_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_GET_CFG_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_association_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = check_hcdev(dev_info);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_ASSOICATION_SET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_ASSOICATION_SET_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_ASSOICATION_SET_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_association_remove(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = check_hcdev(dev_info);
    if (ret != HC_RET_SUCCESS)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_ASSOICATION_REMOVE;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_ASSOICATION_REMOVE_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_ASSOICATION_REMOVE_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_detect(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_DETECT;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (dev_info->event_type == HC_EVENT_RESP_DEVICE_CONNECTED)
    {
        return HC_RET_SUCCESS;
    }
    else if (dev_info->event_type == HC_EVENT_RESP_DEVICE_DISCONNECTED)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dev_na_delete(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DEVICE_NA_DELETE;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;
    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }


    if (HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETED_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else if (HC_EVENT_RESP_DEVICE_ADD_MODE == dev_info->event_type)
    {
        return HC_RET_ADDING_MODE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETE_MODE == dev_info->event_type)
    {
        return HC_RET_DELETING_MODE;
    }
    else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info->event_type)
    {
        return HC_RET_FUNCTION_NOT_SUPPORT;
    }
    else if (HC_EVENT_RESP_DB_OPERATION_FAILURE == dev_info->event_type)
    {
        return HC_RET_DB_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}


HC_RETVAL_E hcapi_event_handle(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    int ret = 0;
    struct timeval tv;
    HC_MSG_S *hcmsg = NULL;
    int i = 0;

    if (dev_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (timeout_msec > 0)
    {
        tv.tv_sec = timeout_msec / MSECS_IN_SEC;
        tv.tv_usec = (timeout_msec % MSECS_IN_SEC) * USECS_IN_MSEC;
    }
    else
    {
        tv.tv_sec = DEFAULT_TIMEOUT_SEC;
        tv.tv_usec = 0;
    }

    ret = hc_client_wait_for_msg(g_event_handle, &tv, &hcmsg);
    if (ret == 0)
    {
        hc_msg_free(hcmsg);
        return HC_RET_TIMEOUT;
    }
    else if (ret < 0)
    {
        hc_msg_free(hcmsg);
        DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
        return HC_RET_RECV_FAILURE;
    }

    memcpy(dev_info, &hcmsg->body.device_info, sizeof(HC_DEVICE_INFO_S));

    hc_msg_free(hcmsg);

    return HC_RET_SUCCESS;
}


/************************** Read Database *************************/
HC_RETVAL_E hcapi_get_devs_all(HC_DEVICE_INFO_S *devs_info, int dev_number)
{
    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_all(dev_number, devs_info);
}

HC_RETVAL_E hcapi_get_devs_by_dev_type(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_INFO_S *devs_info, int dev_number)
{

    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }



    return hcapi_db_get_dev_by_dev_type(dev_type, dev_number, devs_info);
}

HC_RETVAL_E hcapi_get_devs_by_network_type(HC_NETWORK_TYPE_E network_type, HC_DEVICE_INFO_S *devs_info, int dev_number)
{

    if (network_type <= 0 || network_type >= HC_NETWORK_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }


    return hcapi_db_get_dev_by_network_type(network_type, dev_number, devs_info);
}

HC_RETVAL_E hcapi_get_devs_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, HC_DEVICE_INFO_S *devs_info, int dev_number)
{

    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX || location == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_by_dev_type_and_location(dev_type, location, dev_number, devs_info);
}

HC_RETVAL_E hcapi_get_devnum_all(int *dev_number)
{
    return hcapi_db_get_dev_num_all(dev_number);
}

HC_RETVAL_E hcapi_get_devnum_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *dev_number)
{
    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_num_by_dev_type(dev_type, dev_number);
}

HC_RETVAL_E hcapi_get_devnum_by_network_type(HC_NETWORK_TYPE_E network_type, int *dev_number)
{
    if (network_type <= 0 || network_type >= HC_NETWORK_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_num_by_network_type(network_type, dev_number);
}

HC_RETVAL_E hcapi_get_devnum_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int *dev_number)
{
    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX || location == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_num_by_dev_type_and_location(dev_type, location, dev_number);
}

HC_RETVAL_E hcapi_get_dev_by_dev_id(HC_DEVICE_INFO_S *devs_info)
{
    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_by_dev_id(devs_info);
}

HC_RETVAL_E hcapi_add_log(HC_DEVICE_INFO_S* devs_info, char* log, long log_time, int alarm_type, char* name)
{
    if (devs_info == NULL || log == NULL || name == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_dev_log(devs_info, log, log_time, alarm_type, name);
}

HC_RETVAL_E hcapi_add_log_v2(HC_DEVICE_EXT_LOG_S* log_ptr)
{
    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_dev_log_v2(log_ptr);
}


HC_RETVAL_E hcapi_get_log_all(char *buffer, int size)
{
    if (buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_log_all(size, buffer);
}

HC_RETVAL_E hcapi_get_log_all_v2(HC_DEVICE_EXT_LOG_S *buffer, int size)
{
    if (buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_log_all_v2(size, buffer);
}


HC_RETVAL_E hcapi_get_log_all_num(int *size)
{
    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_num_all(size);
}


HC_RETVAL_E hcapi_get_log_by_dev_id(unsigned int dev_id, char *buffer, int size)
{
    if (dev_id == NULL || buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_log_by_dev_id(size, buffer, dev_id);
}

HC_RETVAL_E hcapi_get_log_by_dev_id_v2(unsigned int dev_id, HC_DEVICE_EXT_LOG_S *buffer, int size)
{
    if (dev_id == NULL || buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_log_by_dev_id_v2(size, buffer, dev_id);
}

HC_RETVAL_E hcapi_get_log_by_dev_id_num(unsigned int dev_id, int *size)
{
    if (dev_id == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_num_by_dev_id(dev_id, size);
}


HC_RETVAL_E hcapi_get_log_by_dev_type(HC_DEVICE_TYPE_E dev_type, char *buffer, int size)
{
    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_by_dev_type(size, buffer, dev_type);
}

HC_RETVAL_E hcapi_get_log_by_dev_type_v2(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_EXT_LOG_S *buffer, int size)
{
    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_by_dev_type_v2(size, buffer, dev_type);
}


HC_RETVAL_E hcapi_get_log_by_dev_type_num(HC_DEVICE_TYPE_E dev_type, int *size)
{
    if (dev_type <= 0 || dev_type >= HC_DEVICE_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_num_by_dev_type(dev_type, size);
}


HC_RETVAL_E hcapi_get_log_by_network_type(HC_NETWORK_TYPE_E network_type, char *buffer, int size)
{
    if (network_type <= 0 || network_type >= HC_NETWORK_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_by_network_type(size, buffer, network_type);
}

HC_RETVAL_E hcapi_get_log_by_network_type_v2(HC_NETWORK_TYPE_E network_type, HC_DEVICE_EXT_LOG_S *buffer, int size)
{
    if (network_type <= 0 || network_type >= HC_NETWORK_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (buffer == NULL || size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_by_network_type_v2(size, buffer, network_type);
}


HC_RETVAL_E hcapi_get_log_by_network_type_num(HC_NETWORK_TYPE_E network_type, int *size)
{
    if (network_type <= 0 || network_type >= HC_NETWORK_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_dev_log_num_by_network_type(network_type, size);
}


HC_RETVAL_E hcapi_add_dev_log(HC_DEVICE_INFO_S* devs_info, char* log, long log_time, int alarm_type, char* name)
{
    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_dev_log(devs_info, log, log_time, alarm_type, name);

}


HC_RETVAL_E hcapi_add_dev(HC_DEVICE_INFO_S *devs_info)
{
    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_dev(devs_info);
}


HC_RETVAL_E hcapi_add_devname(unsigned int dev_id, char *buffer)
{
    if (buffer == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_dev_name(dev_id, buffer);
}


HC_RETVAL_E hcapi_set_dev(HC_DEVICE_INFO_S *devs_info)
{
    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_dev(devs_info);
}


HC_RETVAL_E hcapi_set_devname(unsigned int dev_id, char *buffer)
{
    if (buffer == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_dev_name(dev_id, buffer);
}


HC_RETVAL_E hcapi_del_dev(HC_DEVICE_INFO_S *devs_info)
{
    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_del_dev(devs_info);
}

HC_RETVAL_E hcapi_del_dev_all()
{
    return hcapi_db_del_dev_all();
}


HC_RETVAL_E hcapi_add_attr(unsigned int dev_id, char *name, char* value)
{
    if ((dev_id == NULL) || (name == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_attr(dev_id, name, value);
}

HC_RETVAL_E hcapi_get_attr(unsigned int dev_id, char *name, char* value)
{
    if ((dev_id == NULL) || (name == NULL) || (value == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_attr(dev_id, name, value);
}


HC_RETVAL_E hcapi_get_attr_by_dev_id(unsigned int dev_id, int size, HC_DEVICE_EXT_ATTR_S * attr_ptr)
{
    if ((dev_id == NULL) || (size <= 0) || (attr_ptr == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_attr_by_dev_id(dev_id, size, attr_ptr);
}


HC_RETVAL_E hcapi_get_attr_num_by_dev_id(unsigned int dev_id, int *size)
{
    if ((dev_id == NULL) || (size == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_attr_num_by_dev_id(dev_id, size);
}

HC_RETVAL_E hcapi_set_attr(unsigned int dev_id, char *name, char* value)
{
    if ((dev_id == NULL) || (name == NULL) || (value == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_attr(dev_id, name, value);
}


HC_RETVAL_E hcapi_del_attr(unsigned int dev_id, char *name)
{
    if ((dev_id == NULL) || (name == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_del_attr(dev_id, name);
}


HC_RETVAL_E hcapi_del_attr_by_id(unsigned int dev_id)
{
    if ((dev_id == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_del_attr_by_dev_id(dev_id);
}

HC_RETVAL_E hcapi_add_scene(char* scene_id, char *scene_attr)
{
    if (scene_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_scene(scene_id, scene_attr);
}


HC_RETVAL_E hcapi_get_scene(char* scene_id, char *scene_attr)
{
    if (scene_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_scene(scene_id, scene_attr);
}

HC_RETVAL_E hcapi_set_scene(char* scene_id, char *scene_attr)
{
    if (scene_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_scene(scene_id, scene_attr);;
}

HC_RETVAL_E hcapi_del_scene(char* scene_id)
{
    return hcapi_db_del_scene(scene_id);
}


HC_RETVAL_E hcapi_get_scene_num_all(int *size)
{
    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_scene_num_all(size);
}


HC_RETVAL_E hcapi_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S * scene_ptr)
{
    if (scene_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_scene_all(size, scene_ptr);
}


HC_RETVAL_E hcapi_del_scene_all()
{
    return hcapi_db_del_scene_all();
}

HC_RETVAL_E hcapi_add_user_log(HC_USER_LOG_SEVERITY_E severity, char *content)
{
    return hcapi_db_add_user_log(severity, content);
}

HC_RETVAL_E hcapi_send_ext_signal(int msg_type, int network_type, void *data, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;
    HC_DEVICE_INFO_S dev_info;

    memset(&msg, 0, sizeof(msg));
    msg.head.type = msg_type;
    msg.head.src = APPLICATION;
    if (data)
    {
        if (HC_EVENT_EXT_IP_CHANGED == msg_type)
        {
            HC_DEVICE_EXT_IP_CHANGED_S *tmp = (HC_DEVICE_EXT_IP_CHANGED_S *)data;
            msg.body.device_info.device.ext_ip_changed.port = tmp->port;
            strcpy(msg.body.device_info.device.ext_ip_changed.ip_addr, tmp->ip_addr);
            strcpy(msg.body.device_info.device.ext_ip_changed.wan_if_name, tmp->wan_if_name);
        }
        else if (HC_EVENT_EXT_CAMERA_FW_UPGRADE == msg_type)
        {
            HC_DEVICE_EXT_CAMERA_UPGRADE_S *tmp = (HC_DEVICE_EXT_CAMERA_UPGRADE_S *)data;
            msg.body.device_info.device.ext_camera_upgrade.device_type = tmp->device_type;
            msg.body.device_info.device.ext_camera_upgrade.is_new_image = tmp->is_new_image;
            strcpy(msg.body.device_info.device.ext_camera_upgrade.firmware_version, tmp->firmware_version);
            strcpy(msg.body.device_info.device.ext_camera_upgrade.file_path, tmp->file_path);
        }
        else if (HC_EVNET_EXT_ARMMODE_CHANGED == msg_type)
        {
            HC_DEVICE_EXT_ARMMODE_S *tmp = (HC_DEVICE_EXT_ARMMODE_S *)data;
            msg.body.device_info.device.ext_u.armmode.value = tmp->value;
        }
        else if (HC_EVENT_EXT_MODULE_UPGRADE == msg_type)
        {
            HC_UPGRADE_INFO_S *tmp = (HC_UPGRADE_INFO_S *)data;

            msg.body.upgrade_info.type = tmp->type;
            strncpy(&msg.body.upgrade_info.image_file_path, &tmp->image_file_path, CTRL_MAX_PATH - 1);
            msg.body.upgrade_info.image_file_path[CTRL_MAX_PATH - 1] = '\0';
        }
        else
        {
            return HC_RET_INVALID_ARGUMENTS;
        }
    }

    ret = send_command(&msg, network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (timeout_msec > 0)
    {
        memset(&dev_info, 0, sizeof(HC_DEVICE_INFO_S));
        ret = receive_reply(&dev_info, timeout_msec);
        if (ret != HC_RET_SUCCESS)
        {
            return ret;
        }

        if (HC_EVENT_EXT_FW_UPGRADE_LOCK_DONE == dev_info.event_type)
        {
            return HC_RET_SUCCESS;
        }
        else if (HC_EVENT_EXT_FW_UPGRADE_LOCK_FAILED == dev_info.event_type)
        {
            return HC_RET_LOCK_FAILURE;
        }
        else if (HC_EVENT_RESP_FUNCTION_NOT_SUPPORT == dev_info.event_type)
        {
            return HC_RET_FUNCTION_NOT_SUPPORT;
        }
        else if (HC_EVENT_EXT_MODULE_UPGRADE_SUCCESS == dev_info.event_type)
        {
            return HC_RET_SUCCESS;
        }
        else if (HC_EVENT_EXT_MODULE_UPGRADE_FAILURE == dev_info.event_type)
        {
            return HC_RET_FAILURE;
        }
        else
        {
            return HC_RET_UNEXPECTED_EVENT;
        }
    }

    return HC_RET_SUCCESS;

}

HC_RETVAL_E hcapi_get_user_log_all(HC_DEVICE_EXT_USER_LOG_S* log, int size)
{
    if (log == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_user_log_all(size, log);
}

HC_RETVAL_E hcapi_get_user_log_num_all(int *size)
{
    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_user_log_num_all(size);
}


HC_RETVAL_E hcapi_add_location(char* location_id, char *location_attr)
{
    if (location_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_add_location(location_id, location_attr);
}


HC_RETVAL_E hcapi_get_location(char* location_id, char *location_attr, int size)
{
    if (location_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_location(location_id, location_attr, size);
}

HC_RETVAL_E hcapi_set_location(char* location_id, char *location_attr)
{
    if (location_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_location(location_id, location_attr);;
}

HC_RETVAL_E hcapi_del_location(char* location_id)
{
    return hcapi_db_del_location(location_id);
}


HC_RETVAL_E hcapi_get_location_num_all(int *size)
{
    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_location_num_all(size);
}


HC_RETVAL_E hcapi_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S * location_ptr)
{
    if (location_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_location_all(size, location_ptr);
}


HC_RETVAL_E hcapi_del_location_all()
{
    return hcapi_db_del_location_all();
}

HC_RETVAL_E hcapi_set_conf(char* conf_name, char *conf_value)
{
    if (conf_value == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_set_conf(conf_name, conf_value);
}


HC_RETVAL_E hcapi_get_conf(char* conf_name, char *conf_value)
{
    if (conf_value == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    return hcapi_db_get_conf(conf_name, conf_value);
}


HC_RETVAL_E hcapi_voip_event_handle(HC_VOIP_MSG_S *pMsg, unsigned int timeout_msec)
{
    int ret = 0;
    struct timeval tv;
    HC_MSG_S *hcmsg = NULL;
    int i = 0;

    if (pMsg == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (timeout_msec > 0)
    {
        tv.tv_sec = timeout_msec / MSECS_IN_SEC;
        tv.tv_usec = (timeout_msec % MSECS_IN_SEC) * USECS_IN_MSEC;
    }
    else
    {
        tv.tv_sec = DEFAULT_TIMEOUT_SEC;
        tv.tv_usec = 0;
    }

    ret = hc_client_wait_for_msg(g_event_handle, &tv, &hcmsg);
    if (ret == 0)
    {
        hc_msg_free(hcmsg);
        return HC_RET_TIMEOUT;
    }
    else if (ret < 0)
    {
        hc_msg_free(hcmsg);
        DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
        return HC_RET_RECV_FAILURE;
    }

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "hcmsg->head.type '%d'.\n", hcmsg->head.type);

    if (hcmsg->head.type == HC_EVENT_RESP_VOIP_CALL ||
            hcmsg->head.type == HC_EVENT_REQ_VOIP_CALL)
    {
        pMsg->event_type = hcmsg->head.type;
        strncpy(pMsg->action, hcmsg->body.device_info.device.handset.u.action, sizeof(pMsg->action) - 1);
    }

    hc_msg_free(hcmsg);

    return ret;
}

HC_RETVAL_E hcapi_voip_message_send(HC_VOIP_MSG_S *pMsg)
{
    HC_MSG_S msg;
    int ret = 0;

    if (pMsg == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&msg, 0, sizeof(msg));
    msg.head.type = pMsg->event_type;
    msg.head.src = APPLICATION;
    msg.body.device_info.network_type = HC_NETWORK_TYPE_ULE;
    strncpy(msg.body.device_info.device.handset.u.action, pMsg->action, sizeof(msg.body.device_info.device.handset.u.action) - 1);

    ret = send_command(&msg, HC_NETWORK_TYPE_ULE);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }
    return HC_RET_SUCCESS;
}
HC_RETVAL_E hcapi_dect_parameter_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    memset(&msg, 0, sizeof(msg));
    //msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DECT_PARAMETER_SET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;

    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DECT_PARAMETER_SET_SUCCESS == dev_info->event_type)
    {
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DECT_PARAMETER_SET_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_dect_parameter_get(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec)
{
    HC_MSG_S msg;
    int ret = 0;

    memset(&msg, 0, sizeof(msg));
    //msg.head.dev_id = dev_info->dev_id;
    msg.head.type = HC_EVENT_REQ_DECT_PARAMETER_GET;
    msg.head.src = APPLICATION;
    msg.head.dev_type = dev_info->dev_type;

    memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));

    ret = send_command(&msg, dev_info->network_type);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    ret = receive_reply(dev_info, timeout_msec);
    if (ret != HC_RET_SUCCESS)
    {
        return ret;
    }

    if (HC_EVENT_RESP_DECT_PARAMETER_GET_SUCCESS == dev_info->event_type)
    {
        memcpy(&msg.body.device_info, dev_info, sizeof(HC_DEVICE_INFO_S));
        return HC_RET_SUCCESS;
    }
    else if (HC_EVENT_RESP_DECT_PARAMETER_GET_FAILURE == dev_info->event_type)
    {
        return HC_RET_OPERATION_FAILURE;
    }
    else
    {
        return HC_RET_UNEXPECTED_EVENT;
    }

}

HC_RETVAL_E hcapi_get_armmode(HC_ARM_MODE *armmode)
{
    FILE * fp = NULL;

    fp = fopen(HC_ARMMODE_CONFIG_FILE, "r");
    if (NULL == fp)
    {
        // Set ARM mode as default when armmode file is not available.
        armmode->mode = HC_DISARM;
        if (errno == ENOENT)
        {
            // Try to re-create armmode file if the file is lost
            fp = fopen(HC_ARMMODE_CONFIG_FILE, "w");
            if (NULL == fp)
            {
                DEBUG_ERROR("Fail to open file [%s], reason = [%s]\n", HC_ARMMODE_CONFIG_FILE, strerror(errno));
                return HC_RET_FAILURE;
            }
            else
            {
                DEBUG_INFO("File [%s] recovered.\n", HC_ARMMODE_CONFIG_FILE);
                fwrite(armmode, sizeof(HC_ARM_MODE), 1, fp);
                fclose(fp);
            }
        }
        else
        {
            DEBUG_ERROR("Fail to open file [%s], reason = [%s]\n", HC_ARMMODE_CONFIG_FILE, strerror(errno));
            return HC_RET_FAILURE;
        }
    }
    else
    {
        fread(armmode, sizeof(HC_ARM_MODE), 1, fp);
        fclose(fp);
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_set_armmode(HC_ARM_MODE *armmode)
{
    FILE * fp;
    fp = fopen(HC_ARMMODE_CONFIG_FILE, "w+");
    if (NULL == fp)
    {
        DEBUG_ERROR("Fail to open file [%s], reason = [%s]\n", HC_ARMMODE_CONFIG_FILE, strerror(errno));
        return HC_RET_FAILURE;
    }

    fwrite(armmode, sizeof(HC_ARM_MODE), 1, fp);

    fclose(fp);

    return HC_RET_SUCCESS;
}


