/*!
* \file  appservice.c
* \brief  handles the service relevant interfaces
* \Author  kelbch
*
* @(#) %filespec: appsrv.c~31.1.16.1.2 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* 13-Jan-2014 tcmc_asa  -- GIT--  take checksum changes from 3.46.x to 3_x main (3.5x)
* 20-Dec-2013 tcmc_asa  --GIT--      added app_OnCheckSumFailure
* 30-Nov-2012 tcmc_asa 31.1.16.1.3   removed app_SrvSendCurrentDateAndTime
*                                      in app_OnHandsetRegistered
* 26-Oct-2011 tcmc_asa 31.1.8        merged 2 versions 31.1.7
* 26-Oct-2011 tcmc_asa NBGD53#31.1.7 added app_SrvFixedCarrierSet
*  14-Dec-09        sergiym     ?        Add start/stop log commands
*                   Kelbch      1        Initialize
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if ! defined ( WIN32 )
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h> //we need <sys/select.h>; should be included in <sys/types.h> ???
#include <signal.h>
#include <pthread.h>
#endif

#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "cmbs_dbg.h"
#include "cmbs_int.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "tcx_eep.h"
#include "ListsApp.h"
#include "ListChangeNotif.h"
#include "appmsgparser.h"
#include "cmbs_event.h"
#ifdef CMBS_COMA
#include "cfr_coma.h"
#endif
#include "cmbs_han_ie.h"
unsigned short g_reg_hs;

/* Forward Declaration */
extern void app_OnHsLocProgress(void *pvAppRefHandle, void *pv_List);
extern void app_OnHsLocAnswer(void *pvAppRefHandle, void *pv_List);
extern void app_OnHsLocRelease(void *pvAppRefHandle, void *pv_List);
extern E_CMBS_RC cmbs_api_ie_HsNumberGet(void *pv_RefIE, u8 *pu8_HsNumber);
void        app_onDisconnectReq(void);
void        app_onTargetUp(void);
void        app_OnEepromSizeResp(void *pv_List);
extern ST_CMBS_DEV       g_st_DevMedia;
extern ST_CMBS_DEV       g_st_DevCtl;
void  appOnTerminalCapabilitiesInd(void *pv_List);
void  appOnPropritaryDataRcvInd(void *pv_List);

static E_CMBS_RC app_SrvSendCurrentDateAndTime(u8 u8_HsNum);
char           _app_HexString2Byte(char *psz_HexString);

#ifdef CMBS_COMA
static void app_CSSCOMAReset(void);
#endif // CMBS_COMA
//  ========== app_ASC2HEX  ===========
/*!
        \brief     convert a two digits asc ii string to a hex value

        \param[in,out]   pu8_Digits   pointer to two digits string

        \return    u8              return value of convertion

*/

u8             app_ASC2HEX(char *psz_Digits)
{
    u8 u8_Value = 0;
    int i = 0, j = 1;
    if (strlen(psz_Digits) < 2)
        j = 0;

    for (i = 0; (i < 2) && (j >= 0); i++)
    {
        if (psz_Digits[i] >= '0' && psz_Digits[i] <= '9')
        {
            u8_Value |= (psz_Digits[i] - '0') << 4 * j;
        }
        else if (psz_Digits[i] >= 'a' && psz_Digits[i] <= 'f')
        {
            u8_Value |= (0x0a + (psz_Digits[i] - 'a')) << 4 * j;
        }
        else if (psz_Digits[i] >= 'A' && psz_Digits[i] <= 'F')
        {
            u8_Value |= (0x0a + (psz_Digits[i] - 'A')) << 4 * j;
        }
        j--;
    }

    return u8_Value;
}

//  ========== app_OnPageStarted ===========
/*!
        \brief   Paging started
        \param[in,out]   *pv_List   IE list
        \return   <none>
*/
void app_OnPageStarted(void *pv_List)
{
    UNUSED_PARAMETER(pv_List);

    cmbsevent_OnPageStarted();
}

//  ========== app_OnPageStoped ===========
/*!
        \brief   Paging stopped
        \param[in,out]   *pv_List   IE list
        \return   <none>
*/
void app_OnPageStoped(void *pv_List)
{
    UNUSED_PARAMETER(pv_List);

    cmbsevent_OnPageStoped();
}

//  ========== app_OnHandsetRegistered ===========
/*!
        \brief   a new handset is registered OR a handset was deleted.
        \param[in,out]   *pv_List   IE list
        \return   <none>
*/
void app_OnHandsetRegistered(void *pv_List)
{
    ST_IE_HANDSETINFO st_HsInfo;
    ST_IE_RESPONSE st_Res;
    void *pv_IE = NULL;
    u16      u16_IE;

    memset(&st_HsInfo, 0, sizeof(ST_IE_HANDSETINFO));

    cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

    cmbs_api_ie_HandsetInfoGet(pv_IE, &st_HsInfo);
    cmbs_api_ie_ResponseGet(pv_IE, &st_Res);

    if (st_Res.e_Response == CMBS_RESPONSE_OK)
    {
        if (st_HsInfo.u8_State == CMBS_HS_REG_STATE_REGISTERED)
        {
            APPCMBS_INFO(("APPSRV-INFO: Handset:%d IPUI:%02X%02X%02X%02X%02X Type:%s registered\n",
                          st_HsInfo.u8_Hs,
                          st_HsInfo.u8_IPEI[0], st_HsInfo.u8_IPEI[1], st_HsInfo.u8_IPEI[2],
                          st_HsInfo.u8_IPEI[3], st_HsInfo.u8_IPEI[4],
                          cmbs_dbg_GetHsTypeName(st_HsInfo.e_Type)));

            List_AddHsToFirstLine(st_HsInfo.u8_Hs);

            /* Send Date & Time Update */
            // ASA Nov 12: Don't send FACILITY to the handset before it finished Access rights and Location registration,
            // Otherwise there is an issue with the instance use (default PMIT in MBCRouteTbl causes use
            // on new instance, although handset has already an acive link. PR ILD53#3681
            // app_SrvSendCurrentDateAndTime( st_HsInfo.u8_Hs );
        }
        else if (st_HsInfo.u8_State == CMBS_HS_REG_STATE_UNREGISTERED)
        {
            APPCMBS_INFO(("APPSRV-INFO: Handset:%d Unregistered\n", st_HsInfo.u8_Hs));

            List_RemoveHsFromAllLines(st_HsInfo.u8_Hs);
        }

        // if token is send signal to upper application
        //if (g_cmbsappl.n_Token)
        {
            printf("\nfun:%s line:%d app_OnHandsetRegistered\n", __FUNCTION__, __LINE__);
            appcmbs_ObjectReport((void *)&st_HsInfo, sizeof(st_HsInfo), CMBS_IE_HANDSETINFO, CMBS_EV_DSR_HS_REGISTERED);
        }
    }
    else
    {
        APPCMBS_ERROR(("\n[%s] ERROR!\n", __FUNCTION__));
    }

    cmbsevent_OnHandsetRegistered(st_HsInfo.u8_Hs);
}
void app_OnHandsetDeleted(void *pv_List)
{
    ST_IE_HANDSETINFO st_HsInfo;
    ST_IE_RESPONSE st_Res;
    void *pv_IE = NULL;
    u16      u16_IE;

    memset(&st_HsInfo, 0, sizeof(ST_IE_HANDSETINFO));

    cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

    cmbs_api_ie_HandsetInfoGet(pv_IE, &st_HsInfo);
    cmbs_api_ie_ResponseGet(pv_IE, &st_Res);

    if (st_Res.e_Response == CMBS_RESPONSE_OK)
    {
        // if token is send signal to upper application
        if (g_cmbsappl.n_Token)
        {
            appcmbs_ObjectSignal((void *)&st_HsInfo, sizeof(st_HsInfo), CMBS_IE_HANDSETINFO, CMBS_EV_DSR_HS_DELETE_RES);
        }
    }
    else
    {
        APPCMBS_ERROR(("\n[%s] ERROR!\n", __FUNCTION__));
    }

    cmbsevent_OnHandsetRegistered(st_HsInfo.u8_Hs);
}


//  ========== app_OnHandsetInRange ===========
/*!
        \brief    handset in range
        \param[in,out]   *pv_List   IE list
        \return   <none>
*/
void           app_OnHandsetInRange(void *pv_List)
{
    void *pv_IE = NULL;
    u16      u16_IE;
    u8       u8_Hs;
    u32      u32_temp;

    if (pv_List)
    {
        // collect information elements.
        // we expect: HS num
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        cmbs_api_ie_IntValueGet(pv_IE, &u32_temp);
        u8_Hs = (u8)u32_temp;

        APPCMBS_INFO(("APPSRV-INFO: Handset:%d in range\n", u8_Hs));

        cmbsevent_OnHandsetInRange(u8_Hs);

        /* Send Date & Time Update */
        app_SrvSendCurrentDateAndTime(u8_Hs);

        /* Send missed call notification */
        {
            u32 pu32_Lines[APPCALL_LINEOBJ_MAX], u32_NumLines = APPCALL_LINEOBJ_MAX, u32_i;
            LIST_RC list_rc;

            list_rc = List_GetLinesOfHS(u8_Hs, pu32_Lines, &u32_NumLines);
            if (list_rc != LIST_RC_OK)
            {
                printf("app_OnHandsetInRange - Failed getting list of lines... cannot send missed call notification! \n");
                return;
            }

            for (u32_i = 0; u32_i < u32_NumLines; ++u32_i)
            {
                // Send facility message only to this Hs
                ListChangeNotif_MissedCallListChanged(pu32_Lines[u32_i], FALSE, u8_Hs);
            }
        }
    }
}


//  ========== app_OnDateTimeIndication ===========
/*!
        \brief    handset set Date/Time
        \param[in,out]   *pv_List   IE list
        \return   <none>
*/
void           app_OnDateTimeIndication(void *pv_List)
{
    void *pv_IE = NULL;
    u16           u16_IE;
    u16           u16_temp;
    ST_DATE_TIME  st_DateTime;
    u8            u8_Hs;

    if (pv_List)
    {
        // collect information elements.
        // we expect: DateTime and HS num
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        cmbs_api_ie_DateTimeGet(pv_IE, &st_DateTime);
        cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        cmbs_api_ie_HandsetsGet(pv_IE, &u16_temp);
        u8_Hs = (u8)u16_temp;

        APPCMBS_INFO(("APPSRV-INFO: Handset: %d set DateTime: %02d.%02d.%02d %02d:%02d:%02d\n", u8_Hs,
                      st_DateTime.u8_Day, st_DateTime.u8_Month, st_DateTime.u8_Year,
                      st_DateTime.u8_Hours, st_DateTime.u8_Mins, st_DateTime.u8_Secs));

    }
}


//  ========== app_OnRegistrationOpenRSP ===========
/*!
        \brief   response of open registration event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnRegistrationOpenRSP(void *pv_List)
{
    int i = FALSE;

    if (!app_ResponseCheck(pv_List))
    {
        i = TRUE;

        APPCMBS_INFO(("APPSRV-INFO: Registration Open successful\n"));

        cmbsevent_OnRegistrationOpened();

        g_cmbsappl.RegistrationWindowStatus = 1;
    }
    else
    {
        APPCMBS_ERROR(("APPSRV-ERROR: !!! Registration Open failed\n"));
    }

    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal(NULL, 0, i, CMBS_EV_DSR_CORD_OPENREG_RES);
    }
}

//  ========== app_OnRegistrationClose ===========
/*!
        \brief   Notification from target on registration close and the reason for it
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnRegistrationClose(void *pv_List)
{

    void *pv_IE = NULL;
    u16           u16_IE;
    ST_IE_REG_CLOSE_REASON  st_RegCloseReason;

    if (pv_List)
    {
        // collect information elements. we expect: CMBS_IE_REG_CLOSE_REASON
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {

            if (u16_IE == CMBS_IE_REG_CLOSE_REASON)
            {
                // check registration close reason:
                cmbs_api_ie_RegCloseReasonGet(pv_IE, &st_RegCloseReason);
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
    else
    {
        APPCMBS_ERROR(("APPSRV-ERROR: !!! Registration Close failed\n"));
    }

    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal(NULL, 0, 0, CMBS_EV_DSR_CORD_CLOSEREG);
    }

    appcmbs_ObjectReport(NULL, 0, st_RegCloseReason, CMBS_EV_DSR_CORD_CLOSEREG);
    g_cmbsappl.RegistrationWindowStatus = 0;
}


//  ========== app_OnRegistrationCloseRSP ===========
/*!
        \brief   response of close registration event
        \param[in]     *pv_List   IE list
        \return   <none>
*/

void           app_OnRegistrationCloseRSP(void *pv_List)
{
    int i = FALSE;

    if (!app_ResponseCheck(pv_List))
    {
        i = TRUE;

        APPCMBS_INFO(("APPSRV-INFO: Registration Closed successfully by Host\n"));

        cmbsevent_OnRegistrationClosed();
    }
    else
    {
        APPCMBS_ERROR(("APPSRV-ERROR: !!! Registration Close failed\n"));
    }

    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal(NULL, 0, i, CMBS_EV_DSR_CORD_CLOSEREG_RES);
    }

    g_cmbsappl.RegistrationWindowStatus = 0;
}


//  ========== app_OnParamGetRSP ===========
/*!
        \brief   response of param get event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnParamGetRSP(void *pv_List)
{
    void *pv_IE = NULL;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_PARAMETER + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (CMBS_IE_PARAMETER == u16_IE)
            {
                ST_IE_PARAMETER st_Param;

                cmbs_api_ie_ParameterGet(pv_IE, &st_Param);
                // signal parameter setting to upper application
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)st_Param.pu8_Data, st_Param.u16_DataLen, st_Param.e_Param, CMBS_EV_DSR_PARAM_GET_RES);
                }
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//  ========== app_OnParamSetRSP ===========
/*!
        \brief   response of param set event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnParamSetRSP(void *pv_List)
{
    void *pv_IE;
    u16         u16_IE;
    int         i = 0;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_PARAMETER + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (u16_IE == CMBS_IE_PARAMETER)
            {
                APPCMBS_INFO(("APPSRV-INFO: Param \""));
                app_IEToString(pv_IE, u16_IE);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
        // signal to upper application only IE repsonse info
        if (app_ResponseCheck(pv_List) == CMBS_RESPONSE_OK)
        {
            i = TRUE;

            APPCMBS_INFO(("\" set successful\n"));
        }
        else
        {
            APPCMBS_ERROR(("\" set failed !!!\n"));
        }

        if (g_cmbsappl.n_Token)
        {
            appcmbs_ObjectSignal(NULL, 0, i, CMBS_EV_DSR_PARAM_SET_RES);
        }

    }

}

//  ========== app_OnParamAreaGetRSP ===========
/*!
        \brief   response of param area get event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnParamAreaGetRSP(void *pv_List)
{
    void *pv_IE = NULL;
    u16      u16_IE;
    ST_IE_PARAMETER_AREA st_ParamArea;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_PARAMETER_AREA + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (CMBS_IE_PARAMETER_AREA == u16_IE)
            {
                cmbs_api_ie_ParameterAreaGet(pv_IE, &st_ParamArea);
                // signal parameter setting to upper application
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)st_ParamArea.pu8_Data, st_ParamArea.u16_DataLen,
                                         st_ParamArea.e_AreaType, CMBS_EV_DSR_PARAM_AREA_GET_RES);
                }
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//  ========== app_OnParamAreaSetRSP ===========
/*!
        \brief   response of param area set event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnParamAreaSetRSP(void *pv_List)
{
    void *pv_IE;
    u16         u16_IE;
    int         i = 0;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (u16_IE == CMBS_IE_PARAMETER_AREA)
            {
                APPCMBS_INFO(("APPSRV-INFO: Param \""));
                app_IEToString(pv_IE, u16_IE);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
        // signal to upper application only IE repsonse info
        if (app_ResponseCheck(pv_List) == CMBS_RESPONSE_OK)
        {
            i = TRUE;

            APPCMBS_INFO(("\" set successful\n"));
        }
        else
        {
            APPCMBS_ERROR(("\" set failed !!!\n"));
        }

        if (g_cmbsappl.n_Token)
        {
            appcmbs_ObjectSignal(NULL, 0, i, CMBS_EV_DSR_PARAM_AREA_SET_RES);
        }

    }
}


//  ========== app_OnParamAreaGet ===========
/*!
        \brief   target requests data from the host EEPROM file
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnParamAreaGet(void *pv_List)
{
    ST_IE_PARAMETER_AREA
    st_ParamArea;
    void *pv_IE;
    u16            u16_IE;
    u8             u8_Buffer[CMBS_PARAM_AREA_MAX_SIZE];
    u32            u32_AreaSize = 0;
    u8             bo_OK = TRUE;

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (u16_IE == CMBS_IE_PARAMETER_AREA)
            {
                PST_CFR_IE_LIST  p_RespList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
                ST_IE_RESPONSE   st_Resp;

                cmbs_api_ie_ParameterAreaGet(pv_IE, &st_ParamArea);

                APPCMBS_INFO(("APPSRV-INFO: OnParamAreaGet: "));
                APPCMBS_IE(("Param_Area=%d, Offset=%d, Length=%d\n", st_ParamArea.e_AreaType, st_ParamArea.u32_Offset, st_ParamArea.u16_DataLen));

                if (st_ParamArea.e_AreaType == CMBS_PARAM_AREA_TYPE_EEPROM)
                {
                    u32_AreaSize = tcx_EepSize();
                }
                else
                {
                    APPCMBS_WARN(("APPSRV-WARN: Get Param area type:%d not supported\n", st_ParamArea.e_AreaType));
                    bo_OK = FALSE;
                }

                if (st_ParamArea.u32_Offset + st_ParamArea.u16_DataLen > u32_AreaSize)
                {
                    APPCMBS_ERROR(("APPSRV-ERROR: Get Param area requested size bigger than memory area size\n"));
                    bo_OK = FALSE;
                }

                if (bo_OK == FALSE)
                {
                    st_ParamArea.pu8_Data    = NULL;
                    st_ParamArea.u16_DataLen = 0;
                    st_Resp.e_Response       = CMBS_RESPONSE_ERROR;
                }
                else
                {
                    if (st_ParamArea.e_AreaType == CMBS_PARAM_AREA_TYPE_EEPROM)
                    {
                        tcx_EepRead(u8_Buffer, st_ParamArea.u32_Offset, st_ParamArea.u16_DataLen);
                        st_ParamArea.pu8_Data = u8_Buffer;
                        st_Resp.e_Response = CMBS_RESPONSE_OK;
                    }
                }

                cmbs_api_ie_ParameterAreaAdd((void *)p_RespList, &st_ParamArea);
                cmbs_api_ie_ResponseAdd((void *)p_RespList, &st_Resp);

                cmbs_int_EventSend(CMBS_EV_DSR_PARAM_AREA_GET_RES, p_RespList->pu8_Buffer, p_RespList->u16_CurSize);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}



//  ========== app_OnParamAreaSet ===========
/*!
        \brief   target saves data to the host EEPROM file
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnParamAreaSet(void *pv_List)
{
    ST_IE_PARAMETER_AREA
    st_ParamArea;
    void *pv_IE;
    u16            u16_IE;
    u32            u32_AreaSize = 0;
    u8             bo_OK = TRUE;

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            if (u16_IE == CMBS_IE_PARAMETER_AREA)
            {
                PST_CFR_IE_LIST  p_RespList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
                ST_IE_RESPONSE   st_Resp;

                APPCMBS_INFO(("APPSRV-INFO: OnParamAreaSet: "));
                app_IEToString(pv_IE, u16_IE);

                cmbs_api_ie_ParameterAreaGet(pv_IE, &st_ParamArea);

                if (st_ParamArea.e_AreaType == CMBS_PARAM_AREA_TYPE_EEPROM)
                {
                    u32_AreaSize = tcx_EepSize();
                }
                else
                {
                    APPCMBS_WARN(("APPSRV-WARN: Set Param area type:%d not supported\n", st_ParamArea.e_AreaType));
                    bo_OK = FALSE;
                }

                if (st_ParamArea.u32_Offset + st_ParamArea.u16_DataLen > u32_AreaSize)
                {
                    APPCMBS_ERROR(("APPSRV-ERROR: Set Param area requested size bigger than memory area size\n"));
                    bo_OK = FALSE;
                }

                if (bo_OK == FALSE)
                {
                    st_Resp.e_Response = CMBS_RESPONSE_ERROR;
                }
                else
                {
                    if (st_ParamArea.e_AreaType == CMBS_PARAM_AREA_TYPE_EEPROM)
                    {
                        tcx_EepWrite(st_ParamArea.pu8_Data, st_ParamArea.u32_Offset, st_ParamArea.u16_DataLen);
                        st_Resp.e_Response = CMBS_RESPONSE_OK;
                    }
                }

                cmbs_api_ie_ResponseAdd((void *)p_RespList, &st_Resp);

                cmbs_int_EventSend(CMBS_EV_DSR_PARAM_AREA_SET_RES, p_RespList->pu8_Buffer, p_RespList->u16_CurSize);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}


//  ========== app_OnFwVersionGetRSP ===========
/*!
        \brief   response of FW version get event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnFwVersionGetRSP(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_FW_VERSION + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_FW_VERSION)
            {
                printf("CMBS_TARGET Version %d %04x\n", st_IEInfo.Info.st_FwVersion.e_SwModule, st_IEInfo.Info.st_FwVersion.u16_FwVersion);

                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_FwVersion,
                                         sizeof(st_IEInfo.Info.st_FwVersion),
                                         CMBS_IE_FW_VERSION,
                                         CMBS_EV_DSR_FW_VERSION_GET_RES);
                }

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//  ========== app_OnHwVersionGetRSP ===========
/*!
        \brief   response of HW version get event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnHwVersionGetRSP(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HW_VERSION + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_HW_VERSION)
            {
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_HwVersion,
                                         sizeof(st_IEInfo.Info.st_HwVersion),
                                         CMBS_IE_HW_VERSION,
                                         CMBS_EV_DSR_HW_VERSION_GET_RES);
                }

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//  ========== app_OnEEPROMVersionGetRSP ===========
/*!
        \brief   response of EEPROM version get event
        \param[in]     *pv_List   IE list
        \return   <none>
*/

void    app_OnEEPROMVersionGetRSP(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_EEPROM_VERSION + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_EEPROM_VERSION)
            {
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_EEPROMVersion,
                                         sizeof(st_IEInfo.Info.st_EEPROMVersion),
                                         CMBS_IE_EEPROM_VERSION,
                                         CMBS_EV_DSR_EEPROM_VERSION_GET_RES);
                }

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

}

//  ========== app_OnDCSessionStart ===========
/*!
        \brief   target wants to start data call
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_OnDCSessionStart(void *pv_List)
{

    void *pv_IE = NULL;
    u16      u16_IE;
    u16   u16_Handsets = 0;
    u32   u32_CallInstance = 0;


    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HANDSETS + CMBS_IE_CALLINSTANCE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        switch (u16_IE)
        {
            case CMBS_IE_HANDSETS:
                cmbs_api_ie_ShortValueGet(pv_IE, &u16_Handsets, CMBS_IE_HANDSETS);
                break;
            case CMBS_IE_CALLINSTANCE:
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                break;
            default:
                break;
        }
    }
    cmbs_dsr_dc_SessionStartRes(g_cmbsappl.pv_CMBSRef, u16_Handsets, u32_CallInstance, TRUE);
}

//  ========== app_OnDCSessionStartRSP ===========
/*!
        \brief   response to data call session start
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_OnDCSessionStartRSP(void *pv_List)
{
    void *pv_IE = NULL;
    u16           u16_IE;
    ST_IE_RESPONSE          st_Response;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HANDSETS + CMBS_IE_CALLINSTANCE + CMBS_IE_RESPONSE + CMBS_IE_DC_REJECT_REASON
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            switch (u16_IE)
            {
                case CMBS_IE_HANDSETS:
                    break;
                case CMBS_IE_CALLINSTANCE:
                    break;
                case CMBS_IE_DC_REJECT_REASON:
                    break;
                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
                default:
                    break;
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}


//  ========== app_OnDCSessionStop ===========
/*!
        \brief   target wants to stop data call
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_OnDCSessionStop(void *pv_List)
{
    void *pv_IE = NULL;
    u16      u16_IE;
    u16   u16_Handsets = 0;
    u32   u32_CallInstance = 0;


    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HANDSETS + CMBS_IE_CALLINSTANCE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        switch (u16_IE)
        {
            case CMBS_IE_HANDSETS:
                cmbs_api_ie_ShortValueGet(pv_IE, &u16_Handsets, CMBS_IE_HANDSETS);
                break;
            case CMBS_IE_CALLINSTANCE:
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                break;
            default:
                break;
        }
    }
    cmbs_dsr_dc_SessionStopRes(g_cmbsappl.pv_CMBSRef, u16_Handsets, u32_CallInstance, TRUE);
}

//  ========== app_OnDCSessionStopRSP ===========
/*!
        \brief   response to data call session stop
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_OnDCSessionStopRSP(void *pv_List)
{
    void *pv_IE = NULL;
    u16           u16_IE;
    ST_IE_RESPONSE          st_Response;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HANDSETS + CMBS_IE_CALLINSTANCE + CMBS_IE_RESPONSE + CMBS_IE_DC_REJECT_REASON
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            switch (u16_IE)
            {
                case CMBS_IE_HANDSETS:
                    break;
                case CMBS_IE_CALLINSTANCE:
                    break;
                case CMBS_IE_DC_REJECT_REASON:
                    break;
                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
                default:
                    break;
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//  ========== app_OnDCSessionDataSend ===========
/*!
        \brief   Send data via data call
        \param[in]     *pv_List   IE list
        \return   <none>
*/

void app_OnDCSessionDataSend(void *pv_List)
{
    void *pv_IE = NULL;
    u16      u16_IE;
    ST_IE_DATA pst_data;
    u16   u16_Handsets = 0;
    u32   u32_CallInstance = 0;


    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HANDSETS + CMBS_IE_CALLINSTANCE + CMBS_IE_DATA
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        switch (u16_IE)
        {
            case CMBS_IE_HANDSETS:
                cmbs_api_ie_ShortValueGet(pv_IE, &u16_Handsets, CMBS_IE_HANDSETS);
                break;
            case CMBS_IE_CALLINSTANCE:
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                break;
            case CMBS_IE_DATA:
                cmbs_api_ie_DataGet(pv_IE, &pst_data);
                break;
            default:
                break;
        }
    }
    cmbs_dsr_dc_DataSendRes(g_cmbsappl.pv_CMBSRef, u16_Handsets, u32_CallInstance, TRUE);
}

//  ========== app_OnDCSessionDataSendRSP ===========
/*!
        \brief   Response to send data via data call
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_OnDCSessionDataSendRSP(void *pv_List)
{
    void *pv_IE = NULL;
    u16           u16_IE;
    ST_IE_RESPONSE          st_Response;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HANDSETS + CMBS_IE_CALLINSTANCE + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            switch (u16_IE)
            {
                case CMBS_IE_HANDSETS:
                    break;
                case CMBS_IE_CALLINSTANCE:
                    break;
                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
                default:
                    break;
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}


//  ========== app_OnDectSettingsGetRSP ===========
/*!
        \brief   response of DECT settings get event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnDectSettingsGetRSP(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HW_VERSION + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_DECT_SETTINGS_LIST)
            {
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_DectSettings,
                                         sizeof(st_IEInfo.Info.st_DectSettings),
                                         CMBS_IE_DECT_SETTINGS_LIST,
                                         CMBS_EV_DSR_DECT_SETTINGS_LIST_GET_RES);
                }

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

void app_OnGetBaseNameRSP(void *pv_List)
{
    ST_IE_BASE_NAME st_BaseName;
    void *pv_IE;
    u16      u16_IE;

    cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == CMBS_IE_BASE_NAME)
        {
            cmbs_api_ie_BaseNameGet(pv_IE, &st_BaseName);
        }

        cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
    }

    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal((void *)&st_BaseName.u8_BaseName,
                             sizeof(st_BaseName.u8_BaseName),
                             CMBS_IE_BASE_NAME,
                             CMBS_EV_DSR_GET_BASE_NAME_RES);
    }
}

//  ========== app_OnDectSettingsSetRSP ===========
/*!
        \brief   response of DECT settings set event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnDectSettingsSetRSP(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_HW_VERSION + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_RESPONSE)
            {
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_DectSettings,
                                         sizeof(st_IEInfo.Info.st_DectSettings),
                                         CMBS_IE_RESPONSE,
                                         CMBS_EV_DSR_DECT_SETTINGS_LIST_SET_RES);
                }

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//  ========== app_OnSysLog ===========
/*!
        \brief   response of sys log
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnSysLog(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        // we expect: CMBS_IE_SYS_LOG + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_SYS_LOG)
            {
                if (g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_write_log_buffer_cb != NULL)
                {
                    g_CMBSInstance.st_ApplSlot.pFnCbLogBuffer.pfn_cmbs_api_log_write_log_buffer_cb\
                    (st_IEInfo.Info.st_SysLog.u8_Data, st_IEInfo.Info.st_SysLog.u8_DataLen);
                }
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_SysLog,
                                         sizeof(st_IEInfo.Info.st_SysLog),
                                         CMBS_IE_SYS_LOG,
                                         CMBS_EV_DSR_SYS_LOG);
                }
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

//      ========== app_OnSubscribedHsListGet ===========
/*!
        \brief          response of subscribed handsets get
        \param[in]     *pv_List      IE list
        \return         <none>
*/
void app_OnSubscribedHsListGet(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;
    ST_IE_HANDSETINFO handset[12];
    int i = 0;

    cmbsevent_clearHsList();

    memset(handset, 0, sizeof(handset));

    if (pv_List)
    {
        printf("\n");
        printf("List of registered handsets:\n");
        printf("----------------------------\n");

        // collect information elements.
        // we expect: CMBS_IE_SUBSCRIBED_HS_LIST + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
		g_reg_hs = 0;
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_SUBSCRIBED_HS_LIST)
            {
                u8* pu8_HsName = st_IEInfo.Info.st_SubscribedHsList.u8_HsName;
                cmbsevent_addHs2List(st_IEInfo.Info.st_SubscribedHsList.u16_HsID, pu8_HsName);
                if (i < 12)
                {
                    handset[i].u8_Hs = st_IEInfo.Info.st_SubscribedHsList.u16_HsID;
                    i++;
                }
                
                printf("Number:[%d]\tSubscribed,\tName: \"%s\"", st_IEInfo.Info.st_SubscribedHsList.u16_HsID, pu8_HsName);
				g_reg_hs |= (1 << (st_IEInfo.Info.st_SubscribedHsList.u16_HsID - 1));

                if (st_IEInfo.Info.st_SubscribedHsList.u8_FXS_ExNumLen != 0)
                {
                    //the entry is FXS

                    printf(", Extension Number:[%s]\n", st_IEInfo.Info.st_SubscribedHsList.u8_FXS_ExNum);
                }
                else
                {
                    printf("\n");
                }

            }
            else if (u16_IE == CMBS_IE_RESPONSE)
            {
                //if (g_cmbsappl.n_Token)
                /*{
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_Resp,
                                         sizeof(st_IEInfo.Info.st_Resp),
                                         CMBS_IE_SUBSCRIBED_HS_LIST,
                                         CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES);
                }*/
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        appcmbs_ObjectSignal((void *)handset, sizeof(handset), i, CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES);

    }

    cmbsevent_OnHSListUpdated();
}


//      ========== app_OnLineSettingsListGet ===========
/*!
        \brief          response of line settings get
        \param[in]     *pv_List      IE list
        \return         <none>
*/
void app_OnLineSettingsListGet(void *pv_List)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        printf("\n");
        printf("List of Line settings:\n");
        printf("----------------------------\n");
        // collect information elements.
        // we expect: CMBS_IE_LINE_SETTINGS_LIST + CMBS_IE_RESPONSE
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (u16_IE == CMBS_IE_LINE_SETTINGS_LIST)
            {
                printf("\nLineId = %d\nAttached HS mask = 0x%X\nCall Intrusion = %d\nMultiple Calls = %d\nLine Type = %d\n",
                       st_IEInfo.Info.st_LineSettingsList.u8_Line_Id, st_IEInfo.Info.st_LineSettingsList.u16_Attached_HS,
                       st_IEInfo.Info.st_LineSettingsList.u8_Call_Intrusion,
                       st_IEInfo.Info.st_LineSettingsList.u8_Multiple_Calls,
                       st_IEInfo.Info.st_LineSettingsList.e_LineType);
            }
            else if (u16_IE == CMBS_IE_RESPONSE)
            {
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_Resp,
                                         sizeof(st_IEInfo.Info.st_Resp),
                                         CMBS_IE_RESPONSE,
                                         CMBS_EV_DSR_LINE_SETTINGS_LIST_GET_RES);
                }

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}


//  ========== app_OnUserDefinedInd ===========
/*!
        \brief   reception of user defined indication
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void           app_OnUserDefinedInd(void *pv_List, E_CMBS_EVENT_ID e_EventID)
{
    ST_APPCMBS_IEINFO
    st_IEInfo;
    void *pv_IE;
    u16      u16_IE;

    if (pv_List)
    {
        // collect information elements.
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);
            if (CMBS_IE_USER_DEFINED_START <= u16_IE && u16_IE <= CMBS_IE_USER_DEFINED_END)
            {
                if (g_cmbsappl.n_Token)
                {
                    appcmbs_ObjectSignal((void *)&st_IEInfo.Info.st_UserDefined,
                                         sizeof(st_IEInfo.Info.st_UserDefined),
                                         u16_IE,
                                         e_EventID);
                }
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}

void app_OnDCDataSendRes(void *pv_List)
{
    // signal parameter setting to upper application
    UNUSED_PARAMETER(pv_List);
    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal(NULL, 0, 0, CMBS_EV_DSR_DC_DATA_SEND_RES);
    }
}

void app_OnDCSessionStartRes(void *pv_List)
{
    // signal parameter setting to upper application
    UNUSED_PARAMETER(pv_List);
    if (g_cmbsappl.n_Token)
    {
        appcmbs_ObjectSignal(NULL, 0, 0, CMBS_EV_DSR_DC_SESSION_START_RES);
    }
}
void appOnAFEChannelAllocateRes(void *pv_List)
{
    appcmbs_ObjectSignal(pv_List, ((PST_CFR_IE_LIST)pv_List)->u16_CurSize, 0, CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE_RES);
}


void app_OnTurnOnNEMORes(void *pv_List)
{
    if (app_ResponseCheck(pv_List) == CMBS_RESPONSE_OK)
    {
        APPCMBS_INFO(("NEMO set successful\n"));
    }
    else
    {
        APPCMBS_ERROR(("NEMO set failed !!!\n"));
    }
}

#if CHECKSUM_SUPPORT
//  ========== app_OnCheckSumFailure ===========
/*!
        \brief   reception of checksum failure

        \param[in]     *pv_List   IE list

        \return   <none>

*/
void           app_OnCheckSumFailure(void *pv_List)
{
    void *pv_IE;
    u16      u16_IE;
    ST_IE_CHECKSUM_ERROR st_CheckSumError;

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        cmbs_api_ie_ChecksumErrorGet(pv_IE, &st_CheckSumError);

        APPCMBS_INFO((" Checksum failure received\n Error type: 0x%02X, Event: 0x%04X \n",
                      st_CheckSumError.e_CheckSumError, st_CheckSumError.u16_ReceivedEvent));

        /* NOTE: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         * Here the application can resend the IE and clear the WAIT buffer (exit the loop)
         */

    }
}
#endif

//  ========== app_ServiceEntity ===========
/*!
        \brief   CMBS entity to handle response information from target side
        \param[in]  pv_AppRef   application reference
        \param[in]  e_EventID   received CMBS event
        \param[in]  pv_EventData  pointer to IE list
        \return    <int>

*/

int app_ServiceEntity(void *pv_AppRef, E_CMBS_EVENT_ID e_EventID, void *pv_EventData)
{
    UNUSED_PARAMETER(pv_AppRef);

    switch (e_EventID)
    {
        case  CMBS_EV_DSR_HS_REGISTERED:
            app_OnHandsetRegistered(pv_EventData);
            return FALSE; // allow HAN to consume this event as well.

        case  CMBS_EV_DSR_HS_DELETE_RES:
            app_OnHandsetDeleted(pv_EventData);
            break;

        case  CMBS_EV_DSR_HS_IN_RANGE:
            app_OnHandsetInRange(pv_EventData);
            break;

        case  CMBS_EV_DSR_TIME_INDICATION:
            app_OnDateTimeIndication(pv_EventData);
            break;

        case CMBS_EV_DSR_HS_PAGE_PROGRESS:
            app_OnHsLocProgress(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DSR_HS_PAGE_ANSWER:
            app_OnHsLocAnswer(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DSR_HS_PAGE_STOP:
            app_OnHsLocRelease(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DSR_CORD_OPENREG_RES:
            app_OnRegistrationOpenRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_CORD_CLOSEREG_RES:
            app_OnRegistrationCloseRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_CORD_CLOSEREG:
            app_OnRegistrationClose(pv_EventData);
            return FALSE; // allow HAN to consume this event as well.
            break;

        case CMBS_EV_DSR_PARAM_GET_RES:
            app_OnParamGetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_PARAM_SET_RES:
            app_OnParamSetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_PARAM_AREA_GET_RES:
            app_OnParamAreaGetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_PARAM_AREA_SET_RES:
            app_OnParamAreaSetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_PARAM_AREA_GET:
            app_OnParamAreaGet(pv_EventData);
            break;

        case CMBS_EV_DSR_PARAM_AREA_SET:
            app_OnParamAreaSet(pv_EventData);
            break;

        case CMBS_EV_DSR_FW_VERSION_GET_RES:
            app_OnFwVersionGetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_HW_VERSION_GET_RES:
            app_OnHwVersionGetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_SYS_LOG:
            app_OnSysLog(pv_EventData);
            break;

        case CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES:
            app_OnSubscribedHsListGet(pv_EventData);
            break;

        case CMBS_EV_DSR_LINE_SETTINGS_LIST_GET_RES:
            app_OnLineSettingsListGet(pv_EventData);
            break;

        case CMBS_EV_DSR_HS_PAGE_START_RES:
            app_OnPageStarted(pv_EventData);
            break;

        case CMBS_EV_DSR_HS_PAGE_STOP_RES:
            app_OnPageStoped(pv_EventData);
            break;

        case CMBS_EV_DSR_DECT_SETTINGS_LIST_GET_RES:
            app_OnDectSettingsGetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_DECT_SETTINGS_LIST_SET_RES:
            app_OnDectSettingsSetRSP(pv_EventData);
            break;

        case CMBS_EV_DSR_HS_SUBSCRIBED_LIST_SET_RES:
        case CMBS_EV_DSR_LINE_SETTINGS_LIST_SET_RES:
        case CMBS_EV_DSR_GPIO_CONNECT_RES:
        case CMBS_EV_DSR_GPIO_DISCONNECT_RES:
        case CMBS_EV_DSR_ATE_TEST_START_RES:
        case CMBS_EV_DSR_ATE_TEST_LEAVE_RES:
        case CMBS_EV_DSR_FIXED_CARRIER_RES:
        case CMBS_EV_DSR_AFE_ENDPOINT_CONNECT_RES:
        case CMBS_EV_DSR_AFE_ENDPOINT_ENABLE_RES:
        case CMBS_EV_DSR_AFE_ENDPOINT_DISABLE_RES:
        case CMBS_EV_DSR_AFE_CHANNEL_DEALLOCATE_RES:
        case CMBS_EV_DSR_AFE_ENDPOINT_GAIN_RES:
        case CMBS_EV_DSR_DHSG_SEND_BYTE_RES :
        case CMBS_EV_DSR_GPIO_ENABLE_RES:
        case CMBS_EV_DSR_GPIO_DISABLE_RES:
        case CMBS_EV_DSR_GPIO_CONFIG_SET_RES:
        case CMBS_EV_DSR_EXT_INT_CONFIG_RES:
        case CMBS_EV_DSR_EXT_INT_ENABLE_RES:
        case CMBS_EV_DSR_EXT_INT_DISABLE_RES:
        {
            u8 u8_Resp = app_ResponseCheck(pv_EventData);
            if (u8_Resp == CMBS_RESPONSE_OK)
            {
                APPCMBS_INFO(("APPCMBS-SRV: INFO Response: OK\n"));
            }
            else
            {
                APPCMBS_ERROR(("APPCMBS-SRV: ERROR !!! Response ERROR\n"));
            }

            if (g_cmbsappl.n_Token)
            {
                appcmbs_ObjectSignal(NULL, 0, 0, e_EventID);
            }

            break;
        }
        case CMBS_EV_DSR_GPIO_CONFIG_GET_RES:
        {
            u8 u8_Resp = app_ResponseCheck(pv_EventData);
            if (u8_Resp == CMBS_RESPONSE_OK)
            {
                APPCMBS_INFO(("APPCMBS-SRV: INFO Response: OK\n"));
            }
            if (g_cmbsappl.n_Token)
            {
                appcmbs_ObjectSignal(NULL, 0, 0, e_EventID);
            }
        }
        break;

        case CMBS_EV_DSR_TARGET_UP:
            app_onTargetUp();
            break;
        case CMBS_EV_DSR_SYS_START_RES:
            appcmbs_ObjectSignal(NULL, 0, 0, e_EventID);
            appcmbs_ObjectReport(NULL, 0, 0, e_EventID);
            break;
        case CMBS_EV_DEE_HANDSET_LINK_RELEASE:
            app_OnHandsetLinkRelease(pv_EventData);
            break;

        case CMBS_EV_DSR_EEPROM_SIZE_GET_RES:
            app_OnEepromSizeResp(pv_EventData);
            break;

        case CMBS_EV_DSR_RECONNECT_REQ:
            app_onDisconnectReq();
            break;
        case CMBS_EV_DSR_GET_BASE_NAME_RES:
            app_OnGetBaseNameRSP(pv_EventData);
            break;
        case CMBS_EV_DSR_EEPROM_VERSION_GET_RES:
            app_OnEEPROMVersionGetRSP(pv_EventData);
            break;
        case CMBS_EV_DSR_PING_RES:
            // no handler
            if (g_cmbsappl.n_Token)
            {
                appcmbs_ObjectSignal(NULL, 0, 0, CMBS_EV_DSR_PING_RES);
            }
            break;
        case CMBS_EV_DSR_DC_DATA_SEND_RES:
            app_OnDCDataSendRes(pv_EventData);
            break;
        case CMBS_EV_DSR_DC_SESSION_START_RES:
            app_OnDCSessionStartRes(pv_EventData);
            break;
        case CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE_RES:
            appOnAFEChannelAllocateRes(pv_EventData);
            break;
            //    case CMBS_EV_DSR_AFE_TDM_CONNECT_RES:
            //  appOnAFETDMConnectRes(pv_EventData);
            //  break;
            //   case CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES:
            //app_OnReadHanDeviceTableRes(pv_EventData);
            //   break;
        case CMBS_EV_DSR_EXT_INT_INDICATION:
        case CMBS_EV_DSR_AFE_AUX_MEASUREMENT_RES:
            if (g_cmbsappl.n_Token)
            {
                appcmbs_ObjectSignal(NULL, 0, 0, e_EventID);
            }
            break;

        case CMBS_EV_DSR_TERMINAL_CAPABILITIES_IND:
            appOnTerminalCapabilitiesInd(pv_EventData);
            break;

        case CMBS_EV_DSR_HS_PROP_DATA_RCV_IND:
            appOnPropritaryDataRcvInd(pv_EventData);
            break;

#if CHECKSUM_SUPPORT
        case CMBS_EV_CHECKSUM_FAILURE:
            app_OnCheckSumFailure(pv_EventData);
            break;
#endif

        default:
            if (CMBS_EV_DSR_USER_DEFINED_START <= e_EventID && e_EventID <= CMBS_EV_DSR_USER_DEFINED_END)
            {
                app_OnUserDefinedInd(pv_EventData, e_EventID);
                break;
            }
            return FALSE;
    }

    return TRUE;
}


//  ==========  app_HandsetMap ===========
/*!
        \brief      convert handset string to used handset bit mask
                        does also support "all" and "none" as input
        \param[in,out]  psz_Handsets pointer to handset string
        \return    <u16>    return handset bit mask
*/

u16            app_HandsetMap(char *psz_Handsets)
{
    u16   u16_Handset = 0;

    if (!strcmp(psz_Handsets, "all\0"))      //including FXS
        return 0xFFFF;

    if (!strcmp(psz_Handsets, "none\0"))
    {
        return 0;
    }

    sscanf((char *)psz_Handsets, "%hd", &u16_Handset);

    if ((u16_Handset != 0) && (u16_Handset <= CMBS_HS_SUBSCRIBED_MAX_NUM + CMBS_MAX_FXS_EXTENSIONS))
    {
        u16_Handset = (1 << (u16_Handset - 1));
    }

    APPCMBS_INFO(("APPCMBS-SRV: INFO u16_Handset %x\n", u16_Handset));

    return u16_Handset;
}

//  ========== app_SrvHandsetDelete  ===========
/*!
        \brief     call CMBS function of handset delete
        \param[in,out]   psz_Handsets     pointer to parameter string,e.g."1234" or "all"
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvHandsetDelete(char *psz_Handsets)
{
    return cmbs_dsr_hs_Delete(g_cmbsappl.pv_CMBSRef, app_HandsetMap(psz_Handsets));
}

//  ========== app_SrvHandsetPage  ===========
/*!
        \brief     call CMBS function of handset page
        \param[in,out]   psz_Handsets     pointer to parameter string,e.g."1234" or "all" or "none"
        \return     <E_CMBS_RC>
      \note              stopp pagine, re-call this function with "none"
*/
E_CMBS_RC      app_SrvHandsetPage(char *psz_Handsets)
{
    return cmbs_dsr_hs_Page(g_cmbsappl.pv_CMBSRef, app_HandsetMap(psz_Handsets));
}

//      ========== app_SrvHandsetStopPaging  ===========
/*!
        \brief               call CMBS function of stop handsets paging
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
      \note              stopp pagine, re-call this function with "none"
*/
E_CMBS_RC      app_SrvHandsetStopPaging(void)
{
    return cmbs_dsr_hs_StopPaging(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvSubscriptionOpen  ===========
/*!
        \brief     call CMBS function of open registration
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC app_SrvSubscriptionOpen(u32 u32_timeout)
{
    return cmbs_dsr_cord_OpenRegistration(g_cmbsappl.pv_CMBSRef, u32_timeout);
}

//  ========== app_SrvSubscriptionClose  ===========
/*!
        \brief     call CMBS function of open registration
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvSubscriptionClose(void)
{
    return cmbs_dsr_cord_CloseRegistration(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvEncryptionDisable  ===========
/*!
        \brief     call CMBS function of Disabling encryption
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvEncryptionDisable(void)
{
    return cmbs_dsr_cord_DisableEncryption(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvEncryptionEnable  ===========
/*!
        \brief     call CMBS function of Enabling encryption
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvEncryptionEnable(void)
{
    return cmbs_dsr_cord_EnableEncryption(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvFixedCarrierSet  ===========
/*!
        \brief     call CMBS function of setting a fixed carrier
        \param[in,out]   <u8_Value> new carrier setting
        \return     <E_CMBS_RC>
*/

E_CMBS_RC      app_SrvFixedCarrierSet(u8 u8_Value)
{
    return cmbs_dsr_FixedCarrierSet(g_cmbsappl.pv_CMBSRef, u8_Value);
}

//  ========== app_SrvPINCodeSet  ===========
/*!
        \brief     set authentication PIN code, registration code
        \param[in,out]   psz_PIN    pointer to PIN string, e.g. "ffff1590" for PIN 1590
        \return     <E_CMBS_RC>

*/
E_CMBS_RC      app_SrvPINCodeSet(char *psz_PIN)
{
    unsigned int      i;

    u8       szPinCode[CMBS_PARAM_PIN_CODE_LENGTH] = { 0, 0, 0, 0 };

    if (strlen(psz_PIN) == (CMBS_PARAM_PIN_CODE_LENGTH * 2))
    {
        printf("PIN %s\n", psz_PIN);
        for (i = 0; i < (CMBS_PARAM_PIN_CODE_LENGTH * 2); i++)
        {
            printf("%c => %d BCD %02x\n", psz_PIN[i], (i / 2), szPinCode[i / 2]);

            if (psz_PIN[i] == 'f')
            {
                if ((i & 0x01))
                {
                    szPinCode[i / 2] |= 0x0F;
                }
                else
                {
                    szPinCode[i / 2] |= 0xF0;
                }

            }
            else if (psz_PIN[i] >= '0' && psz_PIN[i] <= '9')
            {
                if ((i & 0x01))
                {
                    szPinCode[i / 2] |= (psz_PIN[i] - '0');
                }
                else
                {
                    szPinCode[i / 2] |= (psz_PIN[i] - '0') << 4;
                }
            }
            else
            {
                if ((i & 0x01))
                {
                    szPinCode[i / 2] |= 1;
                }
                else
                {
                    szPinCode[i / 2] |= 1 << 4;
                }

            }
        }
    }
    printf("%02x %02x %02x\n", szPinCode[0], szPinCode[1], szPinCode[2]);
    return cmbs_dsr_param_Set(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_EEPROM, CMBS_PARAM_AUTH_PIN, szPinCode, CMBS_PARAM_PIN_CODE_LENGTH);
}

//  ========== app_SrvTestModeGet  ===========
/*!
        \brief     get test mode state
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvTestModeGet(u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_param_Get(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_EEPROM, CMBS_PARAM_TEST_MODE);
}

//  ========== app_SrvTestModeGet  ===========
/*!
        \brief     set TBR 6 on
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvTestModeSet(void)
{
    u8 u8_TestMode = CMBS_TEST_MODE_TBR6;

    return cmbs_dsr_param_Set(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_EEPROM, CMBS_PARAM_TEST_MODE, &u8_TestMode, CMBS_PARAM_TEST_MODE_LENGTH);
}


//  ========== _app_HexString2Byte  ===========
/*!
      \brief            convert a 2 bytes hex character string to binary
      \param[in]        psz_HexString     2 bytes hex character string
      \return           1 byte
*/
char           _app_HexString2Byte(char *psz_HexString)
{
    char        c;

    // first character
    c = (psz_HexString[0] >= 'A' ? ((psz_HexString[0] & 0xdf) - 'A') + 10 : (psz_HexString[0] - '0'));

    c <<= 4;
    // second character
    c += (psz_HexString[1] >= 'A' ? ((psz_HexString[1] & 0xdf) - 'A') + 10 : (psz_HexString[1] - '0'));

    return c;
}


//  ========== app_SrvParamGet  ===========
/*!
        \brief     get parameter information
        \param[in,out]   e_Param       pre-defined parameter settings
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvParamGet(E_CMBS_PARAM e_Param, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_param_Get(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_EEPROM, e_Param);
}

//  ========== app_SrvParamSet  ===========
/*!
      \brief            set parameter value
      \param[in]        e_Param           pre-defined parameter settings
      \param[in]        pu8_Data          user input data
        \param[in]        u16_Length        length of data
        \param[in]        u32_Token         TRUE tells the upper application to wait for an answer
      \return           <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvParamSet(E_CMBS_PARAM e_Param, u8 *pu8_Data, u16 u16_Length, u32 u32_Token)
{
    if (e_Param != CMBS_PARAM_UNKNOWN)
    {
        appcmbs_PrepareRecvAdd(u32_Token);

        return cmbs_dsr_param_Set(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_EEPROM, e_Param, pu8_Data, u16_Length);
    }
    else
    {
        APPCMBS_ERROR(("ERROR: app_SrvParamSet offset is not implemented, yet \n"));
    }

    return CMBS_RC_ERROR_GENERAL;

}

//      ========== app_ProductionParamGet  ===========
/*!
        \brief               get parameter information
        \param[in,out]       e_Param       pre-defined parameter settings
        \param[in,out]       u32_Token     TRUE, if upper application waits for answer
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_ProductionParamGet(E_CMBS_PARAM e_Param, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_param_Get(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_PRODUCTION, e_Param);
}

//      ========== app_ProductionParamSet  ===========
/*!
      \brief            set parameter value
      \param[in]        e_Param           pre-defined parameter settings
      \param[in]        pu8_Data          user input data
      \param[in]        u16_Length        length of data
      \param[in]        u32_Token         TRUE tells the upper application to wait for an answer
      \return           <E_CMBS_RC>
*/
E_CMBS_RC      app_ProductionParamSet(E_CMBS_PARAM e_Param, u8 *pu8_Data, u16 u16_Length, u32 u32_Token)
{
    if (e_Param != CMBS_PARAM_UNKNOWN)
    {
        appcmbs_PrepareRecvAdd(u32_Token);

        return cmbs_dsr_param_Set(g_cmbsappl.pv_CMBSRef, CMBS_PARAM_TYPE_PRODUCTION, e_Param, pu8_Data, u16_Length);
    }
    else
    {
        APPCMBS_ERROR(("ERROR: app_ProductionParamSet offset is not implemented, yet \n"));
    }

    return CMBS_RC_ERROR_GENERAL;

}


//  ========== app_SrvParamAreaGet  ===========
/*!
        \brief     get parameter area data
        \param[in,out]   e_AreaType    memory area type.
        \param[in,out]   u16_Pos       offset in memory area
        \param[in,out]   u16_Length    length of to read area
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvParamAreaGet(E_CMBS_PARAM_AREA_TYPE e_AreaType, u32 u32_Pos, u16 u16_Length, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_param_area_Get(g_cmbsappl.pv_CMBSRef, e_AreaType, u32_Pos, u16_Length);
}

//  ========== app_SrvParamSet  ===========
/*!
      \brief            set parameter value
      \param[in]        e_AreaType     memory area type
        \param[in]        u16_Pos        offset in memory area
        \param[in]        u16_Length        length of data
      \param[in]        pu8_Data          user input data
        \param[in]        u32_Token         TRUE tells the upper application to wait for an answer
      \return           <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvParamAreaSet(E_CMBS_PARAM_AREA_TYPE e_AreaType, u32 u32_Pos, u16 u16_Length, u8 *pu8_Data, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_param_area_Set(g_cmbsappl.pv_CMBSRef, e_AreaType, u32_Pos, pu8_Data, u16_Length);
}

//  ========== app_SrvFWVersionGet  ===========
/*!
        \brief     get firmware version
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>

*/
E_CMBS_RC      app_SrvFWVersionGet(E_CMBS_MODULE e_Module, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_fw_VersionGet(g_cmbsappl.pv_CMBSRef, e_Module);
}

E_CMBS_RC   app_SrvEEPROMVersionGet(u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_EEPROM_VersionGet(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvHWVersionGet  ===========
/*!
        \brief     get hardware version
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvHWVersionGet(u32 u32_Token)
{
    ST_APPCMBS_CONTAINER st_Container;
    PST_IE_HW_VERSION    pst_hwVersion;

    appcmbs_PrepareRecvAdd(u32_Token);

    cmbs_dsr_hw_VersionGet(g_cmbsappl.pv_CMBSRef);

    appcmbs_WaitForContainer(CMBS_EV_DSR_HW_VERSION_GET_RES, &st_Container);

    pst_hwVersion = (PST_IE_HW_VERSION)st_Container.ch_Info;
    g_CMBSInstance.u8_HwChip = pst_hwVersion->u8_HwChip;
    g_CMBSInstance.u8_HwComType = pst_hwVersion->u8_HwComType;

    return CMBS_RC_OK;
}

//  ========== app_SrvLogBufferStart  ===========
/*!
        \brief     start system log
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvLogBufferStart(void)
{
    return cmbs_dsr_LogBufferStart(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvLogBufferStop  ===========
/*!
        \brief     stop system log
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvLogBufferStop(void)
{
    return cmbs_dsr_LogBufferStop(g_cmbsappl.pv_CMBSRef);
}


//  ========== app_SrvLogBufferRead  ===========
/*!
        \brief     read log buffer
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvLogBufferRead(u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_LogBufferRead(g_cmbsappl.pv_CMBSRef);
}


//  ========== app_SrvSystemReboot  ===========
/*!
        \brief     reboot CMBS Target
        \param[in,out]   <none>
        \return     <E_CMBS_RC>
      \note              also re-register CMBS-API to system!

*/
E_CMBS_RC      app_SrvSystemReboot(void)
{
    u8 u8_RetCode = cmbs_dsr_sys_Reset(g_cmbsappl.pv_CMBSRef);

#ifdef CMBS_COMA
    app_CSSCOMAReset();
#else

    if (g_CMBSInstance.u8_HwComType == CMBS_HW_COM_TYPE_USB)
    {
        // for USB we must close the CDC device to allow the USB dongle to perform a new USB enumeration
        printf("\nReconnecting to target, please wait %d seconds \n", APPCMBS_RECONNECT_TIMEOUT / 1000);
        if ((u8_RetCode = appcmbs_ReconnectApplication(APPCMBS_RECONNECT_TIMEOUT)) != CMBS_RC_OK)
        {
            printf("\n *****************************************************");
            printf("\n Can't reconnect to target!!!!!");
            printf("\n *****************************************************");
        }
    }
#endif

    appcmbs_WaitForContainer(CMBS_EV_DSR_SYS_START_RES, NULL);
    return u8_RetCode;
}

/* == ALTDV == */

//      ========== app_SrvSystemPowerOff  ===========
/*!
        \brief               power off CMBS Target
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvSystemPowerOff(void)
{
    return cmbs_dsr_sys_PowerOff(g_cmbsappl.pv_CMBSRef);
}

//      ========== app_SrvRFSuspend  ===========
/*!
        \brief               RF Suspend on CMBS Target
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvRFSuspend(void)
{
    return cmbs_dsr_sys_RFSuspend(g_cmbsappl.pv_CMBSRef);
}

//      ========== app_SrvRFResume  ===========
/*!
        \brief               RF Resume on CMBS Target
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvRFResume(void)
{
    return cmbs_dsr_sys_RFResume(g_cmbsappl.pv_CMBSRef);
}

//      ========== app_SrvTurnOnNEMo  ===========
/*!
        \brief               Turn On NEMo mode for the CMBS base
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvTurnOnNEMo(void)
{
    return cmbs_dsr_sys_TurnOnNEMo(g_cmbsappl.pv_CMBSRef);
}

//      ========== app_SrvTurnOffNEMo  ===========
/*!
        \brief               Turn Off NEMo mode for the CMBS base
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvTurnOffNEMo(void)
{
    return cmbs_dsr_sys_TurnOffNEMo(g_cmbsappl.pv_CMBSRef);
}

//      ========== app_SrvHandsetSubscribed  ===========
/*!
        \brief               Get information about subscribed handsets
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvRegisteredHandsets(u16 u16_HsMask, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);
    return cmbs_dsr_GET_InternalnameList(g_cmbsappl.pv_CMBSRef, u16_HsMask);
}

//      ========== app_SrvSetNewHandsetName  ===========
/*!
        \brief               Set new name for subscribed handset
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC app_SrvSetNewHandsetName(u16 u16_HsId, u8 *pu8_HsName, u16 u16_HsNameSize, u32 u32_Token)
{
    ST_IE_SUBSCRIBED_HS_LIST    st_SubscribedHsList;

    memset(&st_SubscribedHsList, 0, sizeof(st_SubscribedHsList));

    appcmbs_PrepareRecvAdd(u32_Token);
    st_SubscribedHsList.u16_HsID = u16_HsId;
    memcpy(st_SubscribedHsList.u8_HsName, pu8_HsName, MIN(u16_HsNameSize, CMBS_HS_NAME_MAX_LENGTH));
    st_SubscribedHsList.u16_NameLength = u16_HsNameSize;
    return cmbs_dsr_SET_InternalnameList(g_cmbsappl.pv_CMBSRef, &st_SubscribedHsList);
}

//  ========== app_SrvAddNewExtension  ===========
/*!
        \brief     Add new extension to internal names list
        \param[in,out]   extension number, extension name
        \return     <E_CMBS_RC>
*/
E_CMBS_RC app_SrvAddNewExtension(u8 *pu8_Name, u16 u16_NameSize, u8 *pu8_Number, u8 u8_NumberSize, u32 u32_Token)
{
    ST_IE_SUBSCRIBED_HS_LIST    st_Extension;

    memset(&st_Extension, 0, sizeof(st_Extension));

    appcmbs_PrepareRecvAdd(u32_Token);

    memcpy(st_Extension.u8_HsName, pu8_Name, u16_NameSize);
    st_Extension.u16_NameLength = u16_NameSize;
    st_Extension.u8_HsName[u16_NameSize] = 0;

    memcpy(st_Extension.u8_FXS_ExNum, pu8_Number, u8_NumberSize);
    st_Extension.u8_FXS_ExNumLen = u8_NumberSize;

    return cmbs_dsr_AddNewExtension(g_cmbsappl.pv_CMBSRef, &st_Extension);
}

//  ========== app_SrvSetBaseName  ===========
/*!
        \brief     Set new Base name
        \param[in,out]   base name, base name length
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvSetBaseName(u8 *pu8_BaseName, u8 u8_BaseNameSize, u32 u32_Token)
{
    ST_IE_BASE_NAME st_BaseName;

    memset(&st_BaseName, 0, sizeof(st_BaseName));

    appcmbs_PrepareRecvAdd(u32_Token);


    memcpy(st_BaseName.u8_BaseName, pu8_BaseName, u8_BaseNameSize);
    st_BaseName.u8_BaseNameLen = u8_BaseNameSize;

    return cmbs_dsr_SetBaseName(g_cmbsappl.pv_CMBSRef, &st_BaseName);

}

E_CMBS_RC   app_SrvGetBaseName(u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);
    return cmbs_dsr_GetBaseName(g_cmbsappl.pv_CMBSRef);
}

//      ========== app_SrvLineSettingsGet  ===========
/*!
        \brief               Get information about lines settings
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvLineSettingsGet(u16 u16_LinesMask, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);
    return cmbs_dsr_GET_Line_setting_list(g_cmbsappl.pv_CMBSRef, u16_LinesMask);
}

//      ========== app_SrvLineSettingsSet  ===========
/*!
        \brief               Set information about lines settings
        \param[in,out]       <none>
        \return              <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvLineSettingsSet(ST_IE_LINE_SETTINGS_LIST *pst_LineSettingsList, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);
    return cmbs_dsr_SET_Line_setting_list(g_cmbsappl.pv_CMBSRef, pst_LineSettingsList);
}


E_CMBS_RC      app_OnHandsetLinkRelease(void *pv_List)
{
    u8 u8_HsNumber;
    void *pv_IE;
    u16      u16_IE;

    cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

    cmbs_api_ie_HsNumberGet(pv_IE, &u8_HsNumber);

    printf("app_OnHandsetLinkRelease HandsetNumber=%d\n", u8_HsNumber);

    cmbsevent_OnHandsetLinkReleased(u8_HsNumber);

    return CMBS_RC_OK;
}


//  ========== app_SrvDectSettingsGet  ===========
/*!
        \brief     get DECT settings
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvDectSettingsGet(u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_DectSettingsList_Get(g_cmbsappl.pv_CMBSRef);
}

//  ========== app_SrvDectSettingsGet  ===========
/*!
        \brief     get DECT settings
        \param[in,out]   u32_Token     TRUE, if upper application waits for answer
        \return     <E_CMBS_RC>
*/
E_CMBS_RC      app_SrvDectSettingsSet(ST_IE_DECT_SETTINGS_LIST *pst_DectSettingsList, u32 u32_Token)
{
    appcmbs_PrepareRecvAdd(u32_Token);

    return cmbs_dsr_DectSettingsList_Set(g_cmbsappl.pv_CMBSRef, pst_DectSettingsList);
}

static E_CMBS_RC app_SrvSendCurrentDateAndTime(u8 u8_HsNum)
{
    ST_DATE_TIME   st_DateAndTime;

    time_t t;
    struct tm *t_m;
    t = time(NULL);
    t_m = localtime(&t);

    st_DateAndTime.e_Coding = CMBS_DATE_TIME;
    st_DateAndTime.e_Interpretation = CMBS_CURRENT_TIME;

    st_DateAndTime.u8_Year  = t_m->tm_year - 100;
    st_DateAndTime.u8_Month = t_m->tm_mon + 1;
    st_DateAndTime.u8_Day   = t_m->tm_mday;

    st_DateAndTime.u8_Hours = t_m->tm_hour;
    st_DateAndTime.u8_Mins  = t_m->tm_min;
    st_DateAndTime.u8_Secs  = t_m->tm_sec;

    st_DateAndTime.u8_Zone  = 8;

    return cmbs_dsr_time_Update(g_cmbsappl.pv_CMBSRef, 1, &st_DateAndTime, 1 << (u8_HsNum - 1));
}
//  ========== app_OnEepromSizeResp ===========
/*!
        \brief   Processing of EEprom size responce event
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_OnEepromSizeResp(void *pv_List)
{
    u16 u16_EepromSize = 0;
    void *pv_IE;
    u16      u16_IE;

    cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
    cmbs_api_ie_ShortValueGet(pv_IE, &u16_EepromSize, CMBS_IE_SHORT_VALUE);

    appcmbs_ObjectSignal((char *)&u16_EepromSize, sizeof(u16_EepromSize), 0, CMBS_EV_DSR_EEPROM_SIZE_GET_RES);
}
//  ========== app_SrvGetEepromSize  ===========
/*!
        \brief     get EEprom size
        \param[in,out]   void
        \return     <E_CMBS_RC>
*/
E_CMBS_RC   app_SrvGetEepromSize(void)
{
    appcmbs_PrepareRecvAdd(1);
    return cmbs_dsr_GetEepromSize(g_cmbsappl.pv_CMBSRef);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC   app_SrvReconnectResp(void)
{
    return cmbs_dsr_ReconnectResp(g_cmbsappl.pv_CMBSRef);
}
//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
DWORD app_ReconnectThread(LPVOID lpThreadParameter)
{
    UNUSED_PARAMETER(lpThreadParameter);

    SleepMs(100);
    appcmbs_ReconnectApplication(0);
    cfr_ie_DeregisterThread((u32)GetCurrentThreadId());
    return 0;
}
#elif __linux__
void* app_ReconnectThread(void *ptr)
{
    UNUSED_PARAMETER(ptr);

    SleepMs(100);
    appcmbs_ReconnectApplication(0);
    cfr_ie_DeregisterThread((u32)pthread_self());
    return 0;
}
#endif
//  ========== app_onDisconnectReq ===========
/*!
        \brief   Perform host reconnect on target request
        \param[in]     *pv_List   IE list
        \return   <none>
*/
void app_onDisconnectReq(void)
{
    app_SrvReconnectResp();
    app_onTargetUp();
}


void app_onTargetUp(void)
{
#ifdef WIN32
    ULONG       uReconnectThread;
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)app_ReconnectThread, 0, 0, &uReconnectThread);
#elif __linux__
    pthread_t   uReconnectThread;
    pthread_create(&uReconnectThread, NULL, app_ReconnectThread, (void*) NULL);
#endif
}


#ifdef CMBS_COMA
static void app_CSSCOMAReset(void)
{
    cfr_comaStop();
    SleepMs(3000);
    system("/css/cssload.sh");
    printf("Resetting COMA system.\nPlease, start App again...\n");
    exit(0);
}
#endif //CMBS_COMA

st_AFEConfiguration AFEconfig;


void     appsrv_AFEOpenAudioChannel(void)
{

    ST_IE_MEDIA_CHANNEL st_ChannelConfig;
    ST_IE_MEDIA_DESCRIPTOR st_CodecConfig;
    u8 u8_CodecsLength = 0;
    PST_CFR_IE_LIST pv_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    // construct information element for MediaDesc
    st_CodecConfig.e_Codec = AFEconfig.e_Codec;
    st_CodecConfig.pu8_CodecsList[u8_CodecsLength++] = AFEconfig.e_Codec;
    st_CodecConfig.pu8_CodecsList[u8_CodecsLength++] = AFEconfig.e_Codec == CMBS_AUDIO_CODEC_PCM_LINEAR_NB ? CMBS_AUDIO_CODEC_PCM_LINEAR_WB : CMBS_AUDIO_CODEC_PCM_LINEAR_NB;
    st_CodecConfig.u8_CodecsLength = u8_CodecsLength;
    cmbs_api_ie_MediaDescAdd(pv_RefIEList, &st_CodecConfig);

    // construct information element for media channel.
    // u32_ChannelParameter is the slot mask. u32_ChannelID is recieved from CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE_RES
    st_ChannelConfig.u32_ChannelID = AFEconfig.u32_ChannelID;
    st_ChannelConfig.u32_ChannelParameter = AFEconfig.u32_SlotMask;
    st_ChannelConfig.e_Type = CMBS_MEDIA_TYPE_AUDIO_IOM;

    cmbs_api_ie_MediaChannelAdd(pv_RefIEList, &st_ChannelConfig);

    cmbs_dem_ChannelStart(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

}

void  appsrv_AFECloseAudioChannel(u32 u32_ChannelID)
{
    ST_APPCMBS_CONTAINER        st_Container;
    ST_IE_MEDIA_CHANNEL st_ChannelConfig;
    ST_IE_RESPONSE st_Response;
    PST_CFR_IE_LIST pv_RefIEList = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();

    // we only need the u32_ChannelID
    memset(&st_ChannelConfig, 0, sizeof(ST_IE_MEDIA_CHANNEL));

    st_ChannelConfig.u32_ChannelID = u32_ChannelID;
    cmbs_api_ie_MediaChannelAdd(pv_RefIEList, &st_ChannelConfig);

    cmbs_dem_ChannelStop(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DEM_CHANNEL_STOP_RES, &st_Container);
    st_Response.e_Response = st_Container.ch_Info[0];

    if (st_Response.e_Response != CMBS_RESPONSE_OK)
    {
        printf("CHANNEL STOP returned error!!!\n");
    }
    else
    {
        cmbs_dsr_AFE_ChannelDeallocate(g_cmbsappl.pv_CMBSRef, &st_ChannelConfig);

        appcmbs_PrepareRecvAdd(1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_CHANNEL_DEALLOCATE_RES, &st_Container);
    }

}
void appsrv_AFEAllocateChannel(void)
{
    ST_APPCMBS_CONTAINER        st_Container;
    ST_IE_MEDIA_CHANNEL st_ChannelConfig;
    ST_IE_RESPONSE st_Response;
    void *pv_IE;
    u16      u16_IE;

    cmbs_dsr_AFE_ChannelAllocate(g_cmbsappl.pv_CMBSRef,  AFEconfig.u8_Resource);

    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE_RES, &st_Container);
    st_Response.e_Response = CMBS_RESPONSE_ERROR;

    if (st_Container.ch_Info[0])
    {
        cmbs_api_ie_GetFirst((PST_CFR_IE_LIST)&st_Container.ch_Info, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            switch (u16_IE)
            {
                case CMBS_IE_AFE_INSTANCE_NUM:
                    cmbs_api_ie_AFE_InstanceNumGet(pv_IE, &AFEconfig.u8_InstanceNum);
                    printf("u8_InstanceNumber is %2X \n", AFEconfig.u8_InstanceNum);
                    break;
                case CMBS_IE_MEDIACHANNEL:
                    cmbs_api_ie_MediaChannelGet(pv_IE, &st_ChannelConfig);
                    AFEconfig.u32_ChannelID = st_ChannelConfig.u32_ChannelID;
                    printf("st_ChannelConfig is %2X, %2X, %2X \n", st_ChannelConfig.e_Type, st_ChannelConfig.u32_ChannelID, st_ChannelConfig.u32_ChannelParameter);
                    break;
                case CMBS_IE_RESPONSE:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
            }
            cmbs_api_ie_GetNext((PST_CFR_IE_LIST)&st_Container.ch_Info, &pv_IE, &u16_IE);
        }
    }
    if (st_Response.e_Response == CMBS_RESPONSE_ERROR)
    {
        printf("CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE_RES returned ERROR response, not sending CHANNEL START\n");
    }

}


void app_AFEConnectEndpoints(void)
{
    ST_APPCMBS_CONTAINER        st_Container;

    cmbs_dsr_AFE_EndpointConnect(g_cmbsappl.pv_CMBSRef, &AFEconfig.st_AFEEndpoints); // add CMBS_AFE_ADC_IN_START to each chosen number
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_ENDPOINT_CONNECT_RES, &st_Container);
}

void app_AFEEnableDisableEndpoint(ST_IE_AFE_ENDPOINT *pst_Endpoint, u16 u16_Enable)
{
    ST_APPCMBS_CONTAINER        st_Container;

    appcmbs_PrepareRecvAdd(1);

    if (u16_Enable)
    {
        cmbs_dsr_AFE_EndpointEnable(g_cmbsappl.pv_CMBSRef, pst_Endpoint);
        appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_ENDPOINT_ENABLE_RES, &st_Container);

    }
    else
    {
        cmbs_dsr_AFE_EndpointDisable(g_cmbsappl.pv_CMBSRef, pst_Endpoint);
        appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_ENDPOINT_DISABLE_RES, &st_Container);

    }

}
void appsrv_AFESetEndpointGain(ST_IE_AFE_ENDPOINT_GAIN *pst_EndpointGain, u16 u16_Input)
{

    ST_APPCMBS_CONTAINER        st_Container;
    UNUSED_PARAMETER(u16_Input);
    cmbs_dsr_AFE_SetEndpointGain(g_cmbsappl.pv_CMBSRef, pst_EndpointGain);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_ENDPOINT_GAIN_RES, &st_Container);
}

void appsrv_AFEDHSGSendByte(u8 u8_Value)
{
    ST_APPCMBS_CONTAINER        st_Container;

    cmbs_dsr_DHSGValueSend(g_cmbsappl.pv_CMBSRef, u8_Value);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_DHSG_SEND_BYTE_RES, &st_Container);
}

void app_SrvGPIOEnable(PST_IE_GPIO_ID st_GPIOId)
{
    ST_APPCMBS_CONTAINER        st_Container;
    cmbs_dsr_GPIOEnable(g_cmbsappl.pv_CMBSRef, st_GPIOId);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_ENABLE_RES, &st_Container);
}

void app_SrvGPIODisable(PST_IE_GPIO_ID st_GPIOId)
{
    ST_APPCMBS_CONTAINER        st_Container;
    cmbs_dsr_GPIODisable(g_cmbsappl.pv_CMBSRef, st_GPIOId);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_DISABLE_RES, &st_Container);
}

void app_SrvGPIOSet(PST_IE_GPIO_ID st_GPIOId, PST_GPIO_Properties pst_GPIOProp)
{
    ST_APPCMBS_CONTAINER        st_Container;
    void *pv_RefIEList = NULL;

    pv_RefIEList = cmbs_api_ie_GetList();

    if (pv_RefIEList)
    {
        cmbs_api_ie_GPIOIDAdd(pv_RefIEList, st_GPIOId);

        if (pst_GPIOProp[0].u8_Value != 0xF)
            cmbs_api_ie_GPIOModeAdd(pv_RefIEList, pst_GPIOProp[0].u8_Value);
        if (pst_GPIOProp[1].u8_Value != 0xF)
            cmbs_api_ie_GPIOPullTypeAdd(pv_RefIEList, pst_GPIOProp[1].u8_Value);
        if (pst_GPIOProp[2].u8_Value != 0xF)
            cmbs_api_ie_GPIOPullEnaAdd(pv_RefIEList, pst_GPIOProp[2].u8_Value);
        if (pst_GPIOProp[3].u8_Value != 0xF)
            cmbs_api_ie_GPIOValueAdd(pv_RefIEList, pst_GPIOProp[3].u8_Value);

        cmbs_dsr_GPIOConfigSet(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
        appcmbs_PrepareRecvAdd(1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_CONFIG_SET_RES, &st_Container);
    }
}

void app_SrvGPIOGet(PST_IE_GPIO_ID st_GPIOId, PST_GPIO_Properties pst_GPIOProp)
{

    ST_APPCMBS_CONTAINER        st_Container;
    void *pv_RefIEList = NULL;

    pv_RefIEList = cmbs_api_ie_GetList();

    if (pv_RefIEList)
    {
        cmbs_api_ie_GPIOIDAdd(pv_RefIEList, st_GPIOId);

        if (pst_GPIOProp[0].e_IE != 0xF)
            cmbs_api_ie_GPIOModeAdd(pv_RefIEList, pst_GPIOProp[0].u8_Value);
        if (pst_GPIOProp[1].e_IE != 0xF)
            cmbs_api_ie_GPIOValueAdd(pv_RefIEList, pst_GPIOProp[1].u8_Value);
        if (pst_GPIOProp[2].e_IE != 0xF)
            cmbs_api_ie_GPIOPullTypeAdd(pv_RefIEList, pst_GPIOProp[2].u8_Value);
        if (pst_GPIOProp[3].e_IE != 0xF)
            cmbs_api_ie_GPIOPullEnaAdd(pv_RefIEList, pst_GPIOProp[3].u8_Value);

        cmbs_dsr_GPIOConfigGet(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
        appcmbs_PrepareRecvAdd(1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_CONFIG_GET_RES, &st_Container);
    }

}

void app_SrvExtIntConfigure(PST_IE_GPIO_ID st_GpioId, PST_IE_INT_CONFIGURATION st_Config, u8 u8_IntNumber)
{
    ST_APPCMBS_CONTAINER        st_Container;

    cmbs_dsr_ExtIntConfigure(g_cmbsappl.pv_CMBSRef, st_GpioId, st_Config, u8_IntNumber);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_EXT_INT_CONFIG_RES, &st_Container);
}

void app_SrvExtIntEnable(u8 u8_IntNumber)
{
    ST_APPCMBS_CONTAINER        st_Container;
    cmbs_dsr_ExtIntEnable(g_cmbsappl.pv_CMBSRef, u8_IntNumber);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_EXT_INT_ENABLE_RES, &st_Container);
}

void app_SrvExtIntDisable(u8 u8_IntNumber)
{
    ST_APPCMBS_CONTAINER        st_Container;
    cmbs_dsr_ExtIntDisable(g_cmbsappl.pv_CMBSRef, u8_IntNumber);
    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_EXT_INT_DISABLE_RES, &st_Container);
}

E_CMBS_RC app_SrvLocateSuggest(u16 u16_Handsets)
{
    return cmbs_dsr_LocateSuggestReq(g_cmbsappl.pv_CMBSRef, u16_Handsets);
}

void appOnTerminalCapabilitiesInd(void *pv_List)
{
    UNUSED_PARAMETER(pv_List);
    //TBD: Use u8_EchoParameter parameter to enable/disable echo canceller for calls with this
    //specific HS
}

void appOnPropritaryDataRcvInd(void *pv_List)
{
    UNUSED_PARAMETER(pv_List);
    // TBD: Customer application should handle the proprietary message recieved from the handset
}

void app_SrvEEPROMBackupCreate(void)
{
    char *psz_EepFileName = "EEPROMBackup.bin";
    CMBS_EEPROM_DATA backup;
    ST_APPCMBS_CONTAINER st_Container;

    memset(&backup, 0, sizeof(CMBS_EEPROM_DATA));

    if (tcx_EepNewFile(psz_EepFileName, sizeof(CMBS_EEPROM_DATA)) == 0)
    {
        printf("Can't create new EEPROM file %s size %lu\n", psz_EepFileName, sizeof(CMBS_EEPROM_DATA));
    }
    else
    {
        // CMBS_PARAM_DECT_TYPE
        app_ProductionParamGet(CMBS_PARAM_DECT_TYPE, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_DectType, st_Container.ch_Info, CMBS_PARAM_DECT_TYPE_LENGTH);

        // CMBS_PARAM_RF_FULL_POWER
        app_SrvParamGet(CMBS_PARAM_RF_FULL_POWER, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_RF_FULL_POWER, st_Container.ch_Info,  CMBS_PARAM_RF_FULL_POWER_LENGTH);

        // CMBS_PARAM_RFPI
        app_SrvParamGet(CMBS_PARAM_RFPI, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_RFPI, st_Container.ch_Info, CMBS_PARAM_RFPI_LENGTH);

        // CMBS_PARAM_RXTUN
        app_SrvParamGet(CMBS_PARAM_RXTUN, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_RXTUN, st_Container.ch_Info, CMBS_PARAM_RXTUN_LENGTH);

        //CMBS_PARAM_PREAM_NORM
        app_SrvParamGet(CMBS_PARAM_PREAM_NORM, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_PreamNormal, st_Container.ch_Info, CMBS_PARAM_PREAM_NORM_LENGTH);

        //CMBS_PARAM_RF19APU_SUPPORT_FCC
        app_SrvParamGet(CMBS_PARAM_RF19APU_SUPPORT_FCC, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_RF19APUSupportFCC, st_Container.ch_Info, CMBS_PARAM_RF19APU_SUPPORT_FCC_LENGTH);

        //CMBS_PARAM_RF19APU_DEVIATION
        app_SrvParamGet(CMBS_PARAM_RF19APU_DEVIATION, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_RF19APUDeviation, st_Container.ch_Info, CMBS_PARAM_RF19APU_DEVIATION_LENGTH);

        //CMBS_PARAM_RF19APU_PA2_COMP
        app_SrvParamGet(CMBS_PARAM_RF19APU_PA2_COMP, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_RF19APUPA2Comp, st_Container.ch_Info, CMBS_PARAM_RF19APU_PA2_COMP_LENGTH);

        //CMBS_PARAM_MAX_USABLE_RSSI
        app_SrvParamGet(CMBS_PARAM_MAX_USABLE_RSSI, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_MAXUsableRSSI, st_Container.ch_Info, CMBS_PARAM_MAX_USABLE_RSSI_LENGTH);

        //CMBS_PARAM_LOWER_RSSI_LIMIT
        app_SrvParamGet(CMBS_PARAM_LOWER_RSSI_LIMIT, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_LowerRSSILimit, st_Container.ch_Info, CMBS_PARAM_LOWER_RSSI_LIMIT_LENGTH);

        //CMBS_PARAM_PHS_SCAN_PARAM
        app_SrvParamGet(CMBS_PARAM_PHS_SCAN_PARAM, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_PHSScanParam, st_Container.ch_Info, CMBS_PARAM_PHS_SCAN_PARAM_LENGTH);

        //CMBS_PARAM_JDECT_LEVEL1_M82
        app_SrvParamGet(CMBS_PARAM_JDECT_LEVEL1_M82, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_JDECTLevelM82, st_Container.ch_Info, CMBS_PARAM_JDECT_LEVEL1_M82_LENGTH);

        //CMBS_PARAM_JDECT_LEVEL2_M62
        app_SrvParamGet(CMBS_PARAM_JDECT_LEVEL2_M62, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_JDECTLevelM62, st_Container.ch_Info, CMBS_PARAM_JDECT_LEVEL2_M62_LENGTH);

        //CMBS_PARAM_SUBS_DATA
        app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&backup.m_SUBSDATA, st_Container.ch_Info, CMBS_PARAM_SUBS_DATA_LENGTH);

        if (g_CMBSInstance.u8_HwChip == CMBS_HW_CHIP_VEGAONE)
        {

            app_SrvParamGet(CMBS_PARAM_RVREF, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(&backup.m_RVREF, st_Container.ch_Info, CMBS_PARAM_RVREF_LENGTH);

            app_SrvParamGet(CMBS_PARAM_GFSK, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(&backup.m_GFSK, st_Container.ch_Info, CMBS_PARAM_GFSK_LENGTH);
        }
        backup.m_Initialized = 1;

        // write EEPROM parameters to a file
        tcx_EepOpen(psz_EepFileName);
        tcx_EepWrite((u8 *)&backup, 0, sizeof(CMBS_EEPROM_DATA));
        tcx_EepClose();
    }
}

void app_SrvEEPROMBackupRestore(void)
{

    char *psz_EepFileName = "EEPROMBackup.bin";
    CMBS_EEPROM_DATA st_backup;
    ST_APPCMBS_CONTAINER st_Container;

    printf("Restoring EEPROM from file...\n");
    tcx_EepOpen(psz_EepFileName);

    //Read EEPROM backup values from a file
    tcx_EepRead((u8 *)&st_backup, 0, sizeof(CMBS_EEPROM_DATA));
    if (!st_backup.m_Initialized)
        printf("EEPROM backup file is not initialized!\n");

    tcx_EepClose();

    // CMBS_PARAM_DECT_TYPE
    app_ProductionParamSet(CMBS_PARAM_DECT_TYPE, &st_backup.m_DectType, CMBS_PARAM_DECT_TYPE_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    // CMBS_PARAM_RF_FULL_POWER
    app_SrvParamSet(CMBS_PARAM_RF_FULL_POWER, &st_backup.m_RF_FULL_POWER, CMBS_PARAM_RF_FULL_POWER_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    // CMBS_PARAM_RFPI
    app_SrvParamSet(CMBS_PARAM_RFPI, st_backup.m_RFPI, CMBS_PARAM_RFPI_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    // CMBS_PARAM_RXTUN
    app_SrvParamSet(CMBS_PARAM_RXTUN, st_backup.m_RXTUN, CMBS_PARAM_RXTUN_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_PREAM_NORM
    app_SrvParamSet(CMBS_PARAM_PREAM_NORM, &st_backup.m_PreamNormal, CMBS_PARAM_PREAM_NORM_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_RF19APU_SUPPORT_FCC
    app_SrvParamSet(CMBS_PARAM_RF19APU_SUPPORT_FCC, &st_backup.m_RF19APUSupportFCC, CMBS_PARAM_RF19APU_SUPPORT_FCC_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_RF19APU_DEVIATION
    app_SrvParamSet(CMBS_PARAM_RF19APU_DEVIATION, &st_backup.m_RF19APUDeviation, CMBS_PARAM_RF19APU_DEVIATION_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_RF19APU_PA2_COMP
    app_SrvParamSet(CMBS_PARAM_RF19APU_PA2_COMP, &st_backup.m_RF19APUPA2Comp, CMBS_PARAM_RF19APU_PA2_COMP_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_MAX_USABLE_RSSI
    app_SrvParamSet(CMBS_PARAM_MAX_USABLE_RSSI, &st_backup.m_MAXUsableRSSI, CMBS_PARAM_MAX_USABLE_RSSI_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_LOWER_RSSI_LIMIT
    app_SrvParamSet(CMBS_PARAM_LOWER_RSSI_LIMIT, &st_backup.m_LowerRSSILimit, CMBS_PARAM_LOWER_RSSI_LIMIT_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_PHS_SCAN_PARAM
    app_SrvParamSet(CMBS_PARAM_PHS_SCAN_PARAM, &st_backup.m_PHSScanParam, CMBS_PARAM_PHS_SCAN_PARAM_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_JDECT_LEVEL1_M82
    app_SrvParamSet(CMBS_PARAM_JDECT_LEVEL1_M82, &st_backup.m_JDECTLevelM82, CMBS_PARAM_JDECT_LEVEL1_M82_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    //CMBS_PARAM_JDECT_LEVEL2_M62
    app_SrvParamSet(CMBS_PARAM_JDECT_LEVEL2_M62, &st_backup.m_JDECTLevelM62, CMBS_PARAM_JDECT_LEVEL2_M62_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    // CMBS_PARAM_SUBS_DATA
    app_SrvParamSet(CMBS_PARAM_SUBS_DATA, st_backup.m_SUBSDATA, CMBS_PARAM_SUBS_DATA_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    if (g_CMBSInstance.u8_HwChip == CMBS_HW_CHIP_VEGAONE)
    {
        // CMBS_PARAM_RVREF
        app_SrvParamSet(CMBS_PARAM_RVREF, &st_backup.m_RVREF, CMBS_PARAM_RVREF_LENGTH, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

        // CMBS_PARAM_GFSK
        app_SrvParamSet(CMBS_PARAM_GFSK, &st_backup.m_GFSK, CMBS_PARAM_GFSK_LENGTH, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
    }

}

