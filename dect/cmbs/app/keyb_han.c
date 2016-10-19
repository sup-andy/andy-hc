/*!
* \file  keyb_suota.c
* \brief  cat-iq 2.0 data services tests
* \Author  stein
*
* @(#) %filespec: keyb_han.c~ILD53#4 %
*
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if ! defined ( WIN32 )
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/msg.h>
#else
#include <conio.h>
#include <io.h>
#endif

#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cmbs_int.h"
#include "cfr_mssg.h"
#include "cmbs_dbg.h"
#include "appmsgparser.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "apphan.h"
#include "cmbs_han.h"
#include "tcx_keyb.h"
#include <tcx_util.h>
extern void app_HanDemoPrintMessageFields(ST_IE_HAN_MSG* pMsg);
extern void app_HanDemoSendMessage2Device(void);
extern void app_HanDemoRegularStart(void);

#define KEYB_HAN_ATTR_GET     0x0
#define KEYB_HAN_ATTR_SET_WITHOUT_RESPONSE 0x1
#define KEYB_HAN_ATTR_SET_WITH_RESPONSE  0x2

typedef struct
{
    bool  SetOrGet;
    u16  DeviceId;
    u8  UnitId;
    u16  InterfaceId;
    bool AttrOrPack;
    u8  AttrPackId;
    u8  ElementsInData;
    u8  Data[CMBS_HAN_MAX_MSG_LEN];
} keyb_han_AttributeManipulation;

static keyb_han_AttributeManipulation s_AttribManip;
#ifdef HAN_SERVER_DEMO

static void keyb_HanServerStart(void);
void han_serverStart(void);

#endif // HAN_SERVER_DEMO

//////////////////////////////////////////////////////////////////////////
void keyb_HanSendMngrInit(void)
{
    ST_HAN_CONFIG stHanCfg;
    stHanCfg.u8_HANServiceConfig =
        CMBS_HAN_DEVICE_MNGR_EXT |
        CMBS_HAN_BIND_LOOKUP_EXT |
        CMBS_HAN_GROUP_LOOKUP_EXT;

    app_DsrHanMngrInit(&stHanCfg);
}
//////////////////////////////////////////////////////////////////////////
void keyb_HanSendMngrStart(void)
{
    app_DsrHanMngrStart();
}

//////////////////////////////////////////////////////////////////////////

void keyb_HanGetConnectionStatus(void)
{
    char InputBuffer[100];
    u16 u16_DeviceId;

    // read num of entries
    printf("\nEnter Device Id: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_DeviceId = (u16)atoi(InputBuffer);

    app_DsrHanGetDeviceConnectionStatus(u16_DeviceId);
}


//////////////////////////////////////////////////////////////////////////

void keyb_HanDeleteDevice(void)
{
    char InputBuffer[100];
    char iChar;
    u16 u16_DeviceId;

    printf("\nDelete All Registered Devices? (y/n): ");

    iChar = tcx_getch();
    if ((iChar == 'y') || (iChar == 'Y'))
    {
        u16_DeviceId = 0xffff;
    }
    else if ((iChar == 'n') || (iChar == 'N'))
    {
        // read num of entries
        printf("\nEnter Device Id (dec): ");

        tcx_gets(InputBuffer, sizeof(InputBuffer));
        u16_DeviceId = (u16)atoi(InputBuffer);
    }
    else
    {
        printf("\nWrong input. Aborting... ");
        return;
    }

    app_DsrHanDeleteDevice(u16_DeviceId);
}
//////////////////////////////////////////////////////////////////////////
void keyb_HanReadDeviceTable(u8 isBrief)
{
    char InputBuffer[100];
    u16 u16_NumOfEntries;
    u16 u16_IndexOfFirstEntry;

    // read num of entries
    printf("\nEnter number of entries: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_NumOfEntries = (u16)atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read first entry index
    printf("\nEnter index of first entry: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_IndexOfFirstEntry = (u16)atoi(InputBuffer);

    app_DsrHanDeviceReadTable(u16_NumOfEntries, u16_IndexOfFirstEntry, isBrief);
}

//////////////////////////////////////////////////////////////////////////
void keyb_HanWriteDeviceTable(void)
{
    ST_HAN_DEVICE_ENTRY arrSt_Devices[10];
    arrSt_Devices[0].u16_UnitType     = 1;
    arrSt_Devices[0].st_DeviceUnit.u16_DeviceId  = 2;
    arrSt_Devices[0].st_DeviceUnit.u8_AddressType = 3;
    arrSt_Devices[0].st_DeviceUnit.u8_UnitId  = 4;

    arrSt_Devices[1].u16_UnitType     = 5;
    arrSt_Devices[1].st_DeviceUnit.u16_DeviceId  = 6;
    arrSt_Devices[1].st_DeviceUnit.u8_AddressType = 7;
    arrSt_Devices[1].st_DeviceUnit.u8_UnitId  = 8;

    arrSt_Devices[2].u16_UnitType     = 9;
    arrSt_Devices[2].st_DeviceUnit.u16_DeviceId  = 10;
    arrSt_Devices[2].st_DeviceUnit.u8_AddressType = 11;
    arrSt_Devices[2].st_DeviceUnit.u8_UnitId  = 12;

    arrSt_Devices[3].u16_UnitType     = 13;
    arrSt_Devices[3].st_DeviceUnit.u16_DeviceId  = 14;
    arrSt_Devices[3].st_DeviceUnit.u8_AddressType = 15;
    arrSt_Devices[3].st_DeviceUnit.u8_UnitId  = 16;

    arrSt_Devices[4].u16_UnitType     = 17;
    arrSt_Devices[4].st_DeviceUnit.u16_DeviceId  = 18;
    arrSt_Devices[4].st_DeviceUnit.u8_AddressType = 19;
    arrSt_Devices[4].st_DeviceUnit.u8_UnitId  = 20;

    arrSt_Devices[5].u16_UnitType     = 21;
    arrSt_Devices[5].st_DeviceUnit.u16_DeviceId  = 22;
    arrSt_Devices[5].st_DeviceUnit.u8_AddressType = 23;
    arrSt_Devices[5].st_DeviceUnit.u8_UnitId  = 24;

    app_DsrHanDeviceWriteTable(2, 1, arrSt_Devices);
}
//////////////////////////////////////////////////////////////////////////

void keyb_HanReadBindTable(void)
{
    char InputBuffer[100];
    u16 u16_NumOfEntries;
    u16 u16_IndexOfFirstEntry;

    // read number of entries
    printf("\nEnter number of entries: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_NumOfEntries = (u16)atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read first entry index
    printf("\nEnter index of first entry: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_IndexOfFirstEntry = (u16)atoi(InputBuffer);

    app_DsrHanBindReadTable(u16_NumOfEntries, u16_IndexOfFirstEntry);
}
//////////////////////////////////////////////////////////////////////////
void keyb_HanWriteBindTable(void)
{
#if 0
    ST_HAN_BIND_ENTRY arrSt_Binds[5];
    arrSt_Binds[0].st_SrcDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[0].st_SrcDeviceUnit.u16_DeviceId = 1;
    arrSt_Binds[0].st_SrcDeviceUnit.u8_UnitId  = 2;
    arrSt_Binds[0].st_DstDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[0].st_DstDeviceUnit.u16_DeviceId = 4;
    arrSt_Binds[0].st_DstDeviceUnit.u8_UnitId  = 5;
    arrSt_Binds[0].u16_InterfaceId     = 6;
    arrSt_Binds[0].u8_SrcRole      = 1; // 0=source is client, 1=source is server

    arrSt_Binds[1].st_SrcDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[1].st_SrcDeviceUnit.u16_DeviceId = 7;
    arrSt_Binds[1].st_SrcDeviceUnit.u8_UnitId  = 8;
    arrSt_Binds[1].st_DstDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[1].st_DstDeviceUnit.u16_DeviceId = 9;
    arrSt_Binds[1].st_DstDeviceUnit.u8_UnitId  = 10;
    arrSt_Binds[1].u16_InterfaceId     = 11;
    arrSt_Binds[1].u8_SrcRole      = 1; // 0=source is client, 1=source is server

    arrSt_Binds[2].st_SrcDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[2].st_SrcDeviceUnit.u16_DeviceId = 12;
    arrSt_Binds[2].st_SrcDeviceUnit.u8_UnitId  = 13;
    arrSt_Binds[2].st_DstDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[2].st_DstDeviceUnit.u16_DeviceId = 14;
    arrSt_Binds[2].st_DstDeviceUnit.u8_UnitId  = 15;
    arrSt_Binds[2].u16_InterfaceId     = 16;
    arrSt_Binds[2].u8_SrcRole      = 1; // 0=source is client, 1=source is server

    arrSt_Binds[3].st_SrcDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[3].st_SrcDeviceUnit.u16_DeviceId = 17;
    arrSt_Binds[3].st_SrcDeviceUnit.u8_UnitId  = 18;
    arrSt_Binds[3].st_DstDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[3].st_DstDeviceUnit.u16_DeviceId = 19;
    arrSt_Binds[3].st_DstDeviceUnit.u8_UnitId  = 20;
    arrSt_Binds[3].u16_InterfaceId     = 21;
    arrSt_Binds[3].u8_SrcRole      = 1; // 0=source is client, 1=source is server

    arrSt_Binds[4].st_SrcDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[4].st_SrcDeviceUnit.u16_DeviceId = 22;
    arrSt_Binds[4].st_SrcDeviceUnit.u8_UnitId  = 23;
    arrSt_Binds[4].st_DstDeviceUnit.u8_AddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Binds[4].st_DstDeviceUnit.u16_DeviceId = 24;
    arrSt_Binds[4].st_DstDeviceUnit.u8_UnitId  = 25;
    arrSt_Binds[4].u16_InterfaceId     = 26;
    arrSt_Binds[4].u8_SrcRole      = 1; // 0=source is client, 1=source is server

    app_DsrHanBindWriteTable(2, 1, arrSt_Binds);
#endif
}
//////////////////////////////////////////////////////////////////////////
void keyb_HanReadGroupTable(void)
{
    char InputBuffer[100];
    u16 u16_NumOfEntries;
    u16 u16_IndexOfFirstEntry;

    // read num of entries
    printf("\nEnter number of entries: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_NumOfEntries = (u16)atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read first entry index
    printf("\nEnter index of first entry: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_IndexOfFirstEntry = (u16)atoi(InputBuffer);

    app_DsrHanGroupReadTable(u16_NumOfEntries, u16_IndexOfFirstEntry);
}
//////////////////////////////////////////////////////////////////////////
void keyb_HanWriteGroupTable(void)
{
    ST_HAN_GROUP_ENTRY arrSt_Groups[5];
    const char str_group_name0[] = "han group #1";
    const char str_group_name1[] = "han group #2";
    const char str_group_name2[] = "han group #3";
    const char str_group_name3[] = "han group #4";
    const char str_group_name4[] = "han group #5";

    arrSt_Groups[0].u8_GroupId      = 1;
    memcpy(arrSt_Groups[0].u8_GroupName, str_group_name0, sizeof(str_group_name0));
    arrSt_Groups[0].st_DeviceUnit.u8_AddressType = 0x1; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Groups[0].st_DeviceUnit.u16_DeviceId  = 2;
    arrSt_Groups[0].st_DeviceUnit.u8_UnitId   = 3;

    arrSt_Groups[1].u8_GroupId      = 4;
    memcpy(arrSt_Groups[1].u8_GroupName, str_group_name1, sizeof(str_group_name1));
    arrSt_Groups[1].st_DeviceUnit.u8_AddressType = 0x1; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Groups[1].st_DeviceUnit.u16_DeviceId  = 5;
    arrSt_Groups[1].st_DeviceUnit.u8_UnitId   = 6;

    arrSt_Groups[2].u8_GroupId      = 7;
    memcpy(arrSt_Groups[2].u8_GroupName, str_group_name2, sizeof(str_group_name2));
    arrSt_Groups[2].st_DeviceUnit.u8_AddressType = 0x1; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Groups[2].st_DeviceUnit.u16_DeviceId  = 8;
    arrSt_Groups[2].st_DeviceUnit.u8_UnitId   = 9;

    arrSt_Groups[3].u8_GroupId      = 10;
    memcpy(arrSt_Groups[3].u8_GroupName, str_group_name3, sizeof(str_group_name3));
    arrSt_Groups[3].st_DeviceUnit.u8_AddressType = 0x1; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Groups[3].st_DeviceUnit.u16_DeviceId  = 11;
    arrSt_Groups[3].st_DeviceUnit.u8_UnitId   = 12;

    arrSt_Groups[4].u8_GroupId      = 13;
    memcpy(arrSt_Groups[4].u8_GroupName, str_group_name4, sizeof(str_group_name4));
    arrSt_Groups[4].st_DeviceUnit.u8_AddressType = 0x1; //(0x00=Individual Address, 0x01=Group Address)
    arrSt_Groups[4].st_DeviceUnit.u16_DeviceId  = 14;
    arrSt_Groups[4].st_DeviceUnit.u8_UnitId   = 15;

    app_DsrHanGroupWriteTable(2, 1, arrSt_Groups);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanCreateAttrReport()
{
    ST_IE_HAN_MSG stIe_Msg;
    ST_IE_HAN_MSG_CTL st_HANMsgCtl = { 0, 0, 0 };

    char   data_buffer[15];
    bool   nKeep = TRUE;


    stIe_Msg.pu8_Data = (u8*)&data_buffer;

    // Compose the message
    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    stIe_Msg.u8_DstAddressType = 0;

    printf("\nEnter Device Id (dec) : ");
    tcx_scanf("%hu", &(stIe_Msg.u16_DstDeviceId));

    printf("Enter Unit Id (dec): ");
    tcx_scanf("%hhu", &(stIe_Msg.u8_DstUnitId));


    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 0;   // will be returned in the response


    stIe_Msg.u16_InterfaceId    = ATTRIBUTE_REPORTING_INTERFACE_ID;
    stIe_Msg.e_MsgType      = CMBS_HAN_MSG_TYPE_CMD_WITH_RES;
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server

    while (nKeep)
    {
        printf("Report Type : \n");
        printf("\t1.Periodic Report \n");
        printf("\t2.Event Report \n");
        printf("Choose (1/2): ");
        tcx_scanf("%hhu", &(stIe_Msg.u8_InterfaceMember));
        nKeep = FALSE;
        if (stIe_Msg.u8_InterfaceMember == CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_CREATE_PERIODIC_REP)
        {
            stIe_Msg.u16_DataLen     = 5;
            stIe_Msg.pu8_Data[0]     = 0;
            stIe_Msg.pu8_Data[1]     = 0;
            stIe_Msg.pu8_Data[2]     = 0;
            stIe_Msg.pu8_Data[3]     = 1;
            stIe_Msg.pu8_Data[4]     = 1;
        }
        else if (stIe_Msg.u8_InterfaceMember == CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_CREATE_EVENT_REP)
        {
            stIe_Msg.u16_DataLen     = 3;
            stIe_Msg.pu8_Data[0]     = 0;
            stIe_Msg.pu8_Data[1]     = 0;
            stIe_Msg.pu8_Data[2]     = 0;

        }
        else
        {
            printf("Wrong Value\n");
            nKeep = TRUE;
        }
    }
    printf("---------------MESSAGE SUMMARY-------------------\n");
    printf("------------press any key to send----------------\n");

    app_HanDemoPrintMessageFields(&stIe_Msg);
    tcx_getch();


    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

    //appcmbs_WaitForContainer ( CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container );

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanDeleteAttrReport()
{
    ST_IE_HAN_MSG stIe_Msg;
    ST_IE_HAN_MSG_CTL st_HANMsgCtl = { 0, 0, 0 };

    char   data_buffer[15];


    stIe_Msg.pu8_Data = (u8*)&data_buffer;

    // Compose the message
    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    stIe_Msg.u8_DstAddressType = 0;

    printf("\nEnter Device Id (dec) : ");
    tcx_scanf("%hu", &(stIe_Msg.u16_DstDeviceId));

    printf("Enter Unit Id (dec): ");
    tcx_scanf("%hhu", &(stIe_Msg.u8_DstUnitId));


    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 0;   // will be returned in the response


    stIe_Msg.u16_InterfaceId    = ATTRIBUTE_REPORTING_INTERFACE_ID;
    stIe_Msg.e_MsgType      = CMBS_HAN_MSG_TYPE_CMD_WITH_RES;
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server

    stIe_Msg.u8_InterfaceMember    = CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_DELETE_REP;

    printf("Report ID (hex): ");
    tcx_scanf("%hhx", &(stIe_Msg.pu8_Data[0]));

    stIe_Msg.u16_DataLen     = 1;
    printf("---------------MESSAGE SUMMARY-------------------\n");
    printf("------------press any key to send----------------\n");

    app_HanDemoPrintMessageFields(&stIe_Msg);



    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int keyb_HanAddPeriodicReportEntry(ST_IE_HAN_MSG* stIe_Msg, int IndexInPayload)
{
    UNUSED_PARAMETER(stIe_Msg);
    UNUSED_PARAMETER(IndexInPayload);
    printf("NOT IMPLEMENTED YET - will be done in next versions\n");
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
u16  keyb_han_temp;
int keyb_HanAddEventReportEntry(ST_IE_HAN_MSG* stIe_Msg, int IndexInPayload)
{

    bool  nKeep;
    u8  NumOfElements;
    int  i;
    u8   PackType;

    printf("\nSender Unit Id (dec) : ");
    tcx_scanf("%hhu", &(stIe_Msg->pu8_Data[IndexInPayload++]));

    printf("\nSender Interface Id (hex) : ");
    tcx_scanf("%hx", &keyb_han_temp);

    // set highest bit in this entry to specify Event report entry for this interface
    keyb_han_temp |= 0x8000;
    stIe_Msg->pu8_Data[IndexInPayload++] = keyb_han_temp >> 8;
    stIe_Msg->pu8_Data[IndexInPayload++] = keyb_han_temp & 0xFF;

    nKeep = TRUE;
    while (nKeep)
    {
        printf("\nAttribute Pack : \n");
        printf("\t 1. All Mandatory (0x00)\n");
        printf("\t 2. All Mandatory+Optional (0xFE)\n");
        printf("\t 3. Dynamic (0xFF)\n");
        printf(" Choose (1-3) :");
        tcx_scanf("%hu", &keyb_han_temp);
        PackType = (u8)keyb_han_temp;
        nKeep = FALSE;
        NumOfElements = 1;
        if (PackType == 1)
        {
            PackType = stIe_Msg->pu8_Data[IndexInPayload++] = CMBS_HAN_ATTR_PACK_TYPE_ALL_MANDATORY;
        }
        else if (PackType == 2)
        {
            PackType = stIe_Msg->pu8_Data[IndexInPayload++] = CMBS_HAN_ATTR_PACK_TYPE_ALL_MANDATORY_AND_OPT;
        }
        else if (PackType == 3)
        {
            PackType = stIe_Msg->pu8_Data[IndexInPayload++] = CMBS_HAN_ATTR_PACK_TYPE_DYNAMIC;

            printf("\nDynamic Packing - Number of Elements (dec) (max 255) : ");
            tcx_scanf("%hu", &keyb_han_temp);
            NumOfElements = (u8)keyb_han_temp;
            stIe_Msg->pu8_Data[IndexInPayload++] = NumOfElements;
        }
        else
        {
            nKeep = TRUE;
        }

    }

    for (i = 0;  i < NumOfElements; i++)
    {
        if (PackType == CMBS_HAN_ATTR_PACK_TYPE_DYNAMIC)
        {
            printf("\nDynamic Packing - Attribute %d ID : ", i);
            stIe_Msg->pu8_Data[IndexInPayload++] = 0;
            tcx_scanf("%hhu", &(stIe_Msg->pu8_Data[IndexInPayload++]));
        }
        nKeep = TRUE;
        while (nKeep)
        {
            printf("\nReporting Type : \n");
            printf("\t 0. COV  (0x00)\n");
            printf("\t 1. HT   (0x01)\n");
            printf("\t 2. LT   (0x02)\n");
            printf("\t 3. Equal   (0x03)\n");
            printf("Choose (0-3) :");
            tcx_scanf("%hu", &keyb_han_temp);

            nKeep = FALSE;
            if (keyb_han_temp < 4)
            {
                stIe_Msg->pu8_Data[IndexInPayload++] = (u8)keyb_han_temp;
            }
            else
            {
                nKeep = TRUE;
            }

        }

        switch (keyb_han_temp)
        {
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_COV:
                printf(" COV - Percentage of change :");
                tcx_scanf("%hhu", &(stIe_Msg->pu8_Data[IndexInPayload++]));
                break;
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_HT:
                printf(" HT - High Threshold (max 255) :");
                stIe_Msg->pu8_Data[IndexInPayload++] = 1;
                tcx_scanf("%hhu", &(stIe_Msg->pu8_Data[IndexInPayload++]));
                break;
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_LH:
                printf(" LT - Low Threshold (max 255) :");
                stIe_Msg->pu8_Data[IndexInPayload++] = 1;
                tcx_scanf("%hhu", &(stIe_Msg->pu8_Data[IndexInPayload++]));

                break;
            case CMBS_HAN_ATTR_REPORT_REPORT_TYPE_EQUAL:
                printf(" Equal - Comparison (max 255) :");
                stIe_Msg->pu8_Data[IndexInPayload++] = 1;
                tcx_scanf("%hhu", &(stIe_Msg->pu8_Data[IndexInPayload++]));
                break;
            default:
                break;

        }
    }
    return IndexInPayload;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void keyb_HanAddReportEntry()
{
    int    i;
    ST_IE_HAN_MSG stIe_Msg;
    ST_IE_HAN_MSG_CTL st_HANMsgCtl = { 0, 0, 0 };

    char   data_buffer[100];
    int    CurrentPayloadIndex;


    stIe_Msg.pu8_Data = (u8*)&data_buffer;

    // Compose the message
    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    stIe_Msg.u8_DstAddressType = 0;

    printf("\nEnter Dest Device Id (dec) : ");
    tcx_scanf("%hu", &(stIe_Msg.u16_DstDeviceId));

    printf("\nEnter Dest Unit Id (dec) : ");
    tcx_scanf("%hhu", &(stIe_Msg.u8_DstUnitId));

    printf("Report Id (hex): ");
    tcx_scanf("%hhx", &(stIe_Msg.pu8_Data[0]));


    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 0;   // will be returned in the response


    stIe_Msg.u16_InterfaceId    = ATTRIBUTE_REPORTING_INTERFACE_ID;
    stIe_Msg.e_MsgType      = CMBS_HAN_MSG_TYPE_CMD;
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server

    stIe_Msg.u8_InterfaceMember = (stIe_Msg.pu8_Data[0] & 0x80) ? CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_EVENT_REP : CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_PERIODIC_REP;

    printf("Number of Entries: ");
    tcx_scanf("%hhx", &(stIe_Msg.pu8_Data[1]));

    CurrentPayloadIndex = 1;
    for (i = 0;  i < stIe_Msg.pu8_Data[1]; i++)
    {
        printf("\n\n ** Entry Number %d ** \n", i);
        if (stIe_Msg.u8_InterfaceMember == CMBS_HAN_ATTR_REPORT_C2S_CMD_ID_ADD_ENTRY_EVENT_REP)
        {
            CurrentPayloadIndex = keyb_HanAddEventReportEntry(&stIe_Msg, CurrentPayloadIndex + 1);
        }
        else
        {
            CurrentPayloadIndex = keyb_HanAddPeriodicReportEntry(&stIe_Msg, CurrentPayloadIndex + 1);
        }
    }
    stIe_Msg.u16_DataLen = CurrentPayloadIndex;
    printf("---------------MESSAGE SUMMARY-------------------\n");
    printf("------------press any key to send----------------\n");

    app_HanDemoPrintMessageFields(&stIe_Msg);



    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

    printf(" CURRENTLY THE DEVICE DOES NOT RETURN RESPONCE FOR SUCH REQUEST.\n Impossible to know the status\n");

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanReadReportsTable()
{
    printf("NOT IMPLEMENTED YET - will be done in next versions\n");
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void keyb_HanRegisterUnterface(void)
{
    char InputBuffer[100];
    ST_HAN_MSG_REG_INFO st_HANMsgRegInfo;

    // read number of entries
    printf("\n Enter unit Id (dec): ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    st_HANMsgRegInfo.u8_UnitId = (u8)atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read first entry index
    printf("\nEnter interface Id (hex) [ for all interfaces enter -> ffff ]: ");
    // Handles Dec IF id
    //tcx_gets( InputBuffer, sizeof(InputBuffer) );
    //st_HANMsgRegInfo.u16_InterfaceId = (u16)atoi(InputBuffer);

    tcx_scanf("%hx", &(st_HANMsgRegInfo.u16_InterfaceId));
    app_DsrHanMsgRecvRegister(&st_HANMsgRegInfo);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanUnregisterUnterface(void)
{
    char InputBuffer[100];
    ST_HAN_MSG_REG_INFO st_HANMsgRegInfo;

    printf("\n Enter unit Id (dec): ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    st_HANMsgRegInfo.u8_UnitId = (u8)atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    printf("\nEnter interface Id (hex) [ for all interfaces enter -> ffff ] : ");
    // Handles Dec IF id
    //tcx_gets( InputBuffer, sizeof(InputBuffer) );
    //st_HANMsgRegInfo.u16_InterfaceId = (u16)atoi(InputBuffer);

    tcx_scanf("%hx", &(st_HANMsgRegInfo.u16_InterfaceId));
    app_DsrHanMsgRecvUnregister(&st_HANMsgRegInfo);

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanSendTxRequest(void)
{
    char InputBuffer[100];
    u16 u16_DeviceId;

    printf("\n Enter device Id: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_DeviceId = (u16)atoi(InputBuffer);

    app_DsrHanMsgSendTxRequest(u16_DeviceId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void keyb_HanGetDeviceInfo(void)
{
    ST_IE_HAN_MSG   stIe_Msg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };


    printf("\n Enter Device Id (dec) : ");
    tcx_scanf("%hu", &(s_AttribManip.DeviceId));


    // Compose the message
    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;


    stIe_Msg.u16_DstDeviceId = s_AttribManip.DeviceId;
    stIe_Msg.u8_DstUnitId  = 0;

    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 0;   // will be returned in the response
    stIe_Msg.e_MsgType      = CMBS_HAN_MSG_TYPE_ATTR_GET_PACK;

    stIe_Msg.u16_InterfaceId    = DEVICE_INFORMATION_INTERFACE_ID;
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server
    stIe_Msg.u8_InterfaceMember   = CMBS_HAN_ATTR_PACK_TYPE_ALL_MANDATORY_AND_OPT;  // depending on message type, Command or attribute id
    stIe_Msg.u16_DataLen     = 0;
    stIe_Msg.pu8_Data      = NULL;

    printf("---------------MESSAGE SUMMARY-------------------\n");
    printf("------------press any key to send----------------\n");

    app_HanDemoPrintMessageFields(&stIe_Msg);
// tcx_getch();


    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanSendTxEnd(void)
{
    char InputBuffer[100];
    u16 u16_DeviceId;

    printf("\n Enter device Id: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_DeviceId = (u16)atoi(InputBuffer);

    app_DsrHanMsgSendTxEnd(u16_DeviceId);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanGetSetAttribute(void)
{
    char   FieldName[10];
    char    InputBuffer[100];
    int    i;

    ST_IE_HAN_MSG   stIe_Msg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };


    printf("\n Enter Device Id (dec) : ");
    tcx_scanf("%hu", &(s_AttribManip.DeviceId));

    printf("\n Enter Unit Id (dec): ");
    tcx_scanf("%hhu", &(s_AttribManip.UnitId));

    printf("\n Enter Interface Id (hex): ");
    tcx_scanf("%hx", &(s_AttribManip.InterfaceId));

    printf("\n Attribute (a) or Pack(p) ? (a/p): ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    s_AttribManip.Data[0] = 0;
    if (InputBuffer[0] == 'a' || InputBuffer[0] == 'A')
    {
        printf("\n Attribute Id (hex):");
        tcx_scanf("%hhx", &(s_AttribManip.AttrPackId)); // change

        switch (s_AttribManip.SetOrGet)
        {
            case KEYB_HAN_ATTR_GET:
                stIe_Msg.e_MsgType = CMBS_HAN_MSG_TYPE_ATTR_GET;          // message type
                break;
            case KEYB_HAN_ATTR_SET_WITHOUT_RESPONSE:
            default:
                stIe_Msg.e_MsgType = CMBS_HAN_MSG_TYPE_ATTR_SET;          // message type
                break;
            case KEYB_HAN_ATTR_SET_WITH_RESPONSE:
                stIe_Msg.e_MsgType = CMBS_HAN_MSG_TYPE_ATTR_SET_WITH_RES; // message type
                break;
        }


    }
    else if (InputBuffer[0] == 'p' || InputBuffer[0] == 'P')
    {
        printf("\n Pack Id (hex):");
        tcx_scanf("%hhx", &(s_AttribManip.AttrPackId));

        if (s_AttribManip.AttrPackId == CMBS_HAN_ATTR_PACK_TYPE_DYNAMIC)
        {
            // Dynamic packing
            printf("\n Number of Elements:  %s", FieldName);
            tcx_scanf("%hhu", &(s_AttribManip.ElementsInData));

            s_AttribManip.Data[0] = s_AttribManip.ElementsInData;
            for (i = 0;  i <  s_AttribManip.ElementsInData;  i++)
            {
                printf("\n \t Attribute ID (%d)(hex):  ", (i + 1));
                tcx_scanf("%hhu", &(s_AttribManip.Data[i + 1]));
            }
        }

        switch (s_AttribManip.SetOrGet)
        {
            case KEYB_HAN_ATTR_GET:
                stIe_Msg.e_MsgType = CMBS_HAN_MSG_TYPE_ATTR_GET_PACK;          // message type
                break;
            case KEYB_HAN_ATTR_SET_WITHOUT_RESPONSE:
            default:
                stIe_Msg.e_MsgType = CMBS_HAN_MSG_TYPE_ATTR_SET_PACK;          // message type
                break;
            case KEYB_HAN_ATTR_SET_WITH_RESPONSE:
                stIe_Msg.e_MsgType = CMBS_HAN_MSG_TYPE_ATTR_SET_PACK_WITH_RES; // message type
                break;
        }

    }
    else
    {
        printf("Error in field. Expected a (or A) or p (or P) \n");
        return;
    }

    // Compose the message
    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    stIe_Msg.u8_DstAddressType = 0;
    stIe_Msg.u16_DstDeviceId = s_AttribManip.DeviceId;
    stIe_Msg.u8_DstUnitId  = s_AttribManip.UnitId;

    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 0;   // will be returned in the response


    stIe_Msg.u16_InterfaceId    = s_AttribManip.InterfaceId;
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server
    stIe_Msg.u8_InterfaceMember   = s_AttribManip.AttrPackId;  // depending on message type, Command or attribute id

    // In case of set attribute, get payload from user
    if (stIe_Msg.e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_SET || stIe_Msg.e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_SET_WITH_RES) {

        // Get Attribue payload length from user
        printf("\n Enter payload length (dec) : ");
        tcx_scanf("%hhu", &(s_AttribManip.ElementsInData));

        if (s_AttribManip.ElementsInData >= CMBS_HAN_MAX_MSG_LEN)
        {
            s_AttribManip.ElementsInData = 0;
            printf("ERROR: Payload Length is exceeding the maximal length %d\nAborting...", CMBS_HAN_MAX_MSG_LEN);
            return;
        }

        // Get Attribue value from user
        for (i = 0; i < s_AttribManip.ElementsInData; i++)
        {
            printf("\n Enter payload element # %d (hex): ", (i + 1));
            tcx_scanf("%hhx", &(s_AttribManip.Data[i]));
        }

    }

    s_AttribManip.Data[s_AttribManip.ElementsInData] = 0;
    stIe_Msg.pu8_Data  = s_AttribManip.Data;
    stIe_Msg.u16_DataLen = s_AttribManip.ElementsInData ? (s_AttribManip.ElementsInData) : 0;


    printf("---------------MESSAGE SUMMARY-------------------\n");

    app_HanDemoPrintMessageFields(&stIe_Msg);

    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanGetAttribute(void)
{

    s_AttribManip.SetOrGet = KEYB_HAN_ATTR_GET;
    keyb_HanGetSetAttribute();

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void keyb_HanSetAttribute(void)
{
    char iChar;

    printf("\nSet with Response ? (y/n): ");

    iChar = tcx_getch();
    if ((iChar == 'y') || (iChar == 'Y'))
    {
        s_AttribManip.SetOrGet = KEYB_HAN_ATTR_SET_WITH_RESPONSE;
    }
    else if ((iChar == 'n') || (iChar == 'N'))
    {
        s_AttribManip.SetOrGet = KEYB_HAN_ATTR_SET_WITHOUT_RESPONSE;
    }

    else
    {
        printf("\nWrong input. Aborting... ");
        return;
    }

    keyb_HanGetSetAttribute();


}

void keyb_HanSendHanSuotaMsg(void)
{
    u8      u8_Buffer[CMBS_HAN_MAX_MSG_LEN];
    ST_IE_HAN_MSG   stIe_Msg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };

    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    printf("\nEnter destination device Id: ");
    tcx_gets(u8_Buffer, sizeof(u8_Buffer));
    stIe_Msg.u16_DstDeviceId = (u16)atoi(u8_Buffer);
    memset(u8_Buffer, 0, sizeof(u8_Buffer));

    stIe_Msg.u8_DstUnitId      = 0;
    stIe_Msg.u8_DstAddressType     = 0;  //(0x00=Individual Address, 0x01=Group Address)

    stIe_Msg.st_MsgTransport.u16_Reserved  = 0;
    stIe_Msg.u8_MsgSequence      = 43;   // will be returned in the response
    stIe_Msg.e_MsgType       = 1;         // message type - command
    stIe_Msg.u8_InterfaceType     = 0;      // 0=client , 1=server
    stIe_Msg.u16_InterfaceId     = 0x7f05; // for SUOTA
    stIe_Msg.u8_InterfaceMember     = 1;

    stIe_Msg.pu8_Data = u8_Buffer;
    printf("\nEnter URL: ");
    tcx_gets(u8_Buffer, sizeof(u8_Buffer));
    stIe_Msg.u16_DataLen = strlen(u8_Buffer);


    printf("--------------- MESSAGE SUMMARY -------------------\n");
    //print message summary
    app_HanDemoPrintMessageFields(&stIe_Msg);

    app_DsrHanMsgSend(49, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyb_HanSendHanMsg(void)
{
    u8    u8_Buffer[CMBS_HAN_MAX_MSG_LEN];
    int    iChar;
    u8    i;

    ST_IE_HAN_MSG   stIe_Msg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };

    stIe_Msg.pu8_Data = u8_Buffer;

    printf("\nEnter message manually? (y/n)");
    iChar = tcx_getch();
    if ((iChar == 'y') || (iChar == 'Y'))
    {
        printf("\nEnter source device Id (0): ");
        tcx_scanf("%hu", &(stIe_Msg.u16_SrcDeviceId));

        printf("Enter source unit Id (2): ");
        tcx_scanf("%hhu", &(stIe_Msg.u8_SrcUnitId));


        printf("Enter destination device Id: ");
        tcx_scanf("%hu", &(stIe_Msg.u16_DstDeviceId));

        printf("Enter destination unit Id (e.g.1): ");
        tcx_scanf("%hhu", &(stIe_Msg.u8_DstUnitId));

        printf("Enter destination address type (e.g.0): ");
        tcx_scanf("%hhu", &(stIe_Msg.u8_DstAddressType));

        stIe_Msg.st_MsgTransport.u16_Reserved = 0;

        stIe_Msg.u8_MsgSequence = (u8) stIe_Msg.u16_DstDeviceId; // for now we use the low bye of Device Id

        printf("Enter message type (e.g. command=1;AttGet=4;AttSet=6): ");
        tcx_scanf("%hhu", &(stIe_Msg.e_MsgType));
        stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server
        printf("Enter interface Id(in HEX) (200 for on/off): ");
        tcx_scanf("%hx", &(stIe_Msg.u16_InterfaceId));

        printf("Enter InterfaceMember (for command: on=1;off=2): ");
        tcx_scanf("%hu", &(stIe_Msg.u8_InterfaceMember));

        stIe_Msg.u16_DataLen       = 0;
        printf("Enter Decimal DataLen (0 up to %d): ", CMBS_HAN_MAX_MSG_DATA_LEN);
        tcx_scanf("%hu", &(stIe_Msg.u16_DataLen));

        printf("keyb_HanSendHanMsg Data Length:%d\n", stIe_Msg.u16_DataLen);
        if (stIe_Msg.u16_DataLen)
        {
            if (stIe_Msg.u16_DataLen <= CMBS_HAN_MAX_MSG_DATA_LEN)
            {
                for (i = 0; i < stIe_Msg.u16_DataLen; i++)
                {
                    printf("Enter payload byte no. %d: ", i);
                    tcx_scanf("%hhu", &(u8_Buffer[i]));
                }
            }
            else
                stIe_Msg.u16_DataLen = 0;

        }
        printf("--------------- MESSAGE SUMMARY -------------------\n");
        //print message summary
        app_HanDemoPrintMessageFields(&stIe_Msg);
    }

    else
    {

        stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
        stIe_Msg.u16_SrcDeviceId = 0;
        stIe_Msg.u8_SrcUnitId  = 2;

        stIe_Msg.u8_DstAddressType = 0;
        printf("\n\nEnter destination device Id: ");
        tcx_scanf("%hu", &(stIe_Msg.u16_DstDeviceId));
        stIe_Msg.u8_DstUnitId  = 1;

        stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
        stIe_Msg.u8_MsgSequence     = 53;   // will be returned in the response
        stIe_Msg.e_MsgType      = 1;          // message type
        stIe_Msg.u16_InterfaceId    = 0x200;
        stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server

        printf("\nChoose Command to Execute:\n");
        printf("--------------------------\n");
        printf("1 => On\n");
        printf("2 => Off\n");
        iChar = tcx_getch();

        if (iChar == '1') {
            stIe_Msg.u8_InterfaceMember = 1;
        }
        else if (iChar == '2') {
            stIe_Msg.u8_InterfaceMember = 2;
        }

        stIe_Msg.u16_DataLen = 0;

        printf("--------------- MESSAGE SUMMARY -------------------\n");
        //print message summary
        app_HanDemoPrintMessageFields(&stIe_Msg);
    }

    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);

}
//////////////////////////////////////////////////////////////////////////
void keyb_HanSendCommandToUnit(void)
{
    app_HanDemoSendMessage2Device();
}

void keyb_HanAttrReportingLoop(void)
{
    int n_Keep = TRUE;

    while (n_Keep)
    {
        printf("------------------------------------ \n");
        printf("1 => Create Report     \n");
        printf("2 => Delete Report     \n");
        printf("3 => Add Report Entry     \n");
        printf("4 => Read Reports Table    \n");
        printf("a => Register interface    \n");
        printf("b => Unregister interface    \n");
        printf("c => Send Tx request     \n");
        printf("d => Send Tx end      \n");
        printf("e => Send Han Msg      \n");
        printf("? => Get Device Connection Status (In Link ? )    \n");
        printf("- - - - - - - - - - - - - - -   \n");
        printf("q => Return to Interface Menu\n");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case '1':
                keyb_HanCreateAttrReport();
                break;
            case '2':
                keyb_HanDeleteAttrReport();
                break;
            case '3':
                keyb_HanAddReportEntry();
                break;
            case '4':
                keyb_HanReadReportsTable();
                break;

            case 'a':
            case 'A':
                keyb_HanRegisterUnterface();
                break;

            case 'b':
            case 'B':
                keyb_HanUnregisterUnterface();
                break;

            case 'c':
            case 'C':
                keyb_HanSendTxRequest();
                break;

            case 'd':
            case 'D':
                keyb_HanSendTxEnd();
                break;

            case '?':
                keyb_HanGetConnectionStatus();
                break;
            case 'q':
            case 'Q':
                n_Keep = FALSE;
                break;

            default:
                break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void keyb_HanTestLoop(void)
{
    int n_Keep = TRUE;

    while (n_Keep)
    {
        printf("------------------------------------ \n");
        printf("1 => Han Manager Init     \n");
        printf("2 => Start Han Manager    \n");
        printf("3 => Read Device Table    \n");
        printf("4 => Read Extended Device Table  \n");
        printf("5 => Write Device Table    \n");
        printf("6 => Read Bind Table     \n");
        printf("7 => Write Bind Table     \n");
        printf("8 => Read Group Table     \n");
        printf("9 => Write Group Table    \n");
        printf("a => Register interface    \n");
        printf("b => Unregister interface    \n");
        printf("c => Send Tx request     \n");
        printf("d => Send Tx end      \n");
        printf("e => Send FUN Msg      \n");
        printf("g => Get Attribute/Pack    \n");
        printf("s => Set Attribute/Pack    \n");
        printf("i => Get Device Info     \n");
        printf("r => Attribute Reporting    \n");
        printf("# => Send FUN SUOTA msg       \n");
        printf("- => Delete Han Device    \n");
        printf("? => Get Device Connection Status (In Link ? )    \n");


        printf("- - - - - - - - - - - - - - -   \n");
        printf("q => Return to HAN Menu\n");

        switch (tcx_getch())
        {
            case 'x':
                tcx_appClearScreen();
                break;

            case '1':
//            keyb_HanSendMngrInit();
                app_HanDemoRegularStart();
                break;

            case '2':
                keyb_HanSendMngrStart();
                break;

            case '3':
                keyb_HanReadDeviceTable(TRUE);
                break;

            case '4':
                keyb_HanReadDeviceTable(FALSE);
                break;

            case '5':
                keyb_HanWriteDeviceTable();
                break;

            case '6':
                keyb_HanReadBindTable();
                break;

            case '7':
                keyb_HanWriteBindTable();
                break;

            case '8':
                keyb_HanReadGroupTable();
                break;

            case '9':
                keyb_HanWriteGroupTable();
                break;

            case 'a':
            case 'A':
                keyb_HanRegisterUnterface();
                break;

            case 'b':
            case 'B':
                keyb_HanUnregisterUnterface();
                break;

            case 'c':
            case 'C':
                keyb_HanSendTxRequest();
                break;

            case 'g':
            case 'G':
                keyb_HanGetAttribute();
                break;

            case 's':
            case 'S':
                keyb_HanSetAttribute();
                break;

            case 'r':
            case 'R':
                keyb_HanAttrReportingLoop();
                break;

            case 'i':
            case 'I':
                keyb_HanGetDeviceInfo();
                break;

            case 'd':
            case 'D':
                keyb_HanSendTxEnd();
                break;

            case 'e':
            case 'E':
                keyb_HanSendHanMsg();
                break;

            case '#':
                keyb_HanSendHanSuotaMsg();
                break;

            case '-':
                keyb_HanDeleteDevice();
                break;

            case '?':
                keyb_HanGetConnectionStatus();
                break;

            case 'q':
            case 'Q':
                n_Keep = FALSE;
                break;

            default:
                break;
        }
    }
}

void keyb_HanLoop(void)
{
    int n_Keep = TRUE;


    while (n_Keep)
    {
        u8 hanEepromValue = 0x0;

        //tcx_appClearScreen();

        printf("\n------------------------------------\n");
        printf("1 => Make regular startup    \n");
        printf("2 => Send command to unit    \n");
        printf("3 => Test commands     \n");
#ifdef HAN_SERVER_DEMO
        printf("4 => Run HAN server     \n");
#endif // HAN_SERVER_DEMO
        printf("- - - - - - - - - - - - - - -   \n");
        printf("q => Return to Interface Menu\n");
        switch (tcx_getch())
        {
            case 'x':
                tcx_appClearScreen();
                break;

            case '1':
                app_HanDemoRegularStart();
                break;

            case '2':
                keyb_HanSendCommandToUnit();
                break;

            case '3':
                keyb_HanTestLoop();
                break;

#ifdef HAN_SERVER_DEMO

            case '4':
                keyb_HanServerStart();
                //n_Keep = FALSE;
                break;

#endif // HAN_SERVER_DEMO

            case 'q':
            case 'Q':
                n_Keep = FALSE;
                break;

            default:
                break;
        }

    }
}

#ifdef HAN_SERVER_DEMO

static void keyb_HanServerStart(void)
{
    han_serverStart();
}

#endif // HAN_SERVER_DEMO
