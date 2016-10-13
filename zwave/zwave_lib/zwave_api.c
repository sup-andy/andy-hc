
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "zwave_api.h"
#include "zwave_appl.h"
#include "zwave_serialapi.h"
#include "zwave_security_aes.h"
#include "zwave_association.h"
#include "zwave_spi_ioctl.h"

#define ZWAVE_RESET_LOW "echo 0 > /sys/class/gpio/gpio67/value"
#define ZWAVE_RESET_HI  "echo 1 > /sys/class/gpio/gpio67/value"

extern int hex2bin(char *Flnm, char *bin);

pthread_t timer_thread_id = 0;
pthread_t msgProcess_thread_id = 0;
pthread_t msgProcess2_thread_id = 0;

pthread_mutex_t g_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_addremove_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_addremove_cond = PTHREAD_COND_INITIALIZER;

ZW_STATUS ControllerDataInit(void)
{
    BYTE ver, capabilities, len, nodes[29] = {0}, chip_type, chip_ver;
    DWORD i = 0, j = 0, k = 0;
    WORD nodeID = 0;
    NODEINFO nodeInfo;

    /* 1. get nodes(bitmask) from controller. */
    if (ZW_GetInitData(&ver, &capabilities, &len, nodes, &chip_type, &chip_ver) != ZW_STATUS_OK)
    {
        return ZW_STATUS_FAIL;
    }

    memcpy(zw_nodes->nodes_bitmask, nodes, MAX_NODEMASK_LENGTH);

    for (i = 0; i < MAX_NODEMASK_LENGTH; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (zw_nodes->nodes_bitmask[i] & (0x01 << j))
            {
                nodeID = i * 8 + j + 1;
                /* 2. get nodeInfo by nodeID from controller, except command class. */
                if (ZW_GetNodeProtocolInfo(nodeID, &nodeInfo) == ZW_STATUS_OK)
                {
                    zw_nodes->nodes[nodeID].id = nodeID;
                    zw_nodes->nodes[nodeID].type = Analysis_DeviceType(&nodeInfo);

                    if (nodeID > 1)
                    {
                        /* 3. get device node info from device. */
                        ZW_RequestNodeInfo(nodeID, NULL);
                    }

                    GenVirtualSubNode(nodeID);

                    for (k = 0; k < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; k++)
					{
						if (zw_nodes->nodes[k].type
								&& ((k & 0xff) == nodeID))
						{
							zw_nodes->nodes[k].phy_id = nodeID;
							zw_nodes->nodes[k].dev_num = zw_nodes->nodes[nodeID].dev_num;
							if(zw_nodes->nodes[k].id)
								zw_nodes->nodes_num++;
#ifdef ZWAVE_DRIVER_DEBUG
                                                DEBUG_ERROR("[ZWave Driver] add node [id: %d, type: %d]\r\n", zw_nodes->nodes[k].id, zw_nodes->nodes[k].type);
#endif
						}
					}

                }

            }
        }
    }

    return ZW_STATUS_OK;
}



ZW_STATUS ZWapi_Init(void (*cbFuncReport)(ZW_DEVICE_INFO *), void (*cbFuncEvent)(ZW_EVENT_REPORT *), ZW_NODES_INFO *devices)
{

    ZWapi_Uninit();

    zw_nodes = (ZW_NODES_INFO *)malloc(sizeof(ZW_NODES_INFO));
    if (!zw_nodes)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] malloc zw_nodes error\n");
#endif
        return ZW_STATUS_FAIL;
    }
    memset(zw_nodes, 0, sizeof(ZW_NODES_INFO));

    zw_nodes_association = (ZW_NODES_ASSOCIATION *)malloc(sizeof(ZW_NODES_ASSOCIATION));
    if (!zw_nodes_association)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] malloc zw_nodes_association error \n");
#endif
        goto error_handle;
    }
    memset(zw_nodes_association, 0, sizeof(ZW_NODES_ASSOCIATION));

    zw_nodes_configuration = (ZW_NODES_CONFIGURATION *)malloc(sizeof(ZW_NODES_CONFIGURATION));
    if (!zw_nodes_configuration)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] malloc zw_nodes_configuration error \n");
#endif
        goto error_handle;
    }
    memset(zw_nodes_configuration, 0, sizeof(ZW_NODES_CONFIGURATION));

    StatusUpdateTimer = (ZW_NODE_STATUS_UPDATE_TIMER *)malloc(sizeof(ZW_NODE_STATUS_UPDATE_TIMER));
    if (!StatusUpdateTimer)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] malloc StatusUpdateTimer error \n");
#endif
        goto error_handle;
    }
    memset(StatusUpdateTimer, 0, sizeof(ZW_NODE_STATUS_UPDATE_TIMER));

    cbFuncReportHandle = cbFuncReport;
    cbFuncEventHandler = cbFuncEvent;

    if (pthread_create(&msgProcess_thread_id, NULL, &ZW_msgProcess, NULL))
    {
        goto error_handle;
    }

    if (pthread_create(&msgProcess2_thread_id, NULL, &ZW_msgProcess2, NULL))
    {
        goto error_handle;
    }

    if (pthread_create(&timer_thread_id, NULL, &ZW_timerInit, NULL))
    {
        goto error_handle;
    }

    if (GetNodesInfo(zw_nodes) != ZW_STATUS_OK)
    {
        if (ControllerDataInit() != ZW_STATUS_OK)
        {
#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_ERROR("[ZWave Driver] Get Z-Wave controller data error!\n");
#endif
            goto error_handle;
        }
        SaveNodesInfo(zw_nodes);
    }

    InitSecurity();

    if (devices)
    {
        memcpy(devices, zw_nodes, sizeof(ZW_NODES_INFO));
    }

    ZWapi_UpdateDevicesStatus();

    return ZW_STATUS_OK;

error_handle:
    if (StatusUpdateTimer != NULL)
    {
        free(StatusUpdateTimer);
        StatusUpdateTimer = NULL;
    }

    if (zw_nodes_configuration != NULL)
    {
        free(zw_nodes_configuration);
        zw_nodes_configuration = NULL;
    }

    if (zw_nodes_association != NULL)
    {
        free(zw_nodes_association);
        zw_nodes_association = NULL;
    }

    if (zw_nodes != NULL)
    {
        free(zw_nodes);
        zw_nodes = NULL;
    }

    return ZW_STATUS_FAIL;
}

void ZWapi_Uninit(void)
{
    if (msgProcess_thread_id > 0)
    {
        pthread_cancel(msgProcess_thread_id);
        msgProcess_thread_id = 0;
    }

    if (msgProcess2_thread_id > 0)
    {
        pthread_cancel(msgProcess2_thread_id);
        msgProcess2_thread_id = 0;
    }

    if (timer_thread_id > 0)
    {
        pthread_cancel(timer_thread_id);
        timer_thread_id = 0;
    }

    if (zw_nodes)
    {
        free(zw_nodes);
        zw_nodes = NULL;
    }

    if (zw_nodes_association)
    {
        free(zw_nodes_association);
        zw_nodes_association = NULL;
    }

    if (zw_nodes_configuration)
    {
        free(zw_nodes_configuration);
        zw_nodes_configuration = NULL;
    }

    if (StatusUpdateTimer)
    {
        free(StatusUpdateTimer);
        StatusUpdateTimer = NULL;
    }
}

void ZWapi_GetDeviceInfo(ZW_NODES_INFO *devices)
{
    if (devices)
    {
        memcpy(devices, zw_nodes, sizeof(ZW_NODES_INFO));
    }
}

ZW_STATUS ZWapi_Dispatch(BYTE *pFrame, BYTE dataLen)
{
    if (pFrame == NULL ||dataLen == 0)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    // check the received data valid
    if (CheckDataValid(pFrame, dataLen) != ZW_STATUS_OK)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] checksum error !\n");
#endif
        BYTE resp = NAK;
        return WriteCom(&resp, 1);
    }

    switch (pFrame[IDX_SOF])
    {
        case ACK:
        {
            ack_status = ZW_STATUS_ACK_RECEIVED;
        }
        break;

        case NAK:
        {
            // Z-Wave chip received error data frame
        }
        break;

        case CAN:
        {
            // Z-Wave chip dis-received ACK
        }
        break;

        case SOF:
        {
            DoAck();

            if (pFrame[IDX_TYPE] == RESPONSE)
            {
                gResponseBufferLength = pFrame[IDX_LEN] - 2;
                memcpy(gResponseBuffer, &(pFrame[IDX_CMD_ID]), gResponseBufferLength);
                response_status = ZW_STATUS_RESPONSE_RECEIVED;
                break;
            }
            else if(pFrame[IDX_TYPE] == REQUEST)
            {
                ZW_DATA zw_data;
                memset(&zw_data, 0, sizeof(zw_data));
                zw_data.type = ZW_DATA_TYPE_SOF_FRAME;
                zw_data.data.sof.len = dataLen;
                memcpy(zw_data.data.sof.data, pFrame, dataLen);
                ZW_sendMsgToProcessor2((BYTE *)&zw_data, sizeof(zw_data));
            }
            else
            {
                // error SOF
#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_ERROR("[ZWave Driver] error sof received. \n");
#endif
            }
        }
        break;

        default:
            break;
    }

    return ZW_STATUS_OK;
}



ZW_STATUS ZWapi_SetDefault(void)
{
    ZW_STATUS ret = ZW_STATUS_OK;

    pthread_mutex_lock(&g_cmd_mutex);
    
    ret = ZW_SetDefault(SetDefaultHandler);
    
    pthread_mutex_unlock(&g_cmd_mutex);
    return ret;
}

ZW_STATUS ZWapi_SoftReset(void)
{
    ZW_STATUS ret = ZW_STATUS_OK;
    
    pthread_mutex_lock(&g_cmd_mutex);
    
    ret = ZW_SoftReset();
    
    pthread_mutex_unlock(&g_cmd_mutex);
    return ret;
}

ZW_STATUS ZWapi_AddNodeToNetwork(void (*cbFuncAddNodeSuccess)(ZW_DEVICE_INFO *), void (*cbFuncAddNodeFail)(ZW_STATUS))
{
    ZW_STATUS ret = ZW_STATUS_OK;

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] enter ZWapi_AddNodeToNetwork \n");
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
    ret = ZW_AddNodeToNetwork(ADD_NODE_ANY, AddNodeLearnHandler);
    if(ret == ZW_STATUS_OK)
    {
        cbFuncAddRemoveNodeSuccess = cbFuncAddNodeSuccess;
        cbFuncAddRemoveNodeFail = cbFuncAddNodeFail;
        add_remove_node_flag = 0x01;
        ZW_timerStart(TIMER_ADD_DELETE_NODES, 1, 60, 0);
    }
    else
    {
        pthread_mutex_unlock(&g_cmd_mutex);
    }

#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_INFO("[ZWave Driver] leave ZWapi_AddNodeToNetwork ret=%d\n", ret);
#endif

    //pthread_mutex_unlock(&g_cmd_mutex);
    return ret;
}

ZW_STATUS ZWapi_StopAddNodeToNetwork(void)
{
    ZW_STATUS ret = ZW_STATUS_OK;
    
#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] enter ZWapi_StopAddNodeToNetwork, flag[%d]  \n", add_remove_node_flag);
#endif

    if(add_remove_node_flag)
    {
        add_remove_node_flag = 0;
        ZW_timerCancel(TIMER_ADD_DELETE_NODES);
        cbFuncAddRemoveNodeSuccess = NULL;
        cbFuncAddRemoveNodeFail = NULL;
        ret = ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);

        pthread_mutex_unlock(&g_cmd_mutex);
        ZWapi_NotifyAddRemoveCompleted();
    }


#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] leave ZWapi_StopAddNodeToNetwork ret=%d\n", ret);
#endif

    return ret;
}

ZW_STATUS ZWapi_RemoveNodeFromNetwork(void (*cbFuncRemoveNodeSuccess)(ZW_DEVICE_INFO *), void (*cbFuncRemoveNodeFail)(ZW_STATUS))
{
    ZW_STATUS ret = ZW_STATUS_OK;

#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_INFO("[ZWave Driver] enter ZWapi_RemoveNodeFromNetwork \n");
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);
    ret = ZW_RemoveNodeFromNetwork(REMOVE_NODE_ANY, RemoveNodeLearnHandler);
    if(ret == ZW_STATUS_OK)
    {
        cbFuncAddRemoveNodeSuccess = cbFuncRemoveNodeSuccess;
        cbFuncAddRemoveNodeFail = cbFuncRemoveNodeFail;
        add_remove_node_flag = 0x02;
        ZW_timerStart(TIMER_ADD_DELETE_NODES, 1, 60, 0);
    }
    else
    {
        pthread_mutex_unlock(&g_cmd_mutex);
    }

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] leave ZWapi_RemoveNodeFromNetwork ret=%d\n", ret);
#endif

    //pthread_mutex_unlock(&g_cmd_mutex);
    return ret;
}

ZW_STATUS ZWapi_StopRemoveNodeFromNetwork(void)
{
    ZW_STATUS ret = ZW_STATUS_OK;
    
#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] enter ZWapi_StopRemoveNodeFromNetwork, flag[%d] \n", add_remove_node_flag);
#endif

    if(add_remove_node_flag)
    {
        add_remove_node_flag = 0;
        ZW_timerCancel(TIMER_ADD_DELETE_NODES);
        cbFuncAddRemoveNodeSuccess = NULL;
        cbFuncAddRemoveNodeFail = NULL;
        ret = ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);

        pthread_mutex_unlock(&g_cmd_mutex);
        ZWapi_NotifyAddRemoveCompleted();
    }

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] leave ZWapi_StopRemoveNodeFromNetwork ret=%d\n", ret);
#endif

    return ret;
}

ZW_STATUS ZWapi_SetDeviceStatus(ZW_DEVICE_INFO *device_info)
{
    if (device_info == NULL
            || device_info->id < 2
            || device_info->id >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] enter set device status. id[0x%x] \n", device_info->id);
    show_device_information(__FUNCTION__, __LINE__, device_info);
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    ZW_STATUS ret = ZW_STATUS_FAIL;
    BYTE bNodeID = (device_info->id)&0xff;
    WORD nodeID = device_info->id;

    device_info->type = zw_nodes->nodes[device_info->id].type;
    if(!OwnCommandClass(bNodeID, COMMAND_CLASS_MULTI_CHANNEL_V2))
    {
    	device_info->id = bNodeID;
    }

    switch (device_info->type)
    {
        case ZW_DEVICE_BINARYSWITCH:
            ret = ZW_SetBinarySwitchStatus(device_info->id, device_info->status.binaryswitch_status.value);
            break;
        case ZW_DEVICE_SIREN:
            ret = ZW_SetBinarySwitchStatus(device_info->id, device_info->status.siren_status.value);
            break;    

        case ZW_DEVICE_DIMMER:
        case ZW_DEVICE_CURTAIN:
            ret = ZW_SetMultiLevelSwitchStatus(device_info);
            break;

        case ZW_DEVICE_DOORLOCK:
            ret = ZW_SetDoorLockStatus(device_info);
            break;

        case ZW_DEVICE_THERMOSTAT:
        {
			ret = ZW_ThermostateModeSet(device_info->id, device_info->status.thermostat_status.mode);
			if (device_info->status.thermostat_status.mode == 1
					|| device_info->status.thermostat_status.mode == 2
					|| device_info->status.thermostat_status.mode == 7
					|| device_info->status.thermostat_status.mode == 8
					|| device_info->status.thermostat_status.mode == 9
					|| device_info->status.thermostat_status.mode == 10
					|| device_info->status.thermostat_status.mode == 11
					|| device_info->status.thermostat_status.mode == 12
					|| device_info->status.thermostat_status.mode == 13)
			{
				if (ret == ZW_STATUS_OK &&device_info->status.thermostat_status.value != 0)
				{
					BYTE size = 0, value[4] = {0};
					DWORD lvalue = (DWORD)(device_info->status.thermostat_status.value * 10);
					if (lvalue <= 0xff)
					{
						size = 2;
						value[0] = 0x00;
						value[1] = lvalue;
					}
					else if (lvalue > 0xff && lvalue <= 0xffff)
					{
						size = 2;
						value[0] = (lvalue & 0xff00) >> 8;
						value[1] = (lvalue & 0xff);
					}
					else if (lvalue > 0xffff && lvalue <= 0xffffffff)
					{
						size = 4;
						value[0] = (lvalue >> 24) & 0xff;
						value[1] = (lvalue >> 16) & 0xff;
						value[2] = (lvalue >> 8) & 0xff;
						value[3] = (lvalue & 0xff);
					}
					else
					{
						ret = ZW_STATUS_INVALID_PARAM;
						break;
					}
					ret = ZW_ThermostateSetPointSet(device_info->id, device_info->status.thermostat_status.mode, size, value);
				}
			}
        }
        break;

        case ZW_DEVICE_BATTERY:
            ret = ZW_SetWakeUpInterval(device_info->id, device_info->status.battery_status.interval_time, 0xff);
            break;

        default:
#ifdef ZWAVE_DRIVER_DEBUG
        	DEBUG_ERROR("[ZWave Driver] id:%u, type:%u \n", device_info->id, device_info->type);
#endif
            ret = ZW_STATUS_FAIL;
            break;
    }

    if(!OwnCommandClass(bNodeID, COMMAND_CLASS_MULTI_CHANNEL_V2))
    {
        device_info->id = nodeID;
    }
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] leave set device status.ret[%d], id[0x%x] \n", ret, device_info->id);
#endif

    pthread_mutex_unlock(&g_cmd_mutex);
    return ret;
}

ZW_STATUS ZWapi_GetDeviceStatus(ZW_DEVICE_INFO *device_info)
{
    if (device_info == NULL
            || device_info->id < 2
            || device_info->id >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] enter get device status. id[0x%x] \n", device_info->id);
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    ZW_STATUS ret = ZW_STATUS_FAIL;
    BYTE bNodeID = (device_info->id)&0xff;
    WORD nodeID = device_info->id;

    device_info->id = zw_nodes->nodes[device_info->id].id;
    device_info->phy_id = zw_nodes->nodes[device_info->id].phy_id;
    device_info->dev_num = zw_nodes->nodes[device_info->id].dev_num;
    device_info->type = zw_nodes->nodes[device_info->id].type;
    memcpy(device_info->cmdclass_bitmask, zw_nodes->nodes[device_info->id].cmdclass_bitmask, MAX_CMDCLASS_BITMASK_LEN);
    if(!OwnCommandClass(bNodeID, COMMAND_CLASS_MULTI_CHANNEL_V2))
    {
    	device_info->id = bNodeID;
    }

    switch (device_info->type)
    {
        case ZW_DEVICE_BINARYSWITCH:
            if (OwnCommandClass(bNodeID, COMMAND_CLASS_SWITCH_BINARY))
                ret = ZW_GetBinarySwitchStatus(device_info->id, &device_info->status.binaryswitch_status.value);
            else
                ret = ZW_STATUS_OK;
            break;
        case ZW_DEVICE_SIREN:
            ret = ZW_GetBinarySwitchStatus(device_info->id, &device_info->status.siren_status.value);
            break;    

        case ZW_DEVICE_DIMMER:
        case ZW_DEVICE_CURTAIN:
            ret = ZW_GetMultiLevelSwitchStatus(device_info);
            break;

        case ZW_DEVICE_BINARYSENSOR:
            if(OwnCommandClass(bNodeID, COMMAND_CLASS_SENSOR_BINARY))
                ret = ZW_GetBinarySensorStatus(device_info);
            else
                ret = ZW_STATUS_OK;
            break;

        case ZW_DEVICE_MULTILEVEL_SENSOR:
            ret = ZW_GetMultilevelSensorStatus(device_info);
            break;

        case ZW_DEVICE_BATTERY:
            // for TW Siren and Thermostat demo START.
            if (zw_nodes->nodes[bNodeID].type == ZW_DEVICE_THERMOSTAT
                    || zw_nodes->nodes[bNodeID].type == ZW_DEVICE_SIREN)
            {
#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_ERROR("[ZWave Driver]device [%X] ignore battery attribution for demo.\n", bNodeID);
#endif
                ret = ZW_STATUS_OK;
                break;
            }
            // for TW Siren and Thermostat demo END.
            ret = ZW_GetBatterylevel(device_info);
            ret = ZW_GetWakeUpInterval(device_info->id, &(device_info->status.battery_status.interval_time), NULL);
            ret = ZW_GetWakeUpIntervalCapabilities(device_info->id, &device_info->status.battery_status.minWakeupIntervalSec, &device_info->status.battery_status.maxWakeupIntervalSec, &device_info->status.battery_status.defaultWakeupIntervalSec, &device_info->status.battery_status.wakeupIntervalStepSec);
            break;

        case ZW_DEVICE_DOORLOCK:
            ret = ZW_GetDoorLockStatus(device_info);
            break;

        case ZW_DEVICE_THERMOSTAT:
        {
            ret = ZW_ThermostateSetPointGet(device_info->id, THERMOSTAT_MODE_HEATING, &(device_info->status.thermostat_status.heat_value));
            if(ret == ZW_STATUS_OK)
            {
            }
            else
            {
                break;
            }
            ret = ZW_ThermostateSetPointGet(device_info->id, THERMOSTAT_MODE_COOLING, &(device_info->status.thermostat_status.cool_value));
            if(ret == ZW_STATUS_OK)
            {
            }
            else
            {
                break;
            }
            /*
            ret = ZW_ThermostateSetPointGet(device_info->id, THERMOSTAT_MODE_ENERGY_SAVE_HEATING, &(device_info->status.thermostat_status.energe_save_heat_value));
            if(ret == ZW_STATUS_OK)
            {
            }
            else
            {
                break;
            }
            ret = ZW_ThermostateSetPointGet(device_info->id, THERMOSTAT_MODE_ENERGY_SAVE_COOLING, &(device_info->status.thermostat_status.energe_save_cool_value));
            if(ret == ZW_STATUS_OK)
            {
            }
            else
            {
                break;
            }
            */
            ret = ZW_ThermostateModeGet(device_info->id, &(device_info->status.thermostat_status.mode));
            if(ret == ZW_STATUS_OK &&device_info->status.thermostat_status.mode != THERMOSTAT_MODE_OFF)
            {
                ret = ZW_ThermostateSetPointGet(device_info->id, device_info->status.thermostat_status.mode, &(device_info->status.thermostat_status.value));
                if(ret == ZW_STATUS_OK)
                {
                }
                else
                {
                    break;
                }
            }
            else
            {
                device_info->status.thermostat_status.value = 0;
                break;
            }
        }
        break;

        case ZW_DEVICE_METER:
            ret = ZW_GetMeterStatus(device_info);
            break;

        default:
#ifdef ZWAVE_DRIVER_DEBUG
        	DEBUG_ERROR("[ZWave Driver] id:%u, phy_id:%u, type:%u \n", device_info->id, device_info->phy_id, device_info->type);
#endif
            ret = ZW_STATUS_FAIL;
            break;
    }

    if(!OwnCommandClass(bNodeID, COMMAND_CLASS_MULTI_CHANNEL_V2))
    {
    	device_info->id = nodeID;
    }

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
    show_device_information(__FUNCTION__, __LINE__, device_info);
    DEBUG_INFO("[ZWave Driver] leave get device status.ret[%d],  id[0x%x] \n", ret, device_info->id);
#endif

    return ret;
}

ZW_STATUS ZWapi_isFailedNode(WORD phyNodeID)
{
    if (phyNodeID < 2
            || phyNodeID >= ZW_MAX_NODES)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    ZW_STATUS ret = ZW_STATUS_IS_FAIL_NODE;
    
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] enter ZWapi_isFailedNode. phy id[0x%x] \n", phyNodeID);
        ZWapi_ShowDriverAttachedDevices();
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    if (zw_nodes->nodes[phyNodeID].id == 0
            && zw_nodes->nodes[phyNodeID].type == 0)
    {
        pthread_mutex_unlock(&g_cmd_mutex);
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] leave ZWapi_isFailedNode. phy id[0x%x] ret[%d] \n", phyNodeID, ret);
#endif
        return ret;
    }
    
    ZW_RequestNodeInfo(phyNodeID, NULL);
    ret = ZW_isFailedNode(phyNodeID);

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] leave ZWapi_isFailedNode. phy id[0x%x] ret[%d] \n", phyNodeID, ret);
#endif

    return ret;
}

ZW_STATUS ZWapi_RemoveFailedNodeID(WORD phyNodeID)
{
    if (phyNodeID < 2
            || phyNodeID >= ZW_MAX_NODES)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    ZW_STATUS ret = ZW_STATUS_FAIL;
    DWORD i = 0;

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] enter ZWapi_RemoveFailedNodeID. phy id[0x%x] \n", phyNodeID);
    ZWapi_ShowDriverAttachedDevices();
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    if (zw_nodes->nodes[phyNodeID].id == 0
            && zw_nodes->nodes[phyNodeID].type == 0)
    {
        ret = ZW_STATUS_OK;
        pthread_mutex_unlock(&g_cmd_mutex);
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] leave ZWapi_RemoveFailedNodeID. phy id[0x%x] ret[%d] \n", phyNodeID, ret);
#endif
        return ret;
    }

    ret = ZW_RemoveFailedNodeID(phyNodeID, CB_RemoveFailedNodeID);
    if (ret == ZW_STATUS_OK)
    {
        for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
        {
            if (zw_nodes->nodes[i].id
                    && ((zw_nodes->nodes[i].id & 0xff) == phyNodeID))
            {
                memset(&(zw_nodes->nodes[i]), 0, sizeof(ZW_DEVICE_INFO));
                zw_nodes->nodes_num--;
            }
        }
        memset(&(zw_nodes->nodes[phyNodeID]), 0, sizeof(ZW_DEVICE_INFO));

        SaveNodesInfo(zw_nodes);
    }

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_INFO("[ZWave Driver] leave ZWapi_RemoveFailedNodeID. phy id[0x%x] ret[%d] \n", phyNodeID, ret);
#endif

    return ret;
}

ZW_STATUS ZWapi_SetDeviceConfiguration(ZW_DEVICE_CONFIGURATION *deviceConfiguration)
{
    if (deviceConfiguration == NULL
            || deviceConfiguration->deviceID < 2
            || deviceConfiguration->deviceID >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    ZW_STATUS ret = ZW_STATUS_OK;
    ZW_DEVICE_TYPE deviceType = zw_nodes->nodes[deviceConfiguration->deviceID].type;

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] enter ZWapi_SetDeviceConfiguration. id[0x%x] type[%d] \n", deviceConfiguration->deviceID, deviceType);
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    switch (deviceType)
    {
        case ZW_DEVICE_DOORLOCK:
            ret = ZW_SetDoorLockConfiguration(deviceConfiguration->deviceID, &deviceConfiguration->configuration.doorlock_configuration);
            break;

        case ZW_DEVICE_MULTILEVEL_SENSOR:
        {
            BYTE parameter = 0, bDefault = 0, size = 0, value[4] = {0};
            parameter = deviceConfiguration->configuration.HSM100_configuration.parameterNumber;
            bDefault = deviceConfiguration->configuration.HSM100_configuration.bDefault;
            if (deviceConfiguration->configuration.HSM100_configuration.value <= 0xff)
            {
                size = 1;
                value[0] = (deviceConfiguration->configuration.HSM100_configuration.value) & 0xff;
            }
            else if (deviceConfiguration->configuration.HSM100_configuration.value > 0xff
                     && deviceConfiguration->configuration.HSM100_configuration.value <= 0xffff)
            {
                size = 2;
                value[0] = (deviceConfiguration->configuration.HSM100_configuration.value) & 0xff;
                value[1] = (deviceConfiguration->configuration.HSM100_configuration.value >> 8) & 0xff;
            }
            else if (deviceConfiguration->configuration.HSM100_configuration.value > 0xffff
                     && deviceConfiguration->configuration.HSM100_configuration.value <= 0xffffffff)
            {
                size = 4;
                value[0] = (deviceConfiguration->configuration.HSM100_configuration.value) & 0xff;
                value[1] = (deviceConfiguration->configuration.HSM100_configuration.value >> 8) & 0xff;
                value[2] = (deviceConfiguration->configuration.HSM100_configuration.value >> 16) & 0xff;
                value[3] = (deviceConfiguration->configuration.HSM100_configuration.value >> 24) & 0xff;
            }
            else
            {
                ret = ZW_STATUS_FAIL;
                break;
            }
            ret = ZW_ConfigurationSet(deviceConfiguration->deviceID, parameter, bDefault, size, value);
        }
        break;

        default:
            ret = ZW_STATUS_FAIL;
            break;
    }

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_INFO("[ZWave Driver] leave ZWapi_SetDeviceConfiguration. id[0x%x] type[%d] ret[%d]\n", deviceConfiguration->deviceID, deviceType, ret);
#endif

    return ret;
}

ZW_STATUS ZWapi_GetDeviceConfiguration(ZW_DEVICE_CONFIGURATION *deviceConfiguration)
{
    if (deviceConfiguration == NULL
            || deviceConfiguration->deviceID < 2
            || deviceConfiguration->deviceID >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    ZW_STATUS ret = ZW_STATUS_OK;
    ZW_DEVICE_TYPE deviceType = zw_nodes->nodes[deviceConfiguration->deviceID].type;
    deviceConfiguration->type = deviceType;

#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_INFO("[ZWave Driver] enter ZWapi_GetDeviceConfiguration. id[0x%x] type[%d] \n", deviceConfiguration->deviceID, deviceType);
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    switch (deviceType)
    {
        case ZW_DEVICE_DOORLOCK:
            ret = ZW_GetDoorLockConfiguration(deviceConfiguration->deviceID, &deviceConfiguration->configuration.doorlock_configuration);
            break;

        case ZW_DEVICE_MULTILEVEL_SENSOR:
            ret = ZW_ConfigurationGet(deviceConfiguration->deviceID, deviceConfiguration->configuration.HSM100_configuration.parameterNumber, &(deviceConfiguration->configuration.HSM100_configuration.value));
            break;

        default:
            ret = ZW_STATUS_FAIL;
            break;
    }

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_INFO("[ZWave Driver] leave ZWapi_GetDeviceConfiguration. id[0x%x] type[%d] ret[%d]\n", deviceConfiguration->deviceID, deviceType, ret);
#endif

    return ret;
}

/*
 * sourNodeID: sensor ID
 * desNodeID: switch/dimmer ID
 */
ZW_STATUS ZWapi_AssociationSet(WORD sourNodeID, WORD desNodeID)
{
    if (sourNodeID < 2
            || sourNodeID >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    if (!OwnCommandClass(sourNodeID, COMMAND_CLASS_ASSOCIATION))
    {
        return ZW_STATUS_FAIL;
    }

    ZW_STATUS ret = ZW_STATUS_OK;
    BYTE nodes[4] = {0};

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] enter ZWapi_AssociationSet. source[0x%x] destination[0x%x] \n", sourNodeID, desNodeID);
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    //not check the supported grouping. now just support v1

    ret = ZW_AssociationSet(sourNodeID, 1, desNodeID & 0xff);
    if (ret != ZW_STATUS_OK)
    {
        pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] ZW_AssociationSet fail. ret=%d \n", ret);
#endif
        return ret;
    }
    ret = ZW_AssignReturnRoute(sourNodeID, desNodeID & 0xff, CB_AssignReturnRoute);

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] leave ZWapi_AssociationSet. source[0x%x] destination[0x%x] ret[%d] \n", sourNodeID, desNodeID, ret);
#endif
    
    return ret;
}

/*
 * sourNodeID: sensor ID
 * desNodeID: switch/dimmer ID
 */
ZW_STATUS ZWapi_AssociationRemove(WORD sourNodeID, WORD desNodeID)
{
    if (sourNodeID < 2
            || sourNodeID >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    ZW_STATUS ret = ZW_STATUS_OK;
    BYTE len = 0;

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] enter ZWapi_AssociationRemove. source[0x%x] destination[0x%x] \n", sourNodeID, desNodeID);
#endif

    pthread_mutex_lock(&g_cmd_mutex);

    ret = ZW_AssociationRemove(sourNodeID, 1, desNodeID & 0xff);
    if (ret != ZW_STATUS_OK)
    {
        pthread_mutex_unlock(&g_cmd_mutex);
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] ZW_AssociationRemove fail. ret=%d \n", ret);
#endif
        return ret;
    }
    ret = ZW_AssociationGet(sourNodeID, 1, NULL, &len);
    if (ret == ZW_STATUS_OK && len == 0)
    {
        ret = ZW_DeleteReturnRoute(sourNodeID, CB_DeleteReturnRoute);
    }

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] leave ZWapi_AssociationRemove. source[0x%x] destination[0x%x] ret[%d] \n", sourNodeID, desNodeID, ret);
#endif
    
    return ret;
}


ZW_STATUS ZWapi_AssociationGet(WORD sourNodeID, BYTE *pAssoNodes, BYTE *num)
{
    if (sourNodeID < 2
            || sourNodeID >= ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    if(pAssoNodes == NULL
        ||num == NULL)
    {
        return ZW_STATUS_INVALID_PARAM;
    }

    if (!OwnCommandClass(sourNodeID, COMMAND_CLASS_ASSOCIATION))
    {
        return ZW_STATUS_FAIL;
    }

    ZW_STATUS ret = ZW_STATUS_OK;
    BYTE nodes[255] = {0};
    BYTE len = 0;

#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_INFO("[ZWave Driver] enter ZWapi_AssociationGet. source[0x%x] \n", sourNodeID);
#endif

    //not check the supported grouping. now just support v1

    pthread_mutex_lock(&g_cmd_mutex);

    ret = ZW_AssociationGet(sourNodeID, 1, nodes, &len);
    if (ret != ZW_STATUS_OK)
    {
        pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] ZW_AssociationGet fail. ret=%d \n", ret);
#endif
        return ret;
    }

    *num = len;
    if(len)
    {
        memcpy(pAssoNodes, nodes, len*sizeof(BYTE));
    }

    pthread_mutex_unlock(&g_cmd_mutex);

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] leave ZWapi_AssociationGet. source[0x%x] len[%d] ret[%d] \n", sourNodeID, len, ret);
#endif
    return ret;
}

ZW_STATUS ZWapi_WaitAddRemoveCompleted(struct timespec *ts)
{
    ZW_STATUS ret = ZW_STATUS_OK;
    struct timeval tv_now;

    pthread_mutex_lock(&g_addremove_mutex);
    
    if(ts == NULL)
    {
        pthread_cond_wait(&g_addremove_cond, &g_addremove_mutex);
    }
    else
    {
        memset(&tv_now, 0, sizeof(tv_now));
        gettimeofday(&tv_now, NULL);
        ts->tv_sec += tv_now.tv_sec;
        ts->tv_nsec += tv_now.tv_usec * 1000;
        pthread_cond_timedwait(&g_addremove_cond, &g_addremove_mutex, ts);
    }

    pthread_mutex_unlock(&g_addremove_mutex);
    return ret;
}

ZW_STATUS ZWapi_NotifyAddRemoveCompleted(void)
{
    ZW_STATUS ret = ZW_STATUS_OK;

    pthread_mutex_lock(&g_addremove_mutex);
    pthread_cond_broadcast(&g_addremove_cond);
    pthread_mutex_unlock(&g_addremove_mutex);

    
    return ret;
}


// update all non-battery devices' status
void ZWapi_UpdateDevicesStatus(void)
{
    DWORD i = 0;

    for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
    {
        if (zw_nodes->nodes[i].id > 1)
        {
            switch (zw_nodes->nodes[i].type)
            {
                case ZW_DEVICE_BINARYSWITCH:
                case ZW_DEVICE_DIMMER:
                case ZW_DEVICE_CURTAIN:
                    ZWapi_GetDeviceStatus(&(zw_nodes->nodes[i]));
                    break;
                default:
                    break;
            }
        }
    }

}

#ifdef ZWAVE_DRIVER_DEBUG
void ZWapi_ShowDriverAttachedDevices(void)
{
    WORD i = 0, j = 0, k = 0;
    int n = 0;
    char buff[1024] = {0};

    for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
    {
        if (zw_nodes->nodes[i].type > 0 && zw_nodes->nodes[i].id != 1)
        {
            n += snprintf(buff + n, sizeof(buff), "id: 0x%x, phy id: 0x%x, type: %u, ", zw_nodes->nodes[i].id, zw_nodes->nodes[i].phy_id, zw_nodes->nodes[i].type);
            if(zw_nodes->nodes[i].type == ZW_DEVICE_BINARYSENSOR)
            {
                n += snprintf(buff + n, sizeof(buff), "sub type: %u, ", zw_nodes->nodes[i].status.binarysensor_status.sensor_type);
            }
            else if(zw_nodes->nodes[i].type == ZW_DEVICE_MULTILEVEL_SENSOR)
            {
                n += snprintf(buff + n, sizeof(buff), "sub type: %u, ", zw_nodes->nodes[i].status.multilevelsensor_status.sensor_type);
            }
            else if(zw_nodes->nodes[i].type == ZW_DEVICE_METER)
            {
                n += snprintf(buff + n, sizeof(buff), "sub type: %u, ", zw_nodes->nodes[i].status.meter_status.meter_type);
            }
            n += snprintf(buff + n, sizeof(buff), "cmdclass: ");
            for(j = 0; j < MAX_CMDCLASS_BITMASK_LEN; j++)
            {
                for(k = 0; k < 8; k++)
                {
                    if (zw_nodes->nodes[i].cmdclass_bitmask[j] & (0x01 << k))
                    {
                        n += snprintf(buff + n, sizeof(buff), "%02x  ", j * 8 + k + 1);
                    }
                }
            }
            
            DEBUG_INFO("[ZWave Driver] node info[%s] \n", buff);
            memset(buff, 0, sizeof(buff));
            n = 0;
        }
    }
}
#endif

static unsigned long Crc32_ComputeBuf(unsigned long inCrc32, const void *buf,
                                      size_t bufLen)
{
    static const unsigned long crcTable[256] =
    {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535,
        0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD,
        0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D,
        0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4,
        0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC,
        0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB,
        0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
        0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB,
        0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA,
        0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE,
        0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A,
        0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409,
        0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739,
        0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268,
        0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
        0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8,
        0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF,
        0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703,
        0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7,
        0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE,
        0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6,
        0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D,
        0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
        0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605,
        0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
    unsigned long crc32;
    unsigned char *byteBuf;
    size_t i;

    /** accumulate crc32 for buffer **/
    //crc32 = inCrc32 ^ 0xFFFFFFFF;
    crc32 = 0xFFFFFFFF;
    byteBuf = (unsigned char*) buf;
    for(i = 0; i < bufLen; i++)
    {
        crc32 = ((crc32 >> 8) & 0x00FFFFFF) ^ crcTable[(crc32 ^ byteBuf[i]) & 0xFF];
    }
    //return( crc32 ^ 0xFFFFFFFF );
    return(crc32);
}

void reverseBitsInBuf(unsigned char *buf, int offset, int len)
{
    int i;

    for(i = offset; i < (len + offset); i++)
    {
        buf[i] = (unsigned char)(((buf[i] << 4) & 0xF0) | ((buf[i] >> 4) & 0x0F));
        buf[i] = (unsigned char)(((buf[i] << 2) & 0xCC) | ((buf[i] >> 2) & 0x33));
        buf[i] = (unsigned char)(((buf[i] << 1) & 0xAA) | ((buf[i] >> 1) & 0x55));
    }
}

int DataPrint(unsigned char *data, ssize_t size)
{
    unsigned int addr = 0;
    unsigned int offset = 0;
    unsigned int addr_offset = 0;
    int i, j;
    uint8_t str_data[16];

    if(size % 16 == 0)
    {
        addr_offset = size / 16;
    }
    else
    {
        addr_offset = (size / 16) + 1;
    }

    DEBUG_INFO("~    addr    ~\\       0x00       0x04       0x08       0x0C             ascii  \\\n");
    DEBUG_INFO("----------------------------------------------------------------------------------\n");
    for(i = 0; i < addr_offset; i++)
    {
        DEBUG_INFO("{ 0x%08x }[ ", addr);
        for(j = 0; j < 16; j++)
        {
            // if (j % 4 == 0) {
            // DEBUG_INFO(" 0x");
            // }
            DEBUG_INFO("%02x ", data[offset]);
            str_data[j] = data[offset];
            offset++;
        }

        DEBUG_INFO(" \"");
        for(j = 0; j < 16; j++)
        {
            if(((str_data[j] >= 32) && (str_data[j] <= 126)) ||
                    ((str_data[j] >= 160) && (str_data[j] <= 255)))
                DEBUG_INFO("%c", str_data[j]);
            else
                DEBUG_INFO(".");

        }
        DEBUG_INFO("\"");

        DEBUG_INFO(" ]\n");
        addr = addr + 16;
    }
    return 0;
}

ZW_STATUS ZWapi_UpgradeModule(char *zw_file)
{
    int fd, ret = 0;
    int upgradeStatus = ZW_STATUS_OK;
    ZWAVE_WRITE_S *zw_write = NULL;
    uint32_t crc32_val = 0;
    uint8_t crc_buf[4];
    uint8_t fw_but[ZWAVE_FW_MAX_SIZE_ZW5XX];
    int i;
    int errNum = 0;

    /* 1. Open zwave_spi driver */
    fd = open("/dev/zwave_spi", O_RDWR, 0);
    if(fd < 0)
    {
        errNum = errno;
        DEBUG_ERROR("Open /dev/zwave_spi failed, errNum\n", errNum);
        upgradeStatus = ZW_STATUS_FAIL;
        goto error_return;
    }

    /* 2. Prepare zwave image */
    zw_write = (ZWAVE_WRITE_S*)malloc(sizeof(ZWAVE_WRITE_S));
    if(zw_write == NULL)
    {
        DEBUG_ERROR("malloc zw_write error!\n");
        upgradeStatus = ZW_STATUS_FAIL;
        goto error_return;
    }
    memset(zw_write, 0xff, sizeof(ZWAVE_WRITE_S));

    zw_write->chipType = ZW050x;

    ret = hex2bin(zw_file, (char*) &zw_write->data);
    if(ret != 0)
    {
        DEBUG_ERROR("analyze hex error!\n");
        upgradeStatus = ZW_STATUS_FAIL;
        goto error_return;
    }

    for(i = 0; i < ZWAVE_FW_MAX_SIZE_ZW5XX; i++)
    {
        fw_but[i] = zw_write->data[i];
    }

    reverseBitsInBuf(fw_but, 0, ZWAVE_FW_MAX_SIZE_ZW5XX);

    crc32_val = Crc32_ComputeBuf(0xFFFFFFFF, fw_but, ZWAVE_FW_MAX_SIZE_ZW5XX - 4);
    crc_buf[0] = (uint8_t)(0x000000FF & crc32_val);
    crc_buf[1] = (uint8_t)(0x000000FF & (crc32_val >> 8));
    crc_buf[2] = (uint8_t)(0x000000FF & (crc32_val >> 16));
    crc_buf[3] = (uint8_t)(0x000000FF & (crc32_val >> 24));
    reverseBitsInBuf(crc_buf, 0, 4);

    zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 4] = crc_buf[0];
    zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 3] = crc_buf[1];
    zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 2] = crc_buf[2];
    zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 1] = crc_buf[3];
    //zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 1] = crc_buf[2];
    DEBUG_INFO("F/W crc32: %02x%02x%02x%02x\n",   zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 4],
           zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 3],
           zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 2],
           zw_write->data[ZWAVE_FW_MAX_SIZE_ZW5XX - 1]);
#ifdef DEBUG_VERIFICATIOIN
    DataPrint(&zw_write->data, ZWAVE_FW_MAX_SIZE_ZW5XX);
#endif
    DEBUG_INFO("Z-Wave firmware loading..........\n");
    sleep(1);

    ret = system(ZWAVE_RESET_LOW);
    if(ret != 0)
    {
        DEBUG_ERROR("echo 0 zwave reset fail\n");
        upgradeStatus = ZW_STATUS_FAIL;
        goto error_return;
    }

    ret = ioctl(fd, ZWAVE_SPI_IOCTL_ZW0x0x_WRITE_PAGE, zw_write);
    if(ret < 0)
    {
        DEBUG_ERROR("IOCTLERROR!\n");
        upgradeStatus = ZW_STATUS_FAIL;
        goto error_return;
    }

    ret = system(ZWAVE_RESET_HI);
    if(ret != 0)
    {
        DEBUG_ERROR("echo 1 zwave reset fail\n");
        upgradeStatus = ZW_STATUS_FAIL;
        goto error_return;
    }

    if(zw_write->error)
    {
        DEBUG_ERROR("Z-Wave firmware loaded error.\n");
        upgradeStatus = ZW_STATUS_FAIL;
    }
    else
    {
        DEBUG_INFO("Z-Wave firmware loaded successfully.\n");
    }

error_return:

    if(errNum == 0)
        close(fd);

    if(zw_write != NULL)
        free(zw_write);

    return upgradeStatus;
}
