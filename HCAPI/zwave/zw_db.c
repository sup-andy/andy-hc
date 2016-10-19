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
*   Creator : Gene Chin
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
#include "zwave_api.h"


extern DB_RETVAL_E db_add_zw_dev(ZW_DEVICE_INFO *devs_info, int size);
extern DB_RETVAL_E db_get_zw_dev_all(ZW_DEVICE_INFO *devs_info, int size);
extern DB_RETVAL_E db_get_zw_dev_all_num(int *dev_num);
extern DB_RETVAL_E db_get_zw_dev_by_dev_id(unsigned int dev_id, ZW_DEVICE_INFO *devs_info);
extern DB_RETVAL_E db_del_zw_dev_by_dev_id(unsigned int dev_id);
extern DB_RETVAL_E db_reset_zw_dev(void);

HC_RETVAL_E hcapi_db_add_zw_dev(ZW_DEVICE_INFO *devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_add_zw_dev(devs_info, 1);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error db_add_zw_dev id [%x] ret [%d].\n", devs_info->id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_zw_dev(ZW_DEVICE_INFO *devs_info)
{
    HC_MSG_S msg;
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_del_zw_dev_by_dev_id(devs_info->id);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error db_del_zw_dev_by_dev_id id [%x] ret [%d].\n", devs_info->id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_reset_zw_dev(void)
{
    HC_MSG_S msg;
    int ret = 0;

    ret = db_reset_zw_dev();
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error db_reset_zw_dev ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_zw_dev_all(int dev_count, ZW_DEVICE_INFO* devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_get_zw_dev_all(dev_count, devs_info);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error db_get_zw_dev_all count [%d] ret [%d].\n", dev_count, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_zw_dev_all_num(int *size)
{
    int ret = 0;

    if (size == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = db_get_zw_dev_all_num(size);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("DB Error db_get_zw_dev_all_num ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_zw_dev_by_dev_id(ZW_DEVICE_INFO *devs_info)
{
    int ret = 0;
    ZW_DEVICE_INFO devinfo_resp;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memcpy(&devinfo_resp, devs_info, sizeof(ZW_DEVICE_INFO));
    ret = db_get_zw_dev_by_dev_id(devs_info->id, &devinfo_resp);    

    if (DB_RETVAL_OK == ret) {
        DEBUG_INFO("Get ZB device info[dev_id : %x]", devs_info->id);
    } else {
        DEBUG_ERROR("DB Error dbd_get_dev id [%x] ret [%d].\n", devs_info->id, ret);
        return HC_RET_FAILURE;
    }
    memcpy(devs_info, &devinfo_resp, sizeof(ZW_DEVICE_INFO));

    return HC_RET_SUCCESS;
}


