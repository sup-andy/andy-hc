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
*   Creator : Sharon Tseng
*   File   : lprf_sql.c
*   Abstract:
*   Date   : 12/24/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Sharon.T 12/24/2015 0.1  Initial Dev.
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
#include "lprf_msg.h"

#define SQL_BUF_LEN 512
#define SQL_BUF_LEN_L 2048

typedef struct
{
    unsigned int attribute_id;
    unsigned int data_type;
    char value[128];
}LPRF_ATTRIBUTE_S;

DB_RETVAL_E sql_init_lprf_device_table(sqlite3 *db);
DB_RETVAL_E db_add_lprf_dev(LPRF_DEVICE_INFO_S *devs_info, int size);
DB_RETVAL_E db_get_lprf_dev_by_dev_id(unsigned int dev_id, LPRF_DEVICE_INFO_S *devs_info);
DB_RETVAL_E dbd_add_lprf_dev(LPRF_DEVICE_INFO_S *devs_info);
DB_RETVAL_E dbd_del_lprf_dev_by_dev_id(unsigned int dev_id);
DB_RETVAL_E dbd_get_lprf_dev_by_dev_id(LPRF_DEVICE_INFO_S * dev_info, LPRF_DEVICE_INFO_S* dev_ptr);
DB_RETVAL_E sql_add_lprf_dev(LPRF_DEVICE_INFO_S* devs_info, int size);
DB_RETVAL_E sql_del_lprf_dev_by_dev_id(unsigned int dev_id);
DB_RETVAL_E sql_get_lprf_dev(LPRF_DEVICE_INFO_S *devs_info, int size, const char* sql_stmt);
DB_RETVAL_E sql_reset_lprf_dev(void);

int sql_cb_get_count(void *buf, int argc, char **argv, char **azColName);
extern int string_to_unsigned_num(char *str, OUT int *value);

/****************************CHECK TABLE EXISTS SQL STATEMENT******************/
#define SQL_CMD_CHECK_TABLE_EXIST "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';"

/****************************LPRF_DEVICE_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_LPRF_DEV_TABLE "CREATE TABLE LPRF_DEVICE_TABLE " \
                                            "(LPRF_DEV_ID             INT, " \
                                            " DEVICE_TYPE       INT, " \
                                            " PRIMARY KEY(LPRF_DEV_ID), " \
                                            " FOREIGN KEY(LPRF_DEV_ID) REFERENCES DEVICE_TABLE(DEV_RFID));"

#define SQL_CMD_GET_LPRF_DEV_ID "SELECT DISTINCT LPRF_DEV_ID FROM LPRF_DEVICE_TABLE;"

#define SQL_CMD_GET_LPRF_DEV_NUM "SELECT count(DISTINCT LPRF_DEV_ID) FROM LPRF_DEVICE_TABLE;"

#define SQL_CMD_GET_LPRF_DEV_VALUE_ALL_BY_DEV_ID "SELECT * FROM LPRF_DEVICE_TABLE WHERE LPRF_DEV_ID=%u;"

#define SQL_CMD_ADD_LPRF_DEV "INSERT INTO LPRF_DEVICE_TABLE VALUES (%u, %u);"

#define SQL_CMD_DEL_LPRF_DEV_BY_DEV_ID "DELETE FROM LPRF_DEVICE_TABLE WHERE LPRF_DEV_ID=%u;"

#define SQL_CMD_SELECT_LPRF_DEV_VALUE_NUM_BY_DEV_ID "SELECT count(*) FROM LPRF_DEVICE_TABLE WHERE LPRF_DEV_ID=%u;"

#define SQL_CMD_GET_LPRF_ATTRIBUTE_COUNT_BY_DEV_ID "SELECT count(*) FROM LPRF_DEVICE_TABLE WHERE LPRF_DEV_ID=%u;"

#define SQL_CMD_GET_LPRF_ATTRIBUTE_VALUE_ALL_BY_DEV_ID "SELECT ATTRIBUTE_ID,DATA_TYPE,ATTRIBUTE_VALUE FROM LPRF_DEVICE_TABLE WHERE LPRF_DEV_ID=%u;"

#define SQL_CMD_DEL_LPRF_DEV_VALUE_BY_ID "DELETE FROM LPRF_DEVICE_VALUE_TABLE WHERE LPRF_DEV_ID=%u;"

#define SQL_CMD_DEL_LPRF_DEV_ALL "DELETE FROM LPRF_DEVICE_TABLE;"

DB_RETVAL_E sql_init_lprf_device_table(sqlite3 *db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    if (CAPI_SUCCESS == capi_get_product_has_lprf() &&
            0 == access("/dev/lprf", F_OK))
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "LPRF_DEVICE_TABLE");
        rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Check LPRF_DEVICE_TABLE Exist ERROR !!\n");
            return DB_EXEC_ERROR;
        }

        if(size == 1)
        {
            DEBUG_INFO("[DB]LPRF_DEVICE_TABLE TABLE already exists. skip create\n");
            return DB_TABLE_EXIST;
        }

        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_CREATE_LPRF_DEV_TABLE);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Create LPRF_DEVICE_TABLE ERROR !!\n");
            return DB_EXEC_ERROR;
        }
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E db_add_lprf_dev(LPRF_DEVICE_INFO_S *devs_info, int size)
{
    return sql_add_lprf_dev(devs_info, size);
}

DB_RETVAL_E db_get_lprf_dev_by_dev_id(unsigned int dev_id, LPRF_DEVICE_INFO_S *devs_info)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_LPRF_DEV_VALUE_ALL_BY_DEV_ID, dev_id);
    return sql_get_lprf_dev(devs_info, 1, sql_stmt);
}

DB_RETVAL_E dbd_add_lprf_dev(LPRF_DEVICE_INFO_S *devs_info)
{
    int ret = DB_RETVAL_OK;

    ret = db_add_lprf_dev(devs_info, 1);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E dbd_del_lprf_dev_by_dev_id(unsigned int dev_id)
{
    int ret = DB_RETVAL_OK;

    ret = sql_del_lprf_dev_by_dev_id(dev_id);
    if(DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E dbd_get_lprf_dev_by_dev_id(LPRF_DEVICE_INFO_S * dev_info, LPRF_DEVICE_INFO_S* dev_ptr)
{
    return db_get_lprf_dev_by_dev_id(dev_info->lprf_dev_id, dev_ptr);
}
 
DB_RETVAL_E dbd_reset_lprf_dev()
{
    DB_RETVAL_E ret = DB_RETVAL_OK;

    ret = sql_reset_lprf_dev();
    if (DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E sql_add_lprf_dev(LPRF_DEVICE_INFO_S* devs_info, int size)
{
    int i;
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    sqlite3* db;
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

    memset(sql_stmt, 0, SQL_BUF_LEN);
    for (i = 0; i < size; i++)
    {
        snprintf(sql_stmt, SQL_BUF_LEN, SQL_CMD_ADD_LPRF_DEV, devs_info[i].lprf_dev_id, 
                 devs_info[i].sensor_type/*, devs_info[i].network_status, devs_info[i].device_active, devs_info[i].report_status*/);

        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB] sql_stmt: %s !!\n", sql_stmt);
            ret = DB_ADD_DEV_FAIL;
            goto EXIT;
        }
    }

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_del_lprf_dev_by_dev_id(unsigned int dev_id)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    int attr_num = 0;
    sqlite3* db;
    DB_RETVAL_E ret = DB_RETVAL_OK;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    /* 1.1 Get attribute count */
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_LPRF_ATTRIBUTE_COUNT_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&attr_num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        printf("SQL statement's excution error !!\n");
        ret = DB_GET_NUM_FAIL;
        goto EXIT;
    }

    if(attr_num == 0)
    {
        ret = DB_NO_DEV_VALUE;
        goto EXIT;
    }

    /* 1.2 Delete dev by device id */
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_LPRF_DEV_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
        ret = DB_EXEC_ERROR;
    }

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_get_lprf_dev(LPRF_DEVICE_INFO_S *devs_info, int size, const char* sql_stmt)
{
    int rc;
    int i = 0;
    sqlite3* db;
    sqlite3_stmt* stat;
    DB_RETVAL_E ret = DB_RETVAL_OK;

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
            DEBUG_INFO("[DB]Get Device Error!(%d)\n", rc);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        /* 1.1 Get device info */
        devs_info[i].lprf_dev_id = sqlite3_column_int(stat, 0);
        devs_info[i].sensor_type = sqlite3_column_int(stat, 1);
        //devs_info[i].network_status = sqlite3_column_int(stat, 2);
        //devs_info[i].device_active = sqlite3_column_int(stat, 3);
        //devs_info[i].report_status = sqlite3_column_int(stat, 4);

        //DEBUG_INFO("[DB]SQL Get [0x%x][%u][%u][%u][%u]",
        DEBUG_INFO("[DB]SQL Get [0x%x][%u]",
            devs_info[i].lprf_dev_id,
            devs_info[i].sensor_type
            /*,devs_info[i].network_status,
            devs_info[i].device_active,
            devs_info[i].report_status*/);

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

    sql_close_db(db);

    if(ret == DB_NO_DEV_VALUE)
    {
        DEBUG_INFO("[DB]device get no value");
    }

    return ret;
}

DB_RETVAL_E sql_reset_lprf_dev(void)
{
    char sql_stmt[SQL_BUF_LEN];
    int rc;
    sqlite3* db;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    int size = 0;

    db = sql_open_db();
    if (db == NULL)
    {
        return DB_OPEN_ERROR;
    }

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "LPRF_DEVICE_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check LPRF_DEVICE_TABLE Exist ERROR !!\n");
        ret =  DB_EXEC_ERROR;
        goto EXIT;
    }

    if (size == 1)
    {
        /* 1.1 Reset LPRF_DEVICE_TABLE */
        rc = sql_exec(1, db, SQL_CMD_DEL_LPRF_DEV_ALL, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_INFO("[DB]SQL exec error !!\n");
            DB_EXEC_ERROR;
            goto EXIT;
        }
    }

EXIT:
    sql_close_db(db);

    return ret;
}