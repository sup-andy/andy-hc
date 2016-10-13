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
*   File   : zw_sql.c
*   Abstract:
*   Date   : 12/13/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Mark.Yan  06/03/2015 0.1  Initial Dev.
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

extern int sql_cb_get_count(void *buf, int argc, char **argv, char **azColName);
extern DB_RETVAL_E sql_get_num(int* size, char* sql_stmt);

/****************************CHECK TABLE EXISTS SQL STATEMENT******************/
#define SQL_CMD_CHECK_TABLE_EXIST "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';"

/****************************ZW_DEVICE_TABLE SQL STATEMENT*************************/
#define SQL_CMD_CREATE_ZW_DEV_TABLE "CREATE TABLE ZW_DEVICE_TABLE " \
                                            "(ZW_DEV_ID             INT, " \
                                            " PHY_ID                INT, " \
                                            " DEV_NUM               INT, " \
                                            " DEV_TYPE              INT, " \
                                            " CMDCLASS_BITMASK      BLOB, " \
                                            " PRIMARY KEY(ZW_DEV_ID), " \
                                            " FOREIGN KEY(ZW_DEV_ID) REFERENCES DEVICE_TABLE(DEV_RFID));"

#define SQL_CMD_ADD_ZW_DEV "INSERT INTO ZW_DEVICE_TABLE " \
                             "(ZW_DEV_ID, PHY_ID, DEV_NUM, DEV_TYPE, CMDCLASS_BITMASK) " \
                             "VALUES (%u, %u, %u, %u, ?);"

#define SQL_CMD_GET_ZW_DEV_VALUE_ALL_BY_DEV_ID "SELECT * FROM ZW_DEVICE_TABLE WHERE ZW_DEV_ID=%u;"

#define SQL_CMD_DEL_ZW_DEV_BY_DEV_ID "DELETE FROM ZW_DEVICE_TABLE WHERE ZW_DEV_ID=%u;"

#define SQL_CMD_DEL_ZW_DEV_ALL "DELETE FROM ZW_DEVICE_TABLE;"

#define SQL_CMD_GET_ZW_DEV_COUNT_BY_DEV_ID "SELECT count(*) FROM ZW_DEVICE_TABLE WHERE ZW_DEV_ID=%u;"

#define SQL_CMD_GET_ZW_DEV_ALL "SELECT * FROM ZW_DEVICE_TABLE;"

#define SQL_CMD_GET_ZW_DEV_ALL_NUM "SELECT count(*) FROM ZW_DEVICE_TABLE;"

DB_RETVAL_E sql_init_zw_device_table(sqlite3 *db)
{
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int size = 0;

    if (CAPI_SUCCESS == capi_get_product_has_zwave() &&
            0 == access("/dev/zwave", F_OK))
    {
        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "ZW_DEVICE_TABLE");
        rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Check ZW_DEVICE_TABLE Exist ERROR !!\n");
            return DB_EXEC_ERROR;
        }

        if (size == 1)
        {
            DEBUG_INFO("[DB]ZW_DEVICE_TABLE TABLE already exists. skip create\n");
            return DB_TABLE_EXIST;
        }

        memset(sql_stmt, 0, SQL_BUF_LEN);
        sprintf(sql_stmt, SQL_CMD_CREATE_ZW_DEV_TABLE);
        rc = sql_exec(1, db, sql_stmt, NULL, 0);
        if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            DEBUG_ERROR("[DB]Create ZW_DEVICE_TABLE ERROR !!\n");
            return DB_EXEC_ERROR;
        }
    }
    return DB_RETVAL_OK;
}

DB_RETVAL_E sql_add_zw_dev(ZW_DEVICE_INFO *devs_info, int size)
{
    int i;
    int rc;
    char sql_stmt[SQL_BUF_LEN];
    int dev_num = 0;
    char* zErrMsg;
    sqlite3_stmt* stat = NULL;
    sqlite3* db;
    char *sql_cmd;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    void *buffer;
    int buffer_size;
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
        buffer_size = MAX_CMDCLASS_BITMASK_LEN;
        buffer = (void *)&(devs_info[i].cmdclass_bitmask[0]);

        // (ZW_DEV_ID, PHY_ID, DEV_NUM, DEV_TYPE, CMDCLASS_BITMASK) 
        snprintf(sql_stmt, SQL_BUF_LEN, SQL_CMD_ADD_ZW_DEV, 
                devs_info[i].id, devs_info[i].phy_id, devs_info[i].dev_num, devs_info[i].type);
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
            DEBUG_ERROR("[DB]Add devices fail(%d):%s!\n", rc, sqlite3_errmsg(db));
            sqlite3_finalize(stat);
            ret = DB_ADD_DEV_FAIL;
            goto EXIT;
        }

        sqlite3_finalize(stat);
    }    
    

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_get_zw_dev(ZW_DEVICE_INFO *devs_info, int size, const char* sql_stmt)
{
    int rc;
    int i = 0;
    sqlite3* db;
    sqlite3_stmt* stat;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    const void *buf;
    int buffer_size;

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
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            sqlite3_finalize(stat);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_INFO("[DB]Get Device Error!(%d)\n", rc);
            sqlite3_finalize(stat);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        /*
         (ZW_DEV_ID             INT, " \
        " PHY_ID                INT, " \
        " DEV_NUM               INT, " \
        " DEV_TYPE              INT, " \
        " CMDCLASS_BITMASK      BLOB, " \
        */
        devs_info[i].id = sqlite3_column_int(stat, 0);
        devs_info[i].phy_id = sqlite3_column_int(stat, 1);
        devs_info[i].dev_num = sqlite3_column_int(stat, 2);
        devs_info[i].type = sqlite3_column_int(stat, 3);
        buf = sqlite3_column_blob(stat, 5);
        buffer_size = sizeof(devs_info[i].cmdclass_bitmask);
        memcpy(&(devs_info[i].cmdclass_bitmask), (unsigned char*)buf, buffer_size);
       

        DEBUG_INFO("[DB]SQL Get ZW_DEV_ID[0x%x] PHY_ID[0x%x] DEV_NUM[%u] DEV_TYPE[%u]",
            devs_info[i].id,
            devs_info[i].phy_id,
            devs_info[i].dev_num,
            devs_info[i].type);

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

        sqlite3_finalize(stat);

    }

    //sqlite3_finalize(stat);


    sql_close_db(db);

    if (ret == DB_NO_DEV_VALUE)
    {
        DEBUG_INFO("[DB]device get no value");
    }

    return ret;
}

DB_RETVAL_E sql_get_zw_dev_with_db(sqlite3* db, ZW_DEVICE_INFO *devs_info, int size, const char* sql_stmt)
{
    int rc;
    int i = 0;
    sqlite3_stmt* stat;
    DB_RETVAL_E ret = DB_RETVAL_OK;
    const void *buf;
    int buffer_size;


    DEBUG_INFO("sql_stmt is [%s]", sql_stmt);
    rc = sqlite3_prepare(db, sql_stmt, -1, &stat, 0);
    if (rc != SQLITE_OK)
    {
        DEBUG_ERROR("[DB]prepare fail(%d):%s!\n", rc, sqlite3_errmsg(db));
    }
    
    rc = sqlite3_step(stat);
    if(rc != SQLITE_ROW)
    {
        if(rc == SQLITE_DONE)
        {
            DEBUG_INFO("[DB]No Record Get!(%d)\n", rc);
            sqlite3_finalize(stat);
            return DB_NO_RECORD;
        }
        else
        {
            DEBUG_INFO("[DB]Get Device Error!(%d)\n", rc);
            sqlite3_finalize(stat);
            return DB_GET_DEV_FAIL;
        }
    }
    while ((rc == SQLITE_ROW) && (i < size))
    {
        /*
         (ZW_DEV_ID             INT, " \
        " PHY_ID                INT, " \
        " DEV_NUM               INT, " \
        " DEV_TYPE              INT, " \
        " CMDCLASS_BITMASK      BLOB, " \
        */
        devs_info[i].id = sqlite3_column_int(stat, 0);
        devs_info[i].phy_id = sqlite3_column_int(stat, 1);
        devs_info[i].dev_num = sqlite3_column_int(stat, 2);
        devs_info[i].type = sqlite3_column_int(stat, 3);
        buf = sqlite3_column_blob(stat, 4);
        buffer_size = sizeof(devs_info[i].cmdclass_bitmask);
        memcpy(&(devs_info[i].cmdclass_bitmask), (unsigned char*)buf, buffer_size);
       

        DEBUG_INFO("[DB]SQL Get ZW_DEV_ID[0x%x] PHY_ID[0x%x] DEV_NUM[%u] DEV_TYPE[%u]",
            devs_info[i].id,
            devs_info[i].phy_id,
            devs_info[i].dev_num,
            devs_info[i].type);

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

        sqlite3_finalize(stat);

    }

    //sqlite3_finalize(stat);

    if (ret == DB_NO_DEV_VALUE)
    {
        DEBUG_INFO("[DB]device get no value");
    }

    return ret;
}

DB_RETVAL_E sql_del_zw_dev_by_dev_id(unsigned int dev_id)
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
    sprintf(sql_stmt, SQL_CMD_GET_ZW_DEV_COUNT_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&attr_num);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
        ret = DB_GET_NUM_FAIL;
        goto EXIT;
    }

    if(attr_num == 0)
    {
        ret = DB_NO_DEVICE;
        goto EXIT;
    }

    /* 1.2 Delete dev by device id */
    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_DEL_ZW_DEV_BY_DEV_ID, dev_id);

    rc = sql_exec(1, db, sql_stmt, NULL, 0);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_INFO("[DB]SQL exec error !!\n");
        ret = DB_EXEC_ERROR;
        goto EXIT;
    }

EXIT:
    sql_close_db(db);

    return ret;
}

DB_RETVAL_E sql_reset_zw_dev(void)
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
    sprintf(sql_stmt, SQL_CMD_CHECK_TABLE_EXIST, "ZW_DEVICE_TABLE");
    rc = sql_exec(1, db, sql_stmt, sql_cb_get_count, (void*)&size);
    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        DEBUG_ERROR("[DB]Check ZW_DEVICE_TABLE Exist ERROR !!\n");
        ret =  DB_EXEC_ERROR;
        goto EXIT;
    }

    if (size == 1)
    {
        /* 1.1 Reset ZW_DEVICE_TABLE */
        rc = sql_exec(1, db, SQL_CMD_DEL_ZW_DEV_ALL, NULL, 0);
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

DB_RETVAL_E db_add_zw_dev(ZW_DEVICE_INFO *devs_info, int size)
{
    int ret = DB_RETVAL_OK;

    ret = sql_add_zw_dev(devs_info, size);
    if (DB_RETVAL_OK == ret)
    {
        save_db();
    }

    return ret;
}

DB_RETVAL_E db_get_zw_dev_all(ZW_DEVICE_INFO *devs_info, int size)
{
    return sql_get_zw_dev(devs_info, size, SQL_CMD_GET_ZW_DEV_ALL);
}

DB_RETVAL_E db_get_zw_dev_all_num(int *dev_num)
{
    return sql_get_num(dev_num, SQL_CMD_GET_ZW_DEV_ALL_NUM);
}

DB_RETVAL_E db_get_zw_dev_by_dev_id(unsigned int dev_id, ZW_DEVICE_INFO *devs_info)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_ZW_DEV_VALUE_ALL_BY_DEV_ID, dev_id);
    return sql_get_zw_dev(devs_info, 1, sql_stmt);
}

DB_RETVAL_E db_get_zw_dev_by_dev_id_with_db(sqlite3* db, unsigned int dev_id, ZW_DEVICE_INFO *devs_info)
{
    char sql_stmt[SQL_BUF_LEN];

    memset(sql_stmt, 0, SQL_BUF_LEN);
    sprintf(sql_stmt, SQL_CMD_GET_ZW_DEV_VALUE_ALL_BY_DEV_ID, dev_id);
    return sql_get_zw_dev_with_db(db, devs_info, 1, sql_stmt);
}


DB_RETVAL_E db_del_zw_dev_by_dev_id(unsigned int dev_id)
{
    int ret = DB_RETVAL_OK;

    ret = sql_del_zw_dev_by_dev_id(dev_id);
    if (DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}

DB_RETVAL_E db_reset_zw_dev()
{
    int ret = DB_RETVAL_OK;

    ret = sql_reset_zw_dev();
    if (DB_RETVAL_OK == ret)
    {
        save_db();
    }
    
    return ret;
}
