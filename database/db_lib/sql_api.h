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
*   Creator : Mark Wen
*   File   : sql_api.h
*   Abstract:
*   Date   : 03/06/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Mark.W  06/03/2015 0.1  Initial Dev.
*
******************************************************************************/

/****************************DEFINITIONS***************************************/
//#define DB_PATH "/var/run/device.db"
#define DB_STORAGE_PATH "/storage/config/homectrl/device.db"

//#define LOG_BUF_LEN 192
//#define LOG_MAX_NUM 30
//#define ARMMODE_ID_LENGTH 32
//#define SQL_LOG_LEN 256
//#define SQL_SCENE_LEN 512
//#define DEVICE_NAME_LEN 32
//#define ARMMODE_DEV_ID 23456
//#define LOG_BUF_LEN 192
//#define TIME_STR_LEN 24
//#define ATTR_NAME_LEN 32
//#define ATTR_VALUE_LEN 128
//#define SCENE_ATTR_LEN 1024
//#define LOG_EXT_LEN 256
//#define SCENE_ID_LEN 64
//#define CONF_NAME_LEN 32
//#define CONF_VALUE_LEN 128


#define MAX_LOG_NUM 200
#define MAX_USER_LOG_NUM 1000

#include "hcapi.h"
#ifdef ZB_SQL_SUPPORT
#include "zigbee_msg.h"
#endif

typedef enum
{
    DB_RETVAL_OK = 0,
    DB_NO_RECORD,
    DB_HAS_RECORD,
    DB_GET_RECORD_NUM_FAIL,
    DB_INIT_FAIL,
    DB_PARAM_ERROR,

    DB_OPEN_ERROR,
    DB_CLOSE_ERROR,

    DB_MALLOC_ERROR,
    DB_DEV_SIZE_ERROR,
    DB_EXEC_ERROR,
    DB_ADD_LOG_FAIL,

    DB_ADD_NAME_FAIL,

    DB_TABLE_EXIST,

    DB_NO_DEVICE,
    DB_NO_CHANGE,

    DB_WRONG_DEV_ID,

    DB_ADD_DEV_FAIL,
    DB_GET_NUM_FAIL,
    DB_GET_DEV_FAIL,
    DB_GET_LOG_FAIL,

    DB_NO_DEV_VALUE,

    DB_DEL_ATTR_FAIL,
    DB_DEL_SCENE_FAIL,

    DB_RETVAL_MAX
} DB_RETVAL_E;

typedef enum
{
    DB_LOG_ALARM = 0,
    DB_LOG_WARNING,
    DB_LOG_MAX
} DB_LOG_TYPE_E;


typedef enum
{
    HC_USER_OPER_ADD,
    HC_USER_OPER_SET,
    HC_USER_OPER_DELETE,
    HC_USER_OPER_MAX
}
HC_USER_OPER_TYPE_E;

typedef enum
{
    HC_USER_OPER_SUCCESS,
    HC_USER_OPER_FAIL
} HC_USER_OPER_RESULT_E;

typedef struct {
    char user_name[32];
    HC_USER_OPER_TYPE_E oper_type;
    HC_USER_OPER_RESULT_E oper_result;
} HC_USER_INFO_S;

#define DB_DEV_TYPE_E int


/****************************Packed Database APIs***********************************/
//Add Device API
DB_RETVAL_E db_add_dev(HC_DEVICE_INFO_S* dev_info, int size);

//Get Device APIs
DB_RETVAL_E db_get_dev_all(HC_DEVICE_INFO_S *devs_info, int size);
DB_RETVAL_E db_get_dev_by_dev_type(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_INFO_S *devs_info, int size);
DB_RETVAL_E db_get_dev_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, HC_DEVICE_INFO_S *devs_info, int size);
DB_RETVAL_E db_get_dev_by_network_type(HC_NETWORK_TYPE_E network_type, HC_DEVICE_INFO_S *devs_info, int size);
DB_RETVAL_E db_get_dev_by_dev_id(unsigned int dev_id, HC_DEVICE_INFO_S *devs_info);
DB_RETVAL_E db_get_dev_num_all(int *dev_num);
DB_RETVAL_E db_get_dev_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *dev_num);
DB_RETVAL_E db_get_dev_num_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int *dev_num);
DB_RETVAL_E db_get_dev_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *dev_num);

//Set Device APIs
DB_RETVAL_E db_set_dev(HC_DEVICE_INFO_S *devs_info, int size);

//Delete Device APIs
DB_RETVAL_E db_del_dev_by_dev_id(unsigned int dev_id);
DB_RETVAL_E del_dev_all();

//Init Database API
DB_RETVAL_E db_init_db();

//Add Log API
DB_RETVAL_E db_add_dev_log(HC_DEVICE_INFO_S dev_info, char* log, int log_size, long log_time, int alarm_type, char* scene);

//Get Log APIs
DB_RETVAL_E db_get_dev_log_by_dev_id(char* log, int size, unsigned int dev_id);
DB_RETVAL_E db_get_dev_log_by_dev_type(char* log, int size, HC_DEVICE_TYPE_E dev_type);
DB_RETVAL_E db_get_dev_log_by_network_type(char* log, int size, HC_NETWORK_TYPE_E network_type);
DB_RETVAL_E db_get_dev_log_by_time(char* log, int size, long start_time, long end_time);
DB_RETVAL_E db_get_dev_log_all(char* log, int size);
DB_RETVAL_E db_get_dev_log_num(int* size);
DB_RETVAL_E db_get_dev_log_num_by_dev_id(unsigned int dev_id, int* size);
DB_RETVAL_E db_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int* size);
DB_RETVAL_E db_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int* size);
DB_RETVAL_E db_get_dev_log_num_by_time(int* size, long start_time, long end_time);

//Delete Log APIs
DB_RETVAL_E db_del_dev_log_all();
DB_RETVAL_E db_del_dev_log_by_dev_id(unsigned int dev_id);

//store,update dev nodes info APIs
DB_RETVAL_E db_get_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type);
DB_RETVAL_E db_set_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type);

//Device Name APIs
DB_RETVAL_E db_update_connection_by_dev_id(unsigned int dev_id, char* connection);
DB_RETVAL_E db_update_devname_by_dev_id(unsigned int dev_id, char* devname);
DB_RETVAL_E db_update_location_by_dev_id(unsigned int dev_id, char* location);

//Extended Device APIs
DB_RETVAL_E db_set_dev_ext(HC_DEVICE_EXT_INFO_S *dev_ext_info, int size);
DB_RETVAL_E db_add_dev_ext(HC_DEVICE_EXT_INFO_S *dev_ext_info, int size);
DB_RETVAL_E db_get_dev_ext_by_dev_id(unsigned int dev_id, HC_DEVICE_EXT_INFO_S *dev_ext_info);
DB_RETVAL_E db_del_dev_ext_by_dev_id(unsigned int dev_id);

//Extended Device APIs
DB_RETVAL_E db_set_armmode(int value, char* arm_id);
DB_RETVAL_E db_add_armmode(int value, char* arm_id);
DB_RETVAL_E db_get_armmode(int value, char* arm_id);
DB_RETVAL_E db_del_armmode();

//Add get alarm history API
DB_RETVAL_E db_get_dev_log_by_alarm_type(char* log, int size, int alarm_type);

#if 1
/*************************APIs For DB DEMON USE*****************/
void dbd_sync_db();
DB_RETVAL_E dbd_add_dev(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_get_dev(HC_DEVICE_INFO_S *devs_info, HC_DEVICE_INFO_S *dev_ptr);
DB_RETVAL_E dbd_set_dev(HC_DEVICE_INFO_S *devs_info);
DB_RETVAL_E dbd_get_dev_all(int dev_count, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_get_dev_num_all(int* dev_num);
DB_RETVAL_E dbd_get_dev_by_dev_type(HC_DEVICE_TYPE_E dev_type, int dev_count, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_get_dev_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int * size);
DB_RETVAL_E dbd_get_dev_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int dev_count, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_get_dev_num_by_dev_type_location(HC_DEVICE_TYPE_E dev_type, char *location, int * size);
DB_RETVAL_E dbd_get_dev_by_network_type(HC_NETWORK_TYPE_E network_type, int dev_count, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_get_dev_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size);
DB_RETVAL_E dbd_del_dev(HC_DEVICE_INFO_S *devs_info);
DB_RETVAL_E dbd_del_dev_all();

DB_RETVAL_E dbd_add_log(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_get_log_all(HC_DEVICE_INFO_S * dev_info, char* dev_ptr);
DB_RETVAL_E dbd_get_dev_log_by_dev_type(HC_DEVICE_INFO_S * dev_info, char* dev_ptr);
DB_RETVAL_E dbd_get_dev_log_by_dev_id(HC_DEVICE_INFO_S * dev_info, char* dev_ptr);
DB_RETVAL_E dbd_get_dev_log_by_network_type(HC_DEVICE_INFO_S * dev_info, char* dev_ptr);
DB_RETVAL_E dbd_get_dev_log_by_by_time(HC_DEVICE_INFO_S * dev_info, char* dev_ptr);
DB_RETVAL_E dbd_get_dev_log_by_alarm_type(HC_DEVICE_INFO_S * dev_info, char* dev_ptr);
DB_RETVAL_E dbd_get_dev_log_num(int * size);
DB_RETVAL_E dbd_get_dev_log_num_by_dev_id(unsigned int dev_id, int *size);
DB_RETVAL_E dbd_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size);
DB_RETVAL_E dbd_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size);
DB_RETVAL_E dbd_get_dev_log_num_by_time(HC_DEVICE_INFO_S *devs_info, int * size);
DB_RETVAL_E dbd_get_dev_log_num_by_alarm_type(HC_DEVICE_INFO_S *devs_info, int * size);
DB_RETVAL_E dbd_del_dev_log_all();
DB_RETVAL_E dbd_del_dev_log_by_dev_id(HC_DEVICE_INFO_S *dev_info);

DB_RETVAL_E dbd_update_connection(HC_DEVICE_INFO_S* dev_info);

//Extended Device APIs
DB_RETVAL_E dbd_set_dev_ext(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_add_dev_ext(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_get_dev_ext_by_dev_id(HC_DEVICE_INFO_S *devs_info, HC_DEVICE_INFO_S *dev_ptr);
DB_RETVAL_E dbd_del_dev_ext_by_dev_id(HC_DEVICE_INFO_S *dev_info);

//DEV EXT APIs
DB_RETVAL_E dbd_add_attr(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_get_attr(HC_DEVICE_INFO_S *devs_info, HC_DEVICE_INFO_S *dev_ptr);
DB_RETVAL_E dbd_get_attr_all_by_id(unsigned int dev_id, HC_DEVICE_EXT_ATTR_S *dev_ptr, int size);
DB_RETVAL_E dbd_get_attr_num_by_dev_id(unsigned int dev_id, int* size);
DB_RETVAL_E dbd_set_attr(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_del_attr(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_del_attr_by_dev_id(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_del_attr_all();

//SCENE APIs
DB_RETVAL_E dbd_add_scene(HC_DEVICE_INFO_S * dev_info);
DB_RETVAL_E dbd_get_scene(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S* scene_ptr);
DB_RETVAL_E dbd_get_scene_num_all(int * size);
DB_RETVAL_E dbd_set_scene(HC_DEVICE_INFO_S * dev_info);
DB_RETVAL_E dbd_del_scene(HC_DEVICE_INFO_S * dev_info);
DB_RETVAL_E dbd_del_scene_all();

//USER LOG APIs
DB_RETVAL_E dbd_add_user_log(HC_DEVICE_INFO_S *dev_info);
DB_RETVAL_E dbd_get_user_log_all(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_USER_LOG_S* dev_ptr);
DB_RETVAL_E dbd_get_user_log_num_all(int * size);

//LOCATION APIs
DB_RETVAL_E dbd_add_location(HC_DEVICE_INFO_S * dev_info);
DB_RETVAL_E dbd_get_location(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S* location_ptr);
DB_RETVAL_E dbd_get_location_num_all(int * size);
DB_RETVAL_E dbd_set_location(HC_DEVICE_INFO_S * dev_info);
DB_RETVAL_E dbd_del_location(HC_DEVICE_INFO_S * dev_info);
DB_RETVAL_E dbd_del_location_all();

//CONF APIs
DB_RETVAL_E dbd_get_conf(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E dbd_set_conf(HC_DEVICE_INFO_S * dev_info);

#ifdef ZB_SQL_SUPPORT
//ZB APIs
DB_RETVAL_E dbd_add_zb_dev(ZB_DEVICE_INFO_S* devs_info, int size);
DB_RETVAL_E dbd_del_zb_dev_by_dev_id(unsigned int dev_id);
#endif

#endif

