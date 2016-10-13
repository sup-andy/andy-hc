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
*   File   : hc_util.c
*   Abstract:
*   Date   : 1/16/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Rose  06/03/2013 1.0  Initial Dev.
*
******************************************************************************/

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"

#define STR(arg) { arg, #arg }
#define UNKNOWN -1

typedef struct
{
    int id;
    char *txt;
} MAP_STR_S;

static MAP_STR_S map_msg_txt[] = {
    /* message report */
    STR(HC_EVENT_RESP_DEVICE_ADDED_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_ADDED_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_ADD_MODE),
    STR(HC_EVENT_RESP_DEVICE_ADD_COMPLETE),
    STR(HC_EVENT_RESP_DEVICE_ADD_TIMEOUT),
    STR(HC_EVENT_RESP_DEVICE_ADD_STOP_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_ADD_STOP_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_DELETED_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_DELETED_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_DELETE_MODE),
    STR(HC_EVENT_RESP_DEVICE_DELETE_COMPLETE),
    STR(HC_EVENT_RESP_DEVICE_DELETE_TIMEOUT),
    STR(HC_EVENT_RESP_DEVICE_DELETE_STOP_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_DELETE_STOP_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_HAS_BEEN_REMOVED),
    STR(HC_EVENT_RESP_DEVICE_RESET_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_RESET_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_DEFAULT_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_DEFAULT_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_CONNECTED),
    STR(HC_EVENT_RESP_DEVICE_DISCONNECTED),
    STR(HC_EVENT_RESP_DEVICE_DETECT_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_NA_DELETE_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_NA_DELETE_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_SET_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_SET_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_GET_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_GET_FAILURE),

    STR(HC_EVENT_RESP_DEVICE_NORMAL_MODE),

    STR(HC_EVENT_RESP_DEVICE_SET_CFG_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_SET_CFG_FAILURE),
    STR(HC_EVENT_RESP_DEVICE_GET_CFG_SUCCESS),
    STR(HC_EVENT_RESP_DEVICE_GET_CFG_FAILURE),

    STR(HC_EVENT_RESP_ASSOICATION_SET_SUCCESS),
    STR(HC_EVENT_RESP_ASSOICATION_SET_FAILURE),
    STR(HC_EVENT_RESP_ASSOICATION_REMOVE_SUCCESS),
    STR(HC_EVENT_RESP_ASSOICATION_REMOVE_FAILURE),

    STR(HC_EVENT_RESP_FUNCTION_NOT_SUPPORT),
    STR(HC_EVENT_RESP_DB_OPERATION_FAILURE),
    STR(HC_EVENT_RESP_VOIP_CALL),
    STR(HC_EVENT_RESP_DECT_UPGRADE),

    STR(HC_EVENT_RESP_DECT_PARAMETER_SET_SUCCESS),
    STR(HC_EVENT_RESP_DECT_PARAMETER_SET_FAILURE),
    STR(HC_EVENT_RESP_DECT_PARAMETER_GET_SUCCESS),
    STR(HC_EVENT_RESP_DECT_PARAMETER_GET_FAILURE),

    STR(HC_EVENT_STATUS_DEVICE_STATUS_CHANGED),
    STR(HC_EVENT_STATUS_DEVICE_STATUS_CHANGED_DB_FAILURE),
    STR(HC_EVENT_STATUS_DEVICE_LOW_BATTERY),

    /* message command */
    STR(HC_EVENT_REQ_DEVICE_ADD),
    STR(HC_EVENT_REQ_DEVICE_ADD_STOP),
    STR(HC_EVENT_REQ_DEVICE_DELETE),
    STR(HC_EVENT_REQ_DEVICE_DELETE_STOP),
    STR(HC_EVENT_REQ_DEVICE_SET),
    STR(HC_EVENT_REQ_DEVICE_GET),
    STR(HC_EVENT_REQ_DEVICE_CFG_SET),
    STR(HC_EVENT_REQ_DEVICE_CFG_GET),
    STR(HC_EVENT_REQ_DEVICE_RESTART),
    STR(HC_EVENT_REQ_DEVICE_DEFAULT),
    STR(HC_EVENT_REQ_DEVICE_DETECT),
    STR(HC_EVENT_REQ_DEVICE_NA_DELETE),
    STR(HC_EVENT_REQ_ASSOICATION_SET),
    STR(HC_EVENT_REQ_ASSOICATION_REMOVE),
    STR(HC_EVENT_REQ_DRIVER_STATUS_QUERY),


    STR(HC_EVENT_REQ_VOIP_CALL),
    STR(HC_EVENT_REQ_DECT_UPGRADE),

    STR(HC_EVENT_REQ_DB_ACT),

    STR(HC_EVENT_REQ_DECT_PARAMETER_SET),
    STR(HC_EVENT_REQ_DECT_PARAMETER_GET),

    STR(HC_EVENT_EXT_IP_CHANGED),
    STR(HC_EVENT_EXT_FW_VERSION_CHANGED),
    STR(HC_EVENT_EXT_CAMERA_FW_REQUEST),
    STR(HC_EVENT_EXT_CAMERA_FW_UPGRADE),
    STR(HC_EVENT_EXT_CAMERA_FW_UPGRADE_DONE),
    STR(HC_EVENT_DEVICE_ADD_DELETE_DOING),
    STR(HC_EVENT_WLAN_CONFIGURE_CHANGED),
    STR(HC_EVENT_FACTORY_DEFAULT),
    STR(HC_EVENT_FACTORY_DEFAULT_DONE),
    STR(HC_EVNET_REQ_ARMMODE_SET),
    STR(HC_EVNET_EXT_ARMMODE_CHANGED),
    STR(HC_EVENT_EXT_FW_UPGRADE_LOCK),
    STR(HC_EVENT_EXT_FW_UPGRADE_LOCK_FAILED),
    STR(HC_EVENT_EXT_FW_UPGRADE_LOCK_DONE),
    STR(HC_EVENT_EXT_FW_UPGRADE_UNLOCK),
    STR(HC_EVENT_EXT_MODULE_UPGRADE),
    STR(HC_EVENT_EXT_MODULE_UPGRADE_SUCCESS),
    STR(HC_EVENT_EXT_MODULE_UPGRADE_FAILURE),
    STR(HC_EVENT_EXT_CHECK_NEIGHBOUR),

    /* just used by hc_msg */
    STR(HC_MSG_TYPE_CLIENT_INFO),

    STR(UNKNOWN),
};

static MAP_STR_S map_client_txt[] = {
    STR(APPLICATION),
    STR(DAEMON_ZWAVE),
    STR(DAEMON_ZIGBEE),
    STR(DAEMON_ULE),
    STR(DAEMON_LPRF),
    STR(DAEMON_CAMERA),
    STR(DAEMON_VOIP),
    STR(DAEMON_ALLJOYN),
    STR(DAEMON_ALL),
    STR(UNKNOWN),
};

/*
 * Note: the maximum length of text string is 31.
 */
static MAP_STR_S map_hcdev_txt[] = {
    {HC_DEVICE_TYPE_BINARYSWITCH,           "Binary Switch"},
    {HC_DEVICE_TYPE_DOUBLESWITCH,           "Double Switch"},
    {HC_DEVICE_TYPE_DIMMER,                 "Dimmer"},
    {HC_DEVICE_TYPE_CURTAIN,                "Curtain"},

    {HC_DEVICE_TYPE_BINARY_SENSOR,          "Binary Sensor"},
    {HC_DEVICE_TYPE_MULTILEVEL_SENSOR,      "Multi-Level Sensor"},

    {HC_DEVICE_TYPE_BATTERY,                "Battery"},

    {HC_DEVICE_TYPE_DOORLOCK,               "Doorlock"},
    {HC_DEVICE_TYPE_DOORLOCK_CONFIG,        "Doorlock Config"},
    {HC_DEVICE_TYPE_HSM100_CONFIG,          "HSM100 Config"},

    {HC_DEVICE_TYPE_THERMOSTAT,             "Thermostat"},

    {HC_DEVICE_TYPE_METER,                  "Meter"},

    {HC_DEVICE_TYPE_ASSOCIATION,            "Association"},
    {HC_DEVICE_TYPE_IPCAMERA,               "Camera"},
    {HC_DEVICE_TYPE_KEYFOB,                 "Keyfob"},
    {HC_DEVICE_TYPE_SIREN,                  "Siren"},
    {HC_DEVICE_TYPE_HANDSET,                "Handset"},
    {UNKNOWN,                               "Unknown"},
};

/*
 * Note: the maximum length of text string is 31.
 */
static MAP_STR_S map_binary_switch_txt[] = {
    {HC_BINARY_SWITCH_TYPE_RESERVED,            "Binary Switch"},
    {HC_BINARY_SWITCH_TYPE_WATER_VALVE,     "Water Valve"},
    {UNKNOWN,               "Binary Switch"}
};

/*
 * Note: the maximum length of text string is 31.
 */
static MAP_STR_S map_multilevel_switch_txt[] = {
    {HC_MULTILEVEL_SWITCH_TYPE_RESERVED,            "Multilevel Switch"},
    {HC_MULTILEVEL_SWITCH_TYPE_CURTAIN,     "Curtain"},
    {UNKNOWN,               "Multilevel Switch"}
};

/*
 * Note: the maximum length of text string is 31.
 */
static MAP_STR_S map_binary_sensor_txt[] = {
    {HC_BINARY_SENSOR_TYPE_RESERVED,            "Reserved Sensor"},
    {HC_BINARY_SENSOR_TYPE_GENERAL_PURPOSE,     "General Sensor"},
    {HC_BINARY_SENSOR_TYPE_SMOKE,               "Smoke Sensor"},
    {HC_BINARY_SENSOR_TYPE_CO,                  "CO Sensor"},
    {HC_BINARY_SENSOR_TYPE_CO2,                 "CO2 Sensor"},
    {HC_BINARY_SENSOR_TYPE_HEAT,                "Heat Sensor"},
    {HC_BINARY_SENSOR_TYPE_WATER,               "Water Sensor"},
    {HC_BINARY_SENSOR_TYPE_FREEZE,              "Freeze Sensor"},
    {HC_BINARY_SENSOR_TYPE_TAMPER,              "Temperature Sensor"},
    {HC_BINARY_SENSOR_TYPE_AUX,                 "AUX Sensor"},
    {HC_BINARY_SENSOR_TYPE_DOOR_WINDOW,         "Door Window Sensor"},
    {HC_BINARY_SENSOR_TYPE_TILT,                "TILT Sensor"},
    {HC_BINARY_SENSOR_TYPE_MOTION,              "Motion Sensor"},
    {HC_BINARY_SENSOR_TYPE_GLASS_BREAK,         "Glass Break Sensor"},
    {HC_BINARY_SENSOR_TYPE_RETURN_1ST_SENSOR,   "Return 1ST Sensor"},
    {UNKNOWN,                                   "Binary Sensor"}
};

/*
 * Note: the maximum length of text string is 31.
 */
static MAP_STR_S map_multi_sensor_txt[] = {
    {HC_MULTILEVEL_SENSOR_TYPE_RESERVED,                "Reserved Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE,         "Air Temperature Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_GENERAL_PURPOSE_VALUE,   "General Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_LUMINANCE,               "Luminance Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_POWER,                   "Power Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY,                "Humidity Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_VELOCITY,                "Velocity Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_DIRECTION,               "Direction Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_ATMOSPHERIC_PRESSURE,    "Atmospheric Pressure Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_BAROMETRIC_PRESSURE,     "Barometric Pressure Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_SOLAR_RADIATION,         "Solar Radiation Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_DEW_POINT,               "Dew Point Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_RAIN_RATE,               "Rain Rate Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_TIDE_LEVEL,              "Tide Level Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_WEIGHT,                  "Weight Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_VOLTAGE,                 "Voltage Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_CURRENT,                 "Current Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_CO2_LEVEL,               "CO2 Level Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_AIR_FLOW,                "Air Flow Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_TANK_CAPACITY,           "Tank Capacity Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_DISTANCE,                "Distance Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_ANGLE_POSITION,          "Angle Position Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_ROTATION,                "Rotation Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_WATER_TEMPERATURE,       "Water Temperature Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_SOIL_TEMPERATURE,        "Soit Temperature Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_SEISMIC_INTENSITY,       "Seismic Intensity Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_SEISMIC_MAGNITUDE,       "Seismic Magnitude Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_ULTRAVIOLET,             "Ultraviolet Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_RESISTIVITY,  "Electric Resistivity Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_CONDUCTIVITY, "Electric Conductivity Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_LOUDNESS,                "Loudness Sensor"},
    {HC_MULTILEVEL_SENSOR_TYPE_MOISTURE,                "Moisture Sensor"},
    {UNKNOWN,                                           "Multilevel Sensor"}
};

/*
 * Note: the maximum length of text string is 31.
 */
static MAP_STR_S map_meter_txt[] = {
    {HC_METER_TYPE_RESERVED,                        "Reserved Meter"},
    {HC_METER_TYPE_SINGLE_E_ELECTRIC,               "Single E Electric Meter"},
    {HC_METER_TYPE_GAS,                             "GAS Meter"},
    {HC_METER_TYPE_WATER,                           "Water Meter"},
    {HC_METER_TYPE_TWIN_E_ELECTRIC,                 "Twin E Electric Meter"},
    {HC_METER_TYPE_3P_SINGLE_DIRECT_ELECTRIC,       "3P Single Direct Electric Meter"},
    {HC_METER_TYPE_3P_SINGLE_ECT_ELECTRIC,          "3P Single ECT Meter"},
    {HC_METER_TYPE_1_PHASE_DIRECT_ELECTRIC,         "1 Phase Direct Electric Meter"},
    {HC_METER_TYPE_HEATING,                         "Heating Meter"},
    {HC_METER_TYPE_COOLING,                         "Cooling Meter"},
    {HC_METER_TYPE_COMBINED_HEATING_AND_COOLING,    "Heating & Cooling Meter"},
    {HC_METER_TYPE_ELECTRIC_SUB_METER,              "Electric Sub Meter"},
    {UNKNOWN,                                       "Meter"}
};



char * hc_map_msg_txt(int id)
{
    MAP_STR_S *map = &map_msg_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}

char * hc_map_client_txt(int id)
{
    MAP_STR_S *map = &map_client_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}

char * hc_map_hcdev_txt(int id)
{
    MAP_STR_S *map = &map_hcdev_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}

char * hc_map_bin_sensor_txt(int id)
{
    MAP_STR_S *map = &map_binary_sensor_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}


char * hc_map_multi_sensor_txt(int id)
{
    MAP_STR_S *map = &map_multi_sensor_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}


char * hc_map_meter_txt(int id)
{
    MAP_STR_S *map = &map_meter_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}


char * hc_map_binary_switch_txt(int id)
{
    MAP_STR_S *map = &map_binary_switch_txt[0];

    while (map->id != UNKNOWN && map->id != id)
    {
        map++;
    }

    return map->txt;
}

