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
*   Creator : Harvey Hua
*   File   : dect_api.h
*   Abstract:
*   Date   : 03/11/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*
******************************************************************************/
#ifndef _DECT_API_H_
#define _DECT_API_H_
#include "dect_device.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _DECT_STATUS_
{
    DECT_STATUS_OK = 0,
    DECT_STATUS_FAIL,
    DECT_STATUS_TIMEOUT,
    DECT_STATUS_MAX
}
DECT_STATUS;

DECT_STATUS dect_daemon_init(void);
DECT_STATUS dect_callline_init(void);
DECT_STATUS dect_ule_device_mgr_init(void);
DECT_STATUS dect_thread_int(void);
DECT_STATUS dect_device_set(DECT_HAN_DEVICE_INF *device_info);
DECT_STATUS dect_device_add(unsigned int timeout);
DECT_STATUS dect_device_delete(unsigned int id);
DECT_STATUS dect_stop_register(void);
DECT_STATUS dect_handle_upgrade(char *fw_file_path);
DECT_STATUS dect_handle_voip_call(DECT_HAN_DEVICE_INF *devinfo);
DECT_STATUS dect_handset_ring(unsigned int id);
DECT_STATUS dect_handset_stop_ring(void);
DECT_STATUS dect_set_dect_type(int type);
DECT_STATUS dect_get_dect_type(int *type);
DECT_STATUS dect_system_reboot(void);


#ifdef __cplusplus
}
#endif

#endif

