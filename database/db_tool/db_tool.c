/**********************************************************************
 *    Copyright 2015 WondaLink CO., LTD.
 *    All Rights Reserved. This material can not be duplicated for any
 *    profit-driven enterprise. No portions of this material can be reproduced
 *    in any form without the prior written permission of WondaLink CO., LTD.
 *    Forwarding, transmitting or communicating its contents of this document is
 *    also prohibited.
 *
 *    All titles, proprietaries, trade secrets and copyrights in and related to
 *    information contained in this document are owned by WondaLink CO., LTD.
 *
 *    WondaLink CO., LTD.
 *    23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *    HSIN-CHU, TAIWAN R.O.C.
 *
 ******************************************************************************/
/******************************************************************************
 *    Department:
 *    Project :
 *    Block   :
 *    Creator :
 *    File    :
 *    Abstract:
 *    Date    :
 *    $Id:$
 *
 *    Modification History:
 *    By           Date       Ver.   Modification Description
 *    -----------  ---------- -----  -----------------------------
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"
#include <unistd.h>
#include <time.h>
#include "hcapi.h"

#define SQL_CMD_SELECT_DEVICE "SELECT * FROM DEVICE_TABLE "
#define DEV_GET_BY_ID         SQL_CMD_SELECT_DEVICE"WHERE DEV_ID=%u;"

#define DEBUG_INFO printf
//#define DB_PATH "/var/run/device.db"
#define DB_STORAGE_PATH "/storage/config/homectrl/device.db"
void print_other_table_detail(char *table_name);

char g_db_file[256] = {0};

int callback(void* data, int ncols, char** values, char** headers)
{
    int *k = (int*)data;
    int i;
    DEBUG_INFO("Item %d\t| ", *k);
    for (i = 0; i < ncols; i++)
    {
        if (strstr(headers[i], "DEV_ID") 
                || strstr(headers[i], "PHY_ID")
                || strstr(headers[i], "DEV_RFID"))
            DEBUG_INFO("%s = {0x%x}\t| ", headers[i], atoi(values[i]));
        else
        DEBUG_INFO("%s = {%s} \t|", headers[i], values[i]);
    }

    DEBUG_INFO("\n\n");

    (*k)++;
    return 0;
}

int main(int argc, char **argv)
{
    sqlite3 *db;
    int rc;
    char *sql;
    char *zErr;
    int* data;
    int i = 0;

    if (argc < 2)
    {
        print_db_tool_help();
        return -1;
    }

    if (0 == strcmp(argv[1], "help"))
    {
        print_db_tool_help();
        return 0;
    }

    if (argc == 2)
    {
        //fprintf(stdout, "Please select the db path:\n1:Ram\n2:Storage\n");
        //if ('2' == getchar())
        //{
            strncpy(g_db_file, DB_STORAGE_PATH, sizeof(g_db_file));
        //}
        //else
        //{
        //    strncpy(g_db_file, DB_PATH, sizeof(g_db_file));
        //}

        if (0 == strcmp(argv[1], "device_table"))
        {
            DEBUG_INFO("\n*****start print device_table detail************\n");
            print_dev_table_detail();
            DEBUG_INFO("*****end print device_table detail************\n\n");
            return 0;
        }
        /*
        else if (0 == strcmp(argv[1], "device_log"))
        {
            DEBUG_INFO("*****start print device_log detail************\n");
            print_other_table_detail("device_log");
            DEBUG_INFO("*****end print device_log detail************\n");
            return 0;
        }
        else if (0 == strcmp(argv[1], "user_log"))
        {
            DEBUG_INFO("*****start print user_log detail************\n");
            print_other_table_detail("user_log");
            DEBUG_INFO("*****end print user_log detail************\n");
            return 0;
        }
        else if (0 == strcmp(argv[1], "device_name_table"))
        {
            DEBUG_INFO("*****start print device_name_table detail************\n");
            print_other_table_detail("device_name_table");
            DEBUG_INFO("*****end print device_name_table detail************\n");
            return 0;
        }
        else if (0 == strcmp(argv[1], "device_ext_table"))
        {
            DEBUG_INFO("*****start print device_ext_table detail************\n");
            print_other_table_detail("device_ext_table");
            DEBUG_INFO("*****end print device_ext_table detail************\n");
            return 0;
        }
        else if (0 == strcmp(argv[1], "scene_table"))
        {
            DEBUG_INFO("*****start print scene_table detail************\n");
            print_other_table_detail("scene_table");
            DEBUG_INFO("*****end print scene_table detail************\n");
            return 0;
        }
        else if (0 == strcmp(argv[1], "device_value_table"))
        {
            DEBUG_INFO("*****start print scene_table detail************\n");
            print_other_table_detail("device_value_table");
            DEBUG_INFO("*****end print scene_table detail************\n");
            return 0;
        }*/
        else
        {
            DEBUG_INFO("\n*****start print %s detail************\n", argv[1]);
            print_other_table_detail(argv[1]);
            DEBUG_INFO("*****end print %s detail************\n\n", argv[1]);
            return 0;
        }
    }

    rc = sqlite3_open(argv[1], &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    data = &i;
    rc = sqlite3_exec(db, argv[2], callback, data, &zErr);

    if (rc != SQLITE_OK)
    {
        if (zErr != NULL)
        {
            fprintf(stderr, "SQL error: %s\n", zErr);
            sqlite3_free(zErr);
        }
    }

    sqlite3_close(db);

    return 0;
}

int db_tool_print_out_binary_value_by_dev_type(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_U* content)
{
    HC_DEVICE_U dev_u;
    memset(&dev_u, 0, sizeof(dev_u));
    memcpy(&dev_u, content, sizeof(dev_u));
    switch (dev_type)
    {
        case HC_DEVICE_TYPE_BINARYSWITCH:
            DEBUG_INFO("binaryswitch.value = [%d]\n", dev_u.binaryswitch.value);
            break;
        case  HC_DEVICE_TYPE_DOUBLESWITCH:
            DEBUG_INFO("doubleswitch.value = [%d]\n", dev_u.doubleswitch.value);
            break;
        case HC_DEVICE_TYPE_DIMMER:
            DEBUG_INFO("dimmer.value = [%d]\n", dev_u.dimmer.value);
            break;
        case HC_DEVICE_TYPE_CURTAIN:
            DEBUG_INFO("curtain.value = [%d]\n", dev_u.curtain.value);
            break;
        case HC_DEVICE_TYPE_BINARY_SENSOR:
            DEBUG_INFO("binary_sensor.value = [%d]\n", dev_u.binary_sensor.value);
            break;
        case HC_DEVICE_TYPE_MULTILEVEL_SENSOR:
            DEBUG_INFO("multilevel_sensor.sensor_type = [%d]\n" \
                       "scale = [%d]\nvalue = [%lf]\n",
                       dev_u.multilevel_sensor.sensor_type,
                       dev_u.multilevel_sensor.scale,
                       dev_u.multilevel_sensor.value);
            break;

        case HC_DEVICE_TYPE_BATTERY:
            DEBUG_INFO("battery.betery_level = [%d]\ninterval_time = [%u]\n",
                       dev_u.battery.battery_level, dev_u.battery.interval_time);
            break;
        case HC_DEVICE_TYPE_DOORLOCK:
            DEBUG_INFO("doorlock.doorLookMode = [%d]\ndoorHandlesMode = [%d]\n" \
                       "doorCondition = [%d]\nlockTimeoutMinutes = [%d]\n" \
                       "lockTimeoutSeconds = [%d]\n",
                       dev_u.doorlock.doorLockMode, dev_u.doorlock.doorHandlesMode,
                       dev_u.doorlock.doorCondition, dev_u.doorlock.lockTimeoutMinutes,
                       dev_u.doorlock.lockTimeoutSeconds);
            break;
        case HC_DEVICE_TYPE_DOORLOCK_CONFIG:
            DEBUG_INFO("doorlock_config.operationType = [%d]\ndoorHandlesMode = [%d]\n" \
                       "lockTimeoutMinutes = [%d]\nlockTimeoutSeconds = [%d]\n",
                       dev_u.doorlock_config.operationType, dev_u.doorlock_config.doorHandlesMode,
                       dev_u.doorlock_config.lockTimeoutMinutes, dev_u.doorlock_config.lockTimeoutSeconds);
            break;
        case HC_DEVICE_TYPE_HSM100_CONFIG:
            DEBUG_INFO("hsm100_config.parameterNumber = [%d]\nbDefault = [%d]\n" \
                       "value = [%ld]\n", dev_u.hsm100_config.parameterNumber,
                       dev_u.hsm100_config.bDefault, dev_u.hsm100_config.value);
            break;
        case HC_DEVICE_TYPE_THERMOSTAT:
            DEBUG_INFO("thermostat.mode = [%d]\nvalue = [%f]\n" \
                       "heat_value = [%f]\ncool_value = [%f]\n" \
                       "energe_save_heat_value = [%f]\nenerge_save_cool_value = [%f]\n",
                       dev_u.thermostat.mode, dev_u.thermostat.value,
                       dev_u.thermostat.heat_value, dev_u.thermostat.cool_value,
                       dev_u.thermostat.energe_save_heat_value, dev_u.thermostat.energe_save_cool_value);
            break;
        case HC_DEVICE_TYPE_METER:
            DEBUG_INFO("meter.meter_type = [%d]\nscale = [%d]\nrate_type = [%d]\n" \
                       "value = [%lf]\ndelta_time = [%d]\nprevious_value = [%lf]\n",
                       dev_u.meter.meter_type, dev_u.meter.scale,
                       dev_u.meter.rate_type, dev_u.meter.value,
                       dev_u.meter.delta_time, dev_u.meter.previous_value);
            break;
     
        case HC_DEVICE_TYPE_ASSOCIATION:
            DEBUG_INFO("association.srcPhyId = [%d]\ndstPhyId = [%d]\n",
                       dev_u.association.srcPhyId, dev_u.association.dstPhyId);
            break;
        case HC_DEVICE_TYPE_HANDSET:
            DEBUG_INFO("handset.status = [%d]\nunion.action = [%s]\n",
                       dev_u.handset.status, dev_u.handset.u.action);
            break;
        case HC_DEVICE_TYPE_IPCAMERA:
            DEBUG_INFO("camera.connection = [%d]\nipaddr = [%s]\n" \
                       "onvifuid = [%s]\nstreamurl = [%s]\n" \
                       "remoteurl = [%s]\ndestPath = [%s]\n" \
                       "triggerTime = [%s]\nlabel = [%s]\n",
                       dev_u.camera.connection, dev_u.camera.ipaddress,
                       dev_u.camera.onvifuid, dev_u.camera.streamurl,
                       dev_u.camera.remoteurl, dev_u.camera.destPath,
                       dev_u.camera.triggerTime, dev_u.camera.label);
            break;
        case HC_DEVICE_TYPE_MAX:
        default:
            DEBUG_INFO("dev_type is default or HC_DEVICE_TYPE_MAX\n");
            break;
    }

    return 0;
}

void print_dev_table_detail()
{
    sqlite3 *db;
    int rc;
    char *sql = "SELECT DEV_ID,PHY_ID,DEV_NAME,EVENT_TYPE,NETWORK_TYPE,DEV_TYPE,DEV_VALUE FROM DEVICE_TABLE;";
    unsigned int dev_id, phy_id;
    const unsigned char* dev_name_ptr;
    sqlite3_stmt* stat;
    int event_type, network_type, dev_type;
    int buffer_size;
    const void* buf;
    int i = 0;

    rc = sqlite3_open(g_db_file, &db);
    if (rc)
    {
        DEBUG_INFO(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    DEBUG_INFO("Item  |  DEV_ID  |  PHY_ID  |             DEV_NAME             | EVENT_TYPE | NETWORK_TYPE | DEV_TYPE \n\n");

    rc = sqlite3_prepare(db, sql, -1, &stat, 0);

    while (sqlite3_step(stat) == SQLITE_ROW)
    {
        //(DEV_ID, PHY_ID, DEV_NAME, EVENT_TYPE, NETWORK_TYPE, DEV_TYPE, DEV_VALUE)
        dev_id = sqlite3_column_int(stat, 0);
        phy_id = sqlite3_column_int(stat, 1);
        dev_name_ptr = sqlite3_column_text(stat, 2);
        event_type = sqlite3_column_int(stat, 3);
        network_type = sqlite3_column_int(stat, 4);
        dev_type = sqlite3_column_int(stat, 5);
        buffer_size = sizeof(HC_DEVICE_U);
        buf = sqlite3_column_blob(stat, 6);
        DEBUG_INFO("%-5d | %-8x | %-8u | %-32s | %-10u | %-12d | %-d" \
                   "\nvalue = \n{\n",
                   i, dev_id, phy_id, (char*)dev_name_ptr, event_type, network_type, dev_type);
        db_tool_print_out_binary_value_by_dev_type(dev_type, (HC_DEVICE_U *)buf);
        DEBUG_INFO("}\n\n");
        i++;
    }
    sqlite3_finalize(stat);

    sqlite3_close(db);

    return 1;
}


void print_other_table_detail(char *table_name)
{
    sqlite3 *db;
    int rc;
    char *sql;
    char *zErr;
    int* data;
    char sql_stmt[512];
    int i = 0;

    rc = sqlite3_open(g_db_file, &db);
    if (rc)
    {
        DEBUG_INFO("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    sql = "select * from %s;";
    sprintf(sql_stmt, sql, table_name);
    //DEBUG_INFO("sql_stmt is %s\n\n", sql_stmt);

    data = &i;
    rc = sqlite3_exec(db, sql_stmt, callback, data, &zErr);
    if (rc != SQLITE_OK)
    {
        if (zErr != NULL)
        {
            fprintf(stderr, "SQL error: %s\n", zErr);
            sqlite3_free(zErr);
        }
    }

    sqlite3_close(db);
}

void print_db_tool_help()
{
    DEBUG_INFO("db_tool:\n");
    DEBUG_INFO("    db_tool <table_name>  -- quick command to show <table_name>'s detail info in storage.\n");
    DEBUG_INFO("                          -- Available: \"device_table\", \"device_log\", \"user_log\", \n");
    DEBUG_INFO("                          -- \"device_name_table\", \"device_ext_table\", \"scene_table\"\n");
    DEBUG_INFO("                          -- !!!INFO:Only quick command can show device's value detail\n");
    DEBUG_INFO("    db_tool <DB File DIR> <SQL command> -- excute SQL command to the database.\n");
    DEBUG_INFO("    Ex.   To show the device table detail: db_tool %s \"select * from device_table;\"\n", DB_STORAGE_PATH);
    DEBUG_INFO("          To delete a record of the device table: db_tool %s \"delete from device_table where dev_id=100\"\n", DB_STORAGE_PATH);
    DEBUG_INFO("\n");
}

