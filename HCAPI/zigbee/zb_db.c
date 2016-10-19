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
#include "zigbee_msg.h"

extern DB_RETVAL_E dbd_add_zb_dev(ZB_DEVICE_INFO_S *devs_info);
extern DB_RETVAL_E dbd_del_zb_dev_by_dev_id(unsigned int dev_id);
extern DB_RETVAL_E dbd_reset_zb_dev(void);
extern DB_RETVAL_E dbd_get_zb_dev_by_dev_id(ZB_DEVICE_INFO_S * dev_info, ZB_DEVICE_INFO_S* dev_ptr);

HC_RETVAL_E hcapi_db_add_zb_dev(ZB_DEVICE_INFO_S *devs_info)
{
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_add_zb_dev(devs_info);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error sql_add_zb_dev id [%x] ret [%d].\n", devs_info->extend_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_del_zb_dev(ZB_DEVICE_INFO_S *devs_info)
{
    HC_MSG_S msg;
    int ret = 0;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    ret = dbd_del_zb_dev_by_dev_id(devs_info->extend_id);
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_zb_dev id [%x] ret [%d].\n", devs_info->extend_id, ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_reset_zb_dev(void)
{
    int ret = 0;

    ret = dbd_reset_zb_dev();
    if (DB_RETVAL_OK != ret)
    {
        DEBUG_ERROR("DB Error dbd_del_zb_dev_all ret [%d].\n", ret);
        return HC_RET_FAILURE;
    }

    return HC_RET_SUCCESS;
}

HC_RETVAL_E hcapi_db_get_zb_dev_by_dev_id(ZB_DEVICE_INFO_S* devs_info)
{
    int ret = 0;
    ZB_DEVICE_INFO_S devinfo_resp;

    if (devs_info == NULL)
    {
        return HC_RET_INVALID_ARGUMENTS;
    }

    memcpy(&devinfo_resp, devs_info, sizeof(ZB_DEVICE_INFO_S));
    ret = dbd_get_zb_dev_by_dev_id(devs_info, &devinfo_resp);    

    if (DB_RETVAL_OK == ret) {
        ctrl_log_print(LOG_INFO, __func__, __LINE__, "Get ZB device info[dev_id : %x]", devs_info->extend_id);
    } else {
        ctrl_log_print(LOG_ERR, __func__, __LINE__, "DB Error dbd_get_dev id [%x] ret [%d].\n", devs_info->dev_id, ret);
        return HC_RET_FAILURE;
    }
    memcpy(devs_info, &devinfo_resp, sizeof(ZB_DEVICE_INFO_S));

    return HC_RET_SUCCESS;
}

/***********************API for get data from db daemon************************/
