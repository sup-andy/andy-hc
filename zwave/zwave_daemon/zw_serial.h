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
 *   File   : zw_serial.h
 *   Abstract:
 *   Date   : 12/18/2014
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 ******************************************************************************/


#ifndef _ZW_SERIAL_H_
#define _ZW_SERIAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

int  serial_init(void);

void serial_uninit(void);

int  serial_send(unsigned char *buffer, int length);

void * serial_handle_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif
