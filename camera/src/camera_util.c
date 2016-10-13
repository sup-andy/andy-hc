/*******************************************************************************
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
#include <errno.h>

#include "capi_spiderx.h"
#include "camera_util.h"
#include "ipcam_upgrade.h"

#define MEDIA_URL_MAX_LEN   256
#define CAM_MAX_NUM         8
time_t enable_add_tick = 0;
int start_add = 0;

typedef struct _cam_stream_map_tab_
{
    int count;
    int req_count[CAM_MAX_NUM];
    char media_url[CAM_MAX_NUM][MEDIA_URL_MAX_LEN];
} CAM_STREAM_MAP_TAB;

CAM_STREAM_MAP_TAB g_cam_map_tab;
pthread_mutex_t g_cam_map_tab_mutex;

static QUEUE_UNIT_S *q_camera_task_head = NULL;
static QUEUE_UNIT_S *q_camera_task_tail = NULL;
static int q_camera_task_count = 0;
static pthread_mutex_t q_camera_task_mutex;

typedef enum
{
    DB_OPT_TYPE_ADD = 1,
    DB_OPT_TYPE_DELETE,
    DB_OPT_TYPE_UPDATE,
} DB_OPT_TYPE_E;

extern int camera_keep_looping;
char camera_humidity[][64]={"WD8137-W","IP8137-W"};
char camera_temperature[][64]={"WD8137-W","IP8137-W"};

static int g_camera_msg_fd = -1;
static int g_camera_db_msg_fd = -1;

int g_fw_upgrade_flag = 0;

DAEMON_CAMERA_INFO_S *get_camera_unused(void)
{
    int i = 0;

    for (i = 0; i < CAMERACOUNT; i++)
    {
        if (strlen(camerainfo[i].device.onvifuid) == 0)
        {
            return &camerainfo[i];
        }
    }

    if (i >= CAMERACOUNT)
    {
        DEBUG_ERROR("Suuported camera number reach limitation %d.\n", CAMERACOUNT);
        return NULL;
    }

}


DAEMON_CAMERA_INFO_S *get_camera_by_devid(unsigned int dev_id)
{
    int i = 0;
    int camera_minimum = 0;
    unsigned int dev_id_mask;
    dev_id_mask = dev_id&0xff0000ff;
    for (i = 0; i < CAMERACOUNT; i++)
    {
        if ((camerainfo[i].dev_id == dev_id_mask) || (camerainfo[i].humidity.dev_id & 0xff0000ff == dev_id_mask) || (camerainfo[i].temperature.dev_id & 0xff0000ff == dev_id_mask))
        {
            return &camerainfo[i];
        }
     }

    for (i = 0; i < CAMERACOUNT; i++)
    {
        if((camerainfo[i].dev_id == 0 && camerainfo[i].humidity.dev_id == 0 && camerainfo[i].temperature.dev_id == 0))
        {
            return &camerainfo[i];
        }
     }



    if (i >= CAMERACOUNT)
    {
        DEBUG_ERROR("Suuported camera number reach limitation %d.\n", CAMERACOUNT);
        return NULL;
    }

}


DAEMON_CAMERA_INFO_S *get_camera_by_uid(char *onvifuid)
{
    int i = 0;
    for (i = 0; i < CAMERACOUNT; i++)
    {
        //DEBUG_INFO("%d, [%s]", i, camerainfo[i].device.onvifuid);
        if (strcmp(camerainfo[i].device.onvifuid, onvifuid) == 0)
            return &camerainfo[i];
    }
    return NULL;
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
DAEMON_CAMERA_INFO_S *get_camera_by_ip(char *ipaddress)
{
    int i = 0;
    for (i = 0; i < CAMERACOUNT; i++)
    {
        //DEBUG_INFO("%d, [%s]", i, camerainfo[i].device.onvifuid);
        if (strcmp(camerainfo[i].device.ipaddress, ipaddress) == 0)
            return &camerainfo[i];
    }
    return NULL;
}
#endif

int get_camera_mac(char *mac, char *ip)
{
    int ret = -1;
    FILE *fp;
    char path[1024];
    char command[256];
    /* Open the command for reading. */
    memset(command, 0, sizeof(command));
    sprintf(command, "cat /proc/net/arp | grep %s | awk '{print $4}'", ip);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        DEBUG_ERROR("Failed to run command\n");
        return -1;
    }
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {

    }
    /* close */
    pclose(fp);
    if (strlen(path) > 1 && strlen(path) < 32)
    {
        ret = 0;
        memcpy(mac, path, 17);
    }
    else
    {
        memset(mac, 0, 32);
        ret = -1;
    }

    return ret;
}

int get_camera_id(unsigned int *id)
{
    int maxid = HC_NETWORK_TYPE_CAMERA << 24;
    int i = 0;
    for (i = 0; i < CAMERACOUNT; i++)
    {
        if (camerainfo[i].dev_id > 0)
        {
            if (camerainfo[i].dev_id > maxid)
                maxid = camerainfo[i].dev_id;
        }
    }
    *id = maxid + 1;

    return 0;
}
int get_sensor_id(DAEMON_CAMERA_INFO_S *camera, int sensortye)
{
    if(0 == camera->dev_id)
    {
        DEBUG_ERROR("camera->devid is NULL\n");
        return 0;
    }
    switch(sensortye)
    {
        case HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE:
            camera->temperature.dev_id = camera->dev_id | HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE << 16;
            break;
        case HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY:
            camera->humidity.dev_id = camera->dev_id | HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY << 16;
            break;
        default :
            break;
        }
    return 1;
}
int camera_start_thread(pthread_t *ptid, int priority, void * (*func)(void *), void *data)
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


int camera_stop_thread(pthread_t ptid)
{
    int ret;

    pthread_cancel(ptid);

    ret = pthread_join(ptid, NULL);
    if (ret == 0)
    {
        DEBUG_ERROR("Stop the pthread %u ok\n", (unsigned int)ptid);
    }
    else
    {
        DEBUG_ERROR("Stop the pthread %u failed\n", (unsigned int)ptid);
    }

    return ret;
}

int camera_set_nonblocking(int sock)
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
int camera_queue_init(void)
{
    if (pthread_mutex_init(&q_camera_task_mutex, NULL) != 0)
    {
        return -1;
    }
    return 0;
}

void camera_queue_uninit(void)
{
    pthread_mutex_destroy(&q_camera_task_mutex);
}


int camera_queue_push(QUEUE_UNIT_S *unit)
{
    pthread_mutex_lock(&q_camera_task_mutex);

    if (camera_queue_empty())
    {
        q_camera_task_head = unit;
        q_camera_task_tail = unit;
    }
    else
    {
        q_camera_task_tail->next = unit;
        q_camera_task_tail = unit;
    }
    q_camera_task_count++;

    pthread_mutex_unlock(&q_camera_task_mutex);

    return 0;
}


QUEUE_UNIT_S * camera_queue_popup(void)
{
    QUEUE_UNIT_S *p = NULL;

    pthread_mutex_lock(&q_camera_task_mutex);
    if (camera_queue_empty())
    {
        pthread_mutex_unlock(&q_camera_task_mutex);
        return p;
    }
    p = q_camera_task_head;
    q_camera_task_head = p->next;

    q_camera_task_count--;
    pthread_mutex_unlock(&q_camera_task_mutex);

    return p;
}


int camera_queue_empty(void)
{
    return (q_camera_task_head == NULL) ? 1 : 0;
}


int camera_queue_number(void)
{
    return q_camera_task_count;
}

void camera_queue_clean(void)
{
    QUEUE_UNIT_S *p = NULL;

    pthread_mutex_lock(&q_camera_task_mutex);
    while (!camera_queue_empty())
    {
        p = q_camera_task_head;
        q_camera_task_head = p->next;
        free(p);
    }
    q_camera_task_tail = NULL;
    q_camera_task_count = 0;

    pthread_mutex_unlock(&q_camera_task_mutex);
}

int camera_update_db(HC_DEVICE_INFO_S *hcdev)
{
    int ret = 0;
    char connect_status[32] = {0};

    if (hcdev == NULL)
    {
        return HC_DB_ACT_FAIL;
    }

    if (HC_EVENT_RESP_DEVICE_ADDED_SUCCESS == hcdev->event_type)
    {
        ret = hcapi_get_dev_by_dev_id(hcdev);
        if (ret == HC_RET_SUCCESS)
        {
            return HC_DB_ACT_OK;
        }
        else
        {
            ret = hcapi_add_dev(hcdev);
            if (ret != HC_RET_SUCCESS)
            {
                return HC_DB_ACT_FAIL;
            }
            else
            {
                return HC_DB_ACT_OK;
            }
        }
    }
    else if (HC_EVENT_RESP_DEVICE_DELETED_SUCCESS == hcdev->event_type)
    {
        ret = hcapi_del_dev(hcdev);
        if (ret != HC_RET_SUCCESS)
        {
            return HC_DB_ACT_FAIL;
        }
        else
        {
            return HC_DB_ACT_OK;
        }
    }
    else
    {
        ret = hcapi_set_dev(hcdev);
        if (ret != HC_RET_SUCCESS)
        {
            return HC_DB_ACT_FAIL;
        }
        else
        {
            return HC_DB_ACT_OK;
        }

    }

}

int camera_send_msg_to_app(HC_MSG_S *msg, DAEMON_CAMERA_INFO_S *camera)
{
    HC_DEVICE_INFO_S *hcdev = NULL;
    int ret = 0;
    //printf("send msg to APPLICATION\n");
    if (msg == NULL)
        return -1;

    msg->head.src = DAEMON_CAMERA;
    msg->head.dst = APPLICATION;

    hcdev = &msg->body.device_info;
    hcdev->event_type = msg->head.type;
    hcdev->network_type = HC_NETWORK_TYPE_CAMERA;
    hcdev->dev_type = HC_DEVICE_TYPE_IPCAMERA;

    if (camera)
    {
        msg->head.dev_id = camera->dev_id;

        ret = convert_camera_dev_to_hcdev(camera, hcdev);
        if (ret != 0)
        {
            DEBUG_ERROR("Convert camera device to MSG failed.\n");
            return -1;
        }

        if (msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS)
            memcpy(hcdev->dev_name, camera->dev_name, strlen(camera->dev_name));
    }
    if ((msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS
            || msg->head.type == HC_EVENT_RESP_DEVICE_DELETED_SUCCESS
            || msg->head.type == HC_EVENT_STATUS_DEVICE_STATUS_CHANGED
            || msg->head.type == HC_EVENT_RESP_DEVICE_SET_SUCCESS) && (msg->head.dev_id != 0xff)) 
    {
        if (g_fw_upgrade_flag == 1)
        {
            DEBUG_INFO("FW upgrading, ignore msg %s[%d] appid[%d] devid[%x]", 
                    hc_map_msg_txt(msg->head.type), msg->head.type, msg->head.appid, msg->head.dev_id);   
            return 0;
        }

        ret = camera_update_db(hcdev);
        if (HC_DB_ACT_FAIL == ret)
        {
            DEBUG_ERROR("Update DB failed, id:%08X, name:%s, ret:%d.\n",
                        hcdev->dev_id, hcdev->dev_name, ret);

            if (msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS)
            {
                memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
            }

            msg->head.type = HC_EVENT_RESP_DB_OPERATION_FAILURE;
        }
    }

    if (ret == 0)
    {
        ret = camera_msg_send(msg);
        if (ret <= 0)
        {
            DEBUG_ERROR("Call msg_send failed, ret is %d.\n", ret);
            return -1;
        }
    }
    //printf("send OK\n");

    return 0;
}

int camera_sensor_send_msg_to_app(HC_MSG_S *msg, DAEMON_CAMERA_INFO_S *camera, int sensortye)
{
    HC_DEVICE_INFO_S *hcdev = NULL;
    int ret = 0;
    if (msg == NULL)
        return -1;

    msg->head.src = DAEMON_CAMERA;
    msg->head.dst = APPLICATION;

    if (camera)
    {
        switch(sensortye)
        {
            case HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE:
                msg->head.dev_id = camera->temperature.dev_id;
                break;
            case HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY:
                msg->head.dev_id = camera->humidity.dev_id;
                break;
            default:
                break;

        }

       DEBUG_INFO("msg %0x\n",msg->head.dev_id);
       hcdev = &msg->body.device_info;
       hcdev->event_type = msg->head.type;
       hcdev->network_type = HC_NETWORK_TYPE_CAMERA;
       hcdev->dev_type = HC_DEVICE_TYPE_MULTILEVEL_SENSOR;
       ret = convert_camera_sensor_to_hcdev(camera, hcdev, sensortye);
       if (ret != 0)
       {
           DEBUG_ERROR("Convert camera sensor to MSG failed.\n");
            return -1;
       }

        if (msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS)
            //snprintf(hcdev->dev_name,sizeof(hcdev->dev_name),"%s-sensor",camera->dev_name);
            memcpy(hcdev->dev_name, camera->dev_name, strlen(camera->dev_name));
    }
    if ((msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS
            || msg->head.type == HC_EVENT_RESP_DEVICE_DELETED_SUCCESS
            || msg->head.type == HC_EVENT_STATUS_DEVICE_STATUS_CHANGED
            || msg->head.type == HC_EVENT_RESP_DEVICE_SET_SUCCESS) && (msg->head.dev_id != 0xff))
    {
        ret = camera_update_db(hcdev);
        if (HC_DB_ACT_FAIL == ret)
        {
            DEBUG_ERROR("Update DB failed, id:%08X, name:%s, ret:%d.\n",
                        hcdev->dev_id, hcdev->dev_name, ret);

            if (msg->head.type == HC_EVENT_RESP_DEVICE_ADDED_SUCCESS)
            {
                memset(camera, 0, sizeof(DAEMON_CAMERA_INFO_S));
            }

            msg->head.type = HC_EVENT_RESP_DB_OPERATION_FAILURE;
        }
    }
    DEBUG_INFO("camera->dev_id %0x,ret = %d\n", msg->head.dev_id,ret);

    if (ret == 0)
    {
        ret = camera_msg_send(msg);
        if (ret <= 0)
        {
            DEBUG_ERROR("Call msg_send failed, ret is %d.\n", ret);
            return -1;
        }
    }
    DEBUG_INFO("send OK\n");

    return 0;
}

int trigger_post(int dev_id, char *clipname)
{
    DAEMON_CAMERA_INFO_S devinfo;
    int i = 0;
    DAEMON_CAMERA_INFO_S *tmp = NULL;
    char newname[128] = {0};

    if (dev_id == 0xff)
    {
        for (i = 0; i < CAMERACOUNT; i++)
        {
            tmp = &camerainfo[i];
            if (tmp->dev_id != 0 && tmp->state == CAMERA_ONLINE)
            {
                memset(newname, 0, sizeof(newname));
                //sprintf(newname, "%d_%s", tmp->dev_id, clipname);
#ifdef SUPPORT_UPLOADCLIP_AUTO
                sprintf(newname, "%s-trig-%d_%s", tmp->device.ipaddress, tmp->dev_id, clipname);
#else
                sprintf(newname, "%d_%s", tmp->dev_id, clipname);
#endif
                send_record_trigger_post(tmp, newname);
            }
        }
    }
    else
    {
        for (i = 0; i < CAMERACOUNT; i++)
        {
            tmp = &camerainfo[i];
            if (tmp->dev_id != 0 && tmp->dev_id == dev_id && tmp->state == CAMERA_ONLINE)
            {
                memset(newname, 0, sizeof(newname));
#ifdef SUPPORT_UPLOADCLIP_AUTO
                sprintf(newname, "%s-trig-%d_%s", tmp->device.ipaddress, tmp->dev_id, clipname);
 #else
                sprintf(newname, "%d_%s", tmp->dev_id, clipname);
 #endif
                send_record_trigger_post(tmp, newname);
                break;
            }
        }
    }
    return 0;
}

static int camera_process_msg(HC_MSG_S *pMsg)
{
    int ret = 0;
    DAEMON_CAMERA_INFO_S devinfo;
    int i = 0;
    DAEMON_CAMERA_INFO_S *tmp = NULL;
#ifdef SUPPORT_UPLOADCLIP_AUTO
    char destname[64] = {0};
#endif

    if (pMsg == NULL)
    {
        return -1;
    }

    // when camera lock success, do not accept any msg except below.
    if (g_fw_upgrade_flag == 1)
    {
        if (pMsg->head.type != HC_EVENT_EXT_FW_UPGRADE_LOCK
                && pMsg->head.type != HC_EVENT_EXT_FW_UPGRADE_UNLOCK)
        {
            DEBUG_INFO("Ignore msg %s(%d), appid %d.\n",
                      hc_map_msg_txt(pMsg->head.type), pMsg->head.type, pMsg->head.appid);
            return 0;
        }
    }

    // parse msg.
    DEBUG_INFO("Process msg %s(%d).\n", hc_map_msg_txt(pMsg->head.type), pMsg->head.type);

    memset(&devinfo, 0, sizeof(devinfo));
    ret = convert_msg_to_camera_dev(pMsg, &devinfo);
    if (ret != 0)
    {
        DEBUG_ERROR("Convert HC device %s(%d)to camera device info failed.\n",
                    hc_map_hcdev_txt(pMsg->head.dev_type), pMsg->head.dev_type);
        return -1;
    }
    ret = -1;
    // add task.
    //ret = camera_task_add(pMsg->head.type, (void *)&devinfo);
    if (pMsg->head.type == HC_EVENT_REQ_DEVICE_DELETE)
    {
        DEBUG_INFO("camera delete devinfo.dev_id = %d,devinfo.humidity.dev_id = %d, devinfo.temperature.dev_id.\n", devinfo.dev_id, devinfo.humidity.dev_id, devinfo.temperature.dev_id);
        pthread_mutex_lock(&g_task_mutex);
        ret = camera_del(devinfo.dev_id);
        pthread_mutex_unlock(&g_task_mutex);
        if (ret == 0)
        {

            if(devinfo.humidity.dev_id != 0)
            {
                camera_sensor_msg_reply(0, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &devinfo, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);
            }
            if(devinfo.temperature.dev_id != 0)
            {
                camera_sensor_msg_reply(0, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &devinfo, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);
            }
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETED_SUCCESS, &devinfo);
            ipcam_camera_list_changed();
        }
        else
        {
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DELETED_FAILURE, &devinfo);
        }

    }
    else if (pMsg->head.type == HC_EVENT_REQ_DEVICE_DEFAULT)
    {
        DEBUG_INFO("camera default devinfo.dev_id = %d,devinfo.humidity.dev_id = %d, devinfo.temperature.dev_id.\n", devinfo.dev_id, devinfo.humidity.dev_id, devinfo.temperature.dev_id);
        pthread_mutex_lock(&g_task_mutex);
        ret = camera_clean_all();
        pthread_mutex_unlock(&g_task_mutex);
        if (ret == 0)
        {
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DEFAULT_SUCCESS, NULL);
        }
        else
        {
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_DEFAULT_FAILURE, NULL);
        }

    }
    else if (pMsg->head.type == HC_EVENT_REQ_DEVICE_SET)
    {
        DEBUG_INFO("HC_EVENT_REQ_DEVICE_SET, dev_name = %s, dev_id = %d\n", devinfo.dev_name, devinfo.dev_id);
        if (strlen(devinfo.dev_name) > 0)
        {
            pthread_mutex_lock(&g_task_mutex);
            ret = camera_name(devinfo.dev_id, devinfo.device.name);
            pthread_mutex_unlock(&g_task_mutex);
            if (ret == 0)
            {
                for (i = 0; i < CAMERACOUNT; i++)
                {
                    if (camerainfo[i].dev_id == devinfo.dev_id)
                    {
                        strcpy(camerainfo[i].dev_name, devinfo.device.name);
                    }
                }
                camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_SET_SUCCESS, &devinfo);
            }
            else
            {
                camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_SET_FAILURE, &devinfo);
            }
        }
        else if (strcmp(devinfo.device.label, "trigger") == 0)
        {
			DEBUG_INFO("device.label = %s\n", devinfo.device.label);
            pthread_mutex_lock(&g_task_mutex);
            trigger_post(devinfo.dev_id, devinfo.device.destPath);
            pthread_mutex_unlock(&g_task_mutex);
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_SET_SUCCESS, &devinfo);
        }
        else if (strcmp(devinfo.device.label, "addon") == 0)
        {
            int timeval = atoi(devinfo.device.ext);
            DEBUG_INFO("timeval = %d\n", timeval);
            if (timeval == 0)
                enable_add_tick = 0;
            else
            {
                enable_add_tick = time(NULL) + timeval;
                start_add = 1;
            }
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_SET_SUCCESS, &devinfo);
        }
        else if (strcmp(devinfo.device.label, "armmode") == 0)
        {
            camera_armmode();
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_SET_SUCCESS, &devinfo);
        }
    }
    else if (pMsg->head.type == HC_EVENT_REQ_DEVICE_GET)
    {
        for (i = 0; i < CAMERACOUNT; i++)
        {
            tmp = &camerainfo[i];
            if (tmp->dev_id != 0 && tmp->dev_id == devinfo.dev_id && tmp->state == CAMERA_ONLINE)
            {
                camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_GET_SUCCESS, &devinfo);
                return 0;
            }
        }
        if (i == CAMERACOUNT)
            camera_msg_reply(pMsg->head.appid, HC_EVENT_RESP_DEVICE_GET_FAILURE, &devinfo);
    }
    else if (pMsg->head.type == HC_EVENT_EXT_IP_CHANGED)
    {
        DEBUG_INFO("HC_EVENT_EXT_IP_CHANGED\n");
        ipchange = 1;
    }
    else if (pMsg->head.type == HC_EVENT_WLAN_CONFIGURE_CHANGED)
    {
        DEBUG_INFO("HC_EVENT_WLAN_CONFIGURE_CHANGED\n");
        pthread_mutex_lock(&g_task_mutex);
#ifdef CAM_INIT_USE_CGI
        camera_version_update(1);
#else
        camera_config_update(1);
#endif
        for (i = 0; i < CAMERACOUNT; i++)
        {
            tmp = &camerainfo[i];
            if (tmp->state == CAMERA_ONLINE && tmp->dev_id != 0)
            {
                tmp->state = CAMERA_OFFLINE;
                tmp->device.connection = (int)CAMERA_OFFLINE;
                memset(tmp->device.destPath, 0, sizeof(tmp->device.destPath));
                //send state change event to APP
                HC_MSG_S msg;
                memset(&msg, 0, sizeof(HC_MSG_S));
                msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                tmp->event_type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                DEBUG_INFO("Send off line event, camera->dev_id = %d\n", tmp->dev_id);
                camera_send_msg_to_app(&msg, tmp);
                ipcam_camera_list_changed();
            }
        }
        pthread_mutex_unlock(&g_task_mutex);
    }
    else if (pMsg->head.type == HC_EVENT_FACTORY_DEFAULT)
    {
        DEBUG_INFO("HC_EVENT_FACTORY_DEFAULT\n");

        pthread_mutex_lock(&g_task_mutex);
        for (i = 0; i < CAMERACOUNT; i++)
        {
            tmp = &camerainfo[i];
            if (tmp->state == CAMERA_ONLINE && tmp->dev_id != 0)
            {
                send_sdcard_format_get(tmp);
            }
        }
        pthread_mutex_unlock(&g_task_mutex);

        // reply app
        HC_MSG_S msg;
        memset(&msg, 0, sizeof(HC_MSG_S));
        msg.head.type = HC_EVENT_FACTORY_DEFAULT_DONE;
        msg.head.appid = pMsg->head.appid;
        camera_send_msg_to_app(&msg, NULL);
    }
    else if (HC_EVENT_EXT_FW_VERSION_CHANGED == pMsg->head.type)
    {
        DEBUG_INFO("HC_EVENT_EXT_CAMERA_FW_UPGRADE\n");
        ipcam_handle_firmware_version_change_event();
    }
    else if (HC_EVENT_EXT_CAMERA_FW_UPGRADE == pMsg->head.type)
    {
        DEBUG_INFO("HC_EVENT_EXT_CAMERA_FW_UPGRADE\n");

        ipcam_handle_firmware_upgrade_event(&pMsg->body.device_info.device.ext_camera_upgrade);
    }
    else if (pMsg->head.type == HC_EVENT_EXT_FW_UPGRADE_LOCK)
    {
        if (enable_add_tick)
        {
            camera_msg_reply(pMsg->head.appid, HC_EVENT_EXT_FW_UPGRADE_LOCK_FAILED, NULL);
            return 0;
        }
        else
        {
            DEBUG_INFO("FW upgrade CAMERA lock.\n");
            g_fw_upgrade_flag = 1;
            camera_msg_reply(pMsg->head.appid, HC_EVENT_EXT_FW_UPGRADE_LOCK_DONE, NULL);
            return 0;
        }

    }
    else if (pMsg->head.type == HC_EVENT_EXT_FW_UPGRADE_UNLOCK)
    {
        DEBUG_INFO("FW upgrade CAMERA unlock.\n");
        g_fw_upgrade_flag = 0;
        return 0;
    }

    return 0;
}

int camera_msg_init(void)
{
    // invoke library MSG provided APIs to create socket.
    g_camera_msg_fd = hc_client_msg_init(DAEMON_CAMERA, SOCKET_DEFAULT);
    if (g_camera_msg_fd == -1)
    {
        DEBUG_ERROR("Call camera_msg_init failed line:%d\n", __LINE__);
        return -1;
    }

    return 0;
}

void camera_msg_uninit(void)
{
    if (g_camera_msg_fd !=  -1)
    {
        hc_client_unint(g_camera_msg_fd);
    }
}

int camera_msg_send(HC_MSG_S *pMsg)
{
    return hc_client_send_msg_to_dispacther(g_camera_msg_fd, pMsg);
}


int camera_msg_handle(void)
{
    int ret = 0;
    struct timeval tv;
    //int i = 0;
    HC_MSG_S *hcmsg = NULL;

    while (camera_keep_looping)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        hcmsg = NULL;
        ret = hc_client_wait_for_msg(g_camera_msg_fd, &tv, &hcmsg);

        if (ret == 0)
        {
            hc_msg_free(hcmsg);
            ipcam_handle_timeout_event();
            continue;
        }
        else if (ret == -1)
        {
            hc_msg_free(hcmsg);
            DEBUG_ERROR("Call hc_client_wait_for_msg failed, errno is '%d'.\n", errno);
            camera_msg_uninit();
            camera_msg_init();
            DEBUG_ERROR("new g_camera_msg_fd = '%d'.\n", g_camera_msg_fd);
            continue;
        }

        camera_process_msg(hcmsg);
        hc_msg_free(hcmsg);
    }

    return 0;
}
void camera_msg_reply(int app_id, HC_EVENT_TYPE_E type, DAEMON_CAMERA_INFO_S *devinfo)
{
    int ret = 0;
    HC_MSG_S msg;

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = type;
    msg.head.appid = app_id;  //0, broadcast

    ret = camera_send_msg_to_app(&msg, devinfo);
    if (ret != 0)
    {
        DEBUG_ERROR("Camera send msg to app failed.\n");
    }

}

void camera_sensor_msg_reply(int app_id, HC_EVENT_TYPE_E type, DAEMON_CAMERA_INFO_S *devinfo, int sensortype)
{
    int ret = 0;
    HC_MSG_S msg;

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.type = type;
    msg.head.appid = app_id;  //0, broadcast

    ret = camera_sensor_send_msg_to_app(&msg, devinfo, sensortype);
    if (ret != 0)
    {
        DEBUG_ERROR("Camera send msg to app failed.\n");
    }

}

int camera_msg_send_check_network(unsigned char *mac)
{
    HC_MSG_S msg;
    HC_DEVICE_INFO_S *deviceInfo;
    HC_DEVICE_EXT_NETWORK_DETECTION_S *networkDetect;
    int ret = 0;

    memset(&msg, 0, sizeof(HC_MSG_S));

    msg.head.src = DAEMON_CAMERA;
    msg.head.dst = APPLICATION;
    msg.head.type = HC_EVENT_EXT_CHECK_NEIGHBOUR;
    msg.head.dev_type = HC_DEVICE_TYPE_EXT_NEIGHBOUR;

    deviceInfo = (HC_DEVICE_INFO_S*) &msg.body; 
    deviceInfo->event_type = HC_EVENT_EXT_CHECK_NEIGHBOUR;
    deviceInfo->network_type = HC_NETWORK_TYPE_CAMERA;
    deviceInfo->dev_type = HC_DEVICE_TYPE_EXT_NEIGHBOUR;

    networkDetect = (HC_DEVICE_EXT_NETWORK_DETECTION_S*) &deviceInfo->device;
    memset(networkDetect, 0, sizeof(networkDetect));
    memcpy(networkDetect->mac, mac, sizeof(networkDetect->mac));

    DEBUG_INFO("Send check mac[%s]\n", mac);
    ret = camera_msg_send(&msg);
    if (ret <= 0) {
        DEBUG_ERROR("Call msg_send failed, ret is %d.\n", ret);
        return -1;
    }

    return 0;
}

int convert_camera_dev_to_msg(DAEMON_CAMERA_INFO_S *devinfo, HC_MSG_S *msg)
{
    if (devinfo == NULL || msg == NULL)
    {
        return -1;
    }
    msg->head.dev_type = HC_DEVICE_TYPE_IPCAMERA;
    //memcpy(&(msg->body.device_info.device.camera), &(devinfo->device), sizeof(HC_DEVICE_CAMERA_S));

    return 0;
}

int convert_msg_to_camera_dev(HC_MSG_S *msg, DAEMON_CAMERA_INFO_S *devinfo)
{
    int i;
    DAEMON_CAMERA_INFO_S *camera = NULL;
    if (devinfo == NULL || msg == NULL)
    {
        return -1;
    }
    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        if (msg->head.dev_id != 0 && (msg->head.dev_id == camera->dev_id || msg->head.dev_id == camera->humidity.dev_id || msg->head.dev_id == 
        camera->temperature.dev_id))
        {
            devinfo->dev_id = camera->dev_id;
            devinfo->humidity.dev_id = camera->humidity.dev_id;
            devinfo->temperature.dev_id = camera->temperature.dev_id;
            DEBUG_INFO("camera->dev_id=%0x,camera->humidity.dev_id=%0x,camera->temperature.dev_id=%0x\n",devinfo->dev_id, devinfo->humidity.dev_id, devinfo->temperature.dev_id);
            break;
        }
        else
        {
            DEBUG_INFO("can not find device where dev_id = %0x", msg->head.dev_id);
            devinfo->dev_id =  msg->head.dev_id;
        }
    }

    if (msg->head.dev_type != HC_DEVICE_TYPE_IPCAMERA && msg->head.dev_type != HC_DEVICE_TYPE_MULTILEVEL_SENSOR && msg->head.dev_type != 0)
    {
        DEBUG_ERROR("device type error.msg->head.dev_type = %d\n", msg->head.dev_type);
        return -1;
    }

    if (msg->head.type == HC_EVENT_REQ_DEVICE_SET)
    {
        devinfo->dev_type = msg->head.dev_type;
        memcpy(&(devinfo->device), &(msg->body.device_info.device.camera), sizeof(HC_DEVICE_CAMERA_S));
        memcpy(devinfo->dev_name, devinfo->device.name, 32);
    }
    return 0;
}

int convert_camera_dev_to_hcdev(DAEMON_CAMERA_INFO_S *dev, HC_DEVICE_INFO_S *hcdev)
{
    if (dev == NULL || hcdev == NULL)
    {
        return -1;
    }

    hcdev->dev_id = dev->dev_id;
    hcdev->network_type = HC_NETWORK_TYPE_CAMERA;
    hcdev->dev_type = HC_DEVICE_TYPE_IPCAMERA;

    memcpy(&(hcdev->device.camera), &(dev->device), sizeof(HC_DEVICE_CAMERA_S));
    return 0;
}
int convert_camera_sensor_to_hcdev(DAEMON_CAMERA_INFO_S *dev, HC_DEVICE_INFO_S *hcdev, int sensortype)
{
    if (dev == NULL || hcdev == NULL)
    {
        return -1;
    }

    hcdev->network_type = HC_NETWORK_TYPE_CAMERA;
    hcdev->dev_type = HC_DEVICE_TYPE_MULTILEVEL_SENSOR;
    switch(sensortype)
    {
        case HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY:
            hcdev->dev_id = dev->humidity.dev_id;
            hcdev->device.multilevel_sensor.sensor_type = HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY;
            hcdev->device.multilevel_sensor.scale = HC_SCALE_HUMIDITY_ABSOLUTE;
            hcdev->device.multilevel_sensor.value = dev->humidity.value;
            break;
        case HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE:
            hcdev->dev_id = dev->temperature.dev_id;
            hcdev->device.multilevel_sensor.sensor_type = HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE;
            hcdev->device.multilevel_sensor.scale = HC_SCALE_AIR_TEMPERATURE_CELSIUS;
            hcdev->device.multilevel_sensor.value = dev->temperature.value;
            break;
        default:
            break;
    }

    return 0;
}

int camera_map_tab_init(void)
{
    memset(&g_cam_map_tab, 0, sizeof(CAM_STREAM_MAP_TAB));
    if (pthread_mutex_init(&g_cam_map_tab_mutex, NULL) != 0)
    {
        return -1;
    }
    return 0;
}
int camera_map_tab_uninit(void)
{
    pthread_mutex_destroy(&g_cam_map_tab_mutex);

}

int add_media_url(char *media_url, int *id)
{
    int loop = 0;

    if (NULL == media_url || NULL == id)
    {
        DEBUG_ERROR("Null point input.\n");
        return -1;
    }
    if (g_cam_map_tab.count >= CAM_MAX_NUM)
    {
        DEBUG_ERROR("Map table full.\n");
        return -1;
    }

    for (loop = 0; loop < CAM_MAX_NUM; loop++)
    {
        if (0 == strlen(g_cam_map_tab.media_url[loop]))
        {
            strncpy(g_cam_map_tab.media_url[loop], media_url, MEDIA_URL_MAX_LEN - 1);
            *id = loop + 1;
            g_cam_map_tab.count++;

#ifndef CAM_USE_FFSERVER
            {
                char sys_cmd[256] = {0};
                snprintf(sys_cmd, sizeof(sys_cmd) - 1, "/tmp/stream/stream_cli FeedUpdate %d 1 %s",
                         *id, g_cam_map_tab.media_url[loop]);
                system(sys_cmd);
            }
#endif
            DEBUG_INFO("Successfully add cam url [%d] to [%s]\n",
                       *id, g_cam_map_tab.media_url[loop]);
            return 0;
        }
    }

    return -1;
}

int del_media_url(char *media_url)
{
    int loop = 0;

    if (NULL == media_url)
    {
        DEBUG_ERROR("Null point input.\n");
        return -1;
    }

    if (g_cam_map_tab.count <= 0)
    {
        DEBUG_ERROR("Map table empty.\n");
        return -1;
    }

    for (loop = 0; loop < CAM_MAX_NUM; loop++)
    {
        if (0 == strcmp(g_cam_map_tab.media_url[loop], media_url))
        {
            memset(g_cam_map_tab.media_url[loop], 0x0, MEDIA_URL_MAX_LEN);
            g_cam_map_tab.count--;

#ifndef CAM_USE_FFSERVER
            {
                char sys_cmd[256] = {0};
                snprintf(sys_cmd, sizeof(sys_cmd) - 1, "/tmp/stream/stream_cli FeedUpdate %d 0",
                         loop + 1);
                system(sys_cmd);
            }
#endif

            DEBUG_INFO("Successfully del cam url [%s] from [%d]\n",
                       media_url, loop + 1);
            return 0;
        }
    }

    DEBUG_ERROR("Unexpected error.\n");
    return -1;
}
int camera_has_humidity(DAEMON_CAMERA_INFO_S *camera)
{
    int i;
    for(i = 0; i < sizeof(camera_humidity)/sizeof(camera_humidity[0]); i++)
        if(0 == strcmp(camera->device.modelname, camera_humidity[i]))
            return 1;
    return 0;
}
int camera_has_temperature(DAEMON_CAMERA_INFO_S *camera)
{
    int i;
    for(i = 0; i < sizeof(camera_temperature)/sizeof(camera_temperature[0]); i++)
        if(0 == strcmp(camera->device.modelname, camera_temperature[i]))
            return 1;
    return 0;
}

#ifdef CAM_USE_FFSERVER
void ffmpeg_process_task(int id)
{
#define FFMPEG_START_CMD_FMT  "/tmp/ffmpeg/ffmpeg_%d -rtsp_transport tcp -i %s -vcodec copy -reset_timestamps 0 " \
                              "http://127.0.0.1:8090/live%d.ffm"

    FILE *cmdfp = NULL;
    char run_cmd[256] = {0};
    char buffer[256] = {0};
    char *ptr;
    int req_id = 0;
    int close_id = 0;

    snprintf(run_cmd, sizeof(run_cmd), FFMPEG_START_CMD_FMT,
             id, g_cam_map_tab.media_url[id - 1], id);

    cmdfp = popen(run_cmd, "r");
    if (NULL == cmdfp)
    {
        DEBUG_ERROR("popen error with errno [%d].\n", errno);
        return;
    }

    while (fgets(buffer, sizeof(buffer) - 1, cmdfp) != NULL)
    {
        buffer[strlen(buffer) - 1] = '\0';
        //fprintf(stdout, "::%s\n", buffer);

        ptr = strstr(buffer, "frame=");
        if (NULL != ptr)
        {
            if (strstr(buffer, "bitrate= 0.0kbits/s"))
            {
                DEBUG_ERROR("ffmpeg have no tx.\n");
                break;
            }
        }

        if (strstr(buffer, "error") || strstr(buffer, "st:1 invalid dropping"))
        {
            DEBUG_ERROR("ffmpeg have no tx.\n");
            break;
        }
    }

    pclose(cmdfp);
    return;
}

void kill_ffmpeg_process(int id)
{
    char sys_cmd[256] = {0};

    snprintf(sys_cmd, sizeof(sys_cmd), "killall -9 ffmpeg_%d", id);
    system(sys_cmd);
}

int fork_ffmpeg_process(int id)
{
    pid_t pid = 0;
    int ret = 0;

    pid = fork();
    DEBUG_INFO("fork_ffmpeg_process id [%d] pid [%d]\n", id, pid);

    switch (pid)
    {
        case -1:
            perror("fork");
            break;

        case  0:

            ffmpeg_process_task(id);

            DEBUG_INFO("ffmpeg_process_task [%d] exit\n", id);

            exit(0);
            break;

        default:
            // Parent
            break;
    }

    return pid;
}

void ffmpeg_init()
{
    char sys_cmd[256];
    int loop = 0;

    for (loop = 0; loop < CAM_MAX_NUM; loop++)
    {
        memset(sys_cmd, 0x0, sizeof(sys_cmd));
        snprintf(sys_cmd, sizeof(sys_cmd), "ln -s /tmp/ffmpeg/ffmpeg /tmp/ffmpeg/ffmpeg_%d > /dev/null", loop + 1);
        system(sys_cmd);
    }
}

void *ffserver_monitor_task(void *arg)
{
#define FFSERVER_START_CMD  "/tmp/ffserver/ffserver -f /tmp/ffserver/ffserver.conf &"

    FILE *cmdfp = NULL;
    char buffer[256] = {0};
    char *ptr;
    int req_id = 0;
    int close_id = 0;

    char sys_cmd[256];
    int loop = 0;

    {
        memset(sys_cmd, 0x0, sizeof(sys_cmd));
        snprintf(sys_cmd, sizeof(sys_cmd), "killall -9 ffserver > /dev/null");
        system(sys_cmd);
    }


    memset(&g_cam_map_tab, 0, sizeof(CAM_STREAM_MAP_TAB));
    ffmpeg_init();

    cmdfp = popen(FFSERVER_START_CMD, "r");
    if (NULL == cmdfp)
    {
        DEBUG_ERROR("popen error with errno [%d].\n", errno);
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer) - 1, cmdfp) != NULL)
    {
        buffer[strlen(buffer) - 1] = '\0';
        DEBUG_INFO("ffserver::%s\n", buffer);

        if (strstr(buffer, "cmd:GET") && !strstr(buffer, "127.0.0.1"))
        {
            ptr = strstr(buffer, "url:/live");
            if (NULL != ptr)
            {
                sscanf(ptr, "url:/live%d.mp4", &req_id);

                //fprintf(stdout, "::Request %d\n", req_id);
                if (req_id > CAM_MAX_NUM || 0 == strlen(g_cam_map_tab.media_url[req_id - 1]))
                {
                    DEBUG_ERROR("Error open request [%d:%s].\n", req_id, g_cam_map_tab.media_url[req_id - 1]);
                    continue;
                }

                g_cam_map_tab.req_count[req_id - 1]++;
                if (g_cam_map_tab.req_count[req_id - 1] == 1)
                    fork_ffmpeg_process(req_id);

            }
        }
        else if (strstr(buffer, "close connection") && !strstr(buffer, "127.0.0.1"))
        {
            ptr = strstr(buffer, "[GET] \"/live");
            if (NULL != ptr)
            {
                sscanf(ptr, "[GET] \"/live%d.mp4", &close_id);

                if (close_id > CAM_MAX_NUM || 0 == strlen(g_cam_map_tab.media_url[close_id - 1]))
                {
                    DEBUG_ERROR("Error close request [%d:%s].\n", close_id, g_cam_map_tab.media_url[close_id - 1]);
                    continue;
                }
                g_cam_map_tab.req_count[close_id - 1]--;
                if (0 == g_cam_map_tab.req_count[close_id - 1])
                {
                    kill_ffmpeg_process(close_id);
                }
            }
        }
    }

    pclose(cmdfp);
    return NULL;
}


#endif
