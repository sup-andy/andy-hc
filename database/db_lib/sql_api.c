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
*   File   : sql_api.c
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"
#include "sql_api.h"
#include <unistd.h>
#include <time.h>
#include "hc_common.h"
#include "zwave_api.h"

#define SQL_BUF_LEN 512
#define SQL_BUF_LEN_L 2048

//#define DEBUG_INFO      printf
//#define DEBUG_ERROR     printf
#define DB_VERSION "100"
#define DB_VERSION_NAME "db_ver"
#define CONF_VALUE_LEN 128

/***********************DEVICE_TABLE SQL STATEMENT******************************/
#define SQL_CMD_CREATE_DEVICE_TABLE "CREATE TABLE DEVICE_TABLE " \
                                "(ID            INT, " \
                                " DEV_ID        INTEGER PRIMARY KEY, " \
                                " DEV_NAME      CHAR(64), " \
                                " DEV_TYPE      INT, " \
                                " CONNECTION    TEXT, " \
                                " DEV_VALUE     BLOB, " \
                                " DEV_RFID      INT, " \
                                " LOCATION      CHAR(64)); "

#define SQL_CMD_ADD_DEVICE "INSERT INTO DEVICE_TABLE " \
                       "(DEV_ID, DEV_NAME, DEV_TYPE, DEV_VALUE, DEV_RFID, LOCATION) " \
                       "VALUES (%u, \'%s\', %d, ?, %u, \'%s\');"

#define SQL_CMD_DEL_DEVICE "DELETE FROM DEVICE_TABLE "

#define SQL_CMD_DEL_DEVICE_ALL "DELETE FROM DEVICE_TABLE;"

#define SQL_CMD_SELECT_DEVICE "SELECT * FROM DEVICE_TABLE "

#define SQL_CMD_SELECT_DEVICE_NUM "SELECT count(*) FROM DEVICE_TABLE "

#define SQL_CMD_SELECT_DEVICE_ALL "SELECT * FROM DEVICE_TABLE;"

#define SQL_CMD_SELECT_DEVICE_ALL_NUM "SELECT count(*) FROM DEVICE_TABLE;"

#define SQL_CMD_UPDATE_DEVICE "UPDATE DEVICE_TABLE SET DEV_TYPE=%d,DEV_VALUE=? WHERE DEV_ID=%d;"

/****************************CONNECTION SQL STATEMENT*************************/
#define SQL_CMD_GET_CONNECTION_BY_DEV_ID "SELECT CONNECTION FROM DEVICE_TABLE WHERE DEV_ID=%d;"

#define SQL_CMD_UPDATE_CONNECTION_BY_DEV_ID "UPDATE DEVICE_TABLE SET CONNECTION=? WHERE DEV_ID=%d;"

#define SQL_CMD_UPDATE_DEV_NAME_BY_DEV_ID "UPDATE DEVICE_TABLE SET DEV_NAME=? WHERE DEV_ID=%d;"

#define SQL_CMD_UPDATE_LOCATION_BY_DEV_ID "UPDATE DEVICE_TABLE SET LOCATION=? WHERE DEV_ID=%d;"

/*******************************DEVICE_LOG SQL STATEMENT***********************/
#define SQL_CMD_CREATE_DEVICE_LOG_TABLE "CREATE TABLE DEVICE_LOG " \
                            "(NUM           INTEGER PRIMARY KEY AUTOINCREMENT," \
                            " DEV_ID        INT, " \
                            " DEV_NAME      CHAR(64), " \
                            " DEV_TYPE      INT, " \
                            " NETWORK_TYPE  INT, " \
                            " LOG           TEXT," \
                            " TIME          INT, " \
                            " TIME_STR      TEXT, " \
                            " ALARM_TYPE    INT, " \
                            " SCENE         TEXT, " \
                            " EXT_1         TEXT, " \
                            " EXT_2         TEXT);"

#define SQL_CMD_ADD_DEVICE_LOG "INSERT INTO DEVICE_LOG " \
                    "(NUM, DEV_ID, DEV_TYPE, NETWORK_TYPE, LOG, TIME, ALARM_TYPE, SCENE) " \
                    "VALUES ((SELECT max(NUM) FROM DEVICE_LOG)+1, %d, %d, %d, \'%s\', %ld, %d, \'%s\');"

#define SQL_CMD_ADD_DEVICE_LOG_V2 "INSERT INTO DEVICE_LOG " \
                    "(NUM, DEV_ID, DEV_NAME, DEV_TYPE, NETWORK_TYPE, LOG, TIME, TIME_STR, ALARM_TYPE, SCENE, EXT_1, EXT_2) " \
                    "VALUES ((SELECT max(NUM) FROM DEVICE_LOG)+1, %d, \'%s\', %d, %d, \'%s\', %ld, \'%s\', %d, \'%s\', \'%s\', \'%s\');"

#define SQL_CMD_SELECT_DEVICE_LOG_ALL "SELECT * FROM DEVICE_LOG;"

#define SQL_CMD_DEL_DEVICE_LOG "DELETE FROM DEVICE_LOG "

#define SQL_CMD_DEL_DEVICE_LOG_ALL "DELETE FROM DEVICE_LOG;"

#define SQL_CMD_SELECT_DEVICE_LOG_NUM "SELECT count(*) FROM DEVICE_LOG "

#define SQL_CMD_SELECT_DEVICE_LOG "SELECT DEV_ID,DEV_TYPE,NETWORK_TYPE,LOG," \
                            "datetime(TIME, \'unixepoch\'), ALARM_TYPE, SCENE FROM DEVICE_LOG "

#define SQL_CMD_SELECT_DEVICE_LOG_V2 "SELECT DEV_ID,DEV_NAME,DEV_TYPE,NETWORK_TYPE,LOG," \
                            "TIME,TIME_STR,ALARM_TYPE,SCENE,EXT_1,EXT_2 FROM DEVICE_LOG "


#define SQL_CMD_SELECT_DEVICE_LOG_ALL_DESC "SELECT * FROM DEVICE_LOG ORDER BY ROWID DESC;"

#define SQL_CMD_ADD_DEVICE_LOG_FULL "UPDATE DEVICE_LOG SET NUM=(SELECT max(NUM) FROM DEVICE_LOG)+1, DEV_ID=%d, DEV_TYPE=%d," \
                                   "NETWORK_TYPE=%d, LOG=\'%s\', TIME=%ld, ALARM_TYPE=%d, " \
                                   "SCENE=\'%s\' WHERE NUM = (SELECT min(NUM) FROM DEVICE_LOG);"
                                   
#define SQL_CMD_ADD_DEVICE_LOG_FULL_V2 "UPDATE DEVICE_LOG SET NUM=(SELECT max(NUM) FROM DEVICE_LOG)+1, DEV_ID=%d, DEV_NAME=\'%s\', DEV_TYPE=%d," \
                                       "NETWORK_TYPE=%d, LOG=\'%s\', TIME=%ld, TIME_STR=\'%s\', ALARM_TYPE=%d, " \
                                       "SCENE=\'%s\', EXT_1=\'%s\',EXT_2=\'%s\' WHERE NUM = (SELECT min(NUM) FROM DEVICE_LOG);"

/****************************USER_LOG SQL STATEMENT****************************/
#define SQL_CMD_CREATE_USER_LOG_TABLE "CREATE TABLE USER_LOG " \
                            "(ID            INTEGER PRIMARY KEY AUTOINCREMENT," \
                            " SEVERITY      INT, " \
                            " CONTENT       TEXT);"

#define SQL_CMD_ADD_USER_LOG "INSERT INTO USER_LOG " \
                    "(SEVERITY, CONTENT) " \
                    "VALUES (%d, \'%s\');"

#define SQL_CMD_SELECT_USER_LOG_ALL "SELECT * FROM USER_LOG;"

#define SQL_CMD_SELECT_USER_LOG_V2 "SELECT SEVERITY, CONTENT FROM USER_LOG "

#define SQL_CMD_DEL_USER_LOG "DELETE FROM USER_LOG "

#define SQL_CMD_DEL_USER_LOG_ALL "DELETE FROM USER_LOG;"

#define SQL_CMD_SELECT_USER_LOG_NUM "SELECT count(*) FROM USER_LOG;"

#define SQL_CMD_ADD_USER_LOG_FULL "UPDATE USER_LOG SET ID=(SELECT max(ID) FROM USER_LOG)+1, SEVERITY=%d, CONTENT=\'%s\' " \
                                   " WHERE ID=(SELECT min(ID) FROM USER_LOG);"

/****************************ZWAVE_NODES_INFO SQL STATEMENT********************/
#define SQL_CMD_CREATE_DEV_NODES_TABLE "CREATE TABLE DEV_NODES " \
                                            "(DEV_TYPE    INT,  " \
                                            " NODES_INFO  BLOB);"

#define SQL_CMD_ADD_DEV_NODES_INFO "INSERT INTO DEV_NODES VALUES (%d,?);"

#define SQL_CMD_SELECT_DEV_NODES_INFO "SELECT * FROM DEV_NODES WHERE DEV_TYPE=%d;"

#define SQL_CMD_UPDATE_DEV_NODES_INFO "UPDATE DEV_NODES SET NODES_INFO=? WHERE DEV_TYPE=%d;"

#define SQL_CMD_SELECT_DEV_NODES_NUM "SELECT count(*) FROM DEV_NODES "

/****************************DEVICE_NAME SQL STATEMENT*************************/
#define SQL_CMD_CREATE_DEVICE_NAME_TABLE "CREATE TABLE DEVICE_NAME_TABLE " \
                                            "(DEV_ID        INTEGER PRIMARY KEY, " \
                                            " DEV_NAME      CHAR(64));"

#define SQL_CMD_ADD_DEVICE_NAME "INSERT INTO DEVICE_NAME_TABLE VALUES (%d,?);"

#define SQL_CMD_GET_DEVICE_NAME "SELECT DEV_NAME FROM DEVICE_NAME_TABLE WHERE DEV_ID=%d;"

#define SQL_CMD_DEL_DEVICE_NAME "DELETE FROM DEVICE_NAME_TABLE WHERE DEV_ID=%d;"

#define SQL_CMD_UPDATE_DEVICE_NAME "UPDATE DEVICE_NAME_TABLE SET DEV_NAME=? WHERE DEV_ID=%d;"

#define SQL_CMD_GET_DEVICE_NAME_NUM "SELECT count(*) FROM DEVICE_NAME_TABLE WHERE DEV_ID=%u;"

/***********************DEVICE_TABLE_EXT SQL STATEMENT*************************/
#define SQL_CMD_CREATE_DEVICE_TABLE_EXT "CREATE TABLE DEVICE_TABLE_EXT " \
                                "(ID            INT, " \
                                " DEV_ID        INT, " \
                                " EVENT_TYPE    INT, " \
                                " NETWORK_TYPE  INT, " \
                                " DEV_TYPE      INT, " \
                                " DEV_VALUE     BLOB); "

#define SQL_CMD_ADD_DEVICE_EXT "INSERT INTO DEVICE_TABLE_EXT " \
                       "VALUES (?, ?, ?, ?, ?, ?);"

#define SQL_CMD_DEL_DEVICE_EXT "DELETE FROM DEVICE_TABLE_EXT "

#define SQL_CMD_DEL_DEVICE_EXT_ALL "DELETE FROM DEVICE_TABLE_EXT;"

#define SQL_CMD_SELECT_DEVICE_EXT "SELECT * FROM DEVICE_TABLE_EXT "

#define SQL_CMD_SELECT_DEVICE_EXT_NUM "SELECT count(*) FROM DEVICE_TABLE_EXT "

#define SQL_CMD_SELECT_DEVICE_EXT_ALL "SELECT * FROM DEVICE_TABLE_EXT;"

#define SQL_CMD_SELECT_DEVICE_EXT_ALL_NUM "SELECT count(*) FROM DEVICE_TABLE_EXT;"

#define SQL_CMD_UPDATE_DEVICE_EXT "UPDATE DEVICE_TABLE_EXT SET DEV_VALUE=? WHERE DEV_ID=%d;"

/****************************DEVICE_EXT_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_EXT_ATTR_TABLE "CREATE TABLE DEVICE_EXT_TABLE " \
                                            "(DEV_ID        INT, " \
                                            " ATTR_NAME     CHAR(32)," \
                                            " ATTR_VALUE    CHAR(1024));"

#define SQL_CMD_ADD_EXT_ATTR "INSERT INTO DEVICE_EXT_TABLE VALUES (%u, ?, ?);"

#define SQL_CMD_GET_EXT_ATTR_VALUE "SELECT ATTR_VALUE FROM DEVICE_EXT_TABLE WHERE DEV_ID=%u AND ATTR_NAME=?;"

#define SQL_CMD_GET_EXT_ATTR_ALL_BY_DEV_ID "SELECT ATTR_NAME,ATTR_VALUE FROM DEVICE_EXT_TABLE WHERE DEV_ID=%u;"

#define SQL_CMD_DEL_EXT_ATTR_BY_ID "DELETE FROM DEVICE_EXT_TABLE WHERE DEV_ID=%u;"

#define SQL_CMD_DEL_EXT_ATTR "DELETE FROM DEVICE_EXT_TABLE WHERE DEV_ID=%u AND ATTR_NAME=?;"

#define SQL_CMD_DEL_EXT_ATTR_ALL "DELETE FROM DEVICE_EXT_TABLE;"

#define SQL_CMD_SELECT_EXT_NUM_BY_ID "SELECT count(*) FROM DEVICE_EXT_TABLE WHERE DEV_ID=%u;"

#define SQL_CMD_SELECT_EXT_NUM_BY_ID_AND_NAME "SELECT count(*) FROM DEVICE_EXT_TABLE WHERE DEV_ID=%u AND ATTR_NAME=?;"

#define SQL_CMD_UPDATE_EXT_ATTR "UPDATE DEVICE_EXT_TABLE SET ATTR_VALUE=? WHERE DEV_ID=%u AND ATTR_NAME=?;"

/****************************SCENE_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_SCENE_TABLE "CREATE TABLE SCENE_TABLE " \
                                            "(SCENE_ID        CHAR(64), " \
                                            " SCENE_ATTR      TEXT);"

#define SQL_CMD_ADD_SCENE "INSERT INTO SCENE_TABLE VALUES (?,?);"

#define SQL_CMD_GET_SCENE "SELECT SCENE_ATTR FROM SCENE_TABLE WHERE SCENE_ID=?;"

#define SQL_CMD_DEL_SCENE "DELETE FROM SCENE_TABLE WHERE SCENE_ID=?;"

#define SQL_CMD_UPDATE_SCENE "UPDATE SCENE_TABLE SET SCENE_ATTR=? WHERE SCENE_ID=?;"

#define SQL_CMD_GET_SCENE_ALL "SELECT SCENE_ID,SCENE_ATTR FROM SCENE_TABLE;"

#define SQL_CMD_GET_SCENE_NUM_ALL "SELECT count(*) FROM SCENE_TABLE;"

#define SQL_CMD_DEL_SCENE_ALL "DELETE FROM SCENE_TABLE;"

#define SQL_CMD_SELET_SCENE_NUM_BY_ID "SELECT count(*) FROM SCENE_TABLE WHERE SCENE_ID=?;"

/****************************CHECK TABLE EXISTS SQL STATEMENT******************/
#define SQL_CMD_CHECK_TABLE_EXIST "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';"

/****************************Other Definitions*********************************/
#define GLOBAL_ID_TO_NETWORK_TYPE(id) ((id >> 24) & 0xFF)

/*****************************CONFIGURE_TABLE SQL STATEMENT********************/
#define SQL_CMD_CREATE_CONF_TABLE "CREATE TABLE CONF_TABLE " \
                                     "(CONF_NAME       CHAR(64), " \
                                     " CONF_VALUE      CHAR(128));"

#define SQL_CMD_ADD_CONF "INSERT INTO CONF_TABLE VALUES (\'%s\',\'%s\');"

#define SQL_CMD_GET_CONF "SELECT CONF_VALUE FROM CONF_TABLE WHERE CONF_NAME=\'%s\';"

#define SQL_CMD_DEL_CONF "DELETE FROM CONF_TABLE WHERE CONF_NAME=\'%s\';"

#define SQL_CMD_UPDATE_CONF "UPDATE CONF_TABLE SET CONF_VALUE=\'%s\' WHERE CONF_NAME=\'%s\';"

/****************************DEVICE_VALUE_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_DEV_VALUE_TABLE "CREATE TABLE DEVICE_VALUE_TABLE " \
                                            "(DEV_ID        INT, " \
                                            " NAME     CHAR(64)," \
                                            " VALUE    CHAR(128));"

#define SQL_CMD_ADD_DEV_VALUE "INSERT INTO DEVICE_VALUE_TABLE VALUES (%u, \'%s\',\'%s\');"

#define SQL_CMD_GET_DEV_VALUE "SELECT ATTR_VALUE FROM DEVICE_VALUE_TABLE WHERE DEV_ID=%u AND NAME=\'%s\';"

#define SQL_CMD_GET_DEV_VALUE_ALL_BY_DEV_ID "SELECT NAME,VALUE FROM DEVICE_VALUE_TABLE WHERE DEV_ID=%u;"

#define SQL_CMD_DEL_DEV_VALUE_BY_ID "DELETE FROM DEVICE_VALUE_TABLE WHERE DEV_ID=%u;"

#define SQL_CMD_DEL_DEV_VALUE "DELETE FROM DEVICE_VALUE_TABLE WHERE DEV_ID=%u AND NAME=\'%s\';"

#define SQL_CMD_DEL_DEV_VALUE_ALL "DELETE FROM DEVICE_VALUE_TABLE;"

#define SQL_CMD_SELECT_DEV_VALUE_NUM_BY_ID "SELECT count(*) FROM DEVICE_VALUE_TABLE WHERE DEV_ID=%u;"

#define SQL_CMD_SELECT_DEV_VALUE_NUM_BY_ID_AND_NAME "SELECT count(*) FROM DEVICE_VALUE_TABLE WHERE DEV_ID=%u AND NAME=\'%s\';"

#define SQL_CMD_UPDATE_DEV_VALUE "UPDATE DEVICE_VALUE_TABLE SET VALUE=\'%s\' WHERE DEV_ID=%u AND NAME=\'%s\';"

#define SQL_CMD_ADD_DEV_VALUE_HEAD "INSERT INTO DEVICE_VALUE_TABLE VALUES (%u, \'%s\',\'"

#define SQL_CMD_ADD_DEV_VALUE_TAIL "\');"

#define SQL_CMD_UPDATE_DEV_VALUE_HEAD "UPDATE DEVICE_VALUE_TABLE SET VALUE=\'"

#define SQL_CMD_UPDATE_DEV_VALUE_TAIL "\' WHERE DEV_ID=%u AND NAME=\'%s\';"

/****************************LOCATION_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_LOCATION_TABLE "CREATE TABLE LOCATION_TABLE " \
                                            "(LOCATION_ID        CHAR(64), " \
                                            " LOCATION_ATTR      TEXT);"

#define SQL_CMD_ADD_LOCATION "INSERT INTO LOCATION_TABLE VALUES (?,?);"

#define SQL_CMD_GET_LOCATION "SELECT LOCATION_ATTR FROM LOCATION_TABLE WHERE LOCATION_ID=?;"

#define SQL_CMD_DEL_LOCATION "DELETE FROM LOCATION_TABLE WHERE LOCATION_ID=?;"

#define SQL_CMD_UPDATE_LOCATION "UPDATE LOCATION_TABLE SET LOCATION_ATTR=? WHERE LOCATION_ID=?;"

#define SQL_CMD_GET_LOCATION_ALL "SELECT LOCATION_ID,LOCATION_ATTR FROM LOCATION_TABLE;"

#define SQL_CMD_GET_LOCATION_NUM_ALL "SELECT count(*) FROM LOCATION_TABLE;"

#define SQL_CMD_DEL_LOCATION_ALL "DELETE FROM LOCATION_TABLE;"

#define SQL_CMD_SELET_LOCATION_NUM_BY_ID "SELECT count(*) FROM LOCATION_TABLE WHERE LOCATION_ID=?;"


#define DEV_VALUE_LEN 128
#define DEV_VALUE_NAME_LEN 256
#define VALUE_NAME_LEN DEV_VALUE_NAME_LEN


typedef struct 
{
    unsigned int dev_id;
    char name[DEV_VALUE_NAME_LEN];
    char value[DEV_VALUE_LEN];
}DEV_VALUE_S;

typedef enum 
{
    TYPE_INT = 1,
    TYPE_UINT = 2,
    TYPE_BOOL = 3,
    TYPE_STRING = 4,
    TYPE_DATETIME = 5, 
    TYPE_DOUBLE = 6, 
    TYPE_MAX,
} DBValueType;


#define DT_TYPE_INT 1
#define DT_TYPE_DOUBLE 2
#define DT_TYPE_STRING 3
#define DT_TYPE_UCHAR 4
#define DT_TYPE_LONG 5
#define DT_TYPE_UINT 6
#define DT_TYPE_MAX 255


typedef struct 
{
    char name[VALUE_NAME_LEN];
    int value_type;
}DB_DEV_VALUE_S;

typedef struct
{
    char name[VALUE_NAME_LEN];
    DBValueType type;
    void* param_offset;
} DB_MAP_NODE_S;

typedef struct
{
    HC_DEVICE_TYPE_E dev_type;
    int value_num;
    DB_DEV_VALUE_S dev_value_ptr;
}DEV_TYPE_S;

/*
typedef struct {
    int sensor_type;
    int precision;
    int scale;
    int size;
    double value;
    int battery_level; // 0x00 - 0x64. The value 0xFF indicates a battery low warning.
    int endpoints;
} HC_DEVICE_MULTILEVEL_SENSOR_S;
*/

DB_MAP_NODE_S multi_sensor_value_s[] = 
{
    {"multisensor_multi_sensor", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_MULTILEVEL_SENSOR_S, sensor_type)},
    {"multisensor_scale", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_MULTILEVEL_SENSOR_S, scale)},
    {"multisensor_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_MULTILEVEL_SENSOR_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S binary_switch_value_s[] = 
{
    {"binaryswitch_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_BINARYSWITCH_S, value)},
    {"binaryswitch_type", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_BINARYSWITCH_S, type)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S double_switch_value_s[] = 
{
    {"doubleswitch_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOUBLESWITCH_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S dimmer_value_s[] = 
{
    {"dimmer_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DIMMER_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S curtain_value_s[] = 
{
    {"curtain_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_CURTAIN_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S binary_sensor_value_s[] = 
{
    {"binarysensor_sensor_type", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_BINARY_SENSOR_S, sensor_type)},
    {"binarysensor_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_BINARY_SENSOR_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S motion_sensor_value_s[] = 
{
    {"motionsensor_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_MOTION_SENSOR_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S window_sensor_value_s[] = 
{
    {"windowsensor_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_WINDOW_SENSOR_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S temperature_sensor_value_s[] = 
{
    {"tempsensor_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_TEMPERATURE_SENSOR_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S luminance_sensor_value_s[] = 
{
    {"luminance_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_LUMINANCE_SENSOR_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S battery_value_s[] = 
{
    {"battery_battery_level", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_BATTERY_S, battery_level)},
    {"battery_interval_time", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_BATTERY_S, interval_time)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S doorlock_value_s[] = 
{
    {"dlock_doorLockMode", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_S, doorLockMode)},
    {"dlock_doorHandlesMode", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_S, doorHandlesMode)},
    {"dlock_doorCondition", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_S, doorCondition)},
    {"dlock_lockTimeoutMinutes", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_S, lockTimeoutMinutes)},
    {"dlock_lockTimeoutSeconds", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_S, lockTimeoutSeconds)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S doorlock_config_value_s[] = 
{
    {"dlockcfg_operationType", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_CONFIG_S, operationType)},
    {"dlockcfg_doorHandlesMode", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_CONFIG_S, doorHandlesMode)},
    {"dlockcfg_lockTimeoutMinutes", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_CONFIG_S, lockTimeoutMinutes)},
    {"dlockcfg_lockTimeoutSeconds", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_DOORLOCK_CONFIG_S, lockTimeoutSeconds)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S hsm100_config_value_s[] = 
{
    {"hsm100cfg_parameterNumber", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_HSM100_CONFIG_S, parameterNumber)},
    {"hsm100cfg_bDefault", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_HSM100_CONFIG_S, bDefault)},
    {"hsm100cfg_value", DT_TYPE_LONG, (char *)offsetof(HC_DEVICE_HSM100_CONFIG_S, value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S thermostat_value_s[] = 
{
    {"thermostat_mode", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_THERMOSTAT_S, mode)},
    {"thermostat_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_THERMOSTAT_S, value)},
    {"thermostat_heat_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_THERMOSTAT_S, heat_value)},
    {"thermostat_cool_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_THERMOSTAT_S, cool_value)},
    {"thermostat_energe_save_heat_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_THERMOSTAT_S, energe_save_heat_value)},
    {"thermostat_energe_save_cool_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_THERMOSTAT_S, energe_save_cool_value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S meter_value_s[] = 
{
    {"meter_meter_type", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_METER_S, meter_type)},
    {"meter_scale", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_METER_S, scale)},
    {"meter_rate_type", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_METER_S, rate_type)},
    {"meter_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_METER_S, value)},
    {"meter_delta_time", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_METER_S, delta_time)},
    {"meter_previous_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_METER_S, previous_value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S gas_meter_value_s[] = 
{
    {"gasmeter_meter_type", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_GAS_METER_S, meter_type)},
    {"gasmeter_scale", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_GAS_METER_S, scale)},
    {"gasmeter_rate_type", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_GAS_METER_S, rate_type)},
    {"gasmeter_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_GAS_METER_S, value)},
    {"gasmeter_delta_time", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_GAS_METER_S, delta_time)},
    {"gasmeter_previous_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_GAS_METER_S, previous_value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S power_meter_value_s[] = 
{
    {"powermeter_meter_type", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_POWER_METER_S, meter_type)},
    {"powermeter_scale", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_POWER_METER_S, scale)},
    {"powermeter_rate_type", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_POWER_METER_S, rate_type)},
    {"powermeter_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_POWER_METER_S, value)},
    {"powermeter_delta_time", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_POWER_METER_S, delta_time)},
    {"powermeter_previous_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_POWER_METER_S, previous_value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S water_meter_value_s[] = 
{
    {"watermeter_meter_type", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_WATER_METER_S, meter_type)},
    {"watermeter_scale", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_WATER_METER_S, scale)},
    {"watermeter_rate_type", DT_TYPE_UCHAR, (char *)offsetof(HC_DEVICE_WATER_METER_S, rate_type)},
    {"watermeter_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_WATER_METER_S, value)},
    {"watermeter_delta_time", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_WATER_METER_S, delta_time)},
    {"watermeter_previous_value", DT_TYPE_DOUBLE, (char *)offsetof(HC_DEVICE_WATER_METER_S, previous_value)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S association_value_s[] = 
{
    {"association_srcPhyId", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_ASSOCIATION_S, srcPhyId)},
    {"association_dstPhyId", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_ASSOCIATION_S, dstPhyId)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S handset_value_s[] = 
{
    {"handset_status", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_HANDSET_U, status)},
    {"handset_value", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_HANDSET_U, u)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S camera_value_s[] = 
{
    {"camera_connection", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_CAMERA_S, connection)},
    {"camera_ipaddress", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, ipaddress)},
    {"camera_onvifuid", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, onvifuid)},
    {"camera_streamurl", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, streamurl)},
    {"camera_remoteurl", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, remoteurl)},
    {"camera_destPath", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, destPath)},
    {"camera_triggerTime", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, triggerTime)},
    {"camera_label", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, label)},
    {"camera_name", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, name)},
    {"camera_mac", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, mac)},
    {"camera_ext", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, ext)},
    {"fwversion", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, fwversion)},
    {"modelname", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_CAMERA_S, modelname)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S keyfob_value_s[] = 
{
    {"item_word", DT_TYPE_STRING, (char *)offsetof(HC_DEVICE_KEYFOB_S, item_word)},
    {"", DT_TYPE_MAX, NULL}
};

DB_MAP_NODE_S siren_value_s[] = 
{
    {"warning_mode", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_SIREN_S, warning_mode)},
    {"strobe", DT_TYPE_INT, (char *)offsetof(HC_DEVICE_SIREN_S, strobe)},
    {"", DT_TYPE_MAX, NULL}
};


char* hc_get_dev_type_str(HC_DEVICE_TYPE_E index);
char* hc_get_network_type_str(HC_NETWORK_TYPE_E index);
char* hc_get_event_type_str(HC_EVENT_TYPE_E index);
char* hc_get_alarm_type_str(DB_LOG_TYPE_E index);
int sql_cb_get_count(void *buf, int argc, char **argv, char **azColName);
int sql_cb_print_result(void *data, int argc, char **argv, char **azColName);
DB_RETVAL_E sql_init_db();
sqlite3* sql_open_db();
DB_RETVAL_E sql_close_db(sqlite3* db);
DB_RETVAL_E sql_add_dev_step(HC_DEVICE_INFO_S *dev_info, int size);
DB_RETVAL_E sql_update_dev_step(HC_DEVICE_INFO_S* devs_info, int size);
DB_RETVAL_E sql_add_dev_log(HC_DEVICE_INFO_S dev_info, char* log, int size, long log_time, int alarm_type, char* scene);
DB_RETVAL_E sql_exec(int has_db_handler, sqlite3* db, char* sql, int (*callback)(void*, int, char**, char**), void* data);
DB_RETVAL_E sql_get_num(int* size, char* sql_stmt);
DB_RETVAL_E sql_get_dev_log(char*log, int size, char* sql);
DB_RETVAL_E sql_add_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type);
DB_RETVAL_E sql_init_dev_info_table(sqlite3* db);
DB_RETVAL_E db_init_user_log_table(sqlite3* db);
DB_RETVAL_E sql_init_conf_table(sqlite3* db);
void db_add_database_table_version();
void db_set_database_table_version();
int db_check_datatable_version();
void db_get_database_table_version(char* version);
void db_update_database(int ver_gap);
void save_db();
DB_RETVAL_E db_set_multi_sensor(sqlite3* db, unsigned int dev_id, HC_DEVICE_MULTILEVEL_SENSOR_S *multilevel_sensor);
DB_RETVAL_E sql_update_dev_value_cmd(sqlite3 db, unsigned int dev_id, HC_DEVICE_MULTILEVEL_SENSOR_S *multilevel_sensor, char* sql_cmd);
DB_RETVAL_E sql_get_dev_value(sqlite3 *db, unsigned int dev_id, HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U * dev_ptr);
DB_RETVAL_E sql_init_dev_value_table(sqlite3 *db);
DB_RETVAL_E db_set_device_value(sqlite3* db, unsigned int dev_id, HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U * dev_ptr, char** sql_cmd);
DB_RETVAL_E sql_init_location_table(sqlite3* db);
DB_RETVAL_E sql_init_zb_device_table(sqlite3 *db);

int db_init = 0;

/***************************The Functions**************************************/
sqlite3* sql_open_db()
{
    sqlite3* db;
    int rc;
    rc = sqlite3_open(DB_STORAGE_PATH, &db);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Open database fail:%s! File name is %s\n", sqlite3_errmsg(db), DB_STORAGE_PATH);
        return NULL;
    }

    return db;
}


DB_RETVAL_E sql_close_db(sqlite3* db)
{
    sqlite3_close(db);
    return DB_RETVAL_OK;
}


DB_RETVAL_E sql_add_dev(HC_DEVICE_INFO_S *dev_info, int size)
{
    int i;
    int rc;
    sqlite3* db;
    sqlite3_stmt * stat = NULL;
    void* buffer;
    int buffer_size;
    char sql_stmt[SQL_BUF_LEN];
    DB_RETVAL_E ret = DB_RETVAL_OK;
    unsigned int dev_rfid;
    int nRetryCnt = 0;

    if ((size <= 0) || (dev_info == NULL))
    {
        return DB_PARAM_ERROR;
    }

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    //for (i = 0; i < size; i++)
    {
        buffer_size = sizeof(HC_DEVICE_U);
        buffer = (void*) & (dev_info[i].device.stuff[0]);

        dev_rfid = GLOBAL_ID_TO_DEV_ID(dev_info[i].dev_id);
        //(DEV_ID, PHY_ID, DEV_NAME, EVENT_TYPE, NETWORK_TYPE, DEV_TYPE, DEV_VALUE, DEV_RFID, LOCATION) 
        snprintf(sql_stmt, SQL_BUF_LEN, SQL_CMD_ADD_DEVICE, dev_info[i].dev_id, 
                 dev_info[i].dev_name, dev_info[i].dev_type, dev_rfid, dev_info[i].location);
        //DEBUG_INFO("[DB]Add SQL Statement is [%s]", sql_stmt);

        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_blob(stat, 1, buffer, buffer_size, NULL);
        rc = sqlite3_step(stat);

        while ((nRetryCnt < 10 ) && 
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0;

        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add devices fail(%d):%s!\n", rc, sqlite3_errmsg(db));
            ret = DB_ADD_DEV_FAIL;
        }

        sqlite3_finalize(stat);
    }

    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_get_num(int* size, char* sql_stmt)
{
    int rc;
    int count = 0;

    rc = sql_exec(0, NULL, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
        return DB_GET_NUM_FAIL;
    }

    *size = count;
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_exec(int has_db_handler, sqlite3* db, char* sql, int (*callback)(void*, int, char**, char**), void* data)
{
    int rc;
    char* zErrMsg;
    int nRetryCnt = 0;
    sqlite3* local_db = NULL;

   if (has_db_handler == 0)
   {
       local_db = sql_open_db();
       if (local_db == NULL)
       {
           return DB_OPEN_ERROR;
       }
   }
   else
   {
       local_db = db;
   }

    if (callback == NULL)
        rc = sqlite3_exec(local_db, sql, sql_cb_print_result, data, &zErrMsg);
    else
        rc = sqlite3_exec(local_db, sql, callback, data, &zErrMsg);

    while ((nRetryCnt < 10 ) &&
           (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql, rc);
        nRetryCnt++;
        usleep(100000);
        sqlite3_free(zErrMsg);
        if (callback == NULL)
            rc = sqlite3_exec(local_db, sql, sql_cb_print_result, data, &zErrMsg);
        else
            rc = sqlite3_exec(local_db, sql, callback, data, &zErrMsg);
    }
    nRetryCnt = 0;

    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]SQL statement's excution error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        if (has_db_handler == 0)
        {
            sql_close_db(local_db);
        }
        return DB_EXEC_ERROR;
    }

    if (has_db_handler == 0)
    {
        sql_close_db(local_db);
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_ext_dev(sqlite3 *db, HC_DEVICE_INFO_S *hcdev)
{
    int rc = DB_RETVAL_OK;
    ZW_DEVICE_INFO zwdev;
    
    if (HC_NETWORK_TYPE_ZWAVE == hcdev->network_type)
    {
        memset(&zwdev, 0, sizeof(zwdev));
        rc = db_get_zw_dev_by_dev_id_with_db(db, hcdev->dev_id & 0x00FFFFFF,  &zwdev);
        if (rc != DB_RETVAL_OK)
        {
            DEBUG_ERROR("[DB]SQL Get ZW DEV by id[%x] failed, rc=%d.\n", 
                    hcdev->dev_id & 0x00FFFFFF, rc);
        }
        else
        {
            hcdev->phy_id = zwdev.phy_id;
        }
    }

    return rc;

}

DB_RETVAL_E sql_get_dev(HC_DEVICE_INFO_S *devs_info, int size, const char* sql_stmt)
{
    int rc = DB_RETVAL_OK;
    const void* buf;
    int buffer_size;
    int i = 0;
    sqlite3* db;
    sqlite3_stmt* stat;
    const unsigned char* dev_name_ptr;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    DEBUG_INFO("sql_stmt is [%s]", sql_stmt);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);

    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        // get DEVICE_TABLE info.
        //(DEV_ID, DEV_NAME, DEV_TYPE, CONNECTION, DEV_VALUE, DEV_RFID, LOCATION)
        devs_info[i].dev_id = sqlite3_column_int(stat, 1);
        devs_info[i].phy_id = devs_info[i].dev_id & 0xff;
        
        dev_name_ptr = sqlite3_column_text(stat, 2);
        memset(devs_info[i].dev_name, 0, MAX_DEVICE_NAME_SIZE);
        strncpy(devs_info[i].dev_name, (char*)dev_name_ptr, MAX_DEVICE_NAME_SIZE-1);
        devs_info[i].dev_name[MAX_DEVICE_NAME_SIZE-1] = '\0';
        
        devs_info[i].network_type = devs_info[i].dev_id >> 24;
        devs_info[i].dev_type = sqlite3_column_int(stat, 3);
        buffer_size = sizeof(HC_DEVICE_U);

        dev_name_ptr = sqlite3_column_text(stat, 4);
        memset(devs_info[i].conn_status, 0, MAX_DEVICE_CONNSTATUS_SIZE);
        strncpy(devs_info[i].conn_status, (char*)dev_name_ptr, MAX_DEVICE_CONNSTATUS_SIZE-1);
        devs_info[i].conn_status[MAX_DEVICE_CONNSTATUS_SIZE-1] = '\0';


#ifdef BLOB_DATA
        buf = sqlite3_column_blob(stat, 5);
        memcpy(&(devs_info[i].device.stuff[0]), (unsigned char*)buf, buffer_size);
#endif

        dev_name_ptr = sqlite3_column_text(stat, 7);
        memset(devs_info[i].location, 0, MAX_DEVICE_LOCATION_SIZE);
        strncpy(devs_info[i].location, (char*)dev_name_ptr, MAX_DEVICE_LOCATION_SIZE-1);
        devs_info[i].location[MAX_DEVICE_LOCATION_SIZE-1] = '\0';

#if 0
        // get XX_DEVICE_TABLE info.
        if (devs_info[i].dev_id != 0) 
        {
            sql_get_ext_dev(db, &(devs_info[i]));
        }
#endif

        DEBUG_INFO("[DB]SQL Get [0x%x][0x%x][%s][%d][%d][%d]",
                   devs_info[i].dev_id,
                   devs_info[i].phy_id,
                   devs_info[i].dev_name,
                   devs_info[i].event_type,
                   devs_info[i].network_type,
                   devs_info[i].dev_type);
        
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; // DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
        
    }

    sqlite3_finalize(stat);

#ifndef BLOB_DATA
    for(i = 0; i< size; i++)
    {
        if(devs_info[i].dev_id != 0)
        {            
            rc = sql_get_dev_value(db, devs_info[i].dev_id, devs_info[i].dev_type, &(devs_info[i].device));
        }
    }
#endif
    sql_close_db(db);
    
    if(rc == DB_NO_DEV_VALUE)
    {
        DEBUG_INFO("[DB]device get no value");
        rc = DB_RETVAL_OK;
    }
    
    return rc;
}

DB_RETVAL_E sql_check_dev_exist(unsigned int dev_id)
{
    char sql_stmt[SQL_BUF_LEN];
    int dev_num;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID=%u", dev_id);
    sql_get_num(&dev_num, sql_stmt);

    if (dev_num > 0)
        return DB_HAS_RECORD; //TRUE;
    else
        return DB_NO_RECORD; //FALSE;
}

DB_RETVAL_E sql_set_dev(HC_DEVICE_INFO_S* devs_info, int size)
{
    int i;
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int dev_num = 0;
    void *buffer;
    int buffer_size;
    sqlite3_stmt* stat = NULL;
    sqlite3* db;
    char *sql_cmd;
    unsigned int dev_rfid;
    int nRetryCnt = 0;

    if ((size <= 0) || (devs_info == NULL))
    {
        return DB_PARAM_ERROR;
    }

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    for (i = 0; i < size; i++)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID=%u", devs_info[i].dev_id);
        rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&dev_num);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
            continue;
        }

        buffer_size = sizeof(HC_DEVICE_U);
        buffer = (void*) & (devs_info[i].device.stuff[0]);

        if (dev_num == 0)
        {
            dev_rfid = GLOBAL_ID_TO_DEV_ID(devs_info[i].dev_id);
            //DEBUG_INFO("No exist record, add new device");
            snprintf(sql_stmt, SQL_BUF_LEN, SQL_CMD_ADD_DEVICE, devs_info[i].dev_id, 
                     devs_info[i].dev_name, devs_info[i].dev_type, dev_rfid, devs_info[i].location);
            DEBUG_INFO("[DB]Add SQL Statement is [%s]", sql_stmt);
            
            rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
            rc = sqlite3_bind_blob(stat, 1, buffer, buffer_size, NULL);
            rc = sqlite3_step(stat);
            while ((nRetryCnt < 10 ) && 
        		   (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
                DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
                nRetryCnt++;
                usleep(100000);
                rc = sqlite3_step(stat);
            }
            nRetryCnt = 0;

            if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                DEBUG_ERROR("[DB]set device add error(%d)\n", rc);
                sqlite3_finalize(stat);
                goto EXIT;
            }
            sqlite3_finalize(stat);
#ifndef BLOB_DATA

            rc = db_set_device_value(db, devs_info[i].dev_id, devs_info[i].dev_type, &(devs_info[i].device), &sql_cmd);
            //printf("db_set_device_value = (%d) \n", rc);
            if(rc != DB_RETVAL_OK)
            {
                DEBUG_ERROR("[DB]Combine sql cmd failed.\n");
                goto EXIT;
            }
            //printf("\n**************sql_cmd is [%s]*********************\n", sql_cmd);
            
            rc = sql_exec(1, db, sql_cmd, NULL, 0);
            free(sql_cmd);
            if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
                DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
                goto EXIT;
            }
#endif
        }
        else
        {
            sprintf(sql_stmt, SQL_CMD_UPDATE_DEVICE, devs_info[i].dev_type, devs_info[i].dev_id);
            DEBUG_INFO("[DB]Update SQL Statement is [%s] \n", sql_stmt);
            
            sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
            rc = sqlite3_bind_blob(stat, 1, buffer, buffer_size, NULL);
            rc = sqlite3_step(stat);
            while ((nRetryCnt < 10 ) &&
                   (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
                DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
                nRetryCnt++;
                usleep(100000);
                rc = sqlite3_step(stat);
            }
            nRetryCnt = 0;

            if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                DEBUG_ERROR("[DB]set device update error(%d)\n", rc);
                sqlite3_finalize(stat);
                goto EXIT;
            }
            sqlite3_finalize(stat);
            
#ifndef BLOB_DATA
            rc = db_set_device_value(db, devs_info[i].dev_id, devs_info[i].dev_type, &(devs_info[i].device), &sql_cmd);
            if (rc != DB_RETVAL_OK)
            {
                DEBUG_ERROR("[DB]Combine sql cmd failed.\n");
                goto EXIT;
            }
            //DEBUG_INFO("[DB]Update value SQL Statement is [%s] \n", sql_cmd);

            rc = sql_exec(1, db, sql_cmd, NULL, 0);
            free(sql_cmd);
            if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
                DEBUG_ERROR("[DB]Set Device VALUE TABLE ERROR !!\n");
                goto EXIT;
            }
#endif
        }
    }

EXIT:
    sql_close_db(db);

    return rc;
}

//static int is the indicated return value type of sqlite3 callback function.
int sql_cb_get_count(void *buf, int argc, char **argv, char **azColName)
{
    int *i = buf;
    if (argv)
    {
        //printf("name, argv is %s:%s\n", azColName[0], argv[0]);
        *i = atoi(argv[0]);
    }
    return 0;
}

int sql_cb_print_result(void *data, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        DEBUG_INFO("[DB]%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

DB_RETVAL_E db_add_dev(HC_DEVICE_INFO_S *dev_info, int size)
{
    return sql_add_dev(dev_info, size);
}

DB_RETVAL_E db_set_dev(HC_DEVICE_INFO_S *devs_info, int size)
{
    return sql_set_dev(devs_info, size);
}

DB_RETVAL_E db_get_dev_all(HC_DEVICE_INFO_S *devs_info, int size)
{
    return sql_get_dev(devs_info, size, SQL_CMD_SELECT_DEVICE_ALL);
}

DB_RETVAL_E db_get_dev_by_dev_type(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_INFO_S *devs_info, int size)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_TYPE = %d;", dev_type);
    return sql_get_dev(devs_info, size, sql_stmt);
}

DB_RETVAL_E db_get_dev_by_network_type(HC_NETWORK_TYPE_E network_type, HC_DEVICE_INFO_S *devs_info, int size)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    /* NETWORK_TYPE column in DEVICE_TABLE was removed in v1.00.07. The MSB byte in DEV_ID(4 byte) already contend 
       the NETWORK_TYPE info. */
    switch(network_type)
    {
        case HC_NETWORK_TYPE_APP:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID < 0x1000000;");
            break;
        case HC_NETWORK_TYPE_ZWAVE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x1000000 AND DEV_ID < 0x2000000;");
            break;
        case HC_NETWORK_TYPE_ZIGBEE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x2000000 AND DEV_ID < 0x3000000;");
            break;
        case HC_NETWORK_TYPE_ULE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x3000000 AND DEV_ID < 0x4000000;");
            break;
        case HC_NETWORK_TYPE_LPRF:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x4000000 AND DEV_ID < 0x5000000;");
            break;
        case HC_NETWORK_TYPE_WIFI:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x5000000 AND DEV_ID < 0x6000000;");
            break;
        case HC_NETWORK_TYPE_WIREDCABLE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x6000000 AND DEV_ID < 0x7000000;");
            break;
        case HC_NETWORK_TYPE_CAMERA:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID >= 0x7000000 AND DEV_ID < 0x8000000;");
            break;
        default:
            DEBUG_ERROR("no support network type : %d", network_type);
            return DB_PARAM_ERROR;
    }

    return sql_get_dev(devs_info, size, sql_stmt);
}

DB_RETVAL_E db_get_dev_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, HC_DEVICE_INFO_S *devs_info, int size)
{
    char sql_stmt[SQL_BUF_LEN];

    if(location == NULL) {
        DEBUG_ERROR("Location not available\n");
        return -1;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_TYPE = %d AND LOCATION = '%s';", dev_type, location);
    return sql_get_dev(devs_info, size, sql_stmt);
}

DB_RETVAL_E db_get_dev_by_dev_id(unsigned int dev_id, HC_DEVICE_INFO_S *devs_info)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE"WHERE DEV_ID=%u;", dev_id);
    return sql_get_dev(devs_info, 1, sql_stmt);
}

DB_RETVAL_E db_get_dev_num_all(int *dev_num)
{
    return sql_get_num(dev_num, SQL_CMD_SELECT_DEVICE_NUM";");
}

DB_RETVAL_E db_get_dev_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *dev_num)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_TYPE=%d;", dev_type);

    return sql_get_num(dev_num, sql_stmt);
}

DB_RETVAL_E db_get_dev_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *dev_num)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    /* NETWORK_TYPE column in DEVICE_TABLE was removed in v1.00.07. The MSB byte in DEV_ID(4 byte) already contend 
       the NETWORK_TYPE info. */
    switch(network_type)
    {
        case HC_NETWORK_TYPE_APP:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID < 0x1000000;");
            break;
        case HC_NETWORK_TYPE_ZWAVE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x1000000 AND DEV_ID < 0x2000000;");
            break;
        case HC_NETWORK_TYPE_ZIGBEE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x2000000 AND DEV_ID < 0x3000000;");
            break;
        case HC_NETWORK_TYPE_ULE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x3000000 AND DEV_ID < 0x4000000;");
            break;
        case HC_NETWORK_TYPE_LPRF:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x4000000 AND DEV_ID < 0x5000000;");
            break;
        case HC_NETWORK_TYPE_WIFI:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x5000000 AND DEV_ID < 0x6000000;");
            break;
        case HC_NETWORK_TYPE_WIREDCABLE:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x6000000 AND DEV_ID < 0x7000000;");
            break;
        case HC_NETWORK_TYPE_CAMERA:
            sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID >= 0x7000000 AND DEV_ID < 0x8000000;");
            break;
        default:
            DEBUG_ERROR("no support network type : %d", network_type);
            return DB_PARAM_ERROR;
    }

    return sql_get_num(dev_num, sql_stmt);
}

DB_RETVAL_E db_get_dev_num_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int *dev_num)
{
    char sql_stmt[SQL_BUF_LEN];

    if(location == NULL) {
        DEBUG_ERROR("Location not available\n");
        return -1;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_TYPE=%d AND LOCATION='%s';", dev_type, location);

    return sql_get_num(dev_num, sql_stmt);
}

DB_RETVAL_E db_del_dev_by_dev_id(unsigned int dev_id)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_DEL_DEVICE"WHERE %s = %u;", "DEV_ID", dev_id);

    return sql_exec(0, NULL, sql, NULL, NULL);
}

DB_RETVAL_E db_del_dev_by_network_type(HC_NETWORK_TYPE_E network_type)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_DEL_DEVICE"WHERE %s = %d;", "NETWORK_TYPE", network_type);

    return sql_exec(0, NULL, sql, NULL, NULL);
}

DB_RETVAL_E db_del_dev_all()
{
    int ret = 0;

    ret = sql_exec(0, NULL, SQL_CMD_DEL_DEV_VALUE_ALL, NULL, NULL);
    if (ret != DB_RETVAL_OK)
    {
        DEBUG_ERROR("[DB]Delete DEVICE_VALUE_TABLE ERROR !!\n");
        return ret;
    }

    return sql_exec(0, NULL, SQL_CMD_DEL_DEVICE_ALL, NULL, NULL);    
}

DB_RETVAL_E db_check_dev_exist(unsigned int dev_id)
{
    return sql_check_dev_exist(dev_id);
}

DB_RETVAL_E sql_init_device_table(sqlite3 *db)
{    
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "DEVICE_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check DEVICE_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]DEVICE_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_DEVICE_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create DEVICE_TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_init_device_log_table(sqlite3 *db)
{    
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "DEVICE_LOG");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check DEVICE_LOG Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]DEVICE_LOG TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_DEVICE_LOG_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create DEVICE_LOG TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_init_device_name_table(sqlite3 *db)
{    
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "DEVICE_NAME_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check DEVICE_NAME_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]DEVICE_NAME_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_DEVICE_NAME_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create DEVICE_NAME_TABLE TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_init_ext_attr_table(sqlite3 *db)
{    
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "DEVICE_EXT_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check DEVICE_EXT_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]DEVICE_EXT_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_EXT_ATTR_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create DEVICE_EXT_TABLE TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_init_scene_table(sqlite3 *db)
{    
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "SCENE_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check SCENE_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]SCENE_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_SCENE_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create SCENE_TABLE TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}


DB_RETVAL_E sql_init_db()
{
    sqlite3 *db;
    int cr_general_table_flag = 0, cr_specific_table_flag = 0;
    int ver_gap;

    if (access(DB_STORAGE_PATH, 0) == 0)
    {
        DEBUG_INFO("[DB]the database file is already exist.\n");
        db_init = 1;
    }

    //create database
    db = sql_open_db();
    if (db == NULL)
    {
        DEBUG_ERROR("[DB]Open Database fail!\n");
        return DB_OPEN_ERROR;
    }

    if (sql_init_conf_table(db) == DB_TABLE_EXIST)
    {
        ver_gap = db_check_datatable_version();
        if(ver_gap > 0)
        {
            db_update_database(ver_gap);
        }
    }
    else
    {
        db_add_database_table_version();
    }

    // General device tables
    if (sql_init_device_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create DEVICE_TABLE !!\n");
        cr_general_table_flag++;
    }
    if (sql_init_device_log_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create DEVICE_LOG !!\n");
        cr_general_table_flag++;
    }
    //if (sql_init_device_name_table(db) == DB_RETVAL_OK)
    //{
    //    DEBUG_INFO("[DB]Create DEVICE_NAME_TABLE !!\n");
    //    cr_general_table_flag++;
    //}
    if (sql_init_ext_attr_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create DEVICE_EXT_TABLE !!\n");
        cr_general_table_flag++;
    }
    if (sql_init_scene_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create SCENE_TABLE !!\n");
        cr_general_table_flag++;
    }
    if (sql_init_dev_value_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create DEVICE_VALUE_TABLE !!\n");
        cr_general_table_flag++;
    }
    //if (sql_init_dev_info_table(db) == DB_RETVAL_OK)
    //{
    //    DEBUG_INFO("[DB]Create DEV_NODES !!\n");
    //    cr_general_table_flag++;
    //}
    if (db_init_user_log_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create USER_LOG !!\n");
        cr_general_table_flag++;
    }
    if (sql_init_location_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create LOCATION_TABLE !!\n");
        cr_general_table_flag++;
    }

    // Specific device tables -- Ex: Zigbee, Z-Wave, LPRF database tables
    if (sql_init_zb_device_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create ZB_DEVICE_TABLE !!\n");
        cr_specific_table_flag++;
    }
    if (sql_init_zw_device_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create ZW_DEVICE_TABLE !!\n");
        cr_specific_table_flag++;
    }
    if (sql_init_lprf_device_table(db) == DB_RETVAL_OK)
    {
        DEBUG_INFO("[DB]Create LPRF_DEVICE_TABLE !!\n");
        cr_specific_table_flag++;
    }

    if (cr_general_table_flag != 0 || cr_specific_table_flag != 0)
    {
        DEBUG_INFO("[DB]Some Tables have been created. Do Save!! generic_flag [%d], specific_flag [%d] \n", cr_general_table_flag, cr_specific_table_flag);
        save_db();
    }

    sql_close_db(db);
    db_init = 1;
    return DB_RETVAL_OK;
}

void save_db()
{
    dbd_sync_db();
}

#if 0
void restore_db()
{
    char syscmd[256] = {0};

    if(0 != access(DB_STORAGE_PATH, R_OK))
    {
        return;
    }
    snprintf(syscmd, sizeof(syscmd) - 1, "cp %s %s", DB_STORAGE_PATH, DB_PATH);
    system(syscmd);
}
#endif

DB_RETVAL_E db_init_db()
{
    //if(0 != access(DB_PATH, R_OK))
    //{
    //    restore_db();
    //}
    return sql_init_db();
}

DB_RETVAL_E sql_add_dev_log(HC_DEVICE_INFO_S dev_info, char* log, int size, long log_time, int alarm_type, char* scene)
{
    char sql_stmt[SQL_BUF_LEN];
    char sql_log[SQL_LOG_LEN];
    char sql_scene[SQL_SCENE_LEN];
    int rc;
    int i = 0;
    sqlite3* db;
    char* log_ptr;
    int count = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_LOG_NUM, ";");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
    }

    log_ptr = log;
    for (i = 0; i < size; i++)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        memset(sql_log, 0, SQL_LOG_LEN);
        memset(sql_scene, 0, SQL_SCENE_LEN);
        strncpy(sql_log, log_ptr, SQL_LOG_LEN - 1);
        strncpy(sql_scene, scene, SQL_SCENE_LEN);
        sql_log[SQL_LOG_LEN - 1] = '\0';

        if ((count + i + 1) <= MAX_LOG_NUM)
        {
            sprintf(sql_stmt, SQL_CMD_ADD_DEVICE_LOG, dev_info.dev_id,
                    dev_info.dev_type, dev_info.network_type, log, log_time
                    , alarm_type, scene);
            DEBUG_INFO("[DB]sql_add_dev_log sql_stmt is [%s]\n", sql_stmt);

            rc = sql_exec(1, db, sql_stmt, NULL, 0);
            if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                DEBUG_ERROR("[DB]Add log fail !!\n");
                sql_close_db(db);
                return DB_ADD_LOG_FAIL;
            }
        }
        else
        {
            sprintf(sql_stmt, SQL_CMD_ADD_DEVICE_LOG_FULL, dev_info.dev_id,
                    dev_info.dev_type, dev_info.network_type, log, log_time
                    , alarm_type, scene);
            rc = sql_exec(1, db, sql_stmt, NULL, 0);
            if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                DEBUG_ERROR("[DB]Update log fail !!\n");
                sql_close_db(db);
                return DB_ADD_LOG_FAIL;
            }       
        }

        log_ptr += SQL_LOG_LEN;
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_dev_log_v2(HC_DEVICE_EXT_LOG_S* log_ptr)
{
    char sql_stmt[SQL_BUF_LEN_L];
    int rc;
    sqlite3* db;
    int count = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_LOG_NUM, ";");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
    }

    if ((count + 1) <= MAX_LOG_NUM)
    {
        sprintf(sql_stmt, SQL_CMD_ADD_DEVICE_LOG_V2, log_ptr->dev_id, log_ptr->dev_name,
                log_ptr->dev_type, log_ptr->network_type, log_ptr->log, log_ptr->log_time, log_ptr->log_time_str
                , log_ptr->alarm_type, log_ptr->log_name, log_ptr->ext_1, log_ptr->ext_2);

        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add log fail !!\n");
            sql_close_db(db);
            return DB_ADD_LOG_FAIL;
        }
    }
    else
    {
        sprintf(sql_stmt, SQL_CMD_ADD_DEVICE_LOG_FULL_V2, log_ptr->dev_id, log_ptr->dev_name,
                log_ptr->dev_type, log_ptr->network_type, log_ptr->log, log_ptr->log_time, log_ptr->log_time_str
                , log_ptr->alarm_type, log_ptr->log_name, log_ptr->ext_1, log_ptr->ext_2);

        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add log fail !!\n");
            sql_close_db(db);
            return DB_ADD_LOG_FAIL;
        }
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}


DB_RETVAL_E sql_get_dev_log(char*log, int size, char* sql_stmt)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    int dev_id;
    int dev_type;
    int network_type;
    const unsigned char* time_str;
    const unsigned char* log_str;
    int i = 0;
    char* log_ptr;
    sqlite3* db;
    int alarm_type = 0;
    const unsigned char* scene_ptr;

    DEBUG_INFO("[DB]param size is [%d]", size);
    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    log_ptr = log;

    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    while ((sqlite3_step(stat) == SQLITE_ROW) && (i < size))
    {
        dev_id = sqlite3_column_int(stat, 0);
        dev_type = sqlite3_column_int(stat, 1);
        network_type = sqlite3_column_int(stat, 2);
        log_str = sqlite3_column_text(stat, 3);
        time_str = sqlite3_column_text(stat, 4);
        alarm_type = sqlite3_column_int(stat, 5);
        scene_ptr = sqlite3_column_text(stat, 6);
        snprintf(log_ptr, LOG_BUF_LEN, "[%d][%s][%s][%s][%s][%s][%s]\n", dev_id, time_str
                 , hc_get_dev_type_str(dev_type)
                 , hc_get_network_type_str(network_type)
                 , hc_get_alarm_type_str(alarm_type)
                 , scene_ptr
                 , log_str);
        DEBUG_INFO("[DB]get log is %s\n", log_ptr);
        log_ptr += LOG_BUF_LEN;
        i++;
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_dev_log_v2(HC_DEVICE_EXT_LOG_S* log, int size, char* sql_stmt)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    const unsigned char* time_ptr;
    const unsigned char* log_ptr;
    int i = 0;
    sqlite3* db;
    const unsigned char* scene_ptr;
    const unsigned char* dev_name_ptr;
    const unsigned char* ext_1_ptr;
    const unsigned char* ext_2_ptr;

    //DEBUG_INFO("param size is [%d]", size);
    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_LOG_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {   
        //"SELECT DEV_ID,DEV_NAME,DEV_TYPE,NETWORK_TYPE,LOG," 
        //"TIME,TIME_STR,ALARM_TYPE,SCENE,EXT_1,EXT_2 FROM DEVICE_LOG "
        log[i].dev_id = sqlite3_column_int(stat, 0);
        dev_name_ptr = sqlite3_column_text(stat, 1);
        log[i].dev_type = sqlite3_column_int(stat, 2);
        log[i].network_type = sqlite3_column_int(stat, 3);
        log_ptr = sqlite3_column_text(stat, 4);
        log[i].log_time= sqlite3_column_int(stat, 5);
        time_ptr = sqlite3_column_text(stat, 6);
        log[i].alarm_type= sqlite3_column_int(stat, 7);
        scene_ptr = sqlite3_column_text(stat, 8);
        ext_1_ptr = sqlite3_column_text(stat, 9);
        ext_2_ptr = sqlite3_column_text(stat, 10);

        memset(log[i].dev_name, 0, DEVICE_NAME_LEN);
        strncpy(log[i].dev_name, (char*)dev_name_ptr, DEVICE_NAME_LEN-1);
        log[i].dev_name[DEVICE_NAME_LEN-1]='\0';
        
        memset(log[i].log, 0, SQL_LOG_LEN);
        strncpy(log[i].log, (char*)log_ptr, SQL_LOG_LEN-1);
        log[i].log[SQL_LOG_LEN-1]='\0';

        memset(log[i].log_time_str, 0, TIME_STR_LEN);
        strncpy(log[i].log_time_str, (char*)time_ptr, TIME_STR_LEN-1);
        log[i].log_time_str[TIME_STR_LEN-1] = '\0';

        memset(log[i].log_name, 0, SQL_SCENE_LEN);
        strncpy(log[i].log_name, (char*)scene_ptr, SQL_SCENE_LEN-1);
        log[i].log_name[SQL_SCENE_LEN-1] = '\0';

        memset(log[i].ext_1, 0, LOG_EXT_LEN);
        strncpy(log[i].ext_1, (char*)ext_1_ptr, LOG_EXT_LEN-1);
        log[i].ext_1[LOG_EXT_LEN-1] = '\0';

        memset(log[i].ext_2, 0, LOG_EXT_LEN);
        strncpy(log[i].ext_2, (char*)ext_2_ptr, LOG_EXT_LEN-1);
        log[i].ext_2[LOG_EXT_LEN-1] = '\0';

        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                DEBUG_INFO("[DB]No More Log Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Log Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}


DB_RETVAL_E db_add_dev_log(HC_DEVICE_INFO_S dev_info, char* log, int log_size, long log_time, int alarm_type, char* scene)
{
    return sql_add_dev_log(dev_info, log, log_size, log_time, alarm_type, scene);
}

DB_RETVAL_E db_get_dev_log_all(char* log, int size)
{
    return sql_get_dev_log(log, size, SQL_CMD_SELECT_DEVICE_LOG"ORDER BY NUM DESC;");
}

DB_RETVAL_E db_get_dev_log_all_desc(char* log, int size)
{
    return sql_get_dev_log(log, size, SQL_CMD_SELECT_DEVICE_LOG"ORDER BY NUM DESC;");
}

DB_RETVAL_E db_get_dev_log_by_dev_id(char* log, int size, unsigned int dev_id)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE %s = %u ORDER BY NUM DESC;", "DEV_ID", dev_id);

    return sql_get_dev_log(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_dev_type(char* log, int size, HC_DEVICE_TYPE_E dev_type)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE %s = %d ORDER BY NUM DESC;", "DEV_TYPE", dev_type);

    return sql_get_dev_log(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_network_type(char* log, int size, HC_NETWORK_TYPE_E network_type)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE %s = %d ORDER BY NUM DESC;", "NETWORK_TYPE", network_type);

    return sql_get_dev_log(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_time(char* log, int size, long start_time, long end_time)
{
    char sql[SQL_BUF_LEN];

    if (start_time == 0)
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE TIME<=%ld ORDER BY NUM DESC;", end_time);
    else if (end_time == 0)
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE TIME>%ld ORDER BY NUM DESC;", start_time);
    else
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE TIME BETWEEN %ld AND %ld ORDER BY NUM DESC;", start_time, end_time);

    return sql_get_dev_log(log, size, sql);
}


DB_RETVAL_E db_get_dev_log_by_index(char* log, int size, int index)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"ORDER BY NUM DESC LIMIT %d,%d;", index, size);

    return sql_get_dev_log(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_dev_id_by_index(char* log, int size, int index, unsigned int dev_id)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG"WHERE DEV_ID = %u ORDER BY NUM DESC LIMIT %d,%d;", dev_id, index, size);

    return sql_get_dev_log(log, size, sql);
}


DB_RETVAL_E db_add_dev_log_v2(HC_DEVICE_EXT_LOG_S* log_ptr)
{
    return sql_add_dev_log_v2(log_ptr);
}

DB_RETVAL_E db_get_dev_log_all_v2(HC_DEVICE_EXT_LOG_S* log, int size)
{
    int ret;
    ret = sql_get_dev_log_v2(log, size, SQL_CMD_SELECT_DEVICE_LOG_V2"ORDER BY NUM;");
    return ret;
}


DB_RETVAL_E db_get_dev_log_by_dev_id_v2(HC_DEVICE_EXT_LOG_S* log, int size, unsigned int dev_id)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE %s = %u ORDER BY NUM;", "DEV_ID", dev_id);

    return sql_get_dev_log_v2(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_dev_type_v2(HC_DEVICE_EXT_LOG_S* log, int size, HC_DEVICE_TYPE_E dev_type)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE %s = %d ORDER BY NUM;", "DEV_TYPE", dev_type);

    return sql_get_dev_log_v2(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_network_type_v2(HC_DEVICE_EXT_LOG_S* log, int size, HC_NETWORK_TYPE_E network_type)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE %s = %d ORDER BY NUM;", "NETWORK_TYPE", network_type);

    return sql_get_dev_log_v2(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_by_time_v2(HC_DEVICE_EXT_LOG_S* log, int size, long start_time, long end_time)
{
    char sql[SQL_BUF_LEN];

    if (start_time == 0)
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE TIME<=%ld ORDER BY NUM;", end_time);
    else if (end_time == 0)
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE TIME>%ld ORDER BY NUM;", start_time);
    else
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE TIME BETWEEN %ld AND %ld ORDER BY NUM;", start_time, end_time);

    return sql_get_dev_log_v2(log, size, sql);
}

DB_RETVAL_E db_get_dev_log_num(int* size)
{
    return sql_get_num(size, SQL_CMD_SELECT_DEVICE_LOG_NUM";");
}

DB_RETVAL_E db_get_dev_log_num_by_dev_id(unsigned int dev_id, int* size)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE %s = %u;", "DEV_ID", dev_id);

    return sql_get_num(size, sql);
}

DB_RETVAL_E db_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int* size)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE %s = %d;", "NETWORK_TYPE", network_type);

    return sql_get_num(size, sql);
}

DB_RETVAL_E db_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int* size)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE %s = %d;", "DEV_TYPE", dev_type);

    return sql_get_num(size, sql);
}

DB_RETVAL_E db_get_dev_log_num_by_time(int* size, long start_time, long end_time)
{
    char sql[SQL_BUF_LEN];

    if (start_time == 0)
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE TIME<=%ld", end_time);
    else if (end_time == 0)
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE TIME>%ld", start_time);
    else
        sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE TIME BETWEEN %ld AND %ld;", start_time, end_time);

    return sql_get_num(size, sql);
}

DB_RETVAL_E db_get_dev_log_num_by_alarm_type(int alarm_type, int* size)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_NUM"WHERE %s = %d;", "ALARM_TYPE", alarm_type);

    return sql_get_num(size, sql);
}

DB_RETVAL_E db_del_dev_log_by_dev_id(unsigned int dev_id)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_DEL_DEVICE_LOG"WHERE %s = %u;", "DEV_ID", dev_id);

    return sql_exec(0, NULL, sql, NULL, NULL);
}

DB_RETVAL_E db_del_dev_log_all()
{
    return sql_exec(0, NULL, SQL_CMD_DEL_DEVICE_LOG_ALL, NULL, NULL);
}


char* hc_get_dev_type_str(HC_DEVICE_TYPE_E index)
{
    switch (index)
    {
        case HC_DEVICE_TYPE_BINARYSWITCH:
            return "Binary Switch";
        case HC_DEVICE_TYPE_DIMMER:
            return "Dimmer";
        case HC_DEVICE_TYPE_BINARY_SENSOR:
            return "Binary Sensor";

        case HC_DEVICE_TYPE_DOORLOCK:
            return "Door Lock";
        case HC_DEVICE_TYPE_THERMOSTAT:
            return "Thermostat";
        case HC_DEVICE_TYPE_METER:
            return "Meter";
            
        default:
            return "No Device Type";
    }
    return NULL;
}

char* hc_get_network_type_str(HC_NETWORK_TYPE_E index)
{
    switch (index)
    {
        case HC_NETWORK_TYPE_ZWAVE:
            return "ZWave";
        case HC_NETWORK_TYPE_ZIGBEE:
            return "ZigBee";
        case HC_NETWORK_TYPE_ULE:
            return "ULE";
        case HC_NETWORK_TYPE_LPRF:
            return "LPRF";
        case HC_NETWORK_TYPE_CAMERA:
            return "Camera";
        default:
            return "No Network Type";
    }
    return NULL;
}

char* hc_get_event_type_str(HC_EVENT_TYPE_E index)
{
    switch (index)
    {
        case HC_EVENT_RESP_DEVICE_ADDED_SUCCESS:
            return "Device Added Success";
        case HC_EVENT_RESP_DEVICE_ADDED_FAILURE:
            return "Device Added Failure";
        case HC_EVENT_RESP_DEVICE_DELETED_SUCCESS:
            return "Device Deleted Success";
        case HC_EVENT_RESP_DEVICE_DELETED_FAILURE:
            return "Device Deleted Failure";
        case HC_EVENT_RESP_DEVICE_HAS_BEEN_REMOVED:
            return "Device Has Been Removed";
        case HC_EVENT_RESP_DEVICE_RESET_SUCCESS:
            return "Device Reset Success";
        case HC_EVENT_RESP_DEVICE_RESET_FAILURE:
            return "Device Reset Failure";
        case HC_EVENT_RESP_DEVICE_DEFAULT_SUCCESS:
            return "Device Default Success";
        case HC_EVENT_RESP_DEVICE_DEFAULT_FAILURE:
            return "Device Default Failure";
        case HC_EVENT_RESP_DEVICE_CONNECTED:
            return "Device Connected";
        case HC_EVENT_RESP_DEVICE_DISCONNECTED:
            return "Device Disconnected";
        case HC_EVENT_RESP_DEVICE_DETECT_FAILURE:
            return "Device Detect Failure";
        case HC_EVENT_RESP_DEVICE_NA_DELETE_SUCCESS:
            return "Device NA Delete Success";
        case HC_EVENT_RESP_DEVICE_NA_DELETE_FAILURE:
            return "Device NA Delete Failure";
        case HC_EVENT_RESP_DEVICE_SET_SUCCESS:
            return "Device Set Success";
        case HC_EVENT_RESP_DEVICE_SET_FAILURE:
            return "Device Set Failure";
        case HC_EVENT_RESP_DEVICE_GET_SUCCESS:
            return "Device Get Success";
        case HC_EVENT_RESP_DEVICE_GET_FAILURE:
            return "Device Get Failure";
        case HC_EVENT_STATUS_DEVICE_STATUS_CHANGED:
            return "Device Status Changed";
        case HC_EVENT_STATUS_DEVICE_LOW_BATTERY:
            return "Device Low Battery";
        case HC_EVENT_REQ_DEVICE_ADD:
            return "Device Add";
        case HC_EVENT_REQ_DEVICE_DELETE:
            return "Device Delete";
        case HC_EVENT_REQ_DEVICE_SET:
            return "Device Set";
        case HC_EVENT_REQ_DEVICE_GET:
            return "Device Get";
        case HC_EVENT_REQ_DEVICE_RESTART:
            return "Device Reset";
        case HC_EVENT_REQ_DEVICE_DEFAULT:
            return "Device Default";
        case HC_EVENT_REQ_DEVICE_DETECT:
            return "Device Detect";
        case HC_EVENT_REQ_DEVICE_NA_DELETE:
            return "Device NA Delete";
        case HC_EVENT_MAX:
            return "Event Max";
        default:
            return "No Event Type";
    }
    return NULL;
}

char* hc_get_alarm_type_str(DB_LOG_TYPE_E index)
{
    switch (index)
    {
        case DB_LOG_ALARM:
            return "Alarm";
        case DB_LOG_WARNING:
            return "Warning";
        default:
            return "No Alarm Type";
    }
    return NULL;
}

char* hc_get_operation_type_str(HC_USER_OPER_TYPE_E index)
{
    switch (index)
    {
        case HC_USER_OPER_ADD:
            return "User Add Operation";
        case HC_USER_OPER_SET:
            return "User Set Operation";
        case HC_USER_OPER_DELETE:
            return "User Delete Operation";
        default :
            return "No User Operation Type";
    }
    return NULL;
}

char* hc_get_operation_result_str(HC_USER_OPER_RESULT_E index)
{
    switch (index)
    {
        case HC_USER_OPER_SUCCESS:
            return "User Operation Success!";
        case HC_USER_OPER_FAIL:
            return "User Operation Fail!";
        default:
            return "No Operation Result";
    }
    return NULL;
}

DB_RETVAL_E sql_init_dev_info_table(sqlite3* db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "DEV_NODES");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check DEV_NODES Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]DEV_NODES TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_DEV_NODES_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create DEV_NODES TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type)
{
    int rc;
    sqlite3_stmt *stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN];
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_ADD_DEV_NODES_INFO, dev_type);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_blob(stat, 1, dev_nodes_ptr, size, NULL);
    rc = sqlite3_step(stat);

    while ((nRetryCnt < 10 ) && 
           (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
        nRetryCnt++;
        usleep(100000);
        rc = sqlite3_step(stat);
    }
    nRetryCnt = 0;

    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Add device nodes error !!\n");
        sql_close_db(db);
        return DB_EXEC_ERROR;
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_update_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type)
{
    int rc;
    sqlite3_stmt *stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN];
    int num = 0;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEV_NODES_NUM"WHERE DEV_TYPE=%d", dev_type);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create ZWAVE_NODES Table Error !!\n");
        sql_close_db(db);
        return DB_INIT_FAIL;
    }
    
    if(num == 0)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_ADD_DEV_NODES_INFO, dev_type);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_blob(stat, 1, dev_nodes_ptr, size, NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }

        sqlite3_finalize(stat);
    }
    else
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_SELECT_DEV_NODES_INFO, dev_type);
        rc = sqlite3_prepare_v2(db, SQL_CMD_UPDATE_DEV_NODES_INFO, -1, &stat, 0);
        rc = sqlite3_bind_blob(stat, 1, dev_nodes_ptr, size, NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }

        sqlite3_finalize(stat);
    }
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type)
{
    int rc;
    sqlite3_stmt *stat = NULL;
    const void* buffer;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN];

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEV_NODES_INFO, dev_type);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_step(stat);
    if (rc == SQLITE_ROW)
    {
        buffer = sqlite3_column_blob(stat, 1);
        memcpy(dev_nodes_ptr, buffer, size);
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type)
{
    return sql_get_dev_nodes_info(dev_nodes_ptr, size, dev_type);
}

DB_RETVAL_E db_set_dev_nodes_info(void* dev_nodes_ptr, int size, DB_DEV_TYPE_E dev_type)
{
    return sql_update_dev_nodes_info(dev_nodes_ptr, size, dev_type);
}

DB_RETVAL_E db_init_user_log_table(sqlite3* db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "USER_LOG");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check USER_LOG Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]USER_LOG TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_USER_LOG_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create USER_LOG TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_user_log(HC_USER_LOG_SEVERITY_E severity, char *content)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    int count = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_USER_LOG_NUM);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);

    if ((count + 1) <= MAX_USER_LOG_NUM)
    {
        //"(ID, SEVERITY, CONTENT) " 
        sprintf(sql_stmt, SQL_CMD_ADD_USER_LOG, severity, content);
        DEBUG_INFO("[DB][sql_add_user_log]sql_stmt is %s\n", sql_stmt);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add log fail !!\n");
            sql_close_db(db);
            return DB_ADD_LOG_FAIL;
        }
    }
    else
    {
        sprintf(sql_stmt, SQL_CMD_ADD_USER_LOG_FULL, severity, content);
        DEBUG_INFO("[DB][sql_add_user_log]sql_stmt is %s\n", sql_stmt);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]update user log fail !!\n");
            sql_close_db(db);
            return DB_ADD_LOG_FAIL;
        }
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_user_log(HC_DEVICE_EXT_USER_LOG_S* log, int size, char* sql_stmt)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    const unsigned char* user_name_ptr;
    const unsigned char* time_str_ptr;
    const unsigned char* log_ptr;
    const unsigned char* log_name_ptr;
    const unsigned char* ext_1_ptr;
    const unsigned char* ext_2_ptr;
    int i = 0;
    sqlite3* db;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }

    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))    
    {
        user_name_ptr = sqlite3_column_text(stat, 0);
        log[i].operation= sqlite3_column_int(stat, 1);
        log[i].result= sqlite3_column_int(stat, 2);
        log_ptr = sqlite3_column_text(stat, 3);
        log[i].log_time= sqlite3_column_int(stat, 4);
        time_str_ptr = sqlite3_column_text(stat, 5);
        log_name_ptr = sqlite3_column_text(stat, 6);
        ext_1_ptr = sqlite3_column_text(stat, 7);
        ext_2_ptr = sqlite3_column_text(stat, 8);

        memset(log[i].user_name, 0, DEVICE_NAME_LEN);
        strncpy(log[i].user_name, (char*)user_name_ptr, DEVICE_NAME_LEN-1);
        log[i].user_name[DEVICE_NAME_LEN-1]='\0';

        memset(log[i].log, 0, SQL_LOG_LEN);
        strncpy(log[i].log, (char*)log_ptr, SQL_LOG_LEN-1);
        log[i].log[SQL_LOG_LEN-1]='\0';

        memset(log[i].log_time_str, 0, TIME_STR_LEN);
        strncpy(log[i].log_time_str, (char*)time_str_ptr, TIME_STR_LEN-1);
        log[i].log_time_str[TIME_STR_LEN-1]='\0';

        memset(log[i].ext_1, 0, LOG_EXT_LEN);
        strncpy(log[i].ext_1, (char*)ext_1_ptr, LOG_EXT_LEN-1);
        log[i].ext_1[LOG_EXT_LEN-1]='\0';

        memset(log[i].ext_2, 0, LOG_EXT_LEN);
        strncpy(log[i].ext_2, (char*)ext_2_ptr, LOG_EXT_LEN-1);
        log[i].ext_2[LOG_EXT_LEN-1]='\0';
        //DEBUG_INFO("[sql_get_user_log]get log[%d] is [%s][%d][%d][%s][%ld][%s][%s][%s][%s]\n", i, log[i].user_name,
        //            log[i].operation, log[i].result, log[i].log, 
        //            log[i].log_time, log[i].log_time_str, log[i].log_name,
        //            log[i].ext_1, log[i].ext_2);
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_user_log(HC_USER_LOG_SEVERITY_E severity, char *content)
{
    int ret = DB_RETVAL_OK;
    
    ret = sql_add_user_log(severity, content);
    if (DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E db_get_user_log_all(HC_DEVICE_EXT_USER_LOG_S* log_ptr, int size)
{
    return sql_get_user_log(log_ptr, size, SQL_CMD_SELECT_USER_LOG_V2"ORDER BY NUM DESC;");
}

DB_RETVAL_E db_get_user_log_all_desc(HC_DEVICE_EXT_USER_LOG_S* log_ptr, int size)
{
    return sql_get_user_log(log_ptr, size, SQL_CMD_SELECT_USER_LOG_V2"ORDER BY NUM DESC;");
}

DB_RETVAL_E db_get_user_log_by_user_name(HC_DEVICE_EXT_USER_LOG_S* log_ptr, int size, char* usr_name)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_USER_LOG_V2"WHERE %s = \'%s\' ORDER BY NUM DESC;", "USER_NAME", usr_name);

    return sql_get_user_log(log_ptr, size, sql);
}

DB_RETVAL_E db_get_user_log_by_time(HC_DEVICE_EXT_USER_LOG_S* log_ptr, int size, long start_time, long end_time)
{
    char sql[SQL_BUF_LEN];

    if (start_time == 0)
        sprintf(sql, SQL_CMD_SELECT_USER_LOG_V2"WHERE TIME<=%ld ORDER BY NUM DESC;", end_time);
    else if (end_time == 0)
        sprintf(sql, SQL_CMD_SELECT_USER_LOG_V2"WHERE TIME>%ld ORDER BY NUM DESC;", start_time);
    else
        sprintf(sql, SQL_CMD_SELECT_USER_LOG_V2"WHERE TIME BETWEEN %ld AND %ld ORDER BY NUM DESC;", start_time, end_time);

    return sql_get_user_log(log_ptr, size, sql);
}


DB_RETVAL_E db_get_user_log_by_index(HC_DEVICE_EXT_USER_LOG_S* log_ptr, int size, int index)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_USER_LOG_V2"ORDER BY NUM DESC LIMIT %d,%d;", index, size);

    return sql_get_user_log(log_ptr, size, sql);
}

DB_RETVAL_E db_get_user_log_by_user_name_by_index(HC_DEVICE_EXT_USER_LOG_S* log_ptr, int size, int index, char* usr_name)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_USER_LOG_V2"WHERE USER_NAME = \'%s\' ORDER BY NUM DESC LIMIT %d,%d;", usr_name, index, size);

    return sql_get_user_log(log_ptr, size, sql);
}


DB_RETVAL_E db_get_user_log_num(int* size)
{
    return sql_get_num(size, SQL_CMD_SELECT_USER_LOG_NUM";");
}

DB_RETVAL_E db_get_user_log_num_by_user_name(char* usr_name, int* size)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_USER_LOG_NUM"WHERE %s = %s;", "USER_NAME", usr_name);

    return sql_get_num(size, sql);
}

DB_RETVAL_E db_get_user_log_num_by_time(int* size, long start_time, long end_time)
{
    char sql[SQL_BUF_LEN];

    if (start_time == 0)
        sprintf(sql, SQL_CMD_SELECT_USER_LOG_NUM"WHERE TIME<=%ld", end_time);
    else if (end_time == 0)
        sprintf(sql, SQL_CMD_SELECT_USER_LOG_NUM"WHERE TIME>%ld", start_time);
    else
        sprintf(sql, SQL_CMD_SELECT_USER_LOG_NUM"WHERE TIME BETWEEN %ld AND %ld;", start_time, end_time);

    return sql_get_num(size, sql);
}


DB_RETVAL_E db_del_user_log_by_dev_id(char* usr_name)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_DEL_USER_LOG"WHERE %s = \'%s\';", "USER_NAME", usr_name);

    return sql_exec(0, NULL, sql, NULL, NULL);
}

DB_RETVAL_E db_del_user_log_all()
{
    return sql_exec(0, NULL, SQL_CMD_DEL_USER_LOG_ALL, NULL, NULL);
}

DB_RETVAL_E sql_add_dev_name(unsigned int dev_id, char* dev_name)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    sqlite3_stmt * stat = NULL;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_ADD_DEVICE_NAME, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, dev_name, strlen(dev_name), NULL);
        rc = sqlite3_step(stat);

        while ((nRetryCnt < 10 ) &&
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0;
        
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_ERROR("[DB]Add log fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sqlite3_finalize(stat);
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    sqlite3_finalize(stat);
    
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_dev_ext(HC_DEVICE_EXT_INFO_S *dev_ext_info, int size)
{
    int i;
    int rc;
    sqlite3* db;
    sqlite3_stmt * stat = NULL;
    void* buffer;
    int buffer_size;

    if ((size <= 0) || (dev_ext_info == NULL))
    {
        return DB_PARAM_ERROR;
    }

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    for (i = 0; i < size; i++)
    {
        //DEBUG_INFO("Add Dev[%d], which dev_id is 0x%x, event_type is %s, " 
        //           "dev_type is %s, network_type is %s\n", i,
        //           dev_info[i].dev_id,
        //           hc_get_event_type_str(dev_info[i].event_type),
        //           hc_get_dev_type_str(dev_info[i].dev_type),
        //           hc_get_network_type_str(dev_info[i].network_type));

        buffer_size = sizeof(HC_DEVICE_EXT_U);
        buffer = (void*) & (dev_ext_info[i].device_ext.stuff[0]);

        rc = sqlite3_prepare_v2(db, SQL_CMD_ADD_DEVICE_EXT, -1, &stat, 0);
        rc = sqlite3_bind_int(stat, 1, 0);
        rc = sqlite3_bind_int(stat, 2, dev_ext_info[i].dev_id);
        rc = sqlite3_bind_int(stat, 3, dev_ext_info[i].event_type);
        rc = sqlite3_bind_int(stat, 4, dev_ext_info[i].network_type);
        rc = sqlite3_bind_int(stat, 5, dev_ext_info[i].dev_type);
        rc = sqlite3_bind_blob(stat, 6, buffer, buffer_size, NULL);
        rc = sqlite3_step(stat);
        if (rc != SQLITE_DONE)
        {
            DEBUG_ERROR("[DB]Add devices fail(%d):%s!\n", rc, sqlite3_errmsg(db));
        }

        sqlite3_finalize(stat);
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_dev_ext(HC_DEVICE_EXT_INFO_S *dev_ext_info, int size, const char* sql_stmt)
{
    int rc;
    const void* buf;
    int buffer_size;
    int i = 0;
    sqlite3* db;
    sqlite3_stmt* stat;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);

    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        dev_ext_info[i].dev_id = sqlite3_column_int(stat, 1);
        dev_ext_info[i].event_type = sqlite3_column_int(stat, 2);
        dev_ext_info[i].network_type = sqlite3_column_int(stat, 3);
        dev_ext_info[i].dev_type = sqlite3_column_int(stat, 4);
        buffer_size = sizeof(HC_DEVICE_EXT_U);
        buf = sqlite3_column_blob(stat, 5);
        memcpy(&(dev_ext_info[i].device_ext.stuff[0]), (unsigned char*)buf, buffer_size);
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }

    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_dev_ext(HC_DEVICE_EXT_INFO_S* devs_ext_info, int size)
{
    int i;
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int dev_num = 0;
    void *buffer;
    int buffer_size;
    sqlite3_stmt* stat = NULL;
    sqlite3* db;
    int nRetryCnt = 0;

    if ((size <= 0) || (devs_ext_info == NULL))
    {
        return DB_PARAM_ERROR;
    }

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    for (i = 0; i < size; i++)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_EXT_NUM"WHERE DEV_ID=%u", devs_ext_info[i].dev_id);
        rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&dev_num);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
            continue;
        }

        buffer_size = sizeof(HC_DEVICE_U);
        buffer = (void*) & (devs_ext_info[i].device_ext.stuff[0]);

        if (dev_num == 0)
        {
            //DEBUG_INFO("No exist record, add new device");

            rc = sqlite3_prepare_v2(db, SQL_CMD_ADD_DEVICE_EXT, -1, &stat, 0);
            rc = sqlite3_bind_int(stat, 1, 0);
            rc = sqlite3_bind_int(stat, 2, devs_ext_info[i].dev_id);
            rc = sqlite3_bind_int(stat, 3, devs_ext_info[i].event_type);
            rc = sqlite3_bind_int(stat, 4, devs_ext_info[i].network_type);
            rc = sqlite3_bind_int(stat, 5, devs_ext_info[i].dev_type);
            rc = sqlite3_bind_blob(stat, 6, buffer, buffer_size, NULL);
            rc = sqlite3_step(stat);

            while ((nRetryCnt < 10 ) &&
                   (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
                DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
                nRetryCnt++;
                usleep(100000);
                rc = sqlite3_step(stat);
            }
            nRetryCnt = 0;

            sqlite3_finalize(stat);
        }
        else
        {
            sprintf(sql_stmt, SQL_CMD_UPDATE_DEVICE_EXT, devs_ext_info[i].dev_id);
            sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
            //rc = sqlite3_bind_int(stat, 1, devs_ext_info[i].event_type);
            rc = sqlite3_bind_blob(stat, 1, buffer, buffer_size, NULL);
            rc = sqlite3_step(stat);
            while ((nRetryCnt < 10 ) &&
                   (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
                DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
                nRetryCnt++;
                usleep(100000);
                rc = sqlite3_step(stat);
            }
            nRetryCnt = 0;

            sqlite3_finalize(stat);
        }
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_dev_ext(HC_DEVICE_EXT_INFO_S *dev_ext_info, int size)
{
    return sql_add_dev_ext(dev_ext_info, size);
}

DB_RETVAL_E db_get_dev_ext_by_dev_id(unsigned int dev_id, HC_DEVICE_EXT_INFO_S *dev_ext_info)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_EXT"WHERE DEV_ID=%u;", dev_id);

    return sql_get_dev_ext(dev_ext_info, 1, sql_stmt);
}

DB_RETVAL_E db_set_dev_ext(HC_DEVICE_EXT_INFO_S *dev_ext_info, int size)
{
    return sql_set_dev_ext(dev_ext_info, size);
}


DB_RETVAL_E db_del_dev_ext_by_dev_id(unsigned int dev_id)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_DEL_DEVICE_EXT"WHERE %s = %u;", "DEV_ID", dev_id);

    return sql_exec(0, NULL, sql, NULL, NULL);
}

DB_RETVAL_E db_get_dev_log_by_alarm_type(char* log, int size, int alarm_type)
{
    char sql[SQL_BUF_LEN];

    sprintf(sql, SQL_CMD_SELECT_DEVICE_LOG_V2"WHERE %s = %u ORDER BY NUM DESC;", "ALARM_TYPE", alarm_type);

    return sql_get_dev_log(log, size, sql);
}

DB_RETVAL_E db_set_armmode(int value, char* arm_id)
{
    HC_DEVICE_EXT_INFO_S arm_mode_s;
    memset(&arm_mode_s, 0, sizeof(HC_DEVICE_EXT_INFO_S));
    arm_mode_s.device_ext.armmode.value = value;
    strncpy(arm_mode_s.device_ext.armmode.id, arm_id, 32);
    arm_mode_s.dev_id = ARMMODE_DEV_ID;
    db_set_dev_ext(&arm_mode_s, 1);
    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_armmode(int value, char* arm_id)
{
    HC_DEVICE_EXT_INFO_S arm_mode_s;
    memset(&arm_mode_s, 0, sizeof(HC_DEVICE_EXT_INFO_S));
    arm_mode_s.device_ext.armmode.value = value;
    strncpy(arm_mode_s.device_ext.armmode.id, arm_id, 32);
    arm_mode_s.dev_id = ARMMODE_DEV_ID;
    db_add_dev_ext(&arm_mode_s, 1);
    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_armmode(int value, char* arm_id)
{
    HC_DEVICE_EXT_INFO_S arm_mode_s;
    memset(&arm_mode_s, 0, sizeof(HC_DEVICE_EXT_INFO_S));
    arm_mode_s.dev_id = ARMMODE_DEV_ID;
    db_get_dev_ext_by_dev_id(ARMMODE_DEV_ID, &arm_mode_s);
    value = arm_mode_s.device_ext.armmode.value;
    strncpy(arm_id, arm_mode_s.device_ext.armmode.id, 32);
    return DB_RETVAL_OK;
}

DB_RETVAL_E db_del_armmode()
{
    db_del_dev_ext_by_dev_id(ARMMODE_DEV_ID);
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_dev_ext_attr(unsigned int dev_id, char* attr_name, char* attr_value)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    sqlite3_stmt* stat = NULL;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_ADD_EXT_ATTR, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, attr_name, strlen(attr_name), NULL);
        rc = sqlite3_bind_text(stat, 2, attr_value, strlen(attr_value), NULL);
        rc = sqlite3_step(stat);
		
        while ((nRetryCnt < 10 ) &&
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
		
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Add log fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    
    sqlite3_finalize(stat);
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_update_dev_ext_attr(unsigned int dev_id, char* attr_name, char* attr_value)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    int count;
    sqlite3_stmt* stat = NULL;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_EXT_NUM_BY_ID_AND_NAME, dev_id);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_text(stat, 1, attr_name, strlen(attr_name), NULL);
    rc = sqlite3_step(stat);
    while ((nRetryCnt < 10 ) &&
           (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
        nRetryCnt++;
        usleep(100000);
        rc = sqlite3_step(stat);
    }

    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Update dev ext attr Error(%d), %s\n", rc, sqlite3_errmsg(db));
    }
    count = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);

    if(count == 0)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_ADD_EXT_ATTR, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, attr_name, strlen(attr_name), NULL);
        rc = sqlite3_bind_text(stat, 2, attr_value, strlen(attr_value), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }

        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Add dev ext attr(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    else
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_UPDATE_EXT_ATTR, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, attr_value, strlen(attr_value), NULL);
        rc = sqlite3_bind_text(stat, 2, attr_name, strlen(attr_name), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }

        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Set dev ext attr(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }

    //DEBUG_INFO("Get EXT ATTR is [0x%x][%s][%s]", dev_id, attr_name, attr_value);
    sqlite3_finalize(stat);
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_dev_ext_attr_value(unsigned int dev_id, char* attr_name, char* attr_value)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN];
    const unsigned char * attr_value_ptr;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    
    //sprintf(sql_stmt, SQL_CMD_GET_EXT_ATTR_VALUE, dev_id, attr_name);
    sprintf(sql_stmt, SQL_CMD_GET_EXT_ATTR_VALUE, dev_id);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_bind_text(stat, 1, attr_name, strlen(attr_name), NULL);

    rc = sqlite3_step(stat);
    if(rc == SQLITE_ROW)
    {
        attr_value_ptr = sqlite3_column_text(stat, 0);
        strncpy(attr_value, (char*)attr_value_ptr, ATTR_VALUE_LEN-1);
        attr_value[ATTR_VALUE_LEN-1] = '\0';
    }
    else if(rc ==SQLITE_DONE)
    {
        DEBUG_INFO("[DB]No Device Name Record!\n");
    }
    sqlite3_finalize(stat);

    DEBUG_INFO("[DB]Get EXT ATTR is [0x%x][%s][%s]", dev_id, attr_name, attr_value);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_dev_ext_attr_all_by_dev_id(unsigned int dev_id, int size, HC_DEVICE_EXT_ATTR_S* attr_ptr)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN];
    const unsigned char * attr_value_ptr;
    const unsigned char * attr_name_ptr;
    int i = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    
    sprintf(sql_stmt, SQL_CMD_GET_EXT_ATTR_ALL_BY_DEV_ID, dev_id);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        attr_ptr[i].dev_id = dev_id;
        attr_name_ptr = sqlite3_column_text(stat, 0);
        strncpy(attr_ptr[i].attr_name, (char*)attr_name_ptr, ATTR_NAME_LEN-1);
        attr_ptr[i].attr_name[ATTR_NAME_LEN-1] = '\0';
        attr_value_ptr = sqlite3_column_text(stat, 1);
        strncpy(attr_ptr[i].attr_value, (char*)attr_value_ptr, ATTR_VALUE_LEN-1);
        attr_ptr[i].attr_value[ATTR_VALUE_LEN-1] = '\0';
        //DEBUG_INFO("Get EXT ATTR is [0x%x][%s][%s]\n", dev_id, attr_ptr[i].attr_name, attr_ptr[i].attr_value);
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }

    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_attr(unsigned dev_id, char* attr_name, char* attr_value)
{
    int ret;
    ret = sql_add_dev_ext_attr(dev_id, attr_name, attr_value);
    return ret;
}

DB_RETVAL_E db_get_attr(unsigned dev_id, char* attr_name, char* attr_value)
{
    int ret;
    ret = sql_get_dev_ext_attr_value(dev_id, attr_name, attr_value);
    return ret;
}

DB_RETVAL_E db_get_attr_all_by_id(unsigned dev_id, int size, HC_DEVICE_EXT_ATTR_S* attr_ptr)
{
    int ret;
    ret = sql_get_dev_ext_attr_all_by_dev_id(dev_id, size, attr_ptr);
    return ret;
}

DB_RETVAL_E db_set_attr(unsigned dev_id, char* attr_name, char* attr_value)
{
    int ret;
    ret = sql_update_dev_ext_attr(dev_id, attr_name, attr_value);
    return ret;
}

DB_RETVAL_E db_del_attr(unsigned dev_id, char* attr_name)
{
    char sql_stmt[SQL_BUF_LEN];
    sqlite3_stmt* stat;
    sqlite3* db;
    int rc;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_EXT_ATTR, dev_id);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_text(stat, 1, attr_name, strlen(attr_name), NULL);
    rc = sqlite3_step(stat);

    while ((nRetryCnt < 10 ) &&
          (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
        nRetryCnt++;
        usleep(100000);
        rc = sqlite3_step(stat);
    }
    nRetryCnt = 0 ;

    
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        sqlite3_finalize(stat);
        DEBUG_ERROR("[DB]Del attr fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
        sql_close_db(db);
        return DB_DEL_ATTR_FAIL;
    }

    sqlite3_finalize(stat);
    sql_close_db(db);    
    return DB_RETVAL_OK;
}

DB_RETVAL_E db_del_attr_by_dev_id(unsigned dev_id)
{
    char sql_stmt[SQL_BUF_LEN];
    int ret;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_EXT_ATTR_BY_ID, dev_id);
    ret = sql_exec(0, NULL, sql_stmt, NULL, NULL);
    return ret;
}

DB_RETVAL_E db_get_attr_num_by_dev_id(unsigned int dev_id, int *attr_num)
{
    char sql_stmt[SQL_BUF_LEN];
    int ret;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_EXT_NUM_BY_ID, dev_id);
    ret = sql_get_num(attr_num, sql_stmt);
    return ret;
}

DB_RETVAL_E db_del_attr_all()
{
    return sql_exec(0, NULL, SQL_CMD_DEL_EXT_ATTR_ALL, NULL, NULL);
}

DB_RETVAL_E sql_add_scene(char* scene_id, char* scene_attr)
{
    char sql_stmt[SQL_BUF_LEN_L];
    int rc;
    sqlite3* db;
    sqlite3_stmt* stat = NULL;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    {
        memset(sql_stmt, 0, SQL_BUF_LEN_L);

        sprintf(sql_stmt, SQL_CMD_ADD_SCENE);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, scene_id, strlen(scene_id), NULL);
        rc = sqlite3_bind_text(stat, 2, scene_attr, strlen(scene_attr), NULL);
        rc = sqlite3_step(stat);

        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0 ;


        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_ERROR("[DB]Add scene fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sqlite3_finalize(stat);
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_update_scene(char* scene_id, char* scene_attr)
{
    char sql_stmt[SQL_BUF_LEN_L];
    int rc;
    sqlite3* db;
    int size;
    sqlite3_stmt* stat = NULL;
    const unsigned char * scene_size;
    int nRetryCnt = 0 ;


    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    memset(sql_stmt, 0, SQL_BUF_LEN_L);
    sprintf(sql_stmt, SQL_CMD_SELET_SCENE_NUM_BY_ID);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_text(stat, 1, scene_id, strlen(scene_id), NULL);
    rc = sqlite3_step(stat);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Get scene num fail(%d),  error: %s\n", rc, sqlite3_errmsg(db));
    }
    size = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);

    if(size == 0)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN_L);

        sprintf(sql_stmt, SQL_CMD_ADD_SCENE);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, scene_id, strlen(scene_id), NULL);
        rc = sqlite3_bind_text(stat, 2, scene_attr, strlen(scene_attr), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0 ;


        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Add scene fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    else
    {
        memset(sql_stmt, 0, SQL_BUF_LEN_L);

        sprintf(sql_stmt, SQL_CMD_UPDATE_SCENE);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, scene_attr, strlen(scene_attr), NULL);
        rc = sqlite3_bind_text(stat, 2, scene_id, strlen(scene_id), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0 ;


        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Set scene fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_scene(char* scene_id, char* scene_attr)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN_L];
    const unsigned char * dev_name_ptr;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN_L);
    
    sprintf(sql_stmt, SQL_CMD_GET_SCENE);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_bind_text(stat, 1, scene_id, strlen(scene_id), NULL);
    
    if(sqlite3_step(stat) == SQLITE_ROW)
    {
        dev_name_ptr = sqlite3_column_text(stat, 0);
        strncpy(scene_attr, (char*)dev_name_ptr, SCENE_ATTR_LEN);
        scene_attr[SCENE_ATTR_LEN-1] = '\0';
    }
    else if(sqlite3_step(stat) ==SQLITE_OK)
    {
        DEBUG_INFO("[DB]No Device Name Record!\n");
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S* scene_ptr)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    const unsigned char * scene_attr_ptr;
    const unsigned char * scene_id_ptr;
    int i = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    rc = sqlite3_prepare(db, SQL_CMD_GET_SCENE_ALL, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }

    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))    {
        scene_id_ptr = sqlite3_column_text(stat, 0);
        strncpy(scene_ptr[i].scene_id, (char*)scene_id_ptr, SCENE_ID_LEN-1);
        scene_ptr[i].scene_id[SCENE_ID_LEN-1] = '\0';
        scene_attr_ptr = sqlite3_column_text(stat, 1);
        strncpy(scene_ptr[i].scene_attr, (char*)scene_attr_ptr, SCENE_ATTR_LEN-1);
        scene_ptr[i].scene_attr[SCENE_ATTR_LEN-1] = '\0';
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }

    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_scene(char* scene_id, char* scene_attr)
{
    return sql_add_scene(scene_id, scene_attr);
}

DB_RETVAL_E db_update_scene(char* scene_id, char* scene_attr)
{
    return sql_update_scene(scene_id, scene_attr);
}

DB_RETVAL_E db_get_scene(char* scene_id, char* scene_attr)
{
    return sql_get_scene(scene_id, scene_attr);
}


DB_RETVAL_E db_get_scene_num(int *size)
{
    char sql_stmt[SQL_BUF_LEN];
    int ret;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_SCENE_NUM_ALL);
    ret = sql_get_num(size, sql_stmt);
    return ret;
}

DB_RETVAL_E db_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S* scene_attr)
{
    return sql_get_scene_all(size, scene_attr);
}


DB_RETVAL_E db_del_scene_by_scene_id(char* scene_id)
{
    char sql_stmt[SQL_BUF_LEN];
    sqlite3_stmt* stat;
    sqlite3* db;
    int rc;
    int nRetryCnt = 0 ;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_SCENE);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_text(stat, 1, scene_id, strlen(scene_id), NULL);
    rc = sqlite3_step(stat);

    while ((nRetryCnt < 10 ) &&
          (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
        nRetryCnt++;
        usleep(100000);
        rc = sqlite3_step(stat);
    }
    nRetryCnt = 0 ;


    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        sqlite3_finalize(stat);
        DEBUG_ERROR("[DB]Del scene fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
        sql_close_db(db);
        return DB_DEL_SCENE_FAIL;
    }

    sqlite3_finalize(stat);
    sql_close_db(db);    

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_del_scene_all()
{
    return sql_exec(0, NULL, SQL_CMD_DEL_SCENE_ALL, NULL, NULL);
}

DB_RETVAL_E db_check_valid_dev_id(unsigned int dev_id)
{
    HC_NETWORK_TYPE_E network_type;
    network_type = GLOBAL_ID_TO_NETWORK_TYPE(dev_id);
    if((network_type<=0) || (network_type >=HC_NETWORK_TYPE_MAX))
    {
        DEBUG_ERROR("[DB]device id(%u) wrong, the network type is out of range %d", dev_id, network_type);
        return DB_WRONG_DEV_ID;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_conf(char* conf_name, char* conf_value)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_ADD_CONF, conf_name, conf_value);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add Conf fail !!\n");
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_update_conf(char* conf_name, char* conf_value)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    int size = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_CONF, conf_name);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
    }

    if(size == 0)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_ADD_CONF, conf_name, conf_value);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add log fail !!\n");
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    else
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_UPDATE_CONF, conf_value, conf_name);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Add log fail !!\n");
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_conf(char* conf_name, char* conf_value)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN];
    const unsigned char * conf_value_ptr;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    
    sprintf(sql_stmt, SQL_CMD_GET_CONF, conf_name);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    if(sqlite3_step(stat) == SQLITE_ROW)
    {
        conf_value_ptr = sqlite3_column_text(stat, 0);
        strncpy(conf_value, (char*)conf_value_ptr, CONF_VALUE_LEN-1);
        conf_value[CONF_VALUE_LEN-1] = '\0';
    }
    else if(sqlite3_step(stat) == SQLITE_OK)
    {
        DEBUG_INFO("[DB]No Configure Record!\n");
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_init_conf_table(sqlite3* db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "CONF_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check CONF_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]CONF_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_CONF_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create CONF_TABLE TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

void db_add_database_table_version()
{
    DEBUG_INFO("[DB]No Version Before, Add One");
    sql_add_conf(DB_VERSION_NAME, DB_VERSION);
}

void db_set_database_table_version()
{
    DEBUG_INFO("[DB]Update Version\n");
    sql_update_conf(DB_VERSION_NAME, DB_VERSION);
}

int db_check_datatable_version()
{    
    char ver[CONF_VALUE_LEN];
    int ver_num;
    int cur_num;

    memset(ver, 0, sizeof(ver));
    db_get_database_table_version(ver);
    ver_num = atoi(ver);
    cur_num = atoi(DB_VERSION);

    DEBUG_INFO("[DB]The db version is (%d), while new db version is (%d)", ver_num, cur_num);

    if(cur_num <= ver_num)
        return 0;
    else
        return cur_num - ver_num;
}

void db_get_database_table_version(char* version)
{
    sql_get_conf(DB_VERSION_NAME, version);
}

void db_update_database(int ver_gap)
{
    //add work around solution
    db_set_database_table_version();
}

DB_RETVAL_E db_set_conf(char* conf_name, char* conf_value)
{
    return sql_update_conf(conf_name, conf_value);
}

DB_RETVAL_E db_get_conf(char* conf_name, char* conf_value)
{
    return sql_get_conf(conf_name, conf_value);
}


//DEV_TYPE_S dev_type_s[] = 
//{
//    {HC_DEVICE_TYPE_MULTILEVEL_SENSOR, sizeof(multi_sensor_value_s)/sizeof(DB_DEV_VALUE_S), multi_sensor_value_s}
//};

//int dev_type_num = sizeof(dev_type_s)/sizeof(DEV_TYPE_S);

int stoi(char *str, OUT int *value)
{
    int ret = 1;
    int tmpValue = 0;

    if (NULL != str && strlen(str) > 0)
    {
        int i;
        for (i = 0; i < strlen(str); i++)
        {
            if (!isdigit(str[i]) && ('-' != str[i] || i != 0 || strlen(str) <= 1))
            {
                ret = 0;
                break;
            }
        }
    }
    else
    {
        ret = 0;
    }

    if (ret)
    {
        if ('-' == str[0])
            tmpValue = 0 - atoi(str + 1);
        else
            tmpValue = atoi(str);
    }
    *value = tmpValue;

    return ret;
}

extern int string_to_unsigned_num(char *str, OUT int *value)
{
    int ret = 1;
    int i = 0 , str_len, j = 1;

    if (NULL == value || NULL == str || 0 == strlen(str))
    {
        ret = 0;
    }
    else
    {
        str_len = strlen(str);

        *value = 0;

        for (i = 0; i < str_len; i++)
        {
            if (!isdigit(str[str_len - i - 1]))
            {
                ret = 0;

                break;
            }

            *value +=  j * (str[str_len - i - 1] - '0');

            j = j * 10;
        }
    }

    return ret;
}

void get_value(DBValueType type, char *value, char *data)
{

    if (NULL == value || NULL == data)
    {
        return;
    }

    if (TYPE_STRING == type || TYPE_DATETIME == type)
    {
        strcpy(data, value);
    }
    else if (TYPE_BOOL == type)
    {
        if (0 == strcasecmp(value, "true") || 0 == strcmp(value, "1"))
        {
            *((int *)data) = 1;
        }
        else if (0 == strcasecmp(value, "false") || 0 == strcmp(value, "0"))
        {
            *((int *)data) = 0;
        }
    }
    else if (TYPE_UINT == type)
    {
        string_to_unsigned_num(value, (unsigned int *)data);
    }
    else if (TYPE_DOUBLE == type)
    {
        //get double value
    }
    else
    {
        stoi(value, (int *)data);
    }
}

/*
void db_get_multi_sensor(unsigned int dev_id, HC_DEVICE_MULTILEVEL_SENSOR_S *multilevel_sensor)
{
    char *data = NULL;
    char *value = NULL;
    int i;

    while(0 != strlen(multi_sensor_value_s[i].name))
    {
        data = (unsigned int)multilevel_sensor + (unsigned int)multi_sensor_value_s[i].param_offset;

        //value ;
        
        get_value(multi_sensor_value_s[i].type, value, data);
    }
}


#define SQL_ADD_DEV_VALUE_HEAD "UPDATE DEVICE_EXT_TABLE SET ATTR_VALUE=\'"
#define SQL_ADD_DEV_VALUE_TAIL "\' WHERE DEV_ID=%u AND ATTR_NAME=\'%s\';"


void db_combine_sql_stmt(char* sql_stmt, int type, char * data, unsigned int dev_id, char* name)
{
    switch(type)
    {
        case TYPE_INT:
            sprintf(sql_stmt, SQL_ADD_DEV_VALUE_HEAD"%d"SQL_ADD_DEV_VALUE_TAIL, *((int *)data), dev_id, name);
            break;
        case TYPE_STRING:
            sprintf(sql_stmt, SQL_ADD_DEV_VALUE_HEAD"%s"SQL_ADD_DEV_VALUE_TAIL, ((char *)data), dev_id, name);
            break;
        case TYPE_DOUBLE:
            sprintf(sql_stmt, SQL_ADD_DEV_VALUE_HEAD"%lf"SQL_ADD_DEV_VALUE_TAIL, *((double *)data), dev_id, name);
            break;
    }
}

void db_set_device_value(sqlite3* db, unsigned int dev_id, HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U * dev_ptr)
{
    switch(dev_type)
    {
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            db_set_multi_sensor(db, dev_id, (HC_DEVICE_MULTILEVEL_SENSOR_S *)dev_ptr);
            break;
    }
}

DB_RETVAL_E db_set_multi_sensor(sqlite3* db, unsigned int dev_id, HC_DEVICE_MULTILEVEL_SENSOR_S *multilevel_sensor)
{
    char sql_stmt[SQL_BUF_LEN];
    char *data = NULL;
    int i = 0;
    int rc;
    char *zErrMsg = 0;
    char *sql;
    int sql_size;

    sql_size = sizeof(multi_sensor_value_s)/sizeof(DB_MAP_NODE_S);
    sql = (char*) malloc(SQL_BUF_LEN * sql_size);
    while(0 != strlen(multi_sensor_value_s[i].name))
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        
        data = (unsigned int)multilevel_sensor + (unsigned int)multi_sensor_value_s[i].param_offset;
        
        db_combine_sql_stmt(sql_stmt, multi_sensor_value_s[i].type, data, dev_id, multi_sensor_value_s[i].name);
        
        strcpy(sql, sql_stmt);
        
        i++;
    }

    rc = sqlite3_exec(db, sql_stmt, NULL, 0, &zErrMsg);
        
    if (rc != SQLITE_OK) {
        DEBUG_ERROR("Save error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return rc;
}
*/

DB_RETVAL_E sql_init_location_table(sqlite3* db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "LOCATION_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check LOCATION_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]LOCATION_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_LOCATION_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create LOCATION_TABLE TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}


DB_RETVAL_E sql_add_location(char* location_id, char* location_attr)
{
    char sql_stmt[SQL_BUF_LEN_L];
    int rc;
    sqlite3* db;
    sqlite3_stmt* stat = NULL;
    int nRetryCnt = 0 ;


    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    {
        memset(sql_stmt, 0, SQL_BUF_LEN_L);

        sprintf(sql_stmt, SQL_CMD_ADD_LOCATION);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, location_id, strlen(location_id), NULL);
        rc = sqlite3_bind_text(stat, 2, location_attr, strlen(location_attr), NULL);
        rc = sqlite3_step(stat);

        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0 ;


        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Add location fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_update_location(char* location_id, char* location_attr)
{
    char sql_stmt[SQL_BUF_LEN_L];
    int rc;
    sqlite3* db;
    int size;
    sqlite3_stmt* stat = NULL;
    const unsigned char * location_size;
    int nRetryCnt = 0 ;


    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    memset(sql_stmt, 0, SQL_BUF_LEN_L);
    sprintf(sql_stmt, SQL_CMD_SELET_LOCATION_NUM_BY_ID);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_text(stat, 1, location_id, strlen(location_id), NULL);
    rc = sqlite3_step(stat);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Get location num fail(%d),  error: %s\n", rc, sqlite3_errmsg(db));
    }
    size = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);

    if(size == 0)
    {
        memset(sql_stmt, 0, SQL_BUF_LEN_L);

        sprintf(sql_stmt, SQL_CMD_ADD_LOCATION);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, location_id, strlen(location_id), NULL);
        rc = sqlite3_bind_text(stat, 2, location_attr, strlen(location_attr), NULL);
        rc = sqlite3_step(stat);

        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0 ;


        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Add location fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    else
    {
        memset(sql_stmt, 0, SQL_BUF_LEN_L);

        sprintf(sql_stmt, SQL_CMD_UPDATE_LOCATION);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, location_attr, strlen(location_attr), NULL);
        rc = sqlite3_bind_text(stat, 2, location_id, strlen(location_id), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
              (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        nRetryCnt = 0 ;


        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]Set location fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_location(char* location_id, char* location_attr)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    char sql_stmt[SQL_BUF_LEN_L];
    const unsigned char * dev_name_ptr;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN_L);
    
    sprintf(sql_stmt, SQL_CMD_GET_LOCATION);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_bind_text(stat, 1, location_id, strlen(location_id), NULL);
    
    if(sqlite3_step(stat) == SQLITE_ROW)
    {
        dev_name_ptr = sqlite3_column_text(stat, 0);
        strncpy(location_attr, (char*)dev_name_ptr, LOCATION_ATTR_LEN);
        location_attr[LOCATION_ATTR_LEN-1] = '\0';
    }
    else if(sqlite3_step(stat) ==SQLITE_OK)
    {
        DEBUG_INFO("[DB]No Device Name Record!\n");
    }
    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S* location_ptr)
{
    int rc;
    sqlite3_stmt * stat = NULL;
    sqlite3* db;
    const unsigned char * location_attr_ptr;
    const unsigned char * location_id_ptr;
    int i = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    rc = sqlite3_prepare(db, SQL_CMD_GET_LOCATION_ALL, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }

    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        sql_close_db(db);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))    {
        location_id_ptr = sqlite3_column_text(stat, 0);
        strncpy(location_ptr[i].location_id, (char*)location_id_ptr, LOCATION_ID_LEN-1);
        location_ptr[i].location_id[LOCATION_ID_LEN-1] = '\0';
        location_attr_ptr = sqlite3_column_text(stat, 1);
        strncpy(location_ptr[i].location_attr, (char*)location_attr_ptr, LOCATION_ATTR_LEN-1);
        location_ptr[i].location_attr[LOCATION_ATTR_LEN-1] = '\0';
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }

    sqlite3_finalize(stat);

    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_location(char* location_id, char* location_attr)
{
    DEBUG_INFO("sql_add_location(%s, %s)", location_id, location_attr);
    return sql_add_location(location_id, location_attr);
}

DB_RETVAL_E db_update_location(char* location_id, char* location_attr)
{
    return sql_update_location(location_id, location_attr);
}

DB_RETVAL_E db_get_location(char* location_id, char* location_attr)
{
    return sql_get_location(location_id, location_attr);
}

DB_RETVAL_E db_get_location_num(int *size)
{
    char sql_stmt[SQL_BUF_LEN];
    int ret;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_LOCATION_NUM_ALL);
    ret = sql_get_num(size, sql_stmt);
    return ret;
}

DB_RETVAL_E db_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S* location_ptr)
{
    return sql_get_location_all(size, location_ptr);
}


DB_RETVAL_E db_del_location_by_location_id(char* location_id)
{
    char sql_stmt[SQL_BUF_LEN];
    sqlite3_stmt* stat;
    sqlite3* db;
    int rc;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_LOCATION);
    rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
    rc = sqlite3_bind_text(stat, 1, location_id, strlen(location_id), NULL);
    rc = sqlite3_step(stat);

    while ((nRetryCnt < 10 ) &&
          (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
        nRetryCnt++;
        usleep(100000);
        rc = sqlite3_step(stat);
    }
    nRetryCnt = 0;


    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
        sqlite3_finalize(stat);
        DEBUG_ERROR("[DB]Del location fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
        sql_close_db(db);
        return DB_DEL_SCENE_FAIL;
    }

    sqlite3_finalize(stat);
    sql_close_db(db);    

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_del_location_all()
{
    return sql_exec(0, NULL, SQL_CMD_DEL_LOCATION_ALL, NULL, NULL);
}

void dbd_sync_db()
{
    //char syscmd[256] = {0};
    //snprintf(syscmd, sizeof(syscmd) - 1, "cp %s %s", DB_PATH, DB_STORAGE_PATH);
    //system(syscmd);
    system("sync;sync;sync");
}

/*************APIs for DB Demon Use**************/
DB_RETVAL_E dbd_add_dev(HC_DEVICE_INFO_S *dev_info)
{  
    int ret = DB_RETVAL_OK;
    
    if(DB_WRONG_DEV_ID == db_check_valid_dev_id(dev_info->dev_id))
        return DB_WRONG_DEV_ID;
    
    ret = db_add_dev(dev_info, 1);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_get_dev(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr)
{
    return db_get_dev_by_dev_id(dev_info->dev_id, dev_ptr);
}

DB_RETVAL_E dbd_del_dev(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    char sql[SQL_BUF_LEN] = {0};

#ifndef BLOB_DATA
    sprintf(sql, SQL_CMD_DEL_DEVICE"WHERE %s = %u;"SQL_CMD_DEL_DEV_VALUE_BY_ID, "DEV_ID", dev_info->dev_id, dev_info->dev_id);
#else
    sprintf(sql, SQL_CMD_DEL_DEVICE"WHERE %s = %u;", "DEV_ID", dev_info->dev_id);
#endif
    DEBUG_INFO("sql is [%s]", sql);

    ret = sql_exec(0, NULL, sql, NULL, NULL);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_set_dev(HC_DEVICE_INFO_S *devs_info)
{
    int ret = DB_RETVAL_OK;

    if(DB_WRONG_DEV_ID == db_check_valid_dev_id(devs_info->dev_id))
        return DB_WRONG_DEV_ID;

    ret = sql_set_dev(devs_info, 1);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_del_dev_all()
{
    int ret = DB_RETVAL_OK;

    ret = db_del_dev_all();
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_add_log(HC_DEVICE_INFO_S *dev_info)
{   
    HC_DEVICE_INFO_S dev_tmp;
    int ret = DB_RETVAL_OK;

    dev_tmp.dev_id =dev_info->device.ext_u.add_log.dev_id;
    dev_tmp.dev_type = dev_info->device.ext_u.add_log.dev_type;
    dev_tmp.network_type = dev_info->device.ext_u.add_log.network_type;
    dev_tmp.event_type = dev_info->device.ext_u.add_log.event_type;
    

    ret = db_add_dev_log(dev_tmp, dev_info->device.ext_u.add_log.log, 1, dev_info->device.ext_u.add_log.log_time, 
                    dev_info->device.ext_u.add_log.alarm_type, dev_info->device.ext_u.add_log.log_name);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_get_log_all(HC_DEVICE_INFO_S * dev_info, char* dev_ptr)
{
    int ret;
    ret = db_get_dev_log_all(dev_ptr, dev_info->device.ext_u.get_log.log_num);
    DEBUG_INFO("[DB]db_get_dev_log_all (%d)", ret);
    return ret;
}

DB_RETVAL_E dbd_get_dev_log_by_dev_id(HC_DEVICE_INFO_S * dev_info, char* dev_ptr)
{
    return db_get_dev_log_by_dev_id(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.dev_id);
}

DB_RETVAL_E dbd_get_dev_log_by_dev_type(HC_DEVICE_INFO_S * dev_info, char* dev_ptr)
{
    return db_get_dev_log_by_dev_type(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.dev_type);
}

DB_RETVAL_E dbd_get_dev_log_by_network_type(HC_DEVICE_INFO_S * dev_info, char* dev_ptr)
{
    return db_get_dev_log_by_network_type(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.network_type);
}

DB_RETVAL_E dbd_get_dev_log_by_by_time(HC_DEVICE_INFO_S * dev_info, char* dev_ptr)
{
    return db_get_dev_log_by_time(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.start_time,  dev_info->device.ext_u.get_log.end_time);
}

DB_RETVAL_E dbd_get_dev_log_by_alarm_type(HC_DEVICE_INFO_S * dev_info, char* dev_ptr)
{
    return db_get_dev_log_by_alarm_type(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.alarm_type);
}

DB_RETVAL_E dbd_add_log_v2(HC_DEVICE_INFO_S *dev_info)
{   
    int ret = DB_RETVAL_OK;    

    ret = db_add_dev_log_v2(&(dev_info->device.ext_u.add_log));
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}


DB_RETVAL_E dbd_get_log_all_v2(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_LOG_S* dev_ptr)
{
    int ret;
    ret =  db_get_dev_log_all_v2(dev_ptr, dev_info->device.ext_u.get_log.log_num);
    DEBUG_INFO("[DB]dbd_get_log_all_v2 (%d)", ret);
    return ret;
}

DB_RETVAL_E dbd_get_dev_log_by_dev_id_v2(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_LOG_S* dev_ptr)
{
    return db_get_dev_log_by_dev_id_v2(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.dev_id);
}

DB_RETVAL_E dbd_get_dev_log_by_dev_type_v2(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_LOG_S* dev_ptr)
{
    return db_get_dev_log_by_dev_type_v2(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.dev_type);
}

DB_RETVAL_E dbd_get_dev_log_by_network_type_v2(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_LOG_S* dev_ptr)
{
    return db_get_dev_log_by_network_type_v2(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.network_type);
}

DB_RETVAL_E dbd_get_dev_log_by_by_time_v2(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_LOG_S* dev_ptr)
{
    return db_get_dev_log_by_time_v2(dev_ptr, dev_info->device.ext_u.get_log.log_num, dev_info->device.ext_u.get_log.start_time,  dev_info->device.ext_u.get_log.end_time);
}


DB_RETVAL_E dbd_get_dev_log_num(int * size)
{
    return db_get_dev_log_num(size);
}
DB_RETVAL_E dbd_get_dev_log_num_by_dev_id(unsigned int dev_id, int *size)
{
    return db_get_dev_log_num_by_dev_id(dev_id, size);
}
DB_RETVAL_E dbd_get_dev_log_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size)
{
    return db_get_dev_log_num_by_network_type(network_type, size);
}
DB_RETVAL_E dbd_get_dev_log_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *size)
{
    return db_get_dev_log_num_by_dev_type(dev_type, size);
}
DB_RETVAL_E dbd_get_dev_log_num_by_time(HC_DEVICE_INFO_S *devs_info, int * size)
{
    return db_get_dev_log_num_by_time( size, devs_info->device.ext_u.get_log.start_time, devs_info->device.ext_u.get_log.end_time);
}

DB_RETVAL_E dbd_get_dev_log_num_by_alarm_type(HC_DEVICE_INFO_S *devs_info, int * size)
{
    return db_get_dev_log_num_by_alarm_type(devs_info->device.ext_u.get_log.alarm_type, size);
}

DB_RETVAL_E dbd_del_dev_log_all()
{
    int ret = DB_RETVAL_OK;
    
    ret = db_del_dev_log_all();  
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_del_dev_log_by_dev_id(HC_DEVICE_INFO_S *dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_del_dev_log_by_dev_id(dev_info->device.ext_u.get_log.dev_id);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}


//Extended Device APIs
DB_RETVAL_E dbd_set_dev_ext(HC_DEVICE_INFO_S *dev_info)
{
    HC_DEVICE_EXT_INFO_S dev_ext;
    int ret = DB_RETVAL_OK;
    dev_ext.dev_id = dev_info->dev_id;
    dev_ext.dev_type = dev_info->dev_type;
    dev_ext.event_type = dev_info->event_type;
    dev_ext.network_type = dev_info->event_type;
    memcpy(&(dev_ext.device_ext), &(dev_info->device.ext_u), sizeof(HC_DEVICE_EXT_U));
    ret =  db_set_dev_ext(&dev_ext,1);
    
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_add_dev_ext(HC_DEVICE_INFO_S *dev_info)
{
    HC_DEVICE_EXT_INFO_S dev_ext;
    int ret = DB_RETVAL_OK;
    dev_ext.dev_id = dev_info->dev_id;
    dev_ext.dev_type = dev_info->dev_type;
    dev_ext.event_type = dev_info->event_type;
    dev_ext.network_type = dev_info->event_type;
    memcpy(&(dev_ext.device_ext), &(dev_info->device.ext_u), sizeof(HC_DEVICE_EXT_U));
    ret = db_add_dev_ext(&dev_ext,1);
    
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_get_dev_ext_by_dev_id(HC_DEVICE_INFO_S *devs_info, HC_DEVICE_INFO_S *dev_ptr)
{
    HC_DEVICE_EXT_INFO_S dev_ext;
    DB_RETVAL_E ret;
    memset(&dev_ext, 0, sizeof(HC_DEVICE_EXT_INFO_S));
    ret = db_get_dev_ext_by_dev_id(devs_info->dev_id,&dev_ext);
    dev_ptr->dev_id = dev_ext.dev_id;
    dev_ptr->dev_type = dev_ext.dev_type;
    dev_ptr->event_type = dev_ext.event_type;
    dev_ptr->network_type = dev_ext.network_type;
    memcpy(&(dev_ext.device_ext), &(devs_info->device.ext_u), sizeof(HC_DEVICE_EXT_U));
    memcpy(dev_ptr, &dev_ext, sizeof(HC_DEVICE_INFO_S));

    return  ret;
}

DB_RETVAL_E dbd_del_dev_ext_by_dev_id(HC_DEVICE_INFO_S *dev_info)
{
    int ret = DB_RETVAL_OK;
    ret =  db_del_dev_ext_by_dev_id(dev_info->dev_id);
    
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_get_dev_all(int dev_count, HC_DEVICE_INFO_S* dev_ptr)
{
    return db_get_dev_all(dev_ptr, dev_count);
}

DB_RETVAL_E dbd_get_dev_num_all(int *dev_num)
{
    return db_get_dev_num_all(dev_num);
}

DB_RETVAL_E dbd_get_dev_by_dev_type(HC_DEVICE_TYPE_E dev_type, int dev_count, HC_DEVICE_INFO_S* dev_ptr)
{
    return db_get_dev_by_dev_type(dev_type, dev_ptr, dev_count);
}

DB_RETVAL_E dbd_get_dev_num_by_dev_type(HC_DEVICE_TYPE_E dev_type, int * size)
{
    return db_get_dev_num_by_dev_type(dev_type, size);
}

DB_RETVAL_E dbd_get_dev_by_network_type(HC_NETWORK_TYPE_E network_type, int dev_count, HC_DEVICE_INFO_S* dev_ptr)
{
    return db_get_dev_by_network_type(network_type, dev_ptr, dev_count);
}

DB_RETVAL_E dbd_get_dev_num_by_network_type(HC_NETWORK_TYPE_E network_type, int *size)
{
    return db_get_dev_num_by_network_type(network_type, size);
}


DB_RETVAL_E dbd_add_attr(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_add_attr(dev_info->device.ext_u.attr.dev_id, dev_info->device.ext_u.attr.attr_name, dev_info->device.ext_u.attr.attr_value);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_get_attr(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr)
{
    int ret = DB_RETVAL_OK;
    dev_ptr->device.ext_u.attr.dev_id = dev_info->device.ext_u.attr.dev_id;
    strncpy(dev_ptr->device.ext_u.attr.attr_name, dev_info->device.ext_u.attr.attr_name, ATTR_NAME_LEN-1);
    dev_ptr->device.ext_u.attr.attr_name[ATTR_NAME_LEN-1]='\0';
    ret = db_get_attr(dev_info->device.ext_u.attr.dev_id, dev_info->device.ext_u.attr.attr_name, dev_ptr->device.ext_u.attr.attr_value);
    return ret;
}

DB_RETVAL_E dbd_get_attr_all_by_id(unsigned int dev_id, HC_DEVICE_EXT_ATTR_S* dev_ptr, int size)
{
    int ret = DB_RETVAL_OK;
    ret = db_get_attr_all_by_id(dev_id, size, dev_ptr);
    return ret;
}

DB_RETVAL_E dbd_get_attr_num_by_dev_id(unsigned int dev_id, int* size)
{
    int ret = DB_RETVAL_OK;
    ret = db_get_attr_num_by_dev_id(dev_id, size);
    return ret;
}

DB_RETVAL_E dbd_set_attr(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_set_attr(dev_info->device.ext_u.attr.dev_id, dev_info->device.ext_u.attr.attr_name, dev_info->device.ext_u.attr.attr_value);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_attr(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    
    ret = db_del_attr(dev_info->device.ext_u.attr.dev_id, dev_info->device.ext_u.attr.attr_name);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_attr_by_dev_id(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;

    ret = db_del_attr_by_dev_id(dev_info->device.ext_u.attr.dev_id);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_attr_all()
{
    int ret = DB_RETVAL_OK;

    ret = db_del_attr_all();
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_add_scene(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_add_scene(dev_info->device.ext_u.scene.scene_id, dev_info->device.ext_u.scene.scene_attr);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_get_scene(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr)
{
    int ret = DB_RETVAL_OK;
    strcpy(dev_ptr->device.ext_u.scene.scene_id, dev_info->device.ext_u.scene.scene_id);
    ret = db_get_scene(dev_info->device.ext_u.scene.scene_id, dev_ptr->device.ext_u.scene.scene_attr);
    return ret;
}

DB_RETVAL_E dbd_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S* scene_ptr)
{
    int ret = DB_RETVAL_OK;
    ret = db_get_scene_all(size, scene_ptr);
    return ret;
}

DB_RETVAL_E dbd_get_scene_num_all(int * size)
{
    int ret = DB_RETVAL_OK;
    ret = db_get_scene_num(size);
    return ret;
}

DB_RETVAL_E dbd_set_scene(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_update_scene(dev_info->device.ext_u.scene.scene_id, dev_info->device.ext_u.scene.scene_attr);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_scene(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    
    ret = db_del_scene_by_scene_id(dev_info->device.ext_u.scene.scene_id);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_scene_all()
{
    int ret = DB_RETVAL_OK;

    ret = db_del_scene_all();
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}
/*
DB_RETVAL_E dbd_add_user_log(HC_DEVICE_INFO_S *dev_info)
{   
    int ret = DB_RETVAL_OK;    

    ret = db_add_user_log(&(dev_info->device.ext_u.add_user_log));
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}
*/

DB_RETVAL_E dbd_get_user_log_all(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_EXT_USER_LOG_S* dev_ptr)
{
    return db_get_user_log_all(dev_ptr, dev_info->device.ext_u.get_user_log.log_num);
}

DB_RETVAL_E dbd_get_user_log_num_all(int * size)
{
    return db_get_user_log_num(size);
}

DB_RETVAL_E dbd_add_location(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_add_location(dev_info->device.ext_u.location.location_id, dev_info->device.ext_u.location.location_attr);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_get_location(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr)
{
    int ret = DB_RETVAL_OK;
    strcpy(dev_ptr->device.ext_u.location.location_id, dev_info->device.ext_u.location.location_id);
    ret = db_get_location(dev_info->device.ext_u.location.location_id, dev_ptr->device.ext_u.location.location_attr);
    return ret;
}

DB_RETVAL_E dbd_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S* location_ptr)
{
    int ret = DB_RETVAL_OK;
    ret = db_get_location_all(size, location_ptr);
    return ret;
}

DB_RETVAL_E dbd_get_location_num_all(int * size)
{
    int ret = DB_RETVAL_OK;
    ret = db_get_location_num(size);
    return ret;
}

DB_RETVAL_E dbd_set_location(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_update_location(dev_info->device.ext_u.location.location_id, dev_info->device.ext_u.location.location_attr);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_location(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    
    ret = db_del_location_by_location_id(dev_info->device.ext_u.location.location_id);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_del_location_all()
{
    int ret = DB_RETVAL_OK;

    ret = db_del_location_all();
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    return ret;
}

DB_RETVAL_E dbd_get_conf(HC_DEVICE_INFO_S * dev_info, HC_DEVICE_INFO_S* dev_ptr)
{
    int ret = DB_RETVAL_OK;
    strcpy(dev_ptr->device.ext_u.conf.conf_name, dev_info->device.ext_u.conf.conf_name);
    ret = db_get_conf(dev_info->device.ext_u.conf.conf_name, dev_ptr->device.ext_u.conf.conf_value);
    return ret;
}

DB_RETVAL_E dbd_set_conf(HC_DEVICE_INFO_S * dev_info)
{
    int ret = DB_RETVAL_OK;
    ret = db_set_conf(dev_info->device.ext_u.conf.conf_name, dev_info->device.ext_u.conf.conf_value);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
}

void db_combine_sql_stmt(sqlite3* db, char* sql_stmt, void * data, int type, unsigned int dev_id, char* name)
{
    int count = 0;
    char tmp_buf[SQL_BUF_LEN];
    int dev_num = 0;
    int rc;

    memset(tmp_buf, 0, SQL_BUF_LEN);
    sprintf(tmp_buf, SQL_CMD_SELECT_DEV_VALUE_NUM_BY_ID_AND_NAME, dev_id, name);
    //printf("tmp_buf is [%s]", tmp_buf);
    rc = sql_exec(1, db, tmp_buf, sql_cb_get_count, (void*)&dev_num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]SQL statement's excution error!!\n");
    }
    //printf("count is %d, type is %d\n", dev_num, type);

    if(dev_num == 1)
    {
        switch(type)
        {
            case DT_TYPE_INT:
                sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_VALUE_HEAD"%d"SQL_CMD_UPDATE_DEV_VALUE_TAIL, *((int*)data), dev_id, name);
            break;
            case DT_TYPE_DOUBLE:
                sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_VALUE_HEAD"%lf"SQL_CMD_UPDATE_DEV_VALUE_TAIL, *((double*)data), dev_id, name);
            break;
            case DT_TYPE_STRING:
                sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_VALUE_HEAD"%s"SQL_CMD_UPDATE_DEV_VALUE_TAIL, ((char*)data), dev_id, name);
            break;
            case DT_TYPE_UCHAR:
                sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_VALUE_HEAD"%c"SQL_CMD_UPDATE_DEV_VALUE_TAIL, *((unsigned char*)data), dev_id, name);
            break;
            case DT_TYPE_LONG:
                sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_VALUE_HEAD"%ld"SQL_CMD_UPDATE_DEV_VALUE_TAIL, *((long*)data), dev_id, name);
            break;
            case DT_TYPE_UINT:
                sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_VALUE_HEAD"%u"SQL_CMD_UPDATE_DEV_VALUE_TAIL, *((unsigned int*)data), dev_id, name);
            break;
        }
    }
    else
    {
        switch(type)
        {
            case DT_TYPE_INT:
                sprintf(sql_stmt, SQL_CMD_ADD_DEV_VALUE_HEAD"%d"SQL_CMD_ADD_DEV_VALUE_TAIL, dev_id, name, *((int*)data));
            break;
            case DT_TYPE_DOUBLE:
                sprintf(sql_stmt, SQL_CMD_ADD_DEV_VALUE_HEAD"%lf"SQL_CMD_ADD_DEV_VALUE_TAIL, dev_id, name, *((double*)data));
            break;
            case DT_TYPE_STRING:
                sprintf(sql_stmt, SQL_CMD_ADD_DEV_VALUE_HEAD"%s"SQL_CMD_ADD_DEV_VALUE_TAIL, dev_id, name, ((char*)data));
            break;
            case DT_TYPE_UCHAR:
                sprintf(sql_stmt, SQL_CMD_ADD_DEV_VALUE_HEAD"%c"SQL_CMD_ADD_DEV_VALUE_TAIL, dev_id, name, *((unsigned char*)data));
            break;
            case DT_TYPE_LONG:
                sprintf(sql_stmt, SQL_CMD_ADD_DEV_VALUE_HEAD"%ld"SQL_CMD_ADD_DEV_VALUE_TAIL, dev_id, name, *((long*)data));
            break;
            case DT_TYPE_UINT:
                sprintf(sql_stmt, SQL_CMD_ADD_DEV_VALUE_HEAD"%u"SQL_CMD_ADD_DEV_VALUE_TAIL, dev_id, name, *((unsigned int*)data));
            break;
        }
    }
    //printf("[db_combine_sql_stmt]sql_stmt is %s\n", sql_stmt);
}

DB_RETVAL_E sql_set_binary_switch_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_BINARYSWITCH_S *binaryswitch, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(binaryswitch->value), binary_switch_value_s[0].type, dev_id, binary_switch_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    db_combine_sql_stmt(db, sql_stmt, &(binaryswitch->type), binary_switch_value_s[1].type, dev_id, binary_switch_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_double_switch_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_DOUBLESWITCH_S *doubleswitch, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(doubleswitch->value), double_switch_value_s[0].type, dev_id, double_switch_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_dimmer_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_DIMMER_S *dimmer, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(dimmer->value), dimmer_value_s[0].type, dev_id, dimmer_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_curtain_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_CURTAIN_S *curtain, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(curtain->value), curtain_value_s[0].type, dev_id, curtain_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_binary_sensor_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_BINARY_SENSOR_S *binary_sensor, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 2;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    db_combine_sql_stmt(db, sql_stmt, &(binary_sensor->sensor_type), binary_sensor_value_s[0].type, dev_id, binary_sensor_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    db_combine_sql_stmt(db, sql_stmt, &(binary_sensor->value), binary_sensor_value_s[1].type, dev_id, binary_sensor_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_motion_sensor_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_MOTION_SENSOR_S *motion_sensor, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(motion_sensor->value), motion_sensor_value_s[0].type, dev_id, motion_sensor_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_window_sensor_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_WINDOW_SENSOR_S *window_sensor, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(window_sensor->value), window_sensor_value_s[0].type, dev_id, window_sensor_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_temperature_sensor_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_TEMPERATURE_SENSOR_S *temperature_sensor, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(temperature_sensor->value), temperature_sensor_value_s[0].type, dev_id, temperature_sensor_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_luminance_sensor_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_LUMINANCE_SENSOR_S *luminance_sensor, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(luminance_sensor->value), luminance_sensor_value_s[0].type, dev_id, luminance_sensor_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_battery_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_BATTERY_S *battery, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 2;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(battery->battery_level), battery_value_s[0].type, dev_id, battery_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(battery->interval_time), battery_value_s[1].type, dev_id, battery_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_door_lock_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_DOORLOCK_S *doorlock, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 5;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock->doorLockMode), doorlock_value_s[0].type, dev_id, doorlock_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock->doorHandlesMode), doorlock_value_s[1].type, dev_id, doorlock_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock->doorCondition), doorlock_value_s[2].type, dev_id, doorlock_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock->lockTimeoutMinutes), doorlock_value_s[3].type, dev_id, doorlock_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->value is (%lf)\n", multilevel_sensor->value);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock->lockTimeoutSeconds), doorlock_value_s[4].type, dev_id, doorlock_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_door_lock_config_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_DOORLOCK_CONFIG_S *doorlock_config, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 4;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock_config->operationType), doorlock_config_value_s[0].type, dev_id, doorlock_config_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock_config->doorHandlesMode), doorlock_config_value_s[1].type, dev_id, doorlock_config_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock_config->lockTimeoutMinutes), doorlock_config_value_s[2].type, dev_id, doorlock_config_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(doorlock_config->lockTimeoutSeconds), doorlock_config_value_s[3].type, dev_id, doorlock_config_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
        
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_hsm100_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_HSM100_CONFIG_S *hsm100_config, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 3;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(hsm100_config->parameterNumber), hsm100_config_value_s[0].type, dev_id, hsm100_config_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(hsm100_config->bDefault), hsm100_config_value_s[1].type, dev_id, hsm100_config_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(hsm100_config->value), hsm100_config_value_s[2].type, dev_id, hsm100_config_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
            
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_thermostat_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_THERMOSTAT_S *thermostat, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 6;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(thermostat->mode), thermostat_value_s[0].type, dev_id, thermostat_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(thermostat->value), thermostat_value_s[1].type, dev_id, thermostat_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(thermostat->heat_value), thermostat_value_s[2].type, dev_id, thermostat_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(thermostat->cool_value), thermostat_value_s[3].type, dev_id, thermostat_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(thermostat->energe_save_heat_value), thermostat_value_s[4].type, dev_id, thermostat_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(thermostat->energe_save_cool_value), thermostat_value_s[5].type, dev_id, thermostat_value_s[5].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
            
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_meter_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_METER_S *meter, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 6;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(meter->meter_type), meter_value_s[0].type, dev_id, meter_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(meter->scale), meter_value_s[1].type, dev_id, meter_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(meter->rate_type), meter_value_s[2].type, dev_id, meter_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(meter->value), meter_value_s[3].type, dev_id, meter_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->value is (%lf)\n", multilevel_sensor->value);
    db_combine_sql_stmt(db, sql_stmt, &(meter->delta_time), meter_value_s[4].type, dev_id, meter_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->battery_level is (%d)\n", multilevel_sensor->battery_level);
    db_combine_sql_stmt(db, sql_stmt, &(meter->previous_value), meter_value_s[5].type, dev_id, meter_value_s[5].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_power_meter_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_POWER_METER_S *power_meter, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 6;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(power_meter->meter_type), power_meter_value_s[0].type, dev_id, power_meter_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(power_meter->scale), power_meter_value_s[1].type, dev_id, power_meter_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(power_meter->rate_type), power_meter_value_s[2].type, dev_id, power_meter_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(power_meter->value), power_meter_value_s[3].type, dev_id, power_meter_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->value is (%lf)\n", multilevel_sensor->value);
    db_combine_sql_stmt(db, sql_stmt, &(power_meter->delta_time), power_meter_value_s[4].type, dev_id, power_meter_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->battery_level is (%d)\n", multilevel_sensor->battery_level);
    db_combine_sql_stmt(db, sql_stmt, &(power_meter->previous_value), power_meter_value_s[5].type, dev_id, power_meter_value_s[5].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}


DB_RETVAL_E sql_set_water_meter_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_WATER_METER_S *water_meter, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 6;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(water_meter->meter_type), water_meter_value_s[0].type, dev_id, water_meter_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(water_meter->scale), water_meter_value_s[1].type, dev_id, water_meter_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(water_meter->rate_type), water_meter_value_s[2].type, dev_id, water_meter_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(water_meter->value), water_meter_value_s[3].type, dev_id, water_meter_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->value is (%lf)\n", multilevel_sensor->value);
    db_combine_sql_stmt(db, sql_stmt, &(water_meter->delta_time), water_meter_value_s[4].type, dev_id, water_meter_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->battery_level is (%d)\n", multilevel_sensor->battery_level);
    db_combine_sql_stmt(db, sql_stmt, &(water_meter->previous_value), water_meter_value_s[5].type, dev_id, water_meter_value_s[5].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_gas_meter_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_GAS_METER_S *gas_meter, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 6;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(gas_meter->meter_type), gas_meter_value_s[0].type, dev_id, gas_meter_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(gas_meter->scale), gas_meter_value_s[1].type, dev_id, gas_meter_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(gas_meter->rate_type), gas_meter_value_s[2].type, dev_id, gas_meter_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(gas_meter->value), gas_meter_value_s[3].type, dev_id, gas_meter_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->value is (%lf)\n", multilevel_sensor->value);
    db_combine_sql_stmt(db, sql_stmt, &(gas_meter->delta_time), gas_meter_value_s[4].type, dev_id, gas_meter_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->battery_level is (%d)\n", multilevel_sensor->battery_level);
    db_combine_sql_stmt(db, sql_stmt, &(gas_meter->previous_value), gas_meter_value_s[5].type, dev_id, gas_meter_value_s[5].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_associate_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_ASSOCIATION_S *association, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 2;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(association->srcPhyId), association_value_s[0].type, dev_id, association_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(association->dstPhyId), association_value_s[1].type, dev_id, association_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_handset_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_HANDSET_U *handset, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 2;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(handset->status), handset_value_s[0].type, dev_id, handset_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(handset->u.value), handset_value_s[1].type, dev_id, handset_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_camera_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_CAMERA_S *camera, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 13;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->sensor_type is (%d)\n", multilevel_sensor->sensor_type);
    db_combine_sql_stmt(db, sql_stmt, &(camera->connection), camera_value_s[0].type, dev_id, camera_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->precision is (%d)\n", multilevel_sensor->precision);
    db_combine_sql_stmt(db, sql_stmt, &(camera->ipaddress), camera_value_s[1].type, dev_id, camera_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    //printf("multilevel_sensor->scale is (%d)\n", multilevel_sensor->scale);
    db_combine_sql_stmt(db, sql_stmt, &(camera->onvifuid), camera_value_s[2].type, dev_id, camera_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->size is (%d)\n", multilevel_sensor->size);
    db_combine_sql_stmt(db, sql_stmt, &(camera->streamurl), camera_value_s[3].type, dev_id, camera_value_s[3].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->value is (%lf)\n", multilevel_sensor->value);
    db_combine_sql_stmt(db, sql_stmt, &(camera->remoteurl), camera_value_s[4].type, dev_id, camera_value_s[4].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->battery_level is (%d)\n", multilevel_sensor->battery_level);
    db_combine_sql_stmt(db, sql_stmt, &(camera->destPath), camera_value_s[5].type, dev_id, camera_value_s[5].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->triggerTime), camera_value_s[6].type, dev_id, camera_value_s[6].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->label), camera_value_s[7].type, dev_id, camera_value_s[7].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->name), camera_value_s[8].type, dev_id, camera_value_s[8].name);
    strcat(*sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->mac), camera_value_s[9].type, dev_id, camera_value_s[9].name);
    strcat(*sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->ext), camera_value_s[10].type, dev_id, camera_value_s[10].name);
    strcat(*sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->fwversion), camera_value_s[11].type, dev_id, camera_value_s[11].name);
    strcat(*sql_cmd, sql_stmt);

    memset(sql_stmt, 0, SQL_BUF_LEN);
    //printf("multilevel_sensor->endpoints is (%d)\n", multilevel_sensor->endpoints);
    db_combine_sql_stmt(db, sql_stmt, &(camera->modelname), camera_value_s[12].type, dev_id, camera_value_s[12].name);
    strcat(*sql_cmd, sql_stmt);
    
    DEBUG_INFO("[DB]sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_keyfob_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_KEYFOB_S *keyfob, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);

    db_combine_sql_stmt(db, sql_stmt, &(keyfob->item_word), keyfob_value_s[0].type, dev_id, keyfob_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_siren_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_SIREN_S *siren, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 1;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);

    db_combine_sql_stmt(db, sql_stmt, &(siren->warning_mode), siren_value_s[0].type, dev_id, siren_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);

    db_combine_sql_stmt(db, sql_stmt, &(siren->strobe), siren_value_s[1].type, dev_id, siren_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_set_multi_sensor_value_cmd(sqlite3* db, unsigned int dev_id, HC_DEVICE_MULTILEVEL_SENSOR_S *multilevel_sensor, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 7;//sizeof(multi_sensor_value_s);

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    db_combine_sql_stmt(db, sql_stmt, &(multilevel_sensor->sensor_type), multi_sensor_value_s[0].type, dev_id, multi_sensor_value_s[0].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);    
    db_combine_sql_stmt(db, sql_stmt, &(multilevel_sensor->scale), multi_sensor_value_s[1].type, dev_id, multi_sensor_value_s[1].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    db_combine_sql_stmt(db, sql_stmt, &(multilevel_sensor->value), multi_sensor_value_s[2].type, dev_id, multi_sensor_value_s[2].name);
    strcat(*sql_cmd, sql_stmt);
    //printf("sql_cmd is %s\n, sql_stmt is %s\n",sql_cmd, sql_stmt);
    
    DEBUG_INFO("[DB]sql_cmd is %s\n, sql_stmt is %s\n",*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_set_device_value(sqlite3* db, unsigned int dev_id, HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U * dev_ptr, char** sql_cmd)
{
    int rc = DB_RETVAL_OK;
    switch(dev_type)
    {
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            rc = sql_set_multi_sensor_value_cmd(db, dev_id, (HC_DEVICE_MULTILEVEL_SENSOR_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_BINARYSWITCH:
            rc = sql_set_binary_switch_value_cmd(db, dev_id, (HC_DEVICE_BINARY_SENSOR_S *)dev_ptr, sql_cmd);
            break;
        case  HC_DEVICE_TYPE_DOUBLESWITCH:
            rc = sql_set_double_switch_value_cmd(db, dev_id, (HC_DEVICE_DOUBLESWITCH_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_DIMMER:
            rc = sql_set_dimmer_value_cmd(db, dev_id, (HC_DEVICE_DIMMER_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            rc = sql_set_curtain_value_cmd(db, dev_id, (HC_DEVICE_CURTAIN_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_BINARY_SENSOR:
            rc = sql_set_binary_sensor_value_cmd(db, dev_id, (HC_DEVICE_BINARY_SENSOR_S *)dev_ptr, sql_cmd);
            break;

        case HC_DEVICE_TYPE_BATTERY:
            rc = sql_set_battery_value_cmd(db, dev_id, (HC_DEVICE_BATTERY_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_DOORLOCK:
            rc = sql_set_door_lock_value_cmd(db, dev_id, (HC_DEVICE_DOORLOCK_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_DOORLOCK_CONFIG:
            rc = sql_set_door_lock_config_value_cmd(db, dev_id, (HC_DEVICE_DOORLOCK_CONFIG_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_HSM100_CONFIG:
            rc = sql_set_hsm100_value_cmd(db, dev_id, (HC_DEVICE_HSM100_CONFIG_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_THERMOSTAT:
            rc = sql_set_thermostat_value_cmd(db, dev_id, (HC_DEVICE_THERMOSTAT_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_METER:
            rc = sql_set_meter_value_cmd(db, dev_id, (HC_DEVICE_METER_S *)dev_ptr, sql_cmd);
            break;

        case HC_DEVICE_TYPE_ASSOCIATION:
            rc = sql_set_associate_value_cmd(db, dev_id, (HC_DEVICE_ASSOCIATION_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_HANDSET:
            rc = sql_set_handset_value_cmd(db, dev_id, (HC_DEVICE_HANDSET_U *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_IPCAMERA:
            rc = sql_set_camera_value_cmd(db, dev_id, (HC_DEVICE_CAMERA_S *)dev_ptr, sql_cmd);
            break;    
        case HC_DEVICE_TYPE_KEYFOB:
            rc = sql_set_keyfob_value_cmd(db, dev_id, (HC_DEVICE_KEYFOB_S *)dev_ptr, sql_cmd);
            break;
        case HC_DEVICE_TYPE_SIREN:
            rc = sql_set_siren_value_cmd(db, dev_id, (HC_DEVICE_SIREN_S *)dev_ptr, sql_cmd);
            break;
        default:
            rc = DB_PARAM_ERROR;
            break;
    }
    return rc;
}

void db_get_value(int type, void * data, char* value)
{
    switch(type)
    {
        case DT_TYPE_INT:
            *((int*)data) = atoi(value);
        break;
        case DT_TYPE_DOUBLE:
            *((double*)data) = atof(value);
        break;
        case DT_TYPE_STRING:
            strncpy(data, value, DEV_VALUE_LEN-1);
            ((char*)data)[DEV_VALUE_LEN-1] = '\0';
        break;
        case DT_TYPE_UCHAR:
            *((unsigned char*)data) = *((unsigned char*)value);
        break;
        case DT_TYPE_LONG:
            *((long*)data) = atol(value);
        break;
        case DT_TYPE_UINT:
            string_to_unsigned_num(value, (unsigned int *)data);
        break;
    }
}

DB_RETVAL_E db_get_device_value_str(DEV_VALUE_S *dev_value_ptr, int size, char* name, char* value)
{
    int i = 0;

    for(i=0; i< size; i++)
    {
        if(0==strcmp(dev_value_ptr[i].name, name))
        {
            strncpy(value, dev_value_ptr[i].value, DEV_VALUE_LEN-1);
            value[DEV_VALUE_LEN-1] = '\0';
            //printf("***[%s][%s][%s][%s]****\n", dev_value_ptr[i].name,name,dev_value_ptr[i].value, value);
            return DB_RETVAL_OK;
        }
    }
    
    strcpy(value, "");
    return DB_NO_RECORD;
}

DB_RETVAL_E sql_get_multi_sensor_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_MULTILEVEL_SENSOR_S *multilevel_sensor)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, multi_sensor_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(multi_sensor_value_s[0].type, &(multilevel_sensor->sensor_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, multi_sensor_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(multi_sensor_value_s[1].type, &(multilevel_sensor->scale), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, multi_sensor_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        multilevel_sensor->value = atof(value);
        //db_get_value(multi_sensor_value_s[4].type, &(multilevel_sensor->value), value);

    return rc;
}


DB_RETVAL_E sql_get_binaryswitch_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_BINARYSWITCH_S *binary_switch)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, binary_switch_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(binary_switch_value_s[0].type, &(binary_switch->value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, binary_switch_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(binary_switch_value_s[1].type, &(binary_switch->type), value);
    return rc;
}

DB_RETVAL_E sql_get_doubleswitch_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_DOUBLESWITCH_S *double_switch)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, double_switch_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(double_switch_value_s[0].type, &(double_switch->value), value);
    return rc;
}

DB_RETVAL_E sql_get_dimmer_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_DIMMER_S *dimmer)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, dimmer_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(dimmer_value_s[0].type, &(dimmer->value), value);
    return rc;
}

DB_RETVAL_E sql_get_curtain_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_CURTAIN_S *curtain)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, curtain_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(curtain_value_s[0].type, &(curtain->value), value);
    return rc;
}

DB_RETVAL_E sql_get_binary_sensor_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_BINARY_SENSOR_S *binary_sensor)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, binary_sensor_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(binary_sensor_value_s[0].type, &(binary_sensor->sensor_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, binary_sensor_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(binary_sensor_value_s[1].type, &(binary_sensor->value), value);
    return rc;
}

DB_RETVAL_E sql_get_motion_sensor_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_MOTION_SENSOR_S *motion_sensor)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, motion_sensor_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(motion_sensor_value_s[0].type, &(motion_sensor->value), value);
    return rc;
}

DB_RETVAL_E sql_get_window_sensor_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_WINDOW_SENSOR_S *window_sensor)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, window_sensor_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        window_sensor->value = atof(value);
        //db_get_value(window_sensor_value_s[0].type, &(window_sensor->value), value);
    return rc;
}

DB_RETVAL_E sql_get_temperature_sensor_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_TEMPERATURE_SENSOR_S *temperature_sensor)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, temperature_sensor_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        temperature_sensor->value = atof(value);
        //db_get_value(temperature_sensor_value_s[0].type, &(temperature_sensor->value), value);
    return rc;
}

DB_RETVAL_E sql_get_luminance_sensor_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_LUMINANCE_SENSOR_S *luminance_sensor)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, luminance_sensor_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        luminance_sensor->value = atof(value);
        //db_get_value(luminance_sensor_value_s[0].type, &(luminance_sensor->value), value);
    return rc;
}

DB_RETVAL_E sql_get_battery_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_BATTERY_S *battery)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, battery_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(battery_value_s[0].type, &(battery->battery_level), value);
        
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, battery_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(battery_value_s[1].type, &(battery->interval_time), value);
    return rc;
}

DB_RETVAL_E sql_get_doorlock_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_DOORLOCK_S *doorlock)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_value_s[0].type, &(doorlock->doorLockMode), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_value_s[1].type, &(doorlock->doorHandlesMode), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_value_s[2].type, &(doorlock->doorCondition), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_value_s[3].type, &(doorlock->lockTimeoutMinutes), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_value_s[4].type, &(doorlock->lockTimeoutSeconds), value);

    return rc;
}

DB_RETVAL_E sql_get_doorlock_config_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_DOORLOCK_CONFIG_S *doorlock_config)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_config_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_config_value_s[0].type, &(doorlock_config->operationType), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_config_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_config_value_s[1].type, &(doorlock_config->doorHandlesMode), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_config_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_config_value_s[2].type, &(doorlock_config->lockTimeoutMinutes), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, doorlock_config_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(doorlock_config_value_s[3].type, &(doorlock_config->lockTimeoutSeconds), value);

    return rc;
}

DB_RETVAL_E sql_get_hsm100_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_HSM100_CONFIG_S *hsm100_config)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, hsm100_config_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(hsm100_config_value_s[0].type, &(hsm100_config->parameterNumber), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, hsm100_config_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(hsm100_config_value_s[1].type, &(hsm100_config->bDefault), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, hsm100_config_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(hsm100_config_value_s[2].type, &(hsm100_config->value), value);

    return rc;
}

DB_RETVAL_E sql_get_thermostat_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_THERMOSTAT_S *thermostat)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, thermostat_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(thermostat_value_s[0].type, &(thermostat->mode), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, thermostat_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        thermostat->value = atof(value);
        //db_get_value(thermostat_value_s[1].type, &(thermostat->value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, thermostat_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        thermostat->heat_value = atof(value);
        //db_get_value(thermostat_value_s[2].type, &(thermostat->heat_value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, thermostat_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        thermostat->cool_value = atof(value);
        //db_get_value(thermostat_value_s[3].type, &(thermostat->cool_value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, thermostat_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        thermostat->energe_save_heat_value = atof(value);
        //db_get_value(thermostat_value_s[4].type, &(thermostat->energe_save_heat_value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, thermostat_value_s[5].name, value);
    if(rc == DB_RETVAL_OK)
        thermostat->energe_save_cool_value = atof(value);
        //db_get_value(thermostat_value_s[5].type, &(thermostat->energe_save_cool_value), value);
    
    return rc;
}

DB_RETVAL_E sql_get_meter_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_METER_S *meter)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, meter_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(meter_value_s[0].type, &(meter->meter_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, meter_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(meter_value_s[1].type, &(meter->scale), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, meter_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(meter_value_s[2].type, &(meter->rate_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, meter_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        meter->value = atof(value);
        //db_get_value(meter_value_s[3].type, &(meter->value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, meter_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(meter_value_s[4].type, &(meter->delta_time), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, meter_value_s[5].name, value);
    if(rc == DB_RETVAL_OK)
        meter->previous_value = atof(value);
        //db_get_value(meter_value_s[5].type, &(meter->previous_value), value);
    return rc;
}

DB_RETVAL_E sql_get_power_meter_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_POWER_METER_S *power_meter)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, power_meter_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(power_meter_value_s[0].type, &(power_meter->meter_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, power_meter_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(power_meter_value_s[1].type, &(power_meter->scale), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, power_meter_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(power_meter_value_s[2].type, &(power_meter->rate_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, power_meter_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        power_meter->value = atof(value);
        //db_get_value(power_meter_value_s[3].type, &(power_meter->value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, power_meter_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(power_meter_value_s[4].type, &(power_meter->delta_time), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, power_meter_value_s[5].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(power_meter_value_s[5].type, &(power_meter->previous_value), value);
    return rc;
}

DB_RETVAL_E sql_get_gas_meter_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_GAS_METER_S *gas_meter)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, gas_meter_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(gas_meter_value_s[0].type, &(gas_meter->meter_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, gas_meter_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(gas_meter_value_s[1].type, &(gas_meter->scale), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, gas_meter_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(gas_meter_value_s[2].type, &(gas_meter->rate_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, gas_meter_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        gas_meter->value = atof(value);
        //db_get_value(gas_meter_value_s[3].type, &(gas_meter->value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, gas_meter_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(gas_meter_value_s[4].type, &(gas_meter->delta_time), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, gas_meter_value_s[5].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(gas_meter_value_s[5].type, &(gas_meter->previous_value), value);
    return rc;
}

DB_RETVAL_E sql_get_water_meter_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_WATER_METER_S *water_meter)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, water_meter_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(water_meter_value_s[0].type, &(water_meter->meter_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, water_meter_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(water_meter_value_s[1].type, &(water_meter->scale), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, water_meter_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(water_meter_value_s[2].type, &(water_meter->rate_type), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, water_meter_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        water_meter->value = atof(value);
        //db_get_value(water_meter_value_s[3].type, &(water_meter->value), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, water_meter_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(water_meter_value_s[4].type, &(water_meter->delta_time), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, water_meter_value_s[5].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(water_meter_value_s[5].type, &(water_meter->previous_value), value);
    return rc;
}

DB_RETVAL_E sql_get_association_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_ASSOCIATION_S *association)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, association_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(association_value_s[0].type, &(association->srcPhyId), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, association_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(association_value_s[1].type, &(association->dstPhyId), value);
    
    return rc;
}

DB_RETVAL_E sql_get_handset_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_HANDSET_U *handset)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, handset_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(handset_value_s[0].type, &(handset->status), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, handset_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(handset_value_s[1].type, &(handset->u.value), value);
    
    return rc;
}

DB_RETVAL_E sql_get_camera_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_CAMERA_S *camera)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[0].type, &(camera->connection), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[1].type, (camera->ipaddress), value);
    
    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[2].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[2].type, (camera->onvifuid), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[3].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[3].type, (camera->streamurl), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[4].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[4].type, (camera->remoteurl), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[5].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[5].type, (camera->destPath), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[6].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[6].type, (camera->triggerTime), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[7].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[7].type, (camera->label), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[8].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[8].type, (camera->name), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[9].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[9].type, (camera->mac), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[10].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[10].type, (camera->ext), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[11].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[11].type, (camera->fwversion), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, camera_value_s[12].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(camera_value_s[12].type, (camera->modelname), value);
    
    return rc;
}

DB_RETVAL_E sql_get_keyfob_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_KEYFOB_S *keyfob)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, keyfob_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(keyfob_value_s[0].type, &(keyfob->item_word), value);

    return rc;
}

DB_RETVAL_E sql_get_siren_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_SIREN_S *siren)
{
    char value[DEV_VALUE_LEN];
    int rc = DB_RETVAL_OK;

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, siren_value_s[0].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(siren_value_s[0].type, &(siren->warning_mode), value);

    memset(value, 0, DEV_VALUE_LEN);
    rc = db_get_device_value_str(dev_value_ptr, size, siren_value_s[1].name, value);
    if(rc == DB_RETVAL_OK)
        db_get_value(siren_value_s[1].type, &(siren->strobe), value);

    return rc;
}

DB_RETVAL_E db_get_device_value(DEV_VALUE_S *dev_value_ptr, int size, HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U * dev_ptr)
{
    int rc = DB_RETVAL_OK;
    switch(dev_type)
    {
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            rc = sql_get_multi_sensor_value(dev_value_ptr, size, (HC_DEVICE_MULTILEVEL_SENSOR_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_BINARYSWITCH:
            rc = sql_get_binaryswitch_value(dev_value_ptr, size, (HC_DEVICE_BINARYSWITCH_S *)dev_ptr);
            break;
        case  HC_DEVICE_TYPE_DOUBLESWITCH:
            rc = sql_get_doubleswitch_value(dev_value_ptr, size, (HC_DEVICE_DOUBLESWITCH_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_DIMMER:
            rc = sql_get_dimmer_value(dev_value_ptr, size, (HC_DEVICE_DIMMER_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            rc = sql_get_curtain_value(dev_value_ptr, size, (HC_DEVICE_CURTAIN_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_BINARY_SENSOR:
             rc = sql_get_binary_sensor_value(dev_value_ptr, size, (HC_DEVICE_BINARY_SENSOR_S *)dev_ptr);
           break;
      
        case HC_DEVICE_TYPE_BATTERY:
             rc = sql_get_battery_value(dev_value_ptr, size, (HC_DEVICE_BATTERY_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_DOORLOCK:
             rc = sql_get_doorlock_value(dev_value_ptr, size, (HC_DEVICE_DOORLOCK_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_DOORLOCK_CONFIG:
             rc = sql_get_doorlock_config_value(dev_value_ptr, size, (HC_DEVICE_DOORLOCK_CONFIG_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_HSM100_CONFIG:
             rc = sql_get_hsm100_value(dev_value_ptr, size, (HC_DEVICE_HSM100_CONFIG_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_THERMOSTAT:
             rc = sql_get_thermostat_value(dev_value_ptr, size, (HC_DEVICE_THERMOSTAT_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_METER:
             rc = sql_get_meter_value(dev_value_ptr, size, (HC_DEVICE_METER_S *)dev_ptr);
            break;
     
        case HC_DEVICE_TYPE_ASSOCIATION:
             rc = sql_get_association_value(dev_value_ptr, size, (HC_DEVICE_ASSOCIATION_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_HANDSET:
              rc = sql_get_handset_value(dev_value_ptr, size, (HC_DEVICE_HANDSET_U *)dev_ptr);
           break;
        case HC_DEVICE_TYPE_IPCAMERA:
             rc = sql_get_camera_value(dev_value_ptr, size, (HC_DEVICE_CAMERA_S *)dev_ptr);
            break;    
        case HC_DEVICE_TYPE_KEYFOB:
            rc = sql_get_keyfob_value(dev_value_ptr, size, (HC_DEVICE_KEYFOB_S *)dev_ptr);
            break;
        case HC_DEVICE_TYPE_SIREN:
            rc = sql_get_siren_value(dev_value_ptr, size, (HC_DEVICE_SIREN_S *)dev_ptr);
            break;
        default:
            rc = DB_PARAM_ERROR;
            break;
    }
    return rc;
}

DB_RETVAL_E dbd_update_connection(HC_DEVICE_INFO_S* dev_info)
{
    int ret = DB_RETVAL_OK;

    if(DB_WRONG_DEV_ID == db_check_valid_dev_id(dev_info->device.ext_u.dev_name.dev_id))
        return DB_WRONG_DEV_ID;

    ret = db_update_connection_by_dev_id(dev_info->device.ext_u.attr.dev_id, dev_info->device.ext_u.attr.attr_value);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E sql_update_connection_by_dev_id(unsigned int dev_id, char* connection)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    int count = 0;
    sqlite3_stmt * stat = NULL;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID=%u", dev_id);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
    }

    if (count != 0) {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_UPDATE_CONNECTION_BY_DEV_ID, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, connection, strlen(connection), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]update connection fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    } else {
        DEBUG_ERROR("[DB]update connection fail, device not exist");
        sql_close_db(db);
        return DB_NO_DEVICE;
    }
    
    sqlite3_finalize(stat);
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_update_connection_by_dev_id(unsigned int dev_id, char* connection)
{
    return sql_update_connection_by_dev_id(dev_id, connection);
}

DB_RETVAL_E sql_update_devname_by_dev_id(unsigned int dev_id, char* devname)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    int count = 0;
    sqlite3_stmt * stat = NULL;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID=%u", dev_id);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
    }

    if (count != 0) {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_UPDATE_DEV_NAME_BY_DEV_ID, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, devname, strlen(devname), NULL);
        rc = sqlite3_step(stat);
        while ((nRetryCnt < 10 ) &&
               (rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_INFO("[DB]Retrying SQL Statement [%d][%s]. rc=[%d].", nRetryCnt, sql_stmt, rc);
            nRetryCnt++;
            usleep(100000);
            rc = sqlite3_step(stat);
        }

        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]update connection fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    } else {
        DEBUG_ERROR("[DB]update connection fail, device not exist");
        sql_close_db(db);
        return DB_NO_DEVICE;
    }
    
    sqlite3_finalize(stat);
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_update_devname_by_dev_id(unsigned int dev_id, char* devname)
{
    int ret = DB_RETVAL_OK;

    if(DB_WRONG_DEV_ID == db_check_valid_dev_id(dev_id))
        return DB_WRONG_DEV_ID;

    ret = sql_update_devname_by_dev_id(dev_id, devname);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E sql_update_location_by_dev_id(unsigned int dev_id, char* location)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    int count = 0;
    sqlite3_stmt * stat = NULL;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEVICE_NUM"WHERE DEV_ID=%u", dev_id);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&count);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]SQL statement's excution error !!\n");
    }

    if (count != 0) {
        memset(sql_stmt, 0, SQL_BUF_LEN);

        sprintf(sql_stmt, SQL_CMD_UPDATE_LOCATION_BY_DEV_ID, dev_id);
        rc = sqlite3_prepare_v2(db, sql_stmt, -1, &stat, 0);
        rc = sqlite3_bind_text(stat, 1, location, strlen(location), NULL);
        rc = sqlite3_step(stat);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            sqlite3_finalize(stat);
            DEBUG_ERROR("[DB]update connection fail(%d), SQL error: %s\n", rc, sqlite3_errmsg(db));
            sql_close_db(db);
            return DB_ADD_NAME_FAIL;
        }
    } else {
        DEBUG_ERROR("[DB]update connection fail, device not exist");
        sql_close_db(db);
        return DB_NO_DEVICE;
    }
    
    sqlite3_finalize(stat);
    sql_close_db(db);

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_update_location_by_dev_id(unsigned int dev_id, char* location)
{
    int ret = DB_RETVAL_OK;

    if(DB_WRONG_DEV_ID == db_check_valid_dev_id(dev_id))
        return DB_WRONG_DEV_ID;

    ret = sql_update_location_by_dev_id(dev_id, location);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E sql_get_dev_value_by_dev_id(sqlite3* db, unsigned int dev_id, int size, DEV_VALUE_S* dev_value_ptr)
{
    int rc = DB_RETVAL_OK;
    sqlite3_stmt * stat = NULL;
    char sql_stmt[SQL_BUF_LEN];
    const unsigned char * value_ptr;
    const unsigned char * name_ptr;
    int i = 0;
    
    memset(sql_stmt, 0, SQL_BUF_LEN);
    
    sprintf(sql_stmt, SQL_CMD_GET_DEV_VALUE_ALL_BY_DEV_ID, dev_id);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_ERROR("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        dev_value_ptr[i].dev_id = dev_id;
        name_ptr = sqlite3_column_text(stat, 0);
        strncpy(dev_value_ptr[i].name, (char*)name_ptr, DEV_VALUE_NAME_LEN-1);
        dev_value_ptr[i].name[DEV_VALUE_NAME_LEN-1] = '\0';
        value_ptr = sqlite3_column_text(stat, 1);
        strncpy(dev_value_ptr[i].value, (char*)value_ptr, DEV_VALUE_LEN-1);
        dev_value_ptr[i].value[DEV_VALUE_LEN-1] = '\0';
        DEBUG_INFO("[DB][0x%x]%s=%s\n", dev_id, dev_value_ptr[i].name, dev_value_ptr[i].value);
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else 
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }
        
        if(i >= size)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }

    sqlite3_finalize(stat);

    //sql_close_db(db);

    return DB_RETVAL_OK;
}


DB_RETVAL_E sql_get_dev_value(sqlite3 *db, unsigned int dev_id, HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U * dev_ptr)
{
    char sql_stmt[SQL_BUF_LEN];
    int value_num = 0;
    DEV_VALUE_S* dev_value_ptr;
    int rc = DB_RETVAL_OK;
    int i;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_SELECT_DEV_VALUE_NUM_BY_ID, dev_id);
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&value_num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("SQL statement's excution error !!\n");
        return DB_GET_NUM_FAIL;
    }
    
    if(value_num == 0)
    {
        return DB_NO_DEV_VALUE;
    }
    
    dev_value_ptr = (DEV_VALUE_S*)malloc(value_num * sizeof(DEV_VALUE_S));
    if(dev_value_ptr == NULL)
    {
        return DB_MALLOC_ERROR;
    }

    memset(dev_value_ptr, 0, value_num * sizeof(DEV_VALUE_S));
    rc = sql_get_dev_value_by_dev_id(db, dev_id, value_num, dev_value_ptr);
    if(rc != DB_RETVAL_OK)
    {
        free(dev_value_ptr);
        return rc;
    }

    rc = db_get_device_value(dev_value_ptr, value_num, dev_type, dev_ptr);
    if(rc != DB_RETVAL_OK)
    {
        free(dev_value_ptr);
        return rc;
    }
    
    free(dev_value_ptr);
   
    return rc;
}

DB_RETVAL_E sql_init_dev_value_table(sqlite3 *db)
{    
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "DEVICE_VALUE_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check DEVICE_VALUE_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if(size == 1)
    {
        DEBUG_INFO("[DB]DEVICE_VALUE_TABLE TABLE already exists. skip create\n");
        return DB_TABLE_EXIST;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CREATE_DEV_VALUE_TABLE);
    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Create DEVICE_VALUE_TABLE TABLE ERROR !!\n");
        return DB_EXEC_ERROR;
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_del_dev_value_by_dev_id(sqlite3*db, unsigned dev_id)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_DEV_VALUE_BY_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
        return DB_EXEC_ERROR;
    }
    return rc;
}

