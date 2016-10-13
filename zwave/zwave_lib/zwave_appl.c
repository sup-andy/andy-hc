
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <assert.h>

#include "zwave_appl.h"
#include "zwave_serialapi.h"
#include "zwave_security_aes.h"
#include "zwave_association.h"

#define ZWAVE_NODES_INFO_FILE    "/storage/config/homectrl/zwave/zwave_nodes_info"

#define ZWAVE_DRIVER_ADDRESS "/var/zwave_driver_address"
#define ZWAVE_DRIVER_ADDRESS2 "/var/zwave_driver_address2"
ZW_NODES_CONFIGURATION *zw_nodes_configuration = NULL;
ZW_NODE_STATUS_UPDATE_TIMER *StatusUpdateTimer = NULL;
pthread_mutex_t g_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
WORD gNodeCmd = 0;
BYTE gProperties = 0;

int ZW_sendMsgToProcessor(BYTE *data, int len)
{
    assert(data != NULL);
    assert(len > 0);

    static int sockfd = -1;
    struct sockaddr_un servaddr;
    int ret = -1;

    if(sockfd == -1)
    {
        if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
        {
#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_ERROR("[ZWave Driver] socket error. \n");
#endif
            return -1;
        }
    
        bzero(&servaddr, 0);
        servaddr.sun_family = AF_LOCAL;
        strncpy(servaddr.sun_path, ZWAVE_DRIVER_ADDRESS, sizeof(servaddr.sun_path));
    
        if ((connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0)
        {
#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_ERROR("[ZWave Driver] connect errstr = %s \n", strerror(errno));
#endif
            close(sockfd);
            sockfd = -1;
            return -1;
        }
    }

    ret = send(sockfd, data, len, 0);
    if (ret != len)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] send data to processor fail. ret = %d \n", ret);
#endif
        if(ret == -1)
        {
            close(sockfd);
            sockfd = -1;
            return -1;
        }
        ret = -1;
    }

    return ret;
}

int ZW_sendMsgToProcessor2(BYTE *data, int len)
{
    assert(data != NULL);
    assert(len > 0);

    static int sockfd = -1;
    struct sockaddr_un servaddr;
    int ret = -1;

    if(sockfd == -1)
    {
        if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
        {
#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_ERROR("[ZWave Driver] socket error. \n");
#endif
            return -1;
        }
    
        bzero(&servaddr, 0);
        servaddr.sun_family = AF_LOCAL;
        strncpy(servaddr.sun_path, ZWAVE_DRIVER_ADDRESS2, sizeof(servaddr.sun_path));
    
        if ((connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0)
        {
#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_ERROR("[ZWave Driver] connect errstr = %s \n", strerror(errno));
#endif
            close(sockfd);
            sockfd = -1;
            return -1;
        }
    }

    ret = send(sockfd, data, len, 0);
    if (ret != len)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver] send data to processor fail. ret = %d \n", ret);
#endif
        if(ret == -1)
        {
            close(sockfd);
            sockfd = -1;
            return -1;
        }
        ret = -1;
    }

    return ret;
}

void *ZW_timerInit(void *args)
{
    struct timeval now;
    int idx;
    ZW_DATA zw_data;
    int ret = 0;

    //memset(timer_array, 0, sizeof(timer_array));

    while (1)
    {
        memset(&now, 0, sizeof(now));
        gettimeofday(&now, NULL);

        for (idx = 0; idx < TIMER_MAX; idx++)
        {
            if (timer_array[idx].timeout.tv_sec && timer_array[idx].timeout.tv_usec)
            {
                if ((timer_array[idx].timeout.tv_sec < now.tv_sec)
                        || ((timer_array[idx].timeout.tv_sec == now.tv_sec) && (timer_array[idx].timeout.tv_usec <= now.tv_usec)))
                {
                    // timeout happen
                    memset(&(timer_array[idx].timeout), 0, sizeof(timer_array[idx].timeout));

                    memset(&zw_data, 0, sizeof(zw_data));
                    zw_data.type = ZW_DATA_TYPE_TIMER;
                    zw_data.data.timer.type = idx;
                    ret = ZW_sendMsgToProcessor((BYTE *)&zw_data, sizeof(zw_data));

                    if (timer_array[idx].type == 0)
                    {
                        // period timer
                        timer_array[idx].timeout.tv_sec = now.tv_sec + timer_array[idx].sec + (now.tv_usec + timer_array[idx].usec) / 1000000L;
                        timer_array[idx].timeout.tv_usec = (now.tv_usec + timer_array[idx].usec) % 1000000L;
                    }
                }
            }
        }

        MsecSleep(500); // sleep 500 ms
    }
}

void ZW_timerStart(ZW_TIMER_IDENTIFIER timer, int type, long sec, long usec)
{
    struct timeval now;

    pthread_mutex_lock(&g_timer_mutex);

    memset(&now, 0, sizeof(now));
    gettimeofday(&now, NULL);

    memset(&(timer_array[timer]), 0, sizeof(timer_array[timer]));
    timer_array[timer].type = type;
    timer_array[timer].sec = sec;
    timer_array[timer].usec = usec;
    timer_array[timer].timeout.tv_sec = now.tv_sec + sec + (now.tv_usec + usec) / 1000000L;
    timer_array[timer].timeout.tv_usec = (now.tv_usec + usec) % 1000000L;

    pthread_mutex_unlock(&g_timer_mutex);
}

void ZW_timerCancel(ZW_TIMER_IDENTIFIER timer)
{
    pthread_mutex_lock(&g_timer_mutex);
    memset(&(timer_array[timer]), 0, sizeof(timer_array[timer]));
    pthread_mutex_unlock(&g_timer_mutex);
}

void *ZW_msgProcess(void *args)
{
    int sockfd = -1;
    struct sockaddr_un sin;
    socklen_t sin_len = sizeof(sin);
    int client = -1, clients[8];
    fd_set rfds, origin_rfds;
    struct sockaddr_in from;
    socklen_t fromLen = sizeof(from);
    int max_fd;
    int ret = -1;
    int i = 0;
    ZW_DATA zw_data;
    ZW_EVENT_REPORT zw_event;

    unlink(ZWAVE_DRIVER_ADDRESS);

    if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] socket error. \n");
#endif
        return NULL;
    }

    bzero(&sin, sizeof(sin));
    sin.sun_family = AF_LOCAL;
    strncpy(sin.sun_path, ZWAVE_DRIVER_ADDRESS, sizeof(sin.sun_path));

    if (bind(sockfd, (struct sockaddr *)&sin, sin_len) < 0)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] bind error. \n");
#endif
        close(sockfd);
        return NULL;
    }

    if(listen(sockfd, 1) < 0)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] listen error. \n");
#endif
        close(sockfd);
        return NULL;
    }

    memset(clients, -1, sizeof(clients));

    FD_ZERO(&origin_rfds);
    FD_SET(sockfd, &origin_rfds);
    max_fd = sockfd;
    while (1)
    {
        memcpy(&rfds, &origin_rfds, sizeof(rfds));
        ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret <= 0)
        {
            // error happened
            continue;
        }
        else
        {
            if(FD_ISSET(sockfd, &rfds))
            {
                bzero(&from, sizeof(from));
                client = accept(sockfd, (struct sockaddr *)&from, &fromLen);
                if(client > 0)
                {
                    for(i = 0; i < 8; i++)
                    {
                        if(clients[i] == -1)
                            break;
                    }
                    if(i == 8)
                    {
                        continue;
                    }

                    if(client > max_fd)
                        max_fd = client;

                    clients[i] = client;
                    FD_SET(client, &origin_rfds);

                }
            }

            for(i = 0; i < 8; i++)
            {
                if(clients[i] > 0 && FD_ISSET(clients[i], &rfds))
                {
                    memset(&zw_data, 0, sizeof(zw_data));
                    ret = recv(clients[i], (void *)&zw_data, sizeof(zw_data), 0);
                    if(ret == sizeof(zw_data))
                    {
                        if (zw_data.type == ZW_DATA_TYPE_TIMER)
                        {
                            if (zw_data.data.timer.type == TIMER_ADD_DELETE_NODES)
                            {
                                if(add_remove_node_flag == 0x01)
                                {
                                    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
                                }
                                else if(add_remove_node_flag == 0x02)
                                {
                                    ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);
                                }
                                add_remove_node_flag = 0;
                                pthread_mutex_unlock(&g_cmd_mutex);
                                if (cbFuncAddRemoveNodeFail)
                                {
                                    cbFuncAddRemoveNodeFail(ZW_STATUS_TIMEOUT);
                                }
                            
                                ZWapi_NotifyAddRemoveCompleted();
                            }
                            else if (zw_data.data.timer.type == TIMER_UPDATE_NODE_STATUS)
                            {
                                DWORD j = 0;
                                for (j = 0; j < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; j ++)
                                {
                                    if (StatusUpdateTimer->timer[j])
                                    {
                                        StatusUpdateTimer->timer[j]--;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                    
                                    if (StatusUpdateTimer->timer[j] == 0)
                                    {
                                        if (zw_nodes->nodes[j].type == ZW_DEVICE_BINARYSWITCH
                                                || zw_nodes->nodes[j].type == ZW_DEVICE_SIREN
                                                || zw_nodes->nodes[j].type == ZW_DEVICE_DIMMER)
                                        {
                                            if(zw_nodes->nodes[j].id)
                                            {
                                                if(cbFuncEventHandler)
                                                {
                                                    memset(&zw_event, 0, sizeof(zw_event));
                                                    zw_event.type = ZW_EVENT_TYPE_NODEINFO;
                                                    zw_event.phy_id = zw_nodes->nodes[j].phy_id;

#ifdef ZWAVE_DRIVER_DEBUG
                                                    DEBUG_INFO("[ZWave Driver] Ready to report the nodeinfo event. phy id[0x%x]  \n", zw_nodes->nodes[j].phy_id);
#endif
                                                    cbFuncEventHandler(&zw_event);
                                                }
                                            }
                                        }
                                        
                                    }

                                }
                                
                                for (j = 0; j < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; j ++)
                                {
                                    if (StatusUpdateTimer->timer[j])
                                        break;
                                }
                                if(j == ZW_MAX_NODES * EXTEND_NODE_CHANNEL)
                                {
                                    ZW_timerCancel(TIMER_UPDATE_NODE_STATUS);
                                }
                            }
                        }
                        else if (zw_data.type == ZW_DATA_TYPE_ADD_NODE_RESPONSE)
                        {
                            ZW_DEVICE_INFO deviceInfo;
                            memcpy(&deviceInfo, &zw_data.data.addRemoveNodeRes.deviceInfo, sizeof(ZW_DEVICE_INFO));
#ifdef ZWAVE_DRIVER_DEBUG
                            DEBUG_INFO("[ZWave Driver] Ready to add node. id [%u] type[%u] \n", deviceInfo.id, deviceInfo.type);
#endif
                            ret = GenVirtualSubNode(deviceInfo.id);
                            if(ret == ZW_STATUS_OK)
                            {
                                ret = ConfigNodesDefaultValue(deviceInfo.id);
                            }
                            if(ret == ZW_STATUS_OK)
                            {
                                AddNodesReport(deviceInfo.id);
                            }
                            else
                            {
                                memset(&(zw_nodes->nodes[deviceInfo.id]), 0, sizeof(ZW_DEVICE_INFO));
                                if(cbFuncAddRemoveNodeFail)
                                    cbFuncAddRemoveNodeFail(ZW_STATUS_FAIL);
                            }
                            
                            ZWapi_NotifyAddRemoveCompleted();
                        }
                        else if (zw_data.type == ZW_DATA_TYPE_REMOVE_NODE_RESPONSE)
                        {
                            ZW_DEVICE_INFO deviceInfo;
                            memcpy(&deviceInfo, &zw_data.data.addRemoveNodeRes.deviceInfo, sizeof(ZW_DEVICE_INFO));
#ifdef ZWAVE_DRIVER_DEBUG
                            DEBUG_INFO("[ZWave Driver] Ready to remove node. id[%u] type[%u] \n", deviceInfo.id, deviceInfo.type);
#endif
                            if(deviceInfo.id)
                            {
                                /* Siren analyze CMD type is binaryswitch, but we convert it to Siren. */
                                for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
                                {
                                    if (zw_nodes->nodes[i].id 
                                            && zw_nodes->nodes[i].id == deviceInfo.id)
                                    {
                                        if (zw_nodes->nodes[i].type != deviceInfo.type)
                                        {
                                            deviceInfo.type = zw_nodes->nodes[i].type;
                                        }
                                        break;
                                    }
                                }
                                
                                if (cbFuncAddRemoveNodeSuccess)
                                {
                                    cbFuncAddRemoveNodeSuccess(&deviceInfo);
                                }
                                memset(&(zw_nodes->nodes[deviceInfo.id]), 0, sizeof(ZW_DEVICE_INFO));
                                zw_nodes->nodes_num--;
                                
                                for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
                                {
                                    if (zw_nodes->nodes[i].id
                                            && ((zw_nodes->nodes[i].id & 0xff) == deviceInfo.id))
                                    {
                                        if (cbFuncAddRemoveNodeSuccess)
                                        {
                                            cbFuncAddRemoveNodeSuccess(&(zw_nodes->nodes[i]));
                                        }
                                        memset(&(zw_nodes->nodes[zw_nodes->nodes[i].id]), 0, sizeof(ZW_DEVICE_INFO));
                                        zw_nodes->nodes_num--;
                                    }
                                }
                            }
                            else
                            {
                                if (cbFuncAddRemoveNodeSuccess)
                                {
                                    cbFuncAddRemoveNodeSuccess(&deviceInfo);
                                }
                            }
                            memset(&zw_nodes->nodes[deviceInfo.id], 0, sizeof(ZW_DEVICE_INFO));

                            SaveNodesInfo(zw_nodes);

                            ZWapi_NotifyAddRemoveCompleted();
                        }
                        else if (zw_data.type == ZW_DATA_TYPE_WAKE_UP)
                        {
                            BYTE nodeid = zw_data.data.wakeup.nodeid;
        
#ifdef ZWAVE_DRIVER_DEBUG
                            DEBUG_INFO("[ZWave Driver] node[0x%x] wake up \n", nodeid);
#endif
                            if(cbFuncEventHandler)
                            {
                                memset(&zw_event, 0, sizeof(zw_event));
                                zw_event.type = ZW_EVENT_TYPE_WAKEUP;
                                zw_event.phy_id = nodeid;

#ifdef ZWAVE_DRIVER_DEBUG
                                DEBUG_INFO("[ZWave Driver] Ready to report the wakeup event. phy id[0x%x]  \n", zw_event.phy_id);
#endif
                                cbFuncEventHandler(&zw_event);
                            }

                        }
                        else
                        {
                        }
                        
                    }
                    else
                    {
#ifdef ZWAVE_DRIVER_DEBUG
                            DEBUG_INFO("[ZWave Driver] recv data error. ret = %d\n", ret);
#endif
                        if(ret == -1)
                        {
                            if(errno == EINTR ||errno == EAGAIN)
                            {
                                ;
                            }
                            else
                            {
                                close(clients[i]);
                                clients[i] = -1;
                                FD_CLR(clients[i], &origin_rfds);
                            }
                        }
                        else if(ret == 0)
                        {
                            close(clients[i]);
                            clients[i] = -1;
                            FD_CLR(clients[i], &origin_rfds);
                        }
                    }
                }
            }
        }
    }

    close(sockfd);

}


void *ZW_msgProcess2(void *args)
{
    int sockfd = -1;
    struct sockaddr_un sin;
    socklen_t sin_len = sizeof(sin);
    int client = -1, clients[8];
    fd_set rfds, origin_rfds;
    struct sockaddr_in from;
    socklen_t fromLen = sizeof(from);
    int max_fd;
    int ret = -1;
    int i = 0;
    ZW_DATA zw_data;

    unlink(ZWAVE_DRIVER_ADDRESS2);

    if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] socket error. \n");
#endif
        return NULL;
    }

    bzero(&sin, sizeof(sin));
    sin.sun_family = AF_LOCAL;
    strncpy(sin.sun_path, ZWAVE_DRIVER_ADDRESS2, sizeof(sin.sun_path));

    if (bind(sockfd, (struct sockaddr *)&sin, sin_len) < 0)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] bind error. \n");
#endif
        close(sockfd);
        return NULL;
    }

    if(listen(sockfd, 1) < 0)
    {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] listen error. \n");
#endif
        close(sockfd);
        return NULL;
    }

    memset(clients, -1, sizeof(clients));

    FD_ZERO(&origin_rfds);
    FD_SET(sockfd, &origin_rfds);
    max_fd = sockfd;
    while (1)
    {
        memcpy(&rfds, &origin_rfds, sizeof(rfds));
        ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret <= 0)
        {
            // error happened
            continue;
        }
        else
        {
            if(FD_ISSET(sockfd, &rfds))
            {
                bzero(&from, sizeof(from));
                client = accept(sockfd, (struct sockaddr *)&from, &fromLen);
                if(client > 0)
                {
                    for(i = 0; i < 8; i++)
                    {
                        if(clients[i] == -1)
                            break;
                    }
                    if(i == 8)
                    {
                        continue;
                    }

                    if(client > max_fd)
                        max_fd = client;

                    clients[i] = client;
                    FD_SET(client, &origin_rfds);

                }
            }

            for(i = 0; i < 8; i++)
            {
                if(clients[i] > 0 && FD_ISSET(clients[i], &rfds))
                {
                    memset(&zw_data, 0, sizeof(zw_data));
                    ret = recv(clients[i], (void *)&zw_data, sizeof(zw_data), 0);
                    if(ret == sizeof(zw_data))
                    {
                        if(zw_data.type == ZW_DATA_TYPE_SOF_FRAME)
                        {
                            FrameHandler(zw_data.data.sof.data, zw_data.data.sof.len);
                        }
                        else
                        {
                        
                        }
                    }
                    else
                    {
#ifdef ZWAVE_DRIVER_DEBUG
                            DEBUG_INFO("[ZWave Driver] recv data error. ret = %d\n", ret);
#endif
                        if(ret == -1)
                        {
                            if(errno == EINTR ||errno == EAGAIN)
                            {
                                ;
                            }
                            else
                            {
                                close(clients[i]);
                                clients[i] = -1;
                                FD_CLR(clients[i], &origin_rfds);
                            }
                        }
                        else if(ret == 0)
                        {
                            close(clients[i]);
                            clients[i] = -1;
                            FD_CLR(clients[i], &origin_rfds);
                        }
                    }
                }
            }
        }
    }

    close(sockfd);

}

ZW_STATUS GenVirtualSubNode(BYTE bNodeID)
{
	ZW_STATUS retVal = ZW_STATUS_OK;
	DWORD k = 0;
	WORD nodeID = 0;
	BYTE len, endPoints = 0, cmd[256];
	NODEINFO nodeInfo;
	WORD subNodeInd = 0;
    
    WORD manufactureId = 0, productTypeId = 0, productId = 0;

    pthread_mutex_lock(&g_cmd_mutex);

    if(ZW_ManufacturerSpecificGet(bNodeID, &manufactureId, &productTypeId, &productId) == ZW_STATUS_OK)
    {
        DEBUG_INFO("[ZWave Driver] get device [%u] manufacturer specific ok.manufactureId[0x%x],  productTypeId[0x%x], productId[0x%x] \n", 
        bNodeID, manufactureId, productTypeId, productId);
    }
    else
    {
        DEBUG_INFO("[ZWave Driver] get device[%u] manufacturer specific fail. \n", bNodeID);
    }
    
    memset(&(zw_nodes->nodes[bNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
    
	zw_nodes->nodes[bNodeID].dev_num = 1;

    /* 
     * 1. basical device, id == bSource 
     * 2. multi-channel device, id == bSource | channelEndpointID << 8
     * 3. uncertain type device, id == bSource | myIndex << 8
     */
    if (OwnCommandClass(bNodeID, COMMAND_CLASS_MULTI_CHANNEL_V2))
    {
    	nodeID = bNodeID;
        if (ZW_GetMultiChannelEndPoint(bNodeID, &endPoints) == ZW_STATUS_OK)
        {
            for (k = 1; k <= endPoints; k++)
            {
                memset(cmd, 0, sizeof(cmd));
                len = 0;
                if (ZW_GetMultiChannelCapability(bNodeID, k, cmd, &len) == ZW_STATUS_OK)
                {
                    memset(&nodeInfo, 0, sizeof(nodeInfo));
                    nodeInfo.nodeType.basic = BASIC_TYPE_SLAVE;
                    nodeInfo.nodeType.generic = cmd[1];
                    nodeInfo.nodeType.specific = cmd[2];
                    nodeID = bNodeID | (k << 8); // bSource | channelEndpointID << 8
                    subNodeInd = k;
                        
                    zw_nodes->nodes[bNodeID].dev_num++;
                    zw_nodes->nodes[nodeID].id = nodeID;
                    zw_nodes->nodes[nodeID].type = Analysis_DeviceType(&nodeInfo);
                    if (zw_nodes->nodes[nodeID].type == ZW_DEVICE_BINARYSWITCH && endPoints == 2)
                    {
                        //zw_nodes->nodes[nodeID].type = ZW_DEVICE_DOUBLESWITCH;
                    }
                    else if (zw_nodes->nodes[nodeID].type == ZW_DEVICE_MULTILEVEL_SENSOR)
                    {
                        if (ZW_GetMultilevelSensorStatus(&(zw_nodes->nodes[nodeID])) == ZW_STATUS_OK)
                        {
                        	switch(zw_nodes->nodes[nodeID].status.multilevelsensor_status.sensor_type)
                        	{
                        	    /* if it undefined multilevel sensor_type, we check it's  
                        	       manufactureId, productTypeId, productId */
                            	case ZW_MULTILEVEL_SENSOR_TYPE_GENERAL_PURPOSE_VALUE:
                                {
                                    AnalysisDeviceTypebyManufacturerSpecific(nodeID, manufactureId, productTypeId, productId);
                                }
                                break;
                            	default:
                            		break;
                        	}
                        }
                        else
                        {
                            // get Multi-level sensor status fail
#ifdef ZWAVE_DRIVER_DEBUG
                            DEBUG_ERROR("get Multi-level sensor status fail, nodeID[%d] endpoint[%lu] \n", nodeID, k);
#endif
                            retVal = ZW_STATUS_FAIL;
                            goto END;
                        }
                    }
                    else if(zw_nodes->nodes[nodeID].type == ZW_DEVICE_BINARYSENSOR)
                    {
                    	;
                    }

                }
                else
                {
                    // get Multi-Channel node capabilities fail
#ifdef ZWAVE_DRIVER_DEBUG
                    DEBUG_ERROR("get Multi-Channel node capabilities fail, nodeID[%d] endpoint[%lu] \n", bNodeID, k);
#endif
                    retVal = ZW_STATUS_FAIL;
                    goto END;
                }
            }

            zw_nodes->nodes[bNodeID].id = 0;
            zw_nodes->nodes[bNodeID].dev_num--;
        }
        else
        {
            // get Multi-Channel endpoints fail
#ifdef ZWAVE_DRIVER_DEBUG
            DEBUG_ERROR("get Multi-Channel endpoints fail, nodeID[%d] \n", bNodeID);
#endif
            retVal = ZW_STATUS_FAIL;
            goto END;
        }

    }
    else
    {
        if(OwnCommandClass(bNodeID, COMMAND_CLASS_SWITCH_BINARY))
        {
            /* (not mutil-channel && has binary switch COMMAND_CLASS && device type not BINARYSWITCH), 
               we virtual it to thermostat. */
            if(zw_nodes->nodes[bNodeID].type != ZW_DEVICE_BINARYSWITCH)
            {
                subNodeInd ++;
                nodeID = bNodeID | (subNodeInd << 8);
                zw_nodes->nodes[bNodeID].dev_num ++;
        
                zw_nodes->nodes[nodeID].id = nodeID;
                zw_nodes->nodes[nodeID].type = ZW_DEVICE_BINARYSWITCH;
                if(zw_nodes->nodes[bNodeID].type == ZW_DEVICE_THERMOSTAT)
                {
                    memset(&(zw_nodes->nodes[nodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[nodeID].status.binaryswitch_status.type = ZW_BINARY_SWITCH_TYPE_THERMOSTAT;
                }
            }
            else
            {
                /* (not mutil-channel && has binary switch COMMAND_CLASS && device type is BINARYSWITCH  
                   && has thermostat COMMAND_CLASS), we virtual it to themostat. */
                if(OwnCommandClass(bNodeID, COMMAND_CLASS_THERMOSTAT_MODE))
                {
                    zw_nodes->nodes[bNodeID].type = ZW_DEVICE_THERMOSTAT;

                    subNodeInd ++;
                    nodeID = bNodeID | (subNodeInd << 8);
                    zw_nodes->nodes[bNodeID].dev_num ++;
            
                    zw_nodes->nodes[nodeID].id = nodeID;
                    zw_nodes->nodes[nodeID].type = ZW_DEVICE_BINARYSWITCH;
                    memset(&(zw_nodes->nodes[nodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[nodeID].status.binaryswitch_status.type = ZW_BINARY_SWITCH_TYPE_THERMOSTAT;
                }
                else
                {
                    /* (not mutil-channel && has binary switch COMMAND_CLASS && device type is BINARYSWITCH  
                       && do not has thermostat COMMAND_CLASS), actually it's binaryswitch. 
                       But current there is a device "Water Cop", we would like to distinguish 
                       between binaryswitch and water cop. */
                    AnalysisDeviceTypebyManufacturerSpecific(bNodeID, manufactureId, productTypeId, productId);
                }
            }
        }

        // just for TW thermostat demo START
        if (OwnCommandClass(bNodeID, COMMAND_CLASS_THERMOSTAT_MODE) 
                && !OwnCommandClass(bNodeID, COMMAND_CLASS_SWITCH_BINARY))
        {
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_ERROR("[ZWave Driver]get device [%u] TW thermostat demo patch\n", bNodeID);
#endif
        
            subNodeInd ++;
            nodeID = bNodeID | (subNodeInd << 8);
            zw_nodes->nodes[bNodeID].dev_num ++;

            zw_nodes->nodes[nodeID].id = nodeID;
            zw_nodes->nodes[nodeID].type = ZW_DEVICE_BINARYSWITCH;
            memset(&(zw_nodes->nodes[nodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
            zw_nodes->nodes[nodeID].status.binaryswitch_status.type = ZW_BINARY_SWITCH_TYPE_THERMOSTAT;
        }
        // just for TW thermostat demo END
            
        /* (not mutil-channel && has multilevel sensor COMMAND_CLASS && do not has meter COMMAND_CLASS),
           we think it's multilevel sensor. */
        if(OwnCommandClass(bNodeID, COMMAND_CLASS_SENSOR_MULTILEVEL)
                && !OwnCommandClass(bNodeID, COMMAND_CLASS_METER))
        {
            if(zw_nodes->nodes[bNodeID].type != ZW_DEVICE_MULTILEVEL_SENSOR)
            {
                subNodeInd ++;
                nodeID = bNodeID | (subNodeInd << 8);
                zw_nodes->nodes[bNodeID].dev_num ++;
                
                zw_nodes->nodes[nodeID].id = nodeID;
                zw_nodes->nodes[nodeID].type = ZW_DEVICE_MULTILEVEL_SENSOR;
                
                ZW_GetMultilevelSensorStatus(&(zw_nodes->nodes[bNodeID]));
                memcpy(&(zw_nodes->nodes[nodeID].status), &(zw_nodes->nodes[bNodeID].status), sizeof(ZW_DEVICE_STATUS));
            }
            else
            {
                if(zw_nodes->nodes[bNodeID].id)
                    ZW_GetMultilevelSensorStatus(&(zw_nodes->nodes[bNodeID]));
            }
        }
    
    
        if (OwnCommandClass(bNodeID, COMMAND_CLASS_METER))
        {
            if(zw_nodes->nodes[bNodeID].type != ZW_DEVICE_METER)
            {
                subNodeInd ++;
            nodeID = bNodeID | (subNodeInd << 8);
            zw_nodes->nodes[bNodeID].dev_num ++;
    
            zw_nodes->nodes[nodeID].id = nodeID;
            zw_nodes->nodes[nodeID].type = ZW_DEVICE_METER;
            
            ZW_GetMeterStatus(&(zw_nodes->nodes[bNodeID]));
                memcpy(&(zw_nodes->nodes[nodeID].status), &(zw_nodes->nodes[bNodeID].status), sizeof(ZW_DEVICE_STATUS));
            }
        }
    
        if (OwnCommandClass(bNodeID, COMMAND_CLASS_SENSOR_BINARY))
        {
            if(zw_nodes->nodes[bNodeID].type != ZW_DEVICE_BINARYSENSOR)
            {
                // Generate a virtual binary sensor if needed
            }
            else
            {
                // Get binary sensor sub-type
                ZW_GetBinarySensorStatus(&zw_nodes->nodes[bNodeID]);
                /* if it undefined binary sensor_type, we check it's  
                   manufactureId, productTypeId, productId */
                if(zw_nodes->nodes[bNodeID].status.binarysensor_status.sensor_type == ZW_BINARY_SENSOR_TYPE_RESERVED)
                {
                    AnalysisDeviceTypebyManufacturerSpecific(bNodeID, manufactureId, productTypeId, productId);
                }
                
            }
        }
    }

    /* has battery COMMAND_CLASS, virtual a battery device. */
    if (OwnCommandClass(bNodeID, COMMAND_CLASS_BATTERY))
    {
    	if(zw_nodes->nodes[bNodeID].type != ZW_DEVICE_BATTERY)
    	{
    		subNodeInd ++;
		nodeID = bNodeID | (subNodeInd << 8);
		zw_nodes->nodes[bNodeID].dev_num ++;

		zw_nodes->nodes[nodeID].id = nodeID;
		zw_nodes->nodes[nodeID].type = ZW_DEVICE_BATTERY;
    	}
    }

END:
    pthread_mutex_unlock(&g_cmd_mutex);
    return retVal;
}

ZW_STATUS ConfigNodesDefaultValue(WORD NodeID)
{
	ZW_STATUS retVal = ZW_STATUS_OK;
	BYTE bNodeID = NodeID & 0xff;

    pthread_mutex_lock(&g_cmd_mutex);

    if(OwnCommandClass(bNodeID, COMMAND_CLASS_SECURITY))
    {
    	if (ZW_SecurityNetworkKeySet(bNodeID) != ZW_STATUS_OK)
    	{
    		memset(&(zw_nodes->nodes[NodeID]), 0, sizeof(ZW_DEVICE_INFO));
    		retVal = ZW_STATUS_FAIL;
    	}
    }

    if(OwnCommandClass(bNodeID, COMMAND_CLASS_ASSOCIATION))
    {
        ZW_AssociationSet(bNodeID, 1, 0x01);
        ZW_AssignReturnRoute(bNodeID, 0x01, CB_AssignReturnRoute);
    }
    
    pthread_mutex_unlock(&g_cmd_mutex);
	return retVal;
}

void AddNodesReport(BYTE bNodeID)
{
	DWORD i = 0;

	for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
	{
		if (zw_nodes->nodes[i].id
				&& ((zw_nodes->nodes[i].id & 0xff) == bNodeID))
		{
			zw_nodes->nodes[i].phy_id = zw_nodes->nodes[bNodeID].phy_id;
			zw_nodes->nodes[i].dev_num = zw_nodes->nodes[bNodeID].dev_num;
			if (cbFuncAddRemoveNodeSuccess)
			{
				cbFuncAddRemoveNodeSuccess(&(zw_nodes->nodes[i]));
			}
			zw_nodes->nodes_num++;
		}
	}

	SaveNodesInfo(zw_nodes);

}

BYTE EncapCmdBasicGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_BasicGetFrame.cmdClass = COMMAND_CLASS_BASIC;
    pBuf->ZW_BasicGetFrame.cmd = BASIC_GET;

    return sizeof(pBuf->ZW_BasicGetFrame);
}

BYTE EncapCmdBasicSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE status)
{
    pBuf->ZW_BasicSetFrame.cmdClass = COMMAND_CLASS_BASIC;
    pBuf->ZW_BasicSetFrame.cmd = BASIC_SET;
    pBuf->ZW_BasicSetFrame.value = status;

    return sizeof(pBuf->ZW_BasicSetFrame);
}

BYTE EncapCmdBinaryGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_SwitchBinaryGetFrame.cmdClass = COMMAND_CLASS_SWITCH_BINARY;
    pBuf->ZW_SwitchBinaryGetFrame.cmd = SWITCH_BINARY_GET;

    return sizeof(pBuf->ZW_SwitchBinaryGetFrame);
}

BYTE EncapCmdBinarySet(ZW_APPLICATION_TX_BUFFER *pBuf , BYTE value)
{
    pBuf->ZW_SwitchBinarySetFrame.cmdClass = COMMAND_CLASS_SWITCH_BINARY;
    pBuf->ZW_SwitchBinarySetFrame.cmd = SWITCH_BINARY_SET;
    pBuf->ZW_SwitchBinarySetFrame.switchValue = value;

    return sizeof(pBuf->ZW_SwitchBinarySetFrame);
}

BYTE EncapCmdMultilevelGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_SwitchMultilevelGetFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
    pBuf->ZW_SwitchMultilevelGetFrame.cmd = SWITCH_MULTILEVEL_GET;

    return sizeof(pBuf->ZW_SwitchMultilevelGetFrame);
}

BYTE EncapCmdMultilevelSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE value)
{
    pBuf->ZW_SwitchMultilevelSetFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
    pBuf->ZW_SwitchMultilevelSetFrame.cmd = SWITCH_MULTILEVEL_SET;
    pBuf->ZW_SwitchMultilevelSetFrame.value = value;

    return sizeof(pBuf->ZW_SwitchMultilevelSetFrame);
}

BYTE EncapCmdMultilevelSwitchStopLevelChange(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_SwitchMultilevelStopLevelChangeFrame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
    pBuf->ZW_SwitchMultilevelStopLevelChangeFrame.cmd = SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE;

    return sizeof(pBuf->ZW_SwitchMultilevelStopLevelChangeFrame);
}

BYTE EncapCmdMeterGetV3(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE scale)
{
    pBuf->ZW_MeterGetV3Frame.cmdClass = COMMAND_CLASS_METER;
    pBuf->ZW_MeterGetV3Frame.cmd = METER_GET_V3;
    pBuf->ZW_MeterGetV3Frame.properties1 = ((scale & 0x07) << 0x03);

    return sizeof(pBuf->ZW_MeterGetV3Frame);
}

BYTE EncapCmdDoorLockOperationGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_DoorLockOperationGetFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK;
    pBuf->ZW_DoorLockOperationGetFrame.cmd = DOOR_LOCK_OPERATION_GET;

    return sizeof(pBuf->ZW_DoorLockOperationGetFrame);
}

BYTE EncapCmdDoorLockOperationSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE DoorLockMode)
{
    pBuf->ZW_DoorLockOperationSetFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK;
    pBuf->ZW_DoorLockOperationSetFrame.cmd = DOOR_LOCK_OPERATION_SET;
    pBuf->ZW_DoorLockOperationSetFrame.doorLockMode = DoorLockMode;

    return sizeof(pBuf->ZW_DoorLockOperationSetFrame);
}

BYTE EncapCmdDoorLockConfigurationGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_DoorLockConfigurationGetFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK;
    pBuf->ZW_DoorLockConfigurationGetFrame.cmd = DOOR_LOCK_CONFIGURATION_GET;

    return sizeof(pBuf->ZW_DoorLockConfigurationGetFrame);
}

BYTE EncapCmdDoorLockConfigurationSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE operationType, BYTE handlesMode, BYTE minutes, BYTE seconds)
{
    pBuf->ZW_DoorLockConfigurationSetFrame.cmdClass = COMMAND_CLASS_DOOR_LOCK;
    pBuf->ZW_DoorLockConfigurationSetFrame.cmd = DOOR_LOCK_CONFIGURATION_SET;
    pBuf->ZW_DoorLockConfigurationSetFrame.operationType = operationType;
    pBuf->ZW_DoorLockConfigurationSetFrame.properties1 = handlesMode;
    pBuf->ZW_DoorLockConfigurationSetFrame.lockTimeoutMinutes = minutes;
    pBuf->ZW_DoorLockConfigurationSetFrame.lockTimeoutSeconds = seconds;

    return sizeof(pBuf->ZW_DoorLockConfigurationSetFrame);
}

BYTE EncapCmdThermostatModeGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_ThermostatModeGetFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_MODE;
    pBuf->ZW_ThermostatModeGetFrame.cmd = THERMOSTAT_MODE_GET;

    return sizeof(pBuf->ZW_ThermostatModeGetFrame);
}

BYTE EncapCmdThermostatModeSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE mode)
{
    pBuf->ZW_ThermostatModeSetFrame.cmdClass = COMMAND_CLASS_THERMOSTAT_MODE;
    pBuf->ZW_ThermostatModeSetFrame.cmd = THERMOSTAT_MODE_SET;
    pBuf->ZW_ThermostatModeSetFrame.level = mode & 0x1f;

    return sizeof(pBuf->ZW_ThermostatModeSetFrame);
}

BYTE EncapCmdThermostatSetpointGet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE setpointType)
{
    pBuf->ZW_ThermostatSetpointGetV3Frame.cmdClass = COMMAND_CLASS_THERMOSTAT_SETPOINT;
    pBuf->ZW_ThermostatSetpointGetV3Frame.cmd = THERMOSTAT_SETPOINT_GET;
    pBuf->ZW_ThermostatSetpointGetV3Frame.level = setpointType & 0x0f;

    return sizeof(pBuf->ZW_ThermostatSetpointGetV3Frame);
}

BYTE EncapCmdThermostatSetpointSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE setpointType, BYTE precision, BYTE scale, BYTE size, long value)
{
    pBuf->ZW_ThermostatSetpointSet4byteV3Frame.cmdClass = COMMAND_CLASS_THERMOSTAT_SETPOINT;
    pBuf->ZW_ThermostatSetpointSet4byteV3Frame.cmd = THERMOSTAT_SETPOINT_SET;
    pBuf->ZW_ThermostatSetpointSet4byteV3Frame.level = setpointType & 0x0f;
    pBuf->ZW_ThermostatSetpointSet4byteV3Frame.level2 = ((precision << 5) & 0x07) + ((scale << 3) & 0x03) + (size & 0x07);

    if (size == 1)
    {
        pBuf->ZW_ThermostatSetpointSet1byteV3Frame.value1 = value & 0xff;
        return sizeof(pBuf->ZW_ThermostatSetpointSet1byteV3Frame);
    }
    else if (size == 2)
    {
        pBuf->ZW_ThermostatSetpointSet2byteV3Frame.value1 = value & 0xff;
        pBuf->ZW_ThermostatSetpointSet2byteV3Frame.value2 = (value >> 8) & 0xff;
        return sizeof(pBuf->ZW_ThermostatSetpointSet2byteV3Frame);
    }
    else if (size == 4)
    {
        pBuf->ZW_ThermostatSetpointSet4byteV3Frame.value1 = value & 0xff;
        pBuf->ZW_ThermostatSetpointSet4byteV3Frame.value2 = (value >> 8) & 0xff;
        pBuf->ZW_ThermostatSetpointSet4byteV3Frame.value3 = (value >> 16) & 0xff;
        pBuf->ZW_ThermostatSetpointSet4byteV3Frame.value4 = (value >> 24) & 0xff;
        return sizeof(pBuf->ZW_ThermostatSetpointSet4byteV3Frame);
    }
    else
    {
    }

    return 0;
}

BYTE EncapCmdBinarySensorGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_SensorBinaryGetFrame.cmdClass = COMMAND_CLASS_SENSOR_BINARY;
    pBuf->ZW_SensorBinaryGetFrame.cmd = SENSOR_BINARY_GET;

    return sizeof(pBuf->ZW_SensorBinaryGetFrame);
}

BYTE EncapCmdMultilevelSensorGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_SensorMultilevelGetFrame.cmdClass = COMMAND_CLASS_SENSOR_MULTILEVEL;
    pBuf->ZW_SensorMultilevelGetFrame.cmd = SENSOR_MULTILEVEL_GET;

    return sizeof(pBuf->ZW_SensorMultilevelGetFrame);
}

BYTE EncapCmdMultiChannelEndPointGetV3(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_MultiChannelEndPointGetV3Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V3;
    pBuf->ZW_MultiChannelEndPointGetV3Frame.cmd = MULTI_CHANNEL_END_POINT_GET_V3;

    return sizeof(pBuf->ZW_MultiChannelEndPointGetV3Frame);
}

BYTE EncapCmdMultiChannelCapabilityGetV3(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE endPoint)
{
    pBuf->ZW_MultiChannelCapabilityGetV3Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V3;
    pBuf->ZW_MultiChannelCapabilityGetV3Frame.cmd = MULTI_CHANNEL_CAPABILITY_GET_V3;
    pBuf->ZW_MultiChannelCapabilityGetV3Frame.properties1 = endPoint;

    return sizeof(pBuf->ZW_MultiChannelCapabilityGetV3Frame);
}

BYTE EncapCmdMultiChannelEncapsulationV2(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE SourEndPoint, BYTE DesEndPoint, ALL_EXCEPT_ENCAP *pCmds, BYTE cmdLength)
{
    assert(pCmds != NULL);
    assert(cmdLength > 0);

    pBuf->ZW_MultiChannelCmdEncapV2Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V2;
    pBuf->ZW_MultiChannelCmdEncapV2Frame.cmd = MULTI_CHANNEL_CMD_ENCAP_V2;
    pBuf->ZW_MultiChannelCmdEncapV2Frame.properties1 = SourEndPoint;
    pBuf->ZW_MultiChannelCmdEncapV2Frame.properties2 = DesEndPoint;
    memcpy(&(pBuf->ZW_MultiChannelCmdEncapV2Frame.encapFrame), pCmds, cmdLength);

    return (4 + cmdLength);
}

BYTE EncapCmdBatterylevelGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_BatteryGetFrame.cmdClass = COMMAND_CLASS_BATTERY;
    pBuf->ZW_BatteryGetFrame.cmd = BATTERY_GET;

    return sizeof(pBuf->ZW_BatteryGetFrame);
}

BYTE EncapCmdWakeUpIntervalSet(ZW_APPLICATION_TX_BUFFER *pBuf, BYTE seconds1, BYTE seconds2, BYTE seconds3, BYTE nodeid)
{
    pBuf->ZW_WakeUpIntervalSetFrame.cmdClass = COMMAND_CLASS_WAKE_UP;
    pBuf->ZW_WakeUpIntervalSetFrame.cmd = WAKE_UP_INTERVAL_SET;
    pBuf->ZW_WakeUpIntervalSetFrame.seconds1 = seconds1;
    pBuf->ZW_WakeUpIntervalSetFrame.seconds2 = seconds2;
    pBuf->ZW_WakeUpIntervalSetFrame.seconds3 = seconds3;
    pBuf->ZW_WakeUpIntervalSetFrame.nodeid = nodeid;

    return sizeof(pBuf->ZW_WakeUpIntervalSetFrame);
}

BYTE EncapCmdWakeUpIntervalGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_WakeUpIntervalGetFrame.cmdClass = COMMAND_CLASS_WAKE_UP;
    pBuf->ZW_WakeUpIntervalGetFrame.cmd = WAKE_UP_INTERVAL_GET;

    return sizeof(pBuf->ZW_WakeUpIntervalGetFrame);
}

BYTE EncapCmdWakeUpIntervalCapabilitiesGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_WakeUpIntervalCapabilitiesGetV2Frame.cmdClass = COMMAND_CLASS_WAKE_UP;
    pBuf->ZW_WakeUpIntervalCapabilitiesGetV2Frame.cmd = WAKE_UP_INTERVAL_CAPABILITIES_GET_V2;

    return sizeof(pBuf->ZW_WakeUpIntervalCapabilitiesGetV2Frame);
}

BYTE EncapCmdSecurityNonceGet(ZW_APPLICATION_TX_BUFFER *pBuf)
{
    pBuf->ZW_SecurityNonceGetFrame.cmdClass = COMMAND_CLASS_SECURITY;
    pBuf->ZW_SecurityNonceGetFrame.cmd = SECURITY_NONCE_GET;

    return sizeof(pBuf->ZW_SecurityNonceGetFrame);
}

ZW_DEVICE_TYPE Analysis_DeviceType(NODEINFO *nodeInfo)
{
    assert(nodeInfo != NULL);

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] basic[%02x] generic[%02x] specific[%02x] \n", 
            nodeInfo->nodeType.basic, nodeInfo->nodeType.generic, nodeInfo->nodeType.specific);
#endif

    switch (nodeInfo->nodeType.basic)
    {
        case BASIC_TYPE_CONTROLLER:
        {
            switch (nodeInfo->nodeType.generic)
            {
                case GENERIC_TYPE_GENERIC_CONTROLLER:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_PORTABLE_REMOTE_CONTROLLER
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_PORTABLE_SCENE_CONTROLLER)
                    {
                        return ZW_DEVICE_PORTABLE_CONTROLLER;
                    }
                    else
                    {
                        return ZW_DEVICE_PORTABLE_CONTROLLER;
                    }
                }
                break;
                default:
                    return ZW_DEVICE_UNKNOW;
                    break;
            }
        }
        break;
        case BASIC_TYPE_STATIC_CONTROLLER:
        {
            switch (nodeInfo->nodeType.generic)
            {
                case GENERIC_TYPE_STATIC_CONTROLLER:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_PC_CONTROLLER)
                    {
                        return ZW_DEVICE_STATIC_CONTROLLER;
                    }
                    else
                    {
                        return ZW_DEVICE_STATIC_CONTROLLER;
                    }
                }
                break;
                default:
                    return ZW_DEVICE_UNKNOW;
                    break;
            }
        }
        break;
        case BASIC_TYPE_SLAVE:
            //break;
        case BASIC_TYPE_ROUTING_SLAVE:
        {
            switch (nodeInfo->nodeType.generic)
            {
                case GENERIC_TYPE_SWITCH_BINARY:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_POWER_SWITCH_BINARY
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_SCENE_SWITCH_BINARY)
                    {
                        return ZW_DEVICE_BINARYSWITCH;
                    }
                    else
                    {
                        return ZW_DEVICE_BINARYSWITCH;
                    }
                }
                break;
                case GENERIC_TYPE_SWITCH_MULTILEVEL:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_POWER_SWITCH_MULTILEVEL)
                    {
                        return ZW_DEVICE_DIMMER;
                    }
                    else if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_MOTOR_MULTIPOSITION)
                    {
                        return ZW_DEVICE_CURTAIN;
                    }
                    else
                    {
                        return ZW_DEVICE_DIMMER;
                    }
                }
                break;
                case GENERIC_TYPE_SENSOR_BINARY:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_NOT_USED
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_ROUTING_SENSOR_BINARY)
                    {
                        return ZW_DEVICE_BINARYSENSOR;
                    }
                    else
                    {
                        return ZW_DEVICE_BINARYSENSOR;
                    }
                }
                break;
                case GENERIC_TYPE_SENSOR_MULTILEVEL:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_NOT_USED
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_ROUTING_SENSOR_MULTILEVEL)
                    {
                        return ZW_DEVICE_MULTILEVEL_SENSOR;
                    }
                    else
                    {
                        return ZW_DEVICE_MULTILEVEL_SENSOR;
                    }
                }
                break;
                case GENERIC_TYPE_ENTRY_CONTROL:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_NOT_USED
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_DOOR_LOCK
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_ADVANCED_DOOR_LOCK
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_SECURE_KEYPAD_DOOR_LOCK)
                    {
                        return ZW_DEVICE_DOORLOCK;
                    }
                    else
                    {
                        return ZW_DEVICE_DOORLOCK;
                    }
                }
                break;
                case GENERIC_TYPE_THERMOSTAT:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_NOT_USED
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_THERMOSTAT_GENERAL
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_THERMOSTAT_GENERAL_V2)
                    {
                        return ZW_DEVICE_THERMOSTAT;
                    }
                    else
                    {
                        return ZW_DEVICE_THERMOSTAT;
                    }
                }
                break;
                case GENERIC_TYPE_METER:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_NOT_USED
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_SIMPLE_METER
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_ADV_ENERGY_CONTROL
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_WHOLE_HOME_METER_SIMPLE)
                    {
                        return ZW_DEVICE_METER;
                    }
                    else
                    {
                        return ZW_DEVICE_METER;
                    }
                }
                break;
                case GENERIC_TYPE_SWITCH_REMOTE:
                {
                    if (nodeInfo->nodeType.specific == SPECIFIC_TYPE_NOT_USED
                            || nodeInfo->nodeType.specific == SPECIFIC_TYPE_SWITCH_REMOTE_MULTILEVEL)
                    {
                        return ZW_DEVICE_KEYFOB;
                    }
                    else
                    {
                        return ZW_DEVICE_KEYFOB;
                    }
                }
                break;
                default:
                    return ZW_DEVICE_UNKNOW;
                    break;
            }
        }
        break;
        default:
            break;
    }

    return ZW_DEVICE_UNKNOW;
}

int AnalysisDeviceTypebyManufacturerSpecific(WORD wNodeID, WORD manufacturerID, WORD productTypeID, WORD productID)
{
    BYTE value[16] = {0};

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] wNodeID[%04X] manufacturerID[%04x] productTypeID[%02x] productID[%02x] \n", 
                wNodeID, manufacturerID, productTypeID, productID);
#endif
   
    switch(manufacturerID)
    {
        case 0x60:
        {
            if(productTypeID == 0x02)
            {
                if(productID == 0x01                // SM103     EVERSPRING IND.CO.,LTD.
                    ||productID == 0x02)           // HSM02-0
                {
                    
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_BINARYSENSOR;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.binarysensor_status.sensor_type = ZW_BINARY_SENSOR_TYPE_DOOR_WINDOW;
                    return ZW_BINARY_SENSOR_TYPE_DOOR_WINDOW;
                }
            }
            else if (productTypeID == 0x0c)
            {
                if (productID == 0x01)      // SE812 Indoor Siren
                {
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_SIREN;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.siren_status.value = 0;
                    return ZW_DEVICE_SIREN;
                }
            }
        }
        break;
        case 0x011a:
        {
            if (productTypeID == 0x601)
            {
                if (productID == 0x903)     // ZWN-BDS
                {
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_BINARYSENSOR;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.binarysensor_status.sensor_type = ZW_BINARY_SENSOR_TYPE_DOOR_WINDOW;
                    return ZW_BINARY_SENSOR_TYPE_DOOR_WINDOW;
                }
            }
        }
        break;
        case 0x011f:
        {
            if(productTypeID == 0x01)
            {
                if(productID == 0x01)
                {
                    // PIRZWAVE1
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_BINARYSENSOR;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.binarysensor_status.sensor_type = ZW_BINARY_SENSOR_TYPE_MOTION;
                    return ZW_BINARY_SENSOR_TYPE_MOTION;
                }
            }
        }
        break;
        case 0x014a:
        {
            if (productTypeID == 0x01)
            {
                if (productID == 0x01)
                {
                    // PIR ZWAVE 2
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_BINARYSENSOR;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.binarysensor_status.sensor_type = ZW_BINARY_SENSOR_TYPE_MOTION;
                    return ZW_BINARY_SENSOR_TYPE_MOTION;
                }
            }
        }
        break;
        case 0x0116:
        {
            if (productTypeID == 0x01)
            {
                if(productID == 0x01)
                {
                    // HSP02 Motion Detector
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_BINARYSENSOR;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.binarysensor_status.sensor_type = ZW_BINARY_SENSOR_TYPE_MOTION;
                    return ZW_BINARY_SENSOR_TYPE_MOTION;
                }
            }
        }
        break;
        case 0x1e:
        {
            if(productTypeID == 0x02)
            {
                if(productID == 0x01)
                {
                    // HSM100, HomeSeer
                    zw_nodes->nodes[wNodeID].type = ZW_DEVICE_BINARYSENSOR;
                    memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                    zw_nodes->nodes[wNodeID].status.binarysensor_status.sensor_type = ZW_BINARY_SENSOR_TYPE_MOTION;

                    value[0] = 1;
                    ZW_ConfigurationSet(wNodeID, HSM100_CONFIGURATION_TYPE_ON_TIME, 0, 1, value);
                    return ZW_BINARY_SENSOR_TYPE_MOTION;
                }
            }
        }
        break;
        case 0x0084:
        {
            if(productTypeID == 0x0213)
            {
                if(productID == 0x020d)
                {
                    // WaterCop
                    if(zw_nodes->nodes[wNodeID].type == ZW_DEVICE_BINARYSWITCH)
                    {
                        memset(&(zw_nodes->nodes[wNodeID].status), 0, sizeof(ZW_DEVICE_STATUS));
                        zw_nodes->nodes[wNodeID].status.binaryswitch_status.type = ZW_BINARY_SWITCH_TYPE_WATER_VALVE;
                        return ZW_DEVICE_BINARYSWITCH;
                    }
                    
                }
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

WORD CorrectReportNodeId(WORD sourceNode, BYTE cmdClass, BYTE cmd, ZW_DEVICE_TYPE type)
{
    WORD i = 0;
    
    if (gGettingNodeStatus == ZW_STATUS_GET_NODE_STATUS
        && gGettingStatusNodeID == sourceNode
        && gNodeCmd == ((cmdClass << 8) | cmd)
        && exe_status == ZW_STATUS_IDLE)
    {
        return 0;
    }

    if(zw_nodes->nodes[sourceNode].type != type)
    {
        for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
        {
            if(zw_nodes->nodes[i].id
                &&(zw_nodes->nodes[i].id & 0xff) == (sourceNode&0xff)
                &&zw_nodes->nodes[i].type == type)
            {
                return i;
            }
        }
    }

    return 0;
}

void ApplicationCommandHandler(BYTE  rxStatus, WORD  sourceNode, ZW_APPLICATION_TX_BUFFER  *pCmd, BYTE  cmdLength)
{
    if(pCmd == NULL)
    {
        return;
    }

    DWORD i = 0;
    WORD wNodeId = 0;
    BYTE cmdClass = pCmd->ZW_Common.cmdClass;
    BYTE cmd = pCmd->ZW_Common.cmd;

    if(cmdClass != COMMAND_CLASS_MULTI_CHANNEL_V2
        &&zw_nodes->nodes[sourceNode].id == 0)
    {
        sourceNode |= (0x01 << 8);
    }

#ifdef ZWAVE_DRIVER_DEBUG
	DEBUG_INFO("[ZWave Driver] rxStatus[0x%x] sourceNode[0x%x] id[0x%x] cmdClass[0x%x], cmd[0x%x] \n", 
	        rxStatus, sourceNode, zw_nodes->nodes[sourceNode].id, cmdClass, cmd);
#endif

    switch (cmdClass)
    {
        case COMMAND_CLASS_BASIC:
        {
            if (cmd == BASIC_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.basic_status.value = pCmd->ZW_BasicReportFrame.value;

                if ((zw_nodes->nodes[sourceNode].type == ZW_DEVICE_BINARYSWITCH)
                        && zw_nodes->nodes[sourceNode].status.basic_status.value == 0xff)
                {
                    zw_nodes->nodes[sourceNode].status.basic_status.value = 0x01;
                }
            }
            else if (cmd == BASIC_SET)
            {
                zw_nodes->nodes[sourceNode].status.basic_status.value = pCmd->ZW_BasicReportFrame.value;
            }
        }
        break;
        case COMMAND_CLASS_SWITCH_BINARY:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_BINARYSWITCH);
            if(wNodeId)
                sourceNode = wNodeId;
            
            if (cmd == SWITCH_BINARY_REPORT)
            {
                BYTE value = pCmd->ZW_SwitchBinaryReportFrame.value;
                if (value == 0xff)
                    value = 0x01;

				zw_nodes->nodes[sourceNode].status.binaryswitch_status.value = value;
            }
            else
            {
            }
        }
        break;
        case COMMAND_CLASS_SWITCH_MULTILEVEL:
        {
            if (cmd == SWITCH_MULTILEVEL_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.dimmer_status.value = pCmd->ZW_SwitchMultilevelReportFrame.value;
            }
            else
            {
            }
        }
        break;
        case COMMAND_CLASS_ALARM:
        {
        	if(cmd == ALARM_REPORT)
        	{
                    BYTE alarm_type = 0, alarm_level = 0;
                    BYTE zw_alarm_status = 0;
                    
                    alarm_type = pCmd->ZW_AlarmReportFrame.alarmType;
                    alarm_level = pCmd->ZW_AlarmReportFrame.alarmLevel;

                    if (zw_nodes->nodes[sourceNode].type == ZW_DEVICE_SIREN)
                    {
                        if (alarm_type == 0x02) // ALARM_APPLIED command
                            zw_nodes->nodes[sourceNode].status.siren_status.value = 0x00;
                        else
                            zw_nodes->nodes[sourceNode].status.siren_status.value = 0x01;
                    }
                    else
                    {
                        if(cmdLength >= 9)
                        {
                            zw_alarm_status = pCmd->ZW_AlarmReport1byteV2Frame.zwaveAlarmStatus;

                            zw_nodes->nodes[sourceNode].status.binarysensor_status.value = zw_alarm_status;
                        }
                        else
                        {
                            zw_nodes->nodes[sourceNode].status.binarysensor_status.value = 0;
                        }
                    }
        	}
        	else if(cmd == ALARM_SET_V2)
        	{

        	}
        }
        break;
        case COMMAND_CLASS_SENSOR_BINARY:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_BINARYSENSOR);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == SENSOR_BINARY_REPORT)
            {
                BYTE sensorType = 0, value = 0;
                if (cmdLength == sizeof(pCmd->ZW_SensorBinaryReportV2Frame))
                {
                    sensorType = pCmd->ZW_SensorBinaryReportV2Frame.sensorType;
                    value = pCmd->ZW_SensorBinaryReportV2Frame.sensorValue;
                }
                else if(cmdLength == sizeof(pCmd->ZW_SensorBinaryReportFrame))
                {
                    value = pCmd->ZW_SensorBinaryReportFrame.sensorValue;
                }
                else
                {
                	return;
                }

                zw_nodes->nodes[sourceNode].status.binarysensor_status.sensor_type = sensorType;
                zw_nodes->nodes[sourceNode].status.binarysensor_status.value = value;
            }
            else
            {
                // Not support
            }
        }
        break;
        case COMMAND_CLASS_SENSOR_MULTILEVEL:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_MULTILEVEL_SENSOR);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == SENSOR_MULTILEVEL_REPORT)
            {
                /*
                 * |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
                 *         Command Class = COMMAND_CLASS_SENSOR_MULTILEVEL
                 *              Command = SENSOR_MULTILEVEL_REPORT
                 *                            Sensor Type 
                 * |       Precision       |     Scale     |         Size          |
                 *                           Sensor Value 1
                 *                           Sensor Value 2
                 *                               ......
                 *                           Sensor Value n
                 *
                 *
                 * Precision (3 bits)
                 * The precision field describes what the precision of the sensor value is. 
                 * The number indicates the number of decimals. 
                 * The decimal value  1025 with precision 2 is therefore  equal to 10.25. 
                 *
                 * Size (3 bits)
                 * The size field indicates the number of bytes that used for the sensor value. 
                 * This field can take values from 1 (001b), 2 (010b) or 4 (100b). 
                 *
                 */
                if (cmdLength < sizeof(ZW_SENSOR_MULTILEVEL_REPORT_1BYTE_FRAME))
                {
                    // error data
#ifdef ZWAVE_DRIVER_DEBUG
                    DEBUG_INFO("[ZWave Driver] multilevel sensor report data error. sourceNode[0x%x] \n", sourceNode);
#endif
                    return;
                }
                long value1 = 0, value2 = 0, value3 = 0, value4 = 0;
                BYTE sensor_type = 0, precision = 0, scale = 0, size = 0;
                double value = 0, precision1 = 0;

                sensor_type = pCmd->ZW_SensorMultilevelReport1byteFrame.sensorType;
                size = pCmd->ZW_SensorMultilevelReport1byteFrame.level & 0x07;
                scale = (pCmd->ZW_SensorMultilevelReport1byteFrame.level >> 3) & 0x03;
                precision = (pCmd->ZW_SensorMultilevelReport1byteFrame.level >> 5) & 0x07;
                precision1 = pow(10, precision);

                zw_nodes->nodes[sourceNode].status.multilevelsensor_status.sensor_type = sensor_type;
                zw_nodes->nodes[sourceNode].status.multilevelsensor_status.scale = scale;
                if (size == 1)
                {
                    if (cmdLength != sizeof(pCmd->ZW_SensorMultilevelReport1byteFrame))
                    {
                        // error data
                        return;
                    }
                    value1 = pCmd->ZW_SensorMultilevelReport1byteFrame.sensorValue1;

                    value = value1;
                }
                else if (size == 2)
                {
                    if (cmdLength != sizeof(pCmd->ZW_SensorMultilevelReport2byteFrame))
                    {
                        // error data
                        return;
                    }
                    value1 = pCmd->ZW_SensorMultilevelReport2byteFrame.sensorValue1;
                    value2 = pCmd->ZW_SensorMultilevelReport2byteFrame.sensorValue2;

                    value = value2 + (value1 << 8);
                }
                else if (size == 4)
                {
                    if (cmdLength != sizeof(pCmd->ZW_SensorMultilevelReport4byteFrame))
                    {
                        // error data
                        return;
                    }
                    value1 = pCmd->ZW_SensorMultilevelReport4byteFrame.sensorValue1;
                    value2 = pCmd->ZW_SensorMultilevelReport4byteFrame.sensorValue2;
                    value3 = pCmd->ZW_SensorMultilevelReport4byteFrame.sensorValue3;
                    value4 = pCmd->ZW_SensorMultilevelReport4byteFrame.sensorValue4;

                    value = value4 + (value3 << 8) + (value2 << 16) + (value1 << 24);
                }
                else
                {
                    // error data
                    return;
                }

                switch(sensor_type)
                {
                case 0x01: // Air temperature
                	if (scale == 0x01) // Fahrenheit (F)
            		{
            			value = ((value / precision1) - 32) / 1.8;
                        zw_nodes->nodes[sourceNode].status.multilevelsensor_status.scale = 0x00;
            		}
                    else if(scale == 0x00) // Celsius (C)
                    {
                        value = value / precision1;
                    }
                    else
                    {
                        // error happened, unexpect scale
                        value = value / precision1;
                    }
        		break;
                case 0x02: // general purpose value
                	value = value / precision1;
                	break;
                case 0x03:
                	value = value / precision1;
                	break;
                default:
                	value = value / precision1;
                	break;
                }

            	zw_nodes->nodes[sourceNode].status.multilevelsensor_status.value = value;
            }
            else
            {
                // Not support
            }
        }
        break;
        case COMMAND_CLASS_SECURITY:
        {
            SECURITY_COMMAND_DATA *pSecurityCmd;
            pSecurityCmd = (SECURITY_COMMAND_DATA *)malloc(sizeof(SECURITY_COMMAND_DATA));
            if(pSecurityCmd == NULL)
            {
#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_INFO("[ZWave Driver] malloc SECURITY_COMMAND_DATA error \n");
#endif
                return;
            }
            memset(pSecurityCmd, 0, sizeof(SECURITY_COMMAND_DATA));
            pSecurityCmd->rxStatus = rxStatus;
            pSecurityCmd->rxNodeID = sourceNode;
            pSecurityCmd->dataLength = cmdLength -1;
            memcpy(pSecurityCmd->cmdData, (BYTE *)pCmd + 1, pSecurityCmd->dataLength);

            pthread_t tid;
            pthread_create(&tid, NULL, ProcessIncomingSecure, (void *)pSecurityCmd);
            pthread_detach(tid);
            return;
        }
        break;
        case COMMAND_CLASS_DOOR_LOCK:
        {
            if (cmd == DOOR_LOCK_OPERATION_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.doorlock_status.doorLockMode = pCmd->ZW_DoorLockOperationReportFrame.doorLockMode;
                zw_nodes->nodes[sourceNode].status.doorlock_status.doorHandlesMode = pCmd->ZW_DoorLockOperationReportFrame.properties1;
                zw_nodes->nodes[sourceNode].status.doorlock_status.doorCondition = pCmd->ZW_DoorLockOperationReportFrame.doorCondition;
                zw_nodes->nodes[sourceNode].status.doorlock_status.lockTimeoutMinutes = pCmd->ZW_DoorLockOperationReportFrame.lockTimeoutMinutes;
                zw_nodes->nodes[sourceNode].status.doorlock_status.lockTimeoutSeconds = pCmd->ZW_DoorLockOperationReportFrame.lockTimeoutSeconds;
            }
            else if (cmd == DOOR_LOCK_CONFIGURATION_REPORT)
            {
                zw_nodes_configuration->configuration[sourceNode].doorlock_configuration.operationType = pCmd->ZW_DoorLockConfigurationReportFrame.operationType;
                zw_nodes_configuration->configuration[sourceNode].doorlock_configuration.doorHandlesMode = pCmd->ZW_DoorLockConfigurationReportFrame.properties1;
                zw_nodes_configuration->configuration[sourceNode].doorlock_configuration.lockTimeoutMinutes = pCmd->ZW_DoorLockConfigurationReportFrame.lockTimeoutMinutes;
                zw_nodes_configuration->configuration[sourceNode].doorlock_configuration.lockTimeoutSeconds = pCmd->ZW_DoorLockConfigurationReportFrame.lockTimeoutSeconds;
            }
            else
            {
                // Not support
            }
        }
        break;
        case COMMAND_CLASS_THERMOSTAT_FAN_MODE:
        {
            if (cmd == THERMOSTAT_FAN_MODE_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.thermostat_status.fan_mode = (pCmd->ZW_ThermostatFanModeReportFrame.level) & 0x0f;
            }
        }
        break;
        case COMMAND_CLASS_THERMOSTAT_FAN_STATE:
        {
            if (cmd == THERMOSTAT_FAN_STATE_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.thermostat_status.fan_mode = (pCmd->ZW_ThermostatFanStateReportFrame.level) & 0x0f;
            }
        }
        break;
        case COMMAND_CLASS_THERMOSTAT_MODE:
        {            
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_THERMOSTAT);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == THERMOSTAT_MODE_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.thermostat_status.mode = (pCmd->ZW_ThermostatModeReportFrame.level) & 0x1f;
                zw_nodes->nodes[sourceNode].status.thermostat_status.heat_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.cool_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_heat_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_cool_value = ILLEGAL_TEMPERATURE_VALUE;
            }
            else
            {
                // Not support
            }
        }
        break;
        case COMMAND_CLASS_THERMOSTAT_OPERATING_STATE:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_THERMOSTAT);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == THERMOSTAT_OPERATING_STATE_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.thermostat_status.mode = (pCmd->ZW_ThermostatOperatingStateReportFrame.level) & 0x0f;
                zw_nodes->nodes[sourceNode].status.thermostat_status.value = 0;
                zw_nodes->nodes[sourceNode].status.thermostat_status.heat_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.cool_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_heat_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_cool_value = ILLEGAL_TEMPERATURE_VALUE;
            }
        }
        break;
        case COMMAND_CLASS_THERMOSTAT_SETBACK:
        {
            if (cmd == THERMOSTAT_SETBACK_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.thermostat_status.setBack_state = (pCmd->ZW_ThermostatSetbackReportFrame.setbackState);
                zw_nodes->nodes[sourceNode].status.thermostat_status.setBack_type = (pCmd->ZW_ThermostatSetbackReportFrame.properties1) & 0x03;
            }
        }
        break;
        case COMMAND_CLASS_THERMOSTAT_SETPOINT:
        {            
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_THERMOSTAT);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == THERMOSTAT_SETPOINT_REPORT)
            {
                if(cmdLength < sizeof(ZW_THERMOSTAT_SETPOINT_REPORT_1BYTE_V3_FRAME))
                {
                    // incomplete data
                    return;
                }

                int mode = pCmd->ZW_ThermostatSetpointReport1byteV3Frame.level & 0x0f;
                int precision = (pCmd->ZW_ThermostatSetpointReport1byteV3Frame.level2 >> 5) & 0x07;
                int scale = (pCmd->ZW_ThermostatSetpointReport1byteV3Frame.level2 >> 3) & 0x03;
                int size = pCmd->ZW_ThermostatSetpointReport1byteV3Frame.level2 & 0x07;

                double precision1 = pow(10, precision);
                double value = 0;
                long value1 = 0, value2 = 0, value3 = 0, value4 = 0;

#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_INFO("[ZWave Driver] sourceNode[%x] gProperties[%d] mode[%d] precision[%d] precision1[%f] scale[%d] size[%d] \n", 
                            sourceNode, gProperties, mode, precision, precision1, scale, size);
#endif
                // gProperties == 0 indicates device target value changed.
                if (gProperties != 0 && gProperties != mode)
                {
                    // redundant data, discard it
                    return;
                }
                
                if (size == 1)
                {
                    value = pCmd->ZW_ThermostatSetpointReport1byteV3Frame.value1;
                }
                else if (size == 2)
                {
                    value1 = pCmd->ZW_ThermostatSetpointReport2byteV3Frame.value1;
                    value2 = pCmd->ZW_ThermostatSetpointReport2byteV3Frame.value2;
                    value = (value1 << 8) + value2;
                }
                else if (size == 4)
                {
                    value1 = pCmd->ZW_ThermostatSetpointReport4byteV3Frame.value1;
                    value2 = pCmd->ZW_ThermostatSetpointReport4byteV3Frame.value2;
                    value3 = pCmd->ZW_ThermostatSetpointReport4byteV3Frame.value3;
                    value4 = pCmd->ZW_ThermostatSetpointReport4byteV3Frame.value4;
                    value = (value1 << 24) + (value2 << 16) + (value3 << 8) + value4;
                }
                else
                {
                    // error size data
                    return;
                }
                
                zw_nodes->nodes[sourceNode].status.thermostat_status.mode = mode;
                if (scale == 0)
                {
                    // Celsius temperature
                    zw_nodes->nodes[sourceNode].status.thermostat_status.value = value / precision1;
                }
                else if (scale == 1)
                {
                    // Fahrenheit temperature
                    zw_nodes->nodes[sourceNode].status.thermostat_status.value = (value / precision1 - 32) / 1.8;
                }
                else
                {
                    // error scale data
                    return;
                }

                /*
                 * Since thermostat just report 1 type value per time.
                 * Set other values to invalid/illegal value.
                 * High level daemon will handle these invalid/illegal value.
                 */
                zw_nodes->nodes[sourceNode].status.thermostat_status.heat_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.cool_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_heat_value = ILLEGAL_TEMPERATURE_VALUE;
                zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_cool_value = ILLEGAL_TEMPERATURE_VALUE;
                if (mode == THERMOSTAT_MODE_HEATING)
                    zw_nodes->nodes[sourceNode].status.thermostat_status.heat_value = zw_nodes->nodes[sourceNode].status.thermostat_status.value;
                else if (mode == THERMOSTAT_MODE_COOLING)
                    zw_nodes->nodes[sourceNode].status.thermostat_status.cool_value = zw_nodes->nodes[sourceNode].status.thermostat_status.value;
                else if (mode == THERMOSTAT_MODE_ENERGY_SAVE_HEATING)
                    zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_heat_value = zw_nodes->nodes[sourceNode].status.thermostat_status.value;
                else if (mode == THERMOSTAT_MODE_ENERGY_SAVE_COOLING)
                    zw_nodes->nodes[sourceNode].status.thermostat_status.energe_save_cool_value = zw_nodes->nodes[sourceNode].status.thermostat_status.value;
                else
                {
#ifdef ZWAVE_DRIVER_DEBUG
                    DEBUG_INFO("[ZWave Driver] Unsupported thermostat mode[%d] \n", mode);
#endif
                    return;
                }


            }
            else
            {
                // Not support
            }
        }
        break;
        case COMMAND_CLASS_METER:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_METER);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == METER_REPORT)
            {
                if (cmdLength < sizeof(ZW_METER_REPORT_1BYTE_FRAME))
                {
                    return;
                }

                BYTE rate_type = ((pCmd->ZW_MeterReport1byteFrame.meterType) >> 5) & 0x03;
                BYTE meter_type = (pCmd->ZW_MeterReport1byteFrame.meterType) & 0x1f;
                BYTE precision = ((pCmd->ZW_MeterReport1byteFrame.properties1) >> 5) & 0x07;
                BYTE scale = ((pCmd->ZW_MeterReport1byteFrame.properties1) >> 3) & 0x03;
                BYTE size = (pCmd->ZW_MeterReport1byteFrame.properties1) & 0x07;
                double value = 0, previous_value = 0;
                WORD delta_time = 0;
                long value1 = 0, value2 = 0, value3 = 0, value4 = 0;

                if (size == 1)
                {
                    value = pCmd->ZW_MeterReport1byteV3Frame.meterValue1;

                    if ((cmdLength - 4) == (size * 2 + 2))
                    {
                        previous_value = pCmd->ZW_MeterReport1byteV3Frame.previousMeterValue1;

                        value1 = pCmd->ZW_MeterReport1byteV3Frame.deltaTime1;
                        value2 = pCmd->ZW_MeterReport1byteV3Frame.deltaTime2;
                        delta_time = (value1 << 8) + value2;
                    }

                }
                else if (size == 2)
                {
                    value1 = pCmd->ZW_MeterReport2byteV3Frame.meterValue1;
                    value2 = pCmd->ZW_MeterReport2byteV3Frame.meterValue2;
                    value = (value1 << 8) + value2;

                    if ((cmdLength - 4) == (size * 2 + 2))
                    {
                        value1 = pCmd->ZW_MeterReport2byteV3Frame.previousMeterValue1;
                        value2 = pCmd->ZW_MeterReport2byteV3Frame.previousMeterValue2;
                        previous_value = (value1 << 8) + value2;

                        value1 = pCmd->ZW_MeterReport2byteV3Frame.deltaTime1;
                        value2 = pCmd->ZW_MeterReport2byteV3Frame.deltaTime2;
                        delta_time = (value1 << 8) + value2;
                    }

                }
                else if (size == 4)
                {
                    value1 = pCmd->ZW_MeterReport4byteV4Frame.meterValue1;
                    value2 = pCmd->ZW_MeterReport4byteV4Frame.meterValue2;
                    value3 = pCmd->ZW_MeterReport4byteV4Frame.meterValue3;
                    value4 = pCmd->ZW_MeterReport4byteV4Frame.meterValue4;
                    value = (value1 << 24) + (value2 << 16) + (value3 << 8) + value4;

                    if ((cmdLength - 4) == (size * 2 + 2))
                    {
                        value1 = pCmd->ZW_MeterReport4byteV4Frame.previousMeterValue1;
                        value2 = pCmd->ZW_MeterReport4byteV4Frame.previousMeterValue2;
                        value3 = pCmd->ZW_MeterReport4byteV4Frame.previousMeterValue3;
                        value4 = pCmd->ZW_MeterReport4byteV4Frame.previousMeterValue4;
                        previous_value = (value1 << 24) + (value2 << 16) + (value3 << 8) + value4;

                        value1 = pCmd->ZW_MeterReport4byteV4Frame.deltaTime1;
                        value2 = pCmd->ZW_MeterReport4byteV4Frame.deltaTime2;
                        delta_time = (value1 << 8) + value2;
                    }

                }
                else
                {
                    // invalid size
                    return;
                }

                double precision1 = pow(10, precision);
                zw_nodes->nodes[sourceNode].status.meter_status.meter_type = meter_type;
                zw_nodes->nodes[sourceNode].status.meter_status.rate_type = rate_type;
                zw_nodes->nodes[sourceNode].status.meter_status.scale = scale;
                zw_nodes->nodes[sourceNode].status.meter_status.value = value / precision1;
                //zw_nodes->nodes[sourceNode].status.meter_status.delta_time = delta_time;
                zw_nodes->nodes[sourceNode].status.meter_status.previous_value = previous_value / precision1;
                if (meter_type == METER_REPORT_ELECTRIC_METER)
                {
                    //zw_nodes->nodes[sourceNode].type = ZW_DEVICE_ELECTRIC_METER;

                    if (scale == 0x00) // KWh
                    {
                    }
                    else if (scale == 0x02) // W
                    {
                    }
                    else if (scale == 0x04) // V
                    {
                    }
                    else if (scale == 0x05) // A
                    {
                    }

                }
                else if (meter_type == METER_REPORT_GAS_METER)
                {
                    //zw_nodes->nodes[sourceNode].type = ZW_DEVICE_GAS_METER;

                    if (scale == 0x00) // Cubic meters
                    {
                    }

                }
                else if (meter_type == METER_REPORT_WATER_METER)
                {
                    //zw_nodes->nodes[sourceNode].type = ZW_DEVICE_WATER_METER;

                    if (scale == 0x00) // Cubic meters
                    {
                    }

                }
                else
                {
                    // unknow meter type
                    ;
                }
            }
            else
            {
                //
            }
        }
        break;
        case COMMAND_CLASS_MULTI_CHANNEL_V3:
        {
            if (cmd == MULTI_CHANNEL_END_POINT_REPORT_V3)
            {
                memset(gRequestBuffer, 0, sizeof(gRequestBuffer));
                if (cmdLength >= 4)
                {
                    gRequestBufferLength = 1;
                    gRequestBuffer[0] = *((BYTE *)pCmd + 3);
                }

                gGettingNodeStatus = ZW_STATUS_MULTI_CHANNEL_ENDPOINTS_RECEIVED;
                return;
            }
            else if (cmd == MULTI_CHANNEL_CAPABILITY_REPORT_V3)
            {
                memset(gRequestBuffer, 0, sizeof(gRequestBuffer));
                gRequestBufferLength = cmdLength - 2;
                memcpy(gRequestBuffer, (BYTE *)pCmd + 2, gRequestBufferLength);

                gGettingNodeStatus = ZW_STATUS_MULTI_CHANNEL_CAPABILITY_RECEIVED;
                return;
            }
            else if (cmd == MULTI_CHANNEL_CMD_ENCAP_V3)
            {
                WORD nodeID = sourceNode | ((pCmd->ZW_MultiChannelCmdEncapV2Frame.properties1 & 0x07) << 8);
                ZW_APPLICATION_TX_BUFFER *pCmd1 = (ZW_APPLICATION_TX_BUFFER *) & (pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame);
                BYTE len = cmdLength - 4;

                ApplicationCommandHandler(rxStatus, nodeID, pCmd1, len);
                return;
            }
        }
        break;
        case COMMAND_CLASS_BATTERY:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_BATTERY);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == BATTERY_REPORT)
            {
                zw_nodes->nodes[sourceNode].status.battery_status.battery_level = pCmd->ZW_BatteryReportFrame.batteryLevel;
            }
            else
            {
                // Not support
            }
        }
        break;
        case COMMAND_CLASS_WAKE_UP:
        {
            wNodeId = CorrectReportNodeId(sourceNode, cmdClass, cmd, ZW_DEVICE_BATTERY);
            if(wNodeId)
                sourceNode = wNodeId;

            if (cmd == WAKE_UP_NOTIFICATION)
            {
                // the device is awake, we can do something
                ZW_DATA zw_data;
                memset(&zw_data, 0, sizeof(zw_data));
                zw_data.type = ZW_DATA_TYPE_WAKE_UP;
                zw_data.data.wakeup.nodeid = (sourceNode&0xff);
                ZW_sendMsgToProcessor((BYTE *)&zw_data, sizeof(zw_data));

#ifdef ZWAVE_DRIVER_DEBUG
               DEBUG_INFO("[ZWave Driver] node[0x%x] wake up notification \n", sourceNode);
#endif

                return;
            }
            else if (cmd == WAKE_UP_INTERVAL_REPORT)
            {
                BYTE seconds1 = 0, seconds2 = 0, seconds3 = 0, nodeid = 0;

                seconds1 = pCmd->ZW_WakeUpIntervalReportFrame.seconds1;
                seconds2 = pCmd->ZW_WakeUpIntervalReportFrame.seconds2;
                seconds3 = pCmd->ZW_WakeUpIntervalReportFrame.seconds3;
                nodeid = pCmd->ZW_WakeUpIntervalReportFrame.nodeid;

                zw_nodes->nodes[sourceNode].status.battery_status.interval_time = (seconds1 << 16) + (seconds2 << 8) + seconds3;
            }
            else if (cmd == WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2)
            {
                if (cmdLength != sizeof(ZW_WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2_FRAME))
                {
                    // error happened
                    return;
                }

                BYTE seconds1 = 0, seconds2 = 0, seconds3 = 0;

                seconds1 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds1;
                seconds2 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds2;
                seconds3 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds3;
                zw_nodes->nodes[sourceNode].status.battery_status.minWakeupIntervalSec = (seconds1 << 16) + (seconds2 << 8) + seconds3;

                seconds1 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds1;
                seconds2 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds2;
                seconds3 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds3;
                zw_nodes->nodes[sourceNode].status.battery_status.maxWakeupIntervalSec = (seconds1 << 16) + (seconds2 << 8) + seconds3;

                seconds1 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds1;
                seconds2 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds2;
                seconds3 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds3;
                zw_nodes->nodes[sourceNode].status.battery_status.defaultWakeupIntervalSec = (seconds1 << 16) + (seconds2 << 8) + seconds3;

                seconds1 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds1;
                seconds2 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds2;
                seconds3 = pCmd->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds3;
                zw_nodes->nodes[sourceNode].status.battery_status.wakeupIntervalStepSec = (seconds1 << 16) + (seconds2 << 8) + seconds3;

            }
        }
        break;
        case COMMAND_CLASS_ASSOCIATION:
        {
            if (cmd == ASSOCIATION_GROUPINGS_REPORT)
            {
                zw_nodes_association->association[sourceNode].supportedGroupings = pCmd->ZW_AssociationGroupingsReportFrame.supportedGroupings;
            }
            else if (cmd == ASSOCIATION_REPORT)
            {
                if (cmdLength < 5)
                {
                    return;
                }

                BYTE grouping, maxNodesSupported, reportsToFollow;
                BYTE i = 0;
                static BYTE num = 0;

                grouping = pCmd->ZW_AssociationReport1byteFrame.groupingIdentifier;
                maxNodesSupported = pCmd->ZW_AssociationReport1byteFrame.maxNodesSupported;
                reportsToFollow = pCmd->ZW_AssociationReport1byteFrame.reportsToFollow;
                zw_nodes_association->association[sourceNode].grouping[grouping].maxNodesSupported = maxNodesSupported;
                for (i = 0; i < cmdLength - 5; i++)
                {
                    zw_nodes_association->association[sourceNode].grouping[grouping].associatedNodes[num++].nodeID = *((BYTE *)pCmd + 5 + i);
                }
                zw_nodes_association->association[sourceNode].grouping[grouping].associatedNodesNum = num;
                if (reportsToFollow == 0)
                {
                    num = 0;
                }
                else
                {
                    num = zw_nodes_association->association[sourceNode].grouping[grouping].associatedNodesNum;
                    return;
                }
            }
            else if (cmd == ASSOCIATION_SPECIFIC_GROUP_REPORT_V2)
            {
                zw_nodes_association->association[sourceNode].specificGroup = pCmd->ZW_AssociationGroupingsReportV2Frame.supportedGroupings;
            }
        }
        break;
        case COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION:
        {
            if (cmd == COMMAND_RECORDS_SUPPORTED_REPORT)
            {
                if (cmdLength != sizeof(ZW_COMMAND_RECORDS_SUPPORTED_REPORT_FRAME))
                {
                    return;
                }
                BYTE record1 = 0, record2 = 0;
                BYTE maxCommandLength, VAndC, configurableSupported;

                maxCommandLength = (pCmd->ZW_CommandRecordsSupportedReportFrame.properties1 >> 2) & 0x3f;
                VAndC = (pCmd->ZW_CommandRecordsSupportedReportFrame.properties1 >> 1) & 0x01;
                configurableSupported = pCmd->ZW_CommandRecordsSupportedReportFrame.properties1 & 0x01;

                zw_nodes_association->association[sourceNode].maxCommandLength = maxCommandLength;
                zw_nodes_association->association[sourceNode].VAndC = VAndC;
                zw_nodes_association->association[sourceNode].configurableSupported = configurableSupported;

                record1 = pCmd->ZW_CommandRecordsSupportedReportFrame.freeCommandRecords1;
                record2 = pCmd->ZW_CommandRecordsSupportedReportFrame.freeCommandRecords2;
                zw_nodes_association->association[sourceNode].freeCommandRecords = (record1 << 8) + record2;

                record1 = pCmd->ZW_CommandRecordsSupportedReportFrame.maxCommandRecords1;
                record2 = pCmd->ZW_CommandRecordsSupportedReportFrame.maxCommandRecords2;
                zw_nodes_association->association[sourceNode].maxCommandRecords = (record1 << 8) + record2;

                gGettingNodeStatus = ZW_STATUS_ASSOCIATION_RECORDS_SUPPORTED_RECEIVED;
            }
            else if (cmd == COMMAND_CONFIGURATION_REPORT)
            {
                gGettingNodeStatus = ZW_STATUS_ASSOCIATION_CONFIGURATION_GET_RECEIVED;
            }
        }
        break;
        case COMMAND_CLASS_CONFIGURATION:
        {
            if (cmd == CONFIGURATION_REPORT)
            {
                if (cmdLength < 5)
                {
                    return;
                }

                long value1 = 0, value2 = 0, value3 = 0, value4 = 0;
                BYTE parameter = pCmd->ZW_ConfigurationReport1byteFrame.parameterNumber;
                BYTE size = (pCmd->ZW_ConfigurationReport1byteFrame.level) & 0x07;

                zw_nodes_configuration->configuration[sourceNode].HSM100_configuration.parameterNumber = parameter;
                if (size == 1)
                {
                    value1 = pCmd->ZW_ConfigurationReport1byteFrame.configurationValue1;
                    zw_nodes_configuration->configuration[sourceNode].HSM100_configuration.value = value1;
                }
                else if (size == 2)
                {
                    value1 = pCmd->ZW_ConfigurationReport2byteFrame.configurationValue1;
                    value2 = pCmd->ZW_ConfigurationReport2byteFrame.configurationValue2;
                    zw_nodes_configuration->configuration[sourceNode].HSM100_configuration.value = (value1 << 8) + value2;
                }
                else if (size == 4)
                {
                    value1 = pCmd->ZW_ConfigurationReport4byteFrame.configurationValue1;
                    value2 = pCmd->ZW_ConfigurationReport4byteFrame.configurationValue2;
                    value3 = pCmd->ZW_ConfigurationReport4byteFrame.configurationValue3;
                    value4 = pCmd->ZW_ConfigurationReport4byteFrame.configurationValue4;
                    zw_nodes_configuration->configuration[sourceNode].HSM100_configuration.value = (value1 << 24) + (value2 << 16) + (value3 << 8) + value4;
                }
                else
                {
                }
            }
        }
        break;
        case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
        {
            if(cmd == MANUFACTURER_SPECIFIC_REPORT)
            {
                if(cmdLength != sizeof(ZW_MANUFACTURER_SPECIFIC_REPORT_FRAME))
                {
                    return;
                }

                BYTE manufacturerId1 = 0, manufacturerId2 = 0, productTypeId1 = 0, productTypeId2 = 0, productId1 = 0, productId2 = 0;
                WORD manufactureId = 0, productTypeId = 0, productId = 0;

                manufacturerId1 = pCmd->ZW_ManufacturerSpecificReportFrame.manufacturerId1;
                manufacturerId2 = pCmd->ZW_ManufacturerSpecificReportFrame.manufacturerId2;
                productTypeId1 = pCmd->ZW_ManufacturerSpecificReportFrame.productTypeId1;
                productTypeId2 = pCmd->ZW_ManufacturerSpecificReportFrame.productTypeId2;
                productId1 = pCmd->ZW_ManufacturerSpecificReportFrame.productId1;
                productId2 = pCmd->ZW_ManufacturerSpecificReportFrame.productId2;

                manufactureId = manufacturerId1;
                manufactureId = (manufactureId << 8) + manufacturerId2;
                productTypeId = productTypeId1;
                productTypeId = (productTypeId << 8) + productTypeId2;
                productId = productId1;
                productId = (productId << 8) + productId2;

                zw_nodes->nodes[sourceNode].status.manufacturer_specific_info.manufacturerID = manufactureId;
                zw_nodes->nodes[sourceNode].status.manufacturer_specific_info.productTypeID = productTypeId;
                zw_nodes->nodes[sourceNode].status.manufacturer_specific_info.productID = productId;
                
            }
        }
        break;
        default:
            //break;
            return;
    }

#ifdef ZWAVE_DRIVER_DEBUG
	DEBUG_INFO("[ZWave Driver] gGettingNodeStatus[%u] gGettingStatusNodeID[0x%x] gNodeCmd[0x%x] exe_status[%u] sourceNode[0x%x] id[0x%x]\n", 
	        gGettingNodeStatus, gGettingStatusNodeID, gNodeCmd, exe_status, sourceNode, zw_nodes->nodes[sourceNode].id);
#endif

    if (gGettingNodeStatus == ZW_STATUS_GET_NODE_STATUS
        && gGettingStatusNodeID == sourceNode
        && gNodeCmd == ((cmdClass << 8) | cmd)
        && exe_status == ZW_STATUS_IDLE)
    {
        gGettingStatusNodeID = ILLEGAL_NODE_ID;
        gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS_RECEIVED;
    }
    else
    {
        if (cbFuncReportHandle)
        {
        	if(zw_nodes->nodes[sourceNode].id)
        	{
			StatusUpdateTimer->timer[sourceNode] = 0;
			cbFuncReportHandle(&(zw_nodes->nodes[sourceNode]));
        	}
        }
    }

}

void ApplicationControllerUpdate(BYTE  bStatus, BYTE  bNodeID, BYTE  *pCmd, BYTE  bLen)
{

    switch (bStatus)
    {
        case UPDATE_STATE_NODE_INFO_RECEIVED:
        {
            unsigned i = 3;
            for (i = 3; i < bLen; i++)
            {
                zw_nodes->nodes[bNodeID].cmdclass_bitmask[(pCmd[i] - 1) >> 3] |= 0x01 << ((pCmd[i] - 1) & 0x7);
            }

            for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i++)
            {
                if (zw_nodes->nodes[i].id
                        && ((zw_nodes->nodes[i].id & 0xff) == bNodeID)
                        && exe_status != ZW_STATUS_REQUEST_NODEINFO)
                {
                    StatusUpdateTimer->timer[i] = UPDATE_NODE_STATUS_TIMER;
                    ZW_timerStart(TIMER_UPDATE_NODE_STATUS, 0, 1, 0);
                }
            }
            
            if(exe_status == ZW_STATUS_REQUEST_NODEINFO
                &&gNodeRequest == bNodeID)
                exe_status = ZW_STATUS_NODEINFO_RECEIVED;
        }
        break;
        case UPDATE_STATE_NODE_INFO_REQ_DONE:
        {
        }
        break;
        case UPDATE_STATE_NODE_INFO_REQ_FAILED:
        {
            if(exe_status == ZW_STATUS_REQUEST_NODEINFO)
                exe_status = ZW_STATUS_NODEINFO_RECEIVED_FAIL;
        }
        break;
        case UPDATE_STATE_ROUTING_PENDING:
            break;
        case UPDATE_STATE_NEW_ID_ASSIGNED:
        {
        }
        break;
        case UPDATE_STATE_DELETE_DONE:
        {
        }
        break;
        case UPDATE_STATE_SUC_ID:
            break;
        default:
            break;
    }

    // cmd_status = ZW_STATUS_IDLE;

}

void FrameHandler(BYTE *pFrame, BYTE len)
{
    switch (pFrame[IDX_CMD_ID])
    {
        case FUNC_ID_APPLICATION_COMMAND_HANDLER:
        case FUNC_ID_PROMISCUOUS_APPLICATION_COMMAND_HANDLER:
        {
            /* REQ | 0x04 | rxStatus | sourceNode | cmdLength | pCmd[ ]
            * When a foreign frame is received in promiscuous mode:
            * REQ | 0xD1 | rxStatus | sourceNode | cmdLength | pCmd[ ] | destNode
            */
            BYTE cmd[ZW_BUF_SIZE] = {0};
            BYTE rxStatus = 0, bNodeID = 0, cmdLength = 0;
            BYTE *pCmd = NULL;
    
            if (pFrame[IDX_LEN] < 6)
            {
                // error data
                break;
            }
    
            rxStatus = pFrame[4];
            bNodeID = pFrame[5];
            cmdLength = pFrame[6];
    
            if (cmdLength)
            {
                pCmd = &(pFrame[7]);
                memcpy(cmd, pCmd, cmdLength);
            }
            ApplicationCommandHandler(rxStatus, bNodeID, (ZW_APPLICATION_TX_BUFFER *)cmd, cmdLength);
    
        }
        break;
        case FUNC_ID_ZW_APPLICATION_CONTROLLER_UPDATE:
        {
            /* Act as waiting for node information, or update controller
            * REQ | 0x49 | bStatus | bNodeID | bLen | basic | generic | specific | commandclasses[ ] */
            BYTE cmd[ZW_BUF_SIZE] = {0};
            BYTE bStatus = 0, bNodeID = 0, bLen = 0;
            BYTE *pCmd = NULL;
    
            if (pFrame[IDX_LEN] < 6)
            {
                // error data
                break;
            }
            bStatus = pFrame[4];
            bNodeID = pFrame[5];
            bLen = pFrame[6];
    
            if (bLen)
            {
                pCmd = &(pFrame[7]);
                memcpy(cmd, pCmd, bLen);
            }
            ApplicationControllerUpdate(bStatus, bNodeID, cmd, bLen);
        }
        break;
        case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
            /* REQ | 0x4A | funcID | bStatus | bSource | bLen | basic | generic | specific | cmdclasses[ ] */
            //break;
        case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
        {
            /* REQ | 0x4B | funcID | bStatus | bSource | bLen | basic | generic | specific | cmdclasses[ ] */
            LEARN_INFO learn_info;
            BYTE funcID = 0, bStatus = 0, bSource = 0, bLen = 0;
            BYTE *pCmd = NULL;
    
            if (pFrame[IDX_LEN] < 7)
            {
                // error data
#ifdef ZWAVE_DRIVER_DEBUG
                DEBUG_ERROR("[ZWave Driver] error sof received. funcId[%u], len[%u] \n", pFrame[IDX_CMD_ID], pFrame[IDX_LEN]);
#endif
                break;
            }
            funcID = pFrame[4];
            bStatus = pFrame[5];
            bSource = pFrame[6];
            bLen = pFrame[7];
    
            memset(&learn_info, 0, sizeof(learn_info));
            learn_info.bStatus = bStatus;
            learn_info.bSource = bSource;
            learn_info.bLen = bLen;
            if (bLen)
            {
                learn_info.pCmd = (BYTE *)malloc(bLen + 1);
                if (learn_info.pCmd == NULL)
                {
                    return;
                }
                memset(learn_info.pCmd, 0, bLen + 1);
                pCmd = &(pFrame[8]);
                memcpy(learn_info.pCmd, pCmd, bLen);
            }

            if (cbFuncLearnModeHandler)
            {
                cbFuncLearnModeHandler(&learn_info);
            }
            
            if(learn_info.pCmd)
            {
                free(learn_info.pCmd);
            }
        }
        break;
        case FUNC_ID_ZW_SEND_DATA:
        {
            /*REQ | 0x13 | funcID | txStatus */
            BYTE funcID = 0, txStatus = 0;
    
            if (pFrame[IDX_LEN] < 5)
            {
                // error data
                break;
            }
            funcID = pFrame[4];
            txStatus = pFrame[5];

            if (cbFuncSendData)
            {
                cbFuncSendData(txStatus);
            }
        
        }
        break;
        case FUNC_ID_ZW_SET_DEFAULT:
        {
            /*REQ | 0x42 | funcID*/
            if (pFrame[IDX_LEN] < 4)
            {
                // error data
                break;
            }
    
            if (cbFuncSetDefaultHandler)
            {
                cbFuncSetDefaultHandler();
            }
            
        }
        break;
        case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
        {
            /* REQ | 0x61 | funcID | txStatus */
            BYTE funcID = 0, txStatus = 0;
    
            if (pFrame[IDX_LEN] < 5)
            {
                // error data
                break;
            }
            funcID = pFrame[4];
            txStatus = pFrame[5];
    
            if (cbFuncRemoveFailedNodeHandler)
            {
                cbFuncRemoveFailedNodeHandler(txStatus);
            }
            
        }
        break;
        case FUNC_ID_MEMORY_PUT_BUFFER:
        {
            //REQ | 0x24 | funcID
            if(cbFuncMemoryPutBuffer)
            {
                cbFuncMemoryPutBuffer();
            }
        }
        break;
        case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
        {
            // REQ | 0x46 | funcID | bStatus
            BYTE funcID = 0, txStatus = 0;
    
            if (pFrame[IDX_LEN] < 5)
            {
                // error data
                break;
            }
            funcID = pFrame[4];
            txStatus = pFrame[5];
    
            if (cbFuncAssignReturnRouteHandler)
            {
                cbFuncAssignReturnRouteHandler(txStatus);
            }
            
        }
        break;
        case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
        {
            // REQ | 0x47 | funcID | bStatus
            BYTE funcID = 0, txStatus = 0;
    
            if (pFrame[IDX_LEN] < 5)
            {
                // error data
                break;
            }
            funcID = pFrame[4];
            txStatus = pFrame[5];
    
            if (cbFuncDeleteReturnRouteHandler)
            {
                cbFuncDeleteReturnRouteHandler(txStatus);
            }
            
        }
        break;
        default:
            break;
    }
}

ZW_STATUS ZW_GetMultiChannelEndPoint(BYTE bNodeID, BYTE *pEndPoints)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdMultiChannelEndPointGetV3(&appTxBuffer);

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        return retVal;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_MULTI_CHANNEL_ENDPOINTS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        *pEndPoints = gRequestBuffer[0] & 0x7f;
    }

    gGettingNodeStatus = ZW_STATUS_IDLE;
    return retVal;
}


ZW_STATUS ZW_GetMultiChannelCapability(BYTE bNodeID, BYTE bEndPoint, BYTE *pCmd, BYTE *cmdLength)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdMultiChannelCapabilityGetV3(&appTxBuffer, bEndPoint);

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        return retVal;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_MULTI_CHANNEL_CAPABILITY_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        *cmdLength = gRequestBufferLength;
        memcpy(pCmd, gRequestBuffer, gRequestBufferLength);
    }

    gGettingNodeStatus = ZW_STATUS_IDLE;
    return retVal;
}


ZW_STATUS ZW_GetBatterylevel(ZW_DEVICE_INFO *dev_info)
{
    assert(dev_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = dev_info->id & 0xff;
    endPoint = (dev_info->id >> 8) & 0xff;


#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] get node [0x%x] battery level \n", dev_info->id);
#endif

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = dev_info->id;
    gNodeCmd = (COMMAND_CLASS_BATTERY << 8) | BATTERY_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdBatterylevelGet(&appTxBuffer);

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        dev_info->status.battery_status.battery_level = zw_nodes->nodes[dev_info->id].status.battery_status.battery_level;
    }

END:
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] get node [0x%x] battery level. ret[%d] level [%u] \n", dev_info->id, retVal, dev_info->status.battery_status.battery_level);
#endif

    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}

ZW_STATUS ZW_SetWakeUpInterval(WORD nodeID, unsigned int interval_time, WORD rxNodeID)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    BYTE seconds1 = 0, seconds2 = 0, seconds3 = 0;

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;

    seconds1 = (interval_time >> 16) & 0xff;
    seconds2 = (interval_time >> 8) & 0xff;
    seconds3 = interval_time & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdWakeUpIntervalSet(&appTxBuffer, seconds1, seconds2, seconds3, (rxNodeID & 0xff));

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_GetWakeUpInterval(WORD nodeID, unsigned int *interval_time, WORD *rxNodeID)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;

#ifdef ZWAVE_DRIVER_DEBUG
    DEBUG_INFO("[ZWave Driver] get node [0x%x] wakeup interval \n", nodeID);
#endif

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = nodeID;
    gNodeCmd = (COMMAND_CLASS_WAKE_UP << 8) | WAKE_UP_INTERVAL_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdWakeUpIntervalGet(&appTxBuffer);

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (interval_time)
            *interval_time = zw_nodes->nodes[nodeID].status.battery_status.interval_time;
    }

END:
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] get node [0x%x] wakeup interval. ret[%d] interval time [%u] \n", nodeID, retVal, zw_nodes->nodes[nodeID].status.battery_status.interval_time);
#endif

    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}

ZW_STATUS ZW_GetWakeUpIntervalCapabilities(WORD nodeID, unsigned int *pMinWakeupIntervalSec, unsigned int *pMaxWakeupIntervalSec, unsigned int *pDefaultWakeupIntervalSec, unsigned int *pWakeupIntervalStepSec)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;

#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] get node [0x%u] wakeup interval capabilities \n", nodeID);
#endif

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = nodeID;
    gNodeCmd = (COMMAND_CLASS_WAKE_UP << 8) | WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdWakeUpIntervalCapabilitiesGet(&appTxBuffer);

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (pMinWakeupIntervalSec)
            *pMinWakeupIntervalSec = zw_nodes->nodes[nodeID].status.battery_status.minWakeupIntervalSec;
        if (pMaxWakeupIntervalSec)
            *pMaxWakeupIntervalSec = zw_nodes->nodes[nodeID].status.battery_status.maxWakeupIntervalSec;
        if (pDefaultWakeupIntervalSec)
            *pDefaultWakeupIntervalSec = zw_nodes->nodes[nodeID].status.battery_status.defaultWakeupIntervalSec;
        if (pWakeupIntervalStepSec)
            *pWakeupIntervalStepSec = zw_nodes->nodes[nodeID].status.battery_status.wakeupIntervalStepSec;
    }

END:
#ifdef ZWAVE_DRIVER_DEBUG
        DEBUG_INFO("[ZWave Driver] get node [0x%x] wakeup interval capabilities. ret[%d] minWakeupIntervalSec[%u] maxWakeupIntervalSec[%u] defaultWakeupIntervalSec[%u] wakeupIntervalStepSec[%u]\n", nodeID, retVal,
        zw_nodes->nodes[nodeID].status.battery_status.minWakeupIntervalSec,
        zw_nodes->nodes[nodeID].status.battery_status.maxWakeupIntervalSec,
        zw_nodes->nodes[nodeID].status.battery_status.defaultWakeupIntervalSec,
        zw_nodes->nodes[nodeID].status.battery_status.wakeupIntervalStepSec);
#endif

    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}

ZW_STATUS ZW_SetBinarySwitchStatus(WORD NodeID, BYTE value)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    if (value > 0)
    {
        value = BINARYSWITCH_STATUS_ON;
    }

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdBinarySet(&appTxBuffer, value);
    if (endPoint)
    {
        ALL_EXCEPT_ENCAP appTxBuffer1;
        memcpy(&appTxBuffer1, &appTxBuffer, sizeof(appTxBuffer));

        memset(&appTxBuffer, 0, sizeof(appTxBuffer));
        len = EncapCmdMultiChannelEncapsulationV2(&appTxBuffer, 0x01, endPoint, &appTxBuffer1, len);
    }

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_GetBinarySwitchStatus(WORD NodeID, BYTE *value)
{
    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_SWITCH_BINARY << 8) | SWITCH_BINARY_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdBinaryGet(&appTxBuffer);
    if (endPoint)
    {
        ALL_EXCEPT_ENCAP appTxBuffer1;
        memcpy(&appTxBuffer1, &appTxBuffer, sizeof(appTxBuffer));

        memset(&appTxBuffer, 0, sizeof(appTxBuffer));
        len = EncapCmdMultiChannelEncapsulationV2(&appTxBuffer, 0x01, endPoint, &appTxBuffer1, len);
    }

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
		if (value)
			*value = zw_nodes->nodes[NodeID].status.binaryswitch_status.value;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}

ZW_STATUS ZW_SetMultiLevelSwitchStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    if (device_info->type == ZW_DEVICE_DIMMER)
    {
        if (device_info->status.dimmer_status.value <= 0x63 || device_info->status.dimmer_status.value == 0xff)
        {
            len = EncapCmdMultilevelSet(&appTxBuffer, device_info->status.dimmer_status.value);
        }
        else
        {
            return ZW_STATUS_FAIL;
        }
    }
    else if (device_info->type == ZW_DEVICE_CURTAIN)
    {
        if (device_info->status.curtain_status.value <= 0x63 || device_info->status.curtain_status.value == 0xff)
        {
            len = EncapCmdMultilevelSet(&appTxBuffer, device_info->status.curtain_status.value);
        }
        else if (device_info->status.curtain_status.value == CURTAIN_STOP)
        {
            len = EncapCmdMultilevelSwitchStopLevelChange(&appTxBuffer);
        }
        else
        {
            return ZW_STATUS_FAIL;
        }
    }
    else
    {
        return ZW_STATUS_FAIL;
    }
    if (endPoint)
    {
        ALL_EXCEPT_ENCAP appTxBuffer1;
        memcpy(&appTxBuffer1, &appTxBuffer, sizeof(appTxBuffer));

        memset(&appTxBuffer, 0, sizeof(appTxBuffer));
        len = EncapCmdMultiChannelEncapsulationV2(&appTxBuffer, 0x01, endPoint, &appTxBuffer1, len);
    }

    retVal =  ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_GetMultiLevelSwitchStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = device_info->id;
    gNodeCmd = (COMMAND_CLASS_SWITCH_MULTILEVEL << 8) | SWITCH_MULTILEVEL_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdMultilevelGet(&appTxBuffer);
    if (endPoint)
    {
        ALL_EXCEPT_ENCAP appTxBuffer1;
        memcpy(&appTxBuffer1, &appTxBuffer, sizeof(appTxBuffer));

        memset(&appTxBuffer, 0, sizeof(appTxBuffer));
        len = EncapCmdMultiChannelEncapsulationV2(&appTxBuffer, 0x01, endPoint, &appTxBuffer1, len);
    }

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (device_info->type == ZW_DEVICE_DIMMER)
        {
            device_info->status.dimmer_status.value = zw_nodes->nodes[device_info->id].status.dimmer_status.value;
        }
        else if (device_info->type == ZW_DEVICE_CURTAIN)
        {
            device_info->status.curtain_status.value = zw_nodes->nodes[device_info->id].status.curtain_status.value;
        }
        else
        {
        }
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}

/******************************
Sensor Type            Value
Reserved                 0x00
General purpose       0x01
Smoke                     0x02
CO                           0x03
CO2                         0x04
Heat                         0x05
Water                       0x06
Freeze                      0x07
Tamper                    0x08
Aux                          0x09
Door/Window            0x0A
Tilt                           0x0B
Motion                      0x0C
Glass Break              0x0D
Reserved                 0x0E-0xFE
Return 1st Sensor Type on supported list      0xFF
************************************/
ZW_STATUS ZW_GetBinarySensorStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = device_info->id;
    gNodeCmd = (COMMAND_CLASS_SENSOR_BINARY << 8) | SENSOR_BINARY_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdBinarySensorGet(&appTxBuffer);
    if (endPoint)
    {
        ALL_EXCEPT_ENCAP appTxBuffer1;
        memcpy(&appTxBuffer1, &appTxBuffer, sizeof(appTxBuffer));

        memset(&appTxBuffer, 0, sizeof(appTxBuffer));
        len = EncapCmdMultiChannelEncapsulationV2(&appTxBuffer, 0x01, endPoint, &appTxBuffer1, len);
    }

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = REQUEST_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        device_info->status.binarysensor_status.value = zw_nodes->nodes[device_info->id].status.binarysensor_status.value;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}

ZW_STATUS ZW_GetMultilevelSensorStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = device_info->id;
    gNodeCmd = (COMMAND_CLASS_SENSOR_MULTILEVEL << 8) | SENSOR_MULTILEVEL_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdMultilevelSensorGet(&appTxBuffer);
    if (endPoint)
    {
        ALL_EXCEPT_ENCAP appTxBuffer1;
        memcpy(&appTxBuffer1, &appTxBuffer, sizeof(appTxBuffer));

        memset(&appTxBuffer, 0, sizeof(appTxBuffer));
        len = EncapCmdMultiChannelEncapsulationV2(&appTxBuffer, 0x01, endPoint, &appTxBuffer1, len);
    }

    retVal = ZW_SendData(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
    	memcpy(&(device_info->status), &(zw_nodes->nodes[device_info->id].status), sizeof(ZW_DEVICE_STATUS));
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}


ZW_STATUS ZW_SetDoorLockStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdDoorLockOperationSet(&appTxBuffer, device_info->status.doorlock_status.doorLockMode);

    retVal =  ZW_SendDataSecure(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_GetDoorLockStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = device_info->id;
    gNodeCmd = (COMMAND_CLASS_DOOR_LOCK << 8) | DOOR_LOCK_OPERATION_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdDoorLockOperationGet(&appTxBuffer);

    retVal =  ZW_SendDataSecure(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        memcpy(&(device_info->status.doorlock_status), &(zw_nodes->nodes[device_info->id].status.doorlock_status), sizeof(ZW_DOORLOCK_STATUS));
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}


ZW_STATUS ZW_SetDoorLockConfiguration(WORD nodeID, ZW_DOORLOCK_CONFIGURATION *doorLock_configuration)
{
    assert(doorLock_configuration != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdDoorLockConfigurationSet(&appTxBuffer, doorLock_configuration->operationType, doorLock_configuration->doorHandlesMode, doorLock_configuration->lockTimeoutMinutes, doorLock_configuration->lockTimeoutSeconds);

    retVal =  ZW_SendDataSecure(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_GetDoorLockConfiguration(WORD nodeID, ZW_DOORLOCK_CONFIGURATION *doorLock_configuration)
{
    assert(doorLock_configuration != NULL);

    ZW_APPLICATION_TX_BUFFER appTxBuffer;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = nodeID & 0xff;
    endPoint = (nodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = nodeID;
    gNodeCmd = (COMMAND_CLASS_DOOR_LOCK << 8) | DOOR_LOCK_CONFIGURATION_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    len = EncapCmdDoorLockConfigurationGet(&appTxBuffer);

    retVal =  ZW_SendDataSecure(bNodeID, (BYTE *)&appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        memcpy(doorLock_configuration, &(zw_nodes_configuration->configuration[nodeID].doorlock_configuration), sizeof(ZW_DOORLOCK_CONFIGURATION));
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}

ZW_STATUS ZW_ThermostateFanModeSet(WORD NodeID, BYTE bOff, BYTE fanMode)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_FAN_MODE;
    appTxBuffer[len++] = THERMOSTAT_FAN_MODE_SET;
    appTxBuffer[len++] = ((bOff & 0x01) << 7) + (fanMode & 0x7f);

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}


ZW_STATUS ZW_ThermostateFanModeGet(WORD NodeID, BYTE *bOff, BYTE *fanMode)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_THERMOSTAT_FAN_MODE << 8) | THERMOSTAT_FAN_MODE_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_FAN_MODE;
    appTxBuffer[len++] = THERMOSTAT_FAN_MODE_GET;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (bOff)
            *bOff = 0;
        if (fanMode)
            *fanMode = zw_nodes->nodes[NodeID].status.thermostat_status.fan_mode;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}


ZW_STATUS ZW_ThermostateFanStateGet(WORD NodeID, BYTE *fanOperatingState)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_THERMOSTAT_FAN_STATE << 8) | THERMOSTAT_FAN_STATE_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_FAN_STATE;
    appTxBuffer[len++] = THERMOSTAT_FAN_STATE_GET;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (fanOperatingState)
            *fanOperatingState = zw_nodes->nodes[NodeID].status.thermostat_status.fan_mode;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}


ZW_STATUS ZW_ThermostateModeSet(WORD NodeID, BYTE mode)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_MODE;
    appTxBuffer[len++] = THERMOSTAT_MODE_SET;
    appTxBuffer[len++] = mode & 0x1f;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}


ZW_STATUS ZW_ThermostateModeGet(WORD NodeID, BYTE *mode)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_THERMOSTAT_MODE << 8) | THERMOSTAT_MODE_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_MODE;
    appTxBuffer[len++] = THERMOSTAT_MODE_GET;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (mode)
            *mode = zw_nodes->nodes[NodeID].status.thermostat_status.mode;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}

ZW_STATUS ZW_ThermostateOperatingStateGet(WORD NodeID, BYTE *operatingState)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_THERMOSTAT_OPERATING_STATE << 8) | THERMOSTAT_OPERATING_STATE_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_OPERATING_STATE;
    appTxBuffer[len++] = THERMOSTAT_OPERATING_STATE_GET;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (operatingState)
            *operatingState = zw_nodes->nodes[NodeID].status.thermostat_status.mode;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}

ZW_STATUS ZW_ThermostateSetBackSet(WORD NodeID, BYTE setBack_type, BYTE setBack_state)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_SETBACK;
    appTxBuffer[len++] = THERMOSTAT_SETBACK_SET;
    appTxBuffer[len++] = setBack_type & 0x03;
    appTxBuffer[len++] = setBack_state;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_ThermostateSetBackGet(WORD NodeID, BYTE *setBack_type, BYTE *setBack_state)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_THERMOSTAT_SETBACK << 8) | THERMOSTAT_SETBACK_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_SETBACK;
    appTxBuffer[len++] = THERMOSTAT_SETBACK_GET;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (setBack_type)
            *setBack_type = zw_nodes->nodes[NodeID].status.thermostat_status.setBack_type;
        if (setBack_state)
            *setBack_state = zw_nodes->nodes[NodeID].status.thermostat_status.setBack_state;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}

ZW_STATUS ZW_ThermostateSetPointSet(WORD NodeID, BYTE setPoint_type, BYTE size, BYTE *value)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0, i = 0;
    BYTE bNodeID = 0, endPoint = 0;
    BYTE precison = 1;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_SETPOINT;
    appTxBuffer[len++] = THERMOSTAT_SETPOINT_SET;
    appTxBuffer[len++] = setPoint_type & 0x0f;
    appTxBuffer[len++] = (precison << 5) + (size & 0x07);
    for (i = 0; i < size; i++)
    {
        appTxBuffer[len++] = value[i];
    }

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}

ZW_STATUS ZW_ThermostateSetPointGet(WORD NodeID, BYTE setPoint_type, double *value)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_THERMOSTAT_SETPOINT << 8) | THERMOSTAT_SETPOINT_REPORT;
    gProperties = setPoint_type;
    
    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_THERMOSTAT_SETPOINT;
    appTxBuffer[len++] = THERMOSTAT_SETPOINT_GET;
    appTxBuffer[len++] = setPoint_type & 0x0f;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (value)
        	*value = zw_nodes->nodes[NodeID].status.thermostat_status.value;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    gProperties = 0x00;
    return retVal;
}


ZW_STATUS ZW_MeterReset(WORD NodeID)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_METER;
    appTxBuffer[len++] = METER_RESET_V2;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}


ZW_STATUS ZW_GetMeterStatus(ZW_DEVICE_INFO *device_info)
{
    assert(device_info != NULL);

    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    BYTE bNodeID = 0, endPoint = 0;
    DWORD timeout = 0;

    bNodeID = device_info->id & 0xff;
    endPoint = (device_info->id >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = device_info->id;
    gNodeCmd = (COMMAND_CLASS_METER << 8) | METER_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_METER;
    appTxBuffer[len++] = METER_GET;
    appTxBuffer[len++] = (((device_info->status.meter_status.scale) & 0x07) << 0x03);

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        memcpy(&(device_info->status.meter_status), &(zw_nodes->nodes[device_info->id].status.meter_status), sizeof(ZW_METER_STATUS));
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}



// Command Class Configuration
ZW_STATUS ZW_ConfigurationGet(WORD NodeID, BYTE parameterNumber, long *configurationValue)
{
    ZW_CONFIGURATION_GET_FRAME  ZW_ConfigurationGetFrame;
    ZW_STATUS retVal;
    BYTE len = 0;
    DWORD timeout = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = NodeID;
    gNodeCmd = (COMMAND_CLASS_CONFIGURATION << 8) | CONFIGURATION_REPORT;

    memset(&ZW_ConfigurationGetFrame, 0, sizeof(ZW_ConfigurationGetFrame));
    ZW_ConfigurationGetFrame.cmdClass = COMMAND_CLASS_CONFIGURATION;
    ZW_ConfigurationGetFrame.cmd = CONFIGURATION_GET;
    ZW_ConfigurationGetFrame.parameterNumber = parameterNumber;
    len = sizeof(ZW_CONFIGURATION_GET_FRAME);
    retVal = ZW_SendData(bNodeID, (BYTE *)&ZW_ConfigurationGetFrame, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (configurationValue)
            *configurationValue = zw_nodes_configuration->configuration[bNodeID].HSM100_configuration.value;
    }

END:
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;

}

// Command Class Configuration
ZW_STATUS ZW_ConfigurationSet(WORD NodeID, BYTE parameterNumber, BYTE bDefault, BYTE size, BYTE *configurationValue)
{
    BYTE appTxBuffer[ZW_BUF_SIZE];
    ZW_STATUS retVal;
    BYTE len = 0, i = 0;
    BYTE bNodeID = 0, endPoint = 0;

    bNodeID = NodeID & 0xff;
    endPoint = (NodeID >> 8) & 0xff;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_CONFIGURATION;
    appTxBuffer[len++] = CONFIGURATION_SET;
    appTxBuffer[len++] = parameterNumber;
    appTxBuffer[len++] = ((bDefault & 0x01 << 7)) | (size & 0x07);
    for (i = 0; i < size; i++)
    {
        appTxBuffer[len++] = configurationValue[i];
    }
    retVal = ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);

    return retVal;
}


ZW_STATUS ZW_ManufacturerSpecificGet(BYTE bNodeID, WORD *pmanufacturerID, WORD *pproductTypeID, WORD *pproductID)
{
    BYTE appTxBuffer[sizeof(ZW_APPLICATION_TX_BUFFER)] = {0};
    ZW_STATUS retVal = ZW_STATUS_OK;
    BYTE len = 0;
    DWORD timeout = 0;

    gGettingNodeStatus = ZW_STATUS_GET_NODE_STATUS;
    gGettingStatusNodeID = bNodeID;
    gNodeCmd = (COMMAND_CLASS_MANUFACTURER_SPECIFIC << 8) | MANUFACTURER_SPECIFIC_REPORT;

    memset(&appTxBuffer, 0, sizeof(appTxBuffer));
    appTxBuffer[len++] = COMMAND_CLASS_MANUFACTURER_SPECIFIC;
    appTxBuffer[len++] = MANUFACTURER_SPECIFIC_GET;

    retVal =  ZW_SendData(bNodeID, appTxBuffer, len, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE), CB_SendDataComplete);
    if (retVal != ZW_STATUS_OK)
    {
        goto END;
    }

    timeout = GET_STATUS_TIMER;
    if ((retVal = WaitStatusReady(&gGettingNodeStatus, ZW_STATUS_GET_NODE_STATUS_RECEIVED, &timeout)) == ZW_STATUS_OK)
    {
        if (pmanufacturerID)
            *pmanufacturerID = zw_nodes->nodes[bNodeID].status.manufacturer_specific_info.manufacturerID;
        if (pproductTypeID)
            *pproductTypeID = zw_nodes->nodes[bNodeID].status.manufacturer_specific_info.productTypeID;
        if (pproductID)
            *pproductID = zw_nodes->nodes[bNodeID].status.manufacturer_specific_info.productID;
    }

END:    
    gGettingNodeStatus = ZW_STATUS_IDLE;
    gGettingStatusNodeID = ILLEGAL_NODE_ID;
    gNodeCmd = 0x00;
    return retVal;
}


void show_device_information(const char *func, const unsigned line, ZW_DEVICE_INFO *deviceinfo)
{
    if(func == 0 ||deviceinfo == 0)
    {
        return;
    }
    
    char buff[1024] = {0};
    int n = 0;

    n = snprintf(buff, sizeof(buff), "[ZWave Driver](%s:%u) ", func, line);
    switch (deviceinfo->type)
    {
        case ZW_DEVICE_BINARYSWITCH:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (binaryswitch), value: %u, type: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.binaryswitch_status.value, deviceinfo->status.binaryswitch_status.type);
            break;
        case ZW_DEVICE_DIMMER:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (dimmer), value: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.dimmer_status.value);
            break;
        case ZW_DEVICE_CURTAIN:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (curtain), value: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.curtain_status.value);
            break;
        case ZW_DEVICE_BINARYSENSOR:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (binarysensor), sub-type: %u, value: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.binarysensor_status.sensor_type, deviceinfo->status.binarysensor_status.value);
            break;
        case ZW_DEVICE_MULTILEVEL_SENSOR:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (multilevelsensor), sub-type: %u, value: %f ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.multilevelsensor_status.sensor_type, deviceinfo->status.multilevelsensor_status.value);
            break;
        case ZW_DEVICE_BATTERY:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (battery), value: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.battery_status.battery_level);
            break;
        case ZW_DEVICE_DOORLOCK:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (doorlock), mode: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.doorlock_status.doorLockMode);
            break;
        case ZW_DEVICE_THERMOSTAT:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (thermostat), mode: %u, value: %f, heat: %f, cool: %f, save heat: %f, save cool: %f  ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.thermostat_status.mode, deviceinfo->status.thermostat_status.value,
                deviceinfo->status.thermostat_status.heat_value, deviceinfo->status.thermostat_status.cool_value, deviceinfo->status.thermostat_status.energe_save_heat_value, deviceinfo->status.thermostat_status.energe_save_cool_value);
            break;
        case ZW_DEVICE_METER:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (meter), sub-type: %u, value: %f, scale: %d ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.meter_status.meter_type, deviceinfo->status.meter_status.value, deviceinfo->status.meter_status.scale);
            break;
        case ZW_DEVICE_KEYFOB:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (keyfob), value: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.keyfob_status.value);
            break;
        case ZW_DEVICE_SIREN:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (siren), value: %u ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type, deviceinfo->status.siren_status.value);
            break;
        default:
            n += snprintf(buff+n, sizeof(buff), "ID:%x, PhyID:%x, Type:%u (unknow type) ", 
                deviceinfo->id, deviceinfo->phy_id, deviceinfo->type);
            break;
    }
    
    DEBUG_INFO("%s \n", buff);
    return;
}

ZW_STATUS SaveNodesInfo(ZW_NODES_INFO *devices)
{
    FILE *fp = NULL;
    ZW_NODES_INFO_EXCEP_STATUS *tmp_devices = NULL;
    DWORD i = 0;

    if (!devices)
    {
        return ZW_STATUS_FAIL;
    }

    tmp_devices = (ZW_NODES_INFO_EXCEP_STATUS *)malloc(sizeof(ZW_NODES_INFO_EXCEP_STATUS));
    if (tmp_devices == NULL)
    {
        return ZW_STATUS_FAIL;
    }
    memset(tmp_devices, 0, sizeof(ZW_NODES_INFO_EXCEP_STATUS));
    tmp_devices->nodes_num = devices->nodes_num;
    memcpy(tmp_devices->nodes_bitmask, devices->nodes_bitmask, MAX_NODEMASK_LENGTH);
    for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i ++)
    {
        tmp_devices->nodes[i].id = devices->nodes[i].id;
        tmp_devices->nodes[i].phy_id = devices->nodes[i].phy_id;
        tmp_devices->nodes[i].dev_num = devices->nodes[i].dev_num;
        tmp_devices->nodes[i].type = devices->nodes[i].type;
        memcpy(tmp_devices->nodes[i].cmdclass_bitmask, devices->nodes[i].cmdclass_bitmask, MAX_CMDCLASS_BITMASK_LEN);
    }

    if ((fp = fopen(ZWAVE_NODES_INFO_FILE, "w+")) == NULL)
    {
        free(tmp_devices);
        return ZW_STATUS_FAIL;
    }

    if (fwrite(tmp_devices, 1, sizeof(ZW_NODES_INFO_EXCEP_STATUS), fp) != sizeof(ZW_NODES_INFO_EXCEP_STATUS))
    {
        free(tmp_devices);
        fclose(fp);
        return ZW_STATUS_FAIL;
    }

    free(tmp_devices);
    fclose(fp);
    return ZW_STATUS_OK;
}

ZW_STATUS GetNodesInfo(ZW_NODES_INFO *devices)
{
    FILE *fp = NULL;
    ZW_NODES_INFO_EXCEP_STATUS *tmp_devices = NULL;
    DWORD i = 0;
    struct stat stat_buf;

    if (!devices)
    {
        return ZW_STATUS_FAIL;
    }

    memset(&stat_buf, 0, sizeof(stat_buf));
    if (stat(ZWAVE_NODES_INFO_FILE, &stat_buf) != 0)
    {
        return ZW_STATUS_FAIL;
    }
    if (stat_buf.st_size != sizeof(ZW_NODES_INFO_EXCEP_STATUS))
    {
        return ZW_STATUS_FAIL;
    }

    if ((fp = fopen(ZWAVE_NODES_INFO_FILE, "r")) == NULL)
    {
        return ZW_STATUS_FAIL;
    }

    tmp_devices = (ZW_NODES_INFO_EXCEP_STATUS *)malloc(sizeof(ZW_NODES_INFO_EXCEP_STATUS));
    if (tmp_devices == NULL)
    {
        fclose(fp);
        return ZW_STATUS_FAIL;
    }
    memset(tmp_devices, 0, sizeof(ZW_NODES_INFO_EXCEP_STATUS));

    if (fread(tmp_devices, 1, sizeof(ZW_NODES_INFO_EXCEP_STATUS), fp) != sizeof(ZW_NODES_INFO_EXCEP_STATUS))
    {
        free(tmp_devices);
        fclose(fp);
        return ZW_STATUS_FAIL;
    }

    devices->nodes_num = tmp_devices->nodes_num;
    memcpy(devices->nodes_bitmask, tmp_devices->nodes_bitmask, MAX_NODEMASK_LENGTH);
    for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i ++)
    {
        devices->nodes[i].id = tmp_devices->nodes[i].id;
        devices->nodes[i].phy_id = tmp_devices->nodes[i].phy_id;
        devices->nodes[i].dev_num = tmp_devices->nodes[i].dev_num;
        devices->nodes[i].type = tmp_devices->nodes[i].type;
        memcpy(devices->nodes[i].cmdclass_bitmask, tmp_devices->nodes[i].cmdclass_bitmask, MAX_CMDCLASS_BITMASK_LEN);
    }

    free(tmp_devices);
    fclose(fp);
    return ZW_STATUS_OK;
}

#if 0
ZW_STATUS GetNodesInfoFromDb(ZW_NODES_INFO *devices)
{
    ZW_DEVICE_INFO *zwdev = NULL;
    int zwdev_num = 0;
    DWORD i = 0;
    int ret = 0;

    if (!devices)
    {
        return ZW_STATUS_FAIL;
    }

    db_get_zw_dev_all_num(&zwdev_num);

    if (zwdev_num == 0)
    {
        return ZW_STATUS_OK;
    }

    zwdev = (ZW_DEVICE_INFO *)calloc(zwdev_num, sizeof(ZW_DEVICE_INFO));
    if (zwdev == NULL)
    {
        return ZW_STATUS_FAIL;
    }

    ret = db_get_zw_dev_all(zwdev, zwdev_num);
    if (ret != DB_RETVAL_OK)
    {
        free(zwdev);
        return ZW_STATUS_FAIL;
    }

    devices->nodes_num = zwdev_num;

    // how to process below data?
    memcpy(devices->nodes_bitmask, tmp_devices->nodes_bitmask, MAX_NODEMASK_LENGTH);
    for (i = 0; i < ZW_MAX_NODES * EXTEND_NODE_CHANNEL; i ++)
    {
        devices->nodes[i].id = tmp_devices->nodes[i].id;
        devices->nodes[i].phy_id = tmp_devices->nodes[i].phy_id;
        devices->nodes[i].dev_num = tmp_devices->nodes[i].dev_num;
        devices->nodes[i].type = tmp_devices->nodes[i].type;
        memcpy(devices->nodes[i].cmdclass_bitmask, tmp_devices->nodes[i].cmdclass_bitmask, MAX_CMDCLASS_BITMASK_LEN);
    }

    free(zwdev);
    return ZW_STATUS_OK;
}
#endif

