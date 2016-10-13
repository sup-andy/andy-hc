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
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pthread.h>

#include "zwave_api.h"
#include "zwave_uart.h"
#include "zwave_tool.h"

// just for compile
#if (ZWAVE_DRIVER_DEBUG == 0)
int serial_send(unsigned char *buffer, int length)
{
    return 0;
}
#endif

void help(char *str)
{
	if(strcasecmp(str, "show") == 0)
	{
		fprintf(stderr, "zwave tool show added nodes information usage: \n\n"
			"usage: show \n\n");
	}
	else if(strcasecmp(str, "add") == 0)
	{
		fprintf(stderr, "zwave tool add node usage: \n\n"
			"usage: add \n\n");
	}
	else if(strcasecmp(str, "remove") == 0)
	{
		fprintf(stderr, "zwave tool remove node usage: \n\n"
			"usage: remove \n\n");
	}
	else if(strcasecmp(str, "gs") == 0)
	{
		fprintf(stderr, "zwave tool get node status usage: \n\n"
			"usage: gs [NODE TYPE] [NODE ID] ... \n"
			"[NODE TYPE]: \n"
			"\t  binaryswitch \n"
			"\t\t usage: gs binaryswitch [NODE ID] \n\n"
			"\t  dimmer \n"
			"\t\t usage: gs dimmer [NODE ID] \n\n"
			"\t  curtain \n"
			"\t\t usage: gs curtain [NODE ID] \n\n"
			"\t  binarysensor \n"
			"\t\t usage: gs binarysensor [NODE ID] \n\n"
			"\t  multilevelsensor \n"
			"\t\t usage: gs multilevelsensor [NODE ID] \n\n"
			"\t  doorlock \n"
			"\t\t usage: gs doorlock [NODE ID] \n\n"
			"\t  thermostat \n"
			"\t\t usage: gs thermostat [NODE ID] \n\n"
			"\t  meter \n"
			"\t\t usage: gs meter [NODE ID] [SCALE] \n\n"
			"\t  battery \n"
			"\t\t usage: gets battery [NODE ID] \n\n");
	}
	else if(strcasecmp(str, "ss") == 0)
	{
		fprintf(stderr, "zwave tool set node status usage: \n\n"
			"usage: ss [NODE TYPE] [NODE ID] ... \n"
			"[NODE TYPE]: \n"
			"\t  binaryswitch \n"
			"\t\t usage: ss binaryswitch [NODE ID] [VALUE] \n\n"
			"\t  dimmer \n"
			"\t\t usage: ss dimmer [NODE ID] [VALUE] \n\n"
			"\t  curtain \n"
			"\t\t usage: ss curtain [NODE ID] [VALUE] \n\n"
			"\t  doorlock \n"
			"\t\t usage: ss doorlock [NODE ID] [MODE] \n\n"
			"\t  thermostat \n"
			"\t\t usage: ss thermostat [NODE ID] [MODE] [VALUE]\n\n"
			"\t  battery \n"
			"\t\t usage: ss battery [NODE ID] [INTERVAL TIME] \n\n");
	}
	else if(strcasecmp(str, "tf") == 0)
	{
		fprintf(stderr, "zwave tool test if failed node or not usage: \n\n"
			"usage: tf [PHYSICAL ID] \n\n");
	}
	else if(strcasecmp(str, "rf") == 0)
	{
		fprintf(stderr, "zwave tool remove failed node usage: \n\n"
			"usage: rf [PHYSICAL ID] \n\n");
	}
	else if(strcasecmp(str, "gc") == 0)
	{
		fprintf(stderr, "zwave tool get configuration usage: \n\n"
			"usage: gc [PHYSICAL ID] [NODE TYPE] ...\n"
			"[NODE TYPE]: \n"
			"\t  doorlock \n"
			"\t\t usage: gc doorlock [PHYSICAL ID] \n"
			"\t  hsm100 \n "
			"\t\t usage: gc hsm100 [PHYSICAL ID] [PARAMETER NUMBER] \n\n");
	}
	else if(strcasecmp(str, "sc") == 0)
	{
		fprintf(stderr, "zwave tool set configuration usage: \n\n"
			"usage: sc [PHYSICAL ID] [NODE TYPE] ...\n"
			"[NODE TYPE]: \n"
			"\t  doorlock \n"
			"\t\t usage: sc doorlock [PHYSICAL ID] [OPERATION TYPE] [MODE] [MINUTES] [SECONDS] \n"
			"\t  hsm100 \n "
			"\t\t usage: sc hsm100 [PHYSICAL ID] [PARAMETER NUMBER] [VALUE] \n\n");
	}
	else if(strcasecmp(str, "ga") == 0)
	{
		fprintf(stderr, "zwave tool get association usage: \n\n"
			"usage: ga [PHYSICAL ID] \n\n");
	}
	else if(strcasecmp(str, "sa") == 0)
	{
		fprintf(stderr, "zwave tool set association usage: \n\n"
			"usage: sa [PHYSICAL ID] [NODE ID] \n\n");
	}
	else if(strcasecmp(str, "ra") == 0)
	{
		fprintf(stderr, "zwave tool remove association usage: \n\n"
			"usage: ra [PHYSICAL ID] [NODE ID] \n\n");
	}
	else if(strcasecmp(str, "default") == 0)
	{
		fprintf(stderr, "zwave tool set controller default usage: \n\n"
			"usage: default \n\n");
	}
	else
	{
		fprintf(stderr, "zwave tool usage:\n\n"
			"usage: [OPERATION TYPE] ... \n"
			"[OPERATION TYPE]: \n"
			"\t show: show added nodes \n"
			"\t add: add node to network \n"
			"\t remove: remove node from network \n"
			"\t gs: get status \n"
			"\t ss: set status \n"
			"\t tf: test is failed node \n"
			"\t rf: remove failed node \n"
			"\t gc: get configuration \n"
			"\t sc: set configuration \n"
			"\t ga: get association \n"
			"\t sa: set association \n"
			"\t ra: remove association \n"
			"\t default: set default \n"
			"\t help: help \n"
			"\t\t help usage: help [OPERATION TYPE] \n\n");
	}

}

void CBFuncReport(ZW_DEVICE_INFO *deviceinfo)
{
    if (!deviceinfo)
        return;

    switch (deviceinfo->type)
    {
        case ZW_DEVICE_BINARYSWITCH:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (binaryswitch), value: %u \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.binaryswitch_status.value);
            break;
        case ZW_DEVICE_DIMMER:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (dimmer), value: %u \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.dimmer_status.value);
            break;
        case ZW_DEVICE_CURTAIN:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (curtain), value: %u \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.dimmer_status.value);
            break;
        case ZW_DEVICE_BINARYSENSOR:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (binarysensor), sub-type: %u, value: %u \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.binarysensor_status.sensor_type, deviceinfo->status.binarysensor_status.value);
            break;
        case ZW_DEVICE_MULTILEVEL_SENSOR:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (multilevelsensor), sub-type: %u, value: %f \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.multilevelsensor_status.sensor_type, deviceinfo->status.multilevelsensor_status.value);
            break;
        case ZW_DEVICE_BATTERY:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (battery), value: %u \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.battery_status.battery_level);
            break;
        case ZW_DEVICE_DOORLOCK:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (doorlock), mode: %u \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.doorlock_status.doorLockMode);
            break;
        case ZW_DEVICE_THERMOSTAT:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (thermostat), mode: %u, value: %f \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.thermostat_status.mode, deviceinfo->status.thermostat_status.value);
            break;
        case ZW_DEVICE_METER:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (meter), sub-type: %u, value: %f \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.meter_status.meter_type, deviceinfo->status.meter_status.value);
            break;
        default:
            fprintf(stderr, "ID:%u, PhyID:%u, Type:%u (unknow type) \n",
                       deviceinfo->id, deviceinfo->phy_id, deviceinfo->type);
            break;
    }

    return;
}

void CBFuncEvent(ZW_EVENT_REPORT *event)
{
    fprintf(stderr, "event report. type[%d] phy id[0x%x] \n", event->type, event->phy_id);
}

void CBFuncAddNodeSuccess(ZW_DEVICE_INFO *deviceInfo)
{
    fprintf(stderr, "New Node. id[0x%x] phy id[0x%x] type[%u] \n", deviceInfo->id, deviceInfo->phy_id, deviceInfo->type);
}

void CBFuncAddNodeFail(ZW_STATUS st)
{
    fprintf(stderr, "Add Node fail. reason[%d] \n", st);
}

void CBFuncRemoveNodeSuccess(ZW_DEVICE_INFO *deviceInfo)
{
    fprintf(stderr, "Remove Node. id[0x%x] phy id[0x%x] type[%u] \n", deviceInfo->id, deviceInfo->phy_id, deviceInfo->type);
}

void CBFuncRemoveNodeFail(ZW_STATUS st)
{
    fprintf(stderr, "Remove Node fail. reason[%d] \n", st);
}

int main(int argc, char *argv[])
{
    int fd = 0;
    pthread_t pthread_rx_process_id;
    char input[1024] = {0};
    char param[64][128];
    int n = 0;
    char *p = NULL;
    
    ZW_STATUS ret = ZW_STATUS_FAIL;
    ZW_DEVICE_INFO deviceinfo;
    ZW_DEVICE_CONFIGURATION deviceconfiguration;
	
    if(argc < 2)
    {
        fprintf(stderr, "Select correct zwave device, please! \n\t usage: zwave_tool [ZWAVE DEVICE] \n");
        return 0;
    }

    if((fd = ZW_UART_open(argv[1])) < 0)
    {
        fprintf(stderr, "Select correct zwave device, please! \n\t usage: zwave_tool [ZWAVE DEVICE] \n");
        return 0;
    }

    if(pthread_create(&pthread_rx_process_id, NULL, *ZW_UART_rx, (void *)&fd))
    {
        fprintf(stderr, "pthread_create pthread_rx_process error \n");
        return 0;
    }

    if(ZWapi_Init(CBFuncReport, CBFuncEvent, NULL) != ZW_STATUS_OK)
    {
        fprintf(stderr, "zwave initialization fail \n");
        return 0;
    }

    fprintf(stderr, "\n \t Now, zwave_tool is ready... \n\n");

    while(1)
    {
        memset(input, 0, sizeof(input));
        if(fgets(input, sizeof(input) - 1, stdin) == NULL)
        {
            fprintf(stderr, "read data error \n");
            continue;
        }
        input[strlen(input) - 1] = '\0';
        memset(param, 0, sizeof(param));
        n = 0;
        p = strtok(input, " ");
        while(p)
        {
            strncpy(param[n], p, sizeof(param[n]) -1);
            fprintf(stderr, "param[%d]: %s \n", n, param[n]);
            n++;
            p = strtok(NULL, " ");
        }
        if(n <= 0)
        {
            continue;
        }

        if(strcasecmp(param[0], "show") == 0)
        {
            ZWapi_ShowDriverAttachedDevices();
            continue;
        }
        else if(strcasecmp(param[0], "add") == 0)
        {
            ret = ZWapi_AddNodeToNetwork(CBFuncAddNodeSuccess, CBFuncAddNodeFail);
        }
        else if(strcasecmp(param[0], "remove") == 0)
        {
            ret = ZWapi_RemoveNodeFromNetwork(CBFuncRemoveNodeSuccess, CBFuncRemoveNodeFail);
        }
        else if(strcasecmp(param[0], "gs") == 0)
        {
            if(n < 3)
            {
                help("gs");
                continue;
            }
            memset(&deviceinfo, 0, sizeof(deviceinfo));
            if(strcasecmp(param[1], "meter") == 0)
            {
                if(n < 4)
                {
                    help("gs");
                    continue;
                }
                else
                {
                    deviceinfo.status.meter_status.scale = atoi(param[3]);
                }
            }
            else
            {
                if(strcasecmp(param[1], "binaryswitch") != 0
                    &&strcasecmp(param[1], "dimmer") != 0
                    &&strcasecmp(param[1], "curtain") != 0
                    &&strcasecmp(param[1], "binarysensor") != 0
                    &&strcasecmp(param[1], "multilevelsensor") != 0
                    &&strcasecmp(param[1], "doorlock") != 0
                    &&strcasecmp(param[1], "thermostat") != 0
                    &&strcasecmp(param[1], "battery") != 0)
                {
                    help("gs");
                    continue;
                }
            }
            deviceinfo.id = atoi(param[2]);
            ret = ZWapi_GetDeviceStatus(&deviceinfo);
        }
        else if(strcasecmp(param[0], "ss") == 0)
        {
            if(n < 4)
            {
                help("ss");
                continue;
            }
            
            memset(&deviceinfo, 0, sizeof(deviceinfo));
            
            if(strcasecmp(param[1], "binaryswitch") == 0)
            {
                deviceinfo.status.binaryswitch_status.value = atoi(param[3]);
                deviceinfo.type = ZW_DEVICE_BINARYSWITCH;
            }
            else if(strcasecmp(param[1], "dimmer") == 0)
            {
                deviceinfo.status.dimmer_status.value = atoi(param[3]);
                deviceinfo.type = ZW_DEVICE_DIMMER;
            }
            else if(strcasecmp(param[1], "curtain") == 0)
            {
                deviceinfo.status.curtain_status.value = atoi(param[3]);
                deviceinfo.type = ZW_DEVICE_CURTAIN;
            }
            else if(strcasecmp(param[1], "doorlock") == 0)
            {
                deviceinfo.status.doorlock_status.doorLockMode = atoi(param[3]);
                deviceinfo.type = ZW_DEVICE_DOORLOCK;
            }
            else if(strcasecmp(param[1], "thermostat") == 0)
            {
                if(n < 5)
                {
                    help("ss");
                    continue;
                }
                else
                {
                    deviceinfo.status.thermostat_status.mode = atoi(param[3]);
                    deviceinfo.status.thermostat_status.value = atof(param[4]);
                    deviceinfo.type = ZW_DEVICE_THERMOSTAT;
                }
            }
            else if(strcasecmp(param[0], "battery") == 0)
            {
                deviceinfo.status.battery_status.battery_level = atoi(param[3]);
                deviceinfo.type = ZW_DEVICE_BATTERY;
            }
            else
            {
                help("ss");
                continue;
            }
            deviceinfo.id = atoi(param[2]);
            ret = ZWapi_SetDeviceStatus(&deviceinfo);
        }
        else if(strcasecmp(param[0], "tf") == 0)
        {
            if(n < 2)
            {
                help("tf");
                continue;
            }

            ret = ZWapi_isFailedNode(atoi(param[1]));
        }
        else if(strcasecmp(param[0], "rf") == 0)
        {
            if(n < 2)
            {
                help("rf");
                continue;
            }

            ret = ZWapi_RemoveFailedNodeID(atoi(param[1]));
        }
        else if(strcasecmp(param[0], "gc") == 0)
        {
            if(n < 3)
            {
                help("gc");
                continue;
            }
            
            memset(&deviceconfiguration, 0, sizeof(deviceconfiguration));
            deviceconfiguration.deviceID = atoi(param[2]);
            if(strcasecmp(param[1], "doorlock") == 0)
            {
                deviceconfiguration.type = ZW_DEVICE_DOORLOCK;
                ret = ZWapi_GetDeviceConfiguration(&deviceconfiguration);
            }
            else if(strcasecmp(param[1], "hsm100") == 0)
            {
                if(n < 4)
                {
                    help("gc");
                    continue;
                }

                deviceconfiguration.type = ZW_DEVICE_MULTILEVEL_SENSOR;
                deviceconfiguration.configuration.HSM100_configuration.parameterNumber = atoi(param[3]);
                ret = ZWapi_GetDeviceConfiguration(&deviceconfiguration);
            }
            else
            {
                help("gc");
                continue;
            }
        }
        else if(strcasecmp(param[0], "sc") == 0)
        {
            if(n < 3)
            {
                help("sc");
                continue;
            }
            
            if(strcasecmp(param[1], "doorlock") == 0)
            {
                if(n < 7)
                {
                    help("sc");
                    continue;
                }
                deviceconfiguration.deviceID = atoi(param[2]);
                deviceconfiguration.type = ZW_DEVICE_DOORLOCK;
                deviceconfiguration.configuration.doorlock_configuration.operationType = atoi(param[3]);
                deviceconfiguration.configuration.doorlock_configuration.doorHandlesMode = atoi(param[4]);
                deviceconfiguration.configuration.doorlock_configuration.lockTimeoutMinutes = atoi(param[5]);
                deviceconfiguration.configuration.doorlock_configuration.lockTimeoutSeconds = atoi(param[6]);
                ret = ZWapi_SetDeviceConfiguration(&deviceconfiguration);
            }
            else if(strcasecmp(param[1], "hsm100") == 0)
            {
                if(n < 5)
                {
                    help("sc");
                    continue;
                }
                deviceconfiguration.deviceID = atoi(param[2]);
                deviceconfiguration.type = ZW_DEVICE_MULTILEVEL_SENSOR;
                deviceconfiguration.configuration.HSM100_configuration.parameterNumber = atoi(param[3]);
                deviceconfiguration.configuration.HSM100_configuration.value = atof(param[4]);
                ret = ZWapi_SetDeviceConfiguration(&deviceconfiguration);
            }
            else
            {
                help("sc");
                continue;
            }
        }
        else if(strcasecmp(param[0], "ga") == 0)
        {
            if(n < 2)
            {
                help("ga");
                continue;
            }
            int nodes_num = 0;
            char node_ids[256] = {0};
            memset(node_ids, 0, sizeof(node_ids));
            ret = ZWapi_AssociationGet(atoi(param[1]), node_ids, &nodes_num);
        }
        else if(strcasecmp(param[0], "sa") == 0)
        {
            if(n < 3)
            {
                help("sa");
                continue;
            }
            ret = ZWapi_AssociationSet(atoi(param[1]), atoi(param[2]));
        }
        else if(strcasecmp(param[0], "ra") == 0)
        {
            if(n < 3)
            {
                help("ra");
                continue;
            }
            ret = ZWapi_AssociationRemove(atoi(param[1]), atoi(param[2]));
        }
        else if(strcasecmp(param[0], "default") == 0)
        {
            ret = ZWapi_SetDefault();
        }
        else if(strcasecmp(param[0], "help") == 0)
        {
            if(n < 2)
            {
                help("help");
                continue;
            }
        
            help(param[1]);
            
            continue;
        }
        else
        {
            help("help");
            continue;
        }
        
    }

    return 0;
}


