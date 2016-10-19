/*******************************************************************
*
*    DESCRIPTION: CMBS HAN header file
*
*    AUTHOR: CMBS Team
*
*    © DSP Group Ltd 2011
*
*******************************************************************/
#ifndef CMBS_HAN_H
#define CMBS_HAN_H

#include "cmbs_api.h"


/************************************************************************************************
*
*
*       HAN Unit Types
*
*
*************************************************************************************************/

// Unit Types as define in the Profile document
#define HAN_UNIT_TYPE_ONOFF_SWITCHABLE      0x0100
#define HAN_UNIT_TYPE_ONOFF_SWITCH       0x0101
#define HAN_UNIT_TYPE_LEVEL_CONTROLABLE      0x0102
#define HAN_UNIT_TYPE_LEVEL_CONTROL       0x0103
#define HAN_UNIT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE   0x0104
#define HAN_UNIT_TYPE_LEVEL_CONTROL_SWITCH     0x0105
#define HAN_UNIT_TYPE_AC_OUTLET        0x0106
#define HAN_UNIT_TYPE_AC_OUTLET_WITH_POWER_METERING  0x0107
#define HAN_UNIT_TYPE_LIGHT         0x0108
#define HAN_UNIT_TYPE_DIMMABLE_LIGHT       0x0109
#define HAN_UNIT_TYPE_DIMMER_SWITCH       0x010A
#define HAN_UNIT_TYPE_DOOR_LOCK        0x010B
#define HAN_UNIT_TYPE_DOOR_BELL        0x010C
#define HAN_UNIT_TYPE_POWER_METER       0x010D

#define HAN_UNIT_TYPE_DETECTOR        0x0200
#define HAN_UNIT_TYPE_DOOR_OPEN_CLOSE_DETECTOR    0x0201
#define HAN_UNIT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR   0x0202
#define HAN_UNIT_TYPE_MOTION_DETECTOR      0x0203
#define HAN_UNIT_TYPE_SMOKE_DETECTOR      0x0204
#define HAN_UNIT_TYPE_GAS_DETECTOR       0x0205
#define HAN_UNIT_TYPE_FLOOD_DETECTOR       0x0206
#define HAN_UNIT_TYPE_GLASS_BREAK_DETECTOR     0x0207
#define HAN_UNIT_TYPE_VIBRATION_DETECTOR     0x0208

#define HAN_UNIT_TYPE_SIREN         0x0280

#define HAN_UNIT_TYPE_PENDANT         0x0300

#define HAN_UNIT_TYPE_USER_INTERFACE       0x0410
#define HAN_UNIT_TYPE_GENERIC_APPLCIATION_LOGIC    0x0411



/************************************************************************************************
*
*
*       HAN Interfaces
*
*
*************************************************************************************************/
/*
   Device Management Interface ID
====================================
This interface is used for registering Devices and Units with their associated data
such as Unit Type and the list of interfaces they are supporting.
It is also used for discovering all device and units by a configuration application
*/
#define DEVICE_MGMT_INTERFACE_ID    0x0001

/* Device Management Interface - Server */
/* Attributes */
#define DEVICE_MGMT_IF_SERVER_ATTR_ID_NUM_OF_DEVICES    0x0001

/* Commands */
#define DEVICE_MGMT_IF_SERVER_CMD_ID_REGISTER_DEVICE    0x0001


/* Device Management Interface - Client */
/* Attributes */
/* Commands */

#define  CMBS_HAN_MASK 0x3000

typedef enum
{
    CMBS_EV_DSR_HAN_MNGR_INIT =    CMBS_EV_DSR_HAN_DEFINED_START,  /*!< Init HAN Manager */
    CMBS_EV_DSR_HAN_MNGR_INIT_RES,          /*!< Response to cmbs_dsr_han_mngr_Init */
    CMBS_EV_DSR_HAN_MNGR_START,             /*!< Start HAN Manager */
    CMBS_EV_DSR_HAN_MNGR_START_RES,      /*!< Response to cmbs_dsr_han_mngr_Start */
    CMBS_EV_DSR_HAN_DEVICE_READ_TABLE,      /*!< Read HAN device table */
    CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES,  /*!< Response to cmbs_dsr_han_device_ReadTable */
    CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE,     /*!< Write HAN device table */
    CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE_RES, /*!< Response to cmbs_dsr_han_device_WriteTable */
    CMBS_EV_DSR_HAN_BIND_READ_TABLE,        /*!< Read HAN bind table */
    CMBS_EV_DSR_HAN_BIND_READ_TABLE_RES,  /*!< Response to cmbs_dsr_han_bind_ReadTable */
    CMBS_EV_DSR_HAN_BIND_WRITE_TABLE,       /*!< Write HAN bind table */
    CMBS_EV_DSR_HAN_BIND_WRITE_TABLE_RES,  /*!< Response to cmbs_dsr_han_bind_WriteTable */
    CMBS_EV_DSR_HAN_GROUP_READ_TABLE,       /*!< Read HAN group table */
    CMBS_EV_DSR_HAN_GROUP_READ_TABLE_RES,  /*!< Response to cmbs_dsr_han_group_ReadTable */
    CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE,      /*!< Write HAN group table */
    CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE_RES,  /*!< Response to cmbs_dsr_han_group_WriteTable */
    CMBS_EV_DSR_HAN_MSG_RECV_REGISTER,      /*!< Register unit in HAN message service */
    CMBS_EV_DSR_HAN_MSG_RECV_REGISTER_RES,  /*!< Response to cmbs_dsr_han_msg_RecvRegister */
    CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER,    /*!< Unregister unit in HAN message service */
    CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER_RES,/*!< Response to cmbs_dsr_han_msg_RecvUnregister */
    CMBS_EV_DSR_HAN_MSG_SEND,               /*!< Send message to a destination HAN unit */
    CMBS_EV_DSR_HAN_MSG_SEND_RES,          /*!< Response to cmbs_dsr_han_msg_Send */
    CMBS_EV_DSR_HAN_MSG_RECV,              /*!< When a device send a message to Host */
    CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST,     /*!< Host sends Tx Request to a battery operated device (keep wake-up request) */
    CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST_RES, /*!< response to CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST */
    CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST,        /*!< Host sends Tx end Request to a battery operated device (ok to go to sleep) */
    CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST_RES,    /*!< response to CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST */
    CMBS_EV_DSR_HAN_MSG_SEND_TX_ENDED,    /*!< Unsolicited event for cases when the device closed the link or did not contact */
    CMBS_EV_DSR_HAN_MSG_SEND_TX_READY,        /*!< device reports to host: Clear to send */

    CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_1_NOTIFICATION, /*!< Notification status about 1st stage registration. Has parameters */
    CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_2_NOTIFICATION, /*!< Notification status about 2nd stage registration. Has parameters */
    CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_3_NOTIFICATION, /*!< Notification status about 3rd stage registration. Has parameters */

    CMBS_EV_DSR_HAN_DEVICE_REG_DELETED,     /*!< Unsolicited event. Notifies about device being deleted without host intervention */

    CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE,    /*!< Sent by host to ask to remove certain ULE/HAN device. Has no impact on non-ULE devices*/
    CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE_RES,      /*!< Notifies about the status of the actual deletion */

    CMBS_EV_DSR_HAN_DEVICE_UNKNOWN_DEV_CONTACT,   /*!< Unsolicited event. Notify the host about intruder for security reasons */

    CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS,
    CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS_RES,

    CMBS_EV_DSR_HAN_MAX

} E_CMBS_HAN_EVENT_ID;

typedef enum
{
    CMBS_IE_HAN_MSG = CMBS_IE_HAN_DEFINED_START,        /* !< Holds a HAN Message structure */
    CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES,             /* !< Holds Brief Device table entries */
    CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES,          /* !< Holds Extended Device table entries */
    CMBS_IE_HAN_BIND_TABLE_ENTRIES,                     /* !< Holds Bind table entries */
    CMBS_IE_HAN_GROUP_TABLE_ENTRIES,                    /* !< Holds Group table entries */
    CMBS_IE_HAN_DEVICE,                                 /* !< HAN Device */
    CMBS_IE_HAN_CFG,                                    /* !< HAN Init Config */
    CMBS_IE_HAN_NUM_OF_ENTRIES,                         /* !< Number of entries */
    CMBS_IE_HAN_INDEX_1ST_ENTRY,                        /* !< Index of first entry*/
    CMBS_IE_HAN_TABLE_ENTRY_TYPE,                       /* !< Is it a basic entry structure or an extended*/
    CMBS_IE_HAN_MSG_REG_INFO,                           /* !< Register info */
    CMBS_IE_HAN_TABLE_UPDATE_INFO,                      /* !< ULE Device table change */
    CMBS_IE_HAN_SEND_FAIL_REASON,
    CMBS_IE_HAN_TX_ENDED_REASON,


    CMBS_IE_HAN_DEVICE_REG_STAGE1_OK_STATUS_PARAMS,
    CMBS_IE_HAN_DEVICE_REG_STAGE2_OK_STATUS_PARAMS,

    CMBS_IE_HAN_DEVICE_FORCEFUL_DELETE_ERROR_STATUS_REASON,

    CMBS_IE_HAN_GENERAL_STATUS,

    CMBS_IE_HAN_UNKNOWN_DEVICE_CONTACTED_PARAMS,
    CMBS_IE_HAN_BASE_INFO,
    CMBS_IE_HAN_DEVICE_CONNECTION_STATUS,
    CMBS_IE_HAN_DEVICE_REG_ERROR_REASON,
    CMBS_IE_HAN_MSG_CONTROL,

    CMBS_IE_HAN_MAX

} E_CMBS_IE_HAN_TYPE;

/****************************** Home Area Network (start)**************************/
/*! \brief HAN message Type */
typedef enum
{
    CMBS_HAN_MSG_TYPE_CMD = 0x01,     /* Message is a command */
    CMBS_HAN_MSG_TYPE_CMD_WITH_RES,   /* Message is a command with response */
    CMBS_HAN_MSG_TYPE_CMD_RES,       /* Message is a command response */
    CMBS_HAN_MSG_TYPE_ATTR_GET,       /* Message is get attribute */
    CMBS_HAN_MSG_TYPE_ATTR_GET_RES,   /* Message is get attribute response */
    CMBS_HAN_MSG_TYPE_ATTR_SET,       /* Message is set attribute */
    CMBS_HAN_MSG_TYPE_ATTR_SET_WITH_RES,  /* Message is set attribute with response */
    CMBS_HAN_MSG_TYPE_ATTR_SET_RES,       /* Message is set attribute response */
    CMBS_HAN_MSG_TYPE_ATTR_GET_PACK,
    CMBS_HAN_MSG_TYPE_ATTR_GET_PACK_RES,
    CMBS_HAN_MSG_TYPE_ATTR_SET_PACK,
    CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_WITH_RES,
    CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_RES,
    CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_ATOMIC,
    CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_ATOMIC_WITH_RES,
    CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_ATOMIC_RES,

    CMBS_HAN_MSG_TYPE_MAX
} E_CMBS_HAN_MSG_TYPE;



/*! \brief HAN command response */
typedef enum
{
    CMBS_IE_HAN_MSG_CMD_RES_OK,
    CMBS_IE_HAN_MSG_CMD_RES_FAIL,

    CMBS_IE_HAN_MSG_CMD_RES_MAX
} E_CMBS_HAN_MSG_CMD_RES;


typedef enum
{
    CMBS_HAN_TABLE_DEVICE = 0x01,  //Device table
    CMBS_HAN_TABLE_BIND,           //Bind table
    CMBS_HAN_TABLE_GROUP,          //Group table
} E_CMBS_HAN_TABLE;

#define CMBS_HAN_MSG_GROUP_NAME_MAX_LEN 0x30

#define CMBS_HAN_DEVICE_TABLE_ENTRY_TYPE_BRIEF   0
#define CMBS_HAN_DEVICE_TABLE_ENTRY_TYPE_EXTENDED  1


#define CMBS_HAN_MAX_DEVICES   10 //todo: change for Scorpion constant
#define CMBS_HAN_MAX_BINDS    10 //todo: change for Scorpion constant
#define CMBS_HAN_MAX_GROUPS    10 //todo: change for Scorpion constant
#define CMBS_HAN_MAX_MSG_LEN   128 //todo: change for Scorpion constant
#define CMBS_HAN_MAX_MSG_DATA_LEN  15  //todo: change for Scorpion constant
#define CMBS_HAN_MAX_UNITS_IN_DEVICE  3  //todo: change for Scorpion constant
#define CMBS_HAN_MAX_OPTIONAL_INTERFACES_IN_UNIT 5  //todo: change for Scorpion constant

#define CMBS_HAN_DIRECTION_CLIENT_2_SERVER  (0)
#define CMBS_HAN_DIRECTION_SERVER_2_CLIENT  (1)

/*! \brief ST_HAN_CONFIG
HAN configuration structure. Configuration parameters for a HAN services for cmbs_dsr_han_mngr_Init() */
#define CMBS_HAN_EXTERNAL   1
#define CMBS_HAN_INTERNAL   0

#define CMBS_HAN_DEVICE_MNGR_EXT (1)
#define CMBS_HAN_BIND_MNGR_EXT  (1<<1)
#define CMBS_HAN_BIND_LOOKUP_EXT (1<<2)
#define CMBS_HAN_GROUP_MNGR_EXT  (1<<3)
#define CMBS_HAN_GROUP_LOOKUP_EXT (1<<4)

#define CMBS_HAN_DEVICE_MNGR(a)  (a&CMBS_HAN_DEVICE_MNGR_EXT)
#define CMBS_HAN_BIND_MNGR(a)  (a&CMBS_HAN_BIND_MNGR_EXT)
#define CMBS_HAN_BIND_LOOKUP(a)  (a&CMBS_HAN_BIND_LOOKUP_EXT)
#define CMBS_HAN_GROUP_MNGR(a)  (a&CMBS_HAN_GROUP_MNGR_EXT)
#define CMBS_HAN_GROUP_LOOKUP(a) (a&CMBS_HAN_GROUP_LOOKUP_EXT)
typedef struct
{
    u8 u8_HANServiceConfig;
    // Bit 0 Device Manager Ext (1) or Int (0)
    // Bit 1 Bind Manager Ext (1) or Int (0)
    // Bit 2 Bind Lookup Ext (1) or Int (0)
    // Bit 3 Group Manager Ext(1) or Int(0)
    // Bit 4 Group Lookup Ext(1) or Int(0)

} ST_HAN_CONFIG, * PST_HAN_CONFIG;

/*! \brief ST_HAN_MSG_REG_INFO
Parameter to be used with cmbs_dsr_han_msg_RecvRegister()
Used with the cmbs_dsr_han_msg_RecvRegister and cmbs_dsr_han_msg_RecvUnregister
Note: Device is not needed since Device=0 is assumed for Base Station
*/
typedef struct
{
    u8  u8_UnitId;          // 0 and 1 are reserved (Internal Units)
    u16 u16_InterfaceId; // 0xFFFF means All Interfaces
} ST_HAN_MSG_REG_INFO, * PST_HAN_MSG_REG_INFO;

typedef struct
{
    u8 u8_IPUI[5];
} ST_HAN_REG_STAGE_1_STATUS;

typedef struct
{
    u8 SetupType;
    u8 NodeResponse;
    u8 u8_IPUI[3];
} ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS;

#define CMBS_HAN_GENERAL_STATUS_OK   0
#define CMBS_HAN_GENERAL_STATUS_ERROR  1

typedef struct
{
    u16 u16_Status;
} ST_HAN_GENERAL_STATUS;

typedef struct
{
    u16 u16_UleVersion;
    u16 u16_FunVersion;
    u32 u32_OriginalDevicePagingInterval;
    u32 u32_ActualDevicePagingInterval;
} ST_HAN_REG_STAGE_2_STATUS;


#define CMBS_HAN_REG_FAILED_REASON_DECT_REG_FAILED      0x1
#define CMBS_HAN_REG_FAILED_REASON_PARAMS_NEGOTIATION_FAILURE  0x2
#define CMBS_HAN_REG_FAILED_REASON_COULD_NOT_FINISH_PVC_RESET   0x3
#define CMBS_HAN_REG_FAILED_REASON_DID_NOT_RECEIVE_FUN_MSG    0x4
#define CMBS_HAN_REG_FAILED_REASON_PROBLEM_IN_FUN_MSG     0x5
#define CMBS_HAN_REG_FAILED_REASON_COULD_NOT_ACK_FUN_MSG    0x6
#define CMBS_HAN_REG_FAILED_REASON_INTERNAL_AFTER_3RD_STAGE    0x7
#define CMBS_HAN_REG_FAILED_REASON_UNEXPECTED       0x8

#define CMBS_HAN_REG_FAILED_REASON_WRONG_ULE_VERSION  ( 0x1 << 2 )
#define CMBS_HAN_REG_FAILED_REASON_WRONG_FUN_VERSION  ( 0x2 << 2 )


/*! \brief ST_HAN_ADDRESS */
typedef struct
{
    u16     u16_DeviceId;
    u8      u8_UnitId;
    u8      u8_AddressType; //(0x00=Individual Address, 0x01=Group Address)
} ST_HAN_ADDRESS, * PST_HAN_ADDRESS;


#define CMBS_HAN_ULE_DEVICE_NOT_REGISTERED         0
#define CMBS_HAN_ULE_DEVICE_REGISTERED_1ST_PHASE       1
#define CMBS_HAN_ULE_DEVICE_REGISTERED_2ND_PHASE_PARAMETERS_NEGOTIATION  2
#define CMBS_HAN_ULE_DEVICE_REGISTERED_2ND_PHASE_PVC_RESET_COMPLETED  3
#define CMBS_HAN_ULE_DEVICE_REGISTERED_3RD_PHASE_RECEIVED_FUN_REGISTER  4
#define CMBS_HAN_ULE_DEVICE_REGISTERED_3RD_PHASE_SENT_FUN_REGISTER_RESP  5
#define CMBS_HAN_ULE_DEVICE_REGISTERED_3RD_PHASE       6

#define CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_INVALID_ID      1
#define CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_DEV_IN_MIDDLE_OF_REG   2
#define CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_BUSY       3
#define CMBS_HAN_DEVICE_DEREGISTRATION_ERROR_UNEXPECTED      4

#define  CMBS_HAN_ULE_DEVICE_CONNECTIONS_STATUS_NOT_IN_LINK_AND_NOT_REQUESTED 0
#define  CMBS_HAN_ULE_DEVICE_CONNECTIONS_STATUS_NOT_IN_LINK_BUT_REQUESTED  1
#define  CMBS_HAN_ULE_DEVICE_CONNECTIONS_STATUS_IN_LINK       2
#define  CMBS_HAN_ULE_DEVICE_CONNECTIONS_STATUS_NOT_REGISTERED     10

typedef struct
{
    u8  u8_UnitId;
    u16 u16_UnitType;
}
ST_HAN_BRIEF_UNIT_INFO;

typedef struct
{
    u16 u16_DeviceId;
    u8 u8_RegistrationStatus; /* CMBS_HAN_ULE_DEVICE_REGISTERED.... */
    u16 u16_RequestedPageTime;
    u16 u16_PageTime;  //final page time, after negotiation
    u8 u8_NumberOfUnits;
    ST_HAN_BRIEF_UNIT_INFO st_UnitsInfo[CMBS_HAN_MAX_UNITS_IN_DEVICE];
}
ST_HAN_BRIEF_DEVICE_INFO, *PST_HAN_BRIEF_DEVICE_INFO;

typedef struct
{
    u8  u8_UnitId;
    u16 u16_UnitType;
    u16  u16_NumberOfOptionalInterfaces;
    u16 u16_OptionalInterfaces[CMBS_HAN_MAX_OPTIONAL_INTERFACES_IN_UNIT];
}
ST_HAN_EXTENDED_UNIT_INFO;

typedef struct
{
    u16 u16_DeviceId;
    u8   u8_IPUI[5];
    u8 u8_RegistrationStatus; /* CMBS_HAN_ULE_DEVICE_REGISTERED.... */
    u16 u16_RequestedPageTime;
    u16 u16_DeviceEMC;
    u16 u16_PageTime;  //final page time, after negotiation
    u8 u8_NumberOfUnits;
    ST_HAN_EXTENDED_UNIT_INFO st_UnitsInfo[CMBS_HAN_MAX_UNITS_IN_DEVICE];
}
ST_HAN_EXTENDED_DEVICE_INFO, *PST_HAN_EXTENDED_DEVICE_INFO;


/*! \brief ST_HAN_DEVICE_ENTRY
Used in ST_IE_HAN_TABLE_ENTRIES

Note 1:
U16_DeviceId is the same as HandsetId used in the DECT registration

Note 2:
The list of Interfaces is implicitly known from the u16_UnitType
A unit of a certain type implements a well defined set of interfaces
In the future the API will add List of Interfaces so that a device can implement additional optional interfaces and inform about them
*/
typedef struct
{
    ST_HAN_ADDRESS  st_DeviceUnit;
    u16             u16_UnitType;
} ST_HAN_DEVICE_ENTRY, * PST_HAN_DEVICE_ENTRY;


/*! \brief ST_HAN_BIND_ENTRY
Used in ST_IE_HAN_TABLE_ENTRIES
*/
typedef struct
{
    u16  u16_SrcDeviceID;
    u8    u8_SrcUnitID;
    u16  u16_SrcInterfaceID;
    u8   u8_SrcInterfaceType;
    u16  u16_DstDeviceID;
    u8   u8_DstUnitID;
    u8   u8_AddressType;
} ST_HAN_BIND_ENTRY, * PST_HAN_BIND_ENTRY;


/*! \brief ST_HAN_GROUP_ENTRY
Used in ST_IE_HAN_TABLE_ENTRIES
*/
typedef struct
{
    u8              u8_GroupId;
    u8              u8_GroupName[CMBS_HAN_MSG_GROUP_NAME_MAX_LEN];
    ST_HAN_ADDRESS  st_DeviceUnit;
} ST_HAN_GROUP_ENTRY, * PST_HAN_GROUP_ENTRY;

/*! \brief ST_HAN_MSG_CONTROL
 Control flags for message
*/
typedef struct
{
    u16   u16_Reserved;
} ST_HAN_MSG_TRANSPORT, * PST_HAN_MSG_TRANSPORT;



#define  CMBS_HAN_SEND_MSG_REASON_DATA_TOO_BIG_ERROR     0
#define  CMBS_HAN_SEND_MSG_REASON_DEVICE_NOT_IN_LINK_ERROR   1
#define  CMBS_HAN_SEND_MSG_REASON_TRANSMIT_FAILED_ERROR    2
#define  CMBS_HAN_SEND_MSG_REASON_DECT_ERROR       3
#define  CMBS_HAN_SEND_MSG_REASON_BUSY_WITH_PREVIOUS_MESSAGES  4
#define    CMBS_HAN_SEND_MSG_REASON_INVALID_DST_DEVICE_LIST    5
#define    CMBS_HAN_SEND_MSG_REASON_NO_TX_REQUEST      6
#define    CMBS_HAN_SEND_MSG_REASON_UNKNOWN_ERROR      7

#define  CMBS_HAN_TX_ENDED_REASON_LINK_FAILED_ERROR    1 //LINK_MANAGER_SERVICE_LINK_FAILED, Device not contacted during the time it was supposed to contact
#define  CMBS_HAN_TX_ENDED_REASON_LINK_DROPPED_ERROR    2 //LINK_MANAGER_SERVICE_LINK_DROPPED, Link terminated due to host inacctivity. Happens after a timeout

/*! \brief ST_IE_HAN_MSG
IE to hold HAN message structure for CMBS_IE_HAN_MSG

Send using binding table:
If u16_DstDevice=0xFFFF and u8_DstUnit=0xFF, then binding table will be used to determine the destination
*/
typedef  struct
{
    u16        u16_SrcDeviceId;
    u8         u8_SrcUnitId;
    u16        u16_DstDeviceId;
    u8         u8_DstUnitId;
    u8         u8_DstAddressType; //(0x00=Individual Address, 0x01=Group Address)

    ST_HAN_MSG_TRANSPORT st_MsgTransport;    // transport layer (Encryption, Reliable Connection, etc.)
    u8                   u8_MsgSequence;     // will be returned in the response
    u8                   e_MsgType;          // message type
    u8                   u8_InterfaceType;   // 1=server, 0=client
    u16                  u16_InterfaceId;
    u8                   u8_InterfaceMember; // depending on message type, Command or attribute id or attribute pack id
    u16                  u16_DataLen;
    u8 *                 pu8_Data;
} ST_IE_HAN_MSG , * PST_IE_HAN_MSG;


#define CMBS_HAN_IE_TABLE_SIZE_1 MAX(CMBS_HAN_MAX_DEVICES*sizeof(ST_HAN_BRIEF_DEVICE_INFO),CMBS_HAN_MAX_BINDS*sizeof(ST_HAN_BIND_ENTRY))
#define CMBS_HAN_IE_TABLE_SIZE  MAX(CMBS_HAN_IE_TABLE_SIZE_1,CMBS_HAN_MAX_GROUPS*sizeof(ST_HAN_GROUP_ENTRY))


/*! \brief ST_IE_HAN_CONFIG
  IE structure for ST_HAN_CONFIG
*/
typedef struct
{
    ST_HAN_CONFIG st_HanCfg; // HAN cfg
} ST_IE_HAN_CONFIG, *PTS_IE_HAN_CONFIG;


/*! \brief ST_IE_HAN_BASE_INFO
  IE structure for ST_IE_HAN_BASE_INFO
*/
typedef struct
{
    u8 u8_UleAppProtocolId; // ULE protocol ID
    u8  u8_UleAppProtocolVersion;  // ULE protocol version
} ST_IE_HAN_BASE_INFO, *PST_IE_HAN_BASE_INFO;

/*! \brief ST_IE_HAN_DEVICE_ENTRIES
IE structure for CMBS_IE_HAN_DEVICE_ENTRIES
*/
typedef struct
{
    u16                      u16_NumOfEntries;  // number of entries in this struct
    u16                      u16_StartEntryIndex; // Index of first Entry in this struct
    ST_HAN_BRIEF_DEVICE_INFO*   pst_DeviceEntries;     // points to beginning of array
} ST_IE_HAN_BRIEF_DEVICE_ENTRIES, * PST_IE_HAN_BRIEF_DEVICE_ENTRIES, ST_IE_HAN_BRIEF_DEVICE_TABLE, * PST_IE_HAN_BRIEF_DEVICE_TABLE;

/*! \brief ST_IE_HAN_DEVICE_ENTRIES
IE structure for CMBS_IE_HAN_DEVICE_ENTRIES
*/
typedef struct
{
    u16                      u16_NumOfEntries;  // number of entries in this struct
    u16                      u16_StartEntryIndex; // Index of first Entry in this struct
    ST_HAN_EXTENDED_DEVICE_INFO*   pst_DeviceEntries;     // points to beginning of array
} ST_IE_HAN_EXTENDED_DEVICE_ENTRIES, * PST_IE_HAN_EXTENDED_DEVICE_ENTRIES, ST_IE_HAN_EXTENDED_DEVICE_TABLE, * PST_IE_HAN_EXTENDED_DEVICE_TABLE;

/*! \brief ST_IE_HAN_BIND_ENTRIES
IE structure for CMBS_IE_HAN_BIND_ENTRIES
*/
typedef struct
{
    u16                     u16_NumOfEntries;  // number of entries in this struct
    u16                     u16_StartEntryIndex; // Index of first Entry in this struct
    ST_HAN_BIND_ENTRY *     pst_BindEntries;     // points to beginning of array
} ST_IE_HAN_BIND_ENTRIES, * PST_IE_HAN_BIND_ENTRIES, ST_IE_HAN_BIND_TABLE, * PST_IE_HAN_BIND_TABLE;

/*! \brief ST_IE_HAN_GROUP_ENTRIES
IE structure for CMBS_IE_HAN_GROUP_ENTRIES
*/
typedef struct
{
    u16                     u16_NumOfEntries;  // number of entries in this struct
    u16                     u16_StartEntryIndex; // Index of first Entry in this struct
    ST_HAN_GROUP_ENTRY *    pst_GroupEntries;     // points to beginning of array
} ST_IE_HAN_GROUP_ENTRIES, * PST_IE_HAN_GROUP_ENTRIES, ST_IE_HAN_GROUP_TABLE, * PST_IE_HAN_GROUP_TABLE;


typedef  struct
{
    E_CMBS_HAN_TABLE    e_Table; // The table that was updated
} ST_IE_HAN_TABLE_UPDATE_INFO, * PST_IE_HAN_TABLE_UPDATE_INFO;


/*! \brief CMBS_IE_HAN_MSG_CONTROL
*/
typedef struct
{

u8  ImmediateSend  :
    1;  // TX request was not submmited before
u8  IsLast   :
    1;  // Release link after confirmtion
u8   Reserved   :
    6;

} ST_IE_HAN_MSG_CTL, * PST_IE_HAN_MSG_CTL;

/****************************** Home Area Network (End)****************************/


/************************************************************************************************
*
*
*       HAN Functions
*
*
*************************************************************************************************/
//*/
//    ==========  cmbs_dsr_han_mngr_Init  ===========
/*!
      \brief
         Initializes the HAN Manager and defines the Services behavior.

         <h2>cmbs_dsr_han_mngr_Init</h2>

      <h3>Introduction</h3>
         Initializes the HAN Manager and defines the Services behavior.
         The configuration defines:
         1. If Device Service is implemented externally or using the internal implementation.
         2. If Group Service Management is implemented externally or internally.
                If Group Service is Internal the Group Lookup is also internal but
                if Group service is external, the Group Lookup can be either external or internal (using cached Group Table).
         3. If Bind Service Management is implemented externally or internally.
                If Bind Service is Internal the Bind Lookup is also internal but
                if Bind service is external, the Bind Lookup can be either external or internal (using cached Binding Table).

      <h3>Use cases</h3>
         Initialize the HAN Manager. Should be called before cmbs_dsr_han_mngr_Start.

      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_mngr_Init (void *pv_AppRefHandle, ST_HAN_CONFIG * pst_HANConfig)
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle  received in cmbs_api_registerCB()
      \param[in,out]    pst_HANConfig       pointer to ST_HAN_CONFIG structure

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_MNGR_INIT_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_mngr_Init(void *pv_AppRefHandle, ST_HAN_CONFIG * pst_HANConfig);

//*/
//    ==========  cmbs_dsr_han_mngr_Start  ===========
/*!
      \brief
         Start the HAN Manager.

         <h2>cmbs_dsr_han_mngr_Start</h2>

      <h3>Introduction</h3>
         Start the HAN Manager which in turn starts the other HAN services (Group Service, Bind Service and Device Service, Message Service).

      <h3>Use cases</h3>
         Start the HAN Manager after Initializing the manger itself.

      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_mngr_Start (void *pv_AppRefHandle);
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle  received in cmbs_api_registerCB()

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_MNGR_START_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_mngr_Start(void *pv_AppRefHandle);


//*/
//    ==========  cmbs_dsr_han_device_ReadTable  ===========
/*!
      \brief
         Read entries from the internal Device table.

         <h2>cmbs_dsr_han_device_ReadTable</h2>

      <h3>Introduction</h3>
        This functions is used for reading the Devices Registered with the Device Management Service.
        Each entry holds the Device address and its Units.
        A Unit is a functional instance of specific Unit Type and supports a list of interfaces of this Unit Type.

        Note: If the Device Management is implemented externally, then this function should not be used and will return Error.

      <h3>Use cases</h3>
         When Device Management Service is implemented internally, this function returns the Device Table to the Host.

      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_device_ReadTable (void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);
         </b><br><br>

      \param[in]        pv_AppRefHandle         reference pointer to AppRefHandle  received in cmbs_api_registerCB()

      \param[in]        u16_NumOfEntries        Num of entries requested

      \param[in]        u16_IndexOfFirstEntry   First entry to return (index of first entry, e.g. if entries 5-10 are requested, u16_IndexOfFirstEntry=5)

      \param[in]        bool_IsBrief      Is the required table should be short (brief) or full (extended)

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_device_ReadTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8 bool_IsBrief);


//*/
//    ==========  cmbs_dsr_han_device_WriteTable  ===========
/*!
      \brief
         Write entries to the internal Device table.

         <h2>cmbs_dsr_han_device_WriteTable</h2>

      <h3>Introduction</h3>
        This functions is used for writing updated Device Table into the Management Service.
        Note: If the Device Management is implemented externally, then this function should not be used and will return Error.

      <h3>Use cases</h3>
         When Device Management Service is implemented internally but Non Volatile memory is not available, this function can be called upon startup to update Device Table.

      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_device_WriteTable (void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_DEVICE_ENTRY * pst_HANDeviceEntriesArray);
         </b><br><br>

      \param[in]        pv_AppRefHandle             reference pointer to AppRefHandle  received in cmbs_api_registerCB()

      \param[in]        u16_NumOfEntries            Num of entries written

      \param[in]        u16_IndexOfFirstEntry       First entry to write (index of first entry, e.g. if entries 5-10 are written, u16_IndexOfFirstEntry=5)

      \param[in]        pst_HANDeviceEntriesArray   Array of entries to write


      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_device_WriteTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_DEVICE_ENTRY * pst_HANDeviceEntriesArray);


//*/
//    ==========  cmbs_dsr_han_bind_ReadTable  ===========
/*!
      \brief
         Read entries from the internal Bind table.

         <h2>cmbs_dsr_han_bind_ReadTable</h2>

      <h3>Introduction</h3>
        This functions is used for reading the Binding Table from the Bind Service.
        Each entry holds Client Device-Unit-Interface & Server Device-Unit-Interface pairs

        Note: If the Bind Lookup is implemented externally, then this function should not be used and will return Error.

      <h3>Use cases</h3>
         This function should be called when Host wants to read the binding table.

      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_bind_ReadTable (void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);
         </b><br><br>

      \param[in]        pv_AppRefHandle         reference pointer to AppRefHandle  received in cmbs_api_registerCB()

      \param[in]        u16_NumOfEntries        Num of entries requested

      \param[in]        u16_IndexOfFirstEntry   First entry to return (index of first entry, e.g. if entries 5-10 are requested, u16_IndexOfFirstEntry=5)

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_BIND_READ_TABLE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_bind_ReadTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);


//*/
//    ==========  cmbs_dsr_han_bind_WriteTable  ===========
/*!
      \brief
         Write entries to the internal Bind table.

         <h2>cmbs_dsr_han_bind_WriteTable</h2>

      <h3>Introduction</h3>
        This functions is used for writing the Binding Table into the Bind Service
        Each entry holds Client Device-Unit-Interface & Server Device-Unit-Interface pairs

      <h3>Use cases</h3>
        If the Bind Management is implemented externally and the Bind Lookup is implemented internally,
        this function must be called so the internal bind lookup will work correctly
      <h3>API Functions description</h3>
         <b>
        E_CMBS_RC cmbs_dsr_han_bind_WriteTable (void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_BIND_ENTRY * pst_HANBindEntriesArray);
         </b><br><br>

      \param[in]        pv_AppRefHandle             reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_NumOfEntries            Num of entries written

      \param[in]        u16_IndexOfFirstEntry       First entry to write (index of first entry, e.g. if entries 5-10 are written, u16_IndexOfFirstEntry=5)

      \param[in]        pst_HANBindEntriesArray     Array of entries to write


      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_BIND_WRITE_TABLE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_bind_WriteTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_BIND_ENTRY * pst_HANBindEntriesArray);

//*/
//    ==========  cmbs_dsr_han_group_ReadTable  ===========
/*!
      \brief
         Read entries from the internal Group table.

         <h2>cmbs_dsr_han_group_ReadTable</h2>

      <h3>Introduction</h3>
        This functions is used for reading the Group Table from the Group Service.
        Each entry holds Group Id, Group Name, list of Device-Unit which belongs to this group.

        Note: If the Group Lookup is implemented externally, then this function should not be used and will return Error.

      <h3>Use cases</h3>
         This function should be called when Host wants to read the group table.

      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_group_ReadTable (void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);
         </b><br><br>

      \param[in]        pv_AppRefHandle         reference pointer to AppRefHandle  received in cmbs_api_registerCB()

      \param[in]        u16_NumOfEntries        Num of entries requested

      \param[in]        u16_IndexOfFirstEntry   First entry to return (index of first entry, e.g. if entries 5-10 are requested, u16_IndexOfFirstEntry=5)

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_GROUP_READ_TABLE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_group_ReadTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry);


//*/
//    ==========  cmbs_dsr_han_group_WriteTable  ===========
/*!
      \brief
         Write entries to the internal Group table.

         <h2>cmbs_dsr_han_group_WriteTable</h2>

      <h3>Introduction</h3>
        This functions is used for writing the Group Table into the Group Service.
        Each entry holds Group ID, Group Name and a list of Device-Unit belonging to this group.

      <h3>Use cases</h3>
        If the Group Management is implemented externally and the Group Lookup is implemented internally,
        this function must be called so the internal group lookup will work correctly
      <h3>API Functions description</h3>
         <b>
        E_CMBS_RC cmbs_dsr_han_group_WriteTable (void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_GROUP_ENTRY * pst_HANGroupEntriesArray);
         </b><br><br>

      \param[in]        pv_AppRefHandle             reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_NumOfEntries            Num of entries written

      \param[in]        u16_IndexOfFirstEntry       First entry to write (index of first entry, e.g. if entries 5-10 are written, u16_IndexOfFirstEntry=5)

      \param[in]        pst_HANGroupEntriesArray    Array of entries to write


      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_group_WriteTable(void *pv_AppRefHandle, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, ST_HAN_GROUP_ENTRY * pst_HANGroupEntriesArray);


//*/
//    ==========  cmbs_dsr_han_msg_RecvRegister  ===========
/*!
      \brief
         Register Units and interfaces with the Message Service.

         <h2>cmbs_dsr_han_msg_RecvRegister</h2>

      <h3>Introduction</h3>
        This functions is used for registering a Device(0)-Unit(yyy)-Interface(zzz) with the Message Service so any message coming to Device 0 Unit yyy interface zzz will be sent to the Host.
        This allows Host to implement a Unit and interfaces and to exchange messages with other units.
        Example 1:
        In a typical implementation, Host should Implement Unit=2 and register all interfaces of Unit 2 with the Message service.
        Any Message destined to Unit 2 will be sent to Host.
        Also, it should add to the Binding Table a binding to Unit=2 Interfaces=All
        Any message coming form any of the devices with no specified destination (Device-Unit) will automatically sent to the Host.

        Example 2:
        If Host wants to implement Device Management then it should register with:
        Device=0, Unit=0,Interface=Device Management Server Interface
        If Host wants to implement Group Management then it should register with:
        Device=0, Unit=0,Interface=Group Management Server Interface
        If Host wants to implement Bind Management then it should register with:
        Device=0, Unit=0,Interface=Bind Management Server Interface

      <h3>Use cases</h3>
        This function is used for registering Units and interfaces with the Message Service
      <h3>API Functions description</h3>
         <b>
        E_CMBS_RC cmbs_dsr_han_msg_RecvRegister (void *pv_AppRefHandle, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        pst_HANMsgRegInfo   HAN Message Registration information structure

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_MSG_RECV_REGISTER_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_msg_RecvRegister(void *pv_AppRefHandle, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);


//*/
//    ==========  cmbs_dsr_han_msg_RecvUnregister  ===========
/*!
      \brief
         Unregister Units and interfaces with the Message Service.

         <h2>cmbs_dsr_han_msg_RecvUnregister</h2>

      <h3>Introduction</h3>
        This functions is used for unregistering a Device(0)-Unit(yyy)-Interface(zzz) in the Message Service.

      <h3>Use cases</h3>
        This function is used for unregistering Units and interfaces from the Message Service
      <h3>API Functions description</h3>
         <b>
            E_CMBS_RC cmbs_dsr_han_msg_RecvUnregister (void *pv_AppRefHandle, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        pst_HANMsgRegInfo   HAN Message Registration information structure

      \return           Return Code

      \see
      CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_msg_RecvUnregister(void *pv_AppRefHandle, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);


//*/
//    ==========  cmbs_dsr_han_msg_SendTxRequest  ===========
/*!
      \brief
            Request to get notified when Device is in reach and can receive messages.

         <h2>cmbs_dsr_han_msg_SendTxRequest</h2>

      <h3>Introduction</h3>
        When the Host wants to send a message to a Device, Since this Device maybe unreachable some of the time,
        this function allows the Host to request that when a Device is in reach (when it sends something to Base for example)
         that the Link will remain open and the Message Service will notify Host that the Device is available.

      <h3>Use cases</h3>
        This function is used for requesting to get notified when Device is in reach and can receive messages.
      <h3>API Functions description</h3>
         <b>
        E_CMBS_RC cmbs_dsr_han_msg_SendTxRequest (void *pv_AppRefHandle, u16 u16_DeviceId);
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_DeviceId        DeviceId, 0xFF=Means Any Device

      \return           Return Code

      \see
        CMBS_EV_DSR_HAN_MSG_SEND_TX_RES
        CMBS_EV_DSR_HAN_MSG_SEND_TX_READY

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_msg_SendTxRequest(void *pv_AppRefHandle, u16 u16_DeviceId);


//*/
//    ==========  cmbs_dsr_han_msg_SendTxEnd  ===========
/*!
      \brief
        This functions informs the Message Service that the link to a specific Device is not required anymore.
         <h2>cmbs_dsr_han_msg_SendTxEnd</h2>

      <h3>Introduction</h3>
        This functions informs the Message Service that the link to a specific Device is not required anymore.

      <h3>Use cases</h3>
        After the Host sends all messages it needs to a specific Device,
        this function should be called so that the link with the Device will be released (Usually to save Battery).

      <h3>API Functions description</h3>
         <b>
        E_CMBS_RC cmbs_dsr_han_msg_SendTxEnd(void *pv_AppRefHandle, u16 u16_DeviceId);
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_DeviceId        DeviceId, 0xFF=Means Any Device

      \return           Return Code

      \see
        CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_msg_SendTxEnd(void *pv_AppRefHandle, u16 u16_DeviceId);

//*/
//    ==========  cmbs_dsr_han_msg_Send  ===========
/*!
      \brief
        This function sends a message to any Device-Unit-interface

         <h2>cmbs_dsr_han_msg_Send</h2>

      <h3>Introduction</h3>
        This function sends a message to any Device-Unit-interface

      <h3>Use cases</h3>
        This function is used for sending messages to other units.
        Note: The CMBS_EV_DSR_HAN_MSG_SEND_RES will include the Request Id that the application put in the message.
        A successful response means that the message was sent successfully to the other Device.

      <h3>API Functions description</h3>
         <b>
        E_CMBS_RC cmbs_dsr_han_msg_Send (void *pv_AppRefHandle, u16 u16_RequestId, ST_IE_HAN_MSG * pst_HANMsg);
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_RequestId       Host should put unique values as this will be return in the response

      \param[in]        pst_HANMsg          pointer to HAN Message structure

      \return           Return Code

      \see
        CMBS_EV_DSR_HAN_MSG_SEND_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_msg_Send(void *pv_AppRefHandle, u16 u16_RequestId, PST_IE_HAN_MSG_CTL pst_MsgCtrl, ST_IE_HAN_MSG * pst_HANMsg);

// ==========  cmbs_dsr_han_device_Delete  ===========
/*!
      \brief
        This function deletes a ULE/HAN Device

         <h2>cmbs_dsr_han_device_Delete</h2>

      <h3>Introduction</h3>
        This function sends a request to the target to delete a registered ULE/HAN device

      <h3>Use cases</h3>
        This function is used for deleting registered ULE/HAN Devices
        This function has no impact on registered non-ULE devices ( handsets )
        This function is a wrapper for sending CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE to the target
  CMBS_EV_DSR_HAN_MSG_SEND_RES will be returned with the status of the deletion

      <h3>API Functions description</h3>
         <b>
  E_CMBS_RC cmbs_dsr_han_device_Delete (void *pv_AppRefHandle, u16 u16_DeviceId )
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_DeviceId       Host should put a DeviceId in question

      \return           Return Code

      \see
        CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE
        CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_device_Delete(void *pv_AppRefHandle, u16 u16_DeviceId);

// ==========  cmbs_dsr_han_device_GetConnectionStatus  ===========
/*!
      \brief
        This function returns whether a specific device is in link or is expected to be in link any soon

         <h2>cmbs_dsr_han_device_GetConnectionStatus</h2>

      <h3>Introduction</h3>
        This function sends a request to the target to to see check if the device is currently in link or was
        was requested to contact soon

      <h3>Use cases</h3>
        This function is used for checking device connection status ( for instance, prior to sending a message to the device )
        This function has no impact on registered non-ULE devices ( handsets )
        This function is a wrapper for sending CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS to the target
  CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS_RES will be returned with the status of the connection

      <h3>API Functions description</h3>
         <b>
  E_CMBS_RC cmbs_dsr_han_device_GetConnectionStatus (void *pv_AppRefHandle, u16 u16_DeviceId )
         </b><br><br>

      \param[in]        pv_AppRefHandle     reference pointer to AppRefHandle received in cmbs_api_registerCB()

      \param[in]        u16_DeviceId       Host should put a DeviceId in question

      \return           Return Code

      \see
        CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE
        CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE_RES

      <b>Sample Code:</b><br><br>
      <code></code>
*/
E_CMBS_RC cmbs_dsr_han_device_GetConnectionStatus(void *pv_AppRefHandle, u16 u16_DeviceId);


#endif //CMBS_HAN_H
/****************************[End Of File]****************************************************************************************************/
