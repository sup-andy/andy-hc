#ifndef _ZWAVE_API_H_
#define _ZWAVE_API_H_

/*
 * DEBUG MACRO DEFINITION
 */
#define ZWAVE_DRIVER_DEBUG  0

#if (ZWAVE_DRIVER_DEBUG == 0)
#include "hc_common.h"
#include "zw_serial.h"
#else
#define DEBUG_INFO printf
#define DEBUG_ERROR  printf
#endif




//#pragma pack(1)

#ifndef NULL
#define NULL  (0)
#endif

#ifndef TRUE
#define TRUE  (1)
#define FALSE (0)
#endif

typedef unsigned char BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;

#define VOID_CALLBACKFUNC(completedFunc)  void (*completedFunc)

/* Max number of nodes in a Z-wave system */
#define ZW_MAX_NODES        232
#define EXTEND_NODE_CHANNEL 8
#define MAX_CMDCLASS_BITMASK_LEN (256/8)

/*The max length of a node mask*/
#define MAX_NODEMASK_LENGTH   (ZW_MAX_NODES/8)

#define ILLEGAL_NODE_ID 0xff       /* To indicate host-id as uninitialized */

#define ILLEGAL_TEMPERATURE_VALUE   -9999.9999

#define ZW_BUF_SIZE    256  // application buffer mustn't been larger than 80 bytes ?

#define ACK_TIMER (10*1000)  // 10 sec
#define RESPONSE_TIMER  (10*1000)
#define REQUEST_TIMER  (40*1000)
#define NODEINFO_REQUEST_TIMER  (40*1000)
#define GET_STATUS_TIMER    (30*1000)
#define UPDATE_NODE_STATUS_TIMER    4

typedef enum _ZW_STATUS_
{
    ZW_STATUS_IDLE = 0,
    ZW_STATUS_OK = 0,
    ZW_STATUS_TIMEOUT,
    ZW_STATUS_FAIL,
    ZW_STATUS_INVALID_PARAM,
    ZW_STATUS_ACK_RECEIVED,
    ZW_STATUS_RESPONSE_RECEIVED,
    ZW_STATUS_IS_FAIL_NODE,
    ZW_STATUS_NOT_FAIL_NODE,

    ZW_STATUS_FAIL_NODE_REMOVE_START,
    ZW_STATUS_FAIL_NODE_REMOVED,
    ZW_STATUS_FAIL_NODE_REMOVED_FAIL,

    ZW_STATUS_SENDDATA,
    ZW_STATUS_SENDDATA_SUCCESSFUL,
    ZW_STATUS_SENDDATA_FAIL,

    ZW_STATUS_GET_NODE_STATUS,
    ZW_STATUS_GET_NODE_STATUS_RECEIVED,
    ZW_STATUS_GET_NODE_STATUS_FAIL,
    ZW_STATUS_SET_NODE_STATUS,
    ZW_STATUS_ADD_NODE,
    ZW_STATUS_REMOVE_NODE,
    ZW_STATUS_GET_VERSION,
    ZW_STATUS_GET_MEMORY_ID,
    ZW_STATUS_GET_INIT_DATA,
    ZW_STATUS_GET_NODE_PROTOCOL_INFO,

    ZW_STATUS_REQUEST_NODEINFO,
    ZW_STATUS_NODEINFO_RECEIVED,
    ZW_STATUS_NODEINFO_RECEIVED_FAIL,

    ZW_STATUS_GET_MULTI_CHANNEL_ENDPOINTS,
    ZW_STATUS_MULTI_CHANNEL_ENDPOINTS_RECEIVED,

    ZW_STATUS_GET_MULTI_CHANNEL_CAPABILITY,
    ZW_STATUS_MULTI_CHANNEL_CAPABILITY_RECEIVED,

    ZW_STATUS_SET_DEFAULT,
    ZW_STATUS_SET_DEFAULT_DONE,

    ZW_STATUS_SOFTRESET,
    ZW_STATUS_SOFTRESET_DONE,

    ZW_STATUS_TEST_FAIL_NODE,

    ZW_STATUS_GET_BATTERY_LEVEL,
    ZW_STATUS_GET_BATTERY_LEVEL_RECEIVED,

    ZW_STATUS_ASSOCIATION_SUPPORTED_GROUPINGS_GET_RECEIVED,
    ZW_STATUS_ASSOCIATION_GET_RECEIVED,
    ZW_STATUS_ASSOCIATION_RECORDS_SUPPORTED_RECEIVED,
    ZW_STATUS_ASSOCIATION_CONFIGURATION_GET_RECEIVED,

    ZW_STATUS_SECURITY_NONCE_GET_RECEIVED,
    ZW_STATUS_SECURITY_NONCE_REPORT_SEND,
    ZW_STATUS_SECURITY_NONCE_REPORT_RECEIVED,
    ZW_STATUS_SECURITY_SCHEME_REPORT_RECEIVED,
    ZW_STATUS_SECURITY_NETWORK_KEY_VERIFY_RECEIVED,

    ZW_STATUS_GET_RANDOM_WORD,
    ZW_STATUS_SET_RF_RECEIVE_MODE,
    ZW_STATUS_GET_BUFFER,
    ZW_STATUS_PUT_BUFFER,
    ZW_STATUS_PUT_BUFFER_REQUEST_RECEIVED,

    ZW_STATUS_ASSIGN_RETURN_ROUTE_REQUEST,
    ZW_STATUS_ASSIGN_RETURN_ROUTE_OK,
    ZW_STATUS_ASSIGN_RETURN_ROUTE_FAIL,

    ZW_STATUS_DELETE_RETURN_ROUTE_REQUEST,
    ZW_STATUS_DELETE_RETURN_ROUTE_OK,
    ZW_STATUS_DELETE_RETURN_ROUTE_FAIL,

    ZW_STATUS_MAX
} ZW_STATUS;

typedef enum
{
    IDX_SOF = 0,
    IDX_LEN,
    IDX_TYPE,
    IDX_CMD_ID,
    IDX_CMD_DATA
} FRAME_IDX;

typedef enum _ZW_DEVICE_TYPE_
{
    ZW_DEVICE_PORTABLE_CONTROLLER = 1,
    ZW_DEVICE_STATIC_CONTROLLER,

    ZW_DEVICE_BINARYSWITCH,
    ZW_DEVICE_MULTILEVEL_SWITCH,

    ZW_DEVICE_DIMMER,
    ZW_DEVICE_CURTAIN,

    ZW_DEVICE_BINARYSENSOR,
    ZW_DEVICE_MULTILEVEL_SENSOR,
    ZW_DEVICE_BATTERY,

    ZW_DEVICE_DOORLOCK,
    ZW_DEVICE_THERMOSTAT,

    ZW_DEVICE_METER,

    ZW_DEVICE_KEYFOB,

    ZW_DEVICE_SIREN,

    ZW_DEVICE_UNKNOW
} ZW_DEVICE_TYPE;

typedef struct _ZW_BASIC_STATUS_
{
    BYTE value;
} ZW_BASIC_STATUS;

/*  binary switch definition */
#define ZW_BINARY_SWITCH_TYPE_RESERVED  0x00
#define ZW_BINARY_SWITCH_TYPE_WATER_VALVE  0x01
#define ZW_BINARY_SWITCH_TYPE_THERMOSTAT  0x02


#define WATER_VALVE_STATUS_CLOSE    0xff
#define WATER_VALVE_STATUS_OPEN    0x00

#define BINARYSWITCH_STATUS_ON 0xff
#define BINARYSWITCH_STATUS_OFF 0x00
typedef struct _ZW_BINARYSWITCH_STATUS_
{
    BYTE value;
    BYTE type;
} ZW_BINARYSWITCH_STATUS;

/* multilevel switch definition */
#define ZW_MULTILEVEL_SWITCH_TYPE_RESERVED 0x00
#define ZW_MULTILEVEL_SWITCH_TYPE_CURTAIN  0x01

typedef struct _ZW_MULTILEVEL_SWITCH_STATUS_
{
    BYTE value;
    BYTE type;
}ZW_MULTILEVEL_SWITCH_STATUS;

typedef struct _ZW_DIMMER_STATUS_
{
    BYTE value;  // 0 - 0x63 or 0xff
} ZW_DIMMER_STATUS;

typedef struct _ZW_KEYFOB_STATUS_
{
    BYTE value;  // 0 - 0x6 or 0xff
} ZW_KEYFOB_STATUS;

typedef struct _ZW_SIREN_STATUS_
{
    BYTE value; 
} ZW_SIREN_STATUS;

#define CURTAIN_ON    0xff
#define CURTAIN_OFF    0x00
#define CURTAIN_STOP     0xfe
typedef struct _ZW_CURTAIN_STATUS_
{
    BYTE value;  // 0x00 - 0x63, oxff or 0xfe(stop)
} ZW_CURTAIN_STATUS;

#define ZW_BINARY_SENSOR_TYPE_RESERVED    0x00
#define ZW_BINARY_SENSOR_TYPE_GENERAL_PURPOSE    0x01
#define ZW_BINARY_SENSOR_TYPE_SMOKE    0x02
#define ZW_BINARY_SENSOR_TYPE_CO    0X03
#define ZW_BINARY_SENSOR_TYPE_CO2    0x04
#define ZW_BINARY_SENSOR_TYPE_HEAT    0x05
#define ZW_BINARY_SENSOR_TYPE_WATER    0x06
#define ZW_BINARY_SENSOR_TYPE_FREEZE    0x07
#define ZW_BINARY_SENSOR_TYPE_TAMPER    0x08
#define ZW_BINARY_SENSOR_TYPE_AUX    0x09
#define ZW_BINARY_SENSOR_TYPE_DOOR_WINDOW    0x0a
#define ZW_BINARY_SENSOR_TYPE_TILT    0x0b
#define ZW_BINARY_SENSOR_TYPE_MOTION    0x0c
#define ZW_BINARY_SENSOR_TYPE_GLASS_BREAK    0x0d
#define ZW_BINARY_SENSOR_TYPE_RETURN_1ST_SENSOR    0xff

typedef struct _ZW_BINARYSENSOR_STATUS_
{
    BYTE value;  // 0, idle; 0xff, event detect
    BYTE sensor_type;
} ZW_BINARYSENSOR_STATUS;

#define ZW_MULTILEVEL_SENSOR_TYPE_RESERVED    0x00
#define ZW_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE    0x01
#define ZW_MULTILEVEL_SENSOR_TYPE_GENERAL_PURPOSE_VALUE    0x02
#define ZW_MULTILEVEL_SENSOR_TYPE_LUMINANCE    0x03
#define ZW_MULTILEVEL_SENSOR_TYPE_POWER    0x04
#define ZW_MULTILEVEL_SENSOR_TYPE_HUMIDITY    0x05
#define ZW_MULTILEVEL_SENSOR_TYPE_VELOCITY    0x06
#define ZW_MULTILEVEL_SENSOR_TYPE_DIRECTION    0x07
#define ZW_MULTILEVEL_SENSOR_TYPE_ATMOSPHERIC_PRESSURE    0x08
#define ZW_MULTILEVEL_SENSOR_TYPE_BAROMETRIC_PRESSURE    0x09
#define ZW_MULTILEVEL_SENSOR_TYPE_SOLAR_RADIATION    0x0a
#define ZW_MULTILEVEL_SENSOR_TYPE_DEW_POINT    0x0b
#define ZW_MULTILEVEL_SENSOR_TYPE_RAIN_RATE    0x0c
#define ZW_MULTILEVEL_SENSOR_TYPE_TIDE_LEVEL    0x0d
#define ZW_MULTILEVEL_SENSOR_TYPE_WEIGHT    0x0e
#define ZW_MULTILEVEL_SENSOR_TYPE_VOLTAGE    0x0f
#define ZW_MULTILEVEL_SENSOR_TYPE_CURRENT    0x10
#define ZW_MULTILEVEL_SENSOR_TYPE_CO2_LEVEL    0x11
#define ZW_MULTILEVEL_SENSOR_TYPE_AIR_FLOW    0x12
#define ZW_MULTILEVEL_SENSOR_TYPE_TANK_CAPACITY    0x13
#define ZW_MULTILEVEL_SENSOR_TYPE_DISTANCE    0x14
#define ZW_MULTILEVEL_SENSOR_TYPE_ANGLE_POSITION    0x15
#define ZW_MULTILEVEL_SENSOR_TYPE_ROTATION    0x16
#define ZW_MULTILEVEL_SENSOR_TYPE_WATER_TEMPERATURE    0x17
#define ZW_MULTILEVEL_SENSOR_TYPE_SOIL_TEMPERATURE    0x18
#define ZW_MULTILEVEL_SENSOR_TYPE_SEISMIC_INTENSITY    0x19
#define ZW_MULTILEVEL_SENSOR_TYPE_SEISMIC_MAGNITUDE    0x1a
#define ZW_MULTILEVEL_SENSOR_TYPE_ULTRAVIOLET    0x1b
#define ZW_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_RESISTIVITY    0x1c
#define ZW_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_CONDUCTIVITY    0x1d
#define ZW_MULTILEVEL_SENSOR_TYPE_LOUDNESS    0x1e
#define ZW_MULTILEVEL_SENSOR_TYPE_MOISTURE    0x1f

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_AIR_TEMPERATURE_
{
    SCALE_AIR_TEMPERATURE_CELSIUS = 0x00,  // Celsius (C)
    SCALE_AIR_TEMPERATURE_FAHRENHEIT = 0x01,  // Fahrenheit (F)
    SCALE_AIR_TEMPERATURE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_AIR_TEMPERATURE;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_GENERAL_PURPOSE_
{
    SCALE_GENERAL_PURPOSE_PERCENTAGE = 0x00,  // Percentage value
    SCALE_GENERAL_PURPOSE_DIMENSIONLESS = 0x01,  // Dimensionless value
    SCALE_GENERAL_PURPOSE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_GENERAL_PURPOSE;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_LUMINANCE_
{
    SCALE_LUMINANCE_PERCENTAGE = 0x00,  // Percentage value
    SCALE_LUMINANCE_LUX = 0x01,  // Lux
    SCALE_LUMINANCE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_LUMINANCE;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_POWER_
{
    SCALE_POWER_W = 0x00,   // W
    SCALE_POWER_BTU_P_H = 0x01,   // Btu/h
    SCALE_POWER_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_POWER;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_HUMIDITY_
{
    SCALE_HUMIDITY_PERCENTAGE = 0x00,  // Percentage value
    SCALE_HUMIDITY_ABSOLUTE = 0x01,   // (g/m3) 每 (v5)
    SCALE_HUMIDITY_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_HUMIDITY;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_VELOCITY_
{
    SCALE_VELOCITY_M_P_S = 0x00, // m/s
    SCALE_VELOCITY_MPH = 0x01,   // Mph
    SCALE_VELOCITY_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_VELOCITY;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_DIRECTION_
{
    SCALE_DIRECTION_DEGRESS = 0x00, // 0 to 360 degrees.  0 = no wind, 90 = east, 180 = south, 270 = west, and 360 = north
    SCALE_DIRECTION_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_DIRECTION;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_ATMOSPHERIC_PRESSURE_
{
    SCALE_ATMOSPHERIC_PRESSURE_KPA = 0x00,  // kPa (kilopascal)
    SCALE_ATMOSPHERIC_PRESSURE_INCHES_OF_MERCURY = 0x01,   // Inches of Mercury
    SCALE_ATMOSPHERIC_PRESSURE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_ATMOSPHERIC_PRESSURE;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_BAROMETRIC_PRESSURE_
{
    SCALE_BAROMETRIC_PRESSURE_KPA = 0x00,  // kPa (kilopascal)
    SCALE_BAROMETRIC_PRESSURE_INCHES_OF_MERCURY = 0x01,   // Inches of Mercury
    SCALE_BAROMETRIC_PRESSURE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_BAROMETRIC_PRESSURE;

typedef enum _ZW_MULTILEVEL_SENSOR_SCALE_SOLAR_RADIATION_
{
    SCALE_SOLAR_RADIATION_W_P_M2 = 0x00,  // W/m2
    SCALE_SOLAR_RADIATION_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_SOLAR_RADIATION;

typedef enum _ZW_MULTILEVEL_SENSOR_DEW_POINT_
{
    SCALE_DEW_POINT_CELSIUS = 0x00,  // Celsius (C)
    SCALE_DEW_POINT_FAHRENHEIT = 0x01,   // Fahrenheit (F)
    SCALE_DEW_POINT_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_DEW_POINT;

typedef enum _ZW_MULTILEVEL_SENSOR_RAIN_RATE_
{
    SCALE_RAIN_RATE_MM_P_H = 0x00,  // mm/h
    SCALE_RAIN_RATE_IN_P_H = 0x01,   // in/h
    SCALE_RAIN_RATE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_RAIN_RATE;

typedef enum _ZW_MULTILEVEL_SENSOR_TIDE_LEVEL_
{
    SCALE_TIDE_LEVEL_M = 0x00,  // m
    SCALE_TIDE_LEVEL_FEET = 0x01,   // Feet
    SCALE_TIDE_LEVEL_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_TIDE_LEVEL;

typedef enum _ZW_MULTILEVEL_SENSOR_WEIGHT_
{
    SCALE_WEIGHT_KG = 0x00,  // Kg
    SCALE_WEIGHT_POUNDS = 0x01,   // pounds
    SCALE_WEIGHT_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_WEIGHT;

typedef enum _ZW_MULTILEVEL_SENSOR_VOLTAGE_
{
    SCALE_VOLTAGE_V = 0x00,  // V
    SCALE_VOLTAGE_MV = 0x01,   // mV
    SCALE_VOLTAGE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_VOLTAGE;

typedef enum _ZW_MULTILEVEL_SENSOR_CURRENT_
{
    SCALE_CURRENT_A = 0x00,  // A
    SCALE_CURRENT_MA = 0x01,   // mA
    SCALE_CURRENT_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_CURRENT;

typedef enum _ZW_MULTILEVEL_SENSOR_CO2_LEVEL_
{
    SCALE_CO2_LEVEL_PPM = 0x00,  // Ppm
    SCALE_CO2_LEVEL_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_CO2_LEVEL;

typedef enum _ZW_MULTILEVEL_SENSOR_AIR_FLOW_
{
    SCALE_AIR_FLOW_M3_P_H= 0x00,  // m3/h
    SCALE_AIR_FLOW_CFM = 0x01,   // cfm (cubic feet per minute)
    SCALE_AIR_FLOW_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_AIR_FLOW;

typedef enum _ZW_MULTILEVEL_SENSOR_TANK_CAPACITY_
{
    SCALE_TANK_CAPACITY_L= 0x00,  // l (liter)
    SCALE_TANK_CAPACITY_CBM = 0x01,   // cbm (cubic meter)
    SCALE_TANK_CAPACITY_GALLONS = 0x02,   // gallons
    SCALE_TANK_CAPACITY_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_TANK_CAPACITY;

typedef enum _ZW_MULTILEVEL_SENSOR_DISTANCE_
{
    SCALE_DISTANCE_M= 0x00,  // M
    SCALE_DISTANCE_CM = 0x01,   // Cm
    SCALE_DISTANCE_FEET = 0x02,   // Feet
    SCALE_DISTANCE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_DISTANCE;

typedef enum _ZW_MULTILEVEL_SENSOR_ANGLE_POSITION_
{
    SCALE_ANGLE_POSITION_PERCENTAGE= 0x00,  // Percentage value
    SCALE_ANGLE_POSITION_DEGREES_NORTH = 0x01,   // Degrees relative to north pole of standing eye view
    SCALE_ANGLE_POSITION_DEGREES_SOUTH = 0x02,   // Degrees relative to south pole of standing eye view
    SCALE_ANGLE_POSITION_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_ANGLE_POSITION;

typedef enum _ZW_MULTILEVEL_SENSOR_ROTATION_
{
    SCALE_ROTATION_RPM= 0x00,  // rpm (revolutions per minute)
    SCALE_ROTATION_HZ = 0x01,   // Hz (Hertz)
    SCALE_ROTATION_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_ROTATION;

typedef enum _ZW_MULTILEVEL_SENSOR_WATER_TEMPERATURE_
{
    SCALE_WATER_TEMPERATURE_CELSIUS= 0x00,  // Celsius (C)
    SCALE_WATER_TEMPERATURE_FAHRENHELT = 0x01,   // Fahrenheit (F)
    SCALE_WATER_TEMPERATURE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_WATER_TEMPERATURE;

typedef enum _ZW_MULTILEVEL_SENSOR_SOIL_TEMPERATURE_
{
    SCALE_SOIL_TEMPERATURE_CELSIUS= 0x00,  // Celsius (C)
    SCALE_SOIL_TEMPERATURE_FAHRENHELT = 0x01,   // Fahrenheit (F)
    SCALE_SOIL_TEMPERATURE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_SOIL_TEMPERATURE;

typedef enum _ZW_MULTILEVEL_SENSOR_SEISMIC_INTENSITY_
{
    SCALE_SEISMIC_INTENSITY_MERCALLI= 0x00,  // Mercalli
    SCALE_SEISMIC_INTENSITY_EUROPEAN_MACROSEISMIC = 0x01,   // European Macroseismic
    SCALE_SEISMIC_INTENSITY_LIEDU= 0x02,  // Liedu
    SCALE_SEISMIC_INTENSITY_SHINDO = 0x03,   // Shindo
    SCALE_SEISMIC_INTENSITY_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_SEISMIC_INTENSITY;

typedef enum _ZW_MULTILEVEL_SENSOR_SEISMIC_MAGNITUDE_
{
    SCALE_SEISMIC_MAGNITUDE_ML= 0x00,  // Local (ML)
    SCALE_SEISMIC_MAGNITUDE_MW = 0x01,   // Moment (MW)
    SCALE_SEISMIC_MAGNITUDE_MS= 0x02,  // Surface wave (MS)
    SCALE_SEISMIC_MAGNITUDE_MB = 0x03,   // Body wave (MB)
    SCALE_SEISMIC_MAGNITUDE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_SEISMIC_MAGNITUDE;

typedef enum _ZW_MULTILEVEL_SENSOR_ULTRAVIOLET_
{
    SCALE_ULTRAVIOLET_UV= 0x00,  // UV index
    SCALE_ULTRAVIOLET_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_ULTRAVIOLET;

typedef enum _ZW_MULTILEVEL_SENSOR_ELECTRICAL_RESISTIVITY_
{
    SCALE_ELECTRICAL_RESISTIVITY_OHM= 0x00,  // ohm metre (次m)
    SCALE_ELECTRICAL_RESISTIVITY_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_ELECTRICAL_RESISTIVITY;

typedef enum _ZW_MULTILEVEL_SENSOR_ELECTRICAL_CONDUCTIVITY_
{
    SCALE_ELECTRICAL_CONDUCTIVITY_SIEMENS_PER_METRE = 0x00,  // siemens per metre (S.M - 1)
    SCALE_ELECTRICAL_CONDUCTIVITY_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_ELECTRICAL_CONDUCTIVITY;

typedef enum _ZW_MULTILEVEL_SENSOR_LOUDNESS_
{
    SCALE_LOUDNESS_DB= 0x00,  // Absolute loudness (dB)
    SCALE_LOUDNESS_DBA = 0x01,   // A-weighted decibels (dBA)
    SCALE_LOUDNESS_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_LOUDNESS;

typedef enum _ZW_MULTILEVEL_SENSOR_MOISTURE_
{
    SCALE_MOISTURE_PERCENTAGE= 0x00,  // Percentage value
    SCALE_MOISTURE_M3_P_M3 = 0x01,   // Volume water content (m3/m3)
    SCALE_MOISTURE_IMPEDANCE= 0x02,  // Impedance (k次)
    SCALE_MOISTURE_AW = 0x03,   // Water activity (aw)
    SCALE_MOISTURE_RESERVED
}ZW_MULTILEVEL_SENSOR_SCALE_MOISTURE;

typedef struct _ZW_MULTILEVEL_SENSOR_STATUS_
{
    double value;
    BYTE sensor_type;
    BYTE scale;
} ZW_MULTILEVEL_SENSOR_STATUS;

typedef struct _ZW_BATTERY_STATUS_
{
    BYTE battery_level;   // 0x00 - 0x64. The value 0xFF indicates a battery low warning.
    unsigned int interval_time;  // 0x00 - 0x00ffffff
    unsigned int minWakeupIntervalSec;    // 0x00 - 0x00ffffff
    unsigned int maxWakeupIntervalSec;  // 0x00 - 0x00ffffff
    unsigned int defaultWakeupIntervalSec;  // 0x00 - 0x00ffffff
    unsigned int wakeupIntervalStepSec;  // 0x00 - 0x00ffffff
} ZW_BATTERY_STATUS;

// doorlock mode
#define DOORLOCK_LOCKMODE_UNSECURED  0x00
#define DOORLOCK_LOCKMODE_UNSECURED_WITH_TIMEOUT 0x01
#define DOORLOCK_LOCKMODE_UNSECURED_FOR_INSIDE_DOOR_HANDLES 0x10
#define DOORLOCK_LOCKMODE_UNSECURED_FOR_INSIDE_DOOR_HANDLES_WITH_TIMEOUT 0x11
#define DOORLOCK_LOCKMODE_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES 0x20
#define DOORLOCK_LOCKMODE_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_WITH_TIMEOUT 0x21
#define DOORLOCK_LOCKMODE_SECURED 0xff

/**********Inside/Outside Door Handles Mode********************************
0-3 bits(Inside Door Handles Mode); 4-7 bits(Outside Door Handles Mode)

Bit Outside Door Handles Mode (4 bits) Inside Door Handles Mode (4 bits)
0 0 = Handle 1 inactive; 1 = Handle 1 active 0 = Handle 1 inactive; 1 = Handle 1 active
1 0 = Handle 2 inactive; 1 = Handle 2 active 0 = Handle 2 inactive; 1 = Handle 2 active
2 0 = Handle 3 inactive; 1 = Handle 3 active 0 = Handle 3 inactive; 1 = Handle 3 active
3 0 = Handle 4 inactive; 1 = Handle 4 active 0 = Handle 4 inactive; 1 = Handle 4 active

************************************************************************/
#define DOORLOCK_HANDLEMODE_INDOOR_HANDLE_1_MASK    0x01
#define DOORLOCK_HANDLEMODE_INDOOR_HANDLE_2_MASK    0x02
#define DOORLOCK_HANDLEMODE_INDOOR_HANDLE_3_MASK    0x04
#define DOORLOCK_HANDLEMODE_INDOOR_HANDLE_4_MASK    0x08
#define DOORLOCK_HANDLEMODE_OUTDOOR_HANDLE_1_MASK    0x10
#define DOORLOCK_HANDLEMODE_OUTDOOR_HANDLE_2_MASK    0x20
#define DOORLOCK_HANDLEMODE_OUTDOOR_HANDLE_3_MASK    0x40
#define DOORLOCK_HANDLEMODE_OUTDOOR_HANDLE_4_MASK    0x80
/************Door Condition*****************

Bit Description
0 0 = Door Open; 1 = Door Closed
1 0 = Bolt Locked; 1 = Bolt Unlocked
2 0 = Latch Open; 1 = Latch Closed
3-7 Reserved

******************************************/
#define DOORLOCK_CONDITION_DOOR_MASK    0x01
#define DOORLOCK_CONDITION_BOLT_MASK    0x02
#define DOORLOCK_CONDITION_LATCH_MASK    0x04
/**********lockTimeoutMinutes/lockTimeoutSeconds********

zero: no timeout
0xfe: not support by this doorlock

****************************************************/
#define DOORLOCK_TIME_NOT_SUPPORT    0xfe
typedef struct _ZW_DOORLOCK_STATUS_
{
    BYTE doorLockMode;                 /**/
    BYTE doorHandlesMode;                  /* masked byte   0-3 bits(Inside Door Handles Mode); 4-7 bits(Outside Door Handles Mode)*/
    BYTE doorCondition;                /*masked byte 0-3 bits effective*/
    BYTE lockTimeoutMinutes;           /**/
    BYTE lockTimeoutSeconds;           /**/
} ZW_DOORLOCK_STATUS;

/**********operationType**********

Hexadecimal Description
0x01 Constant operation
0x02 Timed operation
0x03 每 0XFF Reserved

*********************************/
#define DOORLOCK_OPERATION_TYPE_CONSTANT    0x01
#define DOORLOCK_OPERATION_TYPE_TIMED    0x02
typedef struct _ZW_DOORLOCK_CONFIGURATION_
{
    BYTE      operationType;                /**/
    BYTE      doorHandlesMode;                  /* masked byte */
    BYTE      lockTimeoutMinutes;           /**/
    BYTE      lockTimeoutSeconds;           /**/
} ZW_DOORLOCK_CONFIGURATION;


/************mode type*************

mode              Description
0                 Off - System is off
1                 Heating
2                 Cooling
3                 Auto
4                 Auxiliary/Emergency Heat
5                 Resume
6                 Fan Only
7                 Furnace
8                 Dry Air
9                 Moist Air
10                Auto changeover
11                Energy Save Heating
12                Energy Save Cooling
13                Away Heating

****************************************/

#define THERMOSTAT_FAN_MODE_IDLE    0x00
#define THERMOSTAT_FAN_MODE_RUNNING    0x01
#define THERMOSTAT_FAN_MODE_RUNNING_HIGH    0x02

#define THERMOSTAT_MODE_OFF    0x00
#define THERMOSTAT_MODE_HEATING    0x01
#define THERMOSTAT_MODE_COOLING    0x02
#define THERMOSTAT_MODE_AUTO    0x03
#define THERMOSTAT_MODE_AUXI_HEAT    0x04
#define THERMOSTAT_MODE_RESUME    0x05
#define THERMOSTAT_MODE_FAN_ONLY    0x06
#define THERMOSTAT_MODE_FURNACE    0x07
#define THERMOSTAT_MODE_DRY_AIR    0x08
#define THERMOSTAT_MODE_MOIST_AIR    0x09
#define THERMOSTAT_MODE_AUTO_CHANGEOVER    0x0a
#define THERMOSTAT_MODE_ENERGY_SAVE_HEATING 0x0b
#define THERMOSTAT_MODE_ENERGY_SAVE_COOLING    0x0c
#define THERMOSTAT_MODE_AWAY_HEATING    0x0d

typedef struct _ZW_THERMOSTAT_STATUS_
{
    BYTE fan_mode;
    BYTE mode;
    BYTE setBack_type;
    BYTE setBack_state;
    double value;
    double heat_value;
    double cool_value;
    double energe_save_heat_value;
    double energe_save_cool_value;
} ZW_THERMOSTAT_STATUS;

/************ Meter Type *******************/
#define ZW_METER_TYPE_SINGLE_E_ELECTRIC    0x01
#define ZW_METER_TYPE_GAS    0x02
#define ZW_METER_TYPE_WATER    0x03
#define ZW_METER_TYPE_TWIN_E_ELECTRIC    0x04
#define ZW_METER_TYPE_3P_SINGLE_DIRECT_ELECTRIC    0x05
#define ZW_METER_TYPE_3P_SINGLE_ECT_ELECTRIC    0x06
#define ZW_METER_TYPE_1_PHASE_DIRECT_ELECTRIC    0x07
#define ZW_METER_TYPE_HEATING    0x08
#define ZW_METER_TYPE_COOLING    0x09
#define ZW_METER_TYPE_COMBINED_HEATING_AND_COOLING    0x0a
#define ZW_METER_TYPE_ELECTRIC_SUB_METER    0x0b

/************ Single E Electric scale *******************/
#define METER_ELECTRIC_SCALE_ENERGY    0x00
#define METER_ELECTRIC_SCALE_POWER    0x02

typedef enum _ZW_METER_SCALE_ELECTRIC_
{
    SCALE_ELECTRIC_KWH = 0x00, //kWh
    SCALE_ELECTRIC_KVAH = 0x01, //kVAh
    SCALE_ELECTRIC_W = 0x02, //W
    SCALE_ELECTRIC_PULSE_COUNT = 0x03, //Pulse count
    SCALE_ELECTRIC_V = 0x04, //Voltage (V)
    SCALE_ELECTRIC_A = 0x05, //Amperes (A)
    SCALE_ELECTRIC_POWER_FACTOR = 0x06, //Power Factor (%)
    SCALE_ELECTRIC_RESERVED
}ZW_METER_SCALE_ELECTRIC;

typedef enum _ZW_METER_SCALE_GAS_
{
    SCALE_GAS_CUBIC_METER = 0x00, //Cubic meter
    SCALE_GAS_CUBIC_FEET = 0x01, //Cubic feet
    SCALE_GAS_PULSE_COUNT = 0x03, //Pulse count
    SCALE_GAS_WATER_RESERVED
}ZW_METER_SCALE_GAS_WATER;

typedef enum _ZW_METER_SCALE_WATER_
{
    SCALE_WATER_CUBIC_METER = 0x00, //Cubic meter
    SCALE_WATER_CUBIC_FEET = 0x01, //Cubic feet
    SCALE_WATER_US_GALLON = 0x02, //US gallon
    SCALE_WATER_PULSE_COUNT = 0x03, //Pulse count
    SCALE_WATER_WATER_RESERVED
}ZW_METER_SCALE_WATER;

typedef struct _ZW_METER_STATUS_
{
    BYTE meter_type;
    BYTE scale;
    BYTE rate_type;
    double value;
    WORD delta_time;
    double previous_value;
} ZW_METER_STATUS;

typedef struct _ZW_MANUFACTURER_SPECIFIC_INFO_
{
    WORD manufacturerID;
    WORD productTypeID;
    WORD productID;
}ZW_MANUFACTURER_SPECIFIC_INFO;

typedef union _ZW_DEVICE_STATUS_
{
    ZW_BASIC_STATUS basic_status;
    ZW_BINARYSWITCH_STATUS binaryswitch_status;    // both binaryswith and doubleswitch use this structure
    ZW_DIMMER_STATUS dimmer_status;
    ZW_CURTAIN_STATUS curtain_status;
    ZW_BINARYSENSOR_STATUS binarysensor_status;
    ZW_MULTILEVEL_SENSOR_STATUS multilevelsensor_status;
    ZW_BATTERY_STATUS battery_status;
    ZW_DOORLOCK_STATUS doorlock_status;
    ZW_THERMOSTAT_STATUS thermostat_status;
    ZW_METER_STATUS meter_status;
    ZW_KEYFOB_STATUS keyfob_status;
    ZW_SIREN_STATUS siren_status;
    ZW_MANUFACTURER_SPECIFIC_INFO manufacturer_specific_info;
} ZW_DEVICE_STATUS;


/*
**************zwave configuration header *******************
*/
typedef enum _HSM100_CONFIGURATION_TYPE_
{
    HSM100_CONFIGURATION_TYPE_SENSITIVITY = 1,          /*value range: 0--255; default: 200*/
    HSM100_CONFIGURATION_TYPE_ON_TIME,    /*value range: 1--127; default: 20*/
    HSM100_CONFIGURATION_TYPE_LED_ON_OFF,    /*value range: 0, -1; default: -1*/
    HSM100_CONFIGURATION_TYPE_LIGHT_SHRESHOLD,    /*value range: 0--100; default: 100*/
    HSM100_CONFIGURATION_TYPE_STAY_AWAKE,    /*value range: 0, -1; default: 0*/
    HSM100_CONFIGURATION_TYPE_ON_VALUE,    /*value range: 0, 1--99, -1; default: -1*/
    HSM100_CONFIGURATION_TYPE_TEMP_ADJ,    /*value range: -127--+128*/
    HSM100_CONFIGURATION_TYPE_REPORTS,    /*value range: 0--7; default: 0*/
    HSM100_CONFIGURATION_TYPE_MAX
} HSM100_CONFIGURATION_TYPE;

typedef struct _HSM100_CONFIGURATION_
{
    BYTE parameterNumber;
    BYTE bDefault;
    long value;
} HSM100_CONFIGURATION;

typedef union _ZW_CONFIGURATION_
{
    ZW_DOORLOCK_CONFIGURATION doorlock_configuration;
    HSM100_CONFIGURATION HSM100_configuration;
} ZW_CONFIGURATION;

typedef struct _ZW_DEVICE_CONFIGURATION_
{
    WORD deviceID;
    ZW_DEVICE_TYPE type;
    ZW_CONFIGURATION configuration;
} ZW_DEVICE_CONFIGURATION;

typedef struct _ZW_NODES_CONFIGURATION_
{
    ZW_CONFIGURATION configuration[ZW_MAX_NODES * EXTEND_NODE_CHANNEL];
} ZW_NODES_CONFIGURATION;


/*
**************zwave configuration header end*******************
*/


/*
**************zwave association header*******************
*/

#define SUPPORTED_ASSOCIATED_NODES   8
typedef struct _ZW_ASSOCIATED_NODE_
{
    BYTE nodeID;
} ZW_ASSOCIATED_NODE;

#define SUPPORTED_GROUPINGS    3
typedef struct _ZW_ASSOCIATION_GROUP_
{
    BYTE maxNodesSupported;
    BYTE associatedNodesNum;
    ZW_ASSOCIATED_NODE associatedNodes[SUPPORTED_ASSOCIATED_NODES];
} ZW_ASSOCIATION_GROUP;

typedef struct _ZW_ASSOCIATION_
{
    BYTE supportedGroupings;
    BYTE maxCommandLength;
    BYTE VAndC;
    BYTE configurableSupported;
    WORD freeCommandRecords;
    WORD maxCommandRecords;
    BYTE specificGroup;
    ZW_ASSOCIATION_GROUP grouping[SUPPORTED_GROUPINGS];
} ZW_ASSOCIATION;

typedef struct _ZW_NODES_ASSOCIATION_
{
    ZW_ASSOCIATION association[ZW_MAX_NODES * EXTEND_NODE_CHANNEL];
} ZW_NODES_ASSOCIATION;

typedef struct _ZW_DEVICE_INFO_
{
    /* 
     * Combination of id:
     * 1. id == bSource, phy_id == bSource (binary switch, dimmer)
     * 2. id == bSource | channelEndpointID << 8, phy_id == bSource (motion sensor)
     * 3. id == bSource | myIndex << 8, phy_id == bSource (thermostat)
     */
    WORD id;
    WORD phy_id;
    WORD dev_num;
    ZW_DEVICE_TYPE type;
    ZW_DEVICE_STATUS status;
    /*
     * COMMAND_CLASS range is 0x1 - 0xFF, theoretically one device supports MAX CMDs is 255.
     * Here use 1 bit to indicate 1 CMD, so 32(256/8) Bytes could store all CMDs.
     * cmdclass_bitmask[CMD_VAL - 1) >> 3] |= 0x01 << ((CMD_VAL - 1) & 0x7)
     */
    BYTE cmdclass_bitmask[MAX_CMDCLASS_BITMASK_LEN];
} ZW_DEVICE_INFO;

typedef struct _ZW_NODES_INFO_
{
    WORD nodes_num;
    BYTE nodes_bitmask[MAX_NODEMASK_LENGTH];
    ZW_DEVICE_INFO nodes[ZW_MAX_NODES * EXTEND_NODE_CHANNEL];
} ZW_NODES_INFO;


typedef struct _ZW_DEVICE_INFO_EXCEP_STATUS_
{
    WORD id;
    WORD phy_id;
    WORD dev_num;
    ZW_DEVICE_TYPE type;
    BYTE cmdclass_bitmask[MAX_CMDCLASS_BITMASK_LEN];
} ZW_DEVICE_INFO_EXCEP_STATUS;

typedef struct _ZW_NODES_INFO_EXCEP_STATUS_
{
    WORD nodes_num;
    BYTE nodes_bitmask[MAX_NODEMASK_LENGTH];
    ZW_DEVICE_INFO_EXCEP_STATUS nodes[ZW_MAX_NODES * EXTEND_NODE_CHANNEL];
} ZW_NODES_INFO_EXCEP_STATUS;

typedef enum _ZW_EVENT_TYPE_
{
    ZW_EVENT_TYPE_NODEINFO,
    ZW_EVENT_TYPE_WAKEUP
}ZW_EVENT_TYPE;

typedef struct _ZW_EVENT_REPORT_
{
    ZW_EVENT_TYPE type;
    WORD phy_id;
}ZW_EVENT_REPORT;


extern pthread_mutex_t g_cmd_mutex;
extern pthread_mutex_t g_addremove_mutex;
extern pthread_cond_t g_addremove_cond;

extern ZW_STATUS ZWapi_Init(void (*cbFuncReport)(ZW_DEVICE_INFO *), void (*cbFuncEvent)(ZW_EVENT_REPORT *), ZW_NODES_INFO *devices);
extern void ZWapi_Uninit(void);
extern void ZWapi_GetDeviceInfo(ZW_NODES_INFO *devices);
extern ZW_STATUS ZWapi_Dispatch(BYTE *pFrame, BYTE dataLen);
extern ZW_STATUS ZWapi_SetDefault(void);
extern ZW_STATUS ZWapi_SoftReset(void);
extern ZW_STATUS ZWapi_AddNodeToNetwork(void (*cbFuncAddNodeSuccess)(ZW_DEVICE_INFO *), void (*cbFuncAddNodeFail)(ZW_STATUS));
extern ZW_STATUS ZWapi_StopAddNodeToNetwork(void);
extern ZW_STATUS ZWapi_RemoveNodeFromNetwork(void (*cbFuncRemoveNodeSuccess)(ZW_DEVICE_INFO *), void (*cbFuncRemoveNodeFail)(ZW_STATUS));
extern ZW_STATUS ZWapi_StopRemoveNodeFromNetwork(void);
extern ZW_STATUS ZWapi_SetDeviceStatus(ZW_DEVICE_INFO *device_info);
extern ZW_STATUS ZWapi_GetDeviceStatus(ZW_DEVICE_INFO *device_info);
extern ZW_STATUS ZWapi_isFailedNode(WORD phyNodeID);
extern ZW_STATUS ZWapi_RemoveFailedNodeID(WORD phyNodeID);
extern ZW_STATUS ZWapi_SetDeviceConfiguration(ZW_DEVICE_CONFIGURATION *deviceConfiguration);
extern ZW_STATUS ZWapi_GetDeviceConfiguration(ZW_DEVICE_CONFIGURATION *deviceConfiguration);
extern ZW_STATUS ZWapi_AssociationSet(WORD sourNodeID, WORD desNodeID);
extern ZW_STATUS ZWapi_AssociationRemove(WORD sourNodeID, WORD desNodeID);
extern ZW_STATUS ZWapi_AssociationGet(WORD sourNodeID, BYTE *pAssoNodes, BYTE *num);
extern ZW_STATUS ZWapi_WaitAddRemoveCompleted(struct timespec *ts);
extern ZW_STATUS ZWapi_NotifyAddRemoveCompleted(void);
extern void ZWapi_UpdateDevicesStatus(void);
#ifdef ZWAVE_DRIVER_DEBUG
extern void ZWapi_ShowDriverAttachedDevices(void);
#endif
extern ZW_STATUS ZWapi_UpgradeModule(char *zw_file);

extern void show_device_information(const char *func, const unsigned line, ZW_DEVICE_INFO *deviceinfo);

#endif

