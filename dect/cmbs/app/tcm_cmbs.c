#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "cmbs_platf.h"

#include "ctrl_common_lib.h"
#include "cmbs_int.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "appswup.h"
#include "tcx_util.h"
#include "cmbs_voipline.h"
#include "hanfun_protocol_defs.h"
#include "tcm_cmbs.h"
#include "tcm_call.h"

static unsigned int g_dect_register_device_id = 0;

#define APPL_VERSION 0x0363
#define APPL_BUILD 0x000F
//#define TCM_KEEP_ALIVE_PERIODIC 360 /* 360 seconds */
//#define TCM_ALERT_PERIODIC  15
//#define TCM_TAMPER_PERIODIC  30

#define  TAMPER_INTERFACE_ID  0x0101
#define  MALFUNCTION_INTERFACE_ID  0x0102
#define  POWER_INTERFACE_ID  0x0110
#define  TEST_INTERFACE_ID  0x0200
#define  DEVICE_INFORMATION_ID  0x0005
#define  ATTRIBUTE_REPORTING_ID 0x0006


// setup TDM interface
ST_TDM_CONFIG  g_st_TdmCfg;
ST_CMBS_DEV    g_st_DevMedia;
// setup communication path to CMBS
ST_UART_CONFIG g_st_UartCfg;
ST_CMBS_DEV    g_st_DevCtl;
//#define PACKETSIZE 128 // SPI restrictions
extern ST_CMBS_UPGRADE_SETUP g_stSwupSetup;

extern ST_CB_LOG_BUFFER pfn_log_buffer_Cb;
char sz_EEpromFile[128] = "/usr/share/dspg/eeprom0.bin";

ST_EXTVOIPLINE voiplineTcm[TCM_CALL_MAX_NUM];

//DECT_HAN_DEVICES_STATUS g_Device_status;
DECT_HAN_DEVICES g_Device_info;
char g_Register_Name[64] = "Unspecified";
char g_Register_Location[32] = "r0000000";

extern int tcx_EepNewFile(const char * psz_EepFileName, u32 size);
extern int tcx_EepOpen(char * psz_EepFileName);
extern int tcx_EepRead(u8* pu8_OutBuf, u32 u32_Offset, u32 u32_Size);
extern u32 tcx_EepSize(void);
extern E_CMBS_RC tcx_LogOutputCreate(void);
extern E_CMBS_RC app_DsrHanMngrInit(ST_HAN_CONFIG * pst_HANConfig);
extern E_CMBS_RC app_DsrHanMngrStart(void);
extern int appcmbs_CleanMessage(void);
extern E_CMBS_RC app_DsrHanDeviceReadTable(u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8 isBrief);
extern E_CMBS_RC app_DsrHanMsgSend(u16 u16_RequestId, u16 u16_DestDeviceId, ST_IE_HAN_MSG_CTL* pst_HANMsgCtl , ST_IE_HAN_MSG * pst_HANMsg);
extern E_CMBS_RC app_DsrHanDeleteDevice(u16 u16_DeviceId);
extern E_CMBS_RC app_DsrHanMsgRecvRegister(ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo);

extern void dect_device_register_complete(int result, unsigned int id);
extern void dect_handset_register_complete(TCM_DEVICE_REGISTER_STUTAS_E result, DECT_HAN_DEVICE_INF *device);
extern unsigned int dect_convert_cmbs_id_to_dect(unsigned int id, bool isHandset);
extern void dect_device_data_init();

static void tcm_device_status_change();

void dect_device_status_change(DECT_HAN_DEVICE_INF *devinfo);
void dect_device_connection_status_change(int bconnection, DECT_HAN_DEVICE_INF *devinfo);

u16   tcx_ApplVersionGet(void)
{
    return APPL_VERSION;
}

int   tcx_ApplVersionBuildGet(void)
{
    return APPL_BUILD;
}

static int cmbs_init(E_CMBS_HW_COM_TYPE com_type, u8 com_port)
{
    int ret = CMBS_RC_OK;
    bool tdm_type = 0;

    // detect Com Port
    if (com_port == 0) {
        /* interactive mode */
        com_port = tcx_DetectComPort(TRUE, &g_st_DevCtl.e_DevType);
    }
    printf("COM PORT: %d\n", com_port);

    if (tcx_LogOutputCreate() != CMBS_RC_OK)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "tcx_LogOutputCreate fail!\n");;
    }
    // Config Device Type
    if (com_type == CMBS_HW_COM_TYPE_USB)
        tcx_USBConfig(com_port);
    else if (com_type == CMBS_HW_COM_TYPE_UART) {
        tcx_UARTConfig(com_port);
        tcx_TDMConfig(tdm_type);
    }

    // Start Init
    ret = appcmbs_Initialize(NULL, &g_st_DevCtl, &g_st_DevMedia, pfn_log_buffer_Cb);
    if (ret != CMBS_RC_OK) {
        printf("call appcmbs_Initialize failed!\n");
        return ret;
    }

    return CMBS_RC_OK;
}
int tcm_cmbs_init(void)
{
    //int size = 8192;
    int ret = 0;

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "DSP Group Demo Software Version:%04X Build:%d\n", tcx_ApplVersionGet(), tcx_ApplVersionBuildGet());


    //ret = cmbs_init(CMBS_HW_COM_TYPE_UART/*CMBS_HW_COM_TYPE_USB*/, 0);
    ret = cmbs_init(CMBS_HW_COM_TYPE_UART, 1);
    if (ret != CMBS_RC_OK) {
        printf("CMBS Init failed!\n");
        return -1;
    }

    // Try to get HW Chip version
    if ((g_CMBSInstance.u16_TargetVersion & 0xFF00) != 0x0000 &&   // Bootloader
            (g_CMBSInstance.u16_TargetVersion & 0xF000) != 0x2000 && // CMBS 2xxx version (e.g. 2.99.9) - Old IE Header structure
            (g_CMBSInstance.u16_TargetVersion & 0xFF00) != 0x0200)   // CMBS 02xx version (e.g. 2.99) - Old IE Header structure
        app_SrvHWVersionGet(TRUE);

    extvoip_InitVoiplines();
    voiplineTcm[0].LineID = 0;
    extvoip_RegisterVoipline(0, &voiplineTcm[0]);
    /* register h3 to line 1 */
    voiplineTcm[1].LineID = 1;
    extvoip_RegisterVoipline(1, &voiplineTcm[1]);

    // Start DECT model
    ret = appcmbs_CordlessStart(NULL);
    if (ret == FALSE) {
        printf("CMBS CordlessStart failed!\n");
        return -1;
    }


    return 0;
}

int tcm_han_mgr_start(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    ST_HAN_CONFIG   stHanCfg;
    stHanCfg.u8_HANServiceConfig = 0; // everything implemented internally
    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanMngrInit(&stHanCfg);
    appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_MNGR_INIT_RES, &st_Container);

    if ((st_Container.n_InfoLen != 1) || (st_Container.ch_Info[0] != 0))
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Error during HAN application initialization\n");
        return -1;
    }

    memset(&st_Container, 0x0, sizeof(ST_APPCMBS_CONTAINER));
    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanMngrStart();
    appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_MNGR_START_RES, &st_Container);
    if ((st_Container.n_InfoLen != 1) || (st_Container.ch_Info[0] != 0))
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Error during HAN application start\n");
    }
    else
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Regular startup procedure performed successfully\n");
    }
    return 0;
}
static DECT_DEVICE_TYPE  tcm_GetDeviceType(int type)
{
    switch (type)
    {
        case HAN_UNIT_TYPE_ONOFF_SWITCHABLE :
        {
            return DECT_TYPE_ONOFF_SWITCHABLE;
        }
        case HAN_UNIT_TYPE_ONOFF_SWITCH :
        {
            return DECT_TYPE_ONOFF_SWITCH;
        }
        case HAN_UNIT_TYPE_LEVEL_CONTROLABLE:
        {
            return DECT_TYPE_LEVEL_CONTROLABLE;
        }
        case HAN_UNIT_TYPE_LEVEL_CONTROL:
        {
            return DECT_TYPE_LEVEL_CONTROL;
        }
        case HAN_UNIT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE:
        {
            return DECT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE;
        }
        case HAN_UNIT_TYPE_LEVEL_CONTROL_SWITCH:
        {
            return DECT_TYPE_LEVEL_CONTROL_SWITCH;
        }
        case HAN_UNIT_TYPE_AC_OUTLET:
        {
            return DECT_TYPE_AC_OUTLET;
        }
        case HAN_UNIT_TYPE_AC_OUTLET_WITH_POWER_METERING:
        {
            return DECT_TYPE_AC_OUTLET_WITH_POWER_METERING;
        }
        case HAN_UNIT_TYPE_LIGHT:
        {
            return DECT_TYPE_LIGHT;
        }
        case HAN_UNIT_TYPE_DIMMABLE_LIGHT:
        {
            return DECT_TYPE_DIMMABLE_LIGHT;
        }
        case HAN_UNIT_TYPE_DIMMER_SWITCH:
        {
            return DECT_TYPE_DIMMER_SWITCH;
        }
        case HAN_UNIT_TYPE_DOOR_LOCK:
        {
            return DECT_TYPE_DOOR_LOCK;
        }
        case HAN_UNIT_TYPE_DOOR_BELL:
        {
            return DECT_TYPE_DOOR_BELL;
        }
        case HAN_UNIT_TYPE_POWER_METER:
        {
            return DECT_TYPE_POWER_METER;
        }
        case HAN_UNIT_TYPE_DETECTOR:
        {
            return DECT_TYPE_DETECTOR;
        }
        case HAN_UNIT_TYPE_DOOR_OPEN_CLOSE_DETECTOR:
        {
            return DECT_TYPE_DOOR_OPEN_CLOSE_DETECTOR;
        }
        case HAN_UNIT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR:
        {
            return DECT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR;
        }
        case HAN_UNIT_TYPE_MOTION_DETECTOR:
        {
            return DECT_TYPE_MOTION_DETECTOR;
        }
        case HAN_UNIT_TYPE_SMOKE_DETECTOR:
        {
            return DECT_TYPE_SMOKE_DETECTOR;
        }
        case HAN_UNIT_TYPE_GAS_DETECTOR:
        {
            return DECT_TYPE_GAS_DETECTOR;
        }
        case HAN_UNIT_TYPE_FLOOD_DETECTOR:
        {
            return DECT_TYPE_FLOOD_DETECTOR;
        }
        case HAN_UNIT_TYPE_GLASS_BREAK_DETECTOR:
        {
            return DECT_TYPE_GLASS_BREAK_DETECTOR;
        }
        case HAN_UNIT_TYPE_VIBRATION_DETECTOR:
        {
            return DECT_TYPE_VIBRATION_DETECTOR;
        }
        default:
        {
            return DECT_TYPE_MAX;
        }
    }
}

const char*  tcm_GetDeviceName(int type)
{
    switch (type)
    {
        case DECT_TYPE_ONOFF_SWITCHABLE :
        {
            return "switch";
        }
        case DECT_TYPE_ONOFF_SWITCH :
        {
            return "switch";
        }
        case DECT_TYPE_LEVEL_CONTROLABLE:
        {
            return "control";
        }
        case DECT_TYPE_LEVEL_CONTROL:
        {
            return "control";
        }
        case DECT_TYPE_LEVEL_CONTROLABLE_SWITCHABLE:
        {
            return "switch";
        }
        case DECT_TYPE_LEVEL_CONTROL_SWITCH:
        {
            return "switch";
        }
        case DECT_TYPE_AC_OUTLET:
        {
            return "ac outlet";
        }
        case DECT_TYPE_AC_OUTLET_WITH_POWER_METERING:
        {
            return "ac outlet with power meter";
        }
        case DECT_TYPE_LIGHT:
        {
            return "light";
        }
        case DECT_TYPE_DIMMABLE_LIGHT:
        {
            return "dimmer light";
        }
        case DECT_TYPE_DIMMER_SWITCH:
        {
            return "dimmer switch";
        }
        case DECT_TYPE_DOOR_LOCK:
        {
            return "lock";
        }
        case DECT_TYPE_DOOR_BELL:
        {
            return "bell";
        }
        case DECT_TYPE_POWER_METER:
        {
            return "power";
        }
        case DECT_TYPE_DETECTOR:
        {
            return "detector";
        }
        case DECT_TYPE_DOOR_OPEN_CLOSE_DETECTOR:
        {
            return "door";
        }
        case DECT_TYPE_WINDOW_OPEN_CLOSE_DETECTOR:
        {
            return "window";
        }
        case DECT_TYPE_MOTION_DETECTOR:
        {
            return "motion";
        }
        case DECT_TYPE_SMOKE_DETECTOR:
        {
            return "smoke";
        }
        case DECT_TYPE_GAS_DETECTOR:
        {
            return "gas";
        }
        case DECT_TYPE_FLOOD_DETECTOR:
        {
            return "flood";
        }
        case DECT_TYPE_GLASS_BREAK_DETECTOR:
        {
            return "glass break";
        }
        case DECT_TYPE_VIBRATION_DETECTOR:
        {
            return "vibration";
        }
        default:
        {
            return "unknow";
        }
    }
}
int tcm_GetDeviceList(DECT_HAN_DEVICES *pDevice_info)
{
    ST_APPCMBS_CONTAINER st_Container;
    //ST_HAN_DEVICE_ENTRY  arr_Devices[6];
    u8 u8_Index;
    //DECT_DEVICE_INFO_STRUCT device;
    ST_IE_HAN_EXTENDED_DEVICE_ENTRIES *st_HANDeviceEntries = NULL;
    time_t timer;
    u8 *p = NULL;

    timer = time(NULL);
    //memset(&device, 0, sizeof(device));

    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);

    app_DsrHanDeviceReadTable(CMBS_HAN_MAX_DEVICES, 0, (u8)FALSE);
    if (0 == appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES, &st_Container))
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Read device list failure!!!\n");
        return 1;
    }

    st_HANDeviceEntries = (ST_IE_HAN_EXTENDED_DEVICE_ENTRIES *)&st_Container;
#if 0   //debug
    {
        int i = 0;

        int a[4] = {HAN_UNIT_TYPE_SMOKE_DETECTOR,
                    HAN_UNIT_TYPE_LEVEL_CONTROLABLE,
                    HAN_UNIT_TYPE_AC_OUTLET_WITH_POWER_METERING,
                    HAN_UNIT_TYPE_DOOR_BELL
                   };
        st_HANDeviceEntries->u16_NumOfEntries = 4;
        for (i = 0; i < 4; i++)
        {
            st_HANDeviceEntries->pst_DeviceEntries[i].u16_DeviceId = i + 1;
            st_HANDeviceEntries->pst_DeviceEntries[i].u8_NumberOfUnits = 2;
            st_HANDeviceEntries->pst_DeviceEntries[i].st_UnitsInfo[0].u16_UnitType = a[i];
            st_HANDeviceEntries->pst_DeviceEntries[i].st_UnitsInfo[1].u16_UnitType = 0;
        }
    }
#endif
    printf("\nu16_NumOfEntries %d\n", st_HANDeviceEntries->u16_NumOfEntries);
    pDevice_info->device_num = st_HANDeviceEntries->u16_NumOfEntries;

    for (u8_Index = 0; u8_Index < st_HANDeviceEntries->u16_NumOfEntries; u8_Index++)
    {
        pDevice_info->device[u8_Index].id = dect_convert_cmbs_id_to_dect(st_HANDeviceEntries->pst_DeviceEntries[u8_Index].u16_DeviceId, 0);

        p = st_HANDeviceEntries->pst_DeviceEntries[u8_Index].u8_IPUI;
        sprintf(pDevice_info->device[u8_Index].ipei, "%02X%02X%02X%02X%02X", p[0], p[1], p[2], p[3], p[4]);

        if (st_HANDeviceEntries->pst_DeviceEntries[u8_Index].u8_NumberOfUnits >= 1)
        {
            /* save device type */
            pDevice_info->device[u8_Index].type = tcm_GetDeviceType(st_HANDeviceEntries->pst_DeviceEntries[u8_Index].st_UnitsInfo[0].u16_UnitType);
        }

        pDevice_info->device[u8_Index].connection = 0;
        p = pDevice_info->device[u8_Index].ipei;
        printf("\n device:%d ipei:%s type:%0x \n", pDevice_info->device[u8_Index].id, pDevice_info->device[u8_Index].ipei, st_HANDeviceEntries->pst_DeviceEntries[u8_Index].st_UnitsInfo[0].u16_UnitType);
    }
    return 0;

}

int tcm_dect_device_on_off(int on, int dstid)
{
    u8    u8_Buffer[CMBS_HAN_MAX_MSG_LEN];
    //int    iChar;
    //u8    i;
    ST_APPCMBS_CONTAINER st_Container;

    ST_IE_HAN_MSG   stIe_Msg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };

    stIe_Msg.pu8_Data = u8_Buffer;

    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    stIe_Msg.u8_DstAddressType = 0;
    //printf("\n\nEnter destination device Id: ");
    //tcx_scanf("%hu",&(stIe_Msg.u16_DstDeviceId));
    stIe_Msg.u16_DstDeviceId = dstid;
    stIe_Msg.u8_DstUnitId  = 1;

    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 53;   // will be returned in the response
    stIe_Msg.e_MsgType      = 1;          // message type
    stIe_Msg.u16_InterfaceId    = 0x200;
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server

    /* 1:on, 2:off */
    if (on)
    {
        stIe_Msg.u8_InterfaceMember = 1;
    }
    else
    {
        stIe_Msg.u8_InterfaceMember = 2;
    }

    stIe_Msg.u16_DataLen = 0;

    printf("--------------- MESSAGE SUMMARY -------------------\n");
    //print message summary
    app_HanDemoPrintMessageFields(&stIe_Msg);

    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST_RES, &st_Container))
    {
        return 0;
    }
    else
    {
        return 1;
    }

}


int tcm_dect_device_slide(int value, int dstid)
{
    u8    u8_Buffer[CMBS_HAN_MAX_MSG_LEN];
    //int    iChar;
    //u8    i;
    ST_APPCMBS_CONTAINER st_Container;

    ST_IE_HAN_MSG   stIe_Msg;
    ST_IE_HAN_MSG_CTL   st_HANMsgCtl = { 0, 0, 0 };

    stIe_Msg.pu8_Data = u8_Buffer;

    stIe_Msg.u8_DstAddressType = 0; //(0x00=Individual Address, 0x01=Group Address)
    stIe_Msg.u16_SrcDeviceId = 0;
    stIe_Msg.u8_SrcUnitId  = 2;

    stIe_Msg.u8_DstAddressType = 0;
    //printf("\n\nEnter destination device Id: ");
    //tcx_scanf("%hu",&(stIe_Msg.u16_DstDeviceId));
    stIe_Msg.u16_DstDeviceId = dstid;
    stIe_Msg.u8_DstUnitId  = 1;

    stIe_Msg.st_MsgTransport.u16_Reserved = 0; // Transport Reserved
    stIe_Msg.u8_MsgSequence     = 53;   // will be returned in the response
    stIe_Msg.e_MsgType      = 1;          // message type
    stIe_Msg.u16_InterfaceId    = 0x201;    /* level control */
    stIe_Msg.u8_InterfaceType    = 1;     // 0=client , 1=server


    stIe_Msg.u8_InterfaceMember = value;


    stIe_Msg.u16_DataLen = 0;

    //printf("--------------- MESSAGE SUMMARY -------------------\n");
    //print message summary
    //app_HanDemoPrintMessageFields(&stIe_Msg);

    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanMsgSend(12, stIe_Msg.u16_DstDeviceId, &st_HANMsgCtl, &stIe_Msg);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST_RES, &st_Container))
    {
        return 0;
    }
    else
    {
        return 1;
    }

}

int tcm_close_register(void)
{
    ST_APPCMBS_CONTAINER st_Container;

    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);
    app_SrvSubscriptionClose();
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_CORD_CLOSEREG_RES, &st_Container))
    {
        printf("\n fuc:%s line:%d %d\n", __FUNCTION__, __LINE__, st_Container.n_Info);
        if (st_Container.n_Info)
        {
            printf("\n fuc:%s line:%d \n", __FUNCTION__, __LINE__);
            return 0;
        }
    }
    printf("\n fuc:%s line:%d \n", __FUNCTION__, __LINE__);
    return 1;
}
int tcm_handset_delete(char *hs)
{
    ST_APPCMBS_CONTAINER st_Container;

    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);
    app_SrvHandsetDelete(hs);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_HS_DELETE_RES, &st_Container))
    {
        printf("\n fuc:%s line:%d %d\n", __FUNCTION__, __LINE__, st_Container.n_Info);
        if (st_Container.n_Info)
        {
            printf("\n fuc:%s line:%d \n", __FUNCTION__, __LINE__);
            return 0;
        }
    }
    printf("\n fuc:%s line:%d \n", __FUNCTION__, __LINE__);
    return 1;
}
int tcm_handset_list(DECT_HAN_DEVICES *pDevice_info)
{
    ST_APPCMBS_CONTAINER st_Container;
    ST_IE_HANDSETINFO *p = NULL;
    int i = 0;
    int handset_num = 0;

    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);
	
    app_SrvRegisteredHandsets(0xFFFF, 1);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES, &st_Container))
    {
        p = (ST_IE_HANDSETINFO *)&st_Container.ch_Info;
        for (i = 0; i < st_Container.n_Info; i++)
        {
            
            pDevice_info->device[i].type = DECT_TYPE_HANDSET;
            pDevice_info->device[i].id = dect_convert_cmbs_id_to_dect(p->u8_Hs, 1);
            p++;
        }
        handset_num = st_Container.n_Info;
        pDevice_info->device_num = handset_num;
    }
    return handset_num;
}

int tcm_add_device(unsigned int timeout)
{
    unsigned int time = 0;
    ST_APPCMBS_CONTAINER st_Container;

    if (timeout > 0)
    {
        time = timeout;
    }
    else
    {
        time = 58;
    }

    g_dect_register_device_id = 0;
    
    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);
    app_SrvSubscriptionOpen(time);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_CORD_OPENREG_RES, &st_Container))
    {
        printf("\n fuc:%s line:%d %d\n", __FUNCTION__, __LINE__, st_Container.n_Info);
        if (st_Container.n_Info)
        {
            printf("\n fuc:%s line:%d \n", __FUNCTION__, __LINE__);
            return 0;
        }
    }
    printf("\n fuc:%s line:%d \n", __FUNCTION__, __LINE__);
    return 1;
}
int tcm_delete_device(unsigned int  id)
{
    u16 u16_DeviceId;
    ST_APPCMBS_CONTAINER st_Container;
//    int i = 0;

    if (0 == id)
    {
        u16_DeviceId = 0xffff;
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "delete all device...\n");
    }
    else
    {

        u16_DeviceId = id;
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "delete device %u...\n", id);
    }

    printf("\n delete device %d\n", id);
    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);
    app_DsrHanDeleteDevice(u16_DeviceId);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE_RES, &st_Container))
    {
        if (CMBS_RESPONSE_OK == st_Container.n_Info)
        {
            /* delete device from cache */
            return 0;
        }
        else
        {
            printf("\n delete fail, reason:%s\n", st_Container.ch_Info);
        }

    }

    return 1;
}

void tcm_intialize_device_info()
{
    memset(&g_Device_info, 0, sizeof(g_Device_info));

}

void tcm_HanRegisterUnterface(void)
{
    ST_HAN_MSG_REG_INFO st_HANMsgRegInfo;
    st_HANMsgRegInfo.u8_UnitId = 2;
    st_HANMsgRegInfo.u16_InterfaceId = 0xffff;
    app_DsrHanMsgRecvUnregister(&st_HANMsgRegInfo);
}

void tcm_HanRegisterInterface(void)
{

    ST_HAN_MSG_REG_INFO st_HANMsgRegInfo;


    st_HANMsgRegInfo.u8_UnitId = 2;

    // read first entry index
    st_HANMsgRegInfo.u16_InterfaceId = 0xffff;

    app_DsrHanMsgRecvRegister(&st_HANMsgRegInfo);
}
int tcm_ping()
{
    ST_APPCMBS_CONTAINER        st_Container;

    appcmbs_CleanMessage();
    appcmbs_PrepareRecvAdd(TRUE);
    cmbs_dsr_Ping(g_cmbsappl.pv_CMBSRef);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_PING_RES, &st_Container))
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

void tcm_GetBaseName(char *pName, int len)
{
    ST_APPCMBS_CONTAINER        st_Container;
    ST_IE_BASE_NAME            st_BaseName;
    int i = 0;

    app_SrvGetBaseName(1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_GET_BASE_NAME_RES, &st_Container);

    memcpy(&st_BaseName.u8_BaseName, &st_Container.ch_Info, st_Container.n_InfoLen);

    memset(pName, 0, len);
    //strncpy(pName, st_BaseName.u8_BaseName, len - 1);
    for (i = 0; i < len - 1; i++)
    {
        if ((i >= CMBS_BS_NAME_MAX_LENGTH) || (st_BaseName.u8_BaseName[i] == '\0'))
        {
            break;
        }
        pName[i] = st_BaseName.u8_BaseName[i];
    }

}
int tcm_get_cmbs_info(DECT_CMBS_INFO *pInfo)
{
    char ch_Version[80];
    appcmbs_VersionGet(ch_Version);
    snprintf(pInfo->app_ver, sizeof(pInfo->app_ver), "%02x.%02x - Build %x", (tcx_ApplVersionGet() >> 8), (tcx_ApplVersionGet() & 0xFF), tcx_ApplVersionBuildGet());
    snprintf(pInfo->target_ver, sizeof(pInfo->target_ver),  "%s", ch_Version);

    pInfo->regester = g_cmbsappl.RegistrationWindowStatus;
    pInfo->status = tcm_ping();
    tcm_GetBaseName(pInfo->basename, sizeof(pInfo->basename));

    return 0;
}

int tcm_handset_page(unsigned int id)
{
    int ret = 0;
    char handset[32] = {0};
    if (id)
    {
        sprintf(handset, "%u", id);
    }
    else
    {
        sprintf(handset, "%s", "all");
    }
    if (CMBS_RC_OK != app_SrvHandsetPage(handset))
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "app_SrvHandsetPage failure...\n");
        ret = 1;
    }

    return ret;
}
int tcm_handset_stop_page(void)
{
    if (CMBS_RC_OK != app_SrvHandsetStopPaging())
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "app_SrvHandsetStopPaging failure...\n");
        return 1;
    }
    return 0;
}

int tcm_dect_type_get(int *type)
{
    ST_APPCMBS_CONTAINER st_Container;

    // get current DECT Type
    app_ProductionParamGet(CMBS_PARAM_DECT_TYPE, CMBS_PARAM_DECT_TYPE_LENGTH);
    if (appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container))
    {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Current DECT type is: 0x%X %s ", 
            st_Container.ch_Info[0], _DectType2Str(st_Container.ch_Info[0]));
        *type = st_Container.ch_Info[0];
        return 0;
        
    }
    else
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "app_ProductionParamGet failure...\n");
        return 1;
    }

}

int tcm_dect_type_set(int type)
{
    ST_APPCMBS_CONTAINER st_Container;
    u8  u8_Value;
    int ret = 1;


    u8_Value = (u8)type;

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "set dect type %s\n", _DectType2Str(type));
    
    if (u8_Value < CMBS_DECT_TYPE_MAX)
    {
        app_ProductionParamSet(CMBS_PARAM_DECT_TYPE, &u8_Value, sizeof(u8_Value), 1);
        if (!appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container))
        {
           ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "app_ProductionParamSet failure...\n"); 
        }
    }
    if (st_Container.n_Info == TRUE)
    {
        // reboot module only in case the set was successful
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "Rebooting module...Please wait...\n");
        ret = 0;

    }

    return ret;
}

int tcm_dect_system_reboot()
{
    int ret = CMBS_RC_OK;
    ret = app_SrvSystemReboot();
    if (ret != CMBS_RC_OK)
    {
        ret = 1;;
    }
    return ret;
}
int tcm_device_image_upgrade(char *fw_file_path)
{
    int ret = CMBS_RC_OK;
    u16 version = 0x0000;

    g_stSwupSetup.u16_PacketSize = 4;
    g_stSwupSetup.u16_PacketSize = 1 << (5 + g_stSwupSetup.u16_PacketSize);

    // Get subscription data
    app_SrvEEPROMBackupCreate();

    // get firmware version
    ret = app_SwupGetImageVersion(fw_file_path, &version);
    if (ret != CMBS_RC_OK)
    {
        printf("can't get version of firmware file!\n");
        return ret;
    }

    // Stop All Service
    //dect_handle_upgrade_before();
    // wait 0.2 seconds
    //SleepMs(200);

    // perform firmware upgrade
    printf("Starting FW Upgrade: File: %s, PacketSize: %d, version:0x%2x \n" ,
           fw_file_path, g_stSwupSetup.u16_PacketSize, version);
    ret = app_UpdateAndCheckVersion(fw_file_path, version);
    if (ret != CMBS_RC_OK)
    {
        printf("call app_UpdateAndCheckVersion failed!\n");
        // Start All Service
        //dect_handle_upgrade_after();
        return ret;
    }

    // Set subscription data
    app_SrvEEPROMBackupRestore();
    printf("\nEEPROM data set successfully!...\n");

    // Start All Service
    //dect_handle_upgrade_after();

    //app_SrvLocateSuggest(CMBS_ALL_HS_MASK); //Page all HSs

    return CMBS_RC_OK;
}

static void tcm_handle_han_msg_message(ST_IE_HAN_MSG *pMsg)
{
    int i = 0;
    int status = 0;
    time_t timer;
    int ischange = 0;
    timer = time(NULL);

    if (pMsg->u16_SrcDeviceId > DECT_CMBS_DEVICES_MAX)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Invalid device id [%d], interface id [%0x]!!!\n", pMsg->u16_SrcDeviceId, pMsg->u16_InterfaceId);
        return;
    }

    for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
    {
        if (g_Device_info.device[i].id == dect_convert_cmbs_id_to_dect(pMsg->u16_SrcDeviceId, 0))
        {
            /* rcv device message, device is alive */
            g_Device_info.device[i].keepalive = timer;
            if (DECT_DEVICE_STATUS_DISCONNECTION == g_Device_info.device[i].connection)
            {
                /* device online */
                g_Device_info.device[i].connection = DECT_DEVICE_STATUS_CONNECTION;
                //dect_device_status_change(&g_Device_info.device[i]);
                dect_device_connection_status_change(1, &g_Device_info.device[i]);
                ischange = 1;
                ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "device [%d] online \n", pMsg->u16_SrcDeviceId);
            }
            
            switch (pMsg->u16_InterfaceId)
            {
                case ALERT_INTERFACE_ID:
                {
                    g_Device_info.device[i].alert = timer;
                    if (pMsg->u16_DataLen && pMsg->pu8_Data)
                    {
                        status = pMsg->pu8_Data[pMsg->u16_DataLen - 1]; /* 0:idle, 1,2,3: alarm */
                        
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive alert message, device id = [%u] data = [%u]\n", pMsg->u16_SrcDeviceId, pMsg->pu8_Data[pMsg->u16_DataLen - 1]);
                    }
                    else
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive alert message, device id = [%u], no data\n", pMsg->u16_SrcDeviceId);
                    }
                    if (g_Device_info.device[i].device.binary_sensor.value != status)
                    {
                        /* alert change */
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Device [%d] state change, old state [%d], new state [%d] \n", 
                                             g_Device_info.device[i].id, g_Device_info.device[i].device.binary_sensor.value, status);
                        g_Device_info.device[i].device.binary_sensor.value = status;
                        dect_device_status_change(&g_Device_info.device[i]);
                        ischange = 1;
                    }
                    break;
                }
                case KEEP_ALIVE_INTERFACE_ID:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive keep alive message, device id = [%d]\n", pMsg->u16_SrcDeviceId);
                    break;
                }
                case TAMPER_INTERFACE_ID:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive tamper alarm\n");

                    if (g_Device_info.device[i].hardware_state.tamper == 0)
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Tamper alarm...\n");
                        g_Device_info.device[i].hardware_state.tamper = 1;
                        dect_device_status_change(&g_Device_info.device[i]);
                    }
                    g_Device_info.device[i].tamper = timer;
                    break;
                }
                case MALFUNCTION_INTERFACE_ID:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive malfunction alarm\n");
                    if (pMsg->u16_DataLen && pMsg->pu8_Data[pMsg->u16_DataLen - 1])
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "malfunction , data = [%d]\n", pMsg->pu8_Data[pMsg->u16_DataLen - 1]);
                        g_Device_info.device[i].hardware_state.mulfunction = 1;
                    }
                    else
                    {
                        g_Device_info.device[i].hardware_state.mulfunction = 0;
                    }
                    break;
                }
                case POWER_INTERFACE_ID:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive low battery alarm\n");
                    if (pMsg->u16_DataLen && pMsg->pu8_Data[pMsg->u16_DataLen - 1])
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "low battery, data = [%d]\n", pMsg->pu8_Data[pMsg->u16_DataLen - 1]);
                        g_Device_info.device[i].hardware_state.low_battery = 1;
                    }
                    else
                    {
                        g_Device_info.device[i].hardware_state.low_battery = 0;
                    }
                    break;
                }
                case TEST_INTERFACE_ID:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Receive test key message\n");
                    g_Device_info.device[i].keepalive = timer;
                    if (pMsg->u16_DataLen && pMsg->pu8_Data[pMsg->u16_DataLen - 1])
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Test key on \n");
                    }
                    else
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Test key off \n");
                    }
                    break;
                }
                default:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Unhandle CMBS_EV_DSR_HAN_MSG_RECV message, interface id [%d] \n", pMsg->u16_InterfaceId);
                    break;
                }
            }

            break;
        }
    }

    if (ischange)
    {
        g_Device_info.index += 1;
    }
    if (i >= DECT_CMBS_DEVICES_MAX)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Invalid device id [%d], search device failure!!!\n", pMsg->u16_SrcDeviceId);
    }
}

void *tcm_DeviceKeepAliveThread(void *args)
{
    time_t mtime;
    int i  = 0;
    int change = 0;
    while (1)
    {
        change = 0;
        sleep(3);
        mtime = time(NULL);
        for (i = 0; i < DECT_CMBS_DEVICES_MAX; i++)
        {
            if (g_Device_info.device[i].id)
            {
                /* keep alive timeout */
                if (g_Device_info.device[i].connection)
                {
                    if (mtime - g_Device_info.device[i].keepalive > DECT_KEEP_ALIVE_PERIODIC)
                    {
                        /* device disconnection */
                        g_Device_info.device[i].connection = DECT_DEVICE_STATUS_DISCONNECTION;
                        //dect_device_status_change(&g_Device_info.device[i]);
                        dect_device_connection_status_change(0, &g_Device_info.device[i]);
                        change = 1;
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "device disconnection,id = [%d] IPEI = [%s] !!!!\n", g_Device_info.device[i].id, g_Device_info.device[i].ipei);
                    }
                }

                /* binarysensor alert timeout */
                if (g_Device_info.device[i].type >= DECT_TYPE_DETECTOR &&
                     g_Device_info.device[i].type <= DECT_TYPE_VIBRATION_DETECTOR)
                {
                    if (g_Device_info.device[i].device.binary_sensor.value)
                    {
                        if (mtime - g_Device_info.device[i].alert > DECT_ALERT_PERIODIC)
                        {
                            /* device stop alarm */
                            g_Device_info.device[i].device.binary_sensor.value = 0;
                            dect_device_status_change(&g_Device_info.device[i]);
                            change = 1;
                            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Error:Alert message timeout, device id = [%u] IPEI = [%s] !!!!\n", g_Device_info.device[i].id, g_Device_info.device[i].ipei);
                        }
                    }
                }
                /* device tamper timeout */
                if (g_Device_info.device[i].hardware_state.tamper)
                {
                    if (mtime - g_Device_info.device[i].tamper > DECT_TAMPER_PERIODIC)
                    {
                        g_Device_info.device[i].hardware_state.tamper = 0;
                        dect_device_status_change(&g_Device_info.device[i]);
                        change = 1;
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Tamper message timeout, device id = [%u] IPEI = [%s] !!!!\n", g_Device_info.device[i].id, g_Device_info.device[i].ipei);
                    }
                }

            }
        }
        if (change)
        {
            g_Device_info.index += 1;
        }
    }
    return NULL;
}

/*pthread_t g_hanDeviceKeepAliveThreadId;
int tcm_createDeviceKeepAliveThread(void)
{
    int ret;
    pthread_attr_t  attr;
    //pthread_t id;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024);
//   ret = pthread_create(&id, &attr, tcm_DeviceKeepAliveThread, NULL);
    ret = pthread_create(&g_hanDeviceKeepAliveThreadId, &attr, tcm_DeviceKeepAliveThread, NULL);
    pthread_attr_destroy(&attr);

    return ret;
}*/

void* tcm_DeviceReportThread(void *args)
{
    int   nRetVal;
    ST_APPCMBS_LINUX_CONTAINER LinuxContainer;

    while (1)
    {
        nRetVal = msgrcv(g_cmbsappl.n_MssgReport, &LinuxContainer, sizeof(ST_APPCMBS_CONTAINER), 0, 0);
        if (nRetVal == -1)
        {

        }
        else
        {
            switch (LinuxContainer.Content.n_Event)
            {
                case CMBS_EV_DSR_HAN_MSG_RECV:
                {
                    ST_IE_HAN_MSG *pMsg = (ST_IE_HAN_MSG *)&LinuxContainer.Content.ch_Info;
                    tcm_handle_han_msg_message(pMsg);
                    break;
                }
                case CMBS_EV_DSR_HS_REGISTERED:
                {
                    DECT_HAN_DEVICE_INF dev_info;
                    ST_IE_HANDSETINFO *st_HsInfo = (ST_IE_HANDSETINFO *)&LinuxContainer.Content.ch_Info;
                    memset(&dev_info, 0, sizeof(dev_info));
                    dev_info.id = dect_convert_cmbs_id_to_dect(st_HsInfo->u8_Hs, 1);
                    printf("\nfun:%s line:%d CMBS_EV_DSR_HS_REGISTERED id = %u\n", __FUNCTION__, __LINE__, dev_info.id );
                    snprintf(dev_info.ipei, sizeof(dev_info.ipei), "%0X%0X%0X%0X", 
                        st_HsInfo->u8_IPEI[0], st_HsInfo->u8_IPEI[1], st_HsInfo->u8_IPEI[2], st_HsInfo->u8_IPEI[3]);
                    sprintf(dev_info.name, "handset-%u", st_HsInfo->u8_Hs);
                    dev_info.type = DECT_TYPE_HANDSET;
                    dect_handset_register_complete(TCM_DEVICE_REGISTER_SUCCESS, &dev_info);
                    break;
                }
                case CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_1_NOTIFICATION:
                {
                    g_dect_register_device_id = 0;
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_1_NOTIFICATION \n");
                    if (CMBS_HAN_GENERAL_STATUS_ERROR == LinuxContainer.Content.n_Info)
                    {
                        //notify_daemon_register_fail();
                        dect_device_register_complete(TCM_DEVICE_REGISTER_FAILURE, 0);
                    }
                    break;
                }
                case CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_2_NOTIFICATION:
                {
                    g_dect_register_device_id = (u16)LinuxContainer.Content.ch_Info;
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_2_NOTIFICATION id [%u]\n", g_dect_register_device_id);
                    if (CMBS_HAN_GENERAL_STATUS_ERROR == LinuxContainer.Content.n_Info)
                    {
                        dect_device_register_complete(TCM_DEVICE_REGISTER_FAILURE, 0);
                    }
                    break;
                }
                case CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_3_NOTIFICATION:
                {
                    u16 *pID = (u16*)&LinuxContainer.Content.ch_Info;
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_3_NOTIFICATION, device id [%d], status [%d]\n", *pID, LinuxContainer.Content.n_Info);
                    dect_device_register_complete(TCM_DEVICE_REGISTER_SUCCESS, dect_convert_cmbs_id_to_dect(*pID, 0));
                    break;
                }
                case CMBS_EV_DSR_CORD_CLOSEREG:
                {
                    if (CMBS_REG_CLOSE_TIMEOUT == LinuxContainer.Content.n_Info)
                    {
                        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Device register timeout \n");
                        dect_device_register_complete(TCM_DEVICE_REGISTER_FAILURE, 0);
                    }
                    break;
                }
                case CMBS_EV_DEE_CALL_ESTABLISH:
                case CMBS_EV_DEE_CALL_INBANDINFO:
                case CMBS_EV_DEE_CALL_PROGRESS:
                case CMBS_EV_DEE_CALL_ANSWER:
                case CMBS_EV_DEE_CALL_RELEASE:
                case CMBS_EV_DEE_CALL_RELEASECOMPLETE:
                case CMBS_EV_DEE_CALL_HOLD:
                case CMBS_EV_DEE_CALL_RESUME:
                case CMBS_EV_DCM_CALL_STATE:
                {
                    tcm_HandleEndPointCallEvent(&LinuxContainer);
                    break;
                }
                case CMBS_EV_DSR_HAN_DEVICE_REG_DELETED:
                {
                    dect_device_register_complete(TCM_DEVICE_REGISTER_FAILURE, 0);

                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "CMBS_EV_DSR_HAN_DEVICE_REG_DELETED...\n");
                }
                case CMBS_EV_DSR_SYS_START_RES:
                {
                    tcm_han_mgr_start();

                    tcm_HanRegisterInterface();

                    dect_device_data_init();

                    dect_device_register_complete(TCM_DEVICE_REGISTER_UNKNOW, dect_convert_cmbs_id_to_dect(g_dect_register_device_id, 0));
                    
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "CMBS_EV_DSR_SYS_START_RES...\n");
                    break;
                }
                default:
                {
                    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "Message [%d] is not handled, yet...\n", LinuxContainer.Content.n_Event);
                    break;
                }
            }
        }


    }
    return NULL;
}

/*pthread_t g_hanDeviceReportThreadId;
int tcm_createDeviceReportThread(void)
{
    int ret;
    pthread_attr_t  attr;
//   pthread_t id;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024);
//    ret = pthread_create(&id, &attr, tcm_DeviceReportThread, NULL);
    ret = pthread_create(&g_hanDeviceReportThreadId, &attr, tcm_DeviceReportThread, NULL);
    pthread_attr_destroy(&attr);

    return ret;
}*/


