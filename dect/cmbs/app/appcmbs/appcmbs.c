/*!
    \brief Initialize the CMBS application

*/


#include <stdio.h>
#if ! defined ( WIN32 )
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h> //we need <sys/select.h>; should be included in <sys/types.h> ???
#include <signal.h>
#include <sys/msg.h>
#include <errno.h>
#endif
#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cmbs_han.h"
#include "cmbs_int.h"
#include "cmbs_dbg.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "applog.h"
#include "appmsgparser.h"

// re-used from the CMBS API Module.
extern void             _cmbs_int_MsgQDestroy(int nMsgQId);
extern int              _cmbs_int_MsgQCreate(void);
extern u8               u8_APP_LA_Initialized;
extern void             app_LaInitialize(void);
extern u8     tcx_DetectComPort(bool interactiveMode, E_CMBS_DEVTYPE *pu8_type);
extern void     tcx_USBConfig(u8 u8_Port);
extern ST_CMBS_DEV   g_st_DevMedia;
extern ST_CMBS_DEV       g_st_DevCtl;
ST_CB_LOG_BUFFER    pfn_log_buffer_Cb;

void     han_applUleMsgFiFoInitialize();

/*!***************************************************************************
*
* \brief CMBS application feedback entity
*
*****************************************************************************/

// different CMBS parts use different entities
// which will be called by callback function
// if the entity consumes the event, it shall return TRUE
typedef  int (*PFN_APP_ENTITY_CB)(void *pv_AppRef,
                                  E_CMBS_EVENT_ID e_EventID,
                                  void *pv_EventIEListRef);

// global callback function table of received CMBS Events
PFN_APP_ENTITY_CB     g_pfn_app_Entity[] =
{
    (PFN_APP_ENTITY_CB)app_ServiceEntity,
    (PFN_APP_ENTITY_CB)app_CallEntity,
    (PFN_APP_ENTITY_CB)app_SwupEntity,
    NULL, //(PFN_APP_ENTITY_CB)app_MediaEntity - currently inside call entity
    (PFN_APP_ENTITY_CB)app_FacilityEntity,
    (PFN_APP_ENTITY_CB)app_DataEntity,
    (PFN_APP_ENTITY_CB)app_LaEntity,
#ifndef WIN32
    (PFN_APP_ENTITY_CB)app_SuotaEntity,
#endif
    (PFN_APP_ENTITY_CB)app_HANEntity,
    (PFN_APP_ENTITY_CB)app_RTPEntity,
    (PFN_APP_ENTITY_CB)app_LogEntity,
    (PFN_APP_ENTITY_CB)app_CmdEntity
};
// global object of application
ST_CMBS_APPL    g_cmbsappl;

u8                g_HoldCfm = 1;       /* 0 - decline hold, 1 - accept hold */
u8                g_HoldResumeCfm = 1; /* 0 - decline hold resume, 1 - accept hold resume */
u8 g_TransferAutoCfm = 1;
u8 g_ConfAutoCfm = 1;
u8              g_EarlyMediaAutoCfm = 1;

//  ========== appcmbs_CallBack ===========
/*!
        \brief    registered callback function to CMBS API for received events
        \param[in]   pv_AppRef   application reference pointer
        \param[in]   e_EventID   received event ID
        \param[in]   pv_EventData  IE list pointer
        \return    <none>
*/
void     appcmbs_CallBack(void *pv_AppRef, E_CMBS_EVENT_ID e_EventID, void *pv_EventData)
{
    unsigned int i;

    if (pv_AppRef)
    {} // Just one callback function registered, so we don't have to evaluate pv_AppRef

    for (i = 0; i < (sizeof(g_pfn_app_Entity) / sizeof(PFN_APP_ENTITY_CB)); i++)
    {
        if (g_pfn_app_Entity[i])
        {
            if (g_pfn_app_Entity[i](pv_AppRef, e_EventID, pv_EventData))
            {
                // no handler is needed anymore, event is consumed
                return;
            }
        }
    }
    APPCMBS_WARN(("Warning: Event: %d %s is not handled, yet\n", e_EventID, cmbs_dbg_GetEventName(e_EventID)));
}

void  appcmbs_InitEeprom(u8 *pu8_EEprom, u16 u16_Size)
{
    UNUSED_PARAMETER(pu8_EEprom);
    UNUSED_PARAMETER(u16_Size);
    /*
      u32 u32_offset = 0;
      u32 u32_buf_size;
      ST_APPCMBS_CONTAINER st_Container;

      APPCMBS_INFO(( "APP: INFO Push eeprom settings to target!\n"));

    //  Sleep(1000);

      while (u16_Size)
      {
        if (u16_Size > CMBS_PARAM_AREA_MAX_SIZE)
        {
          u32_buf_size = CMBS_PARAM_AREA_MAX_SIZE;
        }
        else
      {
          u32_buf_size = u16_Size;
      }

        app_SrvParamAreaSet( CMBS_PARAM_AREA_TYPE_EEPROM, u32_offset, u32_buf_size, pu8_EEprom, 1 );
        appcmbs_WaitForContainer( CMBS_EV_DSR_PARAM_AREA_SET_RES, &st_Container );

        pu8_EEprom += u32_buf_size;
        u32_offset += u32_buf_size;
        u16_Size -= u32_buf_size;
      }
    */
}

int appcmbs_CordlessStart(ST_IE_SYPO_SPECIFICATION *SYPOParameters)
{

    // Start DECT module
    cmbs_dsr_sys_Start(g_cmbsappl.pv_CMBSRef, SYPOParameters);

    return appcmbs_WaitForEvent(CMBS_EV_DSR_SYS_START_RES);
}

//  ==========  appcmbs_Initialize ===========
/*!
        \brief     short description
        \param[in,out]   pv_AppReference  application pointer reference
        \return    <int>       return zero if successful
*/
E_CMBS_RC appcmbs_Initialize(void *pv_AppReference, PST_CMBS_DEV pst_DevCtl, PST_CMBS_DEV pst_DevMedia, ST_CB_LOG_BUFFER pfn_log_buffer_Cb)
{
    E_CMBS_RC e_RC = CMBS_RC_OK;
    // reset application
    memset(&g_cmbsappl, 0, sizeof(g_cmbsappl));
    g_cmbsappl.pv_ApplRef = pv_AppReference;

    // initialize application
    appcall_Initialize();
    // intercommunication between engine and application
#ifdef __linux__
    g_cmbsappl.n_MssgAppID = _cmbs_int_MsgQCreate();
    g_cmbsappl.n_MssgReport = _cmbs_int_MsgQCreate();
#elif WIN32
    // initialize without any timeout
    g_cmbsappl.pst_AppSyncEntry  = cfr_MQueueCreate((u32)GetCurrentThreadId(), 0);
#endif
    // initialize CMBS API and register callback function
    // for test application
    if ((e_RC = cmbs_api_Init(CMBS_MODE_MLB, pst_DevCtl, pst_DevMedia)) == CMBS_RC_OK)
    {
        APPCMBS_INFO(("APP: INFO CMBS-API started with Module version %04x\n", cmbs_api_ModuleVersionGet()));

        if ((g_cmbsappl.pv_CMBSRef = cmbs_api_RegisterCb(&g_cmbsappl, (PFN_CMBS_API_CB)appcmbs_CallBack, 0x0100)) == NULL)
        {
            APPCMBS_WARN(("APP: WARN Can't register API callback function\n"));
        }
    }
    else
    {
        APPCMBS_ERROR(("APP: ERROR !!! Initialization failed\n"));

        return e_RC;
    }

    cmbs_int_RegisterLogBufferCb(&g_cmbsappl, &pfn_log_buffer_Cb);

    cmbs_dbg_SetParseIEFunc(app_PrintIe2Log);

    if (!u8_APP_LA_Initialized)
    {
        app_LaInitialize();
        u8_APP_LA_Initialized = 1;
    }

    han_applUleMsgFiFoInitialize();
    return e_RC;
}
//  ========== appcmbs_Cleanup ===========
/*!
        \brief   clean up CMBS API layer and CMBS API

        \param[in]  <none>
        \return   <none>

*/

void     appcmbs_Cleanup(void)
{
    APPCMBS_INFO(("APP: INFO Cleanup CMBS connectivity\n"));
    cmbs_api_UnInit();
#ifdef __linux
    _cmbs_int_MsgQDestroy(g_cmbsappl.n_MssgAppID);
    _cmbs_int_MsgQDestroy(g_cmbsappl.n_MssgReport);
#elif WIN32
    cfr_MQueueDestroy(g_cmbsappl.pst_AppSyncEntry);
#endif
}
//  ========== app_ResponseCheck ===========
/*!
        \brief  check the IE response item and return response code
        \param[in]  pv_List IE list pointer
        \return  <s8>  return response code TRUE/FALSE or -1 in case not finding IE
*/
s8       app_ResponseCheck(void *pv_List)
{
    void *pv_IE;
    u16      u16_IE;

    cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_RESPONSE)
        {
            ST_IE_RESPONSE st_Response;

            // check response code:
            cmbs_api_ie_ResponseGet(pv_IE, &st_Response);

            return st_Response.e_Response;
        }
        cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
    }

    return -1;
}
//  ========== appcmbs_PrepareRecvAdd  ===========
/*!
        \brief  set token to pass received infromation to upper layer
        \param[in]  u32_Token   TRUE for passing
        \return    <none>

*/

void     appcmbs_PrepareRecvAdd(u32 u32_Token)
{
    g_cmbsappl.n_Token = (int)u32_Token;
}

int appcmbs_CleanMessage(void)
{

    int   nRetVal;
    int   bo_Run = TRUE;
    ST_APPCMBS_LINUX_CONTAINER LinuxContainer;
    int i = 0;

    while (bo_Run)
    {
        nRetVal = msgrcv(g_cmbsappl.n_MssgAppID, &LinuxContainer, sizeof(ST_APPCMBS_CONTAINER), 0, IPC_NOWAIT);
        if (nRetVal == -1 && errno == ENOMSG)
        {
            break;
        }
        else
        {
            continue;
        }

        // timeout handler


    }
    return TRUE;
}

//  ========== appcmbs_WaitForContainer ===========
/*!
        \brief   Synchronization function enables the application to wait until
                        requested CMBS event was received.
        \param[in]  n_Event          wait for this CMBS event
        \param[in,out]  pst_Container  pointer to CMBS information contatiner
        \return   <int>            return TRUE, if received, otherwise FALSE on error
      \todo           timeout handling has to be integrated
*/
int appcmbs_WaitForContainer(int n_Event, PST_APPCMBS_CONTAINER pst_Container)
{

    int   nRetVal;
    int   bo_Run = TRUE;
    ST_APPCMBS_LINUX_CONTAINER LinuxContainer;
    int i = 0;

    while (bo_Run)
    {
        nRetVal = msgrcv(g_cmbsappl.n_MssgAppID, &LinuxContainer, sizeof(ST_APPCMBS_CONTAINER), 0, IPC_NOWAIT);
        if (nRetVal == -1 && errno == ENOMSG)
        {
            printf("no message\n");
            //usleep(1000);
            // sleep(1);
            SleepMs(100);
            i++;
            if (i >= 100)
            {
                printf("i = %d second, timeout\n", i / 10);
                break;
            }
        }
        else if (n_Event == LinuxContainer.Content.n_Event)
        {
            if (pst_Container)
                memcpy(pst_Container, &LinuxContainer.Content, sizeof(ST_APPCMBS_CONTAINER));
            g_cmbsappl.n_Token = 0;
            return TRUE;
        }
        else
        {
            continue;
        }

        // timeout handler


    }

    g_cmbsappl.n_Token = 0;
    return FALSE;
}
#if 0
int      appcmbs_WaitForContainer(int n_Event, PST_APPCMBS_CONTAINER pst_Container)
{
#ifdef __linux__
    int   nRetVal;
    int   bo_Run = TRUE;
    ST_APPCMBS_LINUX_CONTAINER LinuxContainer;

    while (bo_Run)
    {
        nRetVal = msgrcv(g_cmbsappl.n_MssgAppID, &LinuxContainer, sizeof(ST_APPCMBS_CONTAINER), 0, 0);
        if (nRetVal == -1)
            bo_Run = FALSE;

        if (n_Event == LinuxContainer.Content.n_Event)
        {
            if (pst_Container)
                memcpy(pst_Container, &LinuxContainer.Content, sizeof(ST_APPCMBS_CONTAINER));
            g_cmbsappl.n_Token = 0;
            return TRUE;
        }

        // timeout handler
    }

    g_cmbsappl.n_Token = 0;


#elif WIN32
    u32  u32_MssgID;
    u16  u16_ParamSize;
    void *pv_Param;
    int   bo_Run = TRUE;

    while (bo_Run)
    {
        if (E_CFR_INTERPROCESS_MSSG  == cfr_MQueueGet(g_cmbsappl.pst_AppSyncEntry, &u32_MssgID, &pv_Param, &u16_ParamSize))
        {
            if (u32_MssgID == CFR_CMBS_MSSG_APPSYNC)
            {
                PST_APPCMBS_LINUX_CONTAINER pstLinuxContainer = (PST_APPCMBS_LINUX_CONTAINER)pv_Param;

                if (n_Event == pstLinuxContainer->Content.n_Event)
                {
                    if (pst_Container)
                        memcpy(pst_Container, &pstLinuxContainer->Content, sizeof(ST_APPCMBS_CONTAINER));
                    g_cmbsappl.n_Token = 0;

                    cfr_MQueueMssgFree(g_cmbsappl.pst_AppSyncEntry, pv_Param);
                    return TRUE;
                }
                else
                {
                    cfr_MQueueMssgFree(g_cmbsappl.pst_AppSyncEntry, pv_Param);
                }
            }
            else
            {
                // later check also for destroy message
                cfr_MQueueMssgFree(g_cmbsappl.pst_AppSyncEntry, pv_Param);
                return TRUE;
            }
        }
        else
        {
            bo_Run = FALSE;
        }
    }
#endif // __linux__
    return FALSE;
}
#endif
int appcmbs_WaitForEvent(int n_Event)
{
    return appcmbs_WaitForContainer(n_Event, NULL);
}

void     appcmbs_ObjectReport(char *psz_Info, int n_InfoLen, int n_Info, int n_Event)
{

    int   nRetVal;

    if (g_cmbsappl.n_MssgReport >= 0)
    {
        ST_APPCMBS_LINUX_CONTAINER LinuxContainer;

        LinuxContainer.mType = 1;
        memcpy(LinuxContainer.Content.ch_Info, psz_Info, n_InfoLen);
        LinuxContainer.Content.n_InfoLen = n_InfoLen;
        LinuxContainer.Content.n_Event = n_Event;
        LinuxContainer.Content.n_Info = n_Info;

        nRetVal = msgsnd(g_cmbsappl.n_MssgReport, &LinuxContainer, sizeof(ST_APPCMBS_CONTAINER), 0);

        if (nRetVal == -1)
        {
            APPCMBS_ERROR(("App:ERROR !!! Container Object was not sent! %d\n", errno));
        }
    }

}


//  ========== appcmbs_ObjectSignal ===========
/*!
        \brief          signal to application of received event
        \param[in]   psz_Info      pointer to information buffer
        \param[in]   n_InfoLen  length of information
        \param[in]   n_Info     additional information, e.g. IE element
        \param[in]   n_Event     received CMBS event
        \return   <none>
*/
void     appcmbs_ObjectSignal(char *psz_Info, int n_InfoLen, int n_Info, int n_Event)
{
#ifdef __linux__
    int   nRetVal;

    if (g_cmbsappl.n_MssgAppID >= 0)
    {
        ST_APPCMBS_LINUX_CONTAINER LinuxContainer;

        LinuxContainer.mType = 1;
        memcpy(LinuxContainer.Content.ch_Info, psz_Info, n_InfoLen);
        LinuxContainer.Content.n_InfoLen = n_InfoLen;
        LinuxContainer.Content.n_Event = n_Event;
        LinuxContainer.Content.n_Info = n_Info;

        nRetVal = msgsnd(g_cmbsappl.n_MssgAppID, &LinuxContainer, sizeof(ST_APPCMBS_CONTAINER), 0);

        if (nRetVal == -1)
        {
            APPCMBS_ERROR(("App:ERROR !!! Container Object was not sent! %d\n", errno));
        }
    }
#elif WIN32
    if (g_cmbsappl.pst_AppSyncEntry)
    {
        PST_APPCMBS_LINUX_CONTAINER pst_Container;

        pst_Container = (PST_APPCMBS_LINUX_CONTAINER)malloc(sizeof(ST_APPCMBS_LINUX_CONTAINER));

        if (pst_Container)
        {
            memset(pst_Container, 0, sizeof(ST_APPCMBS_LINUX_CONTAINER));

            if (psz_Info)
            {
                memcpy(pst_Container->Content.ch_Info, psz_Info, n_InfoLen);
            }

            pst_Container->mType = 1;
            pst_Container->Content.n_InfoLen = n_InfoLen;
            pst_Container->Content.n_Event = n_Event;
            pst_Container->Content.n_Info = n_Info;

            cfr_MQueueSend(g_cmbsappl.pst_AppSyncEntry, CFR_CMBS_MSSG_APPSYNC, (void *)pst_Container, sizeof(ST_APPCMBS_LINUX_CONTAINER));
        }
        free(pst_Container);
    }

#endif // __linux__
}
//  ========== appcmbs_IEInfoGet  ===========
/*!
        \brief    handles general to IE function
        \param[in]     pv_IE   IE  buffer
        \param[in]     u16_IE  enumeration of IE
        \param[in,out]  p_Info  pointer to IE object
        \return    <none>
*/
void     appcmbs_IEInfoGet(void *pv_IE, u16 u16_IE, PST_APPCMBS_IEINFO p_Info)
{
    switch (u16_IE)
    {
        case  CMBS_IE_CALLERNAME:
            cmbs_api_ie_CallerNameGet(pv_IE, &p_Info->Info.st_CallerName);
            break;

        case  CMBS_IE_CALLERPARTY:
            cmbs_api_ie_CallerPartyGet(pv_IE, &p_Info->Info.st_CallerParty);
            break;
        case  CMBS_IE_CALLEDPARTY:
            cmbs_api_ie_CalledPartyGet(pv_IE, &p_Info->Info.st_CalledParty);
            break;

        case  CMBS_IE_CALLINSTANCE:
            cmbs_api_ie_CallInstanceGet(pv_IE, &p_Info->Info.u32_CallInstance);
            break;

        case  CMBS_IE_CALLPROGRESS:
            cmbs_api_ie_CallProgressGet(pv_IE, &p_Info->Info.st_CallProgress);
            break;

        case  CMBS_IE_CALLINFO:
            cmbs_api_ie_CallInfoGet(pv_IE, &p_Info->Info.st_CallInfo);
            break;

        case  CMBS_IE_MEDIACHANNEL:
            cmbs_api_ie_MediaChannelGet(pv_IE, &p_Info->Info.st_MediaChannel);
            break;

        case  CMBS_IE_MEDIADESCRIPTOR:
            cmbs_api_ie_MediaDescGet(pv_IE, &p_Info->Info.st_MediaDesc);
            break;

        case  CMBS_IE_CALLRELEASE_REASON:
            cmbs_api_ie_CallReleaseReasonGet(pv_IE, &p_Info->Info.st_Reason);
            break;

        case  CMBS_IE_PARAMETER:
            cmbs_api_ie_ParameterGet(pv_IE, &p_Info->Info.st_Param);
            break;

        case  CMBS_IE_FW_VERSION:
            cmbs_api_ie_FwVersionGet(pv_IE, &p_Info->Info.st_FwVersion);
            break;

        case  CMBS_IE_HW_VERSION:
            cmbs_api_ie_HwVersionGet(pv_IE, &p_Info->Info.st_HwVersion);
            break;
        case CMBS_IE_EEPROM_VERSION:
            cmbs_api_ie_EEPROMVersionGet(pv_IE, &p_Info->Info.st_EEPROMVersion);
            break;
        case  CMBS_IE_SYS_LOG:
            cmbs_api_ie_SysLogGet(pv_IE, &p_Info->Info.st_SysLog);
            break;

        case  CMBS_IE_SUBSCRIBED_HS_LIST:
            cmbs_api_ie_SubscribedHSListGet(pv_IE, &p_Info->Info.st_SubscribedHsList);
            break;

        case CMBS_IE_LINE_SETTINGS_LIST:
            cmbs_api_ie_LineSettingsListGet(pv_IE, &p_Info->Info.st_LineSettingsList);
            break;

        case  CMBS_IE_RESPONSE:
            cmbs_api_ie_ResponseGet(pv_IE, &p_Info->Info.st_Resp);
            break;

        case  CMBS_IE_LINE_ID:
            cmbs_api_ie_LineIdGet(pv_IE, &p_Info->Info.u8_LineId);
            break;

        case  CMBS_IE_MELODY:
            cmbs_api_ie_MelodyGet(pv_IE, &p_Info->Info.u8_Melody);
            break;

        case  CMBS_IE_DECT_SETTINGS_LIST:
            cmbs_api_ie_DectSettingsListGet(pv_IE, &p_Info->Info.st_DectSettings);
            break;

        default:
            if (CMBS_IE_USER_DEFINED_START <= u16_IE &&  u16_IE <= CMBS_IE_USER_DEFINED_END)
            {
                break;
            }
            APPCMBS_WARN(("APP: WARN IE_ToString: IE:%d not implemented\n", u16_IE));
    }

}

void     appcmbs_VersionGet(char *pc_Version)
{
    u16 u16_Version = cmbs_api_ModuleVersionGet();
    u16 u16_Build   = cmbs_int_ModuleVersionBuildGet();

    if (u16_Version > 0xFFF)
        sprintf(pc_Version, "%02x.%03x - Build %x", (u16_Version >> 12), (u16_Version & 0xFFF), u16_Build);
    else
        sprintf(pc_Version, "%02x.%02x - Build %x", (u16_Version >> 8), (u16_Version & 0xFF), u16_Build);
}

unsigned long   appcmbs_GetTickCount(void)
{
#ifdef WIN32
    return GetTickCount();
#else //__linux__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((unsigned long)tv.tv_sec * 1000);
#endif
}

int appcmbs_ReconnectApplication(unsigned long ulTimeoutMs)
{
    u32 u32_Sec, i;
    int resp;

    cmbs_api_UnInit();

    if (ulTimeoutMs)
    {
        u32_Sec = ulTimeoutMs / 1000;

        for (i = 0; i <= u32_Sec; ++i)
        {
            APPCMBS_INFO(("\r%d sec. remaining...", u32_Sec - i));
            SleepMs(1000);
        }
    }

    APPCMBS_INFO(("\n"));

    // restore USB port (windows only), it could be changed during target init (different RFPI or reinit EEPROM)
#if defined ( WIN32 )
    if (g_st_DevCtl.e_DevType == CMBS_DEVTYPE_USB)
    {
        u8    u8_Port;
        E_CMBS_DEVTYPE  e_type;

        u8_Port = tcx_DetectComPort(FALSE, &e_type); /* non interactive mode */
        if (u8_Port)
        {
            tcx_USBConfig(u8_Port);
        }
    }
#endif

    resp = cmbs_api_Init(CMBS_MODE_MLB, &g_st_DevCtl, &g_st_DevMedia);
    cmbs_api_RegisterCb(&g_cmbsappl, (PFN_CMBS_API_CB)appcmbs_CallBack, 0x0100);
    cmbs_int_RegisterLogBufferCb(&g_cmbsappl, &pfn_log_buffer_Cb);
    cmbs_dbg_SetParseIEFunc(app_PrintIe2Log);

    if (resp == CMBS_RC_OK)
    {
        cmbs_dsr_sys_Start(g_cmbsappl.pv_CMBSRef, NULL);
    }
    else
    {
        APPCMBS_ERROR(("appcmbs_Initialize failure %d!!!", resp));
    }
    return resp;
}

int app_CmdEntity(void *pv_AppRef, E_CMBS_EVENT_ID e_EventID, void *pv_EventData)
{
    UNUSED_PARAMETER(pv_AppRef);
    UNUSED_PARAMETER(pv_EventData);
    // return TRUE for all commands
    return (e_EventID & CMBS_CMD_MASK) == CMBS_CMD_MASK;
}

//*/
