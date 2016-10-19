/*******************************************************************
*
*    DESCRIPTION: Definitions of HAN FUN Protocol elements
*
*    AUTHOR: CMBS Team
*
*    © DSP Group Ltd 2011
*
*******************************************************************/
#ifndef HANFUN_PROTOCOL_H
#define HANFUN_PROTOCOL_H

/*
   Bind Management Interface ID
====================================
This interface is used for binding Device-Unit-Interface Client with Device-Unit-Interface Server
*/
#define BIND_MGMT_INTERFACE_ID    0x0002

/* Bind Management Interface - Server */
/* Attributes */
/* Commands */
#define BIND_MGMT_IF_SERVER_CMD_ID_ADD_BIND         0x0001
#define BIND_MGMT_IF_SERVER_CMD_ID_REMOVE_BIND      0x0002


/* Bind Management Interface - Client */
/* Attributes */
/* Commands */



/*
   Group Management Interface ID
====================================
This interface is used for adding Device-Unit to group
*/
#define GROUP_MGMT_INTERFACE_ID    0x0003

/* Group Management Interface - Server */
/* Attributes */
/* Commands */
#define GROUP_MGMT_IF_SERVER_CMD_ID_CREATE_GROUP            0x0001
#define GROUP_MGMT_IF_SERVER_CMD_ID_DELETE_GROUP            0x0002
#define GROUP_MGMT_IF_SERVER_CMD_ID_ADD_TO_GROUP            0x0003
#define GROUP_MGMT_IF_SERVER_CMD_ID_REMOVE_FROM_GROUP       0x0004
#define GROUP_MGMT_IF_SERVER_CMD_ID_GET_GROUP_INFO          0x0005


/*
   Alert Interface ID
====================================
*/
#define  ALERT_INTERFACE_ID  0x0100
/* Alert Interface - Server */
/* Attributes */
/* Commands */
#define  ALERT_IF_SERVER_CMD_ALERT_OFF     0x0000
#define  ALERT_IF_SERVER_CMD_ALERT_ON     0x0001

#define  ALERT_IF_SERVER_ATTR_ALERT_STATE    0x0001


/*
   On-Off Interface ID
====================================
*/
#define  ON_OFF_INTERFACE_ID  0x0200

#define  ON_OFF_IF_SERVER_CMD_TURN_ON     0x0001
#define  ON_OFF_IF_SERVER_CMD_TURN_OFF     0x0002
#define  ON_OFF_IF_SERVER_CMD_TOGGLE      0x0003

#define  ON_OFF_IF_SERVER_ATTR_STATE      0x0001
/* Group Management Interface - Client */
/* Attributes */
/* Commands */


/*
   Device Information Interface ID
====================================
*/
#define DEVICE_INFORMATION_INTERFACE_ID  0x5

#define DEV_INF_IF_ATTR_HF_RELEASE_VER  0x0001
#define DEV_INF_IF_ATTR_PROF_RELEASE_VER 0x0002
#define DEV_INF_IF_ATTR_IF_RELEASE_VER  0x0003
#define DEV_INF_IF_ATTR_PAGING_CAPS   0x0004
#define DEV_INF_IF_ATTR_MIN_SLEEP_TIME  0x0005
#define DEV_INF_IF_ATTR_ACT_RESP_TIME  0x0006
#define DEV_INF_IF_ATTR_APPL_VER   0x0007
#define DEV_INF_IF_ATTR_HW_VER    0x0008
#define DEV_INF_IF_ATTR_EMC     0x0009
#define DEV_INF_IF_ATTR_IPUE    0x000A
#define DEV_INF_IF_ATTR_MANUF_NAME   0x000B
#define DEV_INF_IF_ATTR_LOCATION   0x000C
#define DEV_INF_IF_ATTR_DEV_ENABLE   0x000D
#define DEV_INF_IF_ATTR_FRIENDLY_NAME  0x000E

#define CMBS_HAN_DEV_INFO_RESP_VAL_OK        0x00
#define CMBS_HAN_DEV_INFO_RESP_VAL_FAIL_NOT_SUPPORTED    0x03
#define CMBS_HAN_DEV_INFO_RESP_VAL_FAIL_UNKNOWN      0xFF

/*
   Attribute Reporting Interface ID
====================================
*/
#define ATTRIBUTE_REPORTING_INTERFACE_ID  0x6

#define CMBS_HAN_ATTR_PACK_TYPE_ALL_MANDATORY      0x00
#define CMBS_HAN_ATTR_PACK_TYPE_ALL_MANDATORY_AND_OPT    0xFE
#define CMBS_HAN_ATTR_PACK_TYPE_DYNAMIC        0xFF

#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_COV      0x00
#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_HT       0x01
#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_LH       0x02
#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_EQUAL      0x03

#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_COV      0x00
#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_HT       0x01
#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_LH       0x02
#define CMBS_HAN_ATTR_REPORT_REPORT_TYPE_EQUAL      0x03

#define CMBS_HAN_ATTR_REPORT_ATTRIB_ID_NUM_OF_ENTRIES    0x01
#define CMBS_HAN_ATTR_REPORT_ATTRIB_ID_NUM_OF_PERIODIC_ENTRIES  0x02
#define CMBS_HAN_ATTR_REPORT_ATTRIB_ID_NUM_OF_EVENT_ENTRIES   0x03

#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_CREATE_PERIODIC_REP    0x01
#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_CREATE_EVENT_REP    0x02
#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_PERIODIC_REP   0x03
#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_EVENT_REP    0x04
#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_DELETE_REP      0x05
#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_GET_PERIODIC_REP_ENTRIES  0x06
#define CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_GET_EVENT_REP_ENTRIES   0x07

#define CMBS_HAN_ATTR_REPORT_S2C_CMD_ID_PERIODIC_REPORT_NOTIFICATION 0x1
#define CMBS_HAN_ATTR_REPORT_S2C_CMD_ID_EVENT_REPORT_NOTIFICATION  0x2

#define CMBS_HAN_ATTR_REPORT_RESP_VAL_OK       0x00
#define CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_NOT_AUTHORIZED   0x01
#define CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_INV_ARG     0x02
#define CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_NOT_ENOUGH_RES   0xFE
#define CMBS_HAN_ATTR_REPORT_RESP_VAL_FAIL_UNKNOWN     0xFF



#define  ON_OFF_IF_SERVER_ATTR_STATE      0x0001
/* Group Management Interface - Client */
/* Attributes */
/* Commands */




/*
   Keep-Alive Interface ID
====================================
*/
#define  KEEP_ALIVE_INTERFACE_ID  0x0115


/* Group Management Interface - Client */
/* Attributes */
/* Commands */



#endif // HANFUN_PROTOCOL_H