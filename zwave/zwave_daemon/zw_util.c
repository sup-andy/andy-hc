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
*   File   : zw_util.c
*   Abstract:
*   Date   : 12/18/2014
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Rose  06/03/2013 1.0  Initial Dev.
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "zwave_api.h"

#include "hc_common.h"
#include "zw_util.h"


#define UNKNOWN -1

typedef struct
{
    int hctype;
    int zwtype;
} MAP_TYPE_S;

static MAP_TYPE_S map_binary_switch_type[] = {
    {HC_BINARY_SWITCH_TYPE_RESERVED,            ZW_BINARY_SWITCH_TYPE_RESERVED},
    {HC_BINARY_SWITCH_TYPE_WATER_VALVE,         ZW_BINARY_SWITCH_TYPE_WATER_VALVE},
    {HC_BINARY_SWITCH_TYPE_THERMOSTAT,          ZW_BINARY_SWITCH_TYPE_THERMOSTAT},
    {UNKNOWN,                                   UNKNOWN}
};

static MAP_TYPE_S map_multilevel_switch_type[] = {
    {HC_MULTILEVEL_SWITCH_TYPE_RESERVED,            ZW_BINARY_SWITCH_TYPE_RESERVED},
    {HC_MULTILEVEL_SWITCH_TYPE_CURTAIN,     ZW_BINARY_SWITCH_TYPE_WATER_VALVE},
    {UNKNOWN,                                   UNKNOWN}
};

static MAP_TYPE_S map_binary_sensor_type[] = {
    {HC_BINARY_SENSOR_TYPE_RESERVED,            ZW_BINARY_SENSOR_TYPE_RESERVED},
    {HC_BINARY_SENSOR_TYPE_GENERAL_PURPOSE,     ZW_BINARY_SENSOR_TYPE_GENERAL_PURPOSE},
    {HC_BINARY_SENSOR_TYPE_SMOKE,               ZW_BINARY_SENSOR_TYPE_SMOKE},
    {HC_BINARY_SENSOR_TYPE_CO,                  ZW_BINARY_SENSOR_TYPE_CO},
    {HC_BINARY_SENSOR_TYPE_CO2,                 ZW_BINARY_SENSOR_TYPE_CO2},
    {HC_BINARY_SENSOR_TYPE_HEAT,                ZW_BINARY_SENSOR_TYPE_HEAT},
    {HC_BINARY_SENSOR_TYPE_WATER,               ZW_BINARY_SENSOR_TYPE_WATER},
    {HC_BINARY_SENSOR_TYPE_FREEZE,              ZW_BINARY_SENSOR_TYPE_FREEZE},
    {HC_BINARY_SENSOR_TYPE_TAMPER,              ZW_BINARY_SENSOR_TYPE_TAMPER},
    {HC_BINARY_SENSOR_TYPE_AUX,                 ZW_BINARY_SENSOR_TYPE_AUX},
    {HC_BINARY_SENSOR_TYPE_DOOR_WINDOW,         ZW_BINARY_SENSOR_TYPE_DOOR_WINDOW},
    {HC_BINARY_SENSOR_TYPE_TILT,                ZW_BINARY_SENSOR_TYPE_TILT},
    {HC_BINARY_SENSOR_TYPE_MOTION,              ZW_BINARY_SENSOR_TYPE_MOTION},
    {HC_BINARY_SENSOR_TYPE_GLASS_BREAK,         ZW_BINARY_SENSOR_TYPE_GLASS_BREAK},
    {HC_BINARY_SENSOR_TYPE_RETURN_1ST_SENSOR,   ZW_BINARY_SENSOR_TYPE_RETURN_1ST_SENSOR},
    {UNKNOWN,                                   UNKNOWN}
};

static MAP_TYPE_S map_multi_sensor_type[] = {
    {HC_MULTILEVEL_SENSOR_TYPE_RESERVED,                ZW_MULTILEVEL_SENSOR_TYPE_RESERVED},
    {HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE,         ZW_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE},
    {HC_MULTILEVEL_SENSOR_TYPE_GENERAL_PURPOSE_VALUE,   ZW_MULTILEVEL_SENSOR_TYPE_GENERAL_PURPOSE_VALUE},
    {HC_MULTILEVEL_SENSOR_TYPE_LUMINANCE,               ZW_MULTILEVEL_SENSOR_TYPE_LUMINANCE},
    {HC_MULTILEVEL_SENSOR_TYPE_POWER,                   ZW_MULTILEVEL_SENSOR_TYPE_POWER},
    {HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY,                ZW_MULTILEVEL_SENSOR_TYPE_HUMIDITY},
    {HC_MULTILEVEL_SENSOR_TYPE_VELOCITY,                ZW_MULTILEVEL_SENSOR_TYPE_VELOCITY},
    {HC_MULTILEVEL_SENSOR_TYPE_DIRECTION,               ZW_MULTILEVEL_SENSOR_TYPE_DIRECTION},
    {HC_MULTILEVEL_SENSOR_TYPE_ATMOSPHERIC_PRESSURE,    ZW_MULTILEVEL_SENSOR_TYPE_ATMOSPHERIC_PRESSURE},
    {HC_MULTILEVEL_SENSOR_TYPE_BAROMETRIC_PRESSURE,     ZW_MULTILEVEL_SENSOR_TYPE_BAROMETRIC_PRESSURE},
    {HC_MULTILEVEL_SENSOR_TYPE_SOLAR_RADIATION,         ZW_MULTILEVEL_SENSOR_TYPE_SOLAR_RADIATION},
    {HC_MULTILEVEL_SENSOR_TYPE_DEW_POINT,               ZW_MULTILEVEL_SENSOR_TYPE_DEW_POINT},
    {HC_MULTILEVEL_SENSOR_TYPE_RAIN_RATE,               ZW_MULTILEVEL_SENSOR_TYPE_RAIN_RATE},
    {HC_MULTILEVEL_SENSOR_TYPE_TIDE_LEVEL,              ZW_MULTILEVEL_SENSOR_TYPE_TIDE_LEVEL},
    {HC_MULTILEVEL_SENSOR_TYPE_WEIGHT,                  ZW_MULTILEVEL_SENSOR_TYPE_WEIGHT},
    {HC_MULTILEVEL_SENSOR_TYPE_VOLTAGE,                 ZW_MULTILEVEL_SENSOR_TYPE_VOLTAGE},
    {HC_MULTILEVEL_SENSOR_TYPE_CURRENT,                 ZW_MULTILEVEL_SENSOR_TYPE_CURRENT},
    {HC_MULTILEVEL_SENSOR_TYPE_CO2_LEVEL,               ZW_MULTILEVEL_SENSOR_TYPE_CO2_LEVEL},
    {HC_MULTILEVEL_SENSOR_TYPE_AIR_FLOW,                ZW_MULTILEVEL_SENSOR_TYPE_AIR_FLOW},
    {HC_MULTILEVEL_SENSOR_TYPE_TANK_CAPACITY,           ZW_MULTILEVEL_SENSOR_TYPE_TANK_CAPACITY,},
    {HC_MULTILEVEL_SENSOR_TYPE_DISTANCE,                ZW_MULTILEVEL_SENSOR_TYPE_DISTANCE,},
    {HC_MULTILEVEL_SENSOR_TYPE_ANGLE_POSITION,          ZW_MULTILEVEL_SENSOR_TYPE_ANGLE_POSITION},
    {HC_MULTILEVEL_SENSOR_TYPE_ROTATION,                ZW_MULTILEVEL_SENSOR_TYPE_ROTATION},
    {HC_MULTILEVEL_SENSOR_TYPE_WATER_TEMPERATURE,       ZW_MULTILEVEL_SENSOR_TYPE_WATER_TEMPERATURE},
    {HC_MULTILEVEL_SENSOR_TYPE_SOIL_TEMPERATURE,        ZW_MULTILEVEL_SENSOR_TYPE_SOIL_TEMPERATURE, },
    {HC_MULTILEVEL_SENSOR_TYPE_SEISMIC_INTENSITY,       ZW_MULTILEVEL_SENSOR_TYPE_SEISMIC_INTENSITY},
    {HC_MULTILEVEL_SENSOR_TYPE_SEISMIC_MAGNITUDE,       ZW_MULTILEVEL_SENSOR_TYPE_SEISMIC_MAGNITUDE},
    {HC_MULTILEVEL_SENSOR_TYPE_ULTRAVIOLET,             ZW_MULTILEVEL_SENSOR_TYPE_ULTRAVIOLET},
    {HC_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_RESISTIVITY,  ZW_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_RESISTIVITY},
    {HC_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_CONDUCTIVITY, ZW_MULTILEVEL_SENSOR_TYPE_ELECTRICAL_CONDUCTIVITY},
    {HC_MULTILEVEL_SENSOR_TYPE_LOUDNESS,                ZW_MULTILEVEL_SENSOR_TYPE_LOUDNESS},
    {HC_MULTILEVEL_SENSOR_TYPE_MOISTURE,                ZW_MULTILEVEL_SENSOR_TYPE_MOISTURE},
    {UNKNOWN,                                           UNKNOWN}
};

static MAP_TYPE_S map_meter_type[] = {
    {HC_METER_TYPE_RESERVED,                        0},
    {HC_METER_TYPE_SINGLE_E_ELECTRIC,               ZW_METER_TYPE_SINGLE_E_ELECTRIC},
    {HC_METER_TYPE_GAS,                             ZW_METER_TYPE_GAS},
    {HC_METER_TYPE_WATER,                           ZW_METER_TYPE_WATER},
    {HC_METER_TYPE_TWIN_E_ELECTRIC,                 ZW_METER_TYPE_TWIN_E_ELECTRIC},
    {HC_METER_TYPE_3P_SINGLE_DIRECT_ELECTRIC,       ZW_METER_TYPE_3P_SINGLE_DIRECT_ELECTRIC},
    {HC_METER_TYPE_3P_SINGLE_ECT_ELECTRIC,          ZW_METER_TYPE_3P_SINGLE_ECT_ELECTRIC},
    {HC_METER_TYPE_1_PHASE_DIRECT_ELECTRIC,         ZW_METER_TYPE_1_PHASE_DIRECT_ELECTRIC},
    {HC_METER_TYPE_HEATING,                         ZW_METER_TYPE_HEATING},
    {HC_METER_TYPE_COOLING,                         ZW_METER_TYPE_COOLING},
    {HC_METER_TYPE_COMBINED_HEATING_AND_COOLING,    ZW_METER_TYPE_COMBINED_HEATING_AND_COOLING},
    {HC_METER_TYPE_ELECTRIC_SUB_METER,              ZW_METER_TYPE_ELECTRIC_SUB_METER},
    {UNKNOWN,                                       UNKNOWN}
};

int map_bin_sensor_hctype(int zwtype)
{
    MAP_TYPE_S *map = &map_binary_sensor_type[0];

    while (map->zwtype != UNKNOWN && map->zwtype != zwtype)
    {
        map++;
    }

    return map->hctype;
}

int map_bin_sensor_zwtype(int hctype)
{
    MAP_TYPE_S *map = &map_binary_sensor_type[0];

    while (map->hctype != UNKNOWN && map->hctype != hctype)
    {
        map++;
    }

    return map->zwtype;
}


int map_multi_sensor_hctype(int zwtype)
{
    MAP_TYPE_S *map = &map_multi_sensor_type[0];

    while (map->zwtype != UNKNOWN && map->zwtype != zwtype)
    {
        map++;
    }

    return map->hctype;
}

int map_multi_sensor_zwtype(int hctype)
{
    MAP_TYPE_S *map = &map_multi_sensor_type[0];

    while (map->hctype != UNKNOWN && map->hctype != hctype)
    {
        map++;
    }

    return map->zwtype;
}

int map_meter_hctype(int zwtype)
{
    MAP_TYPE_S *map = &map_meter_type[0];

    while (map->zwtype != UNKNOWN && map->zwtype != zwtype)
    {
        map++;
    }

    return map->hctype;
}

int map_meter_zwtype(int hctype)
{
    MAP_TYPE_S *map = &map_meter_type[0];

    while (map->hctype != UNKNOWN && map->hctype != hctype)
    {
        map++;
    }

    return map->zwtype;
}

int map_binary_switch_hctype(int zwtype)
{
    MAP_TYPE_S *map = &map_binary_switch_type[0];

    while (map->zwtype != UNKNOWN && map->zwtype != zwtype)
    {
        map++;
    }

    return map->hctype;
}

int map_binary_switch_zwtype(int hctype)
{
    MAP_TYPE_S *map = &map_binary_switch_type[0];

    while (map->hctype != UNKNOWN && map->hctype != hctype)
    {
        map++;
    }

    return map->zwtype;
}


int start_thread(pthread_t *ptid, int priority, void * (*func)(void *), void *data)
{
    pthread_attr_t  attr;

    if ((pthread_attr_init(&attr) != 0))
    {
        return -1;
    }

    if ((pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0))
    {
        return -1;
    }

    if (priority > 0)
    {

        struct sched_param sched;
        sched.sched_priority = priority;

        if ((pthread_attr_setschedparam(&attr, &sched) != 0))
        {
            return -1;
        }
    }

    if ((pthread_create(ptid, &attr, func, data) != 0))
    {
        return -1;
    }

    return 0;
}


int stop_thread(pthread_t ptid)
{
    int ret;

    pthread_cancel(ptid);

    ret = pthread_join(ptid, NULL);
    if (ret == 0)
    {
        DEBUG_INFO("Stop the pthread %u ok\n", ptid);
    }
    else
    {
        DEBUG_ERROR("Stop the pthread %u failed\n", ptid);
    }

    return ret;
}

int set_nonblocking(int sock)
{
    int opts = 0;

    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        return -1;
    }

    if (fcntl(sock, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        return -1;
    }

    return 0;
}

int double_equal(double a, double b)
{
	return (fabs(a-b) < 0.000001);
}


