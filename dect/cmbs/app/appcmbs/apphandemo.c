/*!
*   \file       apphan.c
*   \brief      HAN API
*   \author     CMBS Team
*
*
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "cmbs_han.h"
#include "cmbs_han_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "apphan.h"
#include "tcx_keyb.h"
#include <tcx_util.h>

//////////////////////////////////////////////////////////////////////////
#define  CMBS_MAX_UNIT_NUMBERS   (5)

void app_HanDemoAlertIfMsgHandler(ST_IE_HAN_MSG*pMsg);
void app_HanDemoOnOfIfMsgHandler(ST_IE_HAN_MSG*pMsg);
char* app_HanDemoAlertIF_CommandName(ST_IE_HAN_MSG* pMsg);
char* app_HanDemoOnOff_CommandName(ST_IE_HAN_MSG* pMsg);
void app_HanDemoAlertMakeAction(ST_HAN_DEVICE_ENTRY* pUnit);
void app_HanDemoOnOffMakeAction(ST_HAN_DEVICE_ENTRY* pUnit);

//////////////////////////////////////////////////////////////////////////
///////////////// UTILITY FUNCTIONS  /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

char * app_HanDemoCMBS_Han_Interface_Type(u16 u16_InterfaceId)
{
    switch (u16_InterfaceId)
    {
        case DEVICE_MGMT_INTERFACE_ID:
            return (char*)"DEVICE MANAGEMENT";
            break;
        case BIND_MGMT_INTERFACE_ID:
            return (char*)"BIND MANAGEMENT";
            break;
        case GROUP_MGMT_INTERFACE_ID:
            return (char*)"GROUP MANAGEMENT";
            break;
        case ALERT_INTERFACE_ID:
            return (char*)"ALERT";
            break;
        case ON_OFF_INTERFACE_ID:
            return (char*)"ON_OFF";
            break;
        case 0xFFFF:
            return (char*)"ALL INTERFACES";
            break;
        default:
            return (char*)"UNDEFINED";
            break;
    }
}
//////////////////////////////////////////////////////////////////////////
char * app_HanDemoCMBS_Han_Unit_Type(u16 u16_unitType)
{
    switch (u16_unitType)
    {
        case HAN_UNIT_TYPE_AC_OUTLET:
            return (char*)"AC OUTLET              ";
            break;
        case HAN_UNIT_TYPE_SMOKE_DETECTOR:
            return (char*)"SMOKE SENSOR           ";
            break;
        case HAN_UNIT_TYPE_GLASS_BREAK_DETECTOR:
            return (char*)"GLASS BREAK SENSOR     ";
            break;
        case HAN_UNIT_TYPE_MOTION_DETECTOR:
            return (char*)"MOTION DETECTION SENSOR";
            break;
        default:
            return (char*)"UNDEFINED";
            break;
    }
}
//////////////////////////////////////////////////////////////////////////
u16 app_HanDemoGetUnitInterface(u16 u16_UnitType)
{
    switch (u16_UnitType)
    {
        case HAN_UNIT_TYPE_AC_OUTLET:
            return ON_OFF_INTERFACE_ID;
            break;
        case HAN_UNIT_TYPE_SMOKE_DETECTOR:
            return ALERT_INTERFACE_ID;
            break;
        case HAN_UNIT_TYPE_GLASS_BREAK_DETECTOR:
            return ALERT_INTERFACE_ID;
            break;
        case HAN_UNIT_TYPE_MOTION_DETECTOR:
            return ALERT_INTERFACE_ID;
            break;
        default:
            return ALERT_INTERFACE_ID;
            break;
    }
}

//////////////////////////////////////////////////////////////////////////
///////////////// ALERT INTERFACE /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void app_HanDemoAlertIfMsgHandler(ST_IE_HAN_MSG*pMsg)
{
    u8 u8_Index;
    printf(" \"%s\" command received from D'%dU'%d \n",
           app_HanDemoAlertIF_CommandName(pMsg),
           pMsg->u16_SrcDeviceId, pMsg->u8_SrcUnitId);

    if (pMsg->u16_DataLen)
    {
        printf("\nCommand Data: ");
        for (u8_Index = 0; u8_Index < pMsg->u16_DataLen; u8_Index++)
        {
            printf("%x ", pMsg->pu8_Data[u8_Index]);
        }
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char* app_HanDemoAlertIF_CommandName(ST_IE_HAN_MSG* pMsg)
{
    // nothing to display in client-2-server direction
    if (pMsg->u8_InterfaceType == 0)
        return (char*)"UNKNOWN";

    if (CMBS_HAN_MSG_TYPE_CMD == pMsg->e_MsgType)
    {
        if (pMsg->u8_InterfaceMember == ALERT_IF_SERVER_CMD_ALERT_ON)
        {
            return (char*)"Alert On";
        }
        else if (pMsg->u8_InterfaceMember == ALERT_IF_SERVER_CMD_ALERT_OFF)
        {
            return (char*)"Alert Off";
        }
    }
    return (char*)"UNKNOWN";
}
//////////////////////////////////////////////////////////////////////////
void app_HanDemoAlertMakeAction(ST_HAN_DEVICE_ENTRY* pUnit)
{
    ST_IE_HAN_MSG stIe_Msg;
    ST_IE_HAN_MSG_CTL st_HANMsgCtl = { 0, 0, 0 };
    printf("\nOnly ATTRIBUTE GET command available for this interface");
    printf("\nPress any key to send this command");
    tcx_getch();

    stIe_Msg.u8_DstAddressType       = 0;
    stIe_Msg.u16_SrcDeviceId      = 0;
    stIe_Msg.u8_SrcUnitId      = 2;
    stIe_Msg.u16_DstDeviceId     = pUnit->st_DeviceUnit.u16_DeviceId;
    stIe_Msg.u8_DstUnitId      = pUnit->st_DeviceUnit.u8_UnitId;
    stIe_Msg.st_MsgTransport.u16_Reserved  = 0;
    stIe_Msg.u8_MsgSequence      = 0;
    stIe_Msg.u16_InterfaceId     = ALERT_INTERFACE_ID;
    stIe_Msg.u8_InterfaceType     = 0;
    stIe_Msg.e_MsgType       = CMBS_HAN_MSG_TYPE_ATTR_GET;
    stIe_Msg.u8_InterfaceMember    = 1;
    stIe_Msg.u16_DataLen      = 0;

    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);
}
//////////////////////////////////////////////////////////////////////////
///////////////// ON/OFF INTERFACE /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void app_HanDemoOnOfIfMsgHandler(ST_IE_HAN_MSG*pMsg)
{
    // no commands can be received from this interfaces
    printf(" \"%s\" command received from D'%dU'%d \n",
           app_HanDemoOnOff_CommandName(pMsg),
           pMsg->u16_SrcDeviceId, pMsg->u8_SrcUnitId);
}
//////////////////////////////////////////////////////////////////////////
char* app_HanDemoOnOff_CommandName(ST_IE_HAN_MSG* pMsg)
{
    // only allowed command type for this interface
    if (pMsg->e_MsgType == CMBS_HAN_MSG_TYPE_ATTR_GET_RES)
    {
        return (char*)"Attribute Get Response";
    }
    return (char*)"UNKNOWN";
}
//////////////////////////////////////////////////////////////////////////
void app_HanDemoOnOffMakeAction(ST_HAN_DEVICE_ENTRY* pUnit)
{
    ST_IE_HAN_MSG stIe_Msg;
    ST_IE_HAN_MSG_CTL st_HANMsgCtl = { 0, 0, 0 };

    printf("\n Please choose one of the following actions:");
    printf("\n 1 TURN ON");
    printf("\n 2 TURN OFF");
    printf("\n 3 TOGGLE");
    printf("\n 4 GET ATTRIBUTE");
    switch (tcx_getch())
    {
        case '1':
            stIe_Msg.e_MsgType            = CMBS_HAN_MSG_TYPE_CMD;
            stIe_Msg.u8_InterfaceMember  = ON_OFF_IF_SERVER_CMD_TURN_ON;
            break;

        case '2':
            stIe_Msg.e_MsgType            = CMBS_HAN_MSG_TYPE_CMD;
            stIe_Msg.u8_InterfaceMember   = ON_OFF_IF_SERVER_CMD_TURN_OFF;
            break;

        case '3':
            stIe_Msg.e_MsgType             = CMBS_HAN_MSG_TYPE_CMD;
            stIe_Msg.u8_InterfaceMember  = ON_OFF_IF_SERVER_CMD_TOGGLE;
            break;

        case '4':
            stIe_Msg.e_MsgType               = CMBS_HAN_MSG_TYPE_ATTR_GET;
            stIe_Msg.u8_InterfaceMember     = 1;
            break;

        default:
            printf("\n Wrong choice, nothing will be sent!!!!!");
            return;
            break;
    }
    stIe_Msg.u8_DstAddressType  = 0;
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;
    stIe_Msg.u16_DstDeviceId = pUnit->st_DeviceUnit.u16_DeviceId;
    stIe_Msg.u8_DstUnitId  = pUnit->st_DeviceUnit.u8_UnitId;

    stIe_Msg.st_MsgTransport.u16_Reserved = 0;
    stIe_Msg.u8_MsgSequence                     = 0;
    stIe_Msg.u16_InterfaceId                    = ON_OFF_INTERFACE_ID;
    stIe_Msg.u8_InterfaceType                   = 0;
    stIe_Msg.u16_DataLen                        = 0;

    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);
}
//////////////////////////////////////////////////////////////////////////
void app_HanDemoPrintMessageFields(ST_IE_HAN_MSG* pMsg)
{
    u8 u8_Index;
    printf("Source address      : (Unit type = %d, D'%dU'%d)\n",
           pMsg->u8_DstAddressType,
           pMsg->u16_SrcDeviceId,
           pMsg->u8_SrcUnitId);

    printf("Destination address : (Unit type = %d, D'%dU'%d)\n",
           pMsg->u8_DstAddressType,
           pMsg->u16_DstDeviceId,
           pMsg->u8_DstUnitId);

    printf("Transport : %d\n", pMsg->st_MsgTransport.u16_Reserved);
    printf("Message Sequence : %d\n", pMsg->u8_MsgSequence);
    printf("Message Type : %d \n", pMsg->e_MsgType);
    printf("Interface Type : %d, Interface ID : %d, Interface Member : %d \n", pMsg->u8_InterfaceType, pMsg->u16_InterfaceId, pMsg->u8_InterfaceMember);
    printf("Payload : %d bytes \n", pMsg->u16_DataLen);
    for (u8_Index = 0; u8_Index < pMsg->u16_DataLen; u8_Index++)
    {
        printf(" %d", pMsg->pu8_Data[u8_Index]);
    }
}
//////////////////////////////////////////////////////////////////////////
void app_HanDemoPrintHanMessage(void * pv_IE)
{
    u8 u8_Buffer[CMBS_HAN_MAX_MSG_LEN];
    ST_IE_HAN_MSG stIe_Msg;
    stIe_Msg.pu8_Data = u8_Buffer;
    cmbs_api_ie_HANMsgGet(pv_IE, &stIe_Msg);

    if (stIe_Msg.u16_InterfaceId == ALERT_INTERFACE_ID)
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
    }
}

//////////////////////////////////////////////////////////////////////////
///////////////// USER INTERACTION FUNCTIONS /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void app_HanDemoSendMessage2Device()
{
    ST_APPCMBS_CONTAINER st_Container;
    ST_HAN_DEVICE_ENTRY  arr_Devices[6];
    u8      u8_Length = 0, u8_Index;
    appcmbs_PrepareRecvAdd(TRUE);

    app_DsrHanDeviceReadTable(4, 0, (u8)TRUE);
    appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES, &st_Container);

    if (st_Container.ch_Info[0] == 0)
    {
        printf("\nError during read devices");
        return;
    }

    u8_Length = st_Container.ch_Info[0];
    memcpy(&arr_Devices, &(st_Container.ch_Info[2]), u8_Length * sizeof(ST_HAN_DEVICE_ENTRY));

    printf("\n Please select device ");

    for (u8_Index = 0; u8_Index < u8_Length; u8_Index++)
    {
        printf("\n%u %s D'%uU'%u",
               u8_Index,
               app_HanDemoCMBS_Han_Unit_Type(arr_Devices[u8_Index].u16_UnitType),
               arr_Devices[u8_Index].st_DeviceUnit.u16_DeviceId,
               arr_Devices[u8_Index].st_DeviceUnit.u8_UnitId);
    }

    u8_Index = (u8)tcx_getch() - '0';
    if (u8_Index >= CMBS_MAX_UNIT_NUMBERS) u8_Index = 0;

    switch (app_HanDemoGetUnitInterface(arr_Devices[u8_Index].u16_UnitType))
    {
        case ALERT_INTERFACE_ID:
            app_HanDemoAlertMakeAction(&arr_Devices[u8_Index]);
            break;
        case ON_OFF_INTERFACE_ID:
            app_HanDemoOnOffMakeAction(&arr_Devices[u8_Index]);
            break;
        default:
            printf("\nNo action available for this unit");
            break;
    }
}
//////////////////////////////////////////////////////////////////////////
void app_HanDemoRegularStart(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    ST_HAN_CONFIG   stHanCfg;
    stHanCfg.u8_HANServiceConfig = 0; // everything implemented internally
    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanMngrInit(&stHanCfg);
    appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_MNGR_INIT_RES, &st_Container);

    if ((st_Container.n_InfoLen != 1) || (st_Container.ch_Info[0] != 0))
    {
        printf("\n--------------------------------------------");
        printf("\n Error during HAN application initialization");
        printf("\n--------------------------------------------");
        return;
    }

    memset(&st_Container, 0x0, sizeof(ST_APPCMBS_CONTAINER));
    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanMngrStart();
    appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_MNGR_START_RES, &st_Container);
    if ((st_Container.n_InfoLen != 1) || (st_Container.ch_Info[0] != 0))
    {
        printf("\n--------------------------------------------");
        printf("\n Error during HAN application start");
        printf("\n--------------------------------------------");
    }
    else
    {
        printf("\n--------------------------------------------");
        printf("\n Regular startup procedure performed successfully");
        printf("\n--------------------------------------------");
    }
}
