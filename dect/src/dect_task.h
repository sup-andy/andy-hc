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
 *   File   : dect_task.h
 *   Abstract:
 *   Date   : 03/10/2015
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _DECT_TASK_H_
#define _DECT_TASK_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    DECT_CONTROLLER_MODE_IDLE = 0,
    DECT_CONTROLLER_MODE_ADDING,
    DECT_CONTROLLER_MODE_DELETING,
    DECT_CONTROLLER_MODE_UPGRADING,
    DECT_CONTROLLER_MODE_MAX
}
DECT_CONTROLLER_MODE_E;

typedef struct task_head_s
{
    int type;
    int appid;
    unsigned int timeout;
} TASK_HEAD_S;

int  task_init(void);

void task_uninit(void);

int task_add(TASK_HEAD_S *head, void *devinfo);

void * task_handle_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif

