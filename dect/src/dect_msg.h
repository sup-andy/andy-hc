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
 *   File   : dect_msg.h
 *   Abstract:
 *   Date   : 03/11/2015
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _DECT_MSG_H_
#define _DECT_MSG_H_
#include "hcapi.h"
//#include "hc_msg.h"
//#include "hc_util.h"

#ifdef __cplusplus
extern "C"
{
#endif

int  msg_init(void);

void msg_uninit(void);

int  msg_handle(void);

void msg_DECTReport(DECT_HAN_DEVICE_INF *devinfo);

void msg_DECTAddSuccess(DECT_HAN_DEVICE_INF *devinfo);

void msg_DECTAddFail(int status);

void msg_DECTRemoveSuccess(unsigned int id);

void msg_DECTRemoveFail(int status);

void msg_DECTReply(int appid, HC_EVENT_TYPE_E type, DECT_HAN_DEVICE_INF *devinfo);

void msg_DECTVoiceReq(DECT_HAN_DEVICE_INF *devinfo);

void msg_DECTVoiceReply(DECT_HAN_DEVICE_INF *devinfo);

#ifdef __cplusplus
}
#endif


#endif

