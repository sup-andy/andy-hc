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
 *   File   : zw_msg.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _ZW_MSG_H_
#define _ZW_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif


int  msg_init(void);

void msg_uninit(void);

int  msg_handle(void);

void msg_ZWReportCB(ZW_DEVICE_INFO *devinfo);

void msg_ZWEventCB(ZW_EVENT_REPORT *event);

void msg_ZWAddSuccess(ZW_DEVICE_INFO *devinfo);

void msg_ZWAddFail(ZW_STATUS status);

void msg_ZWRemoveSuccess(ZW_DEVICE_INFO *devinfo);

void msg_ZWRemoveFail(ZW_STATUS status);

void msg_ZWReply(int appid, HC_EVENT_TYPE_E type, ZW_DEVICE_INFO *devinfo);


#ifdef __cplusplus
}
#endif


#endif

