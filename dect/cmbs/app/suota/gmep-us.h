/*
 * GMEP US API: H file
 */
#ifndef _GMEP_US_
#define _GMEP_US_


#ifndef CMBS_API
#include "stdtypes.h"
#else
#include "cmbs_api.h"
#endif

#define LOG_GMEP_US
#ifdef LOG_GMEP_US
#define GMEP_LOG(fmt, args...) printf("\nGMEP-US : %s(): " fmt , __FUNCTION__,##args)
#else
#define GMEP_LOG(fmt, args...)
#endif

#define MAX_SUBSCRIBERS  1
#define GMEP_MAX_SESSIONS  ( 5 * MAX_SUBSCRIBERS )
#define MAX_SW_VER_ID_SIZE 20
#define MSG_BUF_SIZE     4096 //2048  //4096
#define MAX_URL_SIZE    100 //30
#define GMEPD_DIR "/tmp/gmepd"
#define GMPED_APP_FIFO_CLIENT_FMT_LEN 100
#define GMEPD_APP_FIFO_CLIENT_FMT "gmep -%05d"
#define GMEPD_APP_FIFO            GMEPD_DIR"/gmepd"

#define  GMEPD_APP_REGISTER    101
#define     GMEPD_APP_UNREGISTER  202

#define CMBS_SUOTA_PATH_MAX 100
#define CMBS_SUOTA_NAME_MAX 100

typedef enum
{
    GMEP_NO_APPLICATION,
    GMEP_BINARY_CONTENT_DWLD,
    GMEP_SOFTWARE_UPGRADE
} t_GmepApp;

#if 0


typedef enum
{
    SUOTA_HS_VER_IND,
    SUOTA_HS_VER_AVAIL,
    SUOTA_HS_URL_IND,
    SUOTA_NACK
} t_Suota_Cmd;

#endif



#if 0

typedef enum
{
    SUOTA_SESSION_CREATE = 1,
    SUOTA_OPEN_SESSION,
    SUOTA_DATA_SEND,
    SUOTA_DATA_SEND_ACK,
    SUOTA_DATA_RECV,
    SUOTA_HS_VER_IND,
    SUOTA_HS_VER_AVAIL,
    SUOTA_SESSION_CLOSE,
    SUOTA_URL_IND,
    SUOTA_NACK
} t_SuotaCmd;

#endif

#ifndef CMBS_API
typedef enum
{
    GMEP_US_SESSION_CREATE = 1,
    GMEP_US_SESSION_CREATE_ACK,
    GMEP_US_OPEN_SESSION,
    GMEP_US_OPEN_SESSION_ACK,
    GMEP_US_DATA_SEND,
    GMEP_US_DATA_SEND_ACK,
    GMEP_US_REG_CPLANE_CB,
    GMEP_US_REG_CPLANE_CB_ACK,
    GMEP_US_REG_APP_CB,
    GMEP_US_REG_APP_CB_ACK,
    GMEP_US_DATA_RECV,
    GMEP_US_DATA_RECV_ACK,
    GMEP_US_HS_VER_IND,
    GMEP_US_HS_VER_IND_ACK,
    GMEP_US_HS_AVAIL,
    GMEP_US_HS_AVAIL_ACK,
    GMEP_US_SESSION_CLOSE,
    GMEP_US_SESSION_CLOSE_ACK,
    GMEP_US_URL_IND,
    GMEP_US_URL_IND_ACK,
    GMEP_US_NACK,
    GMEP_US_NACK_ACK,
    GMEP_US_CONTROL_SET,
    GMEP_US_CONTROL_SET_ACK,
    GMEP_US_COTROL_RESET,
    GMEP_US_COTROL_RESET_ACK,
    GMEP_US_UPDATE_OPTIONAL_GRP,
    GMEP_US_UPDATE_OPTIONAL_GRP_ACK,
    GMEP_US_FACILITY_CB,
    GMEP_US_PUSH_MODE
} t_GmepCmd;
#else
typedef enum
{
//C-plane commands
    GMEP_US_HS_VER_IND = CMBS_EV_DSR_SUOTA_HS_VERSION_RECEIVED,
    GMEP_US_URL_IND = CMBS_EV_DSR_SUOTA_URL_RECEIVED,      // HS to Host
    GMEP_US_NACK_FROM_HS = CMBS_EV_DSR_SUOTA_NACK_RECEIVED,   // HS to Host,  NEW
    GMEP_US_PUSH_MODE = CMBS_EV_DSR_SUOTA_SEND_SW_UPD_IND,   // host to HS
    GMEP_US_PUSH_MODE_ACK = CMBS_EV_DSR_SUOTA_SEND_SW_UPD_IND_RES,   //NEW
    GMEP_US_HS_AVAIL =  CMBS_EV_DSR_SUOTA_SEND_VERS_AVAIL,
    GMEP_US_HS_AVAIL_ACK =  CMBS_EV_DSR_SUOTA_SEND_VERS_AVAIL_RES,
    GMEP_US_SEND_URL_IND = CMBS_EV_DSR_SUOTA_SEND_URL,      //host to HS, NEW
    GMEP_US_URL_IND_ACK = CMBS_EV_DSR_SUOTA_SEND_URL_RES,
    GMEP_US_NACK = CMBS_EV_DSR_SUOTA_SEND_NACK,        // host to HS
    GMEP_US_NACK_ACK = CMBS_EV_DSR_SUOTA_SEND_NACK_RES,


//U-plane commands
    GMEP_US_SESSION_CREATE = CMBS_EV_DSR_SUOTA_SESSION_CREATE,
    GMEP_US_SESSION_CREATE_ACK = CMBS_EV_DSR_SUOTA_SESSION_CREATE_ACK,
    GMEP_US_OPEN_SESSION = CMBS_EV_DSR_SUOTA_OPEN_SESSION,
    GMEP_US_OPEN_SESSION_ACK = CMBS_EV_DSR_SUOTA_OPEN_SESSION_ACK,
    GMEP_US_DATA_SEND = CMBS_EV_DSR_SUOTA_DATA_SEND,
    GMEP_US_DATA_SEND_ACK = CMBS_EV_DSR_SUOTA_DATA_SEND_ACK,
    GMEP_US_REG_CPLANE_CB = CMBS_EV_DSR_SUOTA_REG_CPLANE_CB,
    GMEP_US_REG_CPLANE_CB_ACK = CMBS_EV_DSR_SUOTA_REG_CPLANE_CB_ACK,
    GMEP_US_REG_APP_CB = CMBS_EV_DSR_SUOTA_REG_APP_CB,
    GMEP_US_REG_APP_CB_ACK = CMBS_EV_DSR_SUOTA_REG_APP_CB_ACK,
    GMEP_US_DATA_RECV = CMBS_EV_DSR_SUOTA_DATA_RECV,
    GMEP_US_DATA_RECV_ACK = CMBS_EV_DSR_SUOTA_DATA_RECV_ACK,
// GMEP_US_HS_VER_IND = CMBS_EV_DSR_SUOTA_HS_VERSION_RECEIVED,
    GMEP_US_HS_VER_IND_ACK = CMBS_EV_DSR_SUOTA_HS_VER_IND_ACK,
// GMEP_US_HS_AVAIL = CMBS_EV_DSR_SUOTA_HS_AVAIL,
// GMEP_US_HS_AVAIL_ACK = CMBS_EV_DSR_SUOTA_HS_AVAIL_ACK,
    GMEP_US_SESSION_CLOSE = CMBS_EV_DSR_SUOTA_SESSION_CLOSE,
    GMEP_US_SESSION_CLOSE_ACK = CMBS_EV_DSR_SUOTA_SESSION_CLOSE_ACK,
// GMEP_US_URL_IND = CMBS_EV_DSR_SUOTA_URL_RECEIVED, // HS to Host
// GMEP_US_URL_IND_ACK = CMBS_EV_DSR_SUOTA_URL_IND_ACK,
// GMEP_US_NACK = CMBS_EV_DSR_SUOTA_NACK,
// GMEP_US_NACK_ACK = CMBS_EV_DSR_SUOTA_NACK_ACK,
    GMEP_US_CONTROL_SET = CMBS_EV_DSR_SUOTA_CONTROL_SET,
    GMEP_US_CONTROL_SET_ACK = CMBS_EV_DSR_SUOTA_CONTROL_SET_ACK,
    GMEP_US_COTROL_RESET = CMBS_EV_DSR_SUOTA_COTROL_RESET,
    GMEP_US_COTROL_RESET_ACK = CMBS_EV_DSR_SUOTA_COTROL_RESET_ACK,
    GMEP_US_UPDATE_OPTIONAL_GRP = CMBS_EV_DSR_SUOTA_UPDATE_OPTIONAL_GRP,
    GMEP_US_UPDATE_OPTIONAL_GRP_ACK = CMBS_EV_DSR_SUOTA_UPDATE_OPTIONAL_GRP_ACK,
    GMEP_US_FACILITY_CB = CMBS_EV_DSR_SUOTA_FACILITY_CB,
// GMEP_US_PUSH_MODE = CMBS_EV_DSR_SUOTA_PUSH_MODE,
} t_GmepCmd;
#endif


#if 0
typedef enum
{
    GMEP_US_SESSION_CREATE = 1,
    GMEP_US_OPEN_SESSION,
    GMEP_US_OPEN_SESSION_ACK,
    GMEP_US_DATA_SEND,
    GMEP_US_DATA_SEND_ACK,
    GMEP_US_REG_CPLANE_CB,
    GMEP_US_REG_CPLANE_CB_ACK,
    GMEP_US_REG_APP_CB,
    GMEP_US_REG_APP_CB_ACK,
    GMEP_US_DATA_RECV,
    GMEP_US_DATA_RECV_ACK,
    GMEP_US_HS_VER_IND,
    GMEP_US_HS_VER_IND_ACK,
    GMEP_US_HS_AVAIL,
    GMEP_US_HS_AVAIL_ACK,
    GMEP_US_SESSION_CLOSE,
    GMEP_US_SESSION_CLOSE_ACK,
    GMEP_US_URL_IND,
    GMEP_US_URL_IND_ACK,
    GMEP_US_NACK,
    GMEP_US_NACK_ACK,
    GMEP_US_CONTROL_SET,
    GMEP_US_CONTROL_SET_ACK,
    GMEP_US_COTROL_RESET,
    GMEP_US_COTROL_RESET_ACK,
    GMEP_US_UPDATE_OPTIONAL_GRP,
    GMEP_US_UPDATE_OPTIONAL_GRP_ACK,
    GMEP_US_FACILITY_CB
} t_GmepCmd;


#endif

typedef struct
{
    u8 u8_Buffer[MAX_SW_VER_ID_SIZE];
    u8 u8_BuffLen;
} t_Suota_VerBuffer;

typedef enum
{
    SUOTA_SW_VER_ID = 1,
    SUOTA_HW_VER_ID
} t_Suota_VerID;


typedef struct
{
    u16 u16_EMC;
    u8  u8_URLStoFollow;
u8  u8_FileNum   :
    4;
u8  u8_Spare   :
    4;
u8  u8_Reason   :
    4;
u8  u8_Flags    :
    4;
} t_Suota_HSVerIndHdr;



typedef struct
{
    t_Suota_HSVerIndHdr st_Header;
    t_Suota_VerBuffer st_SwVer;
    t_Suota_VerBuffer st_HwVer;
} t_st_GmepSuotaHsVerInd;

#if 0
typedef struct
{
    u16 hsNo;
    u16 emc;
    u8 urlsToFollow;
    u8 fileNumber ;//:4;  // (4 bits)
    u8 spare;  ;//:4; //
    u8 flags  ;//:4; //  bits
    u8 reason  ;//:4;
    u8 swVerLen;
    u8* pSwVer;
    u8 hwVerLen;
    u8* pHwVer;
} t_st_GmepSuotaHsVerInd;

#endif

typedef struct
{
    u16 u16_delayInMin;
    u8 u8_URLStoFollow;
    u8 u8_Spare   ;
    u8 u8_UserInteraction ;
} t_Suota_HSVerAvailHdr;

typedef struct
{
    t_Suota_HSVerAvailHdr st_Header;
    t_Suota_VerBuffer st_SwVer;
} t_st_GmepSuotaHsVerAvail;


#if 0

typedef struct
{
    u16 hsNo;
    u16 delayInMs;
    u8 urlsToFollow;
    u8 spare;
    u8 userInteraction;
    u8 swVerLen;
    u8* pswVer;
} t_st_GmepSuotaHsVerAvail;

#endif

typedef struct
{
    u8 u8_URL[MAX_URL_SIZE];
    u8 u8_URLLen;
} t_Suota_URLBuffer;


typedef struct
{
    u8 u8_URLStoFollow;
    t_Suota_URLBuffer st_URL;
} t_st_GmepSuotaUrlInd;

typedef struct
{

    t_Suota_VerBuffer st_SwVer;
    t_Suota_VerBuffer st_HwVer;


} t_st_GmepSuotaPushMode;





#if 0
typedef struct
{
    u16 hsNo;
    u8 urlsToFollow;
    u8 urlLen;
    u8* purlInf;
} t_st_GmepSuotaUrlInd;

#endif


typedef enum
{
    SUOTA_RETRY_LATER_CON_REFUSED = 1,
    SUOTA_RETRY_LATER_FP_RES_OVERFLOW,
    SUOTA_FILE_DOESNOT_EXIST,
    SUOTA_INVALID_URL1_FORMAT,
    SUOTA_UNREACHABLE_URL1,
    SUOTA_CMD_FORMAT_ERROR
} t_Suota_RejectReason;


typedef struct
{
    u16   hsNo;
    t_Suota_RejectReason rejReason;
} t_st_GmepSuotaNegAck;

typedef struct
{
    u32 appSduLen;
    u8* appSdu;
} t_st_GmepSdu;



typedef int (*gmepAppCB)(u8 cmd, void *pData);
typedef int (*cbSuotaCmd)(u8 hsno, u8  cmd, void *pData);
typedef void (*cbFacility)(u8 u8_Handset, u8 bFacilitySent);

typedef struct
{
u8  spare:
    2;   /*for further development */
u8  chop:
    1; /*chop bit set if application data   MAX_SDU_SIZE */
u8  optionalGrp:
    2;
u8  operationalCode:
    2;
u8  cntrlseText:
    1;   /* Always set to 1 */
u8  gmci:
    7;    /* Generic Media Context Identifier(gmci). gmci = GMEP session index */
u8  seqNo:
    1;    /* sequence number set, if chop bit is set */
    u16  pid;          /* Holds the application protocol identifier, here example http (1078 in decimal) */

} t_st_GmepControlSet;

typedef struct
{
    u32 ipSource;
    u32 ipDestination;
    u16 portNumSrc;
    u16 portNumDest;
} t_st_GmepOptionalGroup;

#define MAX_GMEP_SDU_SIZE 750

typedef struct
{
    u8    cmd;
    u32   appId;
    u32   appSessionId;
    u32   bufLen;
    u8   buf[MAX_GMEP_SDU_SIZE];
} t_st_GmepAppMsgCmd;

#if   !defined( MAX )
#define MAX(a, b) ( a>b ? a:b )
#endif
int gmep_us_SessionClose(u32);

extern int gmep_us_Write(t_st_GmepAppMsgCmd * pData);
//extern int gmep_us_Callback();
extern int gmep_us_RegCPlaneCmdsCB(cbSuotaCmd pcbSuotaCb, u32 appId);
extern int gmep_us_RegAppCB(gmepAppCB pcbAppCb, u32 appId);
extern int gmep_us_SendHandVerAvail(t_st_GmepSuotaHsVerAvail *pData, u16 u16_Handset, cbFacility pFacilityCB);
extern int gmep_us_SendUrl(u16 u16_Handset, t_st_GmepSuotaUrlInd *pData, cbFacility pFacilityCB);
extern int gmep_us_SendNegAck(u16 u16_Handset, t_st_GmepSuotaNegAck *negAck, cbFacility pFacilityCB);
extern int gmep_us_GmepSduSend(u32 sessionId, t_st_GmepSdu *pGmepSdu);
extern int gmep_us_UpdateControlSet(u32 sessionId, t_st_GmepControlSet* pControlSet);
extern int gmep_us_ResetControlSet(u32 sessionId);
extern int gmep_us_UpdateOptionalGroup(u32 sessionId, t_st_GmepOptionalGroup* pGmepOS);
extern int gmep_us_Init(int *readFd, int *writeFd, u32 appId);
extern u8  gmep_us_msg_fifo_read(char *buf, int len);
extern u8  gmep_us_SessionCreate(u32 appId, u16 hsNo);
extern int gmep_us_PushModeSend();

#endif /* _GMEP_US_ */










