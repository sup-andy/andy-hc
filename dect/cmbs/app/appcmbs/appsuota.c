/*!
*  \file       appsuota.c
* \brief  handles CAT-iq suota functionality
* \Author  stein
*
* @(#) %filespec: appsuota.c~13 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if ! defined ( WIN32 )
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <signal.h>
#endif

#include "cmbs_api.h"
#include "cmbs_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"

#include "gmep-us.h"

// CMBS remove
extern int  appRdFrmGmepdFd;

void app_SuotaDATAReceived(E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    u16                 u16_IE;
    void*               pv_IE;
    t_st_GmepAppMsgCmd  gmepMsgCmd;
    u16 u16_Handset;
    u16 u16_RequestId;
    ST_IE_DATA   st_Data;

    memset(&gmepMsgCmd, 0, sizeof(gmepMsgCmd));
    gmepMsgCmd.cmd = e_EventID;
    gmepMsgCmd.bufLen = 0;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    while (pv_IE)
    {
        switch (u16_IE)
        {
            case  CMBS_IE_SUOTA_APP_ID:
                printf("app_SuotaDATAReceived CMBS_IE_SUOTA_APP_ID \n");
                cmbs_api_ie_u32ValueGet(pv_IE, &gmepMsgCmd.appId, CMBS_IE_SUOTA_APP_ID);
                break;

            case  CMBS_IE_SUOTA_SESSION_ID:
                printf("app_SuotaDATAReceived CMBS_IE_SUOTA_SESSION_ID \n");
                cmbs_api_ie_u32ValueGet(pv_IE, &gmepMsgCmd.appSessionId, CMBS_IE_SUOTA_SESSION_ID);
                break;

            case CMBS_IE_DATA:
                printf("app_SuotaDATAReceived CMBS_IE_DATA \n");
                cmbs_api_ie_DataGet(pv_IE, &st_Data);
                gmepMsgCmd.bufLen = st_Data.u16_DataLen;
                memcpy(gmepMsgCmd.buf, st_Data.pu8_Data, gmepMsgCmd.bufLen);
                break;

            case  CMBS_IE_HANDSETS:
                cmbs_api_ie_HandsetsGet(pv_IE, &u16_Handset);
                memcpy(gmepMsgCmd.buf, &u16_Handset, 2);
                gmepMsgCmd.bufLen = 2;
                printf("app_SuotaDATAReceived CMBS_IE_HANDSETS  hs %d \n", u16_Handset);
                break;

            case CMBS_IE_RESPONSE:
                break;

            case CMBS_IE_REQUEST_ID:
                cmbs_api_ie_RequestIdGet(pv_IE, &u16_RequestId);
                break;


            default:
                printf("app_SuotaDATAReceived Unexpected IE:%d\n \n", u16_IE);
        }

        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }

    printf("app_SuotaDATAReceived e_EventId %d Send to suota application \n", e_EventID);
    //msg_fifo_write(appRdFrmGmepdFd, (char *)&gmepMsgCmd, sizeof(gmepMsgCmd));
    write(appRdFrmGmepdFd, (char *)&gmepMsgCmd, sizeof(gmepMsgCmd));
}

void app_SuotaURLReceived(E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    UNUSED_PARAMETER(e_EventID);
    UNUSED_PARAMETER(pv_EventData);

    printf("appsuota.c app_SuotaURLReceived not implemented \n");
}

void app_SuotaVersionReceived(E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    u16                 u16_IE;
    void*               pv_IE;
    t_st_GmepAppMsgCmd  gmepMsgCmd;
    u16 u16_Handset;
    u16 u16_RequestId;
    int first;

    ST_SUOTA_HS_VERSION_IND st_HSVerInd;
    ST_VERSION_BUFFER       st_SwVersion;
    ST_VERSION_BUFFER       st_HwVersion;
    first = 1;

    memset(&gmepMsgCmd, 0, sizeof(gmepMsgCmd));
    gmepMsgCmd.cmd = e_EventID;
    gmepMsgCmd.bufLen = 0;

    printf("app_SuotaVersionReceived\n");
    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    while (pv_IE)
    {
        switch (u16_IE)
        {
            case  CMBS_IE_HANDSETS:
                cmbs_api_ie_HandsetsGet(pv_IE, &u16_Handset);
                memcpy(gmepMsgCmd.buf, &u16_Handset, 2);
                gmepMsgCmd.bufLen = 2;
                break;

            case  CMBS_IE_HS_VERSION_DETAILS:
                cmbs_api_ie_VersionIndGet(pv_IE, &st_HSVerInd);

                memcpy(&gmepMsgCmd.buf[gmepMsgCmd.bufLen], &st_HSVerInd.u16_EMC, 2);
                gmepMsgCmd.bufLen += 2;

                gmepMsgCmd.buf[gmepMsgCmd.bufLen] = st_HSVerInd.u8_URLStoFollow;
                gmepMsgCmd.bufLen += 1;

                gmepMsgCmd.buf[gmepMsgCmd.bufLen] = st_HSVerInd.u8_FileNumber;
                gmepMsgCmd.bufLen += 1;

                gmepMsgCmd.bufLen += 1; // spare

                gmepMsgCmd.buf[gmepMsgCmd.bufLen] = st_HSVerInd.u8_Flags;
                gmepMsgCmd.bufLen += 1;

                gmepMsgCmd.buf[gmepMsgCmd.bufLen] = st_HSVerInd.u8_Reason;
                gmepMsgCmd.bufLen += 1;

                break;

            case  CMBS_IE_HS_VERSION_BUFFER:
                if (first)
                {
                    cmbs_api_ie_VersionBufferGet(pv_IE, &st_SwVersion);
                    gmepMsgCmd.buf[gmepMsgCmd.bufLen] = st_SwVersion.u8_VerLen;
                    gmepMsgCmd.bufLen += 1;
                    memcpy(&gmepMsgCmd.buf[gmepMsgCmd.bufLen] , st_SwVersion.pu8_VerBuffer , st_SwVersion.u8_VerLen);
                    gmepMsgCmd.bufLen +=  st_SwVersion.u8_VerLen;
                    first = 0;
                }
                else
                {
                    cmbs_api_ie_VersionBufferGet(pv_IE, &st_HwVersion);
                    gmepMsgCmd.buf[gmepMsgCmd.bufLen] = st_HwVersion.u8_VerLen;
                    gmepMsgCmd.bufLen += 1;
                    memcpy(&gmepMsgCmd.buf[gmepMsgCmd.bufLen] , st_HwVersion.pu8_VerBuffer , st_HwVersion.u8_VerLen);
                    gmepMsgCmd.bufLen +=  st_HwVersion.u8_VerLen;
                }

                break;

            case CMBS_IE_REQUEST_ID:
                cmbs_api_ie_RequestIdGet(pv_IE, &u16_RequestId);
                break;


            case CMBS_IE_SUOTA_SESSION_ID:
                cmbs_api_ie_u32ValueGet(pv_IE, &gmepMsgCmd.appSessionId, CMBS_IE_SUOTA_SESSION_ID);
                break;

            default:
                printf("appsuota.c not handled app_SuotaVersionReceived   IE:%d\n \n", u16_IE);

        }

        cmbs_api_ie_GetNext(pv_EventData, &pv_IE, &u16_IE);
    }

    printf("appsuota.c app_SuotaVersionReceived e_EventId %d Send to suota application \n", e_EventID);
    write(appRdFrmGmepdFd, (char *)&gmepMsgCmd, sizeof(gmepMsgCmd));
}

/**
 *
 *
 * @brief
 *  Handles CMBS SUOTA messages -- translates them to the
 *  original format handled by the 823 - for "same" handling
 *  further the road.
 * @param pv_AppRef
 * @param e_EventID
 * @param pv_EventData
 *
 * @return int
 */
int      app_SuotaEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    t_st_GmepAppMsgCmd  gmepMsgCmd;
    ST_IE_DATA   st_Data;

    UNUSED_PARAMETER(pv_AppRef);
    UNUSED_PARAMETER(pv_EventData);

    memset(&gmepMsgCmd, 0, sizeof(t_st_GmepAppMsgCmd));
    memset(&st_Data, 0, sizeof(ST_IE_DATA));

    if (e_EventID == CMBS_EV_DSR_SUOTA_HS_VERSION_RECEIVED)
    {
        app_SuotaVersionReceived(e_EventID, pv_EventData);
        return TRUE;
    }
    else if (e_EventID ==  CMBS_EV_DSR_SUOTA_URL_RECEIVED)
    {
        app_SuotaURLReceived(e_EventID, pv_EventData);
        return TRUE;
    }
    else if (e_EventID == CMBS_EV_DSR_SUOTA_DATA_RECV  ||
             e_EventID == CMBS_EV_DSR_SUOTA_SESSION_CLOSE_ACK ||
             e_EventID == CMBS_EV_DSR_SUOTA_DATA_SEND_ACK  ||
             e_EventID == CMBS_EV_DSR_SUOTA_SESSION_CLOSE ||
             e_EventID == CMBS_EV_DSR_SUOTA_SEND_VERS_AVAIL_RES ||
             e_EventID == CMBS_EV_DSR_SUOTA_SEND_URL_RES        ||
             e_EventID == CMBS_EV_DSR_SUOTA_SESSION_CREATE_ACK)
    {
        app_SuotaDATAReceived(e_EventID, pv_EventData);
        return TRUE;
    }


    //printf("app_SuotaEntity e_EventId %d Was NOT handled here \n",e_EventID);
    return FALSE;

}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_SoutaSendHSVersionAvail(ST_SUOTA_UPGRADE_DETAILS  pHSVerAvail, u16 u16_Handset, ST_VERSION_BUFFER* pst_SwVersion, u16 u16_RequestId)
{
    return cmbs_dsr_suota_SendHSVersionAvail(g_cmbsappl.pv_CMBSRef, pHSVerAvail, u16_Handset, pst_SwVersion, u16_RequestId);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_SoutaSendNewVersionInd(u16 u16_Handset, E_SUOTA_SU_SubType enSubType, u16 u16_RequestId)
{
    return cmbs_dsr_suota_SendSWUpdateInd(g_cmbsappl.pv_CMBSRef, u16_Handset, enSubType, u16_RequestId);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_SoutaSendURL(u16 u16_Handset, u8 u8_URLToFollow, ST_URL_BUFFER* pst_Url, u16 u16_RequestId)
{
    return cmbs_dsr_suota_SendURL(g_cmbsappl.pv_CMBSRef, u16_Handset, u8_URLToFollow, pst_Url, u16_RequestId);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_SoutaSendNack(u16 u16_Handset, E_SUOTA_RejectReason RejectReason, u16 u16_RequestId)
{
    return cmbs_dsr_suota_SendNack(g_cmbsappl.pv_CMBSRef, u16_Handset, RejectReason, u16_RequestId);
}

//*/
