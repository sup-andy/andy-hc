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
 *   File   : hcapi_db.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _HCAPI_DB_H_
#define _HCAPI_DB_H_
#include <time.h>
#include "hcapi.h"


#ifdef __cplusplus
extern "C"
{
#endif

//doxygen coding  style

/**
* @addtogroup HCAPI_DB
* APIs for accessed DB api.
*/

/** @{ */

/**
********************************************************************************
*  Process home control messages to DB library
*  \param pmsg The message contain.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_related_init();

/***********************************Declaration********************************/
HC_RETVAL_E hcapi_db_get_dev_log_num_all(int *size);
HC_RETVAL_E hcapi_db_get_dev_log_num_by_dev_id(unsigned int dev_id, int *size);
HC_RETVAL_E hcapi_db_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size);
HC_RETVAL_E hcapi_db_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size);
HC_RETVAL_E hcapi_db_add_dev_log_v2(HC_DEVICE_EXT_LOG_S* log_ptr);
HC_RETVAL_E hcapi_db_get_log_all_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr);
HC_RETVAL_E hcapi_db_get_log_by_dev_id_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, unsigned int dev_id);
HC_RETVAL_E hcapi_db_get_dev_log_by_dev_type_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, HC_DEVICE_TYPE_E dev_type);
HC_RETVAL_E hcapi_db_get_dev_log_by_network_type_v2(int size, HC_DEVICE_EXT_LOG_S* log_ptr, HC_NETWORK_TYPE_E network_type);
HC_RETVAL_E hcapi_db_set_dev(HC_DEVICE_INFO_S *devs_info);
HC_RETVAL_E hcapi_db_del_dev(HC_DEVICE_INFO_S *devs_info);
HC_RETVAL_E hcapi_db_del_dev_all();
HC_RETVAL_E hcapi_db_add_attr(unsigned int dev_id, char *name, char* value);
HC_RETVAL_E hcapi_db_get_attr(unsigned int dev_id, char *name, char* value);
HC_RETVAL_E hcapi_db_get_attr_by_dev_id(unsigned int dev_id, int size, HC_DEVICE_EXT_ATTR_S * attr_ptr);
HC_RETVAL_E hcapi_db_get_attr_num_by_dev_id(unsigned int dev_id, int *size);
HC_RETVAL_E hcapi_db_set_attr(unsigned int dev_id, char *name, char* value);
HC_RETVAL_E hcapi_db_del_attr(unsigned int dev_id, char *name);
HC_RETVAL_E hcapi_db_del_attr_by_dev_id(unsigned int dev_id);
HC_RETVAL_E hcapi_db_del_attr_all(void);
HC_RETVAL_E hcapi_db_add_scene(char* scene_id, char *scene_attr);
HC_RETVAL_E hcapi_db_get_scene(char* scene_id, char *scene_attr);
HC_RETVAL_E hcapi_db_set_scene(char* scene_id, char *scene_attr);
HC_RETVAL_E hcapi_db_del_scene(char* scene_id);
HC_RETVAL_E hcapi_db_get_scene_num_all(int *size);
HC_RETVAL_E hcapi_db_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S * scene_ptr);
HC_RETVAL_E hcapi_db_del_scene_all(void);
HC_RETVAL_E hcapi_db_add_user_log(HC_USER_LOG_SEVERITY_E severity, char *content);
HC_RETVAL_E hcapi_db_get_user_log_all(int size, HC_DEVICE_EXT_USER_LOG_S* log_ptr);
HC_RETVAL_E hcapi_db_get_user_log_num_all(int *size);
HC_RETVAL_E hcapi_db_get_dev_by_dev_type(HC_DEVICE_TYPE_E dev_type, int dev_count, HC_DEVICE_INFO_S* devs_info);
HC_RETVAL_E hcapi_db_get_dev_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size);
HC_RETVAL_E hcapi_db_get_dev_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int dev_count, HC_DEVICE_INFO_S* devs_info);
HC_RETVAL_E hcapi_db_get_dev_num_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int *size);
HC_RETVAL_E hcapi_db_get_dev_by_network_type(HC_NETWORK_TYPE_E network_type, int dev_count, HC_DEVICE_INFO_S* devs_info);
HC_RETVAL_E hcapi_db_get_dev_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size);
HC_RETVAL_E hcapi_db_add_location(char* location_id, char *location_attr);
HC_RETVAL_E hcapi_db_get_location(char* location_id, char *location_attr, int size);
HC_RETVAL_E hcapi_db_set_location(char* location_id, char *location_attr);
HC_RETVAL_E hcapi_db_del_location(char* location_id);
HC_RETVAL_E hcapi_db_get_location_num_all(int *size);
HC_RETVAL_E hcapi_db_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S * location_ptr);
HC_RETVAL_E hcapi_db_del_location_all();
HC_RETVAL_E hcapi_db_set_conf(char* conf_name, char *conf_value);
HC_RETVAL_E hcapi_db_get_conf(char* conf_name, char *conf_value);
HC_RETVAL_E hcapi_db_set_connection(unsigned int dev_id, char *connection);


/**
********************************************************************************
*  Add device to DB.
*  \param devs_info The device information.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_add_dev(HC_DEVICE_INFO_S *devs_info);

/**
********************************************************************************
*  Get device info by specified device ID.
*  \param dev_id Device ID.
*  \param devs_info The device information of the specified device.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_by_dev_id(HC_DEVICE_INFO_S* devs_info);

/**
********************************************************************************
*  Get all devices from DB.
*  \param size The number of all devices.
*  \param devs_info It is array buffer includes all device information.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_all(int size, HC_DEVICE_INFO_S* devs_info);

/**
********************************************************************************
*  Get the number of all devices in DB.
*  \param[out] size The number of devices.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_num_all(int *size);

/**
********************************************************************************
*  Get the same network type related logs.
*  \param size The number of log.
*  \param log_ptr The buffer store logs.
*  \param network_type The network type.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_log_by_network_type(int size, char* log_ptr, HC_NETWORK_TYPE_E network_type);

/**
********************************************************************************
*  Get the number of devices with same network type in DB.
*  \param network_type Network type.
*  \param[out] size The number of devices.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size);

/**
********************************************************************************
*  Get the nubmer of logs with the same device type.
*  \param dev_type The device type.
*  \param size The size stroe the number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size);

/**
********************************************************************************
*  Get the same device type related logs.
*  \param size The number of log.
*  \param log_ptr The buffer store logs.
*  \param dev_type The device type.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_log_by_dev_type(int size, char* log_ptr, HC_DEVICE_TYPE_E dev_type);

/**
********************************************************************************
*  Get the nubmer of logs with the same device ID.
*  \param dev_id The device ID.
*  \param size The size stroe the number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_dev_log_num_by_dev_id(unsigned int dev_id, int *size);

/**
********************************************************************************
*  Get all of logs from DB.
*  \param size The number of log.
*  \param log_ptr The buffer store logs.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_log_all(int size, char* log_ptr);

/**
********************************************************************************
*  Get the same device ID related logs.
*  \param size The number of log.
*  \param log_ptr The log_ptr store logs.
*  \param dev_id The device ID.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_get_log_by_dev_id(int size, char* log_ptr, unsigned int dev_id);

/**
********************************************************************************
*  Add log to DB.
*  \param devs_info The device information of the specified device.
*  \param log The log content.
*  \param log_time The log occured time.
*  \param alarm_type The alarm type.
*  \param name The log name.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_db_add_dev_log(HC_DEVICE_INFO_S* devs_info, char* log, long log_time, int alarm_type, char* name);

/** @} */

#ifdef __cplusplus
}
#endif


#endif

