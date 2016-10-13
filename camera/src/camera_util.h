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

#ifndef _CAMERA_UTIL_H_
#define _CAMERA_UTIL_H_
#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "hc_common.h"
#include <time.h>
#include "public.h"

//#include "sql_api.h"
#ifdef __cplusplus
extern "C"
{
#endif
//#pragma pack(1)

#define CAM_INIT_USE_CGI


#if 0 // Enabled StreamDaemon
#define CAM_USE_FFSERVER
#endif

// reference to MAX_IPCAM_LIST_LEN
#define CAMERACOUNT  10
typedef enum {
    CAMERA_NULL = 0,
    CAMERA_ONLINE,
    CAMERA_OFFLINE,
    CAMERA_DELETE
}
CAMERA_STATE_E;

typedef enum {
    E_XML_ADDRESS,
    E_XML_URI,
    E_XML_CLIP
} XML_CONTENT;

typedef struct sensor{
    unsigned int dev_id;
    double value;
}SENSOR_INFO_S;


typedef struct camera_clip_file {
    int load;
    unsigned char triggertime[32];
    unsigned char filename[64];
    struct camera_clip_file *next;
} CAMERA_CLIP_FILE_S;

typedef struct {
    unsigned int dev_id;
    char dev_name[64];
    HC_EVENT_TYPE_E event_type;
    HC_NETWORK_TYPE_E network_type;
    HC_DEVICE_TYPE_E dev_type;
    HC_DEVICE_CAMERA_S device;
    CAMERA_STATE_E state;
    time_t searchtime;
    time_t pollingtime;
    int newclip;
    int clipcount;
    CAMERA_CLIP_FILE_S *clip;
    time_t recordtick;
    SENSOR_INFO_S humidity;
    SENSOR_INFO_S temperature;
} DAEMON_CAMERA_INFO_S;

typedef struct {
	unsigned char onvifuid[64];
	char cliIP[32];
	char ser_ip[SMALL_STR_LEN];
}CAMERA_ADD_PARAM;

typedef struct queue_unit_s {
    int msgtype;
    DAEMON_CAMERA_INFO_S devinfo;
    struct queue_unit_s *next;
} QUEUE_UNIT_S;

#define CAM_MAX_NUM         8
DAEMON_CAMERA_INFO_S camerainfo[CAMERACOUNT];
extern pthread_mutex_t g_task_mutex ;
extern pthread_mutex_t g_cam_map_tab_mutex;
extern pthread_mutex_t g_add_mutex;

extern int ipchange;
extern char *pollingrecvbuf;
extern int POLLINGBUFLEN;
extern time_t enable_add_tick;
extern int start_add;

#define ENABLE_CAMERA_ADD_GAP 180

int camera_queue_init(void);
void camera_queue_uninit(void);
int camera_queue_push(QUEUE_UNIT_S *unit);
QUEUE_UNIT_S * camera_queue_popup(void);
int camera_queue_empty(void);
int camera_queue_number(void);
void camera_queue_clean(void);

int camera_start_thread(pthread_t *ptid, int priority, void * (*func)(void *), void *data);
int camera_stop_thread(pthread_t ptid);
int camera_set_nonblocking(int sock);

//int  camera_task_init(void);
//void camera_task_uninit(void);
//int camera_task_add(int msgtype, void *devinfo);
//void * camera_task_handle_thread(void *arg);

int camera_info_init(void);

int  camera_msg_init(void);
void camera_msg_uninit(void);
int  camera_msg_handle(void);
int camera_msg_send(HC_MSG_S *pMsg);
int camera_send_msg_to_app(HC_MSG_S *msg, DAEMON_CAMERA_INFO_S *devinfo);
int add_media_url(char *media_url, int *id);
int del_media_url(char *media_url);
int camera_map_tab_init(void);
int camera_map_tab_uninit(void);



/*
void msg_ZWReport(ZW_DEVICE_INFO *devinfo);
void msg_ZWAddSuccess(ZW_DEVICE_INFO *devinfo);
void msg_ZWAddFail(ZW_STATUS status);
void msg_ZWRemoveSuccess(ZW_DEVICE_INFO *devinfo);
void msg_ZWRemoveFail(ZW_STATUS status);
void msg_ZWReply(HC_EVENT_TYPE_E type, ZW_DEVICE_INFO *devinfo);
*/

int convert_camera_dev_to_hcdev(DAEMON_CAMERA_INFO_S *dev, HC_DEVICE_INFO_S *hcdev);
int convert_msg_to_camera_dev(HC_MSG_S *msg, DAEMON_CAMERA_INFO_S *devinfo);
int convert_camera_dev_to_msg(DAEMON_CAMERA_INFO_S *devinfo, HC_MSG_S *msg);


int parse_ONVIF_XML(char *buf, char *content, int type);
int parse_config_XML_file(char *file, char *content, int type);
int camera_config_update(int update);

int send_post(char *strUrl, char * strPost, char *Responsebuf, int *p_res_code);
int send_get(char *strUrl, char *Responsebuf);
int send_file_post(char *strUrl, char *path, char *Responsebuf);

#ifdef SUPPORT_UPLOADCLIP_AUTO
int camera_update_state(DAEMON_CAMERA_INFO_S *camera, char *ip, char *serIP, CAMERA_STATE_E newstate) ;
#else
int camera_update_state(DAEMON_CAMERA_INFO_S *camera, char *ip, CAMERA_STATE_E newstate) ;
#endif
DAEMON_CAMERA_INFO_S *get_camera_unused(void);
DAEMON_CAMERA_INFO_S *get_camera_by_uid(char *onvifuid);
int camera_del(int camera_id);
int send_record_trigger_post(DAEMON_CAMERA_INFO_S *camera, char *clipname);
int send_factory_default_post(DAEMON_CAMERA_INFO_S *camera);
int send_sdcard_format_get(DAEMON_CAMERA_INFO_S *camera);
int send_time_get(char *ip);
int get_camera_mac(char *mac, char *ip);
int get_camera_id(unsigned int *id);
int get_sensor_id(DAEMON_CAMERA_INFO_S *camera, int sensortye);
int camera_has_humidity(DAEMON_CAMERA_INFO_S *camera);
int camera_has_temperature(DAEMON_CAMERA_INFO_S *camera);
int camera_armmode(void);

/* camera CGI function */
#ifdef SUPPORT_UPLOADCLIP_AUTO
int vivo_IP8131W_init(char *ip, char *serIP, char *serialnumber);
int vivo_IB8168_init(char *ip, char *serIP, char *serialnumber);
int vivo_default_init(char *ip, char *serIP, char *serialnumber);
#else
int vivo_IP8131W_init(char *ip);
int vivo_IB8168_init(char *ip);
int vivo_default_init(char *ip);
#endif

#ifdef __cplusplus
}
#endif

#endif

