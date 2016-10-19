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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dect_queue.h"



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
        q_task_tail->next = unit;
        q_task_tail = unit;
    }
    q_task_count++;

    pthread_mutex_unlock(&q_task_mutex);

    return 0;
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


