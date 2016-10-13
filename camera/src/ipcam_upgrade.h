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

#ifndef _IPCAM_UPGRADE_H_
#define _IPCAM_UPGRADE_H_

#include "capi_struct.h"
#include "ctrl_common_lib.h"

#ifdef __cplusplus
extern "C"
{
#endif


//#define IPCAM_INTERNAL_DEBUG

#define IPCAM_WAIT_IMAGE_TIMER_INTERVAL     600

#define IPCAM_UPGRADE_CHECK_START       300 // time when start to check result after post image to all devices
#define IPCAM_UPGRADE_CHECK_INTERVAL    120  // check interval
#define IPCAM_UPGRADE_CHECK_RETRY       5
#define IPCAM_QUERY_TIMEOUT_SEC         5
#define IPCAM_RESULT_QUERY_TIMEOUT      5

#define IPCAM_MODEL_K10      "K10"
#define IPCAM_MODEL_IP8130W "IP8130W"
#define IPCAM_MODEL_IP8131W "IP8131W"
#define IPCAM_MODEL_IB8168  "IB8168"
#define IPCAM_MODEL_FD8168  "FD8168"
#define IPCAM_MODEL_IP8137W "IP8137-W"
#define IPCAM_MODEL_WD8137W "WD8137-W"

typedef enum {
    IPCAM_UPGRADE_IDLE = 0,
    IPCAM_UPGRADE_WAIT_IMAGE,
    IPCAM_UPGRADE_RUNNING,
    IPCAM_UPGRADE_UNKNOW
} IPCAM_UPGRADE_STATE_E;

typedef enum {
    IPCAM_SUCCESS = 0,
    IPCAM_INIT_SOCKET_FAILED,
    IPCAM_TCP_FAILED,
    IPCAM_SSL_FAILED,
    IPCAM_SEND_FAILED,
    IPCAM_RECV_FAILED,
    IPCAM_FAILED
} IPCAM_RESULT_E;

typedef enum {
    IPCAM_UPGRADE_STATUS_SUCCESS = 0,
    IPCAM_UPGRADE_STATUS_FAIL,
    IPCAM_UPGRADE_STATUS_OPEN_FW_FAIL,
    IPCAM_UPGRADE_STATUS_RD_FW_FAIL,
    IPCAM_UPGRADE_STATUS_INIT_SOCKET_FAIL,
    IPCAM_UPGRADE_STATUS_TCP_CONNECT_FAIL,
    IPCAM_UPGRADE_STATUS_INIT_SSL_FAIL,
    IPCAM_UPGRADE_STATUS_AUTH_FAIL,
    IPCAM_UPGRADE_STATUS_RECV_FAIL,
    IPCAM_UPGRADE_STATUS_SEND_FAIL,
    IPCAM_UPGRADE_STATUS_DLC_OFFLINE,
    IPCAM_UPGRADE_STATUS_CHECK_MODEL_FAIL,
    IPCAM_UPGRADE_STATUS_CHECK_VERSION_FAIL,
    IPCAM_UPGRADE_STATUS_CHECK_VERSION_AND_MODEL_FAIL,
    IPCAM_UPGRADE_STATUS_GET_CONFIG_FILE_FAIL,
    IPCAM_UPGRADE_STATUS_TO_BE_UPGRADE,
    IPCAM_UPGRADE_STATUS_MAC_DISMATCH,
    IPCAM_UPGRADE_STATUS_MAX
} IPCAM_UPGRADE_STATUS_E;

typedef struct {
    unsigned int dev_id;
    int connection; // 1: onlie; other offline
    char ipaddress[16];
    char mac[32];
    char fwversion[64];
    char modelname[64];
    char result_code[64];
} IPCAM_DEVICE_INFO_S;

typedef struct {
    IPCAM_DEVICE_INFO_S ipcam_device[MAX_IPCAM_LIST_LEN];
    IPCAM_UPGRADE_STATE_E ipcam_upgrade_state;
    CTRL_EXTERNAL_DEVICE_IPCAM_TYPE ipcam_device_type;
    int ipcam_upgrade_noticed;         // 1 noticed 0 not noticed
    int ipcam_list_change_noticed;    // 1 noticed 0 not noticed
    int graceful_exit_flag;
    int error_count;
    int wait_image_flag;
    int wait_image_timeout;
    int result_query_timeout;
    int need_upgrade_count[EXTERNAL_DEVICE_IPCAM_MAX_NUM];
    char ipcam_firmware_version_list[EXTERNAL_DEVICE_IPCAM_MAX_NUM][GENERAL_STR_LEN];
    char ipcam_firmware_version[MAX_STR_LEN];
    char ipcam_firmware_path[MAX_STR_LEN];
} IPCAM_UPGRADE_CB_S;

typedef struct {
    char code_str[AVAGE_STR_LEN];
    char detail_str[GENERAL_STR_LEN];
} IPCAM_UPGRADE_STATUS_S;

/* Shared memory to store ipcam upgrade result info */
typedef struct {
    int result_unread;  // 1- unread msg
    int error_count;
} IPCAM_UPGRADE_SHM_DATA_S;

void ipcam_upgrade_start(void);
void ipcam_upgrade_end(void);
int  ipcam_shm_data_init(void);
void ipcam_signal_handler(int signum);
void ipcam_camera_list_changed(void);
void ipcam_handle_timeout_event(void);
void ipcam_handle_firmware_version_change_event(void);
void ipcam_handle_firmware_upgrade_event(void *notity_firmware_upgrade_info);

#ifdef __cplusplus
}
#endif

#endif
