/*!
*  \file        applog.c
* \brief  handles DECT logger functionality
* \Author  podolskyi
*
* @(#) %filespec: applog.c~3 %
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

#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"

#include "appcmbs.h"
#include "appmsgparser.h"

FILE* g_pDectLogFile = NULL;

//////////////////////////////////////////////////////////////////////////
/// externs
//////////////////////////////////////////////////////////////////////////
extern void cmbs_int_ParseDectMsg(u8 * buff, u8 u8_ILen, u8 u8_HandsetNumber);
extern void app_DectLoggerResetOutputBuffer(void);
extern u8* app_DectLoggerGetOutputBuffer(u16* pIndex);
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_LogStartDectLogger(void)
{
    return cmbs_dsr_StartDectLogger(g_cmbsappl.pv_CMBSRef);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_LogStoptDectLoggerAndRead(void)
{
    return cmbs_dsr_StopDectLoggerAndRead(g_cmbsappl.pv_CMBSRef);
}
//////////////////////////////////////////////////////////////////////////
void app_DectLoggerDataInd(void * pv_EventData)
{
    void *         pv_IE = NULL;
    u16            u16_IE;
    ST_IE_DATA  stData = {0, 0};
    u16 u16_Index = 0, u16_PrintIndex = 0, i;
    u8 *pOutBuffer;

    cmbs_api_ie_GetFirst(pv_EventData, &pv_IE, &u16_IE);
    cmbs_api_ie_DataGet(pv_IE, &stData);

    // check necessary conditions for continue processing
    if ((stData.pu8_Data == NULL) || (g_pDectLogFile == NULL))
        return;

    while (u16_Index < stData.u16_DataLen)
    {
        app_DectLoggerResetOutputBuffer();
        cmbs_int_ParseDectMsg(&stData.pu8_Data[u16_Index + 3], stData.pu8_Data[u16_Index], stData.pu8_Data[u16_Index + 1]);

        if (stData.pu8_Data[u16_Index + 2] == 0)
        {
            fprintf(g_pDectLogFile, "\nTX BS --> HS%d ", stData.pu8_Data[u16_Index + 1]);
        }
        else
        {
            fprintf(g_pDectLogFile, "\nRX BS <-- HS%d ", stData.pu8_Data[u16_Index + 1]);
        }

        for (i = 0; i < stData.pu8_Data[u16_Index]; i++)
        {
            fprintf(g_pDectLogFile, "%x ", stData.pu8_Data[u16_Index + 3 + i]);
        }
        fprintf(g_pDectLogFile, "\n");
        fflush(g_pDectLogFile);

        pOutBuffer = app_DectLoggerGetOutputBuffer(&u16_PrintIndex);
        if (g_pDectLogFile)
        {
            fwrite(pOutBuffer, 1, u16_PrintIndex, g_pDectLogFile);
            fflush(g_pDectLogFile);
        }
        u16_Index += (stData.pu8_Data[u16_Index] + 3);
    }

    cmbs_dsr_DectLoggerDataIndRes(g_cmbsappl.pv_CMBSRef);
}
//////////////////////////////////////////////////////////////////////////
void app_DectLoggerStartResp(void * pv_EventData)
{
    E_CMBS_RC retCode = app_ResponseCheck(pv_EventData) ? CMBS_RC_ERROR_GENERAL : CMBS_RC_OK;
    if (CMBS_RC_OK == retCode)
    {
        g_pDectLogFile = fopen("dect_scenario.log", "w");
        if (!g_pDectLogFile)
        {
            printf("\n Can't create output log file \n");
        }
        else
        {
            printf("\n DectLogger started successfully \n");
        }
    }
    else
    {
        printf("\n Error during starting DectLogger, maybe DECT_DBG not defined? ");
    }
}
//////////////////////////////////////////////////////////////////////////
void app_DectLoggerStopResp(void * pv_EventData)
{
    E_CMBS_RC retCode = app_ResponseCheck(pv_EventData) ? CMBS_RC_ERROR_GENERAL : CMBS_RC_OK;
    if (g_pDectLogFile)
    {
        fclose(g_pDectLogFile);
        g_pDectLogFile = NULL;
    }

    if (CMBS_RC_OK == retCode)
    {
        printf("\n DectLogger stopped successfully \n");
    }
    else
    {
        printf("\n Error during stopping DectLogger, maybe DECT_DBG not defined? \n");
    }
}
//////////////////////////////////////////////////////////////////////////
int app_LogEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    UNUSED_PARAMETER(pv_AppRef);

    switch (e_EventID)
    {
        case CMBS_EV_DSR_START_DECT_LOGGER_RES:
            app_DectLoggerStartResp(pv_EventData);
            break;

        case  CMBS_EV_DSR_DECT_DATA_IND:
            app_DectLoggerDataInd(pv_EventData);
            break;

        case CMBS_EV_DSR_STOP_AND_READ_DECT_LOGGER_RES:
            app_DectLoggerStopResp(pv_EventData);
            break;

        default:
            return FALSE;
            break;
    }

    return TRUE;
}