
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#else
#include <winsock2.h>
#include <process.h>
#endif // WIN32

#include <errno.h>
#include <string.h>
#include "cmbs_api.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "cmbs_han.h"
#include "cfr_ie.h"
#include "apphan.h"
#include "appsrv.h"
#include "appcmbs.h"
#include "cmbs_int.h"
#include "appmsgparser.h"
#include "cmbs_han_ie.h"
#include "hanfun_protocol_defs.h"

#define NUMBER_OF_REG_DEV_ENTRIES CMBS_HAN_MAX_DEVICES

#define CMBS_COMMAND_MAX_LEN 50
#define CMBS_SUBSCRIPTION_TIMEOUT 240

#define CMBS_STRING_SIZE 1600

#define PORT "3490"     // the port users will be connecting to
#define LOCAL_PORT 2269

#define BACKLOG 1     // how many pending connections queue will hold

static int g_selSocket = -1;
static int g_intSocket = -1;

struct sockaddr_in si_me;

char g_interfaceName[20] = {0};
unsigned short g_bindPort;

static void han_closeSocket(int sock);
void app_HanDemoRegularStart(void);
char* app_HanDemoCMBS_Han_Unit_Type(u16 u16_unitType);
static int send2InternalSocket(char *pCommand);

void app_HanServerOnRegClosed(E_CMBS_EVENT_ID e_EventID, void *pv_EventData);
void app_HanServerOnReadDeviceTableRes(void *pv_EventData);
void app_OnHandsetRegistered(void *pv_List);


/**
 * Protocol - Fields and helper functions to retrieve specific
 * fields
 *
 * @brief
 */

char *g_fieldID[] =
{
    "ID: ",
    "DIRECTION: ",
    "DST_DEV_ID: ",
    "DST_UNIT_ID: ",
    "CMD_DATA: ",
    "CMD_ATTRID: ",
    "DATALEN: ",
    "SEQUENCE: ",
    "DATA: ",
    "MSGTYPE: ",
    "SRC_DEV_ID: ",
    "SRC_UNIT_ID: ",
    "SRC_DEVICE_IPUI: ",
    "SRC_UNIT_INTERFACE: ",
    "RES_REQ: ",
};

enum e_fieldId
{
    HAN_SERVER_INTERFACE_ID = 0,
    HAN_SERVER_DIRECTION,
    HAN_SERVER_DST_DEV_ID,
    HAN_SERVER_DST_UNIT_ID,
    HAN_SERVER_CMD_DATA,
    HAN_SERVER_CMD_ATTRID,
    HAN_SERVER_DATALEN,
    HAN_SERVER_SEQUENCE,
    HAN_SERVER_DATA,
    HAN_SERVER_MSGTYPE,
    HAN_SERVER_SRC_DEV_ID,
    HAN_SERVER_SRC_UNIT_ID,
    HAN_SERVER_DEVICE_IPUI,
    HAN_SERVER_DEVICE_UNIT_INTERFACE,
    HAN_SERVER_RES_REQ
};

int getIntFieldByStr(char *pCommand, int fieldId, int *pValue)
{
    char *pCommandStart = NULL;
    int commandLen = strlen(g_fieldID[fieldId]);
#define MAX_VALUE_LEN 20
    char valueStr[MAX_VALUE_LEN + 1];
    int i;

    pCommandStart = strstr(pCommand, g_fieldID[fieldId]);
    if (pCommandStart != NULL)
    {
        printf("hanServer: getIntFieldByStr Found CMD <%s>\n", g_fieldID[fieldId]);

        for (i = 0; i < MAX_VALUE_LEN && pCommandStart[i + commandLen] != '\r' && pCommandStart[i + commandLen] != ' '; i++)
        {
            valueStr[i] = pCommandStart[i + commandLen];
            printf("hanServer: getIntFieldByStr %d\n", valueStr[i]);
        }
        valueStr[i] = 0;
        *pValue = atoi(valueStr);
        printf("hanServer: getIntFieldByStr valueStr %d\n", *pValue);
        return 1;
    }
    else
    {
        printf("hanServer: getIntFieldByStr NOT FOUND\n");
    }

    return 0;
}

int getStrFieldByStr(char *pCommand, int fieldId, char **ppData)
{
    char *pCommandStart = NULL;
    int commandLen = strlen(g_fieldID[fieldId]);
#define MAX_VALUE_LEN 20
    int i = 0;

    printf("hanServer: getStrFieldByStr commandLen %d   cmd %s\n", commandLen, g_fieldID[fieldId]);
    if ((pCommandStart = strstr(pCommand, g_fieldID[fieldId])))
    {
        printf("hanServer: getStrFieldByStr Found Start Of Data \n");
        *ppData = &(pCommandStart[i + commandLen]);
    }

    return 0;
}


#ifndef WIN32

void sigchld_handler(int s)
{
    UNUSED_PARAMETER(s);
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

#endif

void* get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

#ifndef WIN32
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
#endif
    return NULL;
}


/**********************************************************************
 *  Protocol - CB Functions for each Command received by Remote
 ***********************************************************************/
typedef int hanProxyCB(char *pCommand);

typedef struct {
    char         *pCmd;
    hanProxyCB   *pHanProxyCB;
} t_CmdCb;

int regOpen(char *pCommand)
{
    UNUSED_PARAMETER(pCommand);
    printf("hanServer: regOpen Received from Remote Client\n");
    app_SrvSubscriptionOpen(CMBS_SUBSCRIPTION_TIMEOUT);

    return 0;
}

int regClose(char *pCommand)
{
    UNUSED_PARAMETER(pCommand);
    printf("hanServer: regClose Received from Remote Client\n");
    app_SrvSubscriptionClose();

    return 0;
}

extern ST_CMBS_APPL    g_cmbsappl;

int getRegTable(char *pCommand)
{
    short u16_NumOfEntries = NUMBER_OF_REG_DEV_ENTRIES;
    short u16_IndexOfFirstEntry = 0;
    UNUSED_PARAMETER(pCommand);
    cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, TRUE);
    //cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, FALSE);

    return 0;
}

// MUST be call from main thread

int initHan(char *pCommand)
{
    short u16_NumOfEntries = NUMBER_OF_REG_DEV_ENTRIES;
    short u16_IndexOfFirstEntry = 0;

    ST_HAN_MSG_REG_INFO     pst_HANMsgRegInfo;
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    u8                      epromData;

    static int alreadInitialized = 0;
    UNUSED_PARAMETER(pCommand);

    if (alreadInitialized)
    {
        // Read Registration Table only
        cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, TRUE);
        return 0;
    }

    alreadInitialized = 1;

#if 1

    // RF Resume -- moved to SYS_START
    // cmbs_dsr_sys_RFResume(NULL);

    // Set HAN Eprom Parameter
    epromData = 0x80;
    cmbs_dsr_param_area_Set(NULL, CMBS_PARAM_AREA_TYPE_EEPROM, 0x14, &epromData, 1);

    epromData = 0x30;
    cmbs_dsr_param_area_Set(NULL, CMBS_PARAM_AREA_TYPE_EEPROM, 0x1f1, &epromData, 1);

    app_HanDemoRegularStart();

#endif

    // Register for Han Messages
    pst_HANMsgRegInfo.u16_InterfaceId = 0xFFFF;
    pst_HANMsgRegInfo.u8_UnitId = 2;
    cmbs_dsr_han_msg_RecvRegister(p_List, &pst_HANMsgRegInfo);

    // Read Registration Table
    cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, TRUE);

    return 0;

}


unsigned char htoi(const char *ptr)
{
    unsigned char value = 0;
    char ch = *ptr;
    int i = 0;

    for (i = 0; i < 2; i++)
    {

        ch = *ptr;
        if (ch >= '0' && ch <= '9')
            value = (value << 4) + (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            value = (value << 4) + (ch - 'A' + 10);
        else if (ch >= 'a' && ch <= 'f')
            value = (value << 4) + (ch - 'a' + 10);

        ++ptr;
    }
    return value;
}

int populateBinData(char *pCommand, int dataLength, unsigned char *pBinOut)
{
    int inx;
    unsigned char *pAsciiData;

    pAsciiData = (unsigned char *)pCommand;
    if (pAsciiData)
    {
        for (inx = 0; inx < dataLength / 2; inx++)
        {
            pBinOut[inx] = htoi((char*)pAsciiData);
            pAsciiData += 2;
        }
    }
    else
    {
        return -1;
    }
    return 0;
}

void populateHexValue(unsigned char *pInBinData, unsigned short *len, unsigned char *pOutHexBuf)
{
    int inx;
    unsigned char *pHexOut;
    pHexOut = pOutHexBuf;
    for (inx = 0; inx < *len; inx++)
    {
        //sprintf((char*)pHexOut, "%x2", pInBinData[inx]);
        sprintf((char*)pHexOut, " %02x", pInBinData[inx]);
        pHexOut += 3;
    }
    *len = (*len) * 3;
}


int callDelete(char *pCommand)
{
    int value;
    char buffer[8];

    printf("hanServer: callDelete Received from Remote Client \n");

    if (getIntFieldByStr(pCommand, HAN_SERVER_DST_DEV_ID, &value))
    {
        sprintf(buffer, "%d", value);

        app_SrvHandsetDelete(buffer);
    }

    return 0;
}

int callInterface(char *pCommand)
{
    short u16_RequestId = 0;

    ST_IE_HAN_MSG     pst_HANMsg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };
    char outBinBuf[CMBS_HAN_MAX_MSG_DATA_LEN];

    char *pValPtr;
    int value;

    printf("hanServer: callInterface Received from Remote Client \n");
    memset(&pst_HANMsg, 0, sizeof(ST_IE_HAN_MSG));

    //         stIe_Msg.e_MsgType            = CMBS_HAN_MSG_TYPE_CMD 1 ;
    if (getIntFieldByStr(pCommand, HAN_SERVER_MSGTYPE, &value))
        pst_HANMsg.e_MsgType = value;

    //            stIe_Msg.u8_InterfaceMember  = ON_OFF_IF_SERVER_CMD_TURN_ON;
    if (getIntFieldByStr(pCommand, HAN_SERVER_CMD_ATTRID, &value))
        pst_HANMsg.u8_InterfaceMember  = (u8)value;

    //    stIe_Msg.u16_InterfaceId                    = ON_OFF_INTERFACE_ID;
    if (getIntFieldByStr(pCommand, HAN_SERVER_INTERFACE_ID, &value))
        pst_HANMsg.u16_InterfaceId = (short)value;

    //    stIe_Msg.st_DstDeviceUnit                   = pUnit->st_DeviceUnit;

    if (getIntFieldByStr(pCommand, HAN_SERVER_DST_DEV_ID, &value))
        pst_HANMsg.u16_DstDeviceId = (short)value;

    if (getIntFieldByStr(pCommand, HAN_SERVER_DST_UNIT_ID, &value))
        pst_HANMsg.u8_DstUnitId = (u8)value;

    //    stIe_Msg.u16_DataLen                        = 0;
    if (getIntFieldByStr(pCommand, HAN_SERVER_DATALEN, &value))
        pst_HANMsg.u16_DataLen = (u16)value;


    getStrFieldByStr(pCommand, HAN_SERVER_DATA, &pValPtr);


    //    stIe_Msg.u8_MsgSequence                     = 0;
    if (getIntFieldByStr(pCommand, HAN_SERVER_SEQUENCE, &value))
        pst_HANMsg.u8_MsgSequence = (u8)value;

    pst_HANMsg.u8_DstAddressType = 0;
    pst_HANMsg.u16_SrcDeviceId = 0;
    pst_HANMsg.u8_SrcUnitId = 2;

    pst_HANMsg.st_MsgTransport.u16_Reserved = 0;


    pst_HANMsg.u8_InterfaceType       = 1; // New protocol (was 0)

#if 0
    printf("hanServer: callInterface interface %d dev_id %d unit_id %d msg_type %d cmd_inter_mem %d \n",
           pst_HANMsg.u16_InterfaceId,
           //           pst_HANMsg.u8_Direction,
           pst_HANMsg.st_DstDeviceUnit.u16_DeviceId,
           pst_HANMsg.st_DstDeviceUnit.u8_UnitId,
           pst_HANMsg.e_MsgType,
           pst_HANMsg.u16_InterfaceMember);
#endif

    if (pst_HANMsg.u16_DataLen)
    {
        printf("hanServer: callInterface Data Length %d\n", pst_HANMsg.u16_DataLen);
        if (pst_HANMsg.u16_DataLen < CMBS_HAN_MAX_MSG_DATA_LEN)
        {
            char *pStartHexData = strstr(pCommand, "\r\n\r\n"); // Find start of hex data
            if (pStartHexData)
            {
                pStartHexData += 4; // skip CRLFCRLF
                populateBinData(pStartHexData, pst_HANMsg.u16_DataLen, (unsigned char*)outBinBuf);
                pst_HANMsg.u16_DataLen = pst_HANMsg.u16_DataLen / 2;
                pst_HANMsg.pu8_Data = outBinBuf;
            }
            else
                pst_HANMsg.u16_DataLen = 0;
        }
        else
            pst_HANMsg.u16_DataLen = 0;

    }

    // Delay till Tx ready arrives
    app_DsrHanMsgSend(12, pst_HANMsg.u16_DstDeviceId, &st_HANMsgCtl, &pst_HANMsg);

    return 0;
}

int dummy(char *pCommand)
{
    pCommand[CMBS_COMMAND_MAX_LEN] = 0;
    printf("hanServer: dummy couldnt find command %s\n", pCommand);

    return 0;
}


/**********************************************************************
 *  Callbacks from CMBS Target
 ***********************************************************************/
int HanEventCB(char *pCommand);

t_CmdCb g_hanOnTargetEvRcvdCB[] =
{
    { "TARGET_SYS_READY", HanEventCB },
    { "", dummy },
};

int HanEventCB(char *pCommand)
{
    PST_CFR_IE_LIST   p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    //    ST_HAN_MSG_REG_INFO     pst_HANMsgRegInfo;
    UNUSED_PARAMETER(p_List);
    UNUSED_PARAMETER(pCommand);
    printf("hanServer: HanEventCB\n");

    return 0;
}

void app_HanServerOnRegClosed(E_CMBS_EVENT_ID e_EventID, void *pv_EventData)
{
    void *pv_IE = NULL;
    u16           u16_IE;
    //    ST_IE_RESPONSE          st_Response;
    ST_IE_REG_CLOSE_REASON  st_RegCloseReason;

    st_RegCloseReason.e_Reg_Close_Reason = CMBS_REG_CLOSE_MAX;
    UNUSED_PARAMETER(e_EventID);
    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        switch (u16_IE)
        {
            case CMBS_IE_REG_CLOSE_REASON:
                app_IEToString(pv_IE, u16_IE);
                cmbs_api_ie_RegCloseReasonGet(pv_IE, &st_RegCloseReason);
                break;
            case    CMBS_IE_RESPONSE:
                break;
        }

        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }


    if (st_RegCloseReason.e_Reg_Close_Reason == CMBS_REG_CLOSE_HS_REGISTERED)
    {
        printf("hanServer: app_HanServerOnRegClosed Registration Succeeded \n");
        send2InternalSocket("REG_SUCCEEDED\r\n\r\n");
    }
    else
    {
        printf("hanServer: app_HanServerOnRegClosed Registration TimeOut \n");
        send2InternalSocket("REG_TIMEOUT\r\n\r\n");
    }

}

void app_HanServerOnReadDeviceTableRes(void *pv_EventData)
{
    void *pv_IE = NULL;
    u16       u16_IE, u16_Length = 0;
    u8       temp_buffer[2000];
    u8                          *pTmpBufPtr;
    //#define MAX_DEVICES_ARR 5
    u8            u8_Length = 0;
    u8            u8_ArrEntry = 0;
    u8         Units;
    u16        Interfaces;

    printf("\nhanServer: app_HanServerOnReadDeviceTableRes--> \n");
    memset(temp_buffer, 0x0, sizeof(temp_buffer));
    pTmpBufPtr = temp_buffer;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES || u16_IE == CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES)
        {
            if (u16_IE == CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES)
            {
                ST_HAN_BRIEF_DEVICE_INFO  arr_Devices[NUMBER_OF_REG_DEV_ENTRIES];
                ST_IE_HAN_BRIEF_DEVICE_ENTRIES st_HANDeviceEntries;
                st_HANDeviceEntries.pst_DeviceEntries = arr_Devices;

                cmbs_api_ie_HANDeviceTableBriefGet(pv_IE, &st_HANDeviceEntries);
                u16_Length = st_HANDeviceEntries.u16_NumOfEntries * sizeof(ST_HAN_DEVICE_ENTRY);
                memcpy(&arr_Devices, (u8 *)st_HANDeviceEntries.pst_DeviceEntries, u16_Length);

                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal(&st_HANDeviceEntries, sizeof(ST_IE_HAN_BRIEF_DEVICE_ENTRIES), 1, CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES);
                }

                for (u8_ArrEntry = 0; u8_ArrEntry < st_HANDeviceEntries.u16_NumOfEntries && u8_ArrEntry < NUMBER_OF_REG_DEV_ENTRIES; u8_ArrEntry++)
                {
                    {

                        sprintf((char*)pTmpBufPtr, "DEV_REG_STATUS\r\n%s %d\r\n",
                                g_fieldID[HAN_SERVER_SRC_DEV_ID],
                                arr_Devices[u8_ArrEntry].u16_DeviceId);

                        pTmpBufPtr += strlen((char*)pTmpBufPtr);

                        for (Units = 0; Units < arr_Devices[u8_ArrEntry].u8_NumberOfUnits; Units++)
                        {
                            sprintf((char*)pTmpBufPtr, "%s %d\r\nTYPE: %d\r\n\r\n",
                                    g_fieldID[HAN_SERVER_SRC_UNIT_ID],
                                    arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u8_UnitId,
                                    arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_UnitType);
                            pTmpBufPtr += strlen((char*)pTmpBufPtr);
                        }

                    }
                }

                //send2InternalSocket(temp_buffer);
            }

            if (u16_IE == CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES)
            {
                ST_HAN_EXTENDED_DEVICE_INFO   arr_Devices[NUMBER_OF_REG_DEV_ENTRIES];
                ST_IE_HAN_EXTENDED_DEVICE_ENTRIES  st_HANDeviceEntries;
                st_HANDeviceEntries.pst_DeviceEntries = arr_Devices;

                cmbs_api_ie_HANDeviceTableExtendedGet(pv_IE, &st_HANDeviceEntries);
                u16_Length = st_HANDeviceEntries.u16_NumOfEntries * sizeof(ST_HAN_EXTENDED_DEVICE_INFO);
                memcpy(&arr_Devices, (u8 *)st_HANDeviceEntries.pst_DeviceEntries, u16_Length);

                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal(&st_HANDeviceEntries, sizeof(ST_IE_HAN_EXTENDED_DEVICE_ENTRIES), 1, CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES);
                }

                for (u8_ArrEntry = 0; u8_ArrEntry < st_HANDeviceEntries.u16_NumOfEntries && u8_ArrEntry < NUMBER_OF_REG_DEV_ENTRIES; u8_ArrEntry++)
                {
                    sprintf((char*)pTmpBufPtr, "DEV_REG_STATUS\r\n%s %d\r\n",
                            g_fieldID[HAN_SERVER_SRC_DEV_ID],
                            arr_Devices[u8_ArrEntry].u16_DeviceId);

                    pTmpBufPtr += strlen((char*)pTmpBufPtr);

                    sprintf((char*)pTmpBufPtr, "DEV_IPUI\r\n%s %d %d %d %d %d\r\n",
                            g_fieldID[HAN_SERVER_DEVICE_IPUI],
                            arr_Devices[u8_ArrEntry].u8_IPUI[0], arr_Devices[u8_ArrEntry].u8_IPUI[1], arr_Devices[u8_ArrEntry].u8_IPUI[2], arr_Devices[u8_ArrEntry].u8_IPUI[3], arr_Devices[u8_ArrEntry].u8_IPUI[4]);

                    pTmpBufPtr += strlen((char*)pTmpBufPtr);

                    for (Units = 0; Units < arr_Devices[u8_ArrEntry].u8_NumberOfUnits; Units++)
                    {
                        sprintf((char*)pTmpBufPtr, "%s %d\r\nTYPE: %d\r\n\r\n",
                                g_fieldID[HAN_SERVER_SRC_UNIT_ID],
                                arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u8_UnitId,
                                arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_UnitType);
                        pTmpBufPtr += strlen((char*)pTmpBufPtr);

                        for (Interfaces = 0; Interfaces < arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_NumberOfOptionalInterfaces; Interfaces++)
                        {
                            sprintf((char*)pTmpBufPtr, "%s %d\r\nINTERFACE: %d\r\n\r\n",
                                    g_fieldID[HAN_SERVER_DEVICE_UNIT_INTERFACE],
                                    Interfaces,
                                    arr_Devices[u8_ArrEntry].st_UnitsInfo[Units].u16_OptionalInterfaces[Interfaces]);
                            pTmpBufPtr += strlen((char*)pTmpBufPtr);
                        }
                    }
                }
            }

            send2InternalSocket(temp_buffer);

        }
        app_IEToString(pv_IE, u16_IE);
        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);

    }

}

int app_HanServerOnTableUpdate(void *pv_EventData)
{
    short u16_NumOfEntries = NUMBER_OF_REG_DEV_ENTRIES;
    short u16_IndexOfFirstEntry = 0;
    UNUSED_PARAMETER(pv_EventData);
    cmbs_dsr_han_device_ReadTable(g_cmbsappl.pv_CMBSRef, u16_NumOfEntries, u16_IndexOfFirstEntry, TRUE);

    return 0;
}

int app_HanServerOnMsgRecv(void *pv_EventData)
{
    void *pv_IE = NULL;
    u16            u16_IE;

    u8 u8_Buffer[CMBS_HAN_MAX_MSG_LEN * 2];
    ST_IE_HAN_MSG stIe_Msg;

    unsigned char temp_buffer[600];

    stIe_Msg.pu8_Data = u8_Buffer;


    printf("hanServer: app_HanServerOnMsgRecv\n");
    memset(temp_buffer, 0x0, sizeof(temp_buffer));

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_HAN_MSG)   // CMBS_EV_DSR_HAN_MSG_RECV
        {
            //app_HanDemoPrintHanMessage(pv_IE);
            cmbs_api_ie_HANMsgGet(pv_IE, &stIe_Msg);

            if (stIe_Msg.u16_InterfaceId == ALERT_INTERFACE_ID)
                printf("hanServer: Alert Interface \n");
            else if (stIe_Msg.u16_InterfaceId == KEEP_ALIVE_INTERFACE_ID)
                printf("hanServer: Keep Alive Interface \n");


            // Send Keep Alive to Web
            //sprintf(temp_buffer,"INTERFACE\r\n ID: %d\r\n SRC_DEV_ID: %d\r\n SRC_UNIT_ID: %d\r\n MSGTYPE: %d\r\n CMD_ATTRID: %d\r\n",
            //            sprintf(temp_buffer,"INTERFACE\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n\r\n",
            sprintf((char*)temp_buffer, "INTERFACE\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n %s %d\r\n\r\n",
                    g_fieldID[HAN_SERVER_INTERFACE_ID],
                    stIe_Msg.u16_InterfaceId,

                    g_fieldID[HAN_SERVER_SRC_DEV_ID],
                    stIe_Msg.u16_SrcDeviceId,

                    g_fieldID[HAN_SERVER_SRC_UNIT_ID],
                    stIe_Msg.u8_SrcUnitId,

                    g_fieldID[HAN_SERVER_MSGTYPE],
                    stIe_Msg.e_MsgType,

                    // This Field was added, as remote have to get this field to understand the Thermometer and smoke events
                    g_fieldID[HAN_SERVER_CMD_ATTRID],
                    stIe_Msg.u8_InterfaceMember,

                    g_fieldID[HAN_SERVER_DST_DEV_ID],
                    stIe_Msg.u16_DstDeviceId,

                    g_fieldID[HAN_SERVER_DST_UNIT_ID],
                    stIe_Msg.u8_DstUnitId,

                    //                    g_fieldID[HAN_SERVER_DIRECTION],
                    //                    stIe_Msg.u8_Direction,

                    g_fieldID[HAN_SERVER_SEQUENCE],
                    stIe_Msg.u8_MsgSequence,

                    g_fieldID[HAN_SERVER_DATALEN],
                    stIe_Msg.u16_DataLen

                    //                    g_fieldID[HAN_SERVER_RES_REQ],
                    //                    stIe_Msg.st_MsgControl.u8_Reliable

                   );

            if (stIe_Msg.u16_DataLen && stIe_Msg.pu8_Data)
            {
                // Add hex data
                unsigned short binlength, msglen;
                msglen = strlen((char*)temp_buffer);
                binlength = stIe_Msg.u16_DataLen;
                populateHexValue(stIe_Msg.pu8_Data, &binlength, &(temp_buffer[msglen]));
                temp_buffer[msglen + binlength] = 0;
                printf("%s\n", temp_buffer);

            }

            send2InternalSocket((char*)temp_buffer);



            /*if(stIe_Msg.u16_InterfaceId == ALERT_INTERFACE_ID)
            {
                app_HanDemoAlertIfMsgHandler(&stIe_Msg);
            }
            else if (stIe_Msg.u16_InterfaceId == ON_OFF_INTERFACE_ID)
            {
                app_HanDemoOnOfIfMsgHandler(&stIe_Msg);
            }
            else
            {
                app_HanDemoPrintMessageFields(&stIe_Msg);
            }*/

        }
        else
        {
            app_IEToString(pv_IE, u16_IE);
        }


        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }

    appcmbs_ObjectReport(&stIe_Msg, sizeof(stIe_Msg), stIe_Msg.u16_InterfaceId, CMBS_EV_DSR_HAN_MSG_RECV);

    return 0;
}


#if 0
int app_HanServerOnHsRegistered(void *pv_List)
{
    PST_CFR_IE_LIST pst_IEList = (PST_CFR_IE_LIST)pv_List;
    ST_IE_HANDSETINFO st_HsInfo;

    memset(&st_HsInfo, 0, sizeof(ST_IE_HANDSETINFO));

    /* CMBS target send only one IE to Host */
    cmbs_api_ie_HandsetInfoGet(cfr_ie_ItemGet(pst_IEList), &st_HsInfo);

    if (st_HsInfo.u8_State == CMBS_HS_REG_STATE_REGISTERED)
    {
        APPCMBS_INFO(("APPSRV-INFO: Handset:%d IPUI:%02X%02X%02X%02X%02X Type:%s registered\n",
                      st_HsInfo.u8_Hs,
                      st_HsInfo.u8_IPEI[0], st_HsInfo.u8_IPEI[1], st_HsInfo.u8_IPEI[2],
                      st_HsInfo.u8_IPEI[3], st_HsInfo.u8_IPEI[4],
                      getstr_E_CMBS_HSTYPE(st_HsInfo.e_Type)));

        List_AddHsToFirstLine(st_HsInfo.u8_Hs);

        /* Send Date & Time Update */
        //app_SrvSendCurrentDateAndTime( st_HsInfo.u8_Hs );
    }
    else if (st_HsInfo.u8_State == CMBS_HS_REG_STATE_UNREGISTERED)
    {
        APPCMBS_INFO(("APPSRV-INFO: Handset:%d Unregistered\n", st_HsInfo.u8_Hs));

        List_RemoveHsFromAllLines(st_HsInfo.u8_Hs);
    }

    // if token is send signal to upper application
    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal((void*)&st_HsInfo , sizeof(st_HsInfo), CMBS_IE_HANDSETINFO, CMBS_EV_DSR_HS_REGISTERED);
    }

    cmbsevent_OnHandsetRegistered(st_HsInfo.u8_Hs);
}
#endif


static int send2InternalSocket(char *pCommand)
{
    int rc = -1;

    //printf("send2InternalSocket: message:\n %s\n" ,pCommand);

    if (g_intSocket != -1)
    {
        if (sendto(g_intSocket, pCommand, strlen(pCommand), 0, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        {
#ifdef WIN32
            rc = WSAGetLastError();
#endif
            printf("hanServer: send2InternalSocket error sendto %d\n", rc);
        }
    }

    return 0;

}


/**
 *
 *
 * @brief
 *  Command Used from Remote Client
 */
t_CmdCb g_hanProxyCB[] =
{
    { "REG_OPEN", regOpen },
    { "REG_CLOSE", regClose },
    { "INIT", initHan },
    { "REG_TABLE", getRegTable },
    { "INTERFACE", callInterface },
    { "DELETE", callDelete },
    { "", dummy },
};


/**
 *
 *
 * @brief
 */
void cmbsProxy(int new_fd)
{
    char buf[CMBS_STRING_SIZE + 1] = {0};
    int rc;
    int maxfd;
    fd_set read_fds;
    int i;

    // Read registrated devices
    // Read device table and Send information to client
    // Select on events from Client / HAN Devices

    printf("hanServer: cmbsProxy \n");
    printf("hanServer: cmbsProxy port chosen %d\n", ntohs(si_me.sin_port));

    FD_ZERO(&read_fds);
    FD_SET(new_fd, &read_fds);
    FD_SET(g_selSocket, &read_fds);
    maxfd = new_fd > g_selSocket ? new_fd : g_selSocket;

    printf("hanServer: cmbsProxy new_fd %d g_selSocket %d   maxfd %d \n", new_fd, g_selSocket, maxfd);

    while (1)
    {
        fd_set tmpFdSet = read_fds;

        // received commands from remote client + events from han devices

        rc = select(maxfd + 1, &tmpFdSet, NULL, NULL, NULL);
        if (rc < 0)
        {
            printf("cmbsProxy  Error select \n");
            break;
        }

        if (FD_ISSET(new_fd, &tmpFdSet))
        {
            rc = recv(new_fd, buf, CMBS_STRING_SIZE, 0);
            if (rc < 0)
            {
                printf("Error reading from Socket \n");
                break;
            }
            else if (rc == 0)
            {
                printf("cmbsProxy Closing \n\n");
                break;
            }
            // Make sure buf is null terminated
            buf[rc] = 0;
            // parse and execute received command
            for (i = 0; g_hanProxyCB[i].pCmd != NULL; i++)
            {
                if (strstr(buf, g_hanProxyCB[i].pCmd) != NULL)
                {
                    //printf("Trying to match %s\n",g_hanProxyCB[i].pCmd);
                    g_hanProxyCB[i].pHanProxyCB(buf);
                    memset(buf, 0, CMBS_COMMAND_MAX_LEN);
                    break;
                }
            }
        }
        else if (FD_ISSET(g_selSocket, &tmpFdSet))
        {
            // send data to our peer
            rc = recv(g_selSocket, buf, CMBS_STRING_SIZE, 0);
            if (rc > 0)
            {
                buf[rc] = 0;
                printf("hanServer:\n%s\n", buf);
                send(new_fd, buf, strlen(buf), 0);
            }

            // place holder... not used yet
            /*if( strstr(buf,g_hanOnTargetEvRcvdCB[0].pCmd) != NULL )
            {
                g_hanOnTargetEvRcvdCB[0].pHanProxyCB(buf);
            }
            else*/
        }


        //write(new_fd,"ethan\n",strlen("ethan\n"));
    } // while(1)

    printf("hanServer: cmbsProxy Exiting Loop \n\n");
    //close(new_fd);
    //exit(1); // exit thread
}

#ifndef WIN32
pthread_t g_hanProcessThreadId;
#else
uintptr_t g_hanProcessThreadId;
#endif

struct sockaddr_in antelope;

void* hanThread(void *args)
{
    // Start listening on port ...
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address information
    size_t sin_size;
#ifndef WIN32
    struct sigaction sa;
#endif
    int yes = 1;

#ifndef WIN32
    char s[128];
#else
    char *s;
#endif

    //   int rv;
    //    ST_HAN_MSG_REG_INFO pst_HANMsgRegInfo;
    struct sockaddr_in servaddr;

#ifndef WIN32
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
#endif

    UNUSED_PARAMETER(args);
    printf("HAN: hanThread Process Started \n");

#if 0
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // wild card ip address

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        // SO_REUSEADDR reuse socket anyway also if it in TIME_WAIT.
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure
#endif

    while (1)
    {

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
#ifndef WIN32
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, g_interfaceName, sizeof(ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
        ioctl(sockfd, SIOCGIFADDR, &ifr);

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            printf("hanServer Error setsockopt\n\n ");
            exit(1);
        }

#endif

#ifndef WIN32
        ((struct sockaddr_in *)&ifr.ifr_addr)->sin_port = htons(g_bindPort);
#else
        printf("hanServer interface %s\n", g_interfaceName);
#endif

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY); /* WILD CARD FOR IP ADDRESS */
        servaddr.sin_port = htons(g_bindPort); /* port number */
        memset(servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

        //if (bind(sockfd, &ifr.ifr_addr, sizeof(struct sockaddr_in)  ) == -1) {
        if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) == -1)
        {
            printf("hanServer Error bind \n\n ");
            han_closeSocket(sockfd);
            exit(1);
        }

        if (listen(sockfd, BACKLOG) == -1)
        {
            printf("hanServer Error listen %d  interface %s  port %d\n\n ", sockfd, g_interfaceName, g_bindPort);
            exit(1);
        }

#ifndef WIN32

        sa.sa_handler = sigchld_handler; // reap all dead processes
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1)
        {
            perror("sigaction");
            exit(1);
        }

#endif

        // create internal socket for receiving events from Target
        memset((char *)&si_me, 0, sizeof(si_me));

        g_selSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        g_intSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(LOCAL_PORT);
        si_me.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (bind(g_selSocket, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        {
            printf("hanServer: cmbsProxy error bind to address\n");
        }

        printf("server: waiting for connections...\n");

        while (1)  // main accept() loop
        {
            sin_size = sizeof their_addr;
            new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
            if (new_fd == -1)
            {
                printf("hanThread accept returned an error \n");
                if (errno == EINTR)
                    continue;
                else
                    break;
            }

#ifndef WIN32
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
#else
            s = inet_ntoa(antelope.sin_addr);
#endif
            printf("\n hanserver: got connection from %s\n", s);

            cmbsProxy(new_fd); // Loop - we do not call fork - only one connection needed !!!
            han_closeSocket(new_fd);  // parent doesn't need this
        }

        han_closeSocket(sockfd);
    }
}

#ifndef WIN32

unsigned long han_createThread(char *pInterfaceName, unsigned short port)
{
    int ret;
    pthread_attr_t  attr;

    strncpy(g_interfaceName, pInterfaceName, sizeof(g_interfaceName));
    g_interfaceName[sizeof(g_interfaceName) - 1] = '\0';
    g_bindPort = port;

    pthread_attr_init(&attr);
    ret = pthread_create(&g_hanProcessThreadId, &attr, hanThread, NULL);
    pthread_attr_destroy(&attr);

    return ret;
}

unsigned long han_quitThread()
{
    printf("han_quitThread\n");
    if (pthread_cancel(g_hanProcessThreadId) != 0)
    {
        printf("han_quitThread Error Couldnt pthread_cancel  \n");
    }
    if (pthread_join(g_hanProcessThreadId, NULL) != 0)
    {
        printf("han_quitThread Error Couldnt pthread_join  \n");
    }
    return 0;
}

static void han_closeSocket(int sock)
{
    close(sock);
}

#else // WIN32

unsigned long han_createThread(char *pInterfaceName, unsigned short port)
{
    int ret;
    //    DWORD ThreadID;

    ret = 0;

    strncpy(g_interfaceName, pInterfaceName, (sizeof(g_interfaceName) - 1));
    g_bindPort = port;

    initHan("INIT");

    g_hanProcessThreadId = _beginthread((void (*)(void *))hanThread, 0, NULL);

    return ret;
}

static void han_closeSocket(int sock)
{
    closesocket(sock);
}

#endif // WIN32


#if 0
int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // wild card ip address

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        // SO_REUSEADDR reuse socket anyway also if it in TIME_WAIT.
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) // main accept() loop
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork())
        {   // this is the child process
            close(sockfd); // child doesn't need the listener

            cmbsProxy(new_fd);

            /*if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);*/
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

#endif


