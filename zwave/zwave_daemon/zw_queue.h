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
 *   File   : zw_queue.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _ZW_QUEUE_H_
#define _ZW_QUEUE_H_

#include "zwave_api.h"


#ifdef __cplusplus
extern "C"
{
#endif

//#pragma pack(1)

typedef enum {
    MID_DEVICE_TYPE_INFO = 1,
    MID_DEVICE_TYPE_CONFIG,
    MID_DEVICE_TYPE_ASSOCIATION
}
MID_DEVICE_TYPE_E;

typedef struct {
    int srcPhyId;
    int dstPhyId;
} MID_DEVICE_ASSOCIATION_S;

typedef union {
    ZW_DEVICE_INFO devinfo;
    ZW_DEVICE_CONFIGURATION config;
    MID_DEVICE_ASSOCIATION_S association;
} MID_DEVICE_U;

typedef struct queue_unit_s {
    int msgtype;
    int appid;
    int priority;
    MID_DEVICE_U device;
    struct queue_unit_s *prev;
    struct queue_unit_s *next;
} QUEUE_UNIT_S;


int queue_init(void);

void queue_uninit(void);

int queue_push(QUEUE_UNIT_S *unit);

int queue_insert_head(QUEUE_UNIT_S *unit);

int queue_insert_priority(QUEUE_UNIT_S *unit);

int queue_found(QUEUE_UNIT_S *unit);

QUEUE_UNIT_S * queue_popup(void);

int queue_empty(void);

int queue_number(void);

void queue_clean(void);

void queue_dump(void);


#ifdef __cplusplus
}
#endif


#endif

