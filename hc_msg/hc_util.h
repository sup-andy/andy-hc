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
 *   File   : hc_util.h
 *   Abstract:
 *   Date   : 1/16/2015
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _HC_UTIL_H_
#define _HC_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

char * hc_map_msg_txt(int id);

char * hc_map_hcdev_txt(int id);

char * hc_map_client_txt(int id);

char * hc_map_bin_sensor_txt(int id);

char * hc_map_multi_sensor_txt(int id);

char * hc_map_meter_txt(int id);


#ifdef __cplusplus
}
#endif

#endif
