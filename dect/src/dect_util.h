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
 *   Creator : Harvey hua
 *   File   : dect_util.h
 *   Abstract:
 *   Date   : 03/10/2015
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/

#ifndef _DECT_UTIL_H_
#define _DECT_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

int start_thread(pthread_t *ptid, int priority, void * (*func)(void *), void *data);

int stop_thread(pthread_t ptid);

int set_nonblocking(int sock);

#ifdef __cplusplus
}
#endif

#endif

