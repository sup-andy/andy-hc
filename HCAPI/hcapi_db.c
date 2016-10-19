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
*   File   : hcapi_db.c
*   Abstract:
*   Date   : 11/26/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Rose  06/03/2013 1.0  Initial Dev.
*   Sharon 11/26/2015 1.1 Distinguish APIs from HCAPI & Database
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "hcapi.h"
#include "hcapi_db.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "hc_common.h"

#include "sql_api.h"

static int hcapi_check_db_started(void)
{
    if (0 == access(DB_STARTED_FILE, F_OK))
    {
        unlink(DB_STARTED_FILE);
        return 0;
    }
    return -1;
}

HC_RETVAL_E hcapi_db_related_init()
{
    FILE *fp = NULL;
    int wait_count = 0;

    // initialize database
    db_init_db();

    fp = fopen(DB_STARTED_FILE, "w");
    if (fp == NULL)
    {
        DEBUG_ERROR("Create file %s failed, errno is '%d'.\n", DB_STARTED_FILE, errno);
        return HC_RET_FAILURE;
    }
    fclose(fp);

    wait_count = 0;
    while (0 != hcapi_check_db_started())
    {
        if (wait_count++ > 50)
        {
            wait_count = 0;
        }
    
        usleep(200 * 1000);
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_add_dev(HC_DEVICE_INFO_S *devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_set_dev(devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_set_dev id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    /* update device connection status */
    ret = hcapi_db_set_connection(devs_info->dev_id, "online");
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Fail to set connection status, dev_id[%x], ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_dev(HC_DEVICE_INFO_S *devs_info)
{
    HC_MSG_S msg;
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_del_dev(devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_dev id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_set_dev(HC_DEVICE_INFO_S *devs_info)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_tmp;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_tmp, 0, sizeof(HC_DEVICE_INFO_S));

    ret = dbd_get_dev(devs_info, &devs_tmp);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_dev id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    if (0 == memcmp(&devs_tmp.device, &(devs_info->device), sizeof(HC_DEVICE_U)))
    {
        DEBUG_INFO("DB Info DB_ACT_SET_DEV with no change.\n");
        ret = DB_NO_CHANGE;
        return HC_RET_SUCCESS;
    }

    ret = dbd_set_dev(devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_set_dev id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_by_dev_id(HC_DEVICE_INFO_S* devs_info)
{
    int ret = 0;
    HC_DEVICE_INFO_S devinfo_resp;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memcpy(&devinfo_resp, devs_info, sizeof(HC_DEVICE_INFO_S));
    ret = dbd_get_dev(devs_info, &devinfo_resp);    

    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_dev id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }
    memcpy(devs_info, &devinfo_resp, sizeof(HC_DEVICE_INFO_S));

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_all(int dev_count, HC_DEVICE_INFO_S* devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_all(dev_count, devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_all id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_num_all(int *size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_num_all(size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_num_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_by_dev_type(HC_DEVICE_TYPE_E dev_type, int dev_count, HC_DEVICE_INFO_S* devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_by_dev_type(dev_type, dev_count, devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_by_dev_type id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_num_by_dev_type(dev_type, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_num_by_dev_type ret(%d); dev_type(%d); size(%d)\n", ret, dev_type, *size);
        *size = 0;
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_by_network_type(HC_NETWORK_TYPE_E network_type, int dev_count, HC_DEVICE_INFO_S* devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_by_network_type(network_type, dev_count, devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_by_network_type dev_count [%x] ret [%d].\n", dev_count, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_num_by_network_type(network_type, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_num_by_network_type ret(%d); size(%d)\n", ret, *size);
        *size = 0;
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int dev_count, HC_DEVICE_INFO_S* devs_info)
{
    int ret = 0;

    if ((devs_info == NULL) || (location == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_get_dev_by_dev_type_and_location(dev_type, location, devs_info, dev_count);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error db_get_dev_by_dev_type_and_location ,ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_num_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int *size)
{
    int ret = 0;

    if ((size == NULL) || (location == NULL))
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_get_dev_num_by_dev_type_and_location(dev_type, location, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error db_get_dev_num_by_dev_type_and_location ret(%d); dev_type(%d); size(%d)\n", ret, dev_type, *size);
        *size = 0;
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_dev_all()
{
    int ret = 0;

    ret = dbd_del_dev_all();
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_dev_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_add_dev_log(HC_DEVICE_INFO_S* devs_info, char* log, long log_time, int alarm_type, char* name)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    devs_info->device.ext_u.add_log.dev_id = devs_info->dev_id;
    devs_info->device.ext_u.add_log.dev_type = devs_info->dev_type;
    devs_info->device.ext_u.add_log.network_type = devs_info->network_type;
    devs_info->device.ext_u.add_log.event_type = devs_info->event_type;

    memset(devs_info->device.ext_u.add_log.log, 0, SQL_LOG_LEN);
    strncpy(devs_info->device.ext_u.add_log.log, log, (SQL_LOG_LEN - 1));
    devs_info->device.ext_u.add_log.log_time = log_time;
    devs_info->device.ext_u.add_log.alarm_type = alarm_type;
    memset(devs_info->device.ext_u.add_log.log_name, 0, SQL_SCENE_LEN);
    strncpy(devs_info->device.ext_u.add_log.log_name, name, (SQL_SCENE_LEN - 1));

    ret = dbd_add_log(devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_add_log id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_log_all(int size, char* log_ptr)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    ret = dbd_get_log_all(&devs_info_tmp, log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_log_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_num_all(int *size)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_log_num(size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_log_num ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_log_by_dev_id(int size, char* log_ptr, unsigned int dev_id)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.dev_id = dev_id;

    ret = dbd_get_dev_log_by_dev_id_v2(&devs_info_tmp, (HC_DEVICE_EXT_LOG_S*)log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_log_by_dev_id_v2 ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_num_by_dev_id(unsigned int dev_id, int *size)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_log_num_by_dev_id(dev_id, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d] size[%d].\n", ret, *size);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_dev_type(int size, char* log_ptr, HC_DEVICE_TYPE_E dev_type)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.dev_type = dev_type;
    ret = dbd_get_dev_log_by_dev_type_v2(&devs_info_tmp, (HC_DEVICE_EXT_LOG_S*)log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_dev_log_num_by_dev_type(dev_type, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_network_type(int size, char* log_ptr, HC_NETWORK_TYPE_E network_type)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.network_type = network_type;
    ret = dbd_get_dev_log_by_network_type_v2(&devs_info_tmp, (HC_DEVICE_EXT_LOG_S*)log_ptr);

    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_dev_log_by_network_type_v2 ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (network_type <= 0 || network_type >= HC_NETWORK_TYPE_MAX)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    if (size == NULL)
    {
        return HC_RET_FAILURE;
    }

    ret = dbd_get_dev_log_num_by_network_type(network_type, size);

    if (DB_RETVAL_OK != ret)
    {
        *size = 0;
        DEBUG_ERROR("DB Error hcapi_db_get_dev_log_num_by_network_type ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_time(int size, char* log_ptr, long start_time, long end_time)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.start_time = start_time;
    devs_info_tmp.device.ext_u.get_log.end_time = end_time;
    ret = dbd_get_dev_log_by_by_time_v2(&devs_info_tmp, (HC_DEVICE_EXT_LOG_S*)log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_num_by_time(int *size, long start_time, long end_time)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.start_time = start_time;
    devs_info_tmp.device.ext_u.get_log.end_time = end_time;
    ret = dbd_get_dev_log_num_by_time(&devs_info_tmp, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_alarm_type(int size, char* log_ptr, int alarm_type)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.alarm_type = alarm_type;
    ret = dbd_get_dev_log_by_alarm_type(&devs_info_tmp, (char*)log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_num_by_alarm_type(int *size, int alarm_type)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.alarm_type = alarm_type;
    ret = dbd_get_dev_log_num_by_alarm_type(&devs_info_tmp, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_log_all()
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    ret = dbd_del_dev_log_all();
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_dev_log_by_dev_id(unsigned int dev_id)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    ret = dbd_del_dev_log_by_dev_id(dev_id);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_set_dev_name(unsigned int dev_id, char *dev_name)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (dev_name == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_update_devname_by_dev_id(dev_id, dev_name);
    if (ret != DB_RETVAL_OK)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_DB_OPERATION_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_add_dev_log_v2(HC_DEVICE_EXT_LOG_S* log_ptr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    memcpy(&(devs_info_tmp.device.ext_u.add_log), log_ptr, sizeof(HC_DEVICE_EXT_LOG_S));
    ret = dbd_add_log_v2(&devs_info_tmp);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Send DB command fail.\n");
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}


HC_RETVAL_E hcapi_db_get_log_all_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    ret = dbd_get_log_all_v2(&devs_info_tmp, log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_log_by_dev_id_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, unsigned int dev_id)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.dev_id = dev_id;
    ret = dbd_get_dev_log_by_dev_id_v2(&devs_info_tmp, log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_dev_type_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, HC_DEVICE_TYPE_E dev_type)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.dev_type = dev_type;
    ret = dbd_get_dev_log_by_dev_type_v2(&devs_info_tmp, log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_network_type_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, HC_NETWORK_TYPE_E network_type)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.network_type = network_type;
    ret = dbd_get_dev_log_by_network_type_v2(&devs_info_tmp, log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_dev_log_by_time_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, long start_time, long end_time)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_log.log_num = size;
    devs_info_tmp.device.ext_u.get_log.start_time = start_time;
    devs_info_tmp.device.ext_u.get_log.end_time = end_time;
    ret = dbd_get_dev_log_by_by_time_v2(&devs_info_tmp, log_ptr);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}


HC_RETVAL_E hcapi_db_add_attr(unsigned int dev_id, char* attr_name, char* attr_value)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.attr.dev_id = dev_id;
    memset(devs_info_tmp.device.ext_u.attr.attr_name, 0, ATTR_NAME_LEN);
    strncpy(devs_info_tmp.device.ext_u.attr.attr_name, attr_name, ATTR_NAME_LEN - 1);
    devs_info_tmp.device.ext_u.attr.attr_name[ATTR_NAME_LEN - 1] = '\0';
    memset(devs_info_tmp.device.ext_u.attr.attr_value, 0, ATTR_VALUE_LEN);
    strncpy(devs_info_tmp.device.ext_u.attr.attr_value, attr_value, ATTR_VALUE_LEN - 1);
    devs_info_tmp.device.ext_u.attr.attr_value[ATTR_VALUE_LEN - 1] = '\0';
    ret = dbd_add_attr(&devs_info_tmp);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_attr(unsigned int dev_id, char *attr_name, char *attr_value)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (attr_value == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.attr.dev_id = dev_id;
    memset(devs_info_tmp.device.ext_u.attr.attr_name, 0, ATTR_NAME_LEN);
    strncpy(devs_info_tmp.device.ext_u.attr.attr_name, attr_name, ATTR_NAME_LEN - 1);
    devs_info_tmp.device.ext_u.attr.attr_name[ATTR_NAME_LEN - 1] = '\0';

    ret = dbd_get_attr(&devs_info_tmp, &devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }
    strncpy(attr_value, devs_info.device.ext_u.attr.attr_value, strlen(devs_info.device.ext_u.attr.attr_value));

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_attr_by_dev_id(unsigned int dev_id, int size, HC_DEVICE_EXT_ATTR_S * attr_ptr)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (attr_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_attr_all_by_id(dev_id, attr_ptr, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_attr_num_by_dev_id(unsigned int dev_id, int* size)
{
    //SHRAON: This API did not use currently.
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_attr_num_by_dev_id(dev_id, size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}


HC_RETVAL_E hcapi_db_set_attr(unsigned int dev_id, char *attr_name, char *attr_value)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;
    HC_DEVICE_INFO_S old_device_info;

    if (attr_name == NULL || attr_value == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(&old_device_info, 0x0, sizeof(old_device_info));
    devs_info.device.ext_u.attr.dev_id = dev_id;
    memset(devs_info.device.ext_u.attr.attr_name, 0, ATTR_NAME_LEN);
    strncpy(devs_info.device.ext_u.attr.attr_name, attr_name, ATTR_NAME_LEN - 1);
    devs_info.device.ext_u.attr.attr_name[ATTR_NAME_LEN - 1] = '\0';
    memset(devs_info.device.ext_u.attr.attr_value, 0, ATTR_VALUE_LEN);
    strncpy(devs_info.device.ext_u.attr.attr_value, attr_value, ATTR_VALUE_LEN - 1);
    devs_info.device.ext_u.attr.attr_value[ATTR_VALUE_LEN - 1] = '\0';

    ret = dbd_get_attr(&devs_info, &old_device_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_attr id [%x] ret [%d].\n", devs_info.dev_id, ret);
        return HC_RET_FAILURE;
    }
    
    if (0 == memcmp(&old_device_info.device.ext_u.attr,
            &(devs_info.device.ext_u.attr),
            sizeof(HC_DEVICE_EXT_ATTR_S)))
    {
        DEBUG_INFO("DB Info DB_ACT_ATTR_SET with no change.\n");
        ret = DB_NO_CHANGE;
        return HC_RET_SUCCESS;
    }
    
    ret = dbd_set_attr(&devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_attr(unsigned int dev_id, char *attr_name)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (attr_name == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info.device.ext_u.attr.dev_id = dev_id;
    memset(devs_info.device.ext_u.attr.attr_name, 0, ATTR_NAME_LEN);
    strncpy(devs_info.device.ext_u.attr.attr_name, attr_name, ATTR_NAME_LEN - 1);
    devs_info.device.ext_u.attr.attr_name[ATTR_NAME_LEN - 1] = '\0';

    ret = dbd_del_attr(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_attr id [%x] ret [%d].\n", devs_info.dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_attr_by_dev_id(unsigned int dev_id)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info.device.ext_u.attr.dev_id = dev_id;

    ret = dbd_del_attr_by_dev_id(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_attr_by_dev_id id [%x] ret [%d].\n", devs_info.dev_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_attr_all(void)
{
    int ret = 0;

    ret = dbd_del_attr_all();
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_attr_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_add_scene(char* scene_id, char* scene_attr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (scene_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info.device.ext_u.scene.scene_id, 0, SCENE_ID_LEN);
    strncpy(devs_info.device.ext_u.scene.scene_id, scene_id, SCENE_ID_LEN - 1);
    devs_info.device.ext_u.scene.scene_id[SCENE_ID_LEN - 1] = '\0';
    memset(devs_info.device.ext_u.scene.scene_attr, 0, SCENE_ATTR_LEN);
    strncpy(devs_info.device.ext_u.scene.scene_attr, scene_attr, SCENE_ATTR_LEN - 1);
    devs_info.device.ext_u.scene.scene_attr[SCENE_ATTR_LEN - 1] = '\0';

    ret = dbd_add_scene(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_add_scene ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_scene(char* scene_id, char *scene_attr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (scene_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info_tmp.device.ext_u.scene.scene_id, 0, SCENE_ID_LEN);
    strncpy(devs_info_tmp.device.ext_u.scene.scene_id, scene_id, SCENE_ID_LEN - 1);
    devs_info_tmp.device.ext_u.scene.scene_id[SCENE_ID_LEN - 1] = '\0';

    ret = dbd_get_scene(&devs_info_tmp, &devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_scene ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }
    strncpy(scene_attr, devs_info.device.ext_u.scene.scene_attr, SCENE_ATTR_LEN);

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S * scene_ptr)
{
    int ret = 0;

    if (scene_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_scene_all(size, scene_ptr);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_scene_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_scene_num_all(int* size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_scene_num_all(size);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_scene_num_all ret [%d].\n", ret);
        *size = 0;
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}


HC_RETVAL_E hcapi_db_set_scene(char* scene_id, char *scene_attr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (scene_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info.device.ext_u.scene.scene_id, 0, SCENE_ID_LEN);
    strncpy(devs_info.device.ext_u.scene.scene_id, scene_id, SCENE_ID_LEN - 1);
    devs_info.device.ext_u.scene.scene_id[SCENE_ID_LEN - 1] = '\0';
    memset(devs_info.device.ext_u.scene.scene_attr, 0, SCENE_ATTR_LEN);
    strncpy(devs_info.device.ext_u.scene.scene_attr, scene_attr, SCENE_ATTR_LEN - 1);
    devs_info.device.ext_u.scene.scene_attr[SCENE_ATTR_LEN - 1] = '\0';

    ret = dbd_set_scene(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_set_scene ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_scene(char* scene_id)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (scene_id == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info.device.ext_u.scene.scene_id, 0, SCENE_ID_LEN);
    strncpy(devs_info.device.ext_u.scene.scene_id, scene_id, SCENE_ID_LEN - 1);
    devs_info.device.ext_u.scene.scene_id[SCENE_ID_LEN - 1] = '\0';

    ret = dbd_del_scene(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_scene ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_scene_all(void)
{
    int ret = 0;

    ret = dbd_del_scene_all();
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_scene_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_add_user_log(HC_USER_LOG_SEVERITY_E severity, char *content)
{
    int ret = 0;

    if (content == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_add_user_log(severity, content);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_add_user_log ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}


HC_RETVAL_E hcapi_db_get_user_log_all(int size, HC_DEVICE_EXT_USER_LOG_S* log_ptr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (log_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.get_user_log.log_num = size;

    ret = dbd_get_user_log_all(&devs_info_tmp, log_ptr);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_user_log_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_user_log_num_all(int *size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_user_log_num_all(size);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_user_log_num_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_add_location(char* location_id, char* location_attr)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (location_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info.device.ext_u.location.location_id, 0, LOCATION_ID_LEN);
    strncpy(devs_info.device.ext_u.location.location_id, location_id, LOCATION_ID_LEN - 1);
    devs_info.device.ext_u.location.location_id[LOCATION_ID_LEN - 1] = '\0';
    memset(devs_info.device.ext_u.location.location_attr, 0, LOCATION_ATTR_LEN);
    strncpy(devs_info.device.ext_u.location.location_attr, location_attr, LOCATION_ATTR_LEN - 1);
    devs_info.device.ext_u.location.location_attr[LOCATION_ATTR_LEN - 1] = '\0';

    ret = dbd_add_location(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_add_location ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_location(char* location_id, char *location_attr, int size)
{
    //SHRAON: This API did not use currently.
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (location_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info_tmp.device.ext_u.location.location_id, 0, LOCATION_ID_LEN);
    strncpy(devs_info_tmp.device.ext_u.location.location_id, location_id, LOCATION_ID_LEN - 1);
    devs_info_tmp.device.ext_u.location.location_id[LOCATION_ID_LEN - 1] = '\0';

    ret = dbd_get_location(&devs_info_tmp, &devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_location ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }
    memset(location_attr, 0, size);
    strncpy(location_attr, devs_info.device.ext_u.location.location_attr, size - 1);

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S * location_ptr)
{
    int ret = 0;

    if (location_ptr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_location_all(size, location_ptr);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_location_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_location_num_all(int* size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_get_location_num_all(size);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_location_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}


HC_RETVAL_E hcapi_db_set_location(char* location_id, char *location_attr)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;
    unsigned int dev_id = 0;

    if (location_attr == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    dev_id = atoi(location_id);

    ret = db_update_location_by_dev_id(dev_id, location_attr);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_set_location ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_location(char* location_id)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (location_id == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info.device.ext_u.location.location_id, 0, LOCATION_ID_LEN);
    strncpy(devs_info.device.ext_u.location.location_id, location_id, LOCATION_ID_LEN - 1);
    devs_info.device.ext_u.location.location_id[LOCATION_ID_LEN - 1] = '\0';

    ret = dbd_del_location(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_location ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_location_all()
{
    int ret = 0;

    ret = dbd_del_location_all();
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_location_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_set_conf(char* conf_name, char* conf_value)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;

    if (conf_value == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info.device.ext_u.conf.conf_name, 0, CONF_NAME_LEN);
    strncpy(devs_info.device.ext_u.conf.conf_name, conf_name, CONF_NAME_LEN - 1);
    devs_info.device.ext_u.conf.conf_name[CONF_NAME_LEN - 1] = '\0';
    memset(devs_info.device.ext_u.conf.conf_value, 0, CONF_VALUE_LEN);
    strncpy(devs_info.device.ext_u.conf.conf_value, conf_value, CONF_VALUE_LEN - 1);
    devs_info.device.ext_u.conf.conf_value[CONF_VALUE_LEN - 1] = '\0';

    ret = dbd_set_conf(&devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_set_conf ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_conf(char* conf_name, char *conf_value)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (conf_value == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info, 0, sizeof(HC_DEVICE_INFO_S));
    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    memset(devs_info_tmp.device.ext_u.conf.conf_name, 0, CONF_NAME_LEN);
    strncpy(devs_info_tmp.device.ext_u.conf.conf_name, conf_name, CONF_NAME_LEN - 1);
    devs_info_tmp.device.ext_u.conf.conf_name[CONF_NAME_LEN - 1] = '\0';

    ret = dbd_get_conf(&devs_info_tmp, &devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_get_conf ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }
    strncpy(conf_value, devs_info.device.ext_u.conf.conf_value, CONF_VALUE_LEN);

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_set_connection(unsigned int dev_id, char *connection)
{
    int ret = 0;
    HC_DEVICE_INFO_S devs_info_tmp;

    if (connection == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memset(&devs_info_tmp, 0, sizeof(HC_DEVICE_INFO_S));
    devs_info_tmp.device.ext_u.attr.dev_id = dev_id;
    memset(devs_info_tmp.device.ext_u.attr.attr_name, 0, ATTR_NAME_LEN);
    strncpy(devs_info_tmp.device.ext_u.attr.attr_name, "connection", ATTR_NAME_LEN - 1);
    devs_info_tmp.device.ext_u.attr.attr_name[ATTR_NAME_LEN - 1] = '\0';
    memset(devs_info_tmp.device.ext_u.attr.attr_value, 0, ATTR_VALUE_LEN);
    strncpy(devs_info_tmp.device.ext_u.attr.attr_value, connection, ATTR_VALUE_LEN - 1);
    devs_info_tmp.device.ext_u.attr.attr_value[ATTR_VALUE_LEN - 1] = '\0';
    ret = dbd_update_connection(&devs_info_tmp);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}
/***********************API for get data from db daemon************************/
