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
*   File   :  dect_util.c
*   Abstract:
*   Date   : 10/03/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "dect_util.h"
#include "ctrl_common_lib.h"

int start_thread(pthread_t *ptid, int priority, void * (*func)(void *), void *data)
{
    pthread_attr_t  attr;

    if ((pthread_attr_init(&attr) != 0))
    {
        return -1;
    }

    pthread_attr_setstacksize(&attr, 1024 * 1024);
    if ((pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0))
    {
        return -1;
    }

    if (priority > 0)
    {

        struct sched_param sched;
        sched.sched_priority = priority;

        if ((pthread_attr_setschedparam(&attr, &sched) != 0))
        {
            return -1;
        }
    }

    if ((pthread_create(ptid, &attr, func, data) != 0))
    {
        return -1;
    }

    return 0;
}


int stop_thread(pthread_t ptid)
{
    int ret;

    pthread_cancel(ptid);

    ret = pthread_join(ptid, NULL);
    if (ret == 0)
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Stop the pthread %u ok\n", ptid);
    }
    else
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Stop the pthread %u failed\n", ptid);
    }

    return ret;
}

int set_nonblocking(int sock)
{
    int opts = 0;

    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        return -1;
    }

    if (fcntl(sock, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        return -1;
    }

    return 0;
}

