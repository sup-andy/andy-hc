#ifndef _SP_BLE_PROTOCOL_
#define _SP_BLE_PROTOCOL_
#include <stdint.h>
#include "ctrl_common_lib.h"

#define WSDP_UART_BUF_SIZE			512 /* Limit by BLE Module */

#define WSDP_SLIP_END_SIZE			1
#define WSDP_PACKET_HDR_SIZE			4
#define WSDP_MSG_HDR_SIZE 			4
#define WSDP_SLIP_DATA_MAX_SIZE 	(WSDP_UART_BUF_SIZE ? (WSDP_SLIP_END_SIZE * 2))
#define WSDP_PACKET_DATA_MAX_SIZE 	(WSDP_SLIP_DATA_MAX_SIZE - WSDP_PACKET_HDR_SIZE ? 1)
#define WSDP_MSG_DATA_MAX_SIZE		(WSDP_PACKET_DATA_MAX_SIZE - WSDP_MSG_HDR_SIZE)

#define WSDP_SLIP_END       0xC0  /* indicates end of packet */
#define WSDP_SLIP_ESC       0xDB  /* indicates byte stuffing */
#define WSDP_SLIP_ESC_END   0xDC
#define WSDP_SLIP_ESC_ESC   0xDD


enum {
	/* Message Base */
	BLE_MSG_DUMMY 						= 0x0000,
	BLE_MSG_DEBUG 						= 0x0001,
	
	/* Message LE_SP  */
	BLE_MSG_LE_SP_MIN 					= 0x0100,
	BLE_MSG_LE_SP_DEVICE_INFO 				= 0x0101,
	BLE_MSG_LE_SP_BATTERY_LEVEL				= 0x0102,
	BLE_MSG_LE_SP_MAX 					= 0x01FF,
	
	/* Message LE_SC */
	BLE_MSG_LE_SC_MIN 					= 0x0200,
	BLE_MSG_LE_SC_BLE_STATE 				= 0x0201,
	BLE_MSG_LE_SC_DEVICE_INFO 				= 0x0202,
	BLE_MSG_LE_SC_BATTERY_LEVEL 				= 0x0203,
	BLE_MSG_LE_SC_CURRENT_TIME				= 0x0204,
	BLE_MSG_LE_SC_MAX 					= 0x02FF,
	
	/* Message SP_LE */
	BLE_MSG_SP_LE_MIN 					= 0x0300,
	BLE_MSG_SP_LE_GET_DEVICE_INFO 				= 0x0301,
	BLE_MSG_SP_LE_GET_BATTERY_LEVEL 			= 0x0302,
	BLE_MSG_SP_LE_WRITE_ANCS_CONTROL			= 0x03A1,
	BLE_MSG_SP_LE_MAX 					= 0x03FF,
	
	/* Message SC_LE */
	BLE_MSG_SC_LE_MIN 					= 0x0400,
	BLE_MSG_SC_LE_MAX 					= 0x04FF,
	
	/* Message CTRL */
	BLE_MSG_CTRL_MIN  				= 0x1000,
	BLE_MSG_CTRL_GET_SYS_INFO_REQ 			= 0x1001,
	BLE_MSG_CTRL_GET_SYS_INFO_RESP 			= 0x1002,
	#if 0
	BLE_MSG_CTRL_GET_LED_REQ			= 0x1003,
	BLE_MSG_CTRL_GET_LED_RESP			= 0x1004,
	BLE_MSG_CTRL_SET_LED_STATUS			= 0x1005,
	#else  /* Sekar Added on 22nd August 2016  */ 
	
	BLE_MSG_CTRL_SET_BATTERY_LEVEL			= 0x1003,
	BLE_MSG_CTRL_SET_CURRENT_TIME			= 0x1004,

	BLE_MSG_CTRL_SET_LED_STATUS			= 0x1005,

	BLE_MSG_CTRL_GET_IDEVICE_INFO_REQ		= 0x1006,
	BLE_MSG_CTRL_GET_IDEVICE_INFO_RESP		= 0x1007,

	BLE_MSG_CTRL_GET_APP_LIST_REQ			= 0x1008,
	BLE_MSG_CTRL_GET_APP_LIST_RESP 			= 0x1009,
	#endif 

	BLE_MSG_CTRL_FW_UPGRADE_REQ			= 0xFE01,
	BLE_MSG_CTRL_FW_UPGRADE_RESP			= 0xFE02,
	BLE_MSG_CTRL_FW_UPGRADE_DATA			= 0xFE03,
	BLE_MSG_CTRL_FW_UPGRADE_STATUS			= 0xFE04,
	BLE_MSG_CTRL_MAX  				= 0xFFFF,
};


typedef struct __wsdp_pkt_hdr_s
{
	uint8_t  flags;
	uint16_t plen;
	uint8_t  checksum;
} __attribute__((packed)) WSDP_PKT_HDR_S;

typedef struct __wsdp_msg_hdr_s
{
	uint16_t type;
	uint16_t length;
} __attribute__((packed)) WSDP_MSG_HDR_S;

#define SP_MSG_DATA_MAX_SIZE	 	WSDP_MSG_DATA_MAX_SIZE

typedef enum __sp_ble_state_e
{
	SP_BLE_STATE_POWER_ON = 0,		/* send by main(main.c) */
	SP_BLE_STATE_ADVERTISING,		/* send by main(main.c) */
	SP_BLE_STATE_CONNECTED,			/* send by on_ble_evt(main.c) */
	SP_BLE_STATE_PAIRING,			/* send by sec_req_timeout_handler(main.c) */
	SP_BLE_STATE_DISCOVERY,			/* send by device_manager_evt_handler(main.c) */
	SP_BLE_STATE_SERVICE_NOW,		/* send by on_ancs_c_evt(main.c) */
	SP_BLE_STATE_DISCONNECTED,		/* send by on_ble_evt(main.c) */
	SP_BLE_STATE_MAX,
} SP_BLE_STATE_E;

#define BLE_ADDR_LEN   		6
#define BLE_NAME_LEN   	    12 /* End by \0 */
#define BLE_UUID_LEN		16 /* 128 bit */

typedef struct __sp_ble_device_info_s
{
	uint8_t bdaddr[BLE_ADDR_LEN];
	uint8_t service_uuid[BLE_UUID_LEN];
	char 	device_name[BLE_NAME_LEN];
} __attribute__((packed)) SP_BLE_DEVICE_INFO_S;

#endif

