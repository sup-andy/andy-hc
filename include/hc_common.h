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
 *   File   : hc_common.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _HC_COMMON_H_
#define _HC_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define DISPATCHER_STARTED_FILE     "/var/dispatcher_started"
#define DB_STARTED_FILE              "/var/db_started"
#define CAMERA_STARTED_FILE         "/var/camera_started"

#define COMBINE_GLOBAL_ID(type, id) ((type << 24) | id)
#define GLOBAL_ID_TO_DEV_ID(id) (id & 0x00FFFFFF)
#define GLOBAL_ID_TO_NETWORK_TYPE(id) ((id >> 24) & 0xFF)

#define CTRL_LOG_DEBUG

#ifdef CTRL_LOG_DEBUG
#include "capi_struct.h"
#include "ctrl_common_lib.h"

#define HC_CTRL_NAME    HOMECTRL_CTRL_NAME

#define DEBUG_INFO(format, ...)  ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define DEBUG_ERROR(format, ...) ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define DEBUG_INFO  printf
#define DEBUG_ERROR printf
#endif




#ifdef __cplusplus
}
#endif

#endif

