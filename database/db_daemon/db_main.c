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
#include "sql_api.h"

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_common.h"

#ifndef TRUE
#define TRUE  (1)
#define FALSE (0)
#endif

#define DB_SYNC_INTERVAL    30

int keep_looping = 1;

static int g_msg_fd = -1;

static void dbd_signal_handle(int signal)
{
    keep_looping = 0;
}

static int dbd_msg_init(void)
{
    // invoke library MSG provided APIs to create socket.
    g_msg_fd = hc_client_msg_init(DAEMON_DB, SOCKET_DEFAULT);
    if (g_msg_fd == -1)
    {
        DEBUG_ERROR("Call hc_client_msg_init failed, errno is '%d'.\n", errno);
        return -1;
    }

    return 0;
}

static void dbd_msg_uninit(void)
{
    if (g_msg_fd !=  -1)
    {
        hc_client_unint(g_msg_fd);
    }
}

static int dbd_msg_send(HC_MSG_S *pmsg)
{
    int ret = 0;
    dump_buffer(__func__, pmsg, sizeof(HC_MSG_S) + pmsg->head.data_len);

    if (TRUE != pmsg->head.db_resp_none)
    {
        ret = hc_client_send_msg_to_dispacther(g_msg_fd, pmsg);
        if (ret <= 0)
        {
            DEBUG_ERROR("Send msg to dispacher failed. [%d][%d->%d][%d:%d]\n",
                        g_msg_fd, pmsg->head.src, pmsg->head.dst,
                        pmsg->head.db_act, pmsg->head.db_act_ret);
            return -1;
        }

        DEBUG_INFO("Send msg to dispacher OK. [%d][%d->%d][%d:%d]\n",
                   g_msg_fd, pmsg->head.src, pmsg->head.dst,
                   pmsg->head.db_act, pmsg->head.db_act_ret);
    }

    return 0;
}

static int dbd_process_msg(HC_MSG_S *pmsg)
{
    HC_MSG_S hcmsg_resp;
    int ret = 0;
    HC_MSG_S* msg_ptr;
    int mem_size = 0;
    int malloc_flag = 0;
    int ret_send = 0;
    HC_DEVICE_INFO_S old_device_info;

    if (pmsg == NULL)
    {
        return -1;
    }

    dump_buffer(__func__, pmsg, sizeof(HC_MSG_S) + pmsg->head.data_len);

    if (HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == pmsg->head.type)
    {
        pmsg->head.db_act = DB_ACT_ADD_DEV;
        pmsg->head.db_resp_none = FALSE;
    }
    else if (HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == pmsg->head.type)
    {
        pmsg->head.db_act = DB_ACT_DEL_DEV;
        pmsg->head.db_resp_none = FALSE;
    }
    else if (HC_EVENT_RESP_DEVICE_SET_SUCCESS == pmsg->head.type)
    {
        pmsg->head.db_act = DB_ACT_SET_DEV;
        pmsg->head.db_resp_none = FALSE;
    }
    else if (HC_EVENT_STATUS_DEVICE_STATUS_CHANGED == pmsg->head.type)
    {
        pmsg->head.db_act = DB_ACT_SET_DEV;
        pmsg->head.db_resp_none = FALSE;
    }
    else if (HC_EVENT_RESP_DEVICE_CONNECTED == pmsg->head.type)
    {
        pmsg->head.db_act = DB_ACT_ATTR_SET;
        pmsg->head.db_resp_none = FALSE;

        pmsg->body.device_info.device.ext_u.attr.dev_id = pmsg->body.device_info.dev_id;
        strncpy(pmsg->body.device_info.device.ext_u.attr.attr_name,
                "connection", sizeof(pmsg->body.device_info.device.ext_u.attr.attr_name) - 1);
        strncpy(pmsg->body.device_info.device.ext_u.attr.attr_value,
                "online", sizeof(pmsg->body.device_info.device.ext_u.attr.attr_value) - 1);
    }
    else if (HC_EVENT_RESP_DEVICE_DISCONNECTED == pmsg->head.type)
    {
        pmsg->head.db_act = DB_ACT_ATTR_SET;
        pmsg->head.db_resp_none = FALSE;

        pmsg->body.device_info.device.ext_u.attr.dev_id = pmsg->body.device_info.dev_id;
        strncpy(pmsg->body.device_info.device.ext_u.attr.attr_name,
                "connection", sizeof(pmsg->body.device_info.device.ext_u.attr.attr_name) - 1);
        strncpy(pmsg->body.device_info.device.ext_u.attr.attr_value,
                "offline", sizeof(pmsg->body.device_info.device.ext_u.attr.attr_value) - 1);
    }

    memcpy(&hcmsg_resp, pmsg, sizeof(HC_MSG_S));

    // parse msg.
    DEBUG_INFO("DB Process msg %s(%d) from %s(%d) with db act [%d].\n",
               hc_map_msg_txt(pmsg->head.type), pmsg->head.type,
               hc_map_client_txt(pmsg->head.src), pmsg->head.src,
               pmsg->head.db_act);

    // set/get db
    switch (pmsg->head.db_act)
    {
        case DB_ACT_ADD_DEV:
            ret = dbd_set_dev(&(pmsg->body.device_info));
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_set_dev id [%x] ret [%d].\n",
                            pmsg->body.device_info.dev_id, ret);
                break;
            }
            pmsg->body.device_info.device.ext_u.dev_name.dev_id = pmsg->body.device_info.dev_id;
            memset(pmsg->body.device_info.device.ext_u.dev_name.dev_name, 0, DEVICE_NAME_LEN);
            strncpy(pmsg->body.device_info.device.ext_u.dev_name.dev_name, pmsg->body.device_info.dev_name, DEVICE_NAME_LEN);
            pmsg->body.device_info.device.ext_u.dev_name.dev_name[DEVICE_NAME_LEN - 1] = '\0';
            ret = dbd_update_dev_name(&(pmsg->body.device_info));
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_update_dev_name id [%x] with name [%s] ret [%d].\n",
                            pmsg->body.device_info.dev_id, pmsg->body.device_info.dev_name, ret);
                break;
            }
            break;
        case DB_ACT_GET_DEV_BY_ID:
            ret = dbd_get_dev(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info));
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_get_dev id [%x] ret [%d].\n",
                            pmsg->body.device_info.dev_id, ret);
                break;
            }
            break;
        case DB_ACT_SET_DEV:
            memset(&old_device_info, 0x0, sizeof(old_device_info));
            ret = dbd_get_dev(&(pmsg->body.device_info), &old_device_info);
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_get_dev id [%x] ret [%d].\n", pmsg->body.device_info.dev_id, ret);
                break;
            }

            if (0 == memcmp(&old_device_info.device, &(pmsg->body.device_info.device), sizeof(HC_DEVICE_U)))
            {
                DEBUG_INFO("DB Info DB_ACT_SET_DEV with no change.\n");
                ret = DB_NO_CHANGE;
                break;
            }

            ret = dbd_set_dev(&(pmsg->body.device_info));
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_set_dev id [%x] ret [%d].\n", pmsg->body.device_info.dev_id, ret);
                break;
            }
            break;
        case DB_ACT_GET_DEV_ALL:
            mem_size = (pmsg->head.dev_count) * sizeof(HC_DEVICE_INFO_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                ret = dbd_get_dev_all(pmsg->head.dev_count, (HC_DEVICE_INFO_S*)msg_ptr->data);
                malloc_flag = 1;
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_NUM_ALL:
            ret = dbd_get_dev_num_all(&(hcmsg_resp.head.dev_count));
            break;
        case DB_ACT_GET_DEV_BY_DEV_TYPE:
            printf("DB_ACT_GET_DEV_BY_DEV_TYPE(%d)\n", pmsg->head.dev_count);
            mem_size = (pmsg->head.dev_count) * sizeof(HC_DEVICE_INFO_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                ret = dbd_get_dev_by_dev_type((pmsg->head.dev_type), pmsg->head.dev_count, (HC_DEVICE_INFO_S*)msg_ptr->data);
                malloc_flag = 1;
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_NUM_BY_DEV_TYPE:
            printf("DB_ACT_GET_DEV_NUM_BY_DEV_TYPE\n");
            ret = dbd_get_dev_num_by_dev_type((pmsg->head.dev_type), pmsg->head.dev_count);
            break;
        case DB_ACT_GET_DEV_BY_NETWORK_TYPE:
            printf("DB_ACT_GET_DEV_BY_NETWORK_TYPE(%d)\n", pmsg->head.dev_count);
            mem_size = (pmsg->head.dev_count) * sizeof(HC_DEVICE_INFO_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                ret = dbd_get_dev_by_network_type((pmsg->body.device_info.device.ext_u.get_dev.network_type), pmsg->head.dev_count, (HC_DEVICE_INFO_S*)msg_ptr->data);
                malloc_flag = 1;
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_NUM_BY_NETWORK_TYPE:
            printf("DB_ACT_GET_DEV_NUM_BY_NETWORK_TYPE\n");
            ret = dbd_get_dev_num_by_network_type((pmsg->body.device_info.device.ext_u.get_dev.network_type), &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_DEL_DEV:
            ret = dbd_del_dev(&(pmsg->body.device_info));
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_del_dev id [%x] ret [%d].\n",
                            pmsg->body.device_info.dev_id, ret);
                break;
            }
            break;
        case DB_ACT_DEL_DEV_ALL:
            ret = dbd_del_dev_all();
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_del_dev_all ret [%d].\n");
                break;
            }
            break;

        case DB_ACT_ADD_DEV_LOG:
            ret = dbd_add_log_v2(&(pmsg->body.device_info));
            break;
        case DB_ACT_GET_DEV_LOG:
            mem_size = (pmsg->body.device_info.device.ext_u.get_log.log_num) * sizeof(HC_DEVICE_EXT_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_log_all_v2(&(pmsg->body.device_info), (HC_DEVICE_EXT_LOG_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_LOG_BY_DEV_ID:
            mem_size = (pmsg->body.device_info.device.ext_u.get_log.log_num) * sizeof(HC_DEVICE_EXT_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_dev_log_by_dev_id_v2(&(pmsg->body.device_info), (HC_DEVICE_EXT_LOG_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_LOG_BY_DEV_TYPE:
            mem_size = (pmsg->body.device_info.device.ext_u.get_log.log_num) * sizeof(HC_DEVICE_EXT_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_dev_log_by_dev_type_v2(&(pmsg->body.device_info), (HC_DEVICE_EXT_LOG_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_LOG_BY_NETWORK_TYPE:
            mem_size = (pmsg->body.device_info.device.ext_u.get_log.log_num) * sizeof(HC_DEVICE_EXT_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_dev_log_by_network_type_v2(&(pmsg->body.device_info), (HC_DEVICE_EXT_LOG_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_LOG_BY_TIME:
            mem_size = (pmsg->body.device_info.device.ext_u.get_log.log_num) * sizeof(HC_DEVICE_EXT_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_dev_log_by_by_time_v2(&(pmsg->body.device_info), (HC_DEVICE_EXT_LOG_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_LOG_BY_ALARM_TYPE:
            mem_size = (pmsg->body.device_info.device.ext_u.get_log.log_num) * sizeof(HC_DEVICE_EXT_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_dev_log_by_alarm_type(&(pmsg->body.device_info), (char*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_DEV_LOG_NUM:
            ret = dbd_get_dev_log_num(&(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_GET_DEV_LOG_NUM_BY_DEV_ID:
            ret = dbd_get_dev_log_num_by_dev_id((pmsg->body.device_info.device.ext_u.get_log.dev_id), &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_GET_DEV_LOG_NUM_BY_NETWORK_TYPE:
            ret = dbd_get_dev_log_num_by_network_type((pmsg->body.device_info.device.ext_u.get_log.network_type), &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_GET_DEV_LOG_NUM_BY_DEV_TYPE:
            ret = dbd_get_dev_log_num_by_dev_type(pmsg->body.device_info.device.ext_u.get_log.dev_type, &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_GET_DEV_LOG_NUM_BY_TIME:
            ret = dbd_get_dev_log_num_by_time(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_GET_DEV_LOG_NUM_BY_ALARM_TYPE:
            ret = dbd_get_dev_log_num_by_alarm_type(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_DEL_DEV_LOG_ALL:
            ret = dbd_del_dev_log_all();
            break;
        case DB_ACT_DEL_DEV_LOG_BY_DEV_ID:
            ret = dbd_del_dev_log_by_dev_id(&(pmsg->body.device_info));
            break;

        case DB_SET_DEV_EXT:
            //ret = dbd_add_dev_name(&(pmsg->body.device_info));
            ret = dbd_update_dev_name(&(pmsg->body.device_info));
            break;
        case DB_ADD_DEV_EXT:
            ret = dbd_update_dev_name(&(pmsg->body.device_info));
            break;
        case DB_GET_DEV_EXT_BY_DEV_ID:
            ret = dbd_get_dev_name((pmsg->body.device_info.device.ext_u.dev_name.dev_id), (hcmsg_resp.body.device_info.device.ext_u.dev_name.dev_name));
            break;
        case DB_DEL_DEV_EXT_BY_DEV_ID:
            ret = dbd_del_dev_name_by_dev_id((pmsg->body.device_info.device.ext_u.dev_name.dev_id));
            break;

        case DB_ACT_ADD_DEV_NAME:
            ret = dbd_add_dev_name(&(pmsg->body.device_info));
            break;
        case DB_ACT_GET_DEV_NAME:
            ret = dbd_get_dev_name((pmsg->body.device_info.device.ext_u.dev_name.dev_id), (hcmsg_resp.body.device_info.device.ext_u.dev_name.dev_name));
            break;
        case DB_ACT_SET_DEV_NAME:
            ret = dbd_update_dev_name(&(pmsg->body.device_info));
            break;
        case DB_ACT_DEL_DEV_NAME:
            ret = dbd_del_dev_name_by_dev_id((pmsg->body.device_info.device.ext_u.dev_name.dev_id));
            break;


        case DB_ACT_ATTR_ADD:
            ret = dbd_add_attr(&(pmsg->body.device_info));
            break;
        case DB_ACT_ATTR_GET:
            ret = dbd_get_attr(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info));
            break;
        case DB_ACT_ATTR_GET_BY_DEV_ID:
            mem_size = (pmsg->head.dev_count) * sizeof(HC_DEVICE_EXT_ATTR_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_attr_all_by_id((pmsg->body.device_info.device.ext_u.attr.dev_id), (HC_DEVICE_EXT_ATTR_S*)msg_ptr->data, (pmsg->head.dev_count));
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_ATTR_GET_NUM_BY_DEV_ID:
            ret = dbd_get_attr_num_by_dev_id((pmsg->body.device_info.device.ext_u.attr.dev_id), &(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_ATTR_SET:
            memset(&old_device_info, 0x0, sizeof(old_device_info));
            ret = dbd_get_attr(&(pmsg->body.device_info), &old_device_info);
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_get_attr id [%x] ret [%d].\n", pmsg->body.device_info.dev_id, ret);
                break;
            }

            if (0 == memcmp(&old_device_info.device.ext_u.attr,
                            &(pmsg->body.device_info.device.ext_u.attr),
                            sizeof(HC_DEVICE_EXT_ATTR_S)))
            {
                DEBUG_INFO("DB Info DB_ACT_ATTR_SET with no change.\n");
                ret = DB_NO_CHANGE;
                break;
            }

            ret = dbd_set_attr(&(pmsg->body.device_info));
            if (DB_RETVAL_OK != ret)
            {
                DEBUG_ERROR("DB Error dbd_set_attr id [%x] ret [%d].\n", pmsg->body.device_info.dev_id, ret);
                break;
            }
            break;
        case DB_ACT_ATTR_DEL:
            ret = dbd_del_attr(&(pmsg->body.device_info));
            break;
        case DB_ACT_ATTR_DEL_BY_DEV_ID:
            ret = dbd_del_attr_by_dev_id(&(pmsg->body.device_info));
            break;


        case DB_ACT_SCENE_ADD:
            ret = dbd_add_scene(&(pmsg->body.device_info));
            break;
        case DB_ACT_SCENE_GET:
            ret = dbd_get_scene(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info));
            break;
        case DB_ACT_SCENE_ALL:
            mem_size = (pmsg->head.dev_count) * sizeof(HC_DEVICE_EXT_SCENE_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_scene_all((pmsg->head.dev_count), (HC_DEVICE_EXT_SCENE_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_SCENE_ALL_NUM:
            ret = dbd_get_scene_num_all(&(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_SCENE_SET:
            ret = dbd_set_scene(&(pmsg->body.device_info));
            break;
        case DB_ACT_SCENE_DEL:
            ret = dbd_del_scene(&(pmsg->body.device_info));
            break;
        case DB_ACT_SCENE_DEL_ALL:
            ret = dbd_del_scene_all();
            break;

        case DB_ACT_ADD_USER_LOG:
            ret = dbd_add_user_log(&(pmsg->body.device_info));
            break;
        case DB_ACT_GET_USER_LOG:
            mem_size = (pmsg->body.device_info.device.ext_u.get_user_log.log_num) * sizeof(HC_DEVICE_EXT_USER_LOG_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_user_log_all(&(pmsg->body.device_info), (HC_DEVICE_EXT_USER_LOG_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_GET_USER_LOG_NUM:
            ret = dbd_get_user_log_num_all(&(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;

        case DB_ACT_LOCATION_ADD:
            ret = dbd_add_location(&(pmsg->body.device_info));
            break;
        case DB_ACT_LOCATION_GET:
            ret = dbd_get_location(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info));
            break;
        case DB_ACT_LOCATION_ALL:
            mem_size = (pmsg->head.dev_count) * sizeof(HC_DEVICE_EXT_LOCATION_S);
            msg_ptr = (HC_MSG_S*)malloc(sizeof(HC_MSG_S) + mem_size);
            if (NULL != msg_ptr)
            {
                malloc_flag = 1;
                ret = dbd_get_location_all((pmsg->head.dev_count), (HC_DEVICE_EXT_LOCATION_S*)msg_ptr->data);
            }
            else
            {
                ret = DB_MALLOC_ERROR;
            }
            break;
        case DB_ACT_LOCATION_ALL_NUM:
            ret = dbd_get_location_num_all(&(hcmsg_resp.body.device_info.device.ext_u.record_num.value));
            break;
        case DB_ACT_LOCATION_SET:
            ret = dbd_set_location(&(pmsg->body.device_info));
            break;
        case DB_ACT_LOCATION_DEL:
            ret = dbd_del_location(&(pmsg->body.device_info));
            break;
        case DB_ACT_LOCATION_DEL_ALL:
            ret = dbd_del_location_all(&(pmsg->body.device_info));
            break;

        case DB_ACT_SET_CONF:
            ret = dbd_set_conf(&(pmsg->body.device_info));
            break;
        case DB_ACT_GET_CONF:
            ret = dbd_get_conf(&(pmsg->body.device_info), &(hcmsg_resp.body.device_info));
            break;

        default:
            DEBUG_ERROR("Unsupported db act [%d].\n", pmsg->head.db_act);
            return -1;
    }

    DEBUG_INFO("DB act ret [%d].\n", ret);

    if (DB_NO_CHANGE == ret)
    {
        hcmsg_resp.head.db_act_ret = HC_DB_ACT_NO_CHANGE;
    }
    else if (DB_RETVAL_OK == ret)
    {
        hcmsg_resp.head.db_act_ret = HC_DB_ACT_OK;
    }
    else
    {
        hcmsg_resp.head.db_act_ret = HC_DB_ACT_FAIL;
    }

    // send result msg
    hcmsg_resp.head.dst = pmsg->head.src;
    hcmsg_resp.head.src = pmsg->head.dst;
    hcmsg_resp.head.appid = pmsg->head.appid;

    if (malloc_flag == 1)
    {
        memcpy(msg_ptr, &hcmsg_resp, sizeof(HC_MSG_S));
        msg_ptr->head.data_len = mem_size;
    }
    else
    {
        hcmsg_resp.head.data_len = 0;
        msg_ptr = &hcmsg_resp;
    }

    ret_send = dbd_msg_send(msg_ptr);

    if (malloc_flag == 1)
    {
        free(msg_ptr);
    }
    return ret_send;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    HC_MSG_S *hcmsg = NULL;
    struct timeval tv;
    FILE *fp = NULL;
    int count = 0;

#ifdef CTRL_LOG_DEBUG
    ctrl_log_init(HC_CTRL_NAME);
#endif

    // initialize msg handle, comunicate with dispatch.
    ret = dbd_msg_init();
    if (ret != 0)
    {
        return -1;
    }

    /* Ignore broken pipes */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, dbd_signal_handle);
    signal(SIGTERM, dbd_signal_handle);

    db_init_db();

    fp = fopen(DB_STARTED_FILE, "w");
    if (fp == NULL)
    {
        DEBUG_ERROR("Create file %s failed, errno is '%d'.\n",
                    DB_STARTED_FILE, errno);
        dbd_msg_uninit();
        return -1;
    }
    fclose(fp);

    // man loop.
    while (keep_looping)
    {
        count++;
        if (count > DB_SYNC_INTERVAL)
        {
            dbd_sync_db();
            count = 0;
        }
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
            DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
            hc_msg_free(hcmsg);
            dbd_msg_uninit();
            dbd_msg_init();
            continue;
        }

        dbd_process_msg(hcmsg);

        hc_msg_free(hcmsg);

    }

EXIT:
    unlink(DB_STARTED_FILE);
    /* process exit. */
    dbd_msg_uninit();

    return 0;
}
