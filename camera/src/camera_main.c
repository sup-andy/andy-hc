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
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "curl/curl.h"
#include "capi.h"
#include "error.h"
#include "hcapi.h"
#include "hc_msg.h"
#include "hc_common.h"
#include "cJSON.h"
#include "camera_util.h"
#include "libxml/xmlmemory.h"
#include "libxml/parser.h"
#include "ipcam_upgrade.h"
#include "public.h"


/* camera vendor mac prefix */
#define CAMERA_VENROR_NAME "VIVOTEK"
#define CAMERA_VENDOR_MAC_VIVOTEK  "00:02:d1"
/* gloabal variable. */
int camera_keep_looping = 1;
int posttimeout = 1;
int camera_add_ptid_index = 0;
pthread_cond_t g_cond;
pthread_mutex_t g_mutex ;
pthread_mutex_t g_task_mutex ;
pthread_mutex_t g_add_mutex ;
#ifdef SUPPORT_UPLOADCLIP_AUTO
int fifo_fd;
#endif
int ipchange = 0;
char wan_ip[SMALL_STR_LEN];
char lan_ip[SMALL_STR_LEN];
char *pollingrecvbuf = NULL;
int POLLINGBUFLEN = 1024 * 1024;
#define CAMERA_NAME_CONFIG_FILE "/storage/config/homectrl/cameraname"
#define CAMERA_CONFIG_FILE         SYS_CAMERA_CONFIG_STORAGE_PATH
#define CAMERA_VERSION_FILE     "/storage/config/homectrl/camera_version"
#define CAMERAPOLLINGGAP 3
#define CAMERASEARCHCOUNT 20
#define ONVIFPORT 3702
#define ONVIFSEARCHADDR "239.255.255.250"
// Extend the ipcam disconnect check time to 2hr
#define CAMERAOFFLINEGAP 7200
#define CAMERA_CHECK_NETWORK_TIME 30
#define CLIP_VAILD_COUNT 200
#define CLIP_HUMAN_DETECTION_MAX 8
HC_ARM_MODE g_camera_armmode = {0};

void * camera_search_task(void *arg);
void * camera_polling_task(void *arg);
void * camera_clip_task(void *arg);
void * ffserver_monitor_task(void *arg);
void ffmpeg_sig_child_func(int sig);
#ifdef SUPPORT_UPLOADCLIP_AUTO
void ftp_message_handle_thread(void *arg);
#endif

static void get_wanlan_status(void)
{
    int ret = -1;
    memset(wan_ip, 0, SMALL_STR_LEN);
    memset(lan_ip, 0, SMALL_STR_LEN);
    ret = capi_get_runtime_wan_ip(wan_ip, SMALL_STR_LEN);
    DEBUG_INFO("get runtime wan ip : %s\n", wan_ip);
    if (ret != 0)
    {
        DEBUG_ERROR("get runtime wan ip failure\n");
        memset(wan_ip, 0, SMALL_STR_LEN);
    }
    ret = capi_get_runtime_lan_ip(lan_ip, SMALL_STR_LEN);
    DEBUG_INFO("get runtime lan_ip ip : %s\n", lan_ip);
    if (ret != 0)
    {
        DEBUG_ERROR("get runtime lan ip failure\n");
        memset(lan_ip, 0, SMALL_STR_LEN);
    }
}

static void signal_handle(int signal)
{
    camera_keep_looping = 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    struct sigaction newAction;
    pthread_t pid_search_task = 0;
    pthread_t pid_clip_task = 0;
    pthread_t pid_ffserver_task = 0;

#ifdef SUPPORT_UPLOADCLIP_AUTO
    pthread_t pid_ftpmessage_task = 0;
#endif

#ifdef CTRL_LOG_DEBUG
    ctrl_log_init(HC_CAMERA_NAME);
#endif

    capi_init();

#ifdef CAM_INIT_USE_CGI
    camera_version_update(0);
#else
    camera_config_update(0);
#endif
    // initialize msg handle, comunicate with dispatch.
    ret = camera_msg_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to camera_msg_init.\n");
        goto EXIT;
    }

    pthread_cond_init(&g_cond, NULL);
    pthread_mutex_init(&g_mutex, NULL);
    pthread_mutex_init(&g_task_mutex, NULL);
	pthread_mutex_init(&g_add_mutex, NULL);

    ret = hcapi_get_armmode(&g_camera_armmode);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Failed to get armmode.\n");
        return -1;
    }

    ret = camera_info_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to camera_info_init.\n");
        goto EXIT;
    }
#ifdef SUPPORT_UPLOADCLIP_AUTO
    ret = vsftpd_logfd_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to vsftpd_logfd_init.\n");
        goto EXIT;
    }
#endif
    ret = camera_map_tab_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to camera_map_tab_init.\n");
        goto EXIT;
    }
    /* Register signal handler */
    signal(SIGINT, signal_handle);
    signal(SIGTERM, signal_handle);
    signal(SIGCHLD, ipcam_signal_handler);

    /* Ignore broken pipes */
    signal(SIGPIPE, SIG_IGN);

    ret = ipcam_shm_data_init();
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to share memory data, ret = %d", ret);
        goto EXIT;
    }

#ifdef CAM_USE_FFSERVER
    /* catch SIGTERM and SIGINT so we can properly clean up */
    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = signal_handle;
    newAction.sa_flags = SA_RESETHAND;
    ret = sigaction(SIGINT, &newAction, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to install SIGINT handler, ret = %d", ret);
        goto EXIT;
    }

    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = signal_handle;
    newAction.sa_flags = SA_RESETHAND;
    ret = sigaction(SIGTERM, &newAction, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Failed to install SIGTERM handler.\n");
        goto EXIT;
    }
    signal(SIGCHLD, ffmpeg_sig_child_func);
#endif

    pollingrecvbuf = malloc(POLLINGBUFLEN);
    if (pollingrecvbuf == NULL)
    {
        DEBUG_ERROR("pollingrecvbuf malloc failure.\n");
        goto EXIT;
    }

    get_wanlan_status();

    // startup task thread.
    ret = camera_start_thread(&pid_search_task, 0, camera_search_task, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread camera_search_task failed.\n");
        goto EXIT;
    }

    ret = camera_start_thread(&pid_clip_task, 0, camera_clip_task, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread camera_clip_task failed.\n");
        goto EXIT2;
    }

#ifdef SUPPORT_UPLOADCLIP_AUTO
    ret = camera_start_thread(&pid_ftpmessage_task, 0, ftp_message_handle_thread, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread ftp_message_handle_thread failed.\n");
        goto EXIT2;
    }
#endif

#ifdef CAM_USE_FFSERVER
    ret = camera_start_thread(&pid_ffserver_task, 0, ffserver_monitor_task, NULL);
    if (ret != 0)
    {
        DEBUG_ERROR("Start thread ffserver_monitor_task failed.\n");
        goto EXIT2;
    }
#else
    system("killall -9 streamd");
    system("/tmp/stream/streamd 2>/dev/null 1>/dev/null 0</dev/null &");
#endif

    FILE *fp = fopen(CAMERA_STARTED_FILE, "w");
    if (fp == NULL)
    {
        DEBUG_ERROR("Create file %s failed, errno is '%d'.\n", CAMERA_STARTED_FILE, errno);
        goto EXIT2;
    }
    fclose(fp);

    // man loop.
    camera_msg_handle();

EXIT2:
    camera_keep_looping = 0;

    // stop thread.
    if (pid_search_task != 0)
        camera_stop_thread(pid_search_task);
    if (pid_clip_task != 0)
        camera_stop_thread(pid_clip_task);
    if (pid_ffserver_task != 0)
        camera_stop_thread(pid_ffserver_task);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (pid_ftpmessage_task != 0)
        camera_stop_thread(pid_ftpmessage_task);
#endif
EXIT:
    pthread_cond_destroy(&g_cond);
    pthread_mutex_destroy(&g_mutex);
    pthread_mutex_destroy(&g_task_mutex);
    pthread_mutex_destroy(&g_add_mutex);


    camera_msg_uninit();
#ifdef SUPPORT_UPLOADCLIP_AUTO
    vsftpd_logfd_uninit();
#endif
    camera_map_tab_uninit();
    capi_uninit();
    xmlCleanupParser();

    return 0;
}
char g_config_version[256];
int camera_wifi_info_update(char *file)
{
    char command[256];
    char config_version[256];
    char SSID[64];
    char EncrypType[64];
    char Algorithm[64];
    char Presharedkey[64];

    CAPI_RESULT_E result;
    WLAN_BASIC_CFG_S wlan_basic_u_cfg;
    WLAN_SEC_CFG_S wlan_security_u_cfg;

    memset(&wlan_basic_u_cfg, 0, sizeof(wlan_basic_u_cfg));
    memset(&wlan_security_u_cfg, 0, sizeof(wlan_security_u_cfg));

    if ((result = capi_get_wlan_basic(&wlan_basic_u_cfg)) == CAPI_FAILURE)
    {
        DEBUG_ERROR("ERROR: capi_get_wlan_basic_upstream, result: %d\n", result);
        return -1;
    }

    if ((result = capi_get_wlan_security(&wlan_security_u_cfg)) == CAPI_FAILURE)
    {
        DEBUG_ERROR("ERROR: capi_get_wlan_security_upstream, result: %d\n", result);
        return -1;
    }

    if (WIFI_AUTH_WPA2_PERSONAL == wlan_security_u_cfg.wlan_auth_type
            || WIFI_AUTH_WPA_PERSONAL == wlan_security_u_cfg.wlan_auth_type
            || WIFI_AUTH_DISABLED == wlan_security_u_cfg.wlan_auth_type
            || WIFI_AUTH_WEP == wlan_security_u_cfg.wlan_auth_type)
    {
    }
    else
    {
        DEBUG_ERROR("ERROR: Camera unsupport the wlan_auth_type: %d\n", result);
        return -1;
    }

    memset(SSID, 0, sizeof(SSID));
    memset(EncrypType, 0, sizeof(EncrypType));
    memset(Algorithm, 0, sizeof(Algorithm));
    memset(Presharedkey, 0, sizeof(Presharedkey));


    if (WIFI_AUTH_WPA2_PERSONAL == wlan_security_u_cfg.wlan_auth_type)
    {
        memset(command, 0, sizeof(command));
        snprintf(SSID, sizeof(SSID), "SSID=\"%s\"", wlan_basic_u_cfg.wlan_essid);
        sprintf(command, "sed -i \"2c%s\" %s", SSID, file);
        system(command);

        memset(command, 0, sizeof(command));
        snprintf(EncrypType, sizeof(EncrypType), "EncrypType=3");
        sprintf(command, "sed -i \"6c%s\" %s", EncrypType, file);
        system(command);

        memset(command, 0, sizeof(command));
        if (wlan_security_u_cfg.wlan_encry_type == WPA_TKIP)
        {
            snprintf(Algorithm, sizeof(Algorithm), "Algorithm=TKIP");
        }
        else
        {
            snprintf(Algorithm, sizeof(Algorithm), "Algorithm=AES");
        }
        sprintf(command, "sed -i \"16c%s\" %s", Algorithm, file);
        system(command);

        memset(command, 0, sizeof(command));
        snprintf(Presharedkey, sizeof(Presharedkey), "Presharedkey=\"%s\"", wlan_security_u_cfg.wlan_passphrase);
        sprintf(command, "sed -i \"17c%s\" %s", Presharedkey, file);
        system(command);
    }
    else if (WIFI_AUTH_WPA_PERSONAL == wlan_security_u_cfg.wlan_auth_type)
    {
        memset(command, 0, sizeof(command));
        snprintf(SSID, sizeof(SSID), "SSID=\"%s\"", wlan_basic_u_cfg.wlan_essid);
        sprintf(command, "sed -i \"2c%s\" %s", SSID, file);
        system(command);

        memset(command, 0, sizeof(command));
        snprintf(EncrypType, sizeof(EncrypType), "EncrypType=2");
        sprintf(command, "sed -i \"6c%s\" %s", EncrypType, file);
        system(command);

        memset(command, 0, sizeof(command));
        if (wlan_security_u_cfg.wlan_encry_type == WPA_TKIP)
        {
            snprintf(Algorithm, sizeof(Algorithm), "Algorithm=TKIP");
        }
        else
        {
            snprintf(Algorithm, sizeof(Algorithm), "Algorithm=AES");
        }
        sprintf(command, "sed -i \"16c%s\" %s", Algorithm, file);
        system(command);

        memset(command, 0, sizeof(command));
        snprintf(Presharedkey, sizeof(Presharedkey), "Presharedkey=\"%s\"", wlan_security_u_cfg.wlan_passphrase);
        sprintf(command, "sed -i \"17c%s\" %s", Presharedkey, file);
        system(command);

    }
    else if (WIFI_AUTH_DISABLED == wlan_security_u_cfg.wlan_auth_type)
    {
        memset(command, 0, sizeof(command));
        snprintf(SSID, sizeof(SSID), "SSID=\"%s\"", wlan_basic_u_cfg.wlan_essid);
        sprintf(command, "sed -i \"2c%s\" %s", SSID, file);
        system(command);

        memset(command, 0, sizeof(command));
        snprintf(EncrypType, sizeof(EncrypType), "EncrypType=0");
        sprintf(command, "sed -i \"6c%s\" %s", EncrypType, file);
        system(command);
    }
    else if (WIFI_AUTH_WEP == wlan_security_u_cfg.wlan_auth_type)
    {
        memset(command, 0, sizeof(command));
        snprintf(SSID, sizeof(SSID), "SSID=\"%s\"", wlan_basic_u_cfg.wlan_essid);
        sprintf(command, "sed -i \"2c%s\" %s", SSID, file);
        system(command);

        memset(command, 0, sizeof(command));
        snprintf(EncrypType, sizeof(EncrypType), "EncrypType=1");
        sprintf(command, "sed -i \"6c%s\" %s", EncrypType, file);
        system(command);

        sprintf(command, "sed -i \"9cKeyFormat=ASCII\" %s", file);
        system(command);

        memset(command, 0, sizeof(command));
        sprintf(command, "sed -i \"10cKeySelect=%d\" %s", wlan_security_u_cfg.wlan_key_used, file);
        system(command);

        memset(Presharedkey, 0, sizeof(Presharedkey));
        memset(command, 0, sizeof(command));
        snprintf(Presharedkey, sizeof(Presharedkey), "Key1=\"%s\"", wlan_security_u_cfg.wlan_key[0]);
        sprintf(command, "sed -i \"11c%s\" %s", Presharedkey, file);
        system(command);

        memset(Presharedkey, 0, sizeof(Presharedkey));
        memset(command, 0, sizeof(command));
        snprintf(Presharedkey, sizeof(Presharedkey), "Key2=\"%s\"", wlan_security_u_cfg.wlan_key[1]);
        sprintf(command, "sed -i \"12c%s\" %s", Presharedkey, file);
        system(command);

        memset(Presharedkey, 0, sizeof(Presharedkey));
        memset(command, 0, sizeof(command));
        snprintf(Presharedkey, sizeof(Presharedkey), "Key3=\"%s\"", wlan_security_u_cfg.wlan_key[2]);
        sprintf(command, "sed -i \"13c%s\" %s", Presharedkey, file);
        system(command);

        memset(Presharedkey, 0, sizeof(Presharedkey));
        memset(command, 0, sizeof(command));
        snprintf(Presharedkey, sizeof(Presharedkey), "Key4=\"%s\"", wlan_security_u_cfg.wlan_key[3]);
        sprintf(command, "sed -i \"14c%s\" %s", Presharedkey, file);
        system(command);


    }
    /*
        memset(command, 0, sizeof(command));
        //sprintf(command, "sed -i \"145c\t\t\t%s\" %s", config_version, file);
        sprintf(command, "sed -i \"145c%s\" %s", config_version, file);
        system(command);


        memset(command, 0, sizeof(command));
        sprintf(command, "sed -n -e \"145p\" %s", file);
        system(command);
    */
    return 0;
}
int camera_config_version_get(char *cur_version)
{
    char command[256];
    CAPI_RESULT_E result = CAPI_FAILURE;
    char firmware_version[128];
    char product_SN[MAX_STR_LEN];
    memset(firmware_version, 0, sizeof(firmware_version));
    if ((result = capi_get_firmware_version(firmware_version, sizeof(firmware_version))) == CAPI_FAILURE)
    {
        DEBUG_ERROR("ERROR: capi_get_firmware_version, result: %d\n", result);
        return -1;
    }

    memset(&product_SN, 0, sizeof(product_SN));
    result = capi_get_hardware_info(HW_SERIAL_NUM, product_SN, sizeof(product_SN));
    if (result != CAPI_SUCCESS)
    {
        DEBUG_ERROR("ERROR: capi_get_hardware_info, result: %d\n", result);
        return -1;
    }
    snprintf(cur_version, 256, "%s-%s", product_SN, firmware_version);
    return 0;
}
int camera_config_version_update(char *cur_version, char *file)
{

    char command[256];
    time_t tm = time(NULL);
    char config_version[256];
    memset(config_version, 0, sizeof(config_version));

    memset(g_config_version, 0, sizeof(g_config_version));
    snprintf(g_config_version, sizeof(g_config_version), "%s-%d", cur_version, tm);
    snprintf(config_version, sizeof(config_version), "<name>%s-%d</name>", cur_version, tm);
    DEBUG_INFO("Camera config file update version = %s\n", g_config_version);


    memset(command, 0, sizeof(command));
    //sprintf(command, "sed -i \"145c\t\t\t%s\" %s", config_version, file);
    sprintf(command, "sed -i \"145c%s\" %s", config_version, file);
    system(command);

    return 0;
}

int camera_config_update(int update)
{
#define WIRELESS_CONF "etc/network/wireless.conf"
#define EVENT_CONF "etc/conf.d/event/event.xml"
    int ret = -1;
    char command[256];
    char old_version[256];
    char cur_version[256];
    memset(old_version, 0, sizeof(old_version));
    memset(cur_version, 0, sizeof(cur_version));
    if (access(CAMERA_CONFIG_FILE, R_OK) != 0)
    {
        memset(command, 0, sizeof(command));
        //sprintf(command, "cp /nv/camera_config.tar.gz /storage/config/homectrl/");
        sprintf(command, "cp %s  %s", SYS_CAMERA_CONFIG_PATH, APP_HOME_CONTROL_ROOT_DIR);
        system(command);
    }
    if (camera_config_version_get(cur_version) == -1)
    {
        return -1;
    }
    if (access(CAMERA_CONFIG_FILE, R_OK) == 0)
    {
        //chdir("/storage/config/homectrl/");
        chdir(APP_HOME_CONTROL_ROOT_DIR);

        memset(command, 0, sizeof(command));
        sprintf(command, "rm -fr %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR);
        system(command);
        memset(command, 0, sizeof(command));
        sprintf(command, "rm -fr /storage/config/homectrl/etc");
        system(command);

        memset(command, 0, sizeof(command));
        //sprintf(command, "gunzip camera_config.tar.gz");
        sprintf(command, "gunzip %s", SYS_CAMERA_CONFIG_STORAGE_PATH);
        system(command);


        memset(command, 0, sizeof(command));
        //sprintf(command, "tar xmvf camera_config.tar etc/network/wireless.conf");
        sprintf(command, "tar xmvf %s %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR, WIRELESS_CONF);
        system(command);
        if (-1 == camera_wifi_info_update(WIRELESS_CONF))
        {
            DEBUG_ERROR("camera_wifi_info_update failure\n");
        }
        memset(command, 0, sizeof(command));
        //sprintf(command, "tar --update -mvf camera_config.tar etc/network/wireless.conf");
        sprintf(command, "tar --update -vf %s %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR, WIRELESS_CONF);
        system(command);

        memset(command, 0, sizeof(command));
        //sprintf(command, "tar xmvf camera_config.tar etc/conf.d/event/event.xml");
        sprintf(command, "tar xmvf %s %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR, EVENT_CONF);
        system(command);

        ret = parse_config_XML_file(EVENT_CONF, old_version, 0);
        if (ret != 0)
        {
            DEBUG_ERROR("Camera config file parse error\n");
            memset(command, 0, sizeof(command));
            //sprintf(command, "gzip camera_config.tar");
            sprintf(command, "gzip %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR);
            system(command);
            return -1;
        }
        DEBUG_INFO("Camera config file old_version = %s, cur_version = %s\n", old_version, cur_version);
        memset(g_config_version, 0, sizeof(g_config_version));
        memcpy(g_config_version, old_version, strlen(old_version));
        if (strncmp(cur_version, old_version, strlen(cur_version)) == 0 && update == 0)
        {
            memset(command, 0, sizeof(command));
            //sprintf(command, "gzip camera_config.tar");
            sprintf(command, "gzip %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR);
            system(command);
            return 0;
        }


        //camera_config_version_update(cur_version, "/storage/config/homectrl/etc/conf.d/event/event.xml");
        camera_config_version_update(cur_version, EVENT_CONF);

        memset(command, 0, sizeof(command));
        sprintf(command, "rm -fr %s", SYS_CAMERA_CONFIG_STORAGE_PATH);
        system(command);

        memset(command, 0, sizeof(command));
        //sprintf(command, "tar --update -mvf camera_config.tar etc/conf.d/event/event.xml");
        sprintf(command, "tar --update -vf %s %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR, EVENT_CONF);
        system(command);
        memset(command, 0, sizeof(command));
        //sprintf(command, "gzip camera_config.tar");
        sprintf(command, "gzip %s", SYS_CAMERA_CONFIG_STORAGE_PATH_TAR);
        system(command);

    }
    else
    {
        DEBUG_ERROR("Camera config file is not exist\n");
    }
    return 0;
}

char g_camera_version[256];
char g_camera_modelname[256];
int camera_version_update(int update)
{
    char old_version[256];
    char cur_version[256];
    memset(old_version, 0, sizeof(old_version));
    memset(cur_version, 0, sizeof(cur_version));

    FILE *fp = NULL;

    if (camera_config_version_get(cur_version) == -1)
    {
        return -1;
    }

    if (access(CAMERA_VERSION_FILE, R_OK) != 0)
    {
        time_t tm = time(NULL);
        memset(g_camera_version, 0, sizeof(g_camera_version));
        snprintf(g_camera_version, sizeof(g_camera_version), "%s-%d", cur_version, tm);
        fp = fopen(CAMERA_VERSION_FILE, "w");
        if (NULL == fp)
        {
            DEBUG_ERROR("Open %s failed, errno:%d\n", CAMERA_VERSION_FILE, errno);
            return -1;
        }
        fwrite(g_camera_version, 1, strlen(g_camera_version), fp);
        fclose(fp);
    }
    else
    {
        fp = fopen(CAMERA_VERSION_FILE, "r");
        if (NULL == fp)
        {
            DEBUG_ERROR("Open %s failed, errno:%d\n", CAMERA_VERSION_FILE, errno);
            return -1;
        }
        fread(old_version, 1, sizeof(old_version), fp);
        fclose(fp);

        if (strncmp(cur_version, old_version, strlen(cur_version)) == 0 && update == 0)
        {
            memset(g_camera_version, 0, sizeof(g_camera_version));
            strcpy(g_camera_version, old_version);
            return 0;
        }

        unlink(CAMERA_VERSION_FILE);

        time_t tm = time(NULL);
        memset(g_camera_version, 0, sizeof(g_camera_version));
        snprintf(g_camera_version, sizeof(g_camera_version), "%s-%d", cur_version, tm);
        fp = fopen(CAMERA_VERSION_FILE, "w");
        if (NULL == fp)
        {
            DEBUG_ERROR("Open %s failed, errno:%d\n", CAMERA_VERSION_FILE, errno);
            return -1;
        }
        fwrite(g_camera_version, 1, strlen(g_camera_version), fp);
        fclose(fp);
    }

    DEBUG_INFO("g_camera_version=%s\n", g_camera_version);
    return 0;
}

int camera_info_init(void)
{
    int ret = 0;
    int num = 0, i = 0;
    HC_DEVICE_INFO_S *deviceinfo = NULL;
    DAEMON_CAMERA_INFO_S *camera = NULL;

    memset(camerainfo, 0, sizeof(camerainfo));

    ret = hcapi_get_devnum_by_network_type(HC_NETWORK_TYPE_CAMERA, &num);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Get camera num error \n");
        return -1;
    }

    if (num == 0)
    {
        DEBUG_INFO("No cameras in DB.");
        return 0;
    }
    else if (num > CAMERACOUNT)
    {
        DEBUG_ERROR("Warning the DB camera number %d is more than limitation %d.\n", num, CAMERACOUNT);
    }

    deviceinfo = (HC_DEVICE_INFO_S *)calloc(num, sizeof(HC_DEVICE_INFO_S));
    if (deviceinfo == NULL)
    {
        DEBUG_ERROR("calloc error \n");
        return -1;
    }
    ret = hcapi_get_devs_by_network_type(HC_NETWORK_TYPE_CAMERA, deviceinfo, num);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Get camera device information error \n");
        free(deviceinfo);
        return -1;
    }

    for (i = 0; i < num; i++)
    {
        camera = get_camera_by_devid(deviceinfo[i].dev_id);
        if (NULL == camera)
        {
            DEBUG_ERROR("Get unused camera information failed. \n");
            free(deviceinfo);
            return -1;
        }

        DEBUG_INFO("deviceinfo[%d].dev_type = %d\n", i , deviceinfo[i].dev_type);
        if(deviceinfo[i].dev_type == HC_DEVICE_TYPE_IPCAMERA)
        {
            camera->dev_id = deviceinfo[i].dev_id;
            strncpy(camera->device.onvifuid, deviceinfo[i].device.camera.onvifuid, sizeof(camera->device.onvifuid) - 1);
            strncpy(camera->dev_name, deviceinfo[i].dev_name, sizeof(camera->dev_name) - 1);
            strncpy(camera->device.modelname, deviceinfo[i].device.camera.modelname, sizeof(camera->device.modelname));
            strncpy(camera->device.ipaddress, deviceinfo[i].device.camera.ipaddress, sizeof(camera->device.ipaddress));

            DEBUG_INFO("id:[%x], name:[%s], onvifuid:[%s].\n", camera->dev_id, camera->dev_name, camera->device.onvifuid);
        }
        else if(deviceinfo[i].dev_type == HC_DEVICE_TYPE_MULTILEVEL_SENSOR)
        {
            if(deviceinfo[i].device.multilevel_sensor.sensor_type == HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE)
            {
                camera->temperature.dev_id = deviceinfo[i].dev_id;
                DEBUG_INFO("id:[%x], name:[%s], onvifuid:[%s].\n", camera->temperature.dev_id, camera->dev_name, camera->device.onvifuid);
            }
            else if(deviceinfo[i].device.multilevel_sensor.sensor_type == HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY)
            {
                camera->humidity.dev_id = deviceinfo[i].dev_id;
                DEBUG_INFO("id:[%x], name:[%s], onvifuid:[%s].\n", camera->humidity.dev_id, camera->dev_name, camera->device.onvifuid);
            }
        }
    }
    free(deviceinfo);

    return 0;
}

int set_camera_motion_detection()
{
    DAEMON_CAMERA_INFO_S *camera = NULL;
    int i = 0;

    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        if (camera != NULL
                && camera->dev_id != 0
                && camera->state == CAMERA_ONLINE)
        {

            if (check_camera_alarm_enable(camera->dev_id) == 0)
            {
                send_motion_decetion_post(camera, 1);
            }
            else
            {
                send_motion_decetion_post(camera, 0);
            }
        }
    }
    return 0;
}
int camera_armmode(void)
{
    int ret = 0;

    ret = hcapi_get_armmode(&g_camera_armmode);
    if (ret != HC_RET_SUCCESS)
    {
        DEBUG_ERROR("Failed to get armmode.\n");
        return -1;
    }

    DEBUG_INFO("armmode = %d, profile [%s]...\n", g_camera_armmode.mode, g_camera_armmode.devicesid);

    set_camera_motion_detection();

    return 0;
}
int check_camera_alarm_enable(int id)
{
    switch (g_camera_armmode.mode)
    {
        case HC_DISARM:
        {
            return -1;
        }
        case HC_ARM:
        {
            return 0;
        }
        case HC_STAY:
        {
            char *p = NULL;
            char tmp[1024] = {0};
            int devid = 0;
            char item[128] = {0};
            snprintf(tmp, sizeof(tmp), "%s", g_camera_armmode.devicesid);


            p = strtok(tmp, ",");
            while (p)
            {
                /* parse device id:network type,2:1 */
                snprintf(item, sizeof(item), "%s", p);

                devid = atoi(item);

                if (id == devid)
                {
                    return 0;
                }
                p = strtok(NULL, ",");
            }
            break;
        }
        default:
            DEBUG_ERROR("Unknow armmode [%d]...\n", g_camera_armmode.mode);
            break;
    }
    return -1;
}

int get_clip(DAEMON_CAMERA_INFO_S *camera)
{
    char destname[64] = {0};
    char destname2[96] = {0};
    char urlstr[256] = {0};
    char system_cmd[256] = {0};
    CAMERA_CLIP_FILE_S *pclipfile = NULL;
    CAMERA_CLIP_FILE_S *clipfiletmp = NULL;

    pthread_mutex_lock(&g_mutex);
    pclipfile = camera->clip;
    camera->clip = NULL;
    pthread_mutex_unlock(&g_mutex);

    //pthread_mutex_lock(&g_task_mutex);
    while (pclipfile != NULL)
    {
        if (pclipfile->load == 0)
        {
            pclipfile->load = 1;

            memset(destname, 0, sizeof(destname));
            memset(destname2, 0, sizeof(destname2));
            memset(urlstr, 0, sizeof(urlstr));
            memset(system_cmd, 0, sizeof(system_cmd));
            char *name = strrchr(pclipfile->filename, '/');
            if (NULL == name)
            {
                DEBUG_ERROR("ERROR: video_url format error %s\n", pclipfile->filename);
            }
            name++;
#ifdef SUPPORT_UPLOADCLIP_AUTO
            sprintf(destname, "%s", name);
#else
            sprintf(destname, "%s-%s", camera->device.ipaddress, name);
#endif
            sprintf(destname2, "/storage/clip/%s", destname);
            if (access(destname2, R_OK) == 0)
            {
                //printf("the file is exist\n");
                /*
                        memset(system_cmd, 0x0, sizeof(system_cmd));
                        snprintf(system_cmd, sizeof(system_cmd) - 1, "rm -fr %s", destname2);
                        if (system(system_cmd) != 0)
                        {
                            DEBUG_ERROR("ERROR: system wget:%s\n", strerror(errno));
                        }
                        else
                        {
                            DEBUG_INFO("remove file complete\n");
                        }
                        */
            }
            else
            {

                sprintf(urlstr, "http://%s/cgi-bin/admin/downloadMedias.cgi?%s", camera->device.ipaddress, pclipfile->filename);
                memset(system_cmd, 0x0, sizeof(system_cmd));
                //snprintf(system_cmd, sizeof(system_cmd) - 1, "/usr/bin/wget -P /storage/clip/ -O %s %s", destname, urlstr);
                snprintf(system_cmd, sizeof(system_cmd) - 1, "/usr/bin/wget  -O /storage/clip/%s %s", destname, urlstr);
                DEBUG_INFO("system_cmd = %s\n", system_cmd);
                if (system(system_cmd) != 0)
                {
                    DEBUG_ERROR("ERROR: system wget:%s\n", strerror(errno));
                }
                else
                {
                    DEBUG_INFO("Get file complete\n");
                }
            }
        }
        clipfiletmp = pclipfile->next;
        free(pclipfile);
        pclipfile = clipfiletmp;

    }
    /*
    pclipfile = camera->clip;
    while (pclipfile != NULL && pclipfile->load == 1)
    {
            clipfiletmp = pclipfile->next;
            free(pclipfile);
            pclipfile = clipfiletmp;
    }
    camera->clip = pclipfile;
    */
    //pthread_mutex_unlock(&g_task_mutex);

    return 0;
}
int get_clip_count()
{
    FILE *fp;
    int status;
    char path[1024];
    int count = 0;
    /* Open the command for reading. */
    fp = popen("ls -l /storage/clip/ |grep \"^-\" | wc -l", "r");
    if (fp == NULL)
    {
        DEBUG_ERROR("Failed to run command\n");
        return -1;
    }
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {

    }
    DEBUG_INFO("storage file count = %d\n", count = atoi(path));
    /* close */
    pclose(fp);
    return count;

}
int check_clip(DAEMON_CAMERA_INFO_S *camera)
{
    int i = 0;
    int cliptotal = 0;
    int count = 0;
    cliptotal = get_clip_count();
    count = cliptotal - CLIP_VAILD_COUNT * 3 - CLIP_HUMAN_DETECTION_MAX;
    while (count > 0)
    {
        count--;
        system("rm /storage/clip/$(ls /storage/clip -rc|head -1)");
    }
    return 0;
}
void *camera_clip_task(void *arg)
{
    int i = 0;
    DAEMON_CAMERA_INFO_S *camera = NULL;
    while (camera_keep_looping == 1)
    {
        pthread_mutex_lock(&g_mutex);
        pthread_cond_wait(&g_cond, &g_mutex);
        pthread_mutex_unlock(&g_mutex);
        for (i = 0; i < CAMERACOUNT; i++)
        {
            camera = &camerainfo[i];
#ifdef SUPPORT_UPLOADCLIP_AUTO
            if (camera->state == CAMERA_ONLINE)
            {
#else
            if (camera->state == CAMERA_ONLINE && camera->newclip > 0)
            {
                get_clip(camera);
#endif
                check_clip(camera);
            }
        }
    }
    return NULL;
}

void *camera_polling_task(void *arg)
{
    DAEMON_CAMERA_INFO_S *camera = NULL;

    return NULL;
}

void * camera_search_task(void *arg)
{
    DAEMON_CAMERA_INFO_S *camera = NULL;
    long searchcount = 0;
    int ret = -1;
    //camera_db_sync(camerainfo);
    do
    {
        if (searchcount % 1000 == 0 || ipchange == 1)
        {
            if (ipchange == 1)
                system("ifconfig eth2:33 down");
            get_wanlan_status();
        }
        
        //once start auto scan ,send onvif pakcet right now
        if(start_add == 1)
        {
            start_add = 0;
            searchcount = 0;
        }
        pthread_mutex_lock(&g_task_mutex);
        if (searchcount++ % CAMERASEARCHCOUNT == 0)
        {
            camera_search(wan_ip, lan_ip, 5);
        }
        pthread_mutex_unlock(&g_task_mutex);

        if (ipchange == 1)
        {
            system("ifconfig eth2:33 192.168.33.1");
            ipchange = 0;
        }
        if(0 == enable_add_tick)
        {
            pthread_mutex_lock(&g_task_mutex);
            polling_event(searchcount);
            pthread_mutex_unlock(&g_task_mutex);
        }
        
        sleep(CAMERAPOLLINGGAP);


    } while (camera_keep_looping == 1);
    return NULL;
}

int send_onvif_search_send(char *ip)
{
    char bufrecv[2048] = {0};
    struct sockaddr_in addrto = {0};
    struct sockaddr_in from;
    char *searchstr = "<?xml version=\"1.0\" encoding=\"utf-8\"?><Envelope xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\"><Header><wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:d05a357a-a01e-4ad0-9d70-e1bdd24e9393</wsa:MessageID><wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To><wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action></Header><Body><Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"><Types>dn:NetworkVideoTransmitter</Types><Scopes /></Probe></Body></Envelope>";
    //for discover camera IPCAM8137W
    char *searchstr2 ="<?xml version=\"1.0\" encoding=\"utf-8\"?><Envelope xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\"><Header><wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:ea66bf6a-3306-43f1-b762-d7565d442638</wsa:MessageID><wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To><wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action></Header><Body><Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"><Types>tds:Device</Types><Scopes /></Probe></Body></Envelope>";
    int sock = -1;
    int count = 0;
    int count2 = 0;
    do
    {
        if (ip == NULL || strlen(ip) < 7)
            return sock;
        bzero(&addrto, sizeof(struct sockaddr_in));
        addrto.sin_family = AF_INET;
        inet_aton(ip, &addrto.sin_addr.s_addr);

        if (inet_pton(AF_INET, ip, &addrto.sin_addr) < 0)
        {
            DEBUG_ERROR("set ip:%s error\n", ip);
            break;
        }
        addrto.sin_port = htons(ONVIFPORT);

        bzero(&from, sizeof(struct sockaddr_in));
        from.sin_family = AF_INET;
        inet_pton(AF_INET, ONVIFSEARCHADDR, &from.sin_addr);
        from.sin_port = htons(ONVIFPORT);

        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            DEBUG_ERROR("socket failure\n");
            break;
        }
        int reuse = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
        {
            DEBUG_ERROR("setsockopt failure\n");
            close(sock);
            return -1;
        }
        struct timeval timeout, timeout2;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        memset(addrto.sin_zero, '\0', sizeof(addrto.sin_zero));
        if (bind(sock, (struct sockaddr *) & (addrto), sizeof(addrto)) == -1)
        {
            DEBUG_ERROR("bind failure error = %d\n", error);
            close(sock);
            return -1;
        }
        memset(bufrecv, 0, sizeof(bufrecv));
        count = sendto(sock, searchstr, strlen(searchstr), 0, (struct sockaddr *)&from, sizeof(from));
        count2 = sendto(sock, searchstr2, strlen(searchstr2), 0, (struct sockaddr *)&from, sizeof(from));
        if (count == -1 || count2 == -1)
        {
            DEBUG_ERROR("sendto error");
            close(sock);
            return -1;
        }
        else
        {
        }
    } while (0);
    return sock;
}

int send_onvif_media_request(char *mediaurl, char *cliIP)
{
    char *mediastr = "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><GetStreamUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\"><StreamSetup><Stream xmlns=\"http://www.onvif.org/ver10/schema\">RTP-Unicast</Stream><Transport xmlns=\"http://www.onvif.org/ver10/schema\"><Protocol>UDP</Protocol></Transport></StreamSetup><ProfileToken>Profile1</ProfileToken></GetStreamUri></s:Body></s:Envelope>";

    char *str1 = "http://";
    //char *str2 = "/onvif/media_service";
    char *str2 = "/onvif/device_service";
    //char *strUrl = "http://172.16.23.221/onvif/media_service";
    char strUrl[256] = {0};
    char Responsebuf[8192] = {0};
    //char mediaurl[256] = {0};
    int ret = -1;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));

    //strcat(strUrl, str1);
    //strcat(strUrl, cliIP);
    //strcat(strUrl, str2);
    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/onvif/device_service",
             cliIP);

    int res = send_post(strUrl, mediastr, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);
    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    ret = parse_ONVIF_XML(Responsebuf, mediaurl, E_XML_URI);
    return ret;
}

int handle_search_result(void *args)
{
	pthread_detach(pthread_self());
    int ret = -1;
	CAMERA_ADD_PARAM param;
	memset(&param, 0, sizeof(param));
	memcpy(&param, args, sizeof(param));
	
	pthread_mutex_lock(&g_add_mutex);
    DAEMON_CAMERA_INFO_S *camera = get_camera_by_uid(param.onvifuid);
    if (NULL == camera)
    {
        camera = get_camera_unused();
        if (NULL == camera)
        {
            DEBUG_ERROR("Get unused camera resource failed.\n");
			pthread_mutex_unlock(&g_add_mutex);
            return -1;
        }
        strncpy(camera->device.onvifuid, param.onvifuid, sizeof(camera->device.onvifuid) - 1);
		
    }
	pthread_mutex_unlock(&g_add_mutex);
    DEBUG_INFO("id:[%x], cliIP:[%s], onvifuid:[%s]\n", camera->dev_id, param.cliIP, camera->device.onvifuid);

#ifdef SUPPORT_UPLOADCLIP_AUTO
    ret = camera_update_state(camera, param.cliIP, param.ser_ip, CAMERA_ONLINE);
#else
    ret = camera_update_state(camera, param.cliIP, CAMERA_ONLINE);
#endif
    return ret;
}

int camera_search(char *wan_ip, char *lan_ip, int timecount)
{
    char bufrecv[2048] = {0};
    //char onvifuid[64] = {0};
    CAMERA_ADD_PARAM param;

    int ret = -1;
    int wansock = 0;
    int lansock = 0;
    fd_set rfd;
    struct timeval timeout;
    int nRecLen;
    struct sockaddr_in cli;
    char cliIP[32] = {0};
    timeout.tv_sec = timecount;
    timeout.tv_usec = 0;
	int i;
	pthread_t tid;

    //if (wan_ip == NULL || strlen(wan_ip) < 7)
    //  system("ifconfig eth2:33 down");
    wansock = send_onvif_search_send(wan_ip);
    //if (wan_ip == NULL || strlen(wan_ip) < 7)
    //  system("ifconfig eth2:33 192.168.33.1");
    lansock = send_onvif_search_send(lan_ip);
    if (wansock < 0 && lansock < 0)
        return -1;
    while (1)
    {
        FD_ZERO(&rfd);
        if (wansock >= 0)
            FD_SET(wansock, &rfd);
        if (lansock >= 0)
            FD_SET(lansock, &rfd);
        ret = select(wansock > lansock ? (wansock + 1) : (lansock + 1), &rfd, NULL, NULL, &timeout);
        if (ret < 0)
        {
            DEBUG_ERROR("select error\n");
            break;
        }
        else if (ret == 0)
        {
            //DEBUG_ERROR("waiting timeout\n");
            break;
        }
        else
        {
            if (wansock > 0 && FD_ISSET(wansock, &rfd))
            {
                nRecLen = sizeof(cli);
                memset(&cli, 0, nRecLen);
                memset(cliIP, 0, sizeof(cliIP));
                memset(bufrecv, 0, sizeof(bufrecv));
                int nRecEcho = recvfrom(wansock, bufrecv, sizeof(bufrecv), 0, (struct sockaddr*)&cli, &nRecLen);
                if (nRecEcho < 0)
                {
                    DEBUG_ERROR("wan socket recvfrom error\n");
                    close(wansock);
                    wansock = -1;
                    goto error;
                }
                else if (nRecEcho > 0)
                {
                    
					memset(&param, 0, sizeof(param));                    
                    if (0 == parse_ONVIF_XML(bufrecv, param.onvifuid, E_XML_ADDRESS))
                    {
                        inet_ntop(AF_INET, &cli.sin_addr, cliIP, sizeof(cliIP));
						
						memcpy(param.cliIP, cliIP, sizeof(param.cliIP));
						memcpy(param.ser_ip, wan_ip, sizeof(param.ser_ip));
					    ret = camera_start_thread(&tid, 0, handle_search_result, (void*)&param);
						sleep(2);
					    if (ret != 0)
					    {
					        DEBUG_ERROR("Start thread camera_clip_task failed.\n");
					        goto error;
					    }
                    }
                }
            }
            if (lansock > 0 && FD_ISSET(lansock, &rfd))
            {
                nRecLen = sizeof(cli);
                memset(&cli, 0, nRecLen);
                memset(cliIP, 0, sizeof(cliIP));
                memset(bufrecv, 0, sizeof(bufrecv));
                int nRecEcho = recvfrom(lansock, bufrecv, sizeof(bufrecv), 0, (struct sockaddr*)&cli, &nRecLen);
                if (nRecEcho < 0)
                {
                    DEBUG_ERROR("lan socket recvfrom error\n");
                    close(lansock);
                    lansock = -1;
                    goto error;
                }
                else if (nRecEcho > 0)
                {
                    memset(&param, 0, sizeof(param));
                    if (0 == parse_ONVIF_XML(bufrecv, param.onvifuid, E_XML_ADDRESS))
                    {
                        inet_ntop(AF_INET, &cli.sin_addr, cliIP, sizeof(cliIP));

						memcpy(param.cliIP, cliIP, sizeof(param.cliIP));
						memcpy(param.ser_ip, lan_ip, sizeof(param.ser_ip));

						ret = camera_start_thread(&tid, 0, handle_search_result, (void*)&param);
						sleep(2);
					    if (ret != 0)
					    {
					        DEBUG_ERROR("Start thread camera_clip_task failed.\n");
					        goto error;
					    }
                    }
                }
            }
        }
    }

error:
    if (wansock > 0)
    {
        close(wansock);
        wansock = -1;
    }
    if (lansock > 0)
    {
        close(lansock);
        lansock = -1;
    }

    DEBUG_INFO("camera_search\n");

    return NULL;
}

int send_motion_decetion_post(DAEMON_CAMERA_INFO_S *camera, int enable)
{
    char * part1 = "http://";
    char * part3 = "/cgi-bin/admin/setparam.cgi";
    char * part4 = "event_i0_enable=";
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));
    /*
        strcat(strUrl, part1);
        strcat(strUrl, camera->device.ipaddress);
        strcat(strUrl, part3);

        strcat(strUrl2, part4);
        if (enable == 1)
            strcat(strUrl2, "1");
        else {
            strcat(strUrl2, "0");
        }

    */
    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             camera->device.ipaddress);

    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i0_enable=%d&"
             "event_i2_enable=%d",
             enable,enable);
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    int res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);


    return 0;
}

int send_record_trigger_post(DAEMON_CAMERA_INFO_S *camera, char *clipname)
{

    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setvi.cgi",
             camera->device.ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "vi0=1(100)0&filename=%s", clipname);
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    int res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    return 0;
}

int send_factory_default_post(DAEMON_CAMERA_INFO_S *camera)
{
    //http://172.16.74.58/cgi-bin/admin/setparam.cgi?restore_default_all=&system_restore=1

    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             camera->device.ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "restore_default_all=&system_restore=1");
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    int res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    return 0;
}

int send_sdcard_format_get(DAEMON_CAMERA_INFO_S *camera)
{
    //http://172.16.23.220/cgi-bin/admin/formatSD.cgi?

    char strUrl[512] = {0};
    char Responsebuf[1024] = {0};

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/formatSD.cgi?",
             camera->device.ipaddress);

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);

    int res = send_get(strUrl, Responsebuf);
    DEBUG_INFO("send_get res = %d\n", res);

    return 0;
}

int send_time_get(char *ip)
{
    //http://172.16.23.220/cgi-bin/admin/setparam.cgi?system_datetime=062909262015.36&method=manu
    int res = 0;
    char strUrl[512] = {0};
    char Responsebuf[1024] = {0};
    time_t tmv;
    struct tm *ptm;
    char datetime[64] = {0};

    int ghour, lhour;
    int gmin, lmin, diff_min;
    int timezone_val = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));

    time(&tmv);

    ptm = gmtime(&tmv);
    if (NULL == ptm)
    {
        DEBUG_INFO("gmtime failed, errno:%d\n", errno);
        return -1;
    }
    ghour = ptm->tm_hour;
    gmin = ptm->tm_min;

    ptm = localtime(&tmv);
    if (NULL == ptm)
    {
        DEBUG_INFO("localtime failed, errno:%d\n", errno);
        return -1;
    }
    lhour = ptm->tm_hour;
    lmin = ptm->tm_min;

    DEBUG_INFO("G: %d %d, L: %d %d\n", ghour, gmin, lhour, lmin);

    //month day hour minitue year . second
    sprintf(datetime, "%02d%02d%02d%02d%d.%02d",
            ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min,
            ptm->tm_year + 1900, ptm->tm_sec);

    diff_min = lmin - gmin;
    if (diff_min == 15 || diff_min == 30 || diff_min == 45 ||
            diff_min == -15 || diff_min == -30 || diff_min == -45)
        timezone_val = ((lhour - ghour) * 40) + (((lmin - gmin) / 3) * 2);
    else
        timezone_val = ((lhour - ghour) * 40);

    DEBUG_INFO("datetime: %s, timezone_val:%d\n", datetime, timezone_val);

    posttimeout = 10;

    // timezone
    memset(strUrl, 0, sizeof(strUrl));
    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi?system_timezoneindex=%d",
             ip, timezone_val);


    DEBUG_INFO("strUrl: %s\n", strUrl);

    res = send_get(strUrl, Responsebuf);
    DEBUG_INFO("send_get res = %d\n", res);

    sleep(5);

    // datetime
    memset(strUrl, 0, sizeof(strUrl));
    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi?system_datetime=%s&method=manu",
             ip, datetime);

    DEBUG_INFO("strUrl: %s\n", strUrl);

    res = send_get(strUrl, Responsebuf);
    DEBUG_INFO("send_get res = %d\n", res);


    return 0;
}

int send_record_polling_post(DAEMON_CAMERA_INFO_S *camera)
{
    int ret = -1;
    char * part1 = "http://";
    char * part3 = "/cgi-bin/admin/lsctrl.cgi";
    char * part4 = "cmd=search&triggerTime=%27";
    char * part5 = "'+TO+'2035-12-31 23:59:59'";
    char * part6 = "%2000:00:00%27+TO+%272035-12-31%2023:59:59%27";
    char timestr[32] = {0};
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char record[256] = {0};
    char idstr[16] = {0};
    time_t tm;
    struct tm *p = NULL;
    HC_MSG_S msg;
	int res_code = 0;
#define CLIP_VAILD_GAP 180

    memset(pollingrecvbuf, 0, POLLINGBUFLEN);
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));
    memset(timestr, 0, sizeof(timestr));

    tm = camera->recordtick + 1;

    //tm = time(NULL);
    p = gmtime(&tm);
    p = localtime(&tm);
    snprintf(timestr, sizeof(timestr), "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    /*
        strcat(strUrl, part1);
        strcat(strUrl, camera->device.ipaddress);
        strcat(strUrl, part3);

        strcat(strUrl2, part4);
        strcat(strUrl2, timestr);
        strcat(strUrl2, part6);
    */
    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/lsctrl.cgi",
             camera->device.ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "%s%s%s", part4, timestr, part6);

    posttimeout = 10;

    ret = send_post(strUrl, strUrl2, pollingrecvbuf, &res_code) ;
    if (ret != 0 || res_code != 200 || strlen(pollingrecvbuf) < 2)
    {
        DEBUG_INFO("ret = %d, res_code = %d, pollingrecvbuf = %s\n", ret, res_code, pollingrecvbuf);
        return -1;
    }
    camera->newclip = 0;
    camera->pollingtime = time(NULL);
    memset(camera->device.destPath, 0, sizeof(camera->device.destPath));
    pthread_mutex_lock(&g_mutex); //2
    parse_ONVIF_XML(pollingrecvbuf, (char *)camera, E_XML_CLIP);
    pthread_mutex_unlock(&g_mutex); //2

    if (camera->newclip > 0)
    {
        //send signal to clip thread for clip get.
        pthread_mutex_lock(&g_mutex); //2
        pthread_mutex_unlock(&g_mutex);
        pthread_cond_signal(&g_cond);
        DEBUG_INFO("Send trigger event, camera->pollingtime = %d, camera->recordtick = %d\n", camera->pollingtime, camera->recordtick);

        if (camera->newclip == 1 /*&& (camera->pollingtime - camera->recordtick) < CLIP_VAILD_GAP*/)
        {
            //send trigger EVENT signal to APP.
            memset(&msg, 0, sizeof(HC_MSG_S));
            memset(idstr, 0, sizeof(idstr));
            sprintf(idstr, "%d", camera->dev_id);
            DEBUG_INFO("Send trigger event, camera->dev_id = %x, camera->device.destPath = %s\n", camera->dev_id, camera->device.destPath);
            if (NULL == strstr(camera->device.destPath, idstr))
            {
                msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                camera_send_msg_to_app(&msg, camera);
            }
            else
            {
            }
            memset(camera->device.destPath, 0, sizeof(camera->device.destPath));
        } /*
        else if ((camera->pollingtime - camera->recordtick) < CLIP_VAILD_GAP)
        {
            CAMERA_CLIP_FILE_S *tmp = NULL;
            char *p = NULL;
            time_t t;
            struct tm tmp_time;

            tmp = camera->clip;
            while (tmp != NULL)
            {
                p = strptime(tmp->triggertime, "%Y-%m-%d %H:%M:%S", &tmp_time);
                if (p == NULL)
                {

                }
                else
                {
                    t = mktime(&tmp_time);
                    if (camera->pollingtime > t && (camera->pollingtime - t) < CLIP_VAILD_GAP)
                    {
                        memset(&msg, 0, sizeof(HC_MSG_S));
                        memset(idstr, 0, sizeof(idstr));
                        sprintf(idstr, "%d", camera->dev_id);
                        DEBUG_INFO("Send trigger event, camera->dev_id = %d, camera->device.destPath = %s\n", camera->dev_id, camera->device.destPath);
                        DEBUG_INFO("Send trigger event, camera->pollingtime = %d, triggertime = %s\n", camera->pollingtime, tmp->triggertime);
                        if (NULL == strstr(camera->device.destPath, idstr))
                        {
                            msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                            camera_send_msg_to_app(&msg, camera);
                        }
                        else
                        {
                        }
                        memset(camera->device.destPath, 0, sizeof(camera->device.destPath));
                    }
                }
                tmp = tmp->next;
            }
        }
        */
        camera->clipcount += camera->newclip;
    }
    return 0;
}

int camera_check_config(char *ipaddress)
{
    char * part1 = "http://";
    char * part3 = "/cgi-bin/admin/getparam_cache.cgi";
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    FILE *fp = NULL;
    int res = -1;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             ipaddress);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    snprintf(strUrl2,
             sizeof(strUrl2),
             "server_i4_name");
#else
    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i2_name");
#endif
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);


    DEBUG_INFO("Response: [%s], g_config_version: [%s]\n", Responsebuf, g_config_version);
    if (strstr(Responsebuf, g_config_version) != NULL)
    {
        return 0;
    }

    return -1;
}

int camera_send_config(char *ipaddress)
{
    char * part1 = "http://";
    char * part3 = "/cgi-bin/admin/upload_backup.cgi";
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    FILE *fp = NULL;
    int res = -1;

    fp = fopen(CAMERA_CONFIG_FILE, "r");
    if (fp == NULL)
    {
        DEBUG_ERROR("Open file %s failed, errno is '%d'.\n", CAMERA_CONFIG_FILE, errno);
        return -1;
    }
    else
    {
        fclose(fp);
    }
    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/upload_backup.cgi",
             ipaddress);
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    res = send_file_post(strUrl, CAMERA_CONFIG_FILE, Responsebuf);
    if (res == 0 || res == 28)
    {
        DEBUG_INFO("send_post res = %d\n", res);
        return 0;
    }
    else
    {
        DEBUG_ERROR("camera_send_config return error,  res = %d\n", res);
        return -1;
    }

}

int camera_version_check(char *ipaddress)
{
    char * part1 = "http://";
    char * part3 = "/cgi-bin/admin/getparam_cache.cgi";
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    FILE *fp = NULL;
    int res = -1;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             ipaddress);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    snprintf(strUrl2,
             sizeof(strUrl2),
             "server_i4_name");
#else
    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i2_name");
#endif
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s], g_camera_version: [%s]\n", Responsebuf, g_camera_version);
    if (strstr(Responsebuf, g_camera_version) != NULL)
    {
        return 0;
    }

    return -1;
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
int camera_ftpserver_check(char *ipaddress, char *ftpipaddress)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
    char *pStart = NULL;
    char *pEnd = NULL;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "server_i0_ftp_address");
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "server_i0_ftp_address=") != NULL) 
    {
        pStart = Responsebuf;
        pStart += strlen("server_i0_ftp_address=");

        if (*pStart == '\'')
            pStart++;

        pEnd = pStart;
        while (*pEnd != '\'' && *pEnd != '\r' && *pEnd != '\n')
            pEnd++;
        *pEnd = '\0';
        DEBUG_INFO("server_i0_ftp_address: [%s],ftpipaddress :[%s]\n", pStart, ftpipaddress);
        if(0 == strcmp(pStart, ftpipaddress))
            return 0;
    }

    return -1;
}
#endif

int camera_get_vendor(char *ip, char *vendor, char *mac)
{
    FILE *fp = NULL;
    char str_line[512] = {0};
    char macaddr[32] = {0};
    char dummy[32] = {0};
    int found = 0;

    int num = 0;
    int type,flags;
    char getip[16] = {0};

    fp = fopen("/proc/net/arp", "r");
    if (NULL == fp)
    {
        DEBUG_ERROR("open /proc/net/arp failure, errno is %d.\n", errno);
        return -1;
    }

    /* Skip first line */
    fgets(str_line , sizeof(str_line) , fp);
    while(fgets(str_line , sizeof(str_line) , fp))
    {
        num = sscanf(str_line,"%s 0x%x 0x%x %s\n" , getip, &type , &flags, macaddr);
        if(num != 4)
        {
            break;
        }
        if (strcmp(ip, getip) == 0)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (found == 0)
    {
        DEBUG_ERROR("Get mac of %s from arp failed.\n", ip);
        return -1;
    }

    strcpy(mac, macaddr);

    // VIVOTEK mac prefix 00:02:d1
    if (0 == strncasecmp(macaddr, CAMERA_VENDOR_MAC_VIVOTEK, strlen(CAMERA_VENDOR_MAC_VIVOTEK)))
    {
        strcpy(vendor, CAMERA_VENROR_NAME);
    }
    else
    {
        strcpy(vendor, "Unkown");
        return -1;
    }

    return 0;
}

int camera_get_modelname(char *ipaddress, char *modelname)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
    char *pStart = NULL;
    char *pEnd = NULL;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "system_info_modelname");

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "system_info_modelname=") != NULL)
    {
        pStart = Responsebuf;
        pStart += strlen("system_info_modelname=");

        if (*pStart == '\'')
            pStart++;

        pEnd = pStart;
        while (*pEnd != '\'' && *pEnd != '\r' && *pEnd != '\n')
            pEnd++;
        *pEnd = '\0';

        strcpy(modelname, pStart);

        DEBUG_INFO("modelname: [%s]\n", modelname);

        return 0;
    }

    return -1;
}

int camera_get_response(DAEMON_CAMERA_INFO_S *camera)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
	int res_code = 0;
    if(0 == strlen(camera->device.ipaddress))
        return -1;
    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             camera->device.ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "system_info_modelname");
    posttimeout = 10;
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    if (strstr(Responsebuf, "system_info_modelname=") != NULL)
    {
        camera->pollingtime = time(NULL);
        return 0;
    }

    return -1;
}

int camera_get_humidity(char *ipaddress, double *humidity)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
    char *pStart = NULL;
    char *pEnd = NULL;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/sensor.cgi",
             ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "humidity");

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    if (res == CURLE_OK)
    {
        DEBUG_INFO("Response: [%s]\n", Responsebuf);
        *humidity = atof(Responsebuf);
        DEBUG_INFO("Response: [%s] end\n", Responsebuf);
        return 0;
    }
    return -1;
}

int camera_get_temperature(char *ipaddress, double *temperature)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
    char *pStart = NULL;
    char *pEnd = NULL;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/sensor.cgi",
             ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "temperature");

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    if (res == CURLE_OK)
    {
        DEBUG_INFO("Response: [%s]\n", Responsebuf);
        *temperature = atof(Responsebuf);
        DEBUG_INFO("Response: [%s] end\n", Responsebuf);
        return 0;
    }

    return -1;
}

int camera_get_fwversion(char *ipaddress, char *fwversion)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
    char *pStart = NULL;
    char *pEnd = NULL;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "system_info_firmwareversion");

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "system_info_firmwareversion=") != NULL)
    {
        pStart = Responsebuf;
        pStart += strlen("system_info_firmwareversion=");

        if (*pStart == '\'')
            pStart++;

        pEnd = pStart;
        while (*pEnd != '\'' && *pEnd != '\r' && *pEnd != '\n')
            pEnd++;
        *pEnd = '\0';

        strcpy(fwversion, pStart);

        DEBUG_INFO("modelname: [%s]\n", fwversion);

        return 0;
    }

    return -1;
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
int camera_get_serialnumber(char *ipaddress, char *serialnumber)
{
    char strUrl[256] = {0};
    char strUrl2[256] = {0};
    char Responsebuf[1024] = {0};
    int res = -1;
    char *pStart = NULL;
    char *pEnd = NULL;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/getparam_cache.cgi",
             ipaddress);
    snprintf(strUrl2,
             sizeof(strUrl2),
             "system_info_serialnumber");

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "system_info_serialnumber=") != NULL)
    {
        pStart = Responsebuf;
        pStart += strlen("system_info_serialnumber=");

        if (*pStart == '\'')
            pStart++;

        pEnd = pStart;
        while (*pEnd != '\'' && *pEnd != '\r' && *pEnd != '\n')
            pEnd++;
        *pEnd = '\0';

        strcpy(serialnumber, pStart);

        DEBUG_INFO("system_info_serialnumber: [%s]\n", serialnumber);

        return 0;
    }

    return -1;
}
#endif

#ifdef SUPPORT_UPLOADCLIP_AUTO
int camera_config_init(char *ip ,char *serIP)
#else
int camera_config_init(char *ip)
#endif
{
    int ret = 0;
    char modelname[64] = {0};
    char vendor[64] = {0};
    char mac[64] = {0};
#ifdef SUPPORT_UPLOADCLIP_AUTO
    char serialnumber[64] = {0};
#endif
    // get vendor
    ret = camera_get_vendor(ip, vendor, mac);
    if (ret != 0)
    {
        DEBUG_ERROR("Get camera[%s][%s] vendor failed.\n", mac, ip);
        return -1;
    }

    if (0 == strcmp(vendor, CAMERA_VENROR_NAME))
    {
        // get model name
        ret = camera_get_modelname(ip, modelname);
        if (ret != 0)
        {
            DEBUG_ERROR("Get camera[%s][%s][%s] modelname failed.\n", vendor, mac, ip);
            return -1;
        }
#ifdef SUPPORT_UPLOADCLIP_AUTO
        //get serialnumber
        ret = camera_get_serialnumber(ip, serialnumber);
        if (ret != 0)
        {
            DEBUG_ERROR("Get camera[%s] serialnumber failed.\n", serialnumber);
            return -1;
        }
#endif
        // IP8130W's firmware is same as IP8131W. They run same function except the hardware IR luminance and IR-cut.
        if (0 == strcmp(modelname, "IP8131W") || 0 == strcmp(modelname, "IP8130W"))
        {
        #ifdef SUPPORT_UPLOADCLIP_AUTO
            if (0 != vivo_IP8131W_init(ip, serIP, serialnumber))
        #else
            if (0 != vivo_IP8131W_init(ip))
        #endif
            {
                DEBUG_ERROR("Config camera[%s][%s] failed.\n", modelname, ip);
                return -1;
            }
        }
        else if (0 == strcmp(modelname, "IB8168"))
        {
        #ifdef SUPPORT_UPLOADCLIP_AUTO
            if (0 != vivo_IB8168_init(ip, serIP, serialnumber))
        #else
            if (0 != vivo_IB8168_init(ip))
        #endif
            {
                DEBUG_ERROR("Config camera[%s][%s] failed.\n", modelname, ip);
                return -1;
            }
        }
        else if (0 == strcmp(modelname, "WD8137-W") || 0 == strcmp(modelname, "IP8137-W") )
        {
            if (0 != vivo_WD8137W_init(ip, serIP, serialnumber))
            {
                DEBUG_ERROR("Config camera[%s][%s] failed.\n", modelname, ip);
                return -1;
            }
        }
        else
        {
        #ifdef SUPPORT_UPLOADCLIP_AUTO
            if (0 != vivo_default_init(ip, serIP, serialnumber))
        #else
            if (0 != vivo_default_init(ip))
        #endif
            {
                DEBUG_ERROR("Config camera[%s][%s] with default config failed.\n", modelname, ip);
                return -1;
            }
        }
    }
    else
    {
        DEBUG_ERROR("===== Unsupported camera[%s][%s][%s]. =====\n", vendor, mac, ip);
        return -1;
    }

    DEBUG_INFO("Initialize camera[%s][%s][%s][%s] configuration success.\n", vendor, modelname, mac, ip);

    return 0;
}

int polling_event(int searchcount)
{
    DAEMON_CAMERA_INFO_S *camera = NULL;
    time_t tm = 0;
    int i = 0;
    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        tm = time(NULL);
        if ((camera->state == CAMERA_ONLINE || camera->state == CAMERA_NULL) && camera->dev_id != 0)
        {
            if (-1 == check_camera_alarm_enable(camera->dev_id))
            {
                if (camera->searchtime + (CAMERAPOLLINGGAP * CAMERASEARCHCOUNT + 10) < tm)
                {
                    camera->recordtick = tm;
                    camera->searchtime = tm;
                    DEBUG_INFO("Searching is failure, do polling for %x\n", camera->dev_id);
                    #ifndef SUPPORT_UPLOADCLIP_AUTO
                    send_record_polling_post(camera);
                    #else
                    camera_get_response(camera);
                    #endif
                    if (camera->pollingtime + CAMERAOFFLINEGAP < tm)
                    {
                        camera->state = CAMERA_OFFLINE;
                        camera->device.connection = (int)CAMERA_OFFLINE;
                        memset(camera->device.destPath, 0, sizeof(camera->device.destPath));
                        //send state change event to APP
                        HC_MSG_S msg;

                        memset(&msg, 0, sizeof(HC_MSG_S));
                        msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                        camera->event_type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                        DEBUG_INFO("Send off line event, disarm camera->dev_id = %x\n", camera->dev_id);
                        camera_send_msg_to_app(&msg, camera);

                        //for camera humidity
                        if(1 == camera_has_humidity(camera))
                        {
                            DEBUG_INFO("Send off line event, humidity->dev_id = %0x\n", camera->humidity.dev_id);
                            HC_MSG_S msg_hum;
                            memset(&msg_hum, 0, sizeof(HC_MSG_S));
                            msg_hum.head.type = HC_EVENT_RESP_DEVICE_DISCONNECTED;
                            camera_sensor_send_msg_to_app(&msg_hum, camera, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);
                        }
                        if(1 == camera_has_temperature(camera))
                        {
                            DEBUG_INFO("Send off line event, temperature->dev_id = %0x\n", camera->temperature.dev_id);
                            HC_MSG_S msg_tem;
                            memset(&msg_tem, 0, sizeof(HC_MSG_S));
                            msg_tem.head.type = HC_EVENT_RESP_DEVICE_DISCONNECTED;
                            camera_sensor_send_msg_to_app(&msg_tem, camera, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);
                        }
                        ipcam_camera_list_changed();
                    }
                }

            }
            else
            {
                #ifndef SUPPORT_UPLOADCLIP_AUTO
                send_record_polling_post(camera);
                #else
                camera_get_response(camera);
                #endif
                if (camera->pollingtime + CAMERAOFFLINEGAP < tm && camera->searchtime + CAMERAOFFLINEGAP < tm)
                {
                    camera->state = CAMERA_OFFLINE;
                    camera->device.connection = (int)CAMERA_OFFLINE;
                    memset(camera->device.destPath, 0, sizeof(camera->device.destPath));
                    //send state change event to APP
                    HC_MSG_S msg;
                    memset(&msg, 0, sizeof(HC_MSG_S));
                    msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                    camera->event_type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                    DEBUG_INFO("Send off line event, camera->dev_id = %x\n", camera->dev_id);
                    camera_send_msg_to_app(&msg, camera);

                    if(1 == camera_has_humidity(camera))
                    {
                        DEBUG_INFO("Send off line event, humidity->dev_id = %0x\n", camera->humidity.dev_id);
                        HC_MSG_S msg_hum;
                        memset(&msg_hum, 0, sizeof(HC_MSG_S));
                        msg_hum.head.type = HC_EVENT_RESP_DEVICE_DISCONNECTED;
                        camera_sensor_send_msg_to_app(&msg_hum, camera, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);
                    }
                    if(1 == camera_has_temperature(camera))
                    {
                        DEBUG_INFO("Send off line event, temperature->dev_id = %0x\n", camera->temperature.dev_id);
                        HC_MSG_S msg_tem;
                        memset(&msg_tem, 0, sizeof(HC_MSG_S));
                        msg_tem.head.type = HC_EVENT_RESP_DEVICE_DISCONNECTED;
                        camera_sensor_send_msg_to_app(&msg_tem, camera, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);
                    }

                    ipcam_camera_list_changed();
                }
            }
            if(camera->state == CAMERA_ONLINE && camera->dev_id != 0 && searchcount % 20 == 0)
            {
                if(1 == camera_has_humidity(camera))
                {
                    HC_MSG_S msg_hum;
                    memset(&msg_hum, 0, sizeof(HC_MSG_S));
                    msg_hum.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                    camera->event_type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                    if (0 != camera_get_humidity(camera->device.ipaddress, &camera->humidity.value))
                    {
                        DEBUG_ERROR("Get [%s][%s] humidity failed.\n", camera->dev_name, camera->device.ipaddress);
                        return -1;
                    }
                    DEBUG_INFO("camera->humidity.value %f\n", camera->humidity.value);
                    camera_sensor_send_msg_to_app(&msg_hum, camera, HC_MULTILEVEL_SENSOR_TYPE_HUMIDITY);
                }

                if(1 == camera_has_temperature(camera))
                {
                    HC_MSG_S msg_temp;
                    memset(&msg_temp, 0, sizeof(HC_MSG_S));
                    msg_temp.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                    camera->event_type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                    if (0 != camera_get_temperature(camera->device.ipaddress, &camera->temperature.value))
                    {
                        DEBUG_ERROR("Get [%s][%s] temperature failed.\n", camera->dev_name, camera->device.ipaddress);
                        return -1;
                    }
                    DEBUG_INFO("camera->temperature.value %f\n", camera->temperature.value);
                    camera_sensor_send_msg_to_app(&msg_temp, camera, HC_MULTILEVEL_SENSOR_TYPE_AIR_TEMPERATURE);
                }
            }
#ifdef WLAN_RECOVER_NETWORK
            if(camera->pollingtime + CAMERA_CHECK_NETWORK_TIME < tm && searchcount % 20 == 0)
            {
                /* Fail to do polling camera, check network status */
                camera_check_network(camera->dev_id);
            }
#endif
        }
    }
    return 0;
}

void ffmpeg_sig_child_func(int sig)
{
    int stat = 0;
    int child_pid = 0;

    /*
     * Reap the dead child processes,
     * call waitpid() will free the resources
     */
    do
    {
        child_pid = waitpid(-1, &stat, WNOHANG);
    } while (child_pid > 0);
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
int vsftpd_logfd_init(void)
{
    /* create FIFO file for internal communication */
    if (access(FIFO_PATHNAME_TO_CAMERA, W_OK) != 0 && mkfifo(FIFO_PATHNAME_TO_CAMERA, 0666) != 0)
    {
        return -1;
    }

    fifo_fd = open(FIFO_PATHNAME_TO_CAMERA, O_RDWR | O_NONBLOCK);
    if (fifo_fd == -1)
    {
        return -1;
    }
    return 0;
}

int vsftpd_logfd_uninit(void)
{
    if(-1 != fifo_fd)
    {
        close(fifo_fd);
    }
}

void ftp_message_handle_thread(void *arg)
{
    int ret = 0;
    int n = 0;
    fd_set readFds;
    int res;
    char fifo_buf[sizeof(VSF_FIFO_MSG)];
    char *buffer;
    VSF_FIFO_MSG *vsftpd_message;

    DAEMON_CAMERA_INFO_S *camera = NULL;
    CAMERA_CLIP_FILE_S *clipfile = NULL;
    HC_MSG_S msg;
    char idstr[32] = {0};

    //for trigger time
    char timestr[64] = {0};
    time_t tm;
    struct tm *p = NULL;
    //for trigger time

    FD_ZERO(&readFds);
    FD_SET(fifo_fd, &readFds);

    /* set up all the fd stuff for select */

    while (camera_keep_looping == 1)
    {
        ret = select(fifo_fd+1, &readFds, NULL, NULL, NULL);
        if (ret <= 0) continue;

        if (FD_ISSET(fifo_fd, &readFds) == 0)
            continue;

        do {
            memset(fifo_buf, 0, sizeof(fifo_buf));
            res = read(fifo_fd, fifo_buf, sizeof(VSF_FIFO_MSG));
            if (res > 0)
            {
                pthread_cond_signal(&g_cond);
                vsftpd_message = (VSF_FIFO_MSG*)fifo_buf;
                camera = get_camera_by_ip(vsftpd_message->ipaddress);
                if(camera != NULL )
                {
                    memset(idstr, 0, sizeof(idstr));
                    sprintf(idstr, "%d", camera->dev_id);
                    //there will create some log file in vsftpd local_root like evt20151230_024301.log
                    //we only send messsage when received a mp4 file
                    if(vsftpd_message->succeeded && NULL == strstr(vsftpd_message->clips_name, idstr) 
                        && NULL != strstr(vsftpd_message->clips_name, ".mp4") && camera->state == CAMERA_ONLINE
                        && 0 == enable_add_tick)
                    {
                        msg.head.type = HC_EVENT_STATUS_DEVICE_STATUS_CHANGED;
                        DEBUG_INFO("the uuid of the found camera is %s\n",camera->device.onvifuid);
                        DEBUG_INFO("vsftpd_message %s", vsftpd_message->clips_name);

                        //FOR TRIGGER
                        tm = time(NULL);
                        p = gmtime(&tm);
                        p = localtime(&tm);
                        snprintf(timestr, sizeof(timestr), "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
                        pthread_mutex_lock(&g_task_mutex);
                        memset(camera->device.triggerTime,0,sizeof(camera->device.triggerTime));
                        memcpy(camera->device.triggerTime, timestr, strlen(timestr));
                        //FOR TRIGGER
                        memset(camera->device.destPath,0,sizeof(camera->device.destPath));
                        memcpy(camera->device.destPath, vsftpd_message->clips_name, strlen(vsftpd_message->clips_name));

                        pthread_mutex_unlock(&g_task_mutex);
                        ret = camera_send_msg_to_app(&msg, camera);
                        if( ret < 0)
                        {
                            DEBUG_ERROR("camera send message failed ! \n");
                        }

                    }
                    else if(1 != vsftpd_message->succeeded)
                    {
                        DEBUG_ERROR("vsftpd camera upload video failed!\n");
                    }

                }
                else
                {
                    DEBUG_ERROR("vsftpd can not find camera %s\n", vsftpd_message->ipaddress);
                }
            }
        } while (res > 0);

    }

    return NULL;
}
#endif
