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
 *   File   : sp_ble_queue.c
 *   Abstract:
 *   Date   : 06/03/2016
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ctrl_common_lib.h"
#include "sp_ble_queue.h"


#define QUEUE_MAX_COUNT 200
static QUEUE_UNIT_S *q_notification_head = NULL;
static QUEUE_UNIT_S *q_notification_tail = NULL;
static int q_notification_count = 0;
static pthread_mutex_t q_notification_mutex;


int queue_init(void)
{
    if (pthread_mutex_init(&q_notification_mutex, NULL) != 0)
    {
        return -1;
    }
    return 0;
}

void queue_uninit(void)
{
    pthread_mutex_destroy(&q_notification_mutex);
}


int queue_push(QUEUE_UNIT_S *unit)
{
    pthread_mutex_lock(&q_notification_mutex);

    if (queue_empty())
    {
        q_notification_head = unit;
        q_notification_tail = unit;
    }
    else
    {
        unit->prev = q_notification_tail;
        unit->next = NULL;
        q_notification_tail->next = unit;
        q_notification_tail = unit;
    }
    q_notification_count++;

    pthread_mutex_unlock(&q_notification_mutex);

    return 0;
}

int queue_insert_head(QUEUE_UNIT_S *unit)
{
    pthread_mutex_lock(&q_notification_mutex);

    if (queue_empty())
    {
        unit->prev = NULL;
        unit->next = NULL;
        q_notification_head = unit;
        q_notification_tail = unit;
    }
    else
    {
        unit->prev = NULL;
        unit->next = q_notification_head;
        q_notification_head->prev = unit;
        q_notification_head = unit;
    }
    q_notification_count++;
    if (q_notification_count > QUEUE_MAX_COUNT)
    {
        QUEUE_UNIT_S *tmp;
        tmp = q_notification_tail;
        q_notification_tail = q_notification_tail->prev;
        free(tmp);
        q_notification_count --;
    }
    pthread_mutex_unlock(&q_notification_mutex);

    return 0;
}


int queue_insert_tail(QUEUE_UNIT_S *unit)
{
    pthread_mutex_lock(&q_notification_mutex);

    if (queue_empty())
    {
        unit->prev = NULL;
        unit->next = NULL;
        q_notification_head = unit;
        q_notification_tail = unit;
    }
    else
    {
        unit->prev = q_notification_tail;
        unit->next = NULL;
        q_notification_tail->next = unit;
        q_notification_tail = unit;

    }
    q_notification_count++;
    if (q_notification_count > QUEUE_MAX_COUNT)
    {
        QUEUE_UNIT_S *tmp;
        tmp = q_notification_head;
        q_notification_head = q_notification_head->next;
        free(tmp);
        q_notification_count --;
    }
    pthread_mutex_unlock(&q_notification_mutex);

    return 0;
}

void queue_dump(void)
{
    QUEUE_UNIT_S *tmp = NULL;

    pthread_mutex_lock(&q_notification_mutex);

    if (queue_empty())
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Queue is empty.\n");
    }
    else
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "============== Queue Dump %d ==============\n", q_notification_count);
        for (tmp = q_notification_head; tmp != NULL; tmp = tmp->next)
        {
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "uuid:%s, date:%s, message:%s.\n", tmp->ancs_notification.notify_uid,
                           tmp->ancs_notification.Date,
                           tmp->ancs_notification.Message);
        }
    }

    pthread_mutex_unlock(&q_notification_mutex);
}

int queue_found(QUEUE_UNIT_S *unit)
{
    QUEUE_UNIT_S *tmp = NULL;
    int found = 0;

    pthread_mutex_lock(&q_notification_mutex);

    if (queue_empty())
    {
        pthread_mutex_unlock(&q_notification_mutex);
        return found;
    }

    for (tmp = q_notification_head; tmp != NULL; tmp = tmp->next)
    {
        if (memcmp(tmp->ancs_notification.notify_uid, unit->ancs_notification.notify_uid, sizeof(uint32_t) == 0))
        {
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&q_notification_mutex);

    return found;
}

int queue_delete(QUEUE_UNIT_S *unit)
{
    QUEUE_UNIT_S *tmp = NULL;
    int found = 0;

    pthread_mutex_lock(&q_notification_mutex);

    if (queue_empty())
    {
        pthread_mutex_unlock(&q_notification_mutex);
        return found;
    }

    for (tmp = q_notification_head; tmp != NULL; tmp = tmp->next)
    {
        if (memcmp(tmp->ancs_notification.notify_uid, unit->ancs_notification.notify_uid, sizeof(uint32_t) == 0))
        {
            if (tmp->prev == NULL) //means delete first unit
            {
                q_notification_head = q_notification_head->next;
                q_notification_head->prev = NULL;
                free(tmp);
            }
            else if (tmp->prev != NULL && tmp->next != NULL)
            {
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
                free(tmp);
            }
            else if (tmp->next == NULL)
            {
                q_notification_tail = q_notification_tail->prev;
                q_notification_tail->next = NULL;
                free(tmp);
            }
            break;
        }
    }

    pthread_mutex_unlock(&q_notification_mutex);

    return found;
}



QUEUE_UNIT_S * queue_popup(void)
{
    QUEUE_UNIT_S *p = NULL;

    pthread_mutex_lock(&q_notification_mutex);
    if (queue_empty())
    {
        pthread_mutex_unlock(&q_notification_mutex);
        return p;
    }
    p = q_notification_head;
    q_notification_head = p->next;
    if (q_notification_head)
        q_notification_head->prev = NULL;
    else
        q_notification_tail = NULL;

    q_notification_count--;
    pthread_mutex_unlock(&q_notification_mutex);

    return p;
}


int queue_empty(void)
{
    return (q_notification_head == NULL) ? 1 : 0;
}


int queue_number(void)
{
    return q_notification_count;
}

void queue_clean(void)
{
    QUEUE_UNIT_S *p = NULL;

    pthread_mutex_lock(&q_notification_mutex);
    while (!queue_empty())
    {
        p = q_notification_head;
        q_notification_head = p->next;
        free(p);
    }
    q_notification_tail = NULL;
    q_notification_count = 0;

    pthread_mutex_unlock(&q_notification_mutex);
}



