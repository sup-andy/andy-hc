/*!
*  \file       appfacility.c
* \brief  handles CAT-iq facilities functionality
* \Author  stein
*
* @(#) %filespec: appfacility.c~10 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*
*******************************************************************************/

#if ! defined ( WIN32 )
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <signal.h>
#endif

#include "cmbs_api.h"
#include "cfr_mssg.h"
#include "appcmbs.h"

extern u16        app_HandsetMap(char * psz_Handsets);

//  ========== app_FacilityMWI  ===========
/*!
        \brief     sending Voice/SMS/Email Message Waiting Indication
        \param[in,out]   psz_Handsets     pointer to parameter string,e.g."1234" or "all"
        \return     <E_CMBS_RC>
*/
E_CMBS_RC app_FacilityMWI(u16 u16_RequestId, u8 u8_LineId, u16 u16_Messages, char * psz_Handsets, E_CMBS_MWI_TYPE eType)
{
    u16 u16_Handsets = app_HandsetMap(psz_Handsets);

    return cmbs_dsr_gen_SendMWI(g_cmbsappl.pv_CMBSRef,
                                u16_RequestId,
                                u8_LineId,
                                u16_Handsets,
                                u16_Messages,
                                eType);
}

E_CMBS_RC app_FacilityMissedCalls(u16 u16_RequestId, u8 u8_LineId, u16 u16_NewMissedCalls, char * psz_Handsets, bool bNewMissedCall, u16 u16_TotalMissedCalls)
{
    u16 u16_Handsets = app_HandsetMap(psz_Handsets);

    return cmbs_dsr_gen_SendMissedCalls(g_cmbsappl.pv_CMBSRef,
                                        u16_RequestId,
                                        u8_LineId,
                                        u16_Handsets,
                                        u16_NewMissedCalls,
                                        bNewMissedCall,
                                        u16_TotalMissedCalls);
}

E_CMBS_RC app_FacilityListChanged(u16 u16_RequestId, u8 u8_ListId, u8 u8_ListEntries, char * psz_Handsets, u8 u8_LineId, u8 u8_LineSubtype)
{
    u16 u16_Handsets = app_HandsetMap(psz_Handsets);

    return cmbs_dsr_gen_SendListChanged(g_cmbsappl.pv_CMBSRef,
                                        u16_RequestId,
                                        u16_Handsets,
                                        u8_ListId,
                                        u8_ListEntries,
                                        u8_LineId,
                                        u8_LineSubtype);
}

E_CMBS_RC         app_FacilityWebContent(u16 u16_RequestId, u8 u8_NumOfWebCont, char * psz_Handsets)
{
    u16            u16_Handsets = app_HandsetMap(psz_Handsets);

    return cmbs_dsr_gen_SendWebContent(g_cmbsappl.pv_CMBSRef,
                                       u16_RequestId,
                                       u16_Handsets,
                                       u8_NumOfWebCont);
}

E_CMBS_RC         app_FacilityPropEvent(u16 u16_RequestId, u16 u16_PropEvent, u8 * pu8_Data, u8 u8_DataLen, char * psz_Handsets)
{
    u16            u16_Handsets = app_HandsetMap(psz_Handsets);

    return cmbs_dsr_gen_SendPropEvent(g_cmbsappl.pv_CMBSRef,
                                      u16_RequestId,
                                      u16_PropEvent,
                                      pu8_Data,
                                      u8_DataLen,
                                      u16_Handsets);
}

E_CMBS_RC         app_FacilityDateTime(u16 u16_RequestId, ST_DATE_TIME * pst_DateTime, char * psz_Handsets)
{
    u16            u16_Handsets = app_HandsetMap(psz_Handsets);

    return cmbs_dsr_time_Update(g_cmbsappl.pv_CMBSRef,
                                u16_RequestId,
                                pst_DateTime,
                                u16_Handsets);
}

//  ========== app_FacilityEntity ===========
/*!
        \brief   CMBS entity to handle response information from target side
        \param[in]  pv_AppRef   application reference
        \param[in]  e_EventID   received CMBS event
        \param[in]  pv_EventData  pointer to IE list
        \return    <int>
*/
int               app_FacilityEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    UNUSED_PARAMETER(pv_AppRef);
    UNUSED_PARAMETER(pv_EventData);

    if (e_EventID == CMBS_EV_DSR_GEN_SEND_MWI_RES         ||
            e_EventID == CMBS_EV_DSR_GEN_SEND_MISSED_CALLS_RES ||
            e_EventID == CMBS_EV_DSR_GEN_SEND_LIST_CHANGED_RES ||
            e_EventID == CMBS_EV_DSR_GEN_SEND_WEB_CONTENT_RES  ||
            e_EventID == CMBS_EV_DSR_GEN_SEND_PROP_EVENT_RES   ||
            e_EventID == CMBS_EV_DSR_TIME_UPDATE_RES)
    {
        return TRUE;
    }
    else
        return FALSE;

}
//*/
