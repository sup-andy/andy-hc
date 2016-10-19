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
 
#ifndef _DECT_DEVICE_H_
#define _DECT_DEVICE_H_
#include <stdio.h>

#define DECT_CMBS_HANDSET_MAX   12
#define DECT_CMBS_HAN_MAX_DEVICES 10
#define DECT_CMBS_DEVICES_MAX (DECT_CMBS_HAN_MAX_DEVICES + DECT_CMBS_HANDSET_MAX)

#define DECT_KEEP_ALIVE_PERIODIC 3600 /* 3600 seconds */
#define DECT_ALERT_PERIODIC 10 /* 10 seconds */
#define DECT_TAMPER_PERIODIC  40

#define  DECT_ONOFF_SWITCHABLE_NAME "ONOFF SWITCHABLE"
#define  DECT_ONOFF_SWITCH_NAME "ONOFF SWITCH"
#define  DECT_LEVEL_CONTROLABLE_NAME "LEVEL CONTROLABLE"
#define  DECT_LEVEL_CONTROL_NAME "LEVEL CONTROL"
#define  DECT_LEVEL_CONTROLABLE_SWITCHABLE_NAME "LEVEL CONTROLABLE SWITCHABLE"
#define  DECT_LEVEL_CONTROL_SWITCH_NAME "LEVEL CONTROL SWITCH"
#define  DECT_AC_OUTLET_NAME "AC OUTLET"
#define  DECT_AC_OUTLET_WITH_POWER_METERING_NAME "AC OUTLET WITH POWER METERING"
#define  DECT_LIGHT_NAME "LIGHT"
#define  DECT_DIMMABLE_LIGHT_NAME "DIMMABLE LIGHT"
#define  DECT_DIMMER_SWITCH_NAME "DIMMER SWITCH"
#define  DECT_DOOR_LOCK_NAME "DOOR LOCK"
#define  DECT_DOOR_BELL_NAME "DOOR BELL"
#define  DECT_POWER_METER_NAME "POWER METER"
#define  DECT_DETECTOR_NAME "DETECTOR"
#define  DECT_DOOR_OPEN_CLOSE_DETECTOR_NAME "DOOR OPEN CLOSE DETECTOR"
#define  DECT_WINDOW_OPEN_CLOSE_DETECTOR_NAME "WINDOW OPEN CLOSE DETECTOR"
#define  DECT_MOTION_DETECTOR_NAME "MOTION DETECTOR"
#define  DECT_SMOKE_DETECTOR_NAME "SMOKE DETECTOR"
#define  DECT_GAS_DETECTOR_NAME "GAS DETECTOR"
#define  DECT_FLOOD_DETECTOR_NAME "FLOOD DETECTOR"
#define  DECT_GLASS_BREAK_DETECTOR_NAME "GLASS BREAK DETECTOR"
#define  DECT_VIBRATION_DETECTOR_NAME "VIBRATION DETECTOR"


#define DECT_DEVICE_NAME_DEF "Unspecified"         /* default device name */
#define DECT_DEVICE_LOCATION_ID_DEF "r0000000"     /* default device location id */


typedef enum {
    DECT_PARAMETER_TYPE_DECTTYPE = 1,
    DECT_PARAMETER_TYPE_BASENAME,
    DECT_PARAMETER_TYPE_PINCODE,
    DECT_PARAMETER_TYPE_MAX
} DECT_PARAMETER_TYPE_E;

typedef enum dect_device_type
{
    DECT_TYPE_NULL,
    /* binary switch */
    DECT_TYPE_ONOFF_SWITCHABLE = 1,
    DECT_TYPE_ONOFF_SWITCH,
    DECT_TYPE_AC_OUTLET,
    DECT_TYPE_LIGHT,
    DECT_TYPE_DOOR_LOCK,
    DECT_TYPE_DOOR_BELL,
    /* multi level switch */
    DECT_TYPE_LEVEL_CONTROLABLE,
    DECT_TYPE_LEVEL_CONTROL,
    DECT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE,
    DECT_TYPE_LEVEL_CONTROL_SWITCH,
    DECT_TYPE_DIMMABLE_LIGHT,
    DECT_TYPE_DIMMER_SWITCH,
    /* meter */
    DECT_TYPE_AC_OUTLET_WITH_POWER_METERING,
    DECT_TYPE_POWER_METER,
    /* binary sensor */
    DECT_TYPE_DETECTOR,
    DECT_TYPE_DOOR_OPEN_CLOSE_DETECTOR,
    DECT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR,
    DECT_TYPE_MOTION_DETECTOR,
    DECT_TYPE_SMOKE_DETECTOR,
    DECT_TYPE_GAS_DETECTOR,
    DECT_TYPE_FLOOD_DETECTOR,
    DECT_TYPE_GLASS_BREAK_DETECTOR,
    DECT_TYPE_VIBRATION_DETECTOR,

    DECT_TYPE_HANDSET,
    DECT_TYPE_VOICE,

    /* DECT_TYPE_EXT_PARAMETER */
    DECT_TYPE_EXT_PARAMETER,
    DECT_TYPE_MAX
} DECT_DEVICE_TYPE;



typedef enum dect_han_device_status
{
    /* binary switch */
    DECT_DEVICE_STATUS_DISCONNECTION = 0,/*disconnection */
    DECT_DEVICE_STATUS_CONNECTION,  /* connection */
    DECT_DEVICE_STATUS_ALARMING,    /* alarming */
    DECT_DEVICE_STATUS_MAX
} DECT_HAN_DEVICE_STATUS;

typedef struct st_dect_cmbs_info
{
    int regester;   /* 0:CLOSED, 1:OPEN */
    int status;
    char app_ver[32];
    char target_ver[32];
    char basename[32];
} DECT_CMBS_INFO;

typedef struct {
    int value;  // 0, idle; 0xff, event detect
} DECT_DEVICE_BINARY_SENSOR_S;

typedef struct {
    char action[256];
} DECT_DEVICE_VOICE_U;

typedef struct {
    int status; /* 0:on-hook, 1:ring, 2:off-hook */
    int value;  /*0:ring, 1:stop ring */
} DECT_DEVICE_HANDSET_U;

typedef struct {
    DECT_PARAMETER_TYPE_E type;
    union {
        int dect_type;
        char pin_code[16];
        char base_station_name[32];
    } u;

} DECT_DEVICE_EXT_STATION_S;

typedef union {
    DECT_DEVICE_BINARY_SENSOR_S binary_sensor;
    DECT_DEVICE_HANDSET_U handset;
    DECT_DEVICE_VOICE_U voice;
    DECT_DEVICE_EXT_STATION_S station;
} DECT_DEVICE_U;

typedef struct _st_dect_hardware_state
{
    unsigned char tamper : 1;
    unsigned char mulfunction : 1;
    unsigned char low_battery : 1;
    unsigned char reserve : 5;
} DECT_HARDWARE_STATE;
typedef struct st_dect_han_device_inf
{
    DECT_DEVICE_TYPE type;
    int id;
    int connection; /* 0:disconnect, 1:connect*/
    char name[64];
    char location[64];
    char ipei[16];
    time_t keepalive;      /* laest keep alive time  */
    time_t alert;
    time_t tamper;
    DECT_HARDWARE_STATE hardware_state;   /* tamper, malfunction, low battery */
    DECT_DEVICE_U device;
    char reserve[256];
} DECT_HAN_DEVICE_INF;

typedef struct st_dect_han_devices
{
    int device_num;
    unsigned int index;
    DECT_HAN_DEVICE_INF device[DECT_CMBS_DEVICES_MAX];
} DECT_HAN_DEVICES;


//extern DECT_HAN_DEVICES_STATUS g_Device_status;
extern DECT_HAN_DEVICES g_Device_info;
#endif  /* _DECT_DEVICE_H_ */

