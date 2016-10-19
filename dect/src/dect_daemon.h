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
 
#ifndef _DECT_DAEMON_H_
#define _DECT_DAEMON_H_
#include <stdio.h>

#define DECT_LOG_MAX_NUM 20

typedef enum {
    DECT_OPERATION_LIST = 0,
    DECT_OPERATION_DO,
    DECT_OPERATION_SLIDE,
    DECT_OPERATION_ADD,
    DECT_OPERATION_DELETE,
    DECT_OPERATION_NAME,
    DECT_OPERATION_SHOW_STATUS,
    DECT_OPERATION_SHOW_LOG,
    DECT_OPERATION_CLEAR_LOG,
    DECT_OPERATION_REFRESH,
    DECT_OPERATION_HANDSET_LIST,
    DECT_OPERATION_HANDSET_ADD,
    DECT_OPERATION_HANDSET_DELETE
} DECT_OPERATION_MSG_TYPE_E;

typedef struct dect_operation_msg_t {
    DECT_OPERATION_MSG_TYPE_E type;
    int key;
    union
    {
        struct
        {
            int id;
            int value;
        } do_body;

        struct
        {
            char id[32];
            char name[64];
            int type; /* 0:ule device, 1:ipcam device */
        } name_body;
        unsigned int id;
        unsigned int timeout;
        unsigned int index;
    } msg_body;
} DECT_OPERATION_MSG;

typedef struct dect_operation_log_t {
    int result;
    char date[32];
    char action[32];
    char value[64];
} DECT_OPERATION_LOG;

typedef struct dect_operation_log_data_t {
    int num;
    DECT_OPERATION_LOG log[DECT_LOG_MAX_NUM];
} DECT_OPERATION_LOG_DATA;

#endif  /* _DECT_DAEMON_H_ */

