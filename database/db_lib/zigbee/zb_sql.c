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
*   File   : sql_api.c
*   Abstract:
*   Date   : 12/13/2015
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
#include <zigbee_msg.h>

#define SQL_BUF_LEN 512
#define SQL_BUF_LEN_L 2048

typedef struct
{
    unsigned int attribute_id;
    unsigned int data_type;
    char value[128];
}ZB_ATTRIBUTE_S;

DB_RETVAL_E sql_init_zb_device_table(sqlite3 *db);
DB_RETVAL_E db_add_zb_dev(ZB_DEVICE_INFO_S *devs_info, int size);
DB_RETVAL_E db_get_zb_dev_by_dev_id(unsigned int dev_id, ZB_DEVICE_INFO_S *devs_info);
DB_RETVAL_E dbd_add_zb_dev(ZB_DEVICE_INFO_S *devs_info);
DB_RETVAL_E dbd_del_zb_dev_by_dev_id(unsigned int dev_id);
DB_RETVAL_E dbd_reset_zb_dev(void);
DB_RETVAL_E dbd_get_zb_dev_by_dev_id(ZB_DEVICE_INFO_S * dev_info, ZB_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E sql_add_zb_dev(ZB_DEVICE_INFO_S* devs_info, int size);
DB_RETVAL_E sql_del_zb_dev_by_dev_id(unsigned int dev_id);
DB_RETVAL_E sql_reset_zb_dev(void);
DB_RETVAL_E sql_add_zb_device_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_onOff_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_level_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_simpleMeter_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_illuminance_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_temperature_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_humidity_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
DB_RETVAL_E sql_add_zb_ias_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd);
void sql_add_zb_attribute(sqlite3* db, char* sql_stmt, void* data, int attr_id, int type, ZB_DEVICE_INFO_S dev);
DB_RETVAL_E sql_get_zb_dev(ZB_DEVICE_INFO_S *devs_info, int size, const char* sql_stmt);
DB_RETVAL_E sql_get_zb_dev_value(sqlite3 *db, unsigned int dev_id, unsigned int cluster_id, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_onOff_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_level_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_simpleMeter_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_illuminance_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_temperature_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_humidity_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
DB_RETVAL_E db_get_zb_ias_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster);
void db_get_zb_attr(unsigned int data_type, char* attr_val, void* dest);
int sql_cb_get_count(void *buf, int argc, char **argv, char **azColName);
extern int string_to_unsigned_num(char *str, OUT int *value);

/****************************CHECK TABLE EXISTS SQL STATEMENT******************/
#define SQL_CMD_CHECK_TABLE_EXIST "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';"

/****************************ZB_DEVICE_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_ZB_DEV_TABLE "CREATE TABLE ZB_DEVICE_TABLE " \
                                            "(ZB_DEV_ID            INT, " \
                                            " EUI64             INT, " \
                                            " NETWORK_STATUS    INT, " \
                                            " NODE_ID           INT, " \
                                            " ENDPOINT          INT, " \
                                            " DEVICE_TYPE       INT, " \
                                            " CLUSTER_ID        INT, " \
                                            " ATTRIBUTE_ID      INT, " \
                                            " DATA_TYPE         INT, " \
                                            " ATTRIBUTE_VALUE   CHAR(128), " \
                                            " PRIMARY KEY( ZB_DEV_ID, ATTRIBUTE_ID ), " \
                                            " FOREIGN KEY(ZB_DEV_ID) REFERENCES DEVICE_TABLE(DEV_RFID));"

#define SQL_CMD_GET_ZB_DEV_ID "SELECT DISTINCT ZB_DEV_ID FROM ZB_DEVICE_TABLE;"

#define SQL_CMD_GET_ZB_DEV_NUM "SELECT count(DISTINCT ZB_DEV_ID) FROM ZB_DEVICE_TABLE;"

#define SQL_CMD_GET_ZB_DEV_VALUE_ALL_BY_DEV_ID "SELECT * FROM ZB_DEVICE_TABLE WHERE ZB_DEV_ID=%u;"

#define SQL_CMD_ADD_ZB_DEV "INSERT INTO ZB_DEVICE_TABLE VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, \'%s\');"

#define SQL_CMD_DEL_ZB_DEV_BY_DEV_ID "DELETE FROM ZB_DEVICE_TABLE WHERE ZB_DEV_ID=%u;"

#define SQL_CMD_DEL_ZB_DEV_ALL "DELETE FROM ZB_DEVICE_TABLE;"

#define SQL_CMD_SELECT_ZB_DEV_VALUE_NUM_BY_DEV_ID "SELECT count(*) FROM ZB_DEVICE_TABLE WHERE ZB_DEV_ID=%u;"

#define SQL_CMD_GET_ZB_ATTRIBUTE_COUNT_BY_DEV_ID "SELECT count(*) FROM ZB_DEVICE_TABLE WHERE ZB_DEV_ID=%u;"

#define SQL_CMD_GET_ZB_ATTRIBUTE_VALUE_ALL_BY_DEV_ID "SELECT ATTRIBUTE_ID,DATA_TYPE,ATTRIBUTE_VALUE FROM ZB_DEVICE_TABLE WHERE ZB_DEV_ID=%u;"

#define SQL_CMD_DEL_ZB_DEV_VALUE_BY_ID "DELETE FROM ZB_DEVICE_VALUE_TABLE WHERE ZB_DEV_ID=%u;"

DB_RETVAL_E sql_init_zb_device_table(sqlite3 *db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    if (CAPI_SUCCESS == capi_get_product_has_zigbee() &&
            0 == access("/dev/zigbee", F_OK))
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "ZB_DEVICE_TABLE");
        rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Check ZB_DEVICE_TABLE Exist ERROR !!\n");
            return DB_EXEC_ERROR;
        }

        if(size == 1)
        {
            DEBUG_INFO("[DB]ZB_DEVICE_TABLE TABLE already exists. skip create\n");
            return DB_TABLE_EXIST;
        }

        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_CREATE_ZB_DEV_TABLE);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Create ZB_DEVICE_TABLE ERROR !!\n");
            return DB_EXEC_ERROR;
        }
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_zb_dev(ZB_DEVICE_INFO_S *devs_info, int size)
{
    return sql_add_zb_dev(devs_info, size);
}

DB_RETVAL_E db_get_zb_dev_by_dev_id(unsigned int dev_id, ZB_DEVICE_INFO_S *devs_info)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_ZB_DEV_VALUE_ALL_BY_DEV_ID, dev_id);
    return sql_get_zb_dev(devs_info, 1, sql_stmt);
}

DB_RETVAL_E dbd_add_zb_dev(ZB_DEVICE_INFO_S *devs_info)
{
    int ret = DB_RETVAL_OK;

    ret = db_add_zb_dev(devs_info, 1);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E dbd_del_zb_dev_by_dev_id(unsigned int dev_id)
{
    int ret = DB_RETVAL_OK;

    ret = sql_del_zb_dev_by_dev_id(dev_id);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_reset_zb_dev(void)
{
    int ret = DB_RETVAL_OK;

    ret = sql_reset_zb_dev();
    if (DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_get_zb_dev_by_dev_id(ZB_DEVICE_INFO_S * dev_info, ZB_DEVICE_INFO_S* dev_ptr)
{
    return db_get_zb_dev_by_dev_id(dev_info->extend_id, dev_ptr);
}

DB_RETVAL_E sql_add_zb_dev(ZB_DEVICE_INFO_S* devs_info, int size)
{
    int i;
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int dev_num = 0;
    sqlite3* db;
    char *sql_cmd;
    DB_RETVAL_E ret = DB_RETVAL_OK;

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
        ret = sql_add_zb_device_cmd(db, devs_info[i], &sql_cmd);
        if(ret != DB_RETVAL_OK)
        {
            DEBUG_ERROR("[DB]Combine sql cmd failed.\n");
            goto EXIT;
        }

        rc = sql_exec(1, db, sql_cmd, NULL, 0);
        free(sql_cmd);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
            DEBUG_ERROR("[DB]sql_cmd: %s !!\n", sql_cmd);
            ret = DB_ADD_DEV_FAIL;
            goto EXIT;
        }
    }

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_del_zb_dev_by_dev_id(unsigned int dev_id)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    int attr_num = 0;
    sqlite3* db;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    int nRetryCnt = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    /* 1.1 Get attribute count */
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_ZB_ATTRIBUTE_COUNT_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&attr_num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("SQL statement's excution error !!\n");
        ret = DB_GET_NUM_FAIL;
    }

    if(attr_num == 0)
    {
        ret = DB_NO_DEV_VALUE;
        goto EXIT;
    }

    /* 1.2 Delete dev by device id */
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_ZB_DEV_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!n");
        ret = DB_EXEC_ERROR;
        goto EXIT;
    }

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_reset_zb_dev(void)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    int attr_num = 0;
    sqlite3* db;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    int size = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "ZB_DEVICE_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check ZB_DEVICE_TABLE Exist ERROR !!\n");
        return DB_EXEC_ERROR;
    }

    if (size == 1)
    {
        /* 1.1 Delete from ZB_DEVICE_TABLE */
        rc = sql_exec(1, db, SQL_CMD_DEL_ZB_DEV_ALL, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]SQL exec error !!\n");
            ret = DB_EXEC_ERROR;
            goto EXIT;
        }
    }

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_add_zb_device_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    DB_RETVAL_E ret = DB_RETVAL_OK;

#if 0
    switch(dev.dev_id)
    {
        case MAINS_POWER_OUTLET:
            rc = sql_add_zb_onOff_cmd(db, dev, sql_cmd);
            break;
        case DIMMERABLE_LIGHT:
            if(dev.cluster_id == ZCL_SIMPLE_METERING_CLUSTER_ID)
            {

            }
            else
                rc = sql_add_zb_level_cmd(db, dev, sql_cmd);
            break;

        case ON_OFF_LIGHT_SWITCH:
            rc = sql_add_zb_onOff_cmd(db, dev, sql_cmd);
            break;
        case LIGHT_SENSOR:
            rc = sql_add_zb_level_cmd(db, dev, sql_cmd);
            break;
        case TEMPERATURE_SENSOR:
        case IAS_ZONE:
        case IAS_WARMING_DEVICE:
        default:
            return DB_ADD_DEV_FAIL;
    }
#endif
    switch(dev.cluster_id)
    {
        case ZCL_ON_OFF_CLUSTER_ID:
            ret = sql_add_zb_onOff_cmd(db, dev, sql_cmd);
            break;
        case ZCL_LEVEL_CONTROL_CLUSTER_ID:
            ret = sql_add_zb_level_cmd(db, dev, sql_cmd);
            break;
        case ZCL_SIMPLE_METERING_CLUSTER_ID:
            ret = sql_add_zb_simpleMeter_cmd(db, dev, sql_cmd);
            break;
        case ZCL_ILLUM_MEASUREMENT_CLUSTER_ID:
            ret = sql_add_zb_illuminance_cmd(db, dev, sql_cmd);
            break;
        case ZCL_TEMP_MEASUREMENT_CLUSTER_ID:
            ret = sql_add_zb_temperature_cmd(db, dev, sql_cmd);
            break;
        case ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID:
            ret = sql_add_zb_humidity_cmd(db, dev, sql_cmd);
            break;
        case ZCL_IAS_ZONE_CLUSTER_ID:
            ret = sql_add_zb_ias_cmd(db, dev, sql_cmd);
            break;
        default:
            ret = DB_ADD_DEV_FAIL;
    }

    return ret;
}

DB_RETVAL_E sql_add_zb_onOff_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 4;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
/*
    WORD cluster_id;
    BOOL on_off;
    DWORD sample_mfg_specfic;
    BOOL global_scene_control;
    DWORD on_time;
    DWORD off_wait_time;
*/
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.onOff_cluster.on_off, 0x0000, BOOLEAN, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.onOff_cluster.global_scene_control, 0x0400, BOOLEAN, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.onOff_cluster.on_time, 0x0401, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.onOff_cluster.off_wait_time, 0x0402, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zb_level_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 7;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));

    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.current_level, 0x0000, UINT_8BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.remain_time, 0x0001, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.transition_time, 0x0010, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.on_level, 0x0011, UINT_8BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.on_transition_time, 0x0012, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.off_transition_time, 0x0013, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.level_cluster.move_rate, 0x0014, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zb_simpleMeter_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 6;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
/*
    unsigned long long int current_sum_value;
    BYTE status;
    BYTE unit_of_measure;
    BYTE summation_format;
    BYTE meter_device_type;
    int instantaneous_demand;
*/
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.simpleMeter_cluster.current_sum_value, 0x0000, UINT_8BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.simpleMeter_cluster.status, 0x0200, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.simpleMeter_cluster.unit_of_measure, 0x0300, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.simpleMeter_cluster.summation_format, 0x0303, UINT_8BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.simpleMeter_cluster.meter_device_type, 0x0306, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.simpleMeter_cluster.instantaneous_demand, 0x0400, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zb_illuminance_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 5;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
/*
    WORD value;
    WORD min;
    WORD max;
    WORD tolerance;
    BYTE light_sensor_type;
*/
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.illuminance_cluster.value, 0x0000, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.illuminance_cluster.min, 0x0001, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.illuminance_cluster.max, 0x0002, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.illuminance_cluster.tolerance, 0x0003, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.illuminance_cluster.light_sensor_type, 0x0004, ENUM_8BIT, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zb_temperature_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 4;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
/*
    WORD value;
    WORD min;
    WORD max;
    WORD tolerance;
*/
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.temperature_cluster.value, 0x0000, INT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.temperature_cluster.min, 0x0001, INT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.temperature_cluster.max, 0x0002, INT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.temperature_cluster.tolerance, 0x0003, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zb_humidity_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 4;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
/*
    WORD value;
    WORD min;
    WORD max;
    WORD tolerance;
*/
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.humidity_cluster.value, 0x0000, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.humidity_cluster.min, 0x0001, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.humidity_cluster.max, 0x0002, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.humidity_cluster.tolerance, 0x0003, UINT_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zb_ias_cmd(sqlite3* db, ZB_DEVICE_INFO_S dev, char** sql_cmd)
{
    char sql_stmt[SQL_BUF_LEN];
    int size = 4;

    *sql_cmd = (char*)malloc(SQL_BUF_LEN * size);
    if(sql_cmd == NULL)
    {
        return DB_MALLOC_ERROR;
    }
    memset(*sql_cmd, 0, (SQL_BUF_LEN*size));
/*
    BYTE zone_state;
    WORD zone_type;
    WORD zone_status;
    double ias_cie_address;
*/
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.ias_cluster.zone_state, 0x0000, ENUM_8BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.ias_cluster.zone_type, 0x0001, ENUM_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.ias_cluster.zone_status, 0x0002, BITMAP_16BIT, dev);
    strcat(*sql_cmd, sql_stmt);
    sql_add_zb_attribute(db, sql_stmt, (void*) &dev.cluster.ias_cluster.ias_cie_address, 0x0010, IEEE_ADDRESS, dev);
    strcat(*sql_cmd, sql_stmt);

    return DB_RETVAL_OK;
}

void sql_add_zb_attribute(sqlite3* db, char* sql_stmt, void* data, int attr_id, int type, ZB_DEVICE_INFO_S dev)
{
    long long unsigned int euiBuffer;

    memcpy(&euiBuffer, dev.eui64, 8);
    switch(type & 0xF8)
    {
        case NO_DATA:
            return ;
        case DATA_8BIT:
        case BOOLEAN:
        case BITMAP_8BIT:
        case INT_8BIT:
        case ENUM_8BIT:
            sprintf(sql_stmt, "INSERT INTO ZB_DEVICE_TABLE VALUES (%u, %llu, %u, %u, %u, %u, %u, %u, %u, \'%d\');", dev.extend_id, euiBuffer, dev.status, dev.node_id, dev.endpoint, dev.dev_id, dev.cluster_id, attr_id, type, *((int*) data));
        case UINT_8BIT:
            sprintf(sql_stmt, "INSERT INTO ZB_DEVICE_TABLE VALUES (%u, %llu, %u, %u, %u, %u, %u, %u, %u, \'%u\');", dev.extend_id, euiBuffer, dev.status, dev.node_id, dev.endpoint, dev.dev_id, dev.cluster_id, attr_id, type, *((int*) data));
            break;
        case SEMI_FLOAT:
            if(type == DOUBLE_FLOAT) {
                sprintf(sql_stmt, "INSERT INTO ZB_DEVICE_TABLE VALUES (%u, %llu, %u, %u, %u, %u, %u, %u, %u, \'%lf\');", dev.extend_id, euiBuffer, dev.status, dev.node_id, dev.endpoint, dev.dev_id, dev.cluster_id, attr_id, type, *((double*) data));
            } else {
                sprintf(sql_stmt, "INSERT INTO ZB_DEVICE_TABLE VALUES (%u, %llu, %u, %u, %u, %u, %u, %u, %u, \'%f\');", dev.extend_id, euiBuffer, dev.status, dev.node_id, dev.endpoint, dev.dev_id, dev.cluster_id, attr_id, type, *((float*) data));
            }            
            break;
        case IEEE_ADDRESS:
            sprintf(sql_stmt, "INSERT INTO ZB_DEVICE_TABLE VALUES (%u, %llu, %u, %u, %u, %u, %u, %u, %u, \'%llu\');", dev.extend_id, euiBuffer, dev.status, dev.node_id, dev.endpoint, dev.dev_id, dev.cluster_id, attr_id, type, *((int*) data));
            break;
        default:
            DEBUG_ERROR("unknown data type: 0x%x", type);
            return ;
    }
    DEBUG_ERROR("%s[%d] : %s", __func__, __LINE__, sql_stmt);
}

DB_RETVAL_E sql_get_zb_dev(ZB_DEVICE_INFO_S *devs_info, int size, const char* sql_stmt)
{
    int rc;
    int i = 0;
    sqlite3* db;
    sqlite3_stmt* stat;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    long long int euiBuffer;

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
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            sql_close_db(db);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_INFO("[DB]Get Device Error!(%d)\n", rc);
            sql_close_db(db);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
/*
(ZB_DEV_ID         INT, 
 EUI64             INT, 
 NETWORK_STATUS    INT, 
 NODE_ID           INT, 
 ENDPOINT          INT, 
 DEVICE_TYPE       INT, 
 CLUSTER_ID        INT, 
 ATTRIBUTE_ID      INT, 
 DATA_TYPE         INT, 
 ATTRIBUTE_VALUE   CHAR(128));"
*/
/*
    unsigned int extend_id;
    WORD node_id;
    BYTE endpoint;
    EmberEUI64 eui64;
    EmberNetworkStatus status;
    ZIGBEE_HA_DEVICE_ID_E dev_id;
    WORD cluster_id;
    ZIGBEE_CLUSTER_U cluster;
*/
        /* 1.1 Get device info */
        devs_info[i].extend_id = sqlite3_column_int(stat, 0);
        euiBuffer = sqlite3_column_int64(stat, 1);
        memcpy(&devs_info[i].eui64, &euiBuffer, 8);
        devs_info[i].status = sqlite3_column_int(stat, 2);
        devs_info[i].node_id = sqlite3_column_int(stat, 3);
        devs_info[i].endpoint = sqlite3_column_int(stat, 4);
        devs_info[i].dev_id = sqlite3_column_int(stat, 5);
        devs_info[i].cluster_id = sqlite3_column_int(stat, 6);
        devs_info[i].cluster.onOff_cluster.cluster_id = devs_info[i].cluster_id;

        DEBUG_INFO("[DB]SQL Get [%u][%02x%02x%02x%02x%02x%02x%02x%02x][%u][%u][%u][0x%x][%u]",
            devs_info[i].extend_id,
            devs_info[i].eui64[0],
            devs_info[i].eui64[1],
            devs_info[i].eui64[2],
            devs_info[i].eui64[3],
            devs_info[i].eui64[4],
            devs_info[i].eui64[5],
            devs_info[i].eui64[6],
            devs_info[i].eui64[7],
            devs_info[i].status,
            devs_info[i].node_id,
            devs_info[i].endpoint,
            devs_info[i].dev_id,
            devs_info[i].cluster_id);

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

    for(i = 0; i < size; i++)
    {
        if(devs_info[i].extend_id != 0)
        {
            ret = sql_get_zb_dev_value(db, devs_info[i].extend_id, devs_info[i].cluster_id, &(devs_info[i].cluster));
            if(ret != DB_RETVAL_OK) {
                ctrl_log_print(LOG_ERR, __func__, __LINE__, "fail to get zb device value[dev_id : 0x%x]", devs_info[i].extend_id);
            }
        }
    }

    sql_close_db(db);

    if(ret == DB_NO_DEV_VALUE)
    {
        DEBUG_INFO("[DB]device get no value");
    }

    return ret;
}

/* Get ZB_ATTRIBUTE_S table then map ZB_ATTRIBUTE_S table to ZIGBEE_CLUSTER_U according cluster id */
DB_RETVAL_E sql_get_zb_dev_value(sqlite3 *db, unsigned int dev_id, unsigned int cluster_id, ZIGBEE_CLUSTER_U* cluster)
{
    char sql_stmt[SQL_BUF_LEN];
    int attr_num = 0;
    ZB_ATTRIBUTE_S* zb_attr_ptr = NULL;
    const unsigned char* attr_val_ptr;
    int rc;
    int i = 0;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    sqlite3_stmt * stat = NULL;

    /* 1.1 Get attribute count */
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_ZB_ATTRIBUTE_COUNT_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&attr_num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        printf("SQL statement's excution error !!\n");
        ret = DB_GET_NUM_FAIL;
        goto error_return;
    }

    if(attr_num == 0)
    {
        ret = DB_NO_DEV_VALUE;
        goto error_return;
    }

    /* 1.3 SQL get attribute table */
    /* 1.3.1 malloc attribute table */
    zb_attr_ptr = (ZB_ATTRIBUTE_S*)malloc(attr_num * sizeof(ZB_ATTRIBUTE_S));
    if(zb_attr_ptr == NULL)
    {
        ret = DB_MALLOC_ERROR;
        goto error_return;
    }

    memset(zb_attr_ptr, 0, attr_num * sizeof(ZB_ATTRIBUTE_S));

    /* 1.3.2 prepare SQL command : get attribute table */
    memset(sql_stmt, 0, SQL_BUF_LEN);

    sprintf(sql_stmt, SQL_CMD_GET_ZB_ATTRIBUTE_VALUE_ALL_BY_DEV_ID, dev_id);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]Get log fail(%d):%s!\n", rc, sqlite3_errmsg(db));
        ret = DB_GET_DEV_FAIL;
        goto error_return;
    }

    /* 1.3.3 SQL read row */
    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stat);
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            ret = DB_NO_RECORD;
            goto error_return;
        }
        else
        {
            DEBUG_INFO("[DB]Get Device Error!(%d)\n", rc);
            ret = DB_GET_DEV_FAIL;
            goto error_return;
        }
    }
    while ((rc == SQLITE_ROW) && (i < attr_num))
    {
        zb_attr_ptr[i].attribute_id = sqlite3_column_int(stat, 0);
        zb_attr_ptr[i].data_type = sqlite3_column_int(stat, 1);
        attr_val_ptr = sqlite3_column_text(stat, 2);
        strncpy(zb_attr_ptr[i].value, (char*)attr_val_ptr, 128);
        zb_attr_ptr[i].value[127] = '\0';

        DEBUG_INFO("[DB][0x%x][attribute_id : 0x%x]=%s\n", dev_id, zb_attr_ptr[i].attribute_id, zb_attr_ptr[i].value);
        i++;
        rc = sqlite3_step(stat);
        if(rc != SQLITE_ROW)
        {
            if(rc == SQLITE_DONE)
                ; //DEBUG_INFO("[DB]No More Record Get!(%d)\n", rc);
            else
                DEBUG_ERROR("[DB]Get Record Error!(%d)\n", rc);
        }

        if(i >= attr_num)
        {
            //DEBUG_INFO("[DB]Size is enough!(%d)\n", rc);
        }
    }

    /* 1.3.4 Destructor for sqlite3_stmt*/
    sqlite3_finalize(stat);

    /* 1.4 mapping attribute table to ZB_DEVICE_INFO_S - cluster */
    DB_RETVAL_E (*attribute_value_handler) (ZB_ATTRIBUTE_S*, ZIGBEE_CLUSTER_U*);

    switch(cluster_id)
    {
        case ZCL_ON_OFF_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_onOff_value;
            break;
        case ZCL_LEVEL_CONTROL_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_level_value;
            break;
        case ZCL_SIMPLE_METERING_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_simpleMeter_value;
            break;
        case ZCL_ILLUM_MEASUREMENT_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_illuminance_value;
            break;
        case ZCL_TEMP_MEASUREMENT_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_temperature_value;
            break;
        case ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_humidity_value;
            break;
        case ZCL_IAS_ZONE_CLUSTER_ID:
            attribute_value_handler = &db_get_zb_ias_value;
            break;
        default:
            DEBUG_ERROR("no-support cluster id : 0x%x", cluster_id);
            ret = DB_GET_DEV_FAIL;
            goto error_return;
    }

    for(i = 0; i < attr_num; i++)
    {
        ret = attribute_value_handler(&zb_attr_ptr[i], cluster);
        if(ret != DB_RETVAL_OK) {
            ctrl_log_print(LOG_ERR, __func__, __LINE__, "fail to parse attribute value zb_attr_ptr[%d].", i);
            /* not return here */
        }
    }
    ret = DB_RETVAL_OK;

    goto error_return;

error_return:
    if(zb_attr_ptr != NULL)
        free(zb_attr_ptr);

    return ret;
}

/* Mapping attribute value according attribute id */
DB_RETVAL_E db_get_zb_onOff_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
/*
    BOOL on_off;
    DWORD sample_mfg_specfic;
    BOOL global_scene_control;
    DWORD on_time;
    DWORD off_wait_time;
*/
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->onOff_cluster.on_off);
            break;
        case 0x0400:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->onOff_cluster.global_scene_control);
            break;
        case 0x0401:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->onOff_cluster.on_time);
            break;
        case 0x0402:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->onOff_cluster.off_wait_time);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_zb_level_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.current_level);
            break;
        case 0x0001:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.remain_time);
            break;
        case 0x0010:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.transition_time);
            break;
        case 0x0011:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.on_level);
            break;
        case 0x0012:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.on_transition_time);
            break;
        case 0x0013:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.off_transition_time);
            break;
        case 0x0014:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->level_cluster.move_rate);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_zb_simpleMeter_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
/*
    unsigned long long int current_sum_value;
    BYTE status;
    BYTE unit_of_measure;
    BYTE summation_format;
    BYTE meter_device_type;
    int instantaneous_demand;
*/
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->simpleMeter_cluster.current_sum_value);
            break;
        case 0x0200:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->simpleMeter_cluster.status);
            break;
        case 0x0300:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->simpleMeter_cluster.unit_of_measure);
            break;
        case 0x0303:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->simpleMeter_cluster.summation_format);
            break;
        case 0x0306:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->simpleMeter_cluster.meter_device_type);
            break;
        case 0x0400:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->simpleMeter_cluster.instantaneous_demand);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_zb_illuminance_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
/*
    WORD value;
    WORD min;
    WORD max;
    WORD tolerance;
    BYTE light_sensor_type;
*/
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->illuminance_cluster.value);
            break;
        case 0x0001:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->illuminance_cluster.min);
            break;
        case 0x0002:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->illuminance_cluster.max);
            break;
        case 0x0003:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->illuminance_cluster.tolerance);
            break;
        case 0x0004:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->illuminance_cluster.light_sensor_type);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_zb_temperature_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
/*
    WORD value;
    WORD min;
    WORD max;
    WORD tolerance;
*/
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->temperature_cluster.value);
            break;
        case 0x0001:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->temperature_cluster.min);
            break;
        case 0x0002:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->temperature_cluster.max);
            break;
        case 0x0003:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->temperature_cluster.tolerance);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_zb_humidity_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
/*
    WORD value;
    WORD min;
    WORD max;
    WORD tolerance;
*/
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->humidity_cluster.value);
            break;
        case 0x0001:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->humidity_cluster.min);
            break;
        case 0x0002:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->humidity_cluster.max);
            break;
        case 0x0003:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->humidity_cluster.tolerance);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

DB_RETVAL_E db_get_zb_ias_value(ZB_ATTRIBUTE_S* attr, ZIGBEE_CLUSTER_U* cluster)
{
/*
    BYTE zone_state;
    WORD zone_type;
    WORD zone_status;
    double ias_cie_address;
*/
    switch(attr->attribute_id)
    {
        case 0x0000:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->ias_cluster.zone_state);
            break;
        case 0x0001:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->ias_cluster.zone_type);
            break;
        case 0x0002:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->ias_cluster.zone_status);
            break;
        case 0x0010:
            db_get_zb_attr(attr->data_type, attr->value, (void*) &cluster->ias_cluster.ias_cie_address);
            break;
        default:
            DEBUG_ERROR("unknown attribute id: 0x%x", attr->attribute_id);
            return DB_PARAM_ERROR;
    }

    return DB_RETVAL_OK;
}

/* Type conversion according data type */
void db_get_zb_attr(unsigned int data_type, char* attr_val, void* dest)
{
    unsigned int attr_size, dest_size;

    attr_size = sizeof(attr_val);
    dest_size = sizeof(dest);
    ctrl_log_print(LOG_INFO, __func__, __LINE__, "data_type[0x%02x], attr_val[size : %d] = '%s', dest[size : %d]", data_type, attr_size, attr_val, dest_size);

    switch(data_type & 0xF8)
    {
        case NO_DATA:
            return ;
        case DATA_8BIT:
        case BOOLEAN:
        case BITMAP_8BIT:
        case INT_8BIT:
        case ENUM_8BIT:
            *((int*) dest) = atoi(attr_val);
            break;
        case UINT_8BIT:
            string_to_unsigned_num(attr_val, (unsigned int*) dest);
            break;
        case SEMI_FLOAT:
            *((double*) dest) = atof(attr_val);
            break;
        default:
            DEBUG_ERROR("unknown data type: 0x%x", data_type);
            return ;
    }
}
