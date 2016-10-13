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
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "capi_spiderx.h"
#include "hcapi.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "hc_common.h"

#include "camera_util.h"
#include "ipcam_upgrade.h"
#include "ipcam_https.h"


int g_child_pid = -1;
int g_ipcam_use_ssl = 0; // 0 no ssl 1 use ssl

IPCAM_UPGRADE_CB_S g_ipcam_upgrade; // camera upgrade global data

IPCAM_UPGRADE_STATUS_S g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_MAX] =
{
    {"0", "0 - Success"},
    {"1", "1 - Upgrade Failed"},
    {"2", "2 - Open Local Firmware Failed"},
    {"3", "3 - Read Local Firmware Failed"},
    {"10", "10 - Socket Initialization Failed"},
    {"11", "11 - TCP Connect Failed"},
    {"12", "12 - SSL Connect Failed"},
    {"13", "13 - Authentication Failed"},
    {"14", "14 - Error On Read"},
    {"15", "15 - Error On Send"},
    {"20", "20 - Device Offline"},
    {"21", "21 - Get Model Type Failed"},
    {"22", "22 - Get Version Info Failed"},
    {"23", "23 - Get Model Type And Version Info Failed"},
    {"24", "24 - Get Config File Failed"},
    {"30", "30 - Device To Be Upgraded"},
    {"40", "40 - MAC Addr Mismatch"}
};

/* Shared memory to store ipcam upgrade result info */
IPCAM_UPGRADE_SHM_DATA_S * p_ipcam_upgrade_shm_data = NULL;

#define IPCAM_FW_VERSION_KEY   "system_info_firmwareversion="

/*
system_info_modelname='IP8131W'
system_info_extendedmodelname='IP8131W'
system_info_serialnumber='0002D1322F7D'
system_info_firmwareversion='IP8131-VVTK-0100e7'
 */
#define GET_STATE_CGI  \
    "GET /cgi-bin/admin/getparam_cache.cgi?system_info_modelname&system_info_firmwareversion HTTP/1.1\r\n"\
    "Host: %s\r\n"\
    "Accept: text/html\r\n"\
    "Accept-Encoding: gzip, deflate\r\n"\
    "Authorization: Basic %s\r\n"\
    "Connection: keep-alive\r\n\r\n"


#define HTTP_BOUNDARY "---------------------------7d79c2e420538"
#define HTTP_UPLOAD_FILE_NAME "ipcam.bin"
#define HTTP_UPGRADE_CGI                    \
 "POST /cgi-bin/admin/upgrade.cgi HTTP/1.1\r\n"              \
 "Host: %s\r\n"                    \
 "Accept: */*\r\n"                   \
 "Accept-Encoding: gzip, deflate\r\n"              \
 "User-Agent: UEG IPCAM CTRL\r\n"     \
 "Content-Length: %lu\r\n"                 \
 "Content-Type: multipart/form-data; boundary="HTTP_BOUNDARY"\r\n"       \
 "Authorization: Basic %s\r\n"                \
 "Connection: keep-alive\r\n\r\n"
#define HTTP_MULTI_HEAD                   \
 HTTP_BOUNDARY"\r\n"                   \
 "Content-Disposition: form-data; name=\"upfilename\" filename=\""HTTP_UPLOAD_FILE_NAME"\"\r\n" \
 "Content-Type: application/octet-stream\r\n\r\n"
#define HTTP_MULTI_TAIL "\r\n"HTTP_BOUNDARY"--\r\n"


char * convert_state_to_string(IPCAM_UPGRADE_STATE_E state)
{
    switch (state)
    {
        case IPCAM_UPGRADE_IDLE:
            return "UPGRADE_IDLE";
            
        case IPCAM_UPGRADE_WAIT_IMAGE:
            return "UPGRADE_WAIT_IMAGE";
            
        case IPCAM_UPGRADE_RUNNING:
            return "UPGRADE_RUNNING";
            
        default:
            return "Unknown State";
    }
}

CTRL_EXTERNAL_DEVICE_IPCAM_TYPE convert_modelname_to_type(const char * modelname)
{
    if (NULL == modelname)
        return EXTERNAL_DEVICE_IPCAM_MAX_NUM;

    if (0 == strcmp(IPCAM_MODEL_K10, modelname))
        return EXTERNAL_DEVICE_IPCAM_K10;
    else if (0 == strcmp(IPCAM_MODEL_IP8130W, modelname))
        return EXTERNAL_DEVICE_IPCAM_IP8130W;
    else if (0 == strcmp(IPCAM_MODEL_IP8131W, modelname))
        return EXTERNAL_DEVICE_IPCAM_IP8131W;
    else if (0 == strcmp(IPCAM_MODEL_IB8168, modelname))
        return EXTERNAL_DEVICE_IPCAM_IB8168;
    else if (0 == strcmp(IPCAM_MODEL_FD8168, modelname))
        return EXTERNAL_DEVICE_IPCAM_FD8168;
	else if (0 == strcmp(IPCAM_MODEL_IP8137W, modelname))
        return EXTERNAL_DEVICE_IPCAM_IP8137W;	
	else if (0 == strcmp(IPCAM_MODEL_WD8137W, modelname))
        return EXTERNAL_DEVICE_IPCAM_WD8137W;
    else
        return EXTERNAL_DEVICE_IPCAM_MAX_NUM;
}

char * convert_type_to_modelname(CTRL_EXTERNAL_DEVICE_IPCAM_TYPE ipcam_type)
{
    switch (ipcam_type)
    {
        case EXTERNAL_DEVICE_IPCAM_K10:
            return IPCAM_MODEL_K10;
        case EXTERNAL_DEVICE_IPCAM_IP8130W:
            return IPCAM_MODEL_IP8130W;
        case EXTERNAL_DEVICE_IPCAM_IP8131W:
            return IPCAM_MODEL_IP8131W;
        case EXTERNAL_DEVICE_IPCAM_IB8168:
            return IPCAM_MODEL_IB8168;
        case EXTERNAL_DEVICE_IPCAM_FD8168:
            return IPCAM_MODEL_FD8168;
		case EXTERNAL_DEVICE_IPCAM_IP8137W:
			return IPCAM_MODEL_IP8137W;			
		case EXTERNAL_DEVICE_IPCAM_WD8137W:
			return IPCAM_MODEL_WD8137W;
        default:
            return NULL;
    }
}

IPCAM_RESULT_E check_device_connect_by_lan(char *ip)
{
    char lan_mask[IP_ADDR_LEN];
    char lan_ip[IP_ADDR_LEN];
    struct in_addr lan_mask_n;
    struct in_addr lan_ip_n;
    struct in_addr ip_n;

    memset(lan_mask, 0, sizeof(lan_mask));
    memset(lan_ip, 0, sizeof(lan_ip));

    if (CAPI_SUCCESS != capi_get_runtime_lan_netmask(lan_mask, IP_ADDR_LEN)
            || CAPI_SUCCESS != capi_get_runtime_lan_ip(lan_ip, IP_ADDR_LEN))
    {
        return IPCAM_FAILED;
    }

    inet_aton(lan_mask, &lan_mask_n);
    inet_aton(lan_ip, &lan_ip_n);
    inet_aton(ip, &ip_n);

    if ((lan_ip_n.s_addr & lan_mask_n.s_addr)
            == (ip_n.s_addr & lan_mask_n.s_addr))
        return IPCAM_SUCCESS;
    else
        return IPCAM_FAILED;
}

void set_upgrade_state(int upgrade_state)
{
    g_ipcam_upgrade.ipcam_upgrade_state = upgrade_state;
}

void set_timer_wait_image(int enable)
{
    if (enable)
    {
        g_ipcam_upgrade.wait_image_flag = 1;
        g_ipcam_upgrade.wait_image_timeout = IPCAM_WAIT_IMAGE_TIMER_INTERVAL;
    }
    else
    {
        g_ipcam_upgrade.wait_image_flag = 0;
        g_ipcam_upgrade.wait_image_timeout = 0;
    }
}

int ipcam_shm_data_init(void)
{
    p_ipcam_upgrade_shm_data = (IPCAM_UPGRADE_SHM_DATA_S *)mmap(NULL, sizeof(IPCAM_UPGRADE_SHM_DATA_S),
                            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p_ipcam_upgrade_shm_data == MAP_FAILED)
    {
        p_ipcam_upgrade_shm_data = NULL;
        return -1;
    }
    memset(p_ipcam_upgrade_shm_data, 0, sizeof(IPCAM_UPGRADE_SHM_DATA_S));
    return 0;
}

void ipcam_signal_handler(int signum)
{
    int pid = 0;
    int stat = 0;

    switch (signum)
    {
        case SIGINT:
        case SIGTERM:
            break;

        case SIGCHLD:
            while (1)
            {
                pid = waitpid(-1, &stat, WNOHANG);
                if (pid <= 0)
                {
                    break;
                }

                if (pid == g_child_pid)
                {
                    g_child_pid = -1;
                }
            }
            break;
    }
    return;
}

void ipcam_upgrade_cb_update(void)
{
    int i, j;
    IPCAM_DEVICE_INFO_S *ipcam = NULL;
    DAEMON_CAMERA_INFO_S *camera = NULL;
    CTRL_EXTERNAL_DEVICE_IPCAM_TYPE device_type;

    memset(g_ipcam_upgrade.ipcam_device, 0, sizeof(g_ipcam_upgrade.ipcam_device));
    memset(g_ipcam_upgrade.need_upgrade_count, 0, sizeof(g_ipcam_upgrade.need_upgrade_count));

    // initialize ipcam device table from global camerainfo.
    for (i = 0; i < CAMERACOUNT; i++)
    {
        camera = &camerainfo[i];
        ipcam = &g_ipcam_upgrade.ipcam_device[i];
        
        if (0 == camera->dev_id)
            continue;
        
        ipcam->dev_id = camera->dev_id;
        ipcam->connection = camera->device.connection;
        strncpy(ipcam->ipaddress, camera->device.ipaddress, sizeof(ipcam->ipaddress) - 1);
        strncpy(ipcam->mac, camera->device.mac, sizeof(ipcam->mac) - 1);
        strncpy(ipcam->fwversion, camera->device.fwversion, sizeof(ipcam->fwversion) - 1);
        strncpy(ipcam->modelname, camera->device.modelname, sizeof(ipcam->modelname) - 1);
    }
    
    for (i = 0; i < CAMERACOUNT; i++)
    {
        ipcam = &g_ipcam_upgrade.ipcam_device[i];

        if (0 == ipcam->dev_id)
            continue;

        if (1 != ipcam->connection)
            continue;

        // check fwversion, modelname
        if (strlen(ipcam->fwversion) == 0 && strlen(ipcam->modelname) > 0)
        {
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_CHECK_VERSION_FAIL].code_str);
            continue;
        }
        else if (strlen(ipcam->modelname) == 0 && strlen(ipcam->fwversion) > 0)
        {
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_CHECK_MODEL_FAIL].code_str);
            continue;
        }
        else if (strlen(ipcam->modelname) == 0 && strlen(ipcam->fwversion) == 0)
        {
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_CHECK_VERSION_AND_MODEL_FAIL].code_str);
            continue;
        }

        device_type = convert_modelname_to_type(ipcam->modelname);
        if ( EXTERNAL_DEVICE_IPCAM_MAX_NUM == device_type)
        {
            DEBUG_ERROR("Convert modelname[%s] failed, should be unsupported modelname.", ipcam->modelname);
            continue;
        }

        if (0 != strcmp(g_ipcam_upgrade.ipcam_firmware_version_list[device_type], ipcam->fwversion))
        {
            g_ipcam_upgrade.need_upgrade_count[device_type]++;
        }
        
    }

    for (i = 0; i < EXTERNAL_DEVICE_IPCAM_MAX_NUM; i++)
    {
        DEBUG_INFO("type[%s] need upgrade count[%d].\n", 
                convert_type_to_modelname(i), g_ipcam_upgrade.need_upgrade_count[i]);
    }

    return;
}

void ipcam_upgrade_start(void)
{
    HC_DEVICE_EXT_CAMERA_FW_REQUEST_S upgrade_request;
    int i;
    SPIDERX_VERSION_INFO_S spiderx_version;
    int ret;

    // update ipcam firmware version
    memset(&spiderx_version, 0, sizeof(spiderx_version));
    ret = capi_get_spiderx_info(SPIDERX_INFO_TYPE_VERSION, &spiderx_version, sizeof(spiderx_version));
    if (CAPI_SUCCESS != ret)
    {
        DEBUG_ERROR("Retrying capi_get_spiderx_info ret[%d].\n", ret);
        sleep(5);
        ret = capi_get_spiderx_info(SPIDERX_INFO_TYPE_VERSION, &spiderx_version, sizeof(spiderx_version));
        if (ret != CAPI_SUCCESS)
        {
            DEBUG_ERROR("call capi_get_spiderx_info SPIDERX_INFO_TYPE_VERSION failed ret[%d].\n", ret);
            return;
        }
    }
    memcpy(g_ipcam_upgrade.ipcam_firmware_version_list, spiderx_version.device_ipcam_current_version,
           sizeof(g_ipcam_upgrade.ipcam_firmware_version_list));

    DEBUG_INFO("Now, Please take your attention, camera start to prepare upgrade.\n");
    for (i = 0; i < EXTERNAL_DEVICE_IPCAM_MAX_NUM ; i++)
    {
        DEBUG_INFO("Camera firmware: type[%d] model[%s] version[%s].\n", 
                i , convert_type_to_modelname(i), g_ipcam_upgrade.ipcam_firmware_version_list[i]);
    }

    // update g_ipcam_upgrade info.
    ipcam_upgrade_cb_update();

    
    for (i = 0; i < EXTERNAL_DEVICE_IPCAM_MAX_NUM; i++)
    {
        if (g_ipcam_upgrade.need_upgrade_count[i] > 0)
        {
            g_ipcam_upgrade.ipcam_upgrade_state = IPCAM_UPGRADE_WAIT_IMAGE;
            g_ipcam_upgrade.ipcam_device_type = i;
            break;
        }
    }
    if (i == EXTERNAL_DEVICE_IPCAM_MAX_NUM)
    {
        g_ipcam_upgrade.ipcam_upgrade_state = IPCAM_UPGRADE_IDLE;

        DEBUG_INFO("There's no camera need to upgrade.\n");

        return;
    }

    

    /*memset(&dla_status, 0, sizeof(dla_status));
    if (capi_get_dlc_info(DLC_INFO_TYPE_DLA_INFO, (void *)&dla_status, sizeof(dla_status)) == CAPI_SUCCESS)
    {
        if ((dla_status.dla_state == DLA_STATE_ARMED) || (dla_status.dla_state == DLA_STATE_ALARMING))
        {
            sys_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DLA's state[%d] isn't disarmed, upgrade stopped.\n", dla_status.dla_state);
            g_ipcam_upgrade.ipcam_upgrade_state = DLC_IPCAM_UPGRADE_STOP;

            ipcam_get_device_info();
            return;
        }
    }*/

    DEBUG_INFO("Camera upgrade run status change to [%s].\n", convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state));

    memset(&upgrade_request, 0, sizeof(upgrade_request));
    upgrade_request.device_type = g_ipcam_upgrade.ipcam_device_type;

    //ctrl_sendsignal_with_struct(g_ipcam_upgrade.conn, CTRL_COMMON_INTERFACE, CTRL_SIG_IPCAM_REQUEST_FIRMWARE_UPGRADE, (char *)&upgrade_request, sizeof(upgrade_request));
    ipcam_upgrade_send_msg(HC_EVENT_EXT_CAMERA_FW_REQUEST, HC_DEVICE_TYPE_EXT_CAMERA_FW_REQUEST, &upgrade_request, sizeof(upgrade_request));
    
    // set wait image timeout timer
    if (g_ipcam_upgrade.wait_image_flag == 0)
    {
        DEBUG_INFO("Camera wait for image. type[%d] model[%s].\n", upgrade_request.device_type, convert_type_to_modelname(upgrade_request.device_type));
        set_timer_wait_image(1);
    }
    return;


}

void ipcam_upgrade_end(void)
{
    HC_DEVICE_EXT_CAMERA_FW_REQUEST_S upgrade_request;
    HC_DEVICE_EXT_CAMERA_UPGRADE_DONE_S ipcam_upgrade_reply;
    int error_count = 0;
    //DLC_DLA_INFO_S dla_status;
    int i = 0, j = 0;

    memset(&upgrade_request, 0, sizeof(upgrade_request));
    memset(&ipcam_upgrade_reply, 0, sizeof(ipcam_upgrade_reply));

    if (g_ipcam_upgrade.ipcam_upgrade_state == IPCAM_UPGRADE_IDLE)
    {
        DEBUG_ERROR("Why in IDLE state running ipcam_upgrade_end?\n");
        return;
    }
    else if (g_ipcam_upgrade.ipcam_upgrade_state == IPCAM_UPGRADE_WAIT_IMAGE)
    {
        if (g_ipcam_upgrade.wait_image_flag == 1)
        {
            if (g_ipcam_upgrade.wait_image_timeout == 0)
            {
                DEBUG_INFO("camera upgrade state is %s and wait image timeout, change to IDLE.\n", 
                        convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state));
                set_upgrade_state(IPCAM_UPGRADE_IDLE);
            }
            else
            {
                DEBUG_ERROR("camera upgrade state is %s, wait image timer[%d] NOT timeout.\n", 
                        convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state), 
                        g_ipcam_upgrade.wait_image_timeout);
            }
        }
        else
        {
            DEBUG_ERROR("camera upgrade state[%s], but wait_image_flag[%d], wait_image_timeout[%d], why come here.\n", 
                    convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state),
                    g_ipcam_upgrade.wait_image_flag, g_ipcam_upgrade.wait_image_timeout);
            set_upgrade_state(IPCAM_UPGRADE_IDLE);
        }
    }
    else if (g_ipcam_upgrade.ipcam_upgrade_state == IPCAM_UPGRADE_RUNNING)
    {
        DEBUG_INFO("camera upgrade state is %s, need_upgrade_count[%d]=%d, error_count=%d\n", 
                        convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state), 
                        g_ipcam_upgrade.ipcam_device_type, 
                        g_ipcam_upgrade.need_upgrade_count[g_ipcam_upgrade.ipcam_device_type], 
                        p_ipcam_upgrade_shm_data->error_count);
    
        strncpy(ipcam_upgrade_reply.file_path, g_ipcam_upgrade.ipcam_firmware_path, sizeof(ipcam_upgrade_reply.file_path) - 1);
        
        if (g_ipcam_upgrade.need_upgrade_count[g_ipcam_upgrade.ipcam_device_type] == 0)
            remove(g_ipcam_upgrade.ipcam_firmware_path);

        error_count = p_ipcam_upgrade_shm_data->error_count;
        p_ipcam_upgrade_shm_data->result_unread = 0;

        ipcam_upgrade_reply.device_type = g_ipcam_upgrade.ipcam_device_type;
        if (error_count)
            ipcam_upgrade_reply.upgrade_status = IPCAM_FAILED;
        else
            ipcam_upgrade_reply.upgrade_status = IPCAM_SUCCESS;

        //ctrl_sendsignal_with_struct(g_ipcam_upgrade.conn, CTRL_COMMON_INTERFACE, CTRL_SIG_IPCAM_NOTIFY_FIRMWARE_UPGRADE_DONE, (char *)&ipcam_upgrade_reply, sizeof(ipcam_upgrade_reply));
        ipcam_upgrade_send_msg(HC_EVENT_EXT_CAMERA_FW_UPGRADE_DONE, HC_DEVICE_TYPE_EXT_CAMERA_UPGRADE_DONE, &ipcam_upgrade_reply, sizeof(ipcam_upgrade_reply));

        set_upgrade_state(IPCAM_UPGRADE_IDLE);
        
    }
    else 
    {
        DEBUG_INFO("camera upgrade state is %s, === bug fixme ===.\n", 
                        convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state));
        set_upgrade_state(IPCAM_UPGRADE_IDLE);
        return;
    }

    if (g_ipcam_upgrade.ipcam_upgrade_noticed || 
            g_ipcam_upgrade.ipcam_list_change_noticed)
    {
        DEBUG_INFO("There is noticed flag was set, so let's upgrade again.\n");

        /*Reset upgrade state */
        set_upgrade_state(IPCAM_UPGRADE_IDLE);

        // restart ipcam upgrade again
        ipcam_upgrade_start();

        if (IPCAM_UPGRADE_IDLE == g_ipcam_upgrade.ipcam_upgrade_state)
        {
            // no need to do upgrade, notify DLA that upgrade is done to trigger DISCOVERY
            //ctrl_sendsignal_with_struct(g_ipcam_upgrade.conn, CTRL_COMMON_INTERFACE, CTRL_SIG_IPCAM_NOTIFY_FIRMWARE_UPGRADE_DONE, NULL, 0);
            ipcam_upgrade_send_msg(HC_EVENT_EXT_CAMERA_FW_UPGRADE_DONE, HC_DEVICE_TYPE_EXT_CAMERA_UPGRADE_DONE, NULL, 0);
        }

        g_ipcam_upgrade.ipcam_upgrade_noticed = 0;
        g_ipcam_upgrade.ipcam_list_change_noticed = 0;

        return;
    }


}

int ipcam_upgrade_send_msg(int msg_type, int dev_type, void *data, int data_len)
{
    int ret = 0;
    HC_MSG_S msg;

    memset(&msg, 0, sizeof(HC_MSG_S));
    msg.head.src = DAEMON_CAMERA;
    msg.head.dst = APPLICATION;
    msg.head.type = msg_type;
    msg.body.device_info.event_type = msg_type;
    msg.body.device_info.dev_type = dev_type;
    if (data && data_len > 0)
    {
        memcpy(&msg.body.device_info.device, data, data_len);
    }
    
    ret = camera_msg_send(&msg);
    if (ret <= 0)
    {
        DEBUG_ERROR("Call msg_send failed, ret is %d.\n", ret);
        return -1;
    }

    return 0;
}

int ipcam_parse_cgi_data(char *data, char *cgi_response, char *pattern)
{
    if (data == NULL || cgi_response == NULL || pattern == NULL)
    {
        return -1;
    }

    char *pStart = NULL;
    char *pEnd = NULL;

    if ((pStart = strstr(cgi_response, pattern)) == NULL)
    {
        DEBUG_ERROR("match param failed, cgi_response=[%s], pattern=[%s].\n", cgi_response, pattern);
        return -1;
    }

    pStart += strlen(pattern);

    if (*pStart == '\'')
        pStart++;

    pEnd = pStart;
    while (*pEnd != '\'' && *pEnd != '\r' && *pEnd != '\n')
        pEnd++;
    *pEnd = '\0';

    memcpy(data, pStart, pEnd - pStart);

    DEBUG_INFO("data=[%s].\n", data);

    return 0;
}

void ipcam_upgrade_process(void)
{
    int i, n, ipcam_index;
    int fd[MAX_IPCAM_LIST_LEN];
    SSL *ssl[MAX_IPCAM_LIST_LEN];
    SSL_CTX *ctx[MAX_IPCAM_LIST_LEN];
    char serverName[MAX_IPCAM_LIST_LEN][HOST_LENGTH];
    char serverIp[MAX_IPCAM_LIST_LEN][IP_LENGTH];
    char hostinfo[MAX_IPCAM_LIST_LEN][HOST_LENGTH];
    int  serverPort[MAX_IPCAM_LIST_LEN];
    char httpPath[MAX_IPCAM_LIST_LEN][HTTP_PATH_LENGTH];
    int sslConn[MAX_IPCAM_LIST_LEN];
    int upgrade_flag[MAX_IPCAM_LIST_LEN];
    int upgrade_count = 0;
    int error_count = 0;
    char http_ath[MAX_IPCAM_LIST_LEN][MIDDLE_STR_LEN];
    char http_buf[SOCKET_BUFF_LEN];
    char http_response[SOCKET_BUFF_LEN];
    FILE *fp;
    struct stat st;
    IPCAM_DEVICE_INFO_S *ipcam = NULL;
    int max_fd = -1;
    fd_set fds, orig_fds, zero_fds;
    char version[MIDDLE_STR_LEN];
    int check_retry = 0;
    struct timeval timeout;
    int ret = 0;
    CTRL_EXTERNAL_DEVICE_IPCAM_TYPE ipcam_type;
    char *upgrade_modelname = NULL;

    if (g_ipcam_upgrade.ipcam_upgrade_state != IPCAM_UPGRADE_RUNNING)
    {

        DEBUG_ERROR("Camera current state isn't running, it's [%s]\n", convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state));
        error_count = 0xff;
        goto end_of_upgrade;
    }

    DEBUG_INFO("Camera start to upgrade:type[%d] model[%s] version[%s].\n", 
                g_ipcam_upgrade.ipcam_device_type, 
                convert_type_to_modelname(g_ipcam_upgrade.ipcam_device_type), 
                g_ipcam_upgrade.ipcam_firmware_version);

    upgrade_modelname = convert_type_to_modelname(g_ipcam_upgrade.ipcam_device_type);
    if (upgrade_modelname == NULL)
    {
        DEBUG_ERROR("Convert camera type[%d] to modelname failed.\n", g_ipcam_upgrade.ipcam_device_type);
        error_count = 0xff;
        goto end_of_upgrade;
    }

    ret = stat(g_ipcam_upgrade.ipcam_firmware_path, &st);
    if (ret < 0)
    {
        DEBUG_ERROR("Camera stat firmware file[%s] error.\n", g_ipcam_upgrade.ipcam_firmware_path);
        error_count = 0xff;
        goto end_of_upgrade;
    }

    memset(fd, -1, sizeof(fd));
    memset(ssl, 0, sizeof(ssl));
    memset(ctx, 0, sizeof(ctx));
    memset(serverName, 0, sizeof(serverName));
    memset(serverIp, 0, sizeof(serverIp));
    memset(hostinfo, 0, sizeof(hostinfo));
    memset(serverPort, 0, sizeof(serverPort));
    memset(httpPath, 0, sizeof(httpPath));
    memset(sslConn, 0, sizeof(sslConn));
    memset(http_ath, 0, sizeof(http_ath));
    memset(upgrade_flag, 0, sizeof(upgrade_flag));
    for (ipcam_index = 0; ipcam_index < CAMERACOUNT; ipcam_index++)
    {
        ipcam = &g_ipcam_upgrade.ipcam_device[ipcam_index];

        if (0 == ipcam->dev_id)
            continue;

        if (1 != ipcam->connection)
            continue;
        
        if (0 != strcmp(ipcam->modelname, upgrade_modelname))
            continue;

        // check if need to upgrade
        if (0 == strcmp(ipcam->fwversion, g_ipcam_upgrade.ipcam_firmware_version))
        {
            DEBUG_INFO("Camera no need upgrade, type[%d] model[%s]-[%s] ip[%s] mac[%s] version[%s]\n",
                              g_ipcam_upgrade.ipcam_device_type, 
                              convert_type_to_modelname(g_ipcam_upgrade.ipcam_device_type),
                              ipcam->modelname,
                              ipcam->ipaddress,
                              ipcam->mac, 
                              ipcam->fwversion);

            continue;
        }

        DEBUG_INFO("Camera need upgrade, type[%d] model[%s]-[%s] ip[%s] mac[%s] version[%s]->[%s]\n",
                              g_ipcam_upgrade.ipcam_device_type, 
                              convert_type_to_modelname(g_ipcam_upgrade.ipcam_device_type),
                              ipcam->modelname,
                              ipcam->ipaddress,
                              ipcam->mac, 
                              g_ipcam_upgrade.ipcam_firmware_version,
                              ipcam->fwversion);

        // Send HTTP header
        memset(http_buf, 0, sizeof(http_buf));
        snprintf(http_buf, sizeof(http_buf) - 1, "%s:%s",
                 "root", "root");
        ret = encode64(http_buf, http_ath[ipcam_index], userKeyStr);
        if (ret < 0)
        {
            // set result code to IPCAM_UPGRADE_ENCODE_FAILED
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_AUTH_FAIL].code_str);
            error_count++;
            continue;
        }

        memset(http_buf, 0, sizeof(http_buf));
        snprintf(http_buf, sizeof(http_buf) - 1, "%s:%u",
                 ipcam->ipaddress, 80);
        if (g_ipcam_use_ssl
                && (IPCAM_FAILED == check_device_connect_by_lan(ipcam->ipaddress)))
        {
            sslConn[ipcam_index] = 1;
            ret = init_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index],
                                      (void **) & ctx[ipcam_index], serverName[ipcam_index], serverIp[ipcam_index],
                                      hostinfo[ipcam_index], &serverPort[ipcam_index], httpPath[ipcam_index],
                                      &sslConn[ipcam_index], http_buf, NULL, IPPROTO_v4);
        }
        else
        {
            sslConn[ipcam_index] = 0;
            ret = init_session_socket(&fd[ipcam_index], NULL, NULL, serverName[ipcam_index],
                                      serverIp[ipcam_index], hostinfo[ipcam_index], &serverPort[ipcam_index],
                                      httpPath[ipcam_index], &sslConn[ipcam_index], http_buf,
                                      NULL, IPPROTO_v4);
        }
        if (ret != IPCAM_SUCCESS)
        {
            DEBUG_ERROR("Camera[%s][%s] init_session_socket return failed [%d] ssl flag[%d]\n",
                              ipcam->ipaddress, ipcam->mac, ret, sslConn[ipcam_index]);
            // set result code to IPCAM_UPGRADE_CONNECT_FAILED
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            if (ret == IPCAM_INIT_SOCKET_FAILED)
                strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_INIT_SOCKET_FAIL].code_str);
            else if (ret == IPCAM_TCP_FAILED)
                strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_TCP_CONNECT_FAIL].code_str);
            else if (ret == IPCAM_SSL_FAILED)
                strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_INIT_SSL_FAIL].code_str);
            else
                strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_FAIL].code_str);

            error_count++;
            continue;
        }

        memset(http_buf, 0, sizeof(http_buf));
        snprintf(http_buf, sizeof(http_buf) - 1,
                 HTTP_UPGRADE_CGI, serverName[ipcam_index],
                 st.st_size + strlen(HTTP_MULTI_HEAD) + strlen(HTTP_MULTI_TAIL),
                 http_ath[ipcam_index]);
        n = strlen(http_buf);
        if (sslConn[ipcam_index])
        {
            ret = send_raw(&fd[ipcam_index], (void **) & ssl[ipcam_index], http_buf, n , sslConn[ipcam_index]);
        }
        else
        {
            ret = send_raw(&fd[ipcam_index], NULL, http_buf, n , sslConn[ipcam_index]);
        }
        if (ret != IPCAM_SUCCESS)
        {
            destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
            fd[ipcam_index] = -1;
            DEBUG_ERROR("Camera[%s][%s] send HTTP_POST failed buflen[%d] ret[%d:%d:%s]\n",
                              ipcam->ipaddress, ipcam->mac, n, ret, errno, strerror(errno));

            // set result code to IPCAM_UPGRADE_SEND_FAILED
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_SEND_FAIL].code_str);
            error_count++;
            continue;
        }

        n = strlen(HTTP_MULTI_HEAD);
        if (sslConn[ipcam_index])
        {
            ret = send_raw(&fd[ipcam_index], (void **) & ssl[ipcam_index], HTTP_MULTI_HEAD, n , sslConn[ipcam_index]);
        }
        else
        {
            ret = send_raw(&fd[ipcam_index], NULL, HTTP_MULTI_HEAD, n , sslConn[ipcam_index]);
        }
        if (ret != IPCAM_SUCCESS)
        {
            destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
            fd[ipcam_index] = -1;
            DEBUG_ERROR("Camera[%s][%s] send HTTP_MULTI_HEAD failed buflen[%d] ret[%d:%d:%s]\n",
                              ipcam->ipaddress, ipcam->mac, n, ret, errno, strerror(errno));

            // set result code to IPCAM_UPGRADE_SEND_FAILED
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_SEND_FAIL].code_str);
            error_count++;
            continue;
        }

        // Send firmware
        fp = fopen(g_ipcam_upgrade.ipcam_firmware_path, "r+b");
        if (NULL == fp)
        {
            destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
            fd[ipcam_index] = -1;
            DEBUG_ERROR("Camera[%s][%s] fopen fw file[%s] failed ret[%d:%s]\n",
                              ipcam->ipaddress, ipcam->mac, 
                              g_ipcam_upgrade.ipcam_firmware_path, errno, strerror(errno));

            // set result code to IPCAM_UPGRADE_OPEN_FIRMWARE_FAILED
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_OPEN_FW_FAIL].code_str);
            error_count++;
            continue;
        }

        n = 0;
        do
        {
            i = fread(http_buf, 1, sizeof(http_buf), fp);
            if (i <= 0)
            {
                fclose(fp);
                destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
                fd[ipcam_index] = -1;
                DEBUG_ERROR("Camera[%s][%s] fread fw file[%s] failed ret[%d:%s]\n",
                              ipcam->ipaddress, ipcam->mac, 
                              g_ipcam_upgrade.ipcam_firmware_path, errno, strerror(errno));

                // set result code to IPCAM_UPGRADE_READ_FIRMWARE_FAILED
                memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
                strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_RD_FW_FAIL].code_str);
                error_count++;
                goto loop_end;
            }
            n += i;

            if (sslConn[ipcam_index])
            {
                ret = send_raw(&fd[ipcam_index], (void **) & ssl[ipcam_index], http_buf, i , sslConn[ipcam_index]);
            }
            else
            {
                ret = send_raw(&fd[ipcam_index], NULL, http_buf, i , sslConn[ipcam_index]);
            }

            if (ret != IPCAM_SUCCESS)
            {
                fclose(fp);
                destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
                fd[ipcam_index] = -1;
                DEBUG_ERROR("Camera[%s][%s] send_raw failed buflen[%d] ret[%d:%d:%s]\n",
                              ipcam->ipaddress, ipcam->mac, i, ret, errno, strerror(errno));

                // set result code to IPCAM_UPGRADE_SEND_FAILED
                memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
                strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_SEND_FAIL].code_str);
                error_count++;
                goto loop_end;
            }


            if (n >= st.st_size)
            {
                break;
            }
        }
        while (!feof(fp));
        fclose(fp);

        // Send HTTP tail
        n = strlen(HTTP_MULTI_TAIL);
        if (sslConn[ipcam_index])
        {
            ret = send_raw(&fd[ipcam_index], (void **) & ssl[ipcam_index], HTTP_MULTI_TAIL, n , sslConn[ipcam_index]);
        }
        else
        {
            ret = send_raw(&fd[ipcam_index], NULL, HTTP_MULTI_TAIL, n , sslConn[ipcam_index]);
        }
        if (ret != IPCAM_SUCCESS)
        {
            destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
            fd[ipcam_index] = -1;
            DEBUG_ERROR("Camera[%s][%s] send HTTP_MULTI_TAIL failed buflen[%d] ret[%d:%d:%s]\n",
                          ipcam->ipaddress, ipcam->mac, n, ret, errno, strerror(errno));

            // set result code to IPCAM_UPGRADE_SEND_FAILED
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_SEND_FAIL].code_str);
            error_count++;
            continue;
        }

        // post image success, set upgrade_flag[ipcam_index] wait to check result
        upgrade_flag[ipcam_index] = 1;
        upgrade_count ++;
        //destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
        //fd[ipcam_index] = -1;

loop_end:
        ;
    }

    if (0 == upgrade_count)
    {
        goto end_of_upgrade;
    }

    DEBUG_ERROR("Send image to IP cameras completely, wait IP cameras upgrading ...\n");
    
    sleep(IPCAM_UPGRADE_CHECK_START);

    /* vivotek camera, must not close socket when post image finished.
       or else ip camera do not upgrade. */
    for (ipcam_index = 0; ipcam_index < CAMERACOUNT; ipcam_index++)
    {
        if (fd[ipcam_index] > 0)
        {
            destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
            fd[ipcam_index] = -1;
        }
    }
    
    DEBUG_ERROR("Push image to IP cameras completely. Start to check upgrade result.upgrade count[%d].\n", upgrade_count);

    FD_ZERO(&zero_fds);
    do
    {
        max_fd = 0;
        upgrade_count = 0;
        FD_ZERO(&fds);
        FD_ZERO(&orig_fds);

        for (ipcam_index = 0; ipcam_index < CAMERACOUNT; ipcam_index++)
        {
            if (upgrade_flag[ipcam_index])
            {
                upgrade_count++;
                ipcam = &g_ipcam_upgrade.ipcam_device[ipcam_index];

                // connect ipcam
                memset(http_buf, 0, sizeof(http_buf));
                snprintf(http_buf, sizeof(http_buf) - 1, "%s:%u",
                         ipcam->ipaddress, 80);
                if (g_ipcam_use_ssl
                        && (IPCAM_FAILED == check_device_connect_by_lan(ipcam->ipaddress)))
                {
                    sslConn[ipcam_index] = 1;
                    ret = init_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index],
                                              (void **) & ctx[ipcam_index], serverName[ipcam_index], serverIp[ipcam_index],
                                              hostinfo[ipcam_index], &serverPort[ipcam_index], httpPath[ipcam_index],
                                              &sslConn[ipcam_index], http_buf, NULL, IPPROTO_v4);
                }
                else
                {
                    sslConn[ipcam_index] = 0;
                    ret = init_session_socket(&fd[ipcam_index], NULL, NULL, serverName[ipcam_index],
                                              serverIp[ipcam_index], hostinfo[ipcam_index], &serverPort[ipcam_index],
                                              httpPath[ipcam_index], &sslConn[ipcam_index], http_buf,
                                              NULL, IPPROTO_v4);
                }
                if (ret != IPCAM_SUCCESS)
                {
                    DEBUG_ERROR("Camera[%s][%s] init_session_socket return failed [%d] ssl flag[%d]\n",
                                  ipcam->ipaddress, ipcam->mac, ret, sslConn[ipcam_index]);
                    continue;
                }

                // get version info
                memset(http_buf, 0, sizeof(http_buf));
                snprintf(http_buf, sizeof(http_buf) - 1,
                         GET_STATE_CGI, serverName[ipcam_index], http_ath[ipcam_index]);
                n = strlen(http_buf);
                if (sslConn[ipcam_index])
                {
                    ret = send_raw(&fd[ipcam_index], (void **) & ssl[ipcam_index], http_buf, n , sslConn[ipcam_index]);
                }
                else
                {
                    ret = send_raw(&fd[ipcam_index], NULL, http_buf, n , sslConn[ipcam_index]);
                }
                if (ret != IPCAM_SUCCESS)
                {
                    destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
                    fd[ipcam_index] = -1;
                    DEBUG_ERROR("Camera[%s][%s] send HTTP_GET_SYSINFO failed buflen[%d] ret[%d:%d:%s]\n",
                                  ipcam->ipaddress, ipcam->mac, n, ret, errno, strerror(errno));
                    continue;
                }

                // add fd[ipcam_index] to orig_fds
                FD_SET(fd[ipcam_index], &orig_fds);
                max_fd = (max_fd > fd[ipcam_index]) ? max_fd : fd[ipcam_index];
            }
        }

        if (!upgrade_count)
        {
            DEBUG_INFO("There's no IP camera need check!\n");
            break;
        }

        DEBUG_INFO("After send get VERSION request, and wait for response. upgrade count[%d].\n", upgrade_count);


        // wait response for sysinfo
        timeout.tv_sec  = IPCAM_QUERY_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        do
        {
            if (!memcmp(&orig_fds, &zero_fds, sizeof(fd_set)))
            {
                break;
            }
            memcpy(&fds, &orig_fds, sizeof(fd_set));

            ret = select(max_fd + 1, &fds, NULL, NULL, &timeout);
            if (ret > 0)
            {
                for (ipcam_index = 0;  ipcam_index < CAMERACOUNT; ipcam_index++)
                {
                    if (fd[ipcam_index] <= 0 || !FD_ISSET(fd[ipcam_index], &fds))
                    {
                        continue;
                    }

                    ipcam = &g_ipcam_upgrade.ipcam_device[ipcam_index];

                    memset(http_response, 0, sizeof(http_response));
                    if (sslConn[ipcam_index])
                    {
                        ret = recv_raw(&fd[ipcam_index], (void **) & ssl[ipcam_index], http_response, sizeof(http_response), sslConn[ipcam_index], &n);
                    }
                    else
                    {
                        ret = recv_raw(&fd[ipcam_index], NULL, http_response, sizeof(http_response), sslConn[ipcam_index], &n);
                    }
                    if (IPCAM_SUCCESS != ret)
                    {
                        DEBUG_ERROR("Camera[%s][%s] recv sysinfo failed buflen[%d] ret[%d:%d:%s]\n",
                                          ipcam->ipaddress, ipcam->mac, sizeof(http_response), n, errno, strerror(errno));
                        continue;
                    }
                    else if (n == 0)
                    {
                        continue;
                    }

                    memset(version, 0, sizeof(version));
                    if ((ret = ipcam_parse_cgi_data(version, http_response, IPCAM_FW_VERSION_KEY)) != 0)
                    {
                        DEBUG_INFO("IPCam get firmware version failed.\n");
                        memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
                        strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_FAIL].code_str);
                        error_count++;
                    }
                    else
                    {
                        DEBUG_INFO("Cache ipcam_firmware_version:[%s], CGI version:[%s]\n", 
                                    g_ipcam_upgrade.ipcam_firmware_version, version);
                    
                        if (strcmp(g_ipcam_upgrade.ipcam_firmware_version, version))
                        {
                            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
                            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_FAIL].code_str);
                            error_count++;
                        }
                        else
                        {
                            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
                            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_SUCCESS].code_str);

                            g_ipcam_upgrade.need_upgrade_count[g_ipcam_upgrade.ipcam_device_type]--;

                            DEBUG_INFO("Camera type[%d][%s] need_upgrade_count[%d]\n", 
                                    g_ipcam_upgrade.ipcam_device_type, 
                                    convert_type_to_modelname(g_ipcam_upgrade.ipcam_device_type), 
                                    g_ipcam_upgrade.need_upgrade_count[g_ipcam_upgrade.ipcam_device_type]);

                        }

                        FD_CLR(fd[ipcam_index], &orig_fds);
                        upgrade_flag[ipcam_index] = 0;
                        upgrade_count --;
                    }


                    DEBUG_INFO("Camera fetch ipcam[%s][%s]'s newest version[%s].\n",
                                      ipcam->ipaddress, ipcam->mac, version);
                }
            }
            else if (ret < 0)
            {
                // error happened
                continue;
            }
        }
        while (timeout.tv_sec || timeout.tv_usec);

        if (!upgrade_count)
        {
            DEBUG_ERROR("Check upgrade result completely.\n");
            break;
        }

        if (++check_retry >= IPCAM_UPGRADE_CHECK_RETRY)
        {
            DEBUG_ERROR("IPCam reach max check retry times[%d]. Drop!\n", check_retry);
            break;
        }

        DEBUG_ERROR("There's some IP cameras[%d] check failed, retry[%d].\n", upgrade_count, check_retry);

        sleep(IPCAM_UPGRADE_CHECK_INTERVAL);
    }
    while (1);


    for (ipcam_index = 0; ipcam_index < CAMERACOUNT; ipcam_index++)
    {
        if (upgrade_flag[ipcam_index])
        {
            // upgrade_flag[ipcam_index] is not unset, not get upgrade result, set as failed
            ipcam = &g_ipcam_upgrade.ipcam_device[ipcam_index];
            memset(ipcam->result_code, 0, sizeof(ipcam->result_code));
            strcpy(ipcam->result_code, g_ipcam_upgrade_result_status[IPCAM_UPGRADE_STATUS_FAIL].code_str);
            error_count++;
        }

        if (fd[ipcam_index] > 0)
        {
            destroy_session_socket(&fd[ipcam_index], (void **) & ssl[ipcam_index], (void **) & ctx[ipcam_index], sslConn[ipcam_index]);
            fd[ipcam_index] = -1;
        }
    }


end_of_upgrade:
    p_ipcam_upgrade_shm_data->error_count = error_count;
    p_ipcam_upgrade_shm_data->result_unread = 1;
    return;
}

void ipcam_fork_to_upgrade(void)
{
    pid_t pid;

    memset(p_ipcam_upgrade_shm_data, 0, sizeof(IPCAM_UPGRADE_SHM_DATA_S));
    // fork a new process to do ipcam upgrade
    pid = fork();
    if (0 == pid)
    {
        /* Prevent unexpected application terminated due to write/read operation upon disconnected socket*/
        signal(SIGPIPE, SIG_IGN);

        ipcam_upgrade_process();
        exit(0);
    }
    else if (pid > 0)
    {
        g_child_pid = pid;
    }
    else
    {
        // set ipcam upgrade result failed
        p_ipcam_upgrade_shm_data->error_count = 0xff;
        p_ipcam_upgrade_shm_data->result_unread = 1;
        DEBUG_ERROR("Fail to fork process for camera upgrade\n");
    }

    return;
}

void ipcam_camera_list_changed(void)
{
    if (IPCAM_UPGRADE_IDLE == g_ipcam_upgrade.ipcam_upgrade_state)
    {
        ipcam_upgrade_start();
    }
    else
    {
        DEBUG_INFO("Camera is upgrading, set ipcam_list_change_noticed.\n");
        g_ipcam_upgrade.ipcam_list_change_noticed = 1;
    }
}

void ipcam_handle_timeout_event(void)
{ 
    if (--g_ipcam_upgrade.result_query_timeout <= 0)
    {
        g_ipcam_upgrade.result_query_timeout = IPCAM_RESULT_QUERY_TIMEOUT;
        if (p_ipcam_upgrade_shm_data->result_unread == 1)
        {
            p_ipcam_upgrade_shm_data->result_unread = 0;
            ipcam_upgrade_end();
        }
    }

    if (g_ipcam_upgrade.wait_image_flag)
    {
        if (--g_ipcam_upgrade.wait_image_timeout <= 0)
        {
            ipcam_upgrade_end();
            set_timer_wait_image(0);
        }
    }
}

void ipcam_handle_firmware_version_change_event(void)
{
    int i, ret = 0;
    SPIDERX_VERSION_INFO_S spiderx_version;
    // update ipcam firmware version
    memset(&spiderx_version, 0, sizeof(spiderx_version));
    ret = capi_get_spiderx_info(SPIDERX_INFO_TYPE_VERSION, &spiderx_version, sizeof(spiderx_version));
    if (CAPI_SUCCESS != ret)
    {
        DEBUG_ERROR("Retrying capi_get_spiderx_info ret[%d].\n", ret);
        sleep(5);
        ret = capi_get_spiderx_info(SPIDERX_INFO_TYPE_VERSION, &spiderx_version, sizeof(spiderx_version));
        if (CAPI_SUCCESS != ret)
        {
            DEBUG_ERROR("call capi_get_spiderx_info SPIDERX_INFO_TYPE_VERSION failed ret[%d].\n", ret);
            return;
        }
    }

    // debug
    for (i = 0; i < EXTERNAL_DEVICE_IPCAM_MAX_NUM; i++)
    {
#ifdef IPCAM_INTERNAL_DEBUG  
        sprintf(spiderx_version.device_ipcam_current_version[i], "TEST-%s-%d", 
                convert_type_to_modelname(i), time(NULL));
#endif

        DEBUG_INFO("Camera[%s]: New[%s] - Old[%s]\n", 
                convert_type_to_modelname(i), 
                spiderx_version.device_ipcam_current_version[i], 
                g_ipcam_upgrade.ipcam_firmware_version_list[i]);
    }

    if (0 == memcmp(g_ipcam_upgrade.ipcam_firmware_version_list,
                    spiderx_version.device_ipcam_current_version,
                    sizeof(g_ipcam_upgrade.ipcam_firmware_version_list)))
    {
        DEBUG_INFO("Version informed  matches it in cache, camera needn't upgrade.\n");
        return;
    }

    if (IPCAM_UPGRADE_IDLE == g_ipcam_upgrade.ipcam_upgrade_state)
    {
        ipcam_upgrade_start();
    }
    else
    {
        DEBUG_INFO("Camera is upgrading, set ipcam_upgrade_noticed.\n");
        g_ipcam_upgrade.ipcam_upgrade_noticed = 1;
    }
}

void ipcam_handle_firmware_upgrade_event(void *notity_firmware_upgrade_info)
{
    HC_DEVICE_EXT_CAMERA_UPGRADE_DONE_S ipcam_upgrade_reply;
    HC_DEVICE_EXT_CAMERA_UPGRADE_S *ipcam_upgrade_notice;
    CTRL_EXTERNAL_DEVICE_IPCAM_TYPE  device_type;
    char *ipcam_firmware_version;
    char *ipcam_firmware_file;
    int result_code = IPCAM_FAILED;
    int i = 0;

    if (g_ipcam_upgrade.wait_image_flag == 0)
    {
        return;
    }

    /* check para */
    ipcam_upgrade_notice = (HC_DEVICE_EXT_CAMERA_UPGRADE_S *)notity_firmware_upgrade_info;
    if (ipcam_upgrade_notice->device_type < 0
            || ipcam_upgrade_notice->device_type >= EXTERNAL_DEVICE_IPCAM_MAX_NUM)
    {
        DEBUG_ERROR("Unsupport SYS_MSG_IPCAM_NOTIFY_FIRMWARE_UPGRADE_EVENT device type [%d].\n",
                       ipcam_upgrade_notice->device_type);
        goto response_fw_upgrade_event;
    }
    ipcam_firmware_version = ipcam_upgrade_notice->firmware_version;
    ipcam_firmware_file = ipcam_upgrade_notice->file_path;
    device_type = ipcam_upgrade_notice->device_type;

    if (g_ipcam_upgrade.ipcam_upgrade_state != IPCAM_UPGRADE_WAIT_IMAGE)
    {
        DEBUG_ERROR("Camera upgrade state isn't wait. state[%s].\n", 
                    convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state));
        g_ipcam_upgrade.ipcam_upgrade_state = IPCAM_UPGRADE_IDLE;
        goto response_fw_upgrade_event;
    }
    
    if (g_ipcam_upgrade.wait_image_flag)
    {
        // unset wait image timeout timer
        set_timer_wait_image(0);
    }

    // check firmware type
    if (g_ipcam_upgrade.ipcam_device_type != ipcam_upgrade_notice->device_type
            || 0 == strlen(ipcam_upgrade_notice->file_path))
    {
        DEBUG_ERROR("Event check failed, type[%d]-[%d] version[%s] file[%s].\n",
                       ipcam_upgrade_notice->device_type, g_ipcam_upgrade.ipcam_device_type, 
                       ipcam_firmware_version, ipcam_firmware_file);
        // reset upgrade state
        g_ipcam_upgrade.ipcam_upgrade_state = IPCAM_UPGRADE_IDLE;
        goto response_fw_upgrade_event;
    }

    memset(&g_ipcam_upgrade.ipcam_firmware_version, 0, sizeof(g_ipcam_upgrade.ipcam_firmware_version));
    strncpy(g_ipcam_upgrade.ipcam_firmware_version, ipcam_upgrade_notice->firmware_version, sizeof(g_ipcam_upgrade.ipcam_firmware_version) - 1);

    memset(&g_ipcam_upgrade.ipcam_firmware_path, 0, sizeof(g_ipcam_upgrade.ipcam_firmware_path));
    strncpy(g_ipcam_upgrade.ipcam_firmware_path, ipcam_upgrade_notice->file_path, sizeof(g_ipcam_upgrade.ipcam_firmware_path) - 1);

    g_ipcam_upgrade.ipcam_upgrade_state = IPCAM_UPGRADE_RUNNING;
    
    DEBUG_INFO("Camera upgrade state change to [%s]\n", convert_state_to_string(g_ipcam_upgrade.ipcam_upgrade_state));

    // call ipcam_fork_to_upgrade to do ipcam upgrade
    ipcam_fork_to_upgrade();
    return;

response_fw_upgrade_event:
    // send SYS_MSG_IPCAM_NOTIFY_FIRMWARE_UPGRADE_DONE_EVENT back
    memset(&ipcam_upgrade_reply, 0, sizeof(ipcam_upgrade_reply));
    ipcam_upgrade_reply.device_type = ipcam_upgrade_notice->device_type;
    strncpy(ipcam_upgrade_reply.file_path, ipcam_upgrade_notice->file_path, sizeof(ipcam_upgrade_reply.file_path) - 1);
    ipcam_upgrade_reply.upgrade_status = result_code;
    //ctrl_sendsignal_with_struct(g_ipcam_upgrade.conn, CTRL_COMMON_INTERFACE, CTRL_SIG_IPCAM_NOTIFY_FIRMWARE_UPGRADE_DONE, (char *)&ipcam_upgrade_reply, sizeof(ipcam_upgrade_reply));
    ipcam_upgrade_send_msg(HC_EVENT_EXT_CAMERA_FW_UPGRADE_DONE, HC_DEVICE_TYPE_EXT_CAMERA_UPGRADE_DONE, &ipcam_upgrade_reply, sizeof(ipcam_upgrade_reply));

    return;
}


