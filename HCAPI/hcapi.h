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
 *   File   : hcapi.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _HCAPI_H_
#define _HCAPI_H_
#include <time.h>
#ifdef __cplusplus
extern "C"
{
#endif

//#pragma pack(1)

#define MAX_DEVICE_NAME_SIZE     64
#define MAX_DEVICE_LOCATION_SIZE     64
#define MAX_DEVICE_CONNSTATUS_SIZE  16

#define HC_BINARYSWITCH_OPEN    0xFF
#define HC_BINARYSWITCH_CLOSE   0x0
#define HC_BINARYSWITCH_WATER_VALVE_OPEN    0x00
#define HC_BINARYSWITCH_WATER_VALVE_CLOSE   0xFF

#define HC_DOORLOCK_OPEN    0x0
#define HC_DOORLOCK_CLOSE   0xFF

#define HC_THERMOSTAT_MODE_OFF    0x00
#define HC_THERMOSTAT_MODE_HEATING    0x01
#define HC_THERMOSTAT_MODE_COOLING    0x02
#define HC_THERMOSTAT_MODE_AUTO    0x03
#define HC_THERMOSTAT_MODE_AUXI_HEAT    0x04
#define HC_THERMOSTAT_MODE_RESUME    0x05
#define HC_THERMOSTAT_MODE_FAN_ONLY    0x06
#define HC_THERMOSTAT_MODE_FURNACE    0x07
#define HC_THERMOSTAT_MODE_DRY_AIR    0x08
#define HC_THERMOSTAT_MODE_MOIST_AIR    0x09
#define HC_THERMOSTAT_MODE_AUTO_CHANGEOVER    0x0a
#define HC_THERMOSTAT_MODE_ENERGY_SAVE_HEATING 0x0b
#define HC_THERMOSTAT_MODE_ENERGY_SAVE_COOLING    0x0c
#define HC_THERMOSTAT_MODE_AWAY_HEATING    0x0d
#define ARMMODE_BUFF_SIZE 2048
typedef enum
{
    HC_DB_ACT_OK = 0,
    HC_DB_ACT_FAIL,
    HC_DB_ACT_NO_CHANGE
}
HC_DB_ACT_RETVAL_E;

/* define common device value. */
typedef enum {
    HC_DEVICE_VALUE_SWITCH_OFF = 1,
    HC_DEVICE_VALUE_SWITCH_ON,
    HC_DEVICE_VALUE_CURTAIN_OFF,
    HC_DEVICE_VALUE_CURTAIN_ON,
    HC_DEVICE_VALUE_CURTAIN_STOP,
    HC_DEVICE_VALUE_MAX
}
HC_DEVICE_VALUE_E;

/* define the network type. */
typedef enum {
    HC_NETWORK_TYPE_APP = 0,
    HC_NETWORK_TYPE_ZWAVE = 1,
    HC_NETWORK_TYPE_ZIGBEE,
    HC_NETWORK_TYPE_ULE,
    HC_NETWORK_TYPE_LPRF,
    HC_NETWORK_TYPE_WIFI,
    HC_NETWORK_TYPE_WIREDCABLE,
    HC_NETWORK_TYPE_CAMERA,
    HC_NETWORK_TYPE_ALLJOYN,
    HC_NETWORK_TYPE_ALL,
    HC_NETWORK_TYPE_MAX
} HC_NETWORK_TYPE_E;

/* define the device type. */
typedef enum {
    HC_DEVICE_TYPE_BINARYSWITCH = 1,
    HC_DEVICE_TYPE_DOUBLESWITCH,
    HC_DEVICE_TYPE_DIMMER,
    HC_DEVICE_TYPE_CURTAIN,

    HC_DEVICE_TYPE_BINARY_SENSOR,
    HC_DEVICE_TYPE_MULTILEVEL_SENSOR,

    HC_DEVICE_TYPE_BATTERY,

    HC_DEVICE_TYPE_DOORLOCK,
    HC_DEVICE_TYPE_DOORLOCK_CONFIG,
    HC_DEVICE_TYPE_HSM100_CONFIG,

    HC_DEVICE_TYPE_THERMOSTAT,

    HC_DEVICE_TYPE_METER,

    HC_DEVICE_TYPE_ASSOCIATION,

    HC_DEVICE_TYPE_HANDSET,

    HC_DEVICE_TYPE_IPCAMERA,

    HC_DEVICE_TYPE_KEYFOB,

    HC_DEVICE_TYPE_SIREN,

    // external structure
    HC_DEVICE_TYPE_EXT_IP_CHANGED,
    HC_DEVICE_TYPE_EXT_CAMERA_UPGRADE,
    HC_DEVICE_TYPE_EXT_CAMERA_UPGRADE_DONE,
    HC_DEVICE_TYPE_EXT_CAMERA_FW_REQUEST,
    HC_DEVICE_TYPE_EXT_DECT_STATION,
    HC_DEVICE_TYPE_EXT_NEIGHBOUR,
	
    HC_DEVICE_TYPE_MAX
} HC_DEVICE_TYPE_E;

typedef enum {
    HC_BINARY_SWITCH_TYPE_RESERVED = 0,
    HC_BINARY_SWITCH_TYPE_WATER_VALVE,
    HC_BINARY_SWITCH_TYPE_THERMOSTAT,
    HC_BINARY_SWITCH_TYPE_MAX
}HC_BINARY_SWITCH_TYPE_E;

typedef enum {
    HC_MULTILEVEL_SWITCH_TYPE_RESERVED = 0,
    HC_MULTILEVEL_SWITCH_TYPE_CURTAIN,
    HC_MULTILEVEL_SWITCH_TYPE_MAX
}HC_MULTILEVEL_SWITCH_TYPE_E;

typedef enum {
    HC_BINARY_SENSOR_TYPE_RESERVED = 0,
    HC_BINARY_SENSOR_TYPE_GENERAL_PURPOSE,
    HC_BINARY_SENSOR_TYPE_SMOKE,
    HC_BINARY_SENSOR_TYPE_CO,
    HC_BINARY_SENSOR_TYPE_CO2,
    HC_BINARY_SENSOR_TYPE_HEAT,
    HC_BINARY_SENSOR_TYPE_WATER,
    HC_BINARY_SENSOR_TYPE_FREEZE,
    HC_BINARY_SENSOR_TYPE_TAMPER,
    HC_BINARY_SENSOR_TYPE_AUX,
    HC_BINARY_SENSOR_TYPE_DOOR_WINDOW,
    HC_BINARY_SENSOR_TYPE_TILT,
    HC_BINARY_SENSOR_TYPE_MOTION,
    HC_BINARY_SENSOR_TYPE_GLASS_BREAK,
    HC_BINARY_SENSOR_TYPE_RETURN_1ST_SENSOR,
    HC_BINARY_SENSOR_TYPE_MAX
} HC_BINARY_SENSOR_TYPE_E;

typedef enum {
    HC_MULTILEVEL_SENSOR_TYPE_RESERVED = 0,
    HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE,
    HC_MULTILEVEL_SENSOR_TYPE_GENERAL_PURPOSE_VALUE,
    HC_MULTILEVEL_SENSOR_TYPE_LUMINANCE,
    HC_MULTILEVEL_SENSOR_TYPE_POWER,
    HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY,
    HC_MULTILEVEL_SENSOR_TYPE_VELOCITY ,
    HC_MULTILEVEL_SENSOR_TYPE_DIRECTION,
    HC_MULTILEVEL_SENSOR_TYPE_ATMOSPHERIC_PRESSURE,
    HC_MULTILEVEL_SENSOR_TYPE_BAROMETRIC_PRESSURE,
    HC_MULTILEVEL_SENSOR_TYPE_SOLAR_RADIATION,
    HC_MULTILEVEL_SENSOR_TYPE_DEW_POINT,
    HC_MULTILEVEL_SENSOR_TYPE_RAIN_RATE,
    HC_MULTILEVEL_SENSOR_TYPE_TIDE_LEVEL,
    HC_MULTILEVEL_SENSOR_TYPE_WEIGHT,
    HC_MULTILEVEL_SENSOR_TYPE_VOLTAGE,
    HC_MULTILEVEL_SENSOR_TYPE_CURRENT,
    HC_MULTILEVEL_SENSOR_TYPE_CO2_LEVEL,
    HC_MULTILEVEL_SENSOR_TYPE_AIR_FLOW,
    HC_MULTILEVEL_SENSOR_TYPE_TANK_CAPACITY,
    HC_MULTILEVEL_SENSOR_TYPE_DISTANCE,
    HC_MULTILEVEL_SENSOR_TYPE_ANGLE_POSITION,
    HC_MULTILEVEL_SENSOR_TYPE_ROTATION,
    HC_MULTILEVEL_SENSOR_TYPE_WATER_TEMPERATURE,
    HC_MULTILEVEL_SENSOR_TYPE_SOIL_TEMPERATURE,
    HC_MULTILEVEL_SENSOR_TYPE_SEISMIC_INTENSITY,
    HC_MULTILEVEL_SENSOR_TYPE_SEISMIC_MAGNITUDE,
    HC_MULTILEVEL_SENSOR_TYPE_ULTRAVIOLET,
    HC_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_RESISTIVITY,
    HC_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_CONDUCTIVITY,
    HC_MULTILEVEL_SENSOR_TYPE_LOUDNESS,
    HC_MULTILEVEL_SENSOR_TYPE_MOISTURE,
    HC_MULTILEVEL_SENSOR_TYPE_MAX
} HC_MULTILEVEL_SENSOR_TYPE_E;

typedef enum
{
    HC_SCALE_AIR_TEMPERATURE_CELSIUS = 0x00,  // Celsius (C)
    HC_SCALE_AIR_TEMPERATURE_FAHRENHEIT = 0x01,  // Fahrenheit (F)
    HC_SCALE_AIR_TEMPERATURE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_AIR_TEMPERATURE;

typedef enum
{
    HC_SCALE_GENERAL_PURPOSE_PERCENTAGE = 0x00,  // Percentage value
    HC_SCALE_GENERAL_PURPOSE_DIMENSIONLESS = 0x01,  // Dimensionless value
    HC_SCALE_GENERAL_PURPOSE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_GENERAL_PURPOSE;

typedef enum
{
    HC_SCALE_LUMINANCE_PERCENTAGE = 0x00,  // Percentage value
    HC_SCALE_LUMINANCE_LUX = 0x01,  // Lux
    HC_SCALE_LUMINANCE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_LUMINANCE;

typedef enum
{
    HC_SCALE_POWER_W = 0x00,   // W
    HC_SCALE_POWER_BTU_P_H = 0x01,   // Btu/h
    HC_SCALE_POWER_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_POWER;

typedef enum
{
    HC_SCALE_HUMIDITY_PERCENTAGE = 0x00,  // Percentage value
    HC_SCALE_HUMIDITY_ABSOLUTE = 0x01,   // (g/m3) ¨C (v5)
    HC_SCALE_HUMIDITY_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_HUMIDITY;

typedef enum
{
    HC_SCALE_VELOCITY_M_P_S = 0x00, // m/s
    HC_SCALE_VELOCITY_MPH = 0x01,   // Mph
    HC_SCALE_VELOCITY_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_VELOCITY;

typedef enum
{
    HC_SCALE_DIRECTION_DEGRESS = 0x00, // 0 to 360 degrees.  0 = no wind, 90 = east, 180 = south, 270 = west, and 360 = north
    HC_SCALE_DIRECTION_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_DIRECTION;

typedef enum
{
    HC_SCALE_ATMOSPHERIC_PRESSURE_KPA = 0x00,  // kPa (kilopascal)
    HC_SCALE_ATMOSPHERIC_PRESSURE_INCHES_OF_MERCURY = 0x01,   // Inches of Mercury
    HC_SCALE_ATMOSPHERIC_PRESSURE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_ATMOSPHERIC_PRESSURE;

typedef enum
{
    HC_SCALE_BAROMETRIC_PRESSURE_KPA = 0x00,  // kPa (kilopascal)
    HC_SCALE_BAROMETRIC_PRESSURE_INCHES_OF_MERCURY = 0x01,   // Inches of Mercury
    HC_SCALE_BAROMETRIC_PRESSURE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_BAROMETRIC_PRESSURE;

typedef enum
{
    HC_SCALE_SOLAR_RADIATION_W_P_M2 = 0x00,  // W/m2
    HC_SCALE_SOLAR_RADIATION_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_SOLAR_RADIATION;

typedef enum
{
    HC_SCALE_DEW_POINT_CELSIUS = 0x00,  // Celsius (C)
    HC_SCALE_DEW_POINT_FAHRENHEIT = 0x01,   // Fahrenheit (F)
    HC_SCALE_DEW_POINT_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_DEW_POINT;

typedef enum
{
    HC_SCALE_RAIN_RATE_MM_P_H = 0x00,  // mm/h
    HC_SCALE_RAIN_RATE_IN_P_H = 0x01,   // in/h
    HC_SCALE_RAIN_RATE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_RAIN_RATE;

typedef enum
{
    HC_SCALE_TIDE_LEVEL_M = 0x00,  // m
    HC_SCALE_TIDE_LEVEL_FEET = 0x01,   // Feet
    HC_SCALE_TIDE_LEVEL_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_TIDE_LEVEL;

typedef enum
{
    HC_SCALE_WEIGHT_KG = 0x00,  // Kg
    HC_SCALE_WEIGHT_POUNDS = 0x01,   // pounds
    HC_SCALE_WEIGHT_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_WEIGHT;

typedef enum
{
    HC_SCALE_VOLTAGE_V = 0x00,  // V
    HC_SCALE_VOLTAGE_MV = 0x01,   // mV
    HC_SCALE_VOLTAGE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_VOLTAGE;

typedef enum
{
    HC_SCALE_CURRENT_A = 0x00,  // A
    HC_SCALE_CURRENT_MA = 0x01,   // mA
    HC_SCALE_CURRENT_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_CURRENT;

typedef enum
{
    HC_SCALE_CO2_LEVEL_PPM = 0x00,  // Ppm
    HC_SCALE_CO2_LEVEL_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_CO2_LEVEL;

typedef enum
{
    HC_SCALE_AIR_FLOW_M3_P_H= 0x00,  // m3/h
    HC_SCALE_AIR_FLOW_CFM = 0x01,   // cfm (cubic feet per minute)
    HC_SCALE_AIR_FLOW_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_AIR_FLOW;

typedef enum
{
    HC_SCALE_TANK_CAPACITY_L= 0x00,  // l (liter)
    HC_SCALE_TANK_CAPACITY_CBM = 0x01,   // cbm (cubic meter)
    HC_SCALE_TANK_CAPACITY_GALLONS = 0x02,   // gallons
    HC_SCALE_TANK_CAPACITY_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_TANK_CAPACITY;

typedef enum
{
    HC_SCALE_DISTANCE_M= 0x00,  // M
    HC_SCALE_DISTANCE_CM = 0x01,   // Cm
    HC_SCALE_DISTANCE_FEET = 0x02,   // Feet
    HC_SCALE_DISTANCE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_DISTANCE;

typedef enum
{
    HC_SCALE_ANGLE_POSITION_PERCENTAGE= 0x00,  // Percentage value
    HC_SCALE_ANGLE_POSITION_DEGREES_NORTH = 0x01,   // Degrees relative to north pole of standing eye view
    HC_SCALE_ANGLE_POSITION_DEGREES_SOUTH = 0x02,   // Degrees relative to south pole of standing eye view
    HC_SCALE_ANGLE_POSITION_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_ANGLE_POSITION;

typedef enum
{
    HC_SCALE_ROTATION_RPM= 0x00,  // rpm (revolutions per minute)
    HC_SCALE_ROTATION_HZ = 0x01,   // Hz (Hertz)
    HC_SCALE_ROTATION_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_ROTATION;

typedef enum
{
    HC_SCALE_WATER_TEMPERATURE_CELSIUS= 0x00,  // Celsius (C)
    HC_SCALE_WATER_TEMPERATURE_FAHRENHELT = 0x01,   // Fahrenheit (F)
    HC_SCALE_WATER_TEMPERATURE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_WATER_TEMPERATURE;

typedef enum
{
    HC_SCALE_SOIL_TEMPERATURE_CELSIUS= 0x00,  // Celsius (C)
    HC_SCALE_SOIL_TEMPERATURE_FAHRENHELT = 0x01,   // Fahrenheit (F)
    HC_SCALE_SOIL_TEMPERATURE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_SOIL_TEMPERATURE;

typedef enum
{
    HC_SCALE_SEISMIC_INTENSITY_MERCALLI= 0x00,  // Mercalli
    HC_SCALE_SEISMIC_INTENSITY_EUROPEAN_MACROSEISMIC = 0x01,   // European Macroseismic
    HC_SCALE_SEISMIC_INTENSITY_LIEDU= 0x02,  // Liedu
    HC_SCALE_SEISMIC_INTENSITY_SHINDO = 0x03,   // Shindo
    HC_SCALE_SEISMIC_INTENSITY_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_SEISMIC_INTENSITY;

typedef enum
{
    HC_SCALE_SEISMIC_MAGNITUDE_ML= 0x00,  // Local (ML)
    HC_SCALE_SEISMIC_MAGNITUDE_MW = 0x01,   // Moment (MW)
    HC_SCALE_SEISMIC_MAGNITUDE_MS= 0x02,  // Surface wave (MS)
    HC_SCALE_SEISMIC_MAGNITUDE_MB = 0x03,   // Body wave (MB)
    HC_SCALE_SEISMIC_MAGNITUDE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_SEISMIC_MAGNITUDE;

typedef enum
{
    HC_SCALE_ULTRAVIOLET_UV= 0x00,  // UV index
    HC_SCALE_ULTRAVIOLET_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_ULTRAVIOLET;

typedef enum
{
    HC_SCALE_ELECTRICAL_RESISTIVITY_OHM= 0x00,  // ohm metre (¦¸m)
    HC_SCALE_ELECTRICAL_RESISTIVITY_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_ELECTRICAL_RESISTIVITY;

typedef enum
{
    HC_SCALE_ELECTRICAL_CONDUCTIVITY_SIEMENS_PER_METRE = 0x00,  // siemens per metre (S.M - 1)
    HC_SCALE_ELECTRICAL_CONDUCTIVITY_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_ELECTRICAL_CONDUCTIVITY;

typedef enum
{
    HC_SCALE_LOUDNESS_DB= 0x00,  // Absolute loudness (dB)
    HC_SCALE_LOUDNESS_DBA = 0x01,   // A-weighted decibels (dBA)
    HC_SCALE_LOUDNESS_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_LOUDNESS;

typedef enum
{
    HC_SCALE_MOISTURE_PERCENTAGE= 0x00,  // Percentage value
    HC_SCALE_MOISTURE_M3_P_M3 = 0x01,   // Volume water content (m3/m3)
    HC_SCALE_MOISTURE_IMPEDANCE= 0x02,  // Impedance (k¦¸)
    HC_SCALE_MOISTURE_AW = 0x03,   // Water activity (aw)
    HC_SCALE_MOISTURE_RESERVED
}HC_MULTILEVEL_SENSOR_SCALE_MOISTURE;

typedef enum {
    HC_METER_TYPE_RESERVED = 0,
    HC_METER_TYPE_SINGLE_E_ELECTRIC,
    HC_METER_TYPE_GAS,
    HC_METER_TYPE_WATER,
    HC_METER_TYPE_TWIN_E_ELECTRIC,
    HC_METER_TYPE_3P_SINGLE_DIRECT_ELECTRIC,
    HC_METER_TYPE_3P_SINGLE_ECT_ELECTRIC,
    HC_METER_TYPE_1_PHASE_DIRECT_ELECTRIC,
    HC_METER_TYPE_HEATING,
    HC_METER_TYPE_COOLING,
    HC_METER_TYPE_COMBINED_HEATING_AND_COOLING,
    HC_METER_TYPE_ELECTRIC_SUB_METER,
    HC_METER_TYPE_MAX
} HC_METER_TYPE_E;


typedef enum
{
    HC_SCALE_ELECTRIC_KWH = 0x00, //kWh
    HC_SCALE_ELECTRIC_KVAH = 0x01, //kVAh
    HC_SCALE_ELECTRIC_W = 0x02, //W
    HC_SCALE_ELECTRIC_PULSE_COUNT = 0x03, //Pulse count
    HC_SCALE_ELECTRIC_V = 0x04, //Voltage (V)
    HC_SCALE_ELECTRIC_A = 0x05, //Amperes (A)
    HC_SCALE_ELECTRIC_POWER_FACTOR = 0x06, //Power Factor (%)
    HC_SCALE_ELECTRIC_RESERVED
}HC_METER_SCALE_ELECTRIC;

typedef enum
{
    HC_SCALE_GAS_CUBIC_METER = 0x00, //Cubic meter
    HC_SCALE_GAS_CUBIC_FEET = 0x01, //Cubic feet
    HC_SCALE_GAS_PULSE_COUNT = 0x03, //Pulse count
    HC_SCALE_GAS_WATER_RESERVED
}HC_METER_SCALE_GAS_WATER;

typedef enum
{
    HC_SCALE_WATER_CUBIC_METER = 0x00, //Cubic meter
    HC_SCALE_WATER_CUBIC_FEET = 0x01, //Cubic feet
    HC_SCALE_WATER_US_GALLON = 0x02, //US gallon
    HC_SCALE_WATER_PULSE_COUNT = 0x03, //Pulse count
    HC_SCALE_WATER_WATER_RESERVED
}HC_METER_SCALE_WATER;

/* define the device type. */
typedef enum {
    HC_DECT_PARAMETER_TYPE_DECTTYPE = 1,
    HC_DECT_PARAMETER_TYPE_BASENAME,
    HC_DECT_PARAMETER_TYPE_PINCODE,
    HC_DECT_PARAMETER_TYPE_MAX
} HC_DECT_PARAMETER_TYPE_E;

/* define warning mode. */
typedef enum {
    HC_WARNING_MODE_STOP = 0,
    HC_WARNING_MODE_BURGLAR,
    HC_WARNING_MODE_FIRE,
    HC_WARNING_MODE_EMERGENCY,
    HC_WARNING_MODE_MAX
} HC_WARNING_MODE_E;

typedef enum {
    HC_ARM = 1,
    HC_DISARM,
    HC_STAY,
    HC_ARMMODE_MAX
}
HC_ARMMODE_E;

/* define the function return value. */
typedef enum {
    HC_RET_SUCCESS = 0,
    HC_RET_FAILURE,
    HC_RET_TIMEOUT,
    HC_RET_INVALID_ARGUMENTS,
    HC_RET_NO_MEMORY,
    HC_RET_SEND_FAILURE,
    HC_RET_RECV_FAILURE,
    HC_RET_OPERATION_FAILURE,
    HC_RET_ADDING_MODE,
    HC_RET_DELETING_MODE,
    HC_RET_NORMAL_MODE,
    HC_RET_DEVICE_HAS_REMOVED,
    HC_RET_DEVICE_CONNECTED,
    HC_RET_DEVICE_DISCONNECTED,
    HC_RET_FUNCTION_NOT_SUPPORT,
    HC_RET_UNEXPECTED_EVENT,
    HC_RET_DB_OPERATION_FAILURE,
    HC_RET_LOCK_FAILURE,
    HC_RET_MAX
} HC_RETVAL_E;

/*
 * When add new event, please also update hc_util.h at the same time.
 * Please sync them with same sequence will be had good trace from log files.
 */
typedef enum {
    HC_EVENT_RESP_DEVICE_ADDED_SUCCESS = 1,
    HC_EVENT_RESP_DEVICE_ADDED_FAILURE,
    HC_EVENT_RESP_DEVICE_ADD_MODE,
    HC_EVENT_RESP_DEVICE_ADD_COMPLETE,
    HC_EVENT_RESP_DEVICE_ADD_TIMEOUT,
    HC_EVENT_RESP_DEVICE_ADD_STOP_SUCCESS,
    HC_EVENT_RESP_DEVICE_ADD_STOP_FAILURE,
    HC_EVENT_RESP_DEVICE_DELETED_SUCCESS,
    HC_EVENT_RESP_DEVICE_DELETED_FAILURE,
    HC_EVENT_RESP_DEVICE_DELETE_MODE,
    HC_EVENT_RESP_DEVICE_DELETE_COMPLETE,
    HC_EVENT_RESP_DEVICE_DELETE_TIMEOUT,
    HC_EVENT_RESP_DEVICE_DELETE_STOP_SUCCESS,
    HC_EVENT_RESP_DEVICE_DELETE_STOP_FAILURE,
    HC_EVENT_RESP_DEVICE_HAS_BEEN_REMOVED,
    HC_EVENT_RESP_DEVICE_RESET_SUCCESS,
    HC_EVENT_RESP_DEVICE_RESET_FAILURE,
    HC_EVENT_RESP_DEVICE_DEFAULT_SUCCESS,
    HC_EVENT_RESP_DEVICE_DEFAULT_FAILURE,
    HC_EVENT_RESP_DEVICE_CONNECTED,
    HC_EVENT_RESP_DEVICE_DISCONNECTED,
    HC_EVENT_RESP_DEVICE_DETECT_FAILURE,
    HC_EVENT_RESP_DEVICE_NA_DELETE_SUCCESS,
    HC_EVENT_RESP_DEVICE_NA_DELETE_FAILURE,
    HC_EVENT_RESP_DEVICE_SET_SUCCESS,
    HC_EVENT_RESP_DEVICE_SET_FAILURE,
    HC_EVENT_RESP_DEVICE_GET_SUCCESS,
    HC_EVENT_RESP_DEVICE_GET_FAILURE,

    HC_EVENT_RESP_DEVICE_SET_CFG_SUCCESS,
    HC_EVENT_RESP_DEVICE_SET_CFG_FAILURE,
    HC_EVENT_RESP_DEVICE_GET_CFG_SUCCESS,
    HC_EVENT_RESP_DEVICE_GET_CFG_FAILURE,

    HC_EVENT_RESP_ASSOICATION_SET_SUCCESS,
    HC_EVENT_RESP_ASSOICATION_SET_FAILURE,
    HC_EVENT_RESP_ASSOICATION_REMOVE_SUCCESS,
    HC_EVENT_RESP_ASSOICATION_REMOVE_FAILURE,

    HC_EVENT_RESP_FUNCTION_NOT_SUPPORT,
    HC_EVENT_RESP_DB_OPERATION_FAILURE,

    HC_EVENT_RESP_VOIP_CALL,

    HC_EVENT_RESP_DECT_UPGRADE,

    HC_EVENT_RESP_DECT_PARAMETER_SET_SUCCESS,
    HC_EVENT_RESP_DECT_PARAMETER_SET_FAILURE,
    HC_EVENT_RESP_DECT_PARAMETER_GET_SUCCESS,
    HC_EVENT_RESP_DECT_PARAMETER_GET_FAILURE,

    HC_EVENT_RESP_DEVICE_NORMAL_MODE,

    HC_EVENT_STATUS_DEVICE_STATUS_CHANGED,
    HC_EVENT_STATUS_DEVICE_STATUS_CHANGED_DB_FAILURE,
    HC_EVENT_STATUS_DEVICE_LOW_BATTERY,

    HC_EVENT_REQ_DEVICE_ADD,
    HC_EVENT_REQ_DEVICE_ADD_STOP,
    HC_EVENT_REQ_DEVICE_DELETE,
    HC_EVENT_REQ_DEVICE_DELETE_STOP,
    HC_EVENT_REQ_DEVICE_SET,
    HC_EVENT_REQ_DEVICE_GET,
    HC_EVENT_REQ_DEVICE_CFG_SET,
    HC_EVENT_REQ_DEVICE_CFG_GET,
    HC_EVENT_REQ_DEVICE_RESTART,    // restart zwave chip.
    HC_EVENT_REQ_DEVICE_DEFAULT,    // set configuration to default.
    HC_EVENT_REQ_DEVICE_DETECT,
    HC_EVENT_REQ_DEVICE_NA_DELETE,  // delete N/A device
    HC_EVENT_REQ_ASSOICATION_SET,
    HC_EVENT_REQ_ASSOICATION_REMOVE,
    HC_EVENT_REQ_DRIVER_STATUS_QUERY,

    HC_EVENT_REQ_VOIP_CALL,

    HC_EVENT_REQ_DECT_UPGRADE,

    HC_EVENT_REQ_DB_ACT,

    HC_EVENT_REQ_DECT_PARAMETER_SET,
    HC_EVENT_REQ_DECT_PARAMETER_GET,

    //HC_EVENT_CAMERA_TRIGGER,

    // external event
    HC_EVENT_EXT_IP_CHANGED,
    HC_EVENT_EXT_FW_VERSION_CHANGED,
    HC_EVENT_EXT_CAMERA_FW_REQUEST,
    HC_EVENT_EXT_CAMERA_FW_UPGRADE,
    HC_EVENT_EXT_CAMERA_FW_UPGRADE_DONE,
    HC_EVENT_DEVICE_ADD_DELETE_DOING,
    HC_EVENT_WLAN_CONFIGURE_CHANGED,
    HC_EVENT_FACTORY_DEFAULT,
    HC_EVENT_FACTORY_DEFAULT_DONE,

    HC_EVNET_REQ_ARMMODE_SET,
    HC_EVNET_EXT_ARMMODE_CHANGED,
    HC_EVENT_EXT_FW_UPGRADE_LOCK,
    HC_EVENT_EXT_FW_UPGRADE_LOCK_FAILED,
    HC_EVENT_EXT_FW_UPGRADE_LOCK_DONE,
    HC_EVENT_EXT_FW_UPGRADE_UNLOCK,

    HC_EVENT_EXT_MODULE_UPGRADE,
    HC_EVENT_EXT_MODULE_UPGRADE_SUCCESS,
    HC_EVENT_EXT_MODULE_UPGRADE_FAILURE,
    HC_EVENT_EXT_CHECK_NEIGHBOUR,

    HC_EVENT_MAX
} HC_EVENT_TYPE_E;

typedef enum
{
    DB_ACT_NONE = 0,
    DB_ACT_ADD_DEV,
    DB_ACT_GET_DEV_BY_ID,
    DB_ACT_GET_DEV_ALL,
    DB_ACT_GET_DEV_NUM_ALL,
    DB_ACT_GET_DEV_BY_DEV_TYPE,
    DB_ACT_GET_DEV_NUM_BY_DEV_TYPE,
    DB_ACT_GET_DEV_BY_NETWORK_TYPE,
    DB_ACT_GET_DEV_NUM_BY_NETWORK_TYPE,
    DB_ACT_SET_DEV,
    DB_ACT_DEL_DEV,
    DB_ACT_DEL_DEV_ALL,     //11

    DB_ACT_ADD_DEV_LOG,
    DB_ACT_GET_DEV_LOG,
    DB_ACT_GET_DEV_LOG_BY_DEV_ID,
    DB_ACT_GET_DEV_LOG_BY_DEV_TYPE,
    DB_ACT_GET_DEV_LOG_BY_NETWORK_TYPE,
    DB_ACT_GET_DEV_LOG_BY_TIME,
    DB_ACT_GET_DEV_LOG_BY_ALARM_TYPE,
    DB_ACT_GET_DEV_LOG_NUM,
    DB_ACT_GET_DEV_LOG_NUM_BY_DEV_ID,
    DB_ACT_GET_DEV_LOG_NUM_BY_NETWORK_TYPE,
    DB_ACT_GET_DEV_LOG_NUM_BY_DEV_TYPE,
    DB_ACT_GET_DEV_LOG_NUM_BY_TIME,
    DB_ACT_GET_DEV_LOG_NUM_BY_ALARM_TYPE,
    DB_ACT_DEL_DEV_LOG_ALL,
    DB_ACT_DEL_DEV_LOG_BY_DEV_ID,  //26

    DB_SET_DEV_EXT,
    DB_ADD_DEV_EXT,
    DB_GET_DEV_EXT_BY_DEV_ID,
    DB_DEL_DEV_EXT_BY_DEV_ID,   //30

    DB_ACT_ADD_DEV_NAME,
    DB_ACT_GET_DEV_NAME,
    DB_ACT_SET_DEV_NAME,
    DB_ACT_DEL_DEV_NAME,        //34

    DB_ACT_ATTR_ADD,
    DB_ACT_ATTR_GET,
    DB_ACT_ATTR_GET_BY_DEV_ID,
    DB_ACT_ATTR_GET_NUM_BY_DEV_ID,
    DB_ACT_ATTR_SET,
    DB_ACT_ATTR_DEL,
    DB_ACT_ATTR_DEL_BY_DEV_ID,  //41

    DB_ACT_SCENE_ADD,
    DB_ACT_SCENE_GET,
    DB_ACT_SCENE_ALL,
    DB_ACT_SCENE_ALL_NUM,
    DB_ACT_SCENE_SET,
    DB_ACT_SCENE_DEL,
    DB_ACT_SCENE_DEL_ALL,   //48

    DB_ACT_ADD_USER_LOG,
    DB_ACT_GET_USER_LOG,
    DB_ACT_GET_USER_LOG_NUM,    //51

    DB_ACT_LOCATION_ADD,
    DB_ACT_LOCATION_GET,
    DB_ACT_LOCATION_ALL,
    DB_ACT_LOCATION_ALL_NUM,
    DB_ACT_LOCATION_SET,
    DB_ACT_LOCATION_DEL,
    DB_ACT_LOCATION_DEL_ALL,        //58

    DB_ACT_GET_CONF,
    DB_ACT_SET_CONF,        //60

    DB_ACT_TYPE_MAX
} DB_ACT_TYPE_E;

typedef struct {
    HC_ARMMODE_E mode;
    char devicesid[ARMMODE_BUFF_SIZE];
} HC_ARM_MODE;

/* an information set of home control device. */
typedef struct {
    int value; // 0: off; 1: on
    int type;
} HC_DEVICE_BINARYSWITCH_S, HC_DEVICE_DOUBLESWITCH_S;

typedef struct {
    int value;
} HC_DEVICE_DIMMER_S;

typedef struct {
    int value;  // 0x00 - 0x63, oxff or 0xfe(stop)
} HC_DEVICE_CURTAIN_S;

typedef struct {
    int sensor_type;
    int value;  // 0, idle; 0xff, event detect
} HC_DEVICE_BINARY_SENSOR_S;

typedef struct {
    int sensor_type;
    int scale;
    double value;
} HC_DEVICE_MULTILEVEL_SENSOR_S;

typedef struct {
    int battery_level;   // 0x00 - 0x64. The value 0xFF indicates a battery low warning.
    unsigned int interval_time;  // 0x00 - 0x00ffffff
} HC_DEVICE_BATTERY_S;

typedef struct {
    unsigned char value;  // 0 silent; otherwise warning
} HC_DEVICE_MOTION_SENSOR_S;

typedef struct {
    double value;
} HC_DEVICE_WINDOW_SENSOR_S;

typedef struct {
    double value;     // default scale: centigrade
} HC_DEVICE_TEMPERATURE_SENSOR_S;

typedef struct {
    double value;     // default scale: percentage
} HC_DEVICE_LUMINANCE_SENSOR_S;

typedef struct {
    int doorLockMode;                 /**/
    int doorHandlesMode;                  /* masked byte   0-3 bits(Inside Door Handles Mode); 4-7 bits(Outside Door Handles Mode)*/
    int doorCondition;                /*masked byte 0-3 bits effective*/
    int lockTimeoutMinutes;           /**/
    int lockTimeoutSeconds;           /**/
} HC_DEVICE_DOORLOCK_S;

typedef struct {
    int      operationType;                /**/
    int      doorHandlesMode;                  /* masked byte */
    int      lockTimeoutMinutes;           /**/
    int      lockTimeoutSeconds;           /**/
} HC_DEVICE_DOORLOCK_CONFIG_S;

#define HC_HSM100_CONFIG_TYPE_SENSITIVITY           1  /*value range: 0--255; default: 200*/
#define HC_HSM100_CONFIGURATION_TYPE_ON_TIME        2  /*value range: 1--127; default: 1*/
#define HC_HSM100_CONFIGURATION_TYPE_STAY_AWAKE     5  /*value range: 0, -1; default: 0*/

typedef struct {
    int parameterNumber;
    int bDefault; // 0; value available; 1: use default value
    long value;
} HC_DEVICE_HSM100_CONFIG_S;

typedef struct {
    int mode;
    double value;
    double heat_value;
    double cool_value;
    double energe_save_heat_value;
    double energe_save_cool_value;
} HC_DEVICE_THERMOSTAT_S;

typedef struct {
    int meter_type;
    int scale;
    int rate_type;
    double value;
    int delta_time;
    double previous_value;
} HC_DEVICE_METER_S, HC_DEVICE_POWER_METER_S, HC_DEVICE_GAS_METER_S, HC_DEVICE_WATER_METER_S;


typedef struct {
    int connection;
    unsigned char ipaddress[16];
    unsigned char onvifuid[64];
    unsigned char streamurl[128];
    unsigned char remoteurl[128];
    unsigned char destPath[128];
    unsigned char triggerTime[64];
    unsigned char label[8];
    char name[MAX_DEVICE_NAME_SIZE];
    char mac[32];
    char ext[32];
    char fwversion[64];
    char modelname[64];
} HC_DEVICE_CAMERA_S;

typedef struct {
    char item_word[16];
} HC_DEVICE_KEYFOB_S;

typedef struct {
    HC_WARNING_MODE_E warning_mode;
    int strobe; // on: 1, off: 0
} HC_DEVICE_SIREN_S;

typedef struct {
    int srcPhyId;
    int dstPhyId;
} HC_DEVICE_ASSOCIATION_S;

typedef struct {
    int  port;
    char ip_addr[16];
    char wan_if_name[16];
} HC_DEVICE_EXT_IP_CHANGED_S;

typedef struct {
    HC_DECT_PARAMETER_TYPE_E type;
    union {
        int dect_type;
        char pin_code[16];
        char base_station_name[32];
    } u;

} HC_DEVICE_EXT_DECT_STATION_S;
typedef struct {
    int status; /* 0:on-hook, 1:ring, 2:off-hook */
    union {
        int value;  /*0:ring, 1:stop ring */
        char action[256];
    } u;
} HC_DEVICE_HANDSET_U;

typedef struct {
    int  device_type; // CTRL_EXTERNAL_DEVICE_IPCAM_TYPE
    char firmware_version[64];
    int is_new_image;
    char file_path[256];
} HC_DEVICE_EXT_CAMERA_UPGRADE_S;

typedef struct {
    int  device_type; // CTRL_EXTERNAL_DEVICE_IPCAM_TYPE
    int upgrade_status;  // 0 - success, 1 - failure
    char file_path[256];
} HC_DEVICE_EXT_CAMERA_UPGRADE_DONE_S;

typedef struct {
    int  device_type; // CTRL_EXTERNAL_DEVICE_IPCAM_TYPE
} HC_DEVICE_EXT_CAMERA_FW_REQUEST_S;

typedef struct {
    unsigned char ip[16];
    unsigned char mac[32];
    int nud;
} HC_DEVICE_EXT_NETWORK_DETECTION_S;

#define LOG_BUF_LEN 192
#define LOG_MAX_NUM 30
#define ARMMODE_ID_LENGTH 32
#define SQL_LOG_LEN 256
#define SQL_SCENE_LEN 512
#define DEVICE_NAME_LEN MAX_DEVICE_NAME_SIZE
#define ARMMODE_DEV_ID 23456
#define LOG_BUF_LEN 192
#define TIME_STR_LEN 24
#define ATTR_NAME_LEN 32
#define ATTR_VALUE_LEN 1024
#define SCENE_ATTR_LEN 1024
#define LOG_EXT_LEN 256
#define SCENE_ID_LEN 64
#define CONF_NAME_LEN 32
#define CONF_VALUE_LEN 128
#define LOCATION_ATTR_LEN 64
#define LOCATION_ID_LEN 64
#define CONF_NAME_LEN 32
#define CONF_VALUE_LEN 128

typedef struct
{
    unsigned int dev_id;
    int dev_type;
    int network_type;
    long start_time;
    long end_time;
    int alarm_type;
    int index;
    int log_num;
} HC_DEVICE_EXT_GET_LOG_S;

typedef struct
{
    int value;
    char id[ARMMODE_ID_LENGTH];
} HC_DEVICE_EXT_ARMMODE_S;

typedef struct
{
    int value;
} HC_RECORD_NUM_S;

typedef struct
{
    unsigned int dev_id;
    char dev_name[DEVICE_NAME_LEN];
    HC_EVENT_TYPE_E event_type;
    HC_NETWORK_TYPE_E network_type;
    HC_DEVICE_TYPE_E dev_type;
    char log[SQL_LOG_LEN]; // ipaddr
    long log_time;
    char log_time_str[TIME_STR_LEN];
    int alarm_type;
    char log_name[SQL_SCENE_LEN]; // video path
    char ext_1[LOG_EXT_LEN];    // camera name, camera1,camera2
    char ext_2[LOG_EXT_LEN];
} HC_DEVICE_EXT_LOG_S;

typedef struct
{
    long start_time;
    long end_time;
    int index;
    int log_num;
} HC_DEVICE_EXT_GET_USER_LOG_S;

typedef struct
{
    char user_name[DEVICE_NAME_LEN];
    int operation;
    int result;
    char log[SQL_LOG_LEN];
    long log_time;
    char log_time_str[TIME_STR_LEN];
    char log_name[SQL_SCENE_LEN];
    char ext_1[LOG_EXT_LEN];
    char ext_2[LOG_EXT_LEN];
} HC_DEVICE_EXT_USER_LOG_S;

typedef struct
{
    unsigned int dev_id;
    char dev_name[DEVICE_NAME_LEN];
} HC_DEVICE_EXT_NAME_S;

typedef struct
{
    unsigned int dev_id;
    char attr_name[ATTR_NAME_LEN];
    char attr_value[ATTR_VALUE_LEN];
} HC_DEVICE_EXT_ATTR_S;

typedef struct
{
    char scene_id[SCENE_ID_LEN];
    char scene_attr[SCENE_ATTR_LEN];
} HC_DEVICE_EXT_SCENE_S;

typedef struct
{
    char location_id[LOCATION_ID_LEN];
    char location_attr[LOCATION_ATTR_LEN];
} HC_DEVICE_EXT_LOCATION_S;

typedef struct
{
    HC_NETWORK_TYPE_E network_type;
    HC_DEVICE_TYPE_E dev_type;
} HC_DEVICE_EXT_GET_DEV_S;

typedef struct
{
    char conf_name[CONF_NAME_LEN];
    char conf_value[CONF_VALUE_LEN];
} HC_DEVICE_EXT_CONF_S;


typedef union {
    HC_DEVICE_EXT_LOG_S add_log;
    HC_DEVICE_EXT_GET_LOG_S get_log;
    HC_DEVICE_EXT_ARMMODE_S armmode;
    HC_DEVICE_EXT_NAME_S dev_name;
    HC_RECORD_NUM_S record_num;
    HC_DEVICE_EXT_ATTR_S attr;
    HC_DEVICE_EXT_SCENE_S scene;
    HC_DEVICE_EXT_USER_LOG_S add_user_log;
    HC_DEVICE_EXT_GET_LOG_S get_user_log;
    HC_DEVICE_EXT_GET_DEV_S get_dev;
    HC_DEVICE_EXT_LOCATION_S location;
    HC_DEVICE_EXT_CONF_S conf;
    unsigned char stuff[0];
} HC_DEVICE_EXT_U;



typedef union {
    HC_DEVICE_BINARYSWITCH_S binaryswitch;
    HC_DEVICE_DOUBLESWITCH_S doubleswitch;
    HC_DEVICE_DIMMER_S  dimmer;
    HC_DEVICE_CURTAIN_S curtain;
    HC_DEVICE_BINARY_SENSOR_S binary_sensor;
    HC_DEVICE_MULTILEVEL_SENSOR_S multilevel_sensor;
    
    HC_DEVICE_BATTERY_S battery;
    HC_DEVICE_DOORLOCK_S doorlock;
    HC_DEVICE_THERMOSTAT_S thermostat;
    HC_DEVICE_METER_S meter;
    
    HC_DEVICE_ASSOCIATION_S association;

    HC_DEVICE_HANDSET_U handset;
    HC_DEVICE_CAMERA_S camera;
    HC_DEVICE_KEYFOB_S keyfob;
    HC_DEVICE_SIREN_S siren;
    HC_DEVICE_EXT_U ext_u;
    // configuration
    HC_DEVICE_HSM100_CONFIG_S hsm100_config;
    HC_DEVICE_DOORLOCK_CONFIG_S doorlock_config;
    // external structure
    HC_DEVICE_EXT_IP_CHANGED_S ext_ip_changed;
    HC_DEVICE_EXT_CAMERA_UPGRADE_S ext_camera_upgrade;
    HC_DEVICE_EXT_CAMERA_UPGRADE_DONE_S ext_camera_upgrade_done;
    HC_DEVICE_EXT_CAMERA_FW_REQUEST_S ext_camera_fw_request;
    HC_DEVICE_EXT_DECT_STATION_S ext_dect_station;
    HC_DEVICE_EXT_NETWORK_DETECTION_S ext_netwrok_detect;
    unsigned char stuff[0];
} HC_DEVICE_U;

/* define common device info. */
typedef struct {
    unsigned int dev_id;
    unsigned int phy_id;
    char dev_name[MAX_DEVICE_NAME_SIZE];
    char location[MAX_DEVICE_LOCATION_SIZE];
    char conn_status[MAX_DEVICE_CONNSTATUS_SIZE];
    HC_EVENT_TYPE_E event_type;
    HC_NETWORK_TYPE_E network_type;
    HC_DEVICE_TYPE_E dev_type;
    HC_DEVICE_U device;
} HC_DEVICE_INFO_S;

typedef struct {
    HC_DEVICE_TYPE_E dev_type;
    int dev_size;
} HC_DEVICE_DATA_SIZE;

typedef struct {
    unsigned int dev_id;
    char dev_name[MAX_DEVICE_NAME_SIZE];
    HC_EVENT_TYPE_E event_type;
    HC_NETWORK_TYPE_E network_type;
    HC_DEVICE_TYPE_E dev_type;
    HC_DEVICE_EXT_U device_ext;
} HC_DEVICE_EXT_INFO_S;

typedef struct hc_upgrade_info_s
{
    HC_NETWORK_TYPE_E	type;
    char 				image_file_path[256];
    int					error; // 0: upgrade success
} HC_UPGRADE_INFO_S;

typedef struct {
    HC_EVENT_TYPE_E event_type;
    char action[256];
} HC_VOIP_MSG_S;

typedef enum {
    HC_USER_LOG_SEVERITY_MINOR = 0,
    HC_USER_LOG_SEVERITY_NORMAL,
    HC_USER_LOG_SEVERITY_MAJOR,
    HC_USER_LOG_SEVERITY_CRITICAL,
    HC_USER_LOG_SEVERITY_MAX
} HC_USER_LOG_SEVERITY_E;

//#pragma pack()

//doxygen coding  style

/**
* @addtogroup HCAPI
* Common api.
*/

/** @{ */

/**
********************************************************************************
*  Init HCAPI library resources.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_init(void);

/**
********************************************************************************
*  Uninit HCAPI library resources.
********************************************************************************
*/
void hcapi_uninit(void);

/**
********************************************************************************
*  Restart controller.
*  \param network_type The network type of the device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_host_module_restart(HC_NETWORK_TYPE_E network_type, unsigned int timeout_msec);

/**
********************************************************************************
*  Restore to default the configuration of controller.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_host_module_default(unsigned int timeout_msec);

/**
********************************************************************************
*  Add a device to network.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_add(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Stop adding a device to network.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_add_stop(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Delete a device from network.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_delete(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Stop deleting a device from network.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_delete_stop(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Query the driver status.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  Normal, Adding Mode, Deleting Mode.
********************************************************************************
*/
HC_RETVAL_E hcapi_drv_status_query(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Just waiting response.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_result_handle(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Set value of the specified device.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Get value of the specified device.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_get(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Set configuration of the specified device.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_cfg_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Get configuration of the specified device.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_cfg_get(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Set the association of the devices.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_association_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Remove the association of the devices.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_association_remove(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Detect the status of the device. (CONNECTED or DISCONNECTED)
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_detect(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Delete the not available device from network.
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dev_na_delete(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Handle all of broadcast events. (e.g. device status changed.)
*  \param dev_info The device information of the specified device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_event_handle(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Get all devices from DB.
*  \param devs_info It is array buffer includes all device information.
*  \param dev_number The number of all devices
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devs_all(HC_DEVICE_INFO_S *devs_info, int dev_number);

/**
********************************************************************************
*  Get the same device type devices from DB.
*  \param dev_type Device type.
*  \param devs_info It is array buffer includes all device information.
*  \param dev_number The number of all devices
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devs_by_dev_type(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_INFO_S *devs_info, int dev_number);

/**
********************************************************************************
*  Get the same device type and location devices from DB.
*  \param dev_type Device type.
*  \param char* location.
*  \param devs_info It is array buffer includes all device information.
*  \param dev_number The number of all devices
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devs_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, HC_DEVICE_INFO_S *devs_info, int dev_number);

/**
********************************************************************************
*  Get the same network type devices from DB.
*  \param network_type Network type.
*  \param devs_info It is array buffer includes all device information.
*  \param dev_number The number of all devices
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devs_by_network_type(HC_NETWORK_TYPE_E network_type, HC_DEVICE_INFO_S *devs_info, int dev_number);

/**
********************************************************************************
*  Get the number of all devices in DB.
*  \param dev_number The number of devices. (OUTPUT)
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devnum_all(int *dev_number);

/**
********************************************************************************
*  Get the number of devices with same device type in DB.
*  \param dev_type Device type.
*  \param dev_number The number of devices. (OUTPUT)
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devnum_by_dev_type(HC_DEVICE_TYPE_E dev_type, int *dev_number);

/**
********************************************************************************
*  Get the number of devices with same device type in DB.
*  \param dev_type Device type.
*  \param char* location.
*  \param dev_number The number of devices. (OUTPUT)
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devnum_by_dev_type_and_location(HC_DEVICE_TYPE_E dev_type, char *location, int *dev_number);

/**
********************************************************************************
*  Get the number of devices with same network type in DB.
*  \param network_type Network type.
*  \param dev_number The number of devices. (OUTPUT)
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_devnum_by_network_type(HC_NETWORK_TYPE_E network_type, int *dev_number);

/**
********************************************************************************
*  Get device info by specified device ID.
*  \param dev_id Device ID.
*  \param devs_info The device information of the specified device.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_dev_by_dev_id(HC_DEVICE_INFO_S *devs_info);

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
HC_RETVAL_E hcapi_add_log(HC_DEVICE_INFO_S* devs_info, char* log, long log_time, int alarm_type, char* name);

/**
********************************************************************************
*  Get all of logs from DB.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_all(char *buffer, int size);

/**
********************************************************************************
*  Get the number of all logs.
*  \param size The size stroe the number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_all_num(int *size);

/**
********************************************************************************
*  Get the same device ID related logs.
*  \param dev_id The device ID.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_dev_id(unsigned int dev_id, char *buffer, int size);

/**
********************************************************************************
*  Get the nubmer of logs with the same device ID.
*  \param dev_id The device ID.
*  \param size The size stroe the number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_dev_id_num(unsigned int dev_id, int *size);

/**
********************************************************************************
*  Get the same device type related logs.
*  \param dev_type The device type.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_dev_type(HC_DEVICE_TYPE_E dev_type, char *buffer, int size);

/**
********************************************************************************
*  Get the nubmer of logs with the same device type.
*  \param dev_type The device type.
*  \param size The size stroe the number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_dev_type_num(HC_DEVICE_TYPE_E dev_type, int *size);

/**
********************************************************************************
*  Get the same network type related logs.
*  \param network_type The network type.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_network_type(HC_NETWORK_TYPE_E network_type, char *buffer, int size);

/**
********************************************************************************
*  Get the nubmer of logs with the same network type.
*  \param network_type The network type.
*  \param size The size stroe the number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_network_type_num(HC_NETWORK_TYPE_E network_type, int *size);

/**
********************************************************************************
*  Add device to DB.
*  \param devs_info The device information.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_dev(HC_DEVICE_INFO_S *devs_info);
HC_RETVAL_E hcapi_add_dev_log(HC_DEVICE_INFO_S* devs_info, char* log, long log_time, int alarm_type, char* name);

/**
********************************************************************************
*  Add device name to DB by device ID.
*  \param dev_id The device ID.
*  \param buffer The device name
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_devname(unsigned int dev_id, char *buffer);

/**
********************************************************************************
*  Set/Update device name to DB by device ID.
*  \param dev_id The device ID.
*  \param buffer The device name
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_set_devname(unsigned int dev_id, char *buffer);
HC_RETVAL_E hcapi_set_dev(HC_DEVICE_INFO_S *devs_info);

HC_RETVAL_E hcapi_del_dev(HC_DEVICE_INFO_S *devs_info);
HC_RETVAL_E hcapi_del_dev_all();

/**
********************************************************************************
*  Add log to DB.
*  \param log_ptr The log content point.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_log_v2(HC_DEVICE_EXT_LOG_S* log_ptr);

/**
********************************************************************************
*  Get all of logs from DB.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_all_v2(HC_DEVICE_EXT_LOG_S *buffer, int size);

/**
********************************************************************************
*  Get the same device ID related logs.
*  \param dev_id The device ID.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_dev_id_v2(unsigned int dev_id, HC_DEVICE_EXT_LOG_S *buffer, int size);

/**
********************************************************************************
*  Get the same device type related logs.
*  \param dev_type The device type.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_dev_type_v2(HC_DEVICE_TYPE_E dev_type, HC_DEVICE_EXT_LOG_S *buffer, int size);

/**
********************************************************************************
*  Get the same network type related logs.
*  \param network_type The network type.
*  \param buffer The buffer store logs.
*  \param size The number of log.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_log_by_network_type_v2(HC_NETWORK_TYPE_E network_type, HC_DEVICE_EXT_LOG_S *buffer, int size);

/**
********************************************************************************
*  Add attribution to DB by device ID.
*  \param dev_id The device ID.
*  \param name The name of attribution.
*  \param value The value of attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_attr(unsigned int dev_id, char *name, char* value);

/**
********************************************************************************
*  Get attribution from DB by device ID and name.
*  \param dev_id The device ID.
*  \param name The name of attribution..
*  \param value The value of attribution..
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_attr(unsigned int dev_id, char *name, char* value);

/**
********************************************************************************
*  Get attribution from DB by device ID.
*  \param dev_id The device ID.
*  \param size The number of attribution
*  \param attr_ptr The attr_ptr store the attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_attr_by_dev_id(unsigned int dev_id, int size, HC_DEVICE_EXT_ATTR_S * attr_ptr);

/**
********************************************************************************
*  Get the number of attribution from DB by device ID.
*  \param dev_id The device ID.
*  \param size The size store the number of attribution
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_attr_num_by_dev_id(unsigned int dev_id, int *size);

/**
********************************************************************************
*  Set/Update attribution from DB by device ID and name.
*  \param dev_id The device ID.
*  \param name The name of attribution.
*  \param value The value of attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_set_attr(unsigned int dev_id, char *name, char* value);

/**
********************************************************************************
*  Delete attribution from DB by device ID and name.
*  \param dev_id The device ID.
*  \param name The name of attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_del_attr(unsigned int dev_id, char *name);

/**
********************************************************************************
*  Delete attribution from DB by device ID.
*  \param dev_id The device ID.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_del_attr_by_id(unsigned int dev_id);

/**
********************************************************************************
*  Add scene to DB.
*  \param scene_id The scene ID.
*  \param scene_attr The scene attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_scene(char* scene_id, char *scene_attr);

/**
********************************************************************************
*  Get scene from DB.
*  \param scene_id The scene ID.
*  \param scene_attr The scene attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_scene(char* scene_id, char *scene_attr);

/**
********************************************************************************
*  Set/Update scene to DB.
*  \param scene_id The scene ID.
*  \param scene_attr The scene attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_set_scene(char* scene_id, char *scene_attr);

/**
********************************************************************************
*  Delete scene from DB by scene ID.
*  \param scene_id The scene ID.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_del_scene(char* scene_id);

/**
********************************************************************************
*  Get the number of all scene.
*  \param size The size stroe the number of scene.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_scene_num_all(int *size);

/**
********************************************************************************
*  Get all of scene from DB.
*  \param size The number of scene.
*  \param scene_ptr The scene_ptr store all scene.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_scene_all(int size, HC_DEVICE_EXT_SCENE_S * scene_ptr);

/**
********************************************************************************
*  Delete all scene from DB.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_del_scene_all(void);

/**
********************************************************************************
*  Add location to DB.
*  \param location_id The scene ID.
*  \param location_attr The scene attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_location(char* location_id, char *location_attr);

/**
********************************************************************************
*  Get location from DB.
*  \param location_id The location ID.
*  \param location_attr The scene attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_location(char* location_id, char *location_attr, int size);

/**
********************************************************************************
*  Set/Update location from DB.
*  \param location_id The location ID.
*  \param location_attr The scene attribution.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_set_location(char* location_id, char *location_attr);

/**
********************************************************************************
*  Delete location from DB by location ID.
*  \param location_id The location ID.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_del_location(char* location_id);

/**
********************************************************************************
*  Get the number of all location.
*  \param size The size stroe the number of location.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_location_num_all(int *size);

/**
********************************************************************************
*  Get all of location from DB.
*  \param size The number of location.
*  \param location_ptr The location_ptr store all location.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_location_all(int size, HC_DEVICE_EXT_LOCATION_S * location_ptr);

/**
********************************************************************************
*  Delete all location from DB.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_del_location_all(void);

/**
********************************************************************************
*  Set configuration pair name=value.
*  \param conf_name The name of configuration.
*  \param conf_value The value of configuration.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_set_conf(char* conf_name, char *conf_value);

/**
********************************************************************************
*  Get configuration value by name.
*  \param conf_name The name of configuration.
*  \param conf_value The value of configuration.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_conf(char* conf_name, char *conf_value);


/**
********************************************************************************
*  External application/process send ip changed event to HCAPI system.
*  \param msg_type The message type.
*  \param network_type The network type.
*  \param data The data of this message.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_send_ext_signal(int msg_type, int network_type, void *data, unsigned int timeout_msec);

/**
********************************************************************************
*  Add user operation log to DB.
*  \param log The user log point.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_add_user_log(HC_USER_LOG_SEVERITY_E severity, char *content);

/**
********************************************************************************
*  Get all user logs from DB.
*  \param log It is array buffer includes all user logs.
*  \param size The number of user logs.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_user_log_all(HC_DEVICE_EXT_USER_LOG_S* log, int size);

/**
********************************************************************************
*  Get the number of all user logs in DB.
*  \param[out] size The number of user logs.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_get_user_log_num_all(int *size);

/**
********************************************************************************
*  Handle all of voip related event.
*  \param pMsg The voip msg point.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_voip_event_handle(HC_VOIP_MSG_S *pMsg, unsigned int timeout_msec);

/**
********************************************************************************
*  Send out voip message.
*  \param pMsg The voip message point.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_voip_message_send(HC_VOIP_MSG_S *pMsg);

/**
********************************************************************************
*  Set value of the dect device.
*  \param dev_info The device information of the dect device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dect_parameter_set(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);

/**
********************************************************************************
*  Get value of the dect device.
*  \param dev_info The device information of the dect device.
*  \param timeout_msec The waiting timeout of the operation.
*  \return  0-success, >0-failure.
********************************************************************************
*/
HC_RETVAL_E hcapi_dect_parameter_get(HC_DEVICE_INFO_S *dev_info, unsigned int timeout_msec);


HC_RETVAL_E hcapi_get_armmode(HC_ARM_MODE *armmode);


HC_RETVAL_E hcapi_set_armmode(HC_ARM_MODE *armmode);




/** @} */

#ifdef __cplusplus
}
#endif


#endif

