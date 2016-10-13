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
 *   File   : zw_util.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/

#ifndef _ZW_UTIL_H_
#define _ZW_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

int map_bin_sensor_hctype(int zwtype);

int map_bin_sensor_zwtype(int hctype);

int map_multi_sensor_hctype(int zwtype);

int map_multi_sensor_zwtype(int hctype);

int map_meter_hctype(int zwtype);

int map_meter_zwtype(int hctype);

int map_binary_switch_hctype(int zwtype);

int map_binary_switch_zwtype(int hctype);

int start_thread(pthread_t *ptid, int priority, void * (*func)(void *), void *data);

int stop_thread(pthread_t ptid);

int set_nonblocking(int sock);

int double_equal(double a, double b);

#ifdef __cplusplus
}
#endif

#endif

