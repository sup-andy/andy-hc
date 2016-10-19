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
 *   File   : dect_queue.h
 *   Abstract:
 *   Date   : 03/10/2015
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _DECT_QUEUE_H_
#define _DECT_QUEUE_H_
#include "dect_device.h"


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct queue_unit_s {
    int msgtype;
    int appid;
    unsigned int timeout;
    DECT_HAN_DEVICE_INF devinfo;
    struct queue_unit_s *next;
} QUEUE_UNIT_S;


int queue_init(void);

void queue_uninit(void);

int queue_push(QUEUE_UNIT_S *unit);

QUEUE_UNIT_S * queue_popup(void);

int queue_empty(void);

int queue_number(void);

void queue_clean(void);


#ifdef __cplusplus
}
#endif


#endif

