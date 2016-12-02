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
 *   File   : sp_ble_serial.h
 *   Abstract:
 *   Date   : 06/03/2016
 *   $Id:$
 *
 *   Modification History:
 *   By     Date    Ver.  Modification Description
 *   -----------  ---------- -----  -----------------------------
 *
 
******************************************************************************/


#ifndef _ZW_SERIAL_H_
#define _ZW_SERIAL_H_
#include "sp_ble_uart.h"
#include "sp_ble_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int serial_fd;

int  serial_init(void);

void serial_uninit(void);

int  serial_send(unsigned char *buffer, int length);

void * serial_handle_thread(void *arg);
//static void ProcessNotificationAttributeMsg( void *data, unsigned int data_length );
static int fresh_rx_buffer(int start_idx, int end_idx);
static void PraseBleMsg(DBusConnection* dbus_conn, WSDP_MSG_HDR_S* msg_hdr, uint8_t* data);
static int parse_rx_buffer(DBusConnection * conn);




#ifdef __cplusplus
}
#endif

#endif

