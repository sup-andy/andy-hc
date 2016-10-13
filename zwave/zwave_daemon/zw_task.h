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
 *   File   : zw_task.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _ZW_TASK_H_
#define _ZW_TASK_H_

#ifdef __cplusplus
extern "C"
{
#endif

//#pragma pack(1)

#define MAX_MID_DEVICE_NUMBER       232 //  ZW_MAX_NODES        232

typedef enum {
    TASK_PRIORITY_PROTOCOL = 1,
    TASK_PRIORITY_UI,
    TASK_PRIORITY_DETECT
}
ZW_TASK_PRIORITY_E;

typedef enum {
    ZW_CONTROLLER_MODE_IDLE = 0,
    ZW_CONTROLLER_MODE_ADDING,
    ZW_CONTROLLER_MODE_DELETING,
    ZW_CONTROLLER_MODE_RUNNING,
    ZW_CONTROLLER_MODE_MAX
} ZW_CONTROLLER_MODE_E;

typedef struct {
    int dev_id;
    int phy_id;
    int dev_type;
    unsigned int time_count;
    unsigned int detect_interval;
} MID_DEVICE_INFO_S;

MID_DEVICE_INFO_S g_mid_device_info[MAX_MID_DEVICE_NUMBER];

extern char g_image_file_path[256];

int  task_init(void);

void task_uninit(void);

int task_add(int msgtype, int appid, int priority, void *devinfo);

void * task_handle_thread(void *arg);

MID_DEVICE_INFO_S * get_unused_mid_device(void);

MID_DEVICE_INFO_S * get_mid_device_by_dev_id(int dev_id);

int set_mid_device_info(MID_DEVICE_INFO_S *mid_dev, ZW_DEVICE_INFO *zw_dev);

void reset_mid_device_info(MID_DEVICE_INFO_S *mid_dev);

int mid_device_init(ZW_NODES_INFO *zw_nodes);

int mid_device_init_from_db(void);

void * detect_handle_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif

