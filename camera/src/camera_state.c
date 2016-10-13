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
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "camera_util.h"
#include "public.h"


int camera_name(int camera_id, char *name)
{
    int i = 0;
    DAEMON_CAMERA_INFO_S *camera = NULL;
    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        if (camera->dev_id == camera_id)
        {
            strncpy(camera->dev_name, name, sizeof(camera->dev_name));
            return 0;
        }
    }
    return -1;

}
int camera_del(int camera_id)
{
    CAMERA_CLIP_FILE_S *pclipfile = NULL;
    CAMERA_CLIP_FILE_S *clipfiletmp = NULL;
    int i = 0;
    DAEMON_CAMERA_INFO_S *camera = NULL;

    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        if (camera->dev_id == camera_id)
        {
            pclipfile = camera->clip;
            while (pclipfile != NULL)
            {
                clipfiletmp = pclipfile->next;
                free(pclipfile);
                pclipfile = clipfiletmp;
            }
            pthread_mutex_lock(&g_cam_map_tab_mutex);
            del_media_url(camera->device.streamurl);
            pthread_mutex_unlock(&g_cam_map_tab_mutex);
            memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));

            return 0;
        }
    }

    DEBUG_ERROR("Should not come here, id:[%u] not exist in camerainfo, resync with DB.\n", camera_id);

    // synchronize camerainfo with DB, next time delete camera will be success.
    camera_info_init();

    return -1;
}

int camera_clean_all(void)
{
    CAMERA_CLIP_FILE_S *pclipfile = NULL;
    CAMERA_CLIP_FILE_S *clipfiletmp = NULL;
    int i = 0;
    DAEMON_CAMERA_INFO_S *camera = NULL;
    int ret;

    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        if (0 != camera->dev_id)
        {
            pclipfile = camera->clip;
            while (pclipfile != NULL)
            {
                clipfiletmp = pclipfile->next;
                free(pclipfile);
                pclipfile = clipfiletmp;
            }
            pthread_mutex_lock(&g_cam_map_tab_mutex);
            del_media_url(camera->device.streamurl);
            pthread_mutex_unlock(&g_cam_map_tab_mutex);
            memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
        }
    }

    return 0;
}


#ifdef SUPPORT_UPLOADCLIP_AUTO
int camera_add(DAEMON_CAMERA_INFO_S *camera, char *ip, char *serIP)
{
#else
int camera_add(DAEMON_CAMERA_INFO_S *camera, char *ip)
{
#endif
    char mediaurl[256] = {0};
    memset(mediaurl, 0, sizeof(mediaurl));
    time_t tm = time(NULL);
    int medianum = 0;
    char remoteurl[128] = {0};
    HC_MSG_S msg;
    HC_MSG_S msg_humidity;
    HC_MSG_S msg_temperature;
    int is_add = 0, ret = -1;

#ifdef CAM_INIT_USE_CGI
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (0 != camera_version_check(ip) || 0 != camera_ftpserver_check(ip, serIP))
    {
        if (0 != camera_config_init(ip ,serIP))
#else
    if (0 != camera_version_check(ip))
    {
        if (0 != camera_config_init(ip))
#endif
        {
            DEBUG_ERROR("Initialize camera config failed.\n");
            return -1;
        }
    }
#else
    if (0 != camera_check_config(ip))
    {
        camera_send_config(ip);
        return 0;
    }
#endif

    // get mac address
    get_camera_mac(camera->device.mac, ip);
    if (camera->dev_id == 0)
    {
        is_add = 1;
		pthread_mutex_lock(&g_add_mutex);
        get_camera_id(&camera->dev_id);
        memset(camera->dev_name, 0, sizeof(camera->dev_name));
        snprintf(camera->dev_name, sizeof(camera->dev_name) - 1, "camera%d-%s",
                camera->dev_id & 0xff, camera->device.mac);
		pthread_mutex_unlock(&g_add_mutex);
    }

    // get fw version
    if (0 != camera_get_fwversion(ip, camera->device.fwversion))
    {
        DEBUG_ERROR("Get [%s][%s] fwversion failed.\n", camera->dev_name, ip);
        if(is_add == 1)
        {
            memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
        }
        return -1;
    }

    // get model name
    if (0 != camera_get_modelname(ip, camera->device.modelname))
    {
        DEBUG_ERROR("Get [%s][%s] modelname failed.\n", camera->dev_name, ip);
        if(is_add == 1)
        {
            memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
        }
        return -1;
    }

    send_time_get(ip);

    // add media url, enable video stream
    ret = send_onvif_media_request(mediaurl, ip);
    if (ret != 0 || strlen(mediaurl) < 2)
    {
        DEBUG_ERROR("Get mediaurl failed.");
        if(is_add == 1)
        {
            memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
        }
        return -1;
    }

    pthread_mutex_lock(&g_add_mutex);
    ret = add_media_url(mediaurl, &medianum);
    pthread_mutex_unlock(&g_add_mutex);
    if (0 == ret)
    {
        memset(remoteurl, 0, sizeof(remoteurl));
        sprintf(remoteurl, "../../camera/%d", medianum);
        if (strlen(camera->device.remoteurl) > 1)
        {
            pthread_mutex_lock(&g_cam_map_tab_mutex);
            del_media_url(camera->device.streamurl);
            pthread_mutex_unlock(&g_cam_map_tab_mutex);
            memset(camera->device.ipaddress, 0, sizeof(camera->device.ipaddress));
            memset(camera->device.streamurl, 0, sizeof(camera->device.streamurl));
            memset(camera->device.remoteurl, 0, sizeof(camera->device.remoteurl));
        }
        memcpy(camera->device.remoteurl, remoteurl, strlen(remoteurl));
    }

    camera->state = CAMERA_ONLINE;
    camera->dev_type = HC_DEVICE_TYPE_IPCAMERA;
    camera->network_type = HC_NETWORK_TYPE_CAMERA;
    camera->clipcount = 0;
    camera->pollingtime = tm;
    camera->searchtime = tm;
    camera->recordtick = tm;
    camera->device.connection = (int)CAMERA_ONLINE;
    memcpy(camera->device.ipaddress, ip, strlen(ip));
    memcpy(camera->device.streamurl, mediaurl, strlen(mediaurl));

    //send add success msg to dispatcher
    memset(&msg, 0, sizeof(HC_MSG_S));
    memset(&msg_temperature, 0, sizeof(HC_MSG_S));
    memset(&msg_humidity, 0, sizeof(HC_MSG_S));

    if (is_add)
    {
        msg.head.type = HC_EVENT_RESP_DEVICE_ADDED_SUCCESS;
        msg_temperature.head.type = HC_EVENT_RESP_DEVICE_ADDED_SUCCESS;
        msg_humidity.head.type = HC_EVENT_RESP_DEVICE_ADDED_SUCCESS;
    }
    else
    {
        msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
        msg_temperature.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
        msg_humidity.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
    }
    DEBUG_INFO("Send camera add event\n");

    if(1 == camera_has_temperature(camera) && 0 == camera->temperature.dev_id)
    {
        DEBUG_INFO("andy_camera camera.id= %0x,camera.ip= %s,camera.modelname= %s\n",camera->dev_id,camera->device.ipaddress,camera->device.modelname);
        get_sensor_id(camera, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);
        DEBUG_INFO("camera->temperature.dev_id = %0x\n",camera->temperature.dev_id);
        if (0 != camera_get_temperature(camera->device.ipaddress, &camera->temperature.value))
        {
            DEBUG_ERROR("Get [%s][%s] temperature failed.\n", camera->dev_name, ip);
            if(is_add == 1)
            {
                memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
            }
            return -1;
        }
        camera_sensor_send_msg_to_app(&msg_temperature, camera, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);

    }


    if(1 == camera_has_humidity(camera) && 0 == camera->humidity.dev_id)
    {
        get_sensor_id(camera, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);
        DEBUG_INFO("camera->humidity.dev_id = %0x\n",camera->humidity.dev_id);
        if (0 != camera_get_humidity(camera->device.ipaddress, &camera->humidity.value))
        {
            DEBUG_ERROR("Get [%s][%s] humidity failed.\n", camera->dev_name, ip);
            if(is_add == 1)
            {
                memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
            }
            return -1;
        }
        DEBUG_INFO("camera->humidity.value %f\n", camera->humidity.value);
        camera_sensor_send_msg_to_app(&msg_humidity, camera, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);

    }


    camera_send_msg_to_app(&msg, camera);
    camera_armmode();
    ipcam_camera_list_changed();
    return 0;
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
int camera_update_state(DAEMON_CAMERA_INFO_S *camera, char *ip, char *serIP, CAMERA_STATE_E newstate)
#else
int camera_update_state(DAEMON_CAMERA_INFO_S *camera, char *ip, CAMERA_STATE_E newstate)
#endif
{
    int ret = 0;
    //printf("Camera: %s state change, Form %d To %d\n", ip, camera->state, newstate);
    switch (newstate)
    {
        case CAMERA_ONLINE:
            camera->searchtime = time(NULL);
            switch (camera->state)
            {
                case CAMERA_NULL:
                    if (time(NULL) < enable_add_tick || camera->dev_id != 0)
                    {
#ifdef SUPPORT_UPLOADCLIP_AUTO
                        ret = camera_add(camera, ip, serIP);
#else
                        ret = camera_add(camera, ip);
#endif
                    }
                    break;
                case CAMERA_ONLINE:
                    if (strcmp(camera->device.ipaddress, ip) != 0)
                    {
                        memset(camera->device.ipaddress, 0, sizeof(camera->device.ipaddress));
#ifdef SUPPORT_UPLOADCLIP_AUTO
                        ret = camera_add(camera, ip, serIP);
#else
                        ret = camera_add(camera, ip);
#endif
                    }
#ifdef SUPPORT_UPLOADCLIP_AUTO
                    else if(0 != camera_ftpserver_check(ip, serIP))
                    {
                        camera_config_init(ip ,serIP);
                    }
#endif
                    break;
                case CAMERA_OFFLINE:
                #ifdef CAM_INIT_USE_CGI
                    if (strcmp(camera->device.ipaddress, ip) == 0 && camera_version_check(ip) == 
                    0 && strlen(camera->device.remoteurl) > 0)
                #else
                    if (strcmp(camera->device.ipaddress, ip) == 0 && camera_check_config(ip) == 
                    0 && strlen(camera->device.remoteurl) > 0)
                #endif
                    {
                        camera->state = CAMERA_ONLINE;
                        camera->device.connection = (int)CAMERA_ONLINE;
                        //send state change msg to dispatcher
                        HC_MSG_S msg;
                        memset(&msg, 0, sizeof(HC_MSG_S));
                        msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                        DEBUG_INFO("Send on line event. camera->dev_id = %x\n", camera->dev_id);
                        camera_send_msg_to_app(&msg, camera);

                        if(1 == camera_has_humidity(camera))
                        {
                            HC_MSG_S msg_hum;
                            memset(&msg_hum, 0, sizeof(HC_MSG_S));
                            msg_hum.head.type = HC_EVENT_RESP_DEVICE_CONNECTED;
                            camera_sensor_send_msg_to_app(&msg_hum, camera, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);
                        }
                        if(1 == camera_has_temperature(camera))
                        {
                            HC_MSG_S msg_tem;
                            memset(&msg_tem, 0, sizeof(HC_MSG_S));
                            msg_tem.head.type = HC_EVENT_RESP_DEVICE_CONNECTED;
                            camera_sensor_send_msg_to_app(&msg_tem, camera, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);
                        }

                        ipcam_camera_list_changed();
                    }
                    else
                    {
                        memset(camera->device.ipaddress, 0, sizeof(camera->device.ipaddress));
#ifdef SUPPORT_UPLOADCLIP_AUTO
                        ret = camera_add(camera, ip, serIP);
#else
                        ret = camera_add(camera, ip);
#endif
                    }
                    break;
                default:
                    break;
            }
            break;
        case CAMERA_OFFLINE:
            break;
        case CAMERA_NULL:
            break;
        case CAMERA_DELETE:
            break;

    }
    return ret;
}

int camera_check_network(unsigned int id)
{
    DAEMON_CAMERA_INFO_S *cameraInfo = NULL;
    HC_DEVICE_CAMERA_S *camera;
    
    cameraInfo = get_camera_by_devid(id);
    if(cameraInfo == NULL) {
        DEBUG_ERROR("Unknown device id: %d\n", id);
        return -1;
    }

    camera = (HC_DEVICE_CAMERA_S*) &cameraInfo->device;
    camera_msg_send_check_network(camera->mac);

    return 0;
}
