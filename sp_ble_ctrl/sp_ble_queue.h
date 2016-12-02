/******************************************************************************
*
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
 *   Creator : Andy Yang
 *   File   : sp_ble_queue.h
 *   Abstract:
 *   Date   : 06/03/2016
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *

******************************************************************************/


#ifndef _SP_BLE_QUEUE_H_
#define _SP_BLE_QUEUE_H_

#include "sp_ble_uart.h"



#ifdef __cplusplus
extern "C"
{
#endif

//#pragma pack(1)

typedef struct queue_unit_s {
    BLE_UART_ANCS_NOTIFY_S ancs_notification;
    struct queue_unit_s *prev;
    struct queue_unit_s *next;
} QUEUE_UNIT_S;


int queue_init(void);

void queue_uninit(void);

int queue_push(QUEUE_UNIT_S *unit);

int queue_insert_head(QUEUE_UNIT_S *unit);

int queue_insert_tail(QUEUE_UNIT_S *unit);

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


