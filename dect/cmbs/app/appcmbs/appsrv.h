/*!
* \file
* \brief
* \Author  kelbch
*
* @(#) %filespec: appsrv.h~DMZD53#9.1.13 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
* 26-Oct-2001 tcmc_asa NBGD53#9.1.5 added app_SrvFixedCarrierSet
* 26-Oct-2001 tcmc_asa 9.1.6        merged 2 versions 9.1.5
*******************************************************************************/

#if !defined( APPSRV_H )
#define APPSRV_H


#if defined( __cplusplus )
extern "C"
{
#endif

void   appsrv_Initialize(void);

E_CMBS_RC      app_SrvHandsetDelete(char * psz_Handsets);
E_CMBS_RC      app_SrvHandsetPage(char * psz_Handsets);
E_CMBS_RC      app_SrvHandsetStopPaging(void);
E_CMBS_RC      app_SrvSubscriptionOpen(u32 u32_timeout);
E_CMBS_RC      app_SrvSubscriptionClose(void);
E_CMBS_RC      app_SrvEncryptionDisable(void);
E_CMBS_RC      app_SrvEncryptionEnable(void);
E_CMBS_RC      app_SrvFixedCarrierSet(u8 u8_Value);

//E_CMBS_RC      app_SrvPINCodeGet( u32 u32_Token );
E_CMBS_RC      app_SrvPINCodeSet(char * psz_PIN);
//E_CMBS_RC      app_SrvRFPIGet( u32 u32_Token );
E_CMBS_RC      app_SrvTestModeGet(u32 u32_Token);
E_CMBS_RC      app_SrvTestModeSet(void);
E_CMBS_RC      app_SrvFWVersionGet(E_CMBS_MODULE e_Module, u32 u32_Token);
E_CMBS_RC      app_SrvEEPROMVersionGet(u32 u32_Token);
E_CMBS_RC      app_SrvHWVersionGet(u32 u32_Token);
E_CMBS_RC      app_SrvLogBufferStart(void);
E_CMBS_RC      app_SrvLogBufferStop(void);
E_CMBS_RC      app_SrvLogBufferRead(u32 u32_Token);
E_CMBS_RC      app_SrvSystemReboot(void);
E_CMBS_RC      app_SrvSystemPowerOff(void);
E_CMBS_RC      app_SrvRegisteredHandsets(u16 u16_HsMask, u32 u32_Token);
E_CMBS_RC      app_SrvSetNewHandsetName(u16 u16_HsId, u8* pu8_HsName, u16 u16_HsNameSize, u32 u32_Token);
E_CMBS_RC      app_SrvLineSettingsGet(u16 u16_LinesMask, u32 u32_Token);
E_CMBS_RC      app_SrvLineSettingsSet(ST_IE_LINE_SETTINGS_LIST* pst_LineSettingsList, u32 u32_Token);
E_CMBS_RC      app_SrvRFSuspend(void);
E_CMBS_RC      app_SrvRFResume(void);
E_CMBS_RC      app_SrvTurnOnNEMo(void);
E_CMBS_RC      app_SrvTurnOffNEMo(void);
E_CMBS_RC      app_SrvParamGet(E_CMBS_PARAM e_Param, u32 u32_Token);
E_CMBS_RC      app_SrvParamSet(E_CMBS_PARAM e_Param, u8* pu8_Data, u16 u16_Length, u32 u32_Token);
E_CMBS_RC      app_ProductionParamGet(E_CMBS_PARAM e_Param, u32 u32_Token);
E_CMBS_RC      app_ProductionParamSet(E_CMBS_PARAM e_Param, u8* pu8_Data, u16 u16_Length, u32 u32_Token);
E_CMBS_RC      app_SrvParamAreaGet(E_CMBS_PARAM_AREA_TYPE e_AreaType, u32 u32_Pos, u16 u16_Length, u32 u32_Token);
E_CMBS_RC      app_SrvParamAreaSet(E_CMBS_PARAM_AREA_TYPE e_AreaType, u32 u32_Pos, u16 u16_Length, u8* pu8_Data, u32 u32_Token);
E_CMBS_RC      app_OnHandsetLinkRelease(void * pv_List);
E_CMBS_RC      app_SrvDectSettingsGet(u32 u32_Token);
E_CMBS_RC      app_SrvDectSettingsSet(ST_IE_DECT_SETTINGS_LIST* pst_DectSettingsList, u32 u32_Token);
E_CMBS_RC      app_SrvAddNewExtension(u8* pu8_Name, u16 u16_NameSize, u8* pu8_Number, u8 u8_NumberSize, u32 u32_Token);
E_CMBS_RC      app_SrvGetBaseName(u32 u32_Token);
E_CMBS_RC      app_SrvSetBaseName(u8* pu8_BaseName, u8 u8_BaseNameSize, u32 u32_Token);
E_CMBS_RC    app_SrvGetEepromSize(void);
int            app_ServiceEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData);
bool      app_isHsRegistered(u8 u8_HsId);
void      appsrv_AFEOpenAudioChannel(void);
void      appsrv_AFECloseAudioChannel(u32 u32_ChannelID);
void      appsrv_AFEAllocateChannel();
void      app_AFEConnectEndpoints();
void      app_AFEEnableDisableEndpoint(ST_IE_AFE_ENDPOINT* pst_Endpoint, u16 u16_Enable);
void      appsrv_AFESetEndpointGain(ST_IE_AFE_ENDPOINT_GAIN* pst_EndpointGain, u16 u16_Input);
void      appsrv_AFEDHSGSendByte(u8 u8_Value);
void      app_SrvGPIOEnable(PST_IE_GPIO_ID st_GPIOId);
void      app_SrvGPIODisable(PST_IE_GPIO_ID st_GPIOId);
void           app_SrvEEPROMBackupRestore();
void           app_SrvEEPROMBackupCreate();

typedef struct
{
    E_CMBS_AUDIO_CODEC e_Codec;
    ST_IE_AFE_ENDPOINTS_CONNECT st_AFEEndpoints;
    u32       u32_SlotMask;
    u32       u32_ChannelID;
    u8       u8_InstanceNum;
    u8       u8_Resource;

} st_AFEConfiguration;

typedef struct
{
    E_CMBS_IE_TYPE e_IE;          /*! IE type */
    u8         u8_Value;      /*! IE value*/
} ST_GPIO_Properties, * PST_GPIO_Properties;

void     app_SrvGPIOSet(PST_IE_GPIO_ID st_GPIOId, PST_GPIO_Properties pst_GPIOProp);
void     app_SrvGPIOGet(PST_IE_GPIO_ID st_GPIOId, PST_GPIO_Properties pst_GPIOProp);
void     app_SrvExtIntConfigure(PST_IE_GPIO_ID st_GpioId, PST_IE_INT_CONFIGURATION st_Config, u8 u8_IntNumber);
void     app_SrvExtIntEnable(u8 u8_IntNumber);
void     app_SrvExtIntDisable(u8 u8_IntNumber);
E_CMBS_RC     app_SrvLocateSuggest(u16 u16_Handsets);
#if defined( __cplusplus )
}
#endif

#define  CMBS_ALL_HS_MASK   0x03FF // Maximal HS mask, the maximal mask of all possible configurations (e.g. NBS)
#define  CMBS_ALL_RELEVANT_HS_ID    0xFFFF // "ID" for all handsets.

#endif //APPSRV _H
//*/
