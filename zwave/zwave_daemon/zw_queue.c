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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "hc_common.h"
#include "zw_queue.h"



static QUEUE_UNIT_S *q_task_head = NULL;
static QUEUE_UNIT_S *q_task_tail = NULL;
static int q_task_count = 0;
static pthread_mutex_t q_task_mutex;


int queue_init(void)
{
    if (pthread_mutex_init(&q_task_mutex, NULL) != 0)
    {
        return -1;
    }
    return 0;
}

void queue_uninit(void)
{
    pthread_mutex_destroy(&q_task_mutex);
}


int queue_push(QUEUE_UNIT_S *unit)
{
    pthread_mutex_lock(&q_task_mutex);

    if (queue_empty())
    {
        q_task_head = unit;
        q_task_tail = unit;
    }
    else
    {
        unit->prev = q_task_tail;
        unit->next = NULL;
        q_task_tail->next = unit;
        q_task_tail = unit;
    }
    q_task_count++;

    pthread_mutex_unlock(&q_task_mutex);

    return 0;
}

int queue_insert_head(QUEUE_UNIT_S *unit)
{
    pthread_mutex_lock(&q_task_mutex);

    if (queue_empty())
    {
        q_task_head = unit;
        q_task_tail = unit;
    }
    else
    {
        unit->prev = NULL;
        unit->next = q_task_head;
        q_task_head->prev = unit;
        q_task_head = unit;
    }
    q_task_count++;

    pthread_mutex_unlock(&q_task_mutex);

    return 0;
}

int queue_insert_priority(QUEUE_UNIT_S *unit)
{
    QUEUE_UNIT_S *tmp = NULL;

    pthread_mutex_lock(&q_task_mutex);

    if (queue_empty())
    {
        unit->prev = NULL;
        unit->next = NULL;
        q_task_head = unit;
        q_task_tail = unit;
    }
    else
    {
        for (tmp = q_task_head; tmp != NULL; tmp = tmp->next)
        {
            if (unit->priority < tmp->priority)
            {

                if (tmp->prev == NULL)
                {
                    unit->prev = NULL;
                    unit->next = tmp;
                    tmp->prev = unit;
                    q_task_head = unit;
                }
                else
                {
                    unit->next = tmp;
                    unit->prev = tmp->prev;
                    tmp->prev->next = unit;
                    tmp->prev = unit;
                }
                break;
            }

            if (tmp == q_task_tail)
            {
                unit->prev = tmp;
                unit->next = NULL;
                tmp->next = unit;
                q_task_tail = unit;
                break;
            }
        }
    }
    q_task_count++;

    pthread_mutex_unlock(&q_task_mutex);

    return 0;
}

void queue_dump(void)
{
    QUEUE_UNIT_S *tmp = NULL;

    pthread_mutex_lock(&q_task_mutex);

    if (queue_empty())
    {
        DEBUG_INFO("Queue is empty.\n");
    }
    else
    {
        DEBUG_INFO("============== Queue Dump %d ==============\n", q_task_count);
        for (tmp = q_task_head; tmp != NULL; tmp = tmp->next)
        {
            DEBUG_INFO("pri:%d, msg:%d, appid:%d.\n",
                       tmp->priority, tmp->msgtype, tmp->appid);
        }
    }

    pthread_mutex_unlock(&q_task_mutex);
}

int queue_found(QUEUE_UNIT_S *unit)
{
    QUEUE_UNIT_S *tmp = NULL;
    int found = 0;

    pthread_mutex_lock(&q_task_mutex);

    if (queue_empty())
    {
        pthread_mutex_unlock(&q_task_mutex);
        return found;
    }

    for (tmp = q_task_head; tmp != NULL; tmp = tmp->next)
    {
        if (memcmp(tmp, unit, sizeof(QUEUE_UNIT_S)) == 0)
        {
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&q_task_mutex);

    return found;
}

QUEUE_UNIT_S * queue_popup(void)
{
    QUEUE_UNIT_S *p = NULL;

    pthread_mutex_lock(&q_task_mutex);
    if (queue_empty())
    {
        pthread_mutex_unlock(&q_task_mutex);
        return p;
    }
    p = q_task_head;
    q_task_head = p->next;
    if (q_task_head)
        q_task_head->prev = NULL;
    else
        q_task_tail = NULL;

    q_task_count--;
    pthread_mutex_unlock(&q_task_mutex);

    return p;
}


int queue_empty(void)
{
    return (q_task_head == NULL) ? 1 : 0;
}


int queue_number(void)
{
    return q_task_count;
}

void queue_clean(void)
{
    QUEUE_UNIT_S *p = NULL;

    pthread_mutex_lock(&q_task_mutex);
    while (!queue_empty())
    {
        p = q_task_head;
        q_task_head = p->next;
        free(p);
    }
    q_task_tail = NULL;
    q_task_count = 0;

    pthread_mutex_unlock(&q_task_mutex);
}


