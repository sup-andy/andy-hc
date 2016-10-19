/*!
* \file  appcall.c
* \brief     handle the call control and linked it to media
* \Author  kelbch
*
*  appcall Automat
*  Incoming call: 1. start as normal incoming call
*                 2. start automatically media, if target sent CMBS answer message
*
*  Outgoing call: 1. Receive establish
*                 2. Reply with set-up ack
*                 3. Start dial tone
*                 4. After receiving '#' send ringing
*                 5. User interact for active call, with Media on
*
* @(#) %filespec: appcall.c~NBGD53#77 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================
* date        name     version  action
* ----------------------------------------------------------------------------
* 07-Mar-2014 tcmc_asa  ---GIT--   added NODE audio connection type
* 12-Jun-2013 tcmc_asa      take change of V77 and 77.1.1 form 2.99.9 to 3.x branch
* 28-Jan-2013 tcmc_asa 77      added appcall_CallInbandInfoCNIP() PR 3615
* 24-Jan-2013 tcmc_asa 76      add possibility to switch codec on call answer automatically
* 24-May-2012 tcmc_asa 74      added CMBS_TONE_CALL_WAITING_OUTBAND, PR 3205
* 22-May-2012 tcmc_asa 73      added CLIP type CMBS_ADDR_PROPPLAN_NAT_STD
* 16-Feb-2012 tcmc_asa 66       added use of CLIP AdressProperties from Host (PR 10027)
* 02-Feb-2012 tcmc-asa 65        add CNIP screening indicator
* 08-Dec-2011 tcmc-asa 61        CLIP&CNIP presentation
* 19-sep-09  Kelbch  pj1029-478 add demonstration line handling
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
#else
#include "windows.h"
#endif

#include "cmbs_api.h"
#include "cmbs_int.h"
#include "cmbs_dbg.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appcall.h"
#include "ListsApp.h"
#include "ListChangeNotif.h"

#include "cmbs_voipline.h"
#include "appmsgparser.h"
#include "cmbs_event.h"
#include "appsrv.h"

extern u16   app_HandsetMap(char * psz_Handsets);
extern void  keyb_ReleaseNotify(u16 u16_CallId);
extern u16   _keyb_LineIdInput(void);

extern u8 g_HoldResumeCfm;
extern u8 g_HoldCfm;
extern u8 g_TransferAutoCfm;
extern u8 g_ConfAutoCfm;
extern u8 g_EarlyMediaAutoCfm;

u8 g_u8_ToneType = 0;

E_APPCALL_AUTOMAT_MODE  g_call_automat = E_APPCALL_AUTOMAT_MODE_OFF;

/*! \brief global line object */
ST_LINE_OBJ    g_line_obj[APPCALL_LINEOBJ_MAX];
/*! \brief global call object */
ST_CALL_OBJ    g_call_obj[APPCALL_CALLOBJ_MAX];

/* Slot allocation */
u32 g_u32_UsedSlots = 0;

E_APPCALL_PREFERRED_CODEC g_HostCodec = E_APPCALL_PREFERRED_CODEC_RECEIVED_CODEC;

//  ========== _appcall_AddCallEntry ===========
/*!
        \brief           Add a call entry to call list (new missed call, new outgoing call,  new incoming call)
        \param[in]             tListType - List to insert (missed, incoming, outgoing)
        \param[in]             sNumber -   Number of remote party (CLI)
        \param[in]             u8_LineID - Line ID of call
        \return        void
*/
static void _appcall_AddCallEntry(IN LIST_TYPE tListType, IN const char* sNumber, IN u8 u8_LineID)
{
    stCallsListEntry st_Entry;
    u32 pu32_Fields[5], u32_NumOfFields, u32_EntryId;

    if (sNumber[0] == 0)
    {
        /* we cannot add an entry without number */
        printf("Add Call Entry - FAIL - no number!\n");
        return;
    }

    pu32_Fields[0] = FIELD_ID_NUM_OF_CALLS;
    pu32_Fields[1] = FIELD_ID_DATE_AND_TIME;
    pu32_Fields[2] = FIELD_ID_NUMBER;
    pu32_Fields[3] = FIELD_ID_LINE_ID;

    if (tListType == LIST_TYPE_MISSED_CALLS)
    {
        u32_NumOfFields = 5;
        pu32_Fields[4] = FIELD_ID_READ_STATUS;
        st_Entry.bRead = FALSE;
    }
    else
    {
        u32_NumOfFields = 4;
    }

    st_Entry.u32_NumOfCalls = 1;
    time(&st_Entry.t_DateAndTime);
    strncpy(st_Entry.sNumber, sNumber, LIST_NUMBER_MAX_LEN);
    st_Entry.sNumber[LIST_NUMBER_MAX_LEN - 1] = 0;
    st_Entry.u32_LineId = u8_LineID;

    List_InsertEntry(tListType, &st_Entry, pu32_Fields, u32_NumOfFields, &u32_EntryId);

    cmbsevent_onCallListUpdated();
}

//  ========== _appcall_LineObjGet ===========
/*!
        \brief         return a line object identfied by Line ID
        \param[in]            u16_LineId
        \return    <PST_LINE_OBJ>    line object or NULL
*/
PST_LINE_OBJ   _appcall_LineObjGet(u16 u16_LineId)
{
    if (u16_LineId < APPCALL_LINEOBJ_MAX)
    {
        return  g_line_obj + u16_LineId;
    }
    return NULL;
}

//  ========== _appcall_CallObjGet ===========
/*!
        \brief         return a call object identfied by Call ID
        \param[in]            u16_CallId
        \return    <ST_CALL_OBJ>    call object or NULL
*/
PST_CALL_OBJ   _appcall_CallObjGetById(u16 u16_CallId)
{
    if (u16_CallId < APPCALL_CALLOBJ_MAX)
    {
        return  g_call_obj + u16_CallId;
    }
    return NULL;
}

//  ========== _appcall_LineObjIdGet ===========
/*!
        \brief    return a Line ID of line object
        \param[in]       pst_Line  pointer to line object
        \return    <u16>           Line ID
*/
u16   _appcall_LineObjIdGet(PST_LINE_OBJ   pst_Line)
{
    return (u16)(pst_Line - g_line_obj);
}






//  ========== _appcall_CallObjStateString ===========
/*!
        \brief    return the string of enumeration
        \param[in,out]  e_Call   enumeration value
        \return    <char *>     string value
*/
char *               _appcall_CallObjStateString(E_APPCMBS_CALL e_Call)
{
    switch (e_Call)
    {
            caseretstr(E_APPCMBS_CALL_CLOSE);

            caseretstr(E_APPCMBS_CALL_INC_PEND);
            caseretstr(E_APPCMBS_CALL_INC_RING);

            caseretstr(E_APPCMBS_CALL_OUT_PEND);
            caseretstr(E_APPCMBS_CALL_OUT_PEND_DIAL);
            caseretstr(E_APPCMBS_CALL_OUT_INBAND);
            caseretstr(E_APPCMBS_CALL_OUT_PROC);
            caseretstr(E_APPCMBS_CALL_OUT_RING);

            caseretstr(E_APPCMBS_CALL_ACTIVE);
            caseretstr(E_APPCMBS_CALL_RELEASE);

            caseretstr(E_APPCMBS_CALL_ON_HOLD);
            caseretstr(E_APPCMBS_CALL_CONFERENCE);

        default:
            return (char*)"Call State undefined";
    }

    return NULL;
}

//  ========== _appcall_CallObjMediaCodecString ===========
/*!
        \brief    return the string of enumeration
        \param[in,out]  E_CMBS_AUDIO_CODEC
                                     enumeration value
        \return    <char *>     string value
*/
char *               _appcall_CallObjMediaCodecString(E_CMBS_AUDIO_CODEC e_Codec)
{
    switch (e_Codec)
    {
            caseretstr(CMBS_AUDIO_CODEC_UNDEF);
            caseretstr(CMBS_AUDIO_CODEC_PCMU);
            caseretstr(CMBS_AUDIO_CODEC_PCMA);
            caseretstr(CMBS_AUDIO_CODEC_PCMU_WB);
            caseretstr(CMBS_AUDIO_CODEC_PCMA_WB);
            caseretstr(CMBS_AUDIO_CODEC_PCM_LINEAR_WB);
            caseretstr(CMBS_AUDIO_CODEC_PCM_LINEAR_NB);
            caseretstr(CMBS_AUDIO_CODEC_PCM8);

        default:
            return (char*) "Codec undefined";
    }

    return NULL;
}

//  ========== _appcall_CallObjMediaString ===========
/*!
        \brief    return the string of enumeration
        \param[in,out]  E_APPCMBS_MEDIA
                                     enumeration value
        \return    <char *>     string value
*/
char *               _appcall_CallObjMediaString(E_APPCMBS_MEDIA e_Media)
{
    switch (e_Media)
    {
            caseretstr(E_APPCMBS_MEDIA_CLOSE);
            caseretstr(E_APPCMBS_MEDIA_PEND);
            caseretstr(E_APPCMBS_MEDIA_ACTIVE);

        default:
            return (char*)"Media State undefined";
    }

    return NULL;
}

//  ==========  appcall_InfoCall ===========
/*!
        \brief     print the current call information

        \param[in]   n_Call  Identifier of call

        \return     <none>
*/
void appcall_InfoCall(u16 u16_CallId)
{
    if (g_call_obj[u16_CallId].u32_CallInstance)
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO CALL_ID %u\n", u16_CallId));

        if (g_call_obj[u16_CallId].ch_CallerID[0])
        {
            APPCMBS_INFO(("APP_Dongle-CALL: INFO CALLER ID %s\n", g_call_obj[u16_CallId].ch_CallerID));
        }
        if (g_call_obj[u16_CallId].ch_CalledID[0])
        {
            APPCMBS_INFO(("APP_Dongle-CALL: INFO CALLED ID %s\n", g_call_obj[u16_CallId].ch_CalledID));
        }
        APPCMBS_INFO(("APP_Dongle-CALL: INFO Call State  %s\n", _appcall_CallObjStateString(g_call_obj[u16_CallId].e_Call)));
        APPCMBS_INFO(("APP_Dongle-CALL: INFO Media State %s\n", _appcall_CallObjMediaString(g_call_obj[u16_CallId].e_Media)));
        APPCMBS_INFO(("APP_Dongle-CALL: INFO Codec %s\n", _appcall_CallObjMediaCodecString(g_call_obj[u16_CallId].e_Codec)));
        APPCMBS_INFO(("APP_Dongle-CALL: INFO CallInstance %u = 0x%08X\n", g_call_obj[u16_CallId].u32_CallInstance, g_call_obj[u16_CallId].u32_CallInstance));
        APPCMBS_INFO(("APP_Dongle-CALL: INFO LineID %u\n", g_call_obj[u16_CallId].u8_LineId));
        APPCMBS_INFO(("APP_Dongle-CALL: INFO Media Channel ID %u\n", g_call_obj[u16_CallId].u32_ChannelID));
        APPCMBS_INFO(("APP_Dongle-CALL: INFO Media Slots 0x%X\n", g_call_obj[u16_CallId].u32_ChannelParameter));
    }
    else
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO CALL_ID %d is free\n", u16_CallId));
    }
}

//  ========== appcall_InfoPrint ===========
/*
        \brief    print the current call objects information
        \param[in,out]  <none>
        \return    <none>

*/
void          appcall_InfoPrint(void)
{
    u16  i;

    if (g_call_automat)
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO - - - AUTOMAT ON - - -\n"));
    }
    else
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO - - - AUTOMAT OFF - - -\n"));
    }
    printf("       Auto Transfer = %d, Auto Conf = %d,\n       Auto Hold = %d, Auto Resume = %d, Auto EarlyMedia = %d\n",
           g_TransferAutoCfm, g_ConfAutoCfm, g_HoldCfm, g_HoldResumeCfm, g_EarlyMediaAutoCfm);
    printf("       Host Codec = %s\n", (g_HostCodec == E_APPCALL_PREFERRED_CODEC_RECEIVED_CODEC) ? "use received codec" : (g_HostCodec == E_APPCALL_PREFERRED_CODEC_WB) ? "WB" : "NB");
    printf("-================================================-\n\n");

    for (i = 0; i < APPCALL_CALLOBJ_MAX; i++)
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO ----------------------\n"));
        appcall_InfoCall(i);
    }
}

//  ========== appcall_Initialize  ===========
/*!
        \brief    Initialize the call/line management
        \param[in,out]  <none>
        \return    <none>
*/
void          appcall_Initialize(void)
{
    u16  i;

    memset(&g_line_obj, 0, sizeof(g_line_obj));
//   for (i=0; i< APPCALL_LINEOBJ_MAX; i++ )
//   {
//      g_line_obj[i]. = ;   //TODO  additional initialization for line
//   }

    memset(&g_call_obj, 0, sizeof(g_call_obj));
    for (i = 0; i < APPCALL_CALLOBJ_MAX; i++)
    {
        g_call_obj[i].st_CallerParty.pu8_Address = (u8*)g_call_obj[i].ch_CallerID;
        g_call_obj[i].st_CalledParty.pu8_Address = (u8*)g_call_obj[i].ch_CalledID;
        g_call_obj[i].u8_LineId = APPCALL_NO_LINE;
    }
}


//  ========== _appcall_CallObjIdGet ===========
/*!
        \brief    return a Call ID of call object
        \param[in]       pst_Call  pointer to call object
        \return    <u16>           Call ID
*/
u16   _appcall_CallObjIdGet(PST_CALL_OBJ   pst_Call)
{
    return (u16)(pst_Call - g_call_obj);
}

//  ========== _appcall_CallObjGet ===========
/*!
        \brief         return a call object identfied by call instance or caller party
        \param[in]            u32_CallInstance
        \param[in]       psz_CLI
        \return    <PST_CALL_OBJ>    if no free call object available return NULL
*/
PST_CALL_OBJ   _appcall_CallObjGet(u32 u32_CallInstance, char * psz_CLI)
{
    u16 i;

    for (i = 0; i < APPCALL_CALLOBJ_MAX; i++)
    {
        if (u32_CallInstance && (g_call_obj[i].u32_CallInstance == u32_CallInstance))
            return  g_call_obj + i;

        if (psz_CLI && !strcmp(g_call_obj[i].ch_CallerID, psz_CLI))
            return  g_call_obj + i;
    }

    return NULL;
}

//  ========== _appcall_CallObjNew ===========
/*!
        \brief    get a free call object
        \param[in,out]  <none>
        \return    <PST_CALL_OBJ>    if no free call object available return NULL
*/
PST_CALL_OBJ         _appcall_CallObjNew(void)
{
    u16 i;

    for (i = 0; i < APPCALL_CALLOBJ_MAX; i++)
    {
        if (!g_call_obj[i].u32_CallInstance)
            return  g_call_obj + i;
    }

    return NULL;
}

//  ========== _appcall_CallObjDelete ===========
/*!
        \brief    delete a call object identfied by call instance or caller party
        \param[in,out]  u32_CallInstance
        \param[in,out]  psz_CLI
        \return    <none>
*/
void                 _appcall_CallObjDelete(u32 u32_CallInstance, char * psz_CLI)
{
    PST_CALL_OBJ pst_Call;

    pst_Call = _appcall_CallObjGet(u32_CallInstance, psz_CLI);

    if (pst_Call)
    {
        memset(pst_Call, 0, sizeof(ST_CALL_OBJ));
        pst_Call->st_CalledParty.pu8_Address = (u8*)pst_Call->ch_CalledID;
        pst_Call->st_CallerParty.pu8_Address = (u8*)pst_Call->ch_CallerID;
        pst_Call->u8_LineId = APPCALL_NO_LINE;
    }
}

//  ========== _appcall_CallObjLineObjGet ===========
/*!
        \brief   return the line object of call object

        \param[in]  pst_Call  pointer to call object

        \return   <PST_LINE_OBJ>          pointer to line object  or NULL

*/
PST_LINE_OBJ          _appcall_CallObjLineObjGet(PST_CALL_OBJ   pst_Call)
{
    if (pst_Call->u8_LineId != APPCALL_NO_LINE)
    {
        return  g_line_obj + pst_Call->u8_LineId;
    }
    return  NULL;
}

//  ========== _appcall_CallObjLineObjSet ===========
/*!
        \brief   set line object for call object

        \param[in]  pst_Call  pointer to call object
        \param[in]  pst_Line  pointer to line object

        \return   <none>

*/
void                  _appcall_CallObjLineObjSet(PST_CALL_OBJ   pst_Call, PST_LINE_OBJ  pst_Line)
{
    if (pst_Line)
    {
        pst_Call->u8_LineId = (u8)(pst_Line - g_line_obj);
    }
}

//  ========== _appcall_CallObjStateSet  ===========
/*!
        \brief   set call object call state
        \param[in]  pst_Call  pointer to call object
        \param[in]  e_State  call state
        \return   <none>
*/
void                 _appcall_CallObjStateSet(PST_CALL_OBJ pst_Call, E_APPCMBS_CALL e_State)
{
    pst_Call->e_Call = e_State;
}

//  ========== _appcall_CallObjStateGet  ===========
/*!
        \brief   set call object call state
        \param[in]  pst_Call   pointer to call object
        \return   <E_APPCMBS_CALL> call state of this call object
*/
E_APPCMBS_CALL       _appcall_CallObjStateGet(PST_CALL_OBJ pst_Call)
{
    return pst_Call->e_Call;
}

//  ========== _appcall_CallObjMediaSet  ===========
/*!
        \brief   set call object media state
        \param[in]  pst_Call  pointer to call object
        \param[in]  e_State  media state
        \return   <none>
*/
void                 _appcall_CallObjMediaSet(PST_CALL_OBJ pst_Call, E_APPCMBS_MEDIA e_State)
{
    pst_Call->e_Media = e_State;
}

//  ========== _appcall_CallObjMediaGet  ===========
/*!
        \brief   return call object media state
        \param[in]  pst_Call   pointer to call object
        \return   <E_APPCMBS_MEDIA>  media state of this call object
*/
E_APPCMBS_MEDIA       _appcall_CallObjMediaGet(PST_CALL_OBJ pst_Call)
{
    return pst_Call->e_Media;
}



int _appcall_CallObjDigitCollectorEndSymbolCheck(PST_CALL_OBJ pst_Call)
{
    int   i;

    for (i = 0; i < pst_Call->st_CalledParty.u8_AddressLen; i ++)
    {
        if (pst_Call->st_CalledParty.pu8_Address[i] == '#')
        {
            return TRUE;
        }
    }

    return FALSE;

}


//  ========== _appcall_PropertiesIDXGet ===========
/*!
        \brief    find index of IE in exchange object
        \param[in]   pst_Properties  pointer to exchange object
        \param[in]   n_Properties   number of contained IEs
        \param[in]   e_IE            to be find IE

        \return     <int>              index of the IE

*/

int   _appcall_PropertiesIDXGet(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, E_CMBS_IE_TYPE e_IE)
{
    int   i;

    for (i = 0; i < n_Properties; i++)
    {
        if (pst_Properties[i].e_IE == e_IE)
            break;
    }

    return i;
}
//  ========== _appmedia_CallObjMediaPropertySet  ===========
/*!
        \brief   short description
        \param[in]  pst_Call   pointer to call object
        \param[in]  u16_IE   current IE
        \param[in,out] pst_IEInfo  pointer to IE info object
        \return   <int>        return TRUE, if IEs were consumed
*/

int         _appmedia_CallObjMediaPropertySet(PST_CALL_OBJ pst_Call, u16 u16_IE, PST_APPCMBS_IEINFO pst_IEInfo)
{
    if (! pst_Call)
        return FALSE;

    switch (u16_IE)
    {
        case CMBS_IE_MEDIADESCRIPTOR:
			/*
            // printf(" !!!! MEDIADESCRIPTOR: ");
            if ((pst_Call->b_Incoming) && (g_HostCodec == E_APPCALL_SWITCH_RECEIVED_CODEC))
            {
                if (pst_IEInfo->Info.st_MediaDesc.e_Codec == CMBS_AUDIO_CODEC_PCM_LINEAR_WB)
                {
                    pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_NB;
                    //    printf(" NB \n");
                }
                else
                {
                    pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;
                    //      printf(" WB \n");
                }
                pst_Call->pu8_CodecsList[0] =  pst_Call->e_Codec;
                pst_Call->u8_CodecsLength = 1;
            }
            else if ((pst_Call->b_Incoming) || (g_HostCodec == E_APPCALL_PREFERRED_CODEC_RECEIVED_CODEC))
            {
                pst_Call->e_Codec = pst_IEInfo->Info.st_MediaDesc.e_Codec;
                memcpy(&pst_Call->pu8_CodecsList, &pst_IEInfo->Info.st_MediaDesc.pu8_CodecsList, pst_IEInfo->Info.st_MediaDesc.u8_CodecsLength);
                pst_Call->u8_CodecsLength = pst_IEInfo->Info.st_MediaDesc.u8_CodecsLength;
            }
            else
            {
                if (g_HostCodec == E_APPCALL_PREFERRED_CODEC_NB)
                {
                    pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_NB;
                }
                else
                {
                    pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;
                }
                pst_Call->pu8_CodecsList[0] =  pst_Call->e_Codec;
                pst_Call->u8_CodecsLength = 1;
            }
			*/
		    //pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;
			//pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCMU;

			
			pst_Call->e_Codec = pst_IEInfo->Info.st_MediaDesc.e_Codec;
			if (pst_Call->e_Codec != CMBS_AUDIO_CODEC_PCM_LINEAR_WB)
				pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCMU;
			memcpy(&pst_Call->pu8_CodecsList, &pst_IEInfo->Info.st_MediaDesc.pu8_CodecsList, pst_IEInfo->Info.st_MediaDesc.u8_CodecsLength);
			pst_Call->u8_CodecsLength = pst_IEInfo->Info.st_MediaDesc.u8_CodecsLength;
/*
			pst_Call->e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_NB;
            pst_Call->pu8_CodecsList[0] =  pst_Call->e_Codec;
            pst_Call->u8_CodecsLength = 1;
 */           
            return TRUE;

        case CMBS_IE_MEDIACHANNEL:
            pst_Call->u32_ChannelID = pst_IEInfo->Info.st_MediaChannel.u32_ChannelID;
            _appcall_CallObjMediaSet(pst_Call, E_APPCMBS_MEDIA_PEND);
            return TRUE;
    }

    return FALSE;
}

//  ========== _appcall_CheckBackwardCompatibilityLineSelection ===========
/*!
        \brief           Checks if the HS is using CAT-iq 2.0 backward compatibility line selection (using keypad '#')
        \param[in]             pst_CallObjObj - pointer to Call object structure
        \return        TRUE if line selection  identified, FALSE o/w
*/
static bool _appcall_CheckBackwardCompatibilityLineSelection(IN PST_CALL_OBJ pst_CallObj)
{
    if (pst_CallObj->e_Call < E_APPCMBS_CALL_ACTIVE)
    {
        if ((pst_CallObj->st_CalledParty.pu8_Address[0] == '#') && (pst_CallObj->st_CalledParty.u8_AddressLen >= 2))
        {
            if (pst_CallObj->u8_LineId < APPCALL_LINEOBJ_MAX || pst_CallObj->u8_LineId == APPCALL_NO_LINE)
            {
                pst_CallObj->u8_LineId = pst_CallObj->st_CalledParty.pu8_Address[1] - '0';

                _appcall_CallObjLineObjSet(pst_CallObj, _appcall_LineObjGet(pst_CallObj->u8_LineId));

                return TRUE;
            }
            else
            {
                printf("\n _appcall_CheckBackwardCompatibilityLineSelection - unknown line id - treating as digits \n");
            }
        }
    }

    return FALSE;
}

//  ========== appmedia_CallObjMediaStop ===========
/*!
        \brief    stop the media channel identified by Call Instance, Call ID or Caller ID
        \param[in,out]  u32_CallInstance  if not used zero
        \param[in,out]  u16_CallId        Call ID used, if psz_CLI is NULL
        \param[in,out]  psz_CLI        pointer to Caller ID, if not needed NULL
        \return    <none>
*/
void        appmedia_CallObjMediaStop(u32 u32_CallInstance, u16 u16_CallId, char* psz_CLI)
{
    PST_CALL_OBJ   pst_Call;
    void *         pv_RefIEList = NULL;

    if (u32_CallInstance)
    {
        pst_Call = _appcall_CallObjGet(u32_CallInstance, NULL);
    }
    else
    {
        if (! psz_CLI)
        {
            // Call ID is available
            pst_Call = g_call_obj + u16_CallId;
        }
        else
        {
            pst_Call = _appcall_CallObjGet(0, psz_CLI);
        }
    }

    if (pst_Call && (E_APPCMBS_MEDIA_ACTIVE == _appcall_CallObjMediaGet(pst_Call)))
    {
        pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            ST_IE_MEDIA_CHANNEL  st_MediaChannel;

            memset(&st_MediaChannel, 0, sizeof(ST_IE_MEDIA_CHANNEL));
            st_MediaChannel.e_Type        = CMBS_MEDIA_TYPE_AUDIO_IOM;
            st_MediaChannel.u32_ChannelID = pst_Call->u32_ChannelID;

            cmbs_api_ie_MediaChannelAdd(pv_RefIEList, &st_MediaChannel);
            cmbs_dem_ChannelStop(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
            _appcall_CallObjMediaSet(pst_Call, E_APPCMBS_MEDIA_PEND);
        }

        g_u32_UsedSlots &= ~(pst_Call->u32_ChannelParameter);
    }
}

//  ========== appmedia_CallObjMediaStart ===========
/*!
        \brief    start the media channel identified by Call Instance, Call ID or Caller ID
        \param[in,out]  u32_CallInstance    if not used zero
        \param[in,out]  u16_CallId      Call ID used, if psz_CLI is NULL
        \param[in,out]  u16_StartSlot       Start Slot, of 0xFF for auto
        \param[in,out]  psz_CLI          pointer to Caller ID, if not needed NULL
        \return    <none>
*/
void appmedia_CallObjMediaStart(u32 u32_CallInstance, u16 u16_CallId, u16 u16_StartSlot, char * psz_CLI)
{
    PST_CALL_OBJ   pst_Call;
    void *         pv_RefIEList = NULL;

    if (u32_CallInstance)
    {
        pst_Call = _appcall_CallObjGet(u32_CallInstance, NULL);
    }
    else
    {
        if (! psz_CLI)
        {
            // Call ID is available
            pst_Call = g_call_obj + u16_CallId;
        }
        else
        {
            pst_Call = _appcall_CallObjGet(0, psz_CLI);
        }
    }
	printf("appmedia_CallObjMediaStart pst_Call = 0x%lx, status = %d, status2 = %d\n", pst_Call, _appcall_CallObjMediaGet(pst_Call), _appcall_CallObjStateGet(pst_Call));
    if (pst_Call && (E_APPCMBS_MEDIA_PEND == _appcall_CallObjMediaGet(pst_Call)))
    {
        pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            ST_IE_MEDIA_CHANNEL  st_MediaChannel;
            u32                   u32_Slots = 0;

            memset(&st_MediaChannel, 0, sizeof(ST_IE_MEDIA_CHANNEL));
            st_MediaChannel.e_Type               = CMBS_MEDIA_TYPE_AUDIO_IOM;

            // ASA  internal call testing via node !!! */
            if (g_u8_ToneType)
            {
                st_MediaChannel.e_Type = CMBS_MEDIA_TYPE_AUDIO_NODE;
            }

            st_MediaChannel.u32_ChannelID        = pst_Call->u32_ChannelID;
			printf("appmedia_CallObjMediaStart pst_Call->e_Codec = %d\n", pst_Call->e_Codec);

            switch (pst_Call->e_Codec)
            {
                case CMBS_AUDIO_CODEC_PCMU:
                case CMBS_AUDIO_CODEC_PCMA:
                case CMBS_AUDIO_CODEC_PCM8:
                    u32_Slots = 0x01;
                    break;

                case CMBS_AUDIO_CODEC_PCMU_WB:
                case CMBS_AUDIO_CODEC_PCMA_WB:
                case CMBS_AUDIO_CODEC_PCM_LINEAR_NB:
                    u32_Slots = 0x03;
                    break;

                case CMBS_AUDIO_CODEC_PCM_LINEAR_WB:
                    u32_Slots = 0x0F;
                    break;

                default:
                    printf("appmedia_CallObjMediaStart - Unsupported codec!\n");
                    return;
            }
			u16_StartSlot = (8 * pst_Call->u8_LineId);
			printf("u16_StartSlot = %d, pst_Call->u8_LineId = %d\n", u16_StartSlot, pst_Call->u8_LineId);
            if (0xFF == u16_StartSlot)
            {
                /* auto slot delection (out of 12 available slots) */
                int i, SlotMaxOffset;
                switch (u32_Slots)
                {
                    case 0x01:
                        SlotMaxOffset = 31;
                        break;

                    case 0x03:
                        SlotMaxOffset = 30;
                        break;

                    default:
                        SlotMaxOffset = 28;
                        break;
                }
                for (i = 0; i <= SlotMaxOffset; ++i)
                {
                    if (((u32_Slots << i) & g_u32_UsedSlots) == 0)
                    {
                        break;
                    }
                }

                if (i <= SlotMaxOffset)
                {
                    /* available slots exist */
                    st_MediaChannel.u32_ChannelParameter = u32_Slots << i;
                    g_u32_UsedSlots |= u32_Slots << i;
                }
                else
                {
                    /* no available slots */
                    APPCMBS_WARN(("\n\nappmedia_CallObjMediaStart - No free slots - aborting media start\n\n"));
                    return;
                }
            }
            else
            {
                /* manual slot selection */
                st_MediaChannel.u32_ChannelParameter = u32_Slots << (u16_StartSlot);
            }
			printf("appmedia_CallObjMediaStart u32_ChannelParameter = 0x%lx\n", st_MediaChannel.u32_ChannelParameter);

            pst_Call->u32_ChannelParameter = st_MediaChannel.u32_ChannelParameter;

            cmbs_api_ie_MediaChannelAdd(pv_RefIEList, &st_MediaChannel);
            printf("\nCall cmbs_dem_ChannelStart, Channel=%d, start Slot=%d, ChannelInfo=%X, type=%d\n",
                   pst_Call->u32_ChannelID,
                   u16_StartSlot,
                   st_MediaChannel.u32_ChannelParameter ,
                   st_MediaChannel.e_Type
                  );
            cmbs_dem_ChannelStart(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
            _appcall_CallObjMediaSet(pst_Call, E_APPCMBS_MEDIA_ACTIVE);
        }

    }
}


void        appmedia_CallObjMediaInternalConnect(int channel, int context, int connect)
{
    ST_IE_MEDIA_INTERNAL_CONNECT st_MediaIC;
    void *         pv_RefIEList = NULL;

    pv_RefIEList = cmbs_api_ie_GetList();

    if (pv_RefIEList)
    {
        st_MediaIC.e_Type = connect;
        st_MediaIC.u32_ChannelID = channel;
        st_MediaIC.u32_NodeId = context;

        // Add call Instance IE
        cmbs_api_ie_MediaICAdd(pv_RefIEList, &st_MediaIC);

        cmbs_dem_ChannelInternalConnect(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
    }
}

void appmedia_CallObjMediaOffer(u16 u16_CallId, char ch_Audio)
{
    PST_CALL_OBJ            pst_Call = NULL;
    ST_IE_MEDIA_DESCRIPTOR  st_MediaDesc;
    ST_IE_NB_CODEC_OTA      st_OtaCodec;
    void *                  pv_RefIEList = NULL;

    pst_Call = g_call_obj + u16_CallId;

    if (pst_Call)
    {
    	if (_appcall_CallObjMediaGet(pst_Call) == E_APPCMBS_MEDIA_ACTIVE)
            appmedia_CallObjMediaStop(pst_Call->u32_CallInstance, 0, 0);

        pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            // add media descriptor IE
            memset(&st_MediaDesc, 0, sizeof(ST_IE_MEDIA_DESCRIPTOR));

            switch (ch_Audio)
            {
                case 'w':
                    st_MediaDesc.e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;
                    break;

                case 'n':
                case 'g':
                    st_MediaDesc.e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_NB;
                    break;

                case 'a':
                    st_MediaDesc.e_Codec = CMBS_AUDIO_CODEC_PCMA;
                    break;

                case 'u':
                    st_MediaDesc.e_Codec = CMBS_AUDIO_CODEC_PCMU;
                    break;

                case '8':
                    st_MediaDesc.e_Codec = CMBS_AUDIO_CODEC_PCM8;
                    break;

                default:
                    printf("Error - Unknown codec %c %s %d, codec = %d\n", ch_Audio, __FILE__, __LINE__, st_MediaDesc.e_Codec);
                    return;
            }

            cmbs_api_ie_MediaDescAdd(pv_RefIEList, &st_MediaDesc);

            if (ch_Audio == 'g')
            {
                // This is for testing G.711 OTA
                st_OtaCodec.e_Codec = CMBS_NB_CODEC_OTA_G711A;
                cmbs_api_ie_NBOTACodecAdd(pv_RefIEList, &st_OtaCodec);
            }

            printf("\nCall appmedia_CallObjMediaOffer, Channel=%d, Codec %d\n", pst_Call->u32_ChannelID, st_MediaDesc.e_Codec);

            cmbs_dee_CallMediaOffer(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
        }
    }
}

//  ========== appmedia_CallObjTonePlay ===========
/*!
        \brief    play tone on the media channel identified by Call ID or Caller ID
      \param[in]        psz_Value         pointer to CMBS tone enumeration string
      \param[in]        bo_On             TRUE to play, FALSE to stop
        \param[in,out]  u16_CallId         Call ID used, if psz_CLI is NULL
        \param[in,out]  psz_CLI        pointer to caller ID,if not needed NULL
        \return    <none>
*/
void        appmedia_CallObjTonePlay(char * psz_Value, int bo_On, u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ   pst_Call;
    printf("\n %s line:%d appmedia_CallObjTonePlay, %s\ tonen %s\n", __FUNCTION__, __LINE__, bo_On ? "play" : "stop", psz_Value);
	if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }

    if (pst_Call)
    {
        void *         pv_RefIEList = NULL;
		printf("appmedia_CallObjTonePlay pst_Call = 0x%lx, status = %d, status2 = %d\n", pst_Call, _appcall_CallObjMediaGet(pst_Call), _appcall_CallObjStateGet(pst_Call));
		printf("bo_On = %d\n", bo_On);
		//bo_On |= 0x80;
        if (_appcall_CallObjMediaGet(pst_Call) == E_APPCMBS_MEDIA_PEND ||
                _appcall_CallObjMediaGet(pst_Call) == E_APPCMBS_MEDIA_ACTIVE)
        {
			
            pv_RefIEList = cmbs_api_ie_GetList();

            if (pv_RefIEList)
            {
                ST_IE_MEDIA_CHANNEL  st_MediaChannel;

                memset(&st_MediaChannel, 0, sizeof(ST_IE_MEDIA_CHANNEL));
                if (bo_On & 0x80)
                {
                    st_MediaChannel.e_Type        = CMBS_MEDIA_TYPE_AUDIO_NODE;
                }
                else
                {
                    st_MediaChannel.e_Type        = CMBS_MEDIA_TYPE_AUDIO_IOM;
                }

                // reset bo_On
                bo_On &= ~0x0080;

                st_MediaChannel.u32_ChannelID = pst_Call->u32_ChannelID;
                st_MediaChannel.u32_ChannelParameter = pst_Call->u32_ChannelParameter;

                cmbs_api_ie_MediaChannelAdd(pv_RefIEList, &st_MediaChannel);

                if (!bo_On)
                {
                    cmbs_dem_ToneStop(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
                }
                else
                {
                    ST_IE_TONE  st_Tone;

                    memset(&st_Tone, 0, sizeof(ST_IE_TONE));
                    st_Tone.e_Tone = cmbs_dbg_String2E_CMBS_TONE(psz_Value);
                    cmbs_api_ie_ToneAdd(pv_RefIEList, &st_Tone);

                    cmbs_dem_ToneStart(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
                }
            }
        }
        else if (cmbs_dbg_String2E_CMBS_TONE(psz_Value) == CMBS_TONE_CALL_WAITING_OUTBAND)
        {
            // no media channgel connected fo the call waiting,
            // but tone sent outband (<<SIGNAl>> in {CC-INFO})
            pv_RefIEList = cmbs_api_ie_GetList();

            if (pv_RefIEList)
            {
                ST_IE_TONE  st_Tone;
                ST_IE_MEDIA_CHANNEL  st_MediaChannel;

                memset(&st_MediaChannel, 0, sizeof(ST_IE_MEDIA_CHANNEL));
                st_MediaChannel.e_Type        = CMBS_MEDIA_TYPE_AUDIO_IOM;
                st_MediaChannel.u32_ChannelID = u16_CallId;

                cmbs_api_ie_MediaChannelAdd(pv_RefIEList, &st_MediaChannel);
                memset(&st_Tone, 0, sizeof(ST_IE_TONE));
                st_Tone.e_Tone = CMBS_TONE_CALL_WAITING_OUTBAND;
                cmbs_api_ie_ToneAdd(pv_RefIEList, &st_Tone);

                cmbs_dem_ToneStart(g_cmbsappl.pv_CMBSRef, pv_RefIEList);
            }
        }
    }
}

void        _appDTMFPlay(PST_CALL_OBJ pst_Call, u8* pu8_Tone, u8 u8_Len)
{
    int i, e;
    char  ch_Tone[30] = {0};
    // Tone info send
    for (i = 0; i < u8_Len; i++)
    {
        if (pu8_Tone[i] == '*')
        {
            e = CMBS_TONE_DTMF_STAR;
        }
        else if (pu8_Tone[i] == '#')
        {
            e = CMBS_TONE_DTMF_HASH;

        }
        else if (pu8_Tone[i] >= 0x30 && pu8_Tone[i] <= 0x39)
        {
            e = (pu8_Tone[i] - 0x30) + CMBS_TONE_DTMF_0;
        }
        else
        {
            e = (pu8_Tone[i] - 'a') + CMBS_TONE_DTMF_A;
        }
        strncpy(ch_Tone, cmbs_dbg_GetToneName(e), (sizeof(ch_Tone) - 1));

        appmedia_CallObjTonePlay(ch_Tone, TRUE, _appcall_CallObjIdGet(pst_Call), NULL);

        SleepMs(200);          // wait 0.2 seconds
    }

}


/***************************************************************************
*
*     Old function for tracking
*
****************************************************************************/

//  ========== app_OnCallEstablish ===========
/*!
        \brief    CMBS Target wants to make an outgoing call
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void app_OnCallEstablish(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ            pst_Call = NULL;
    PST_LINE_OBJ            pst_Line = NULL;
    ST_APPCMBS_IEINFO       st_IEInfo;
    ST_APPCALL_PROPERTIES   st_Properties;
    void *                  pv_IE = NULL;
    u16                     u16_IE;
    u16                     u16_CallId;
    ST_IE_RESPONSE          st_Response;

    st_Response.e_Response = CMBS_RESPONSE_OK;

    UNUSED_PARAMETER(pvAppRefHandle);
    printf("%s\n", __FUNCTION__);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE, CMBS_IE_CALLERPARTY, CMBS_IE_CALLEDPARTY (optional) , CMBS_IE_MEDIADESCRIPTOR, CMBS_IE_LINE_ID (optional)

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    pst_Call = _appcall_CallObjNew();
                    if (pst_Call)
                    {
                        pst_Call->u32_CallInstance = st_IEInfo.Info.u32_CallInstance;
                    }
                    break;

                case CMBS_IE_LINE_ID:
                    if (pst_Call)
                    {
                        //printf("app_OnCallEstablish: HS chose LineID %u\n", st_IEInfo.Info.u8_LineId);

                        //pst_Line = _appcall_LineObjGet(st_IEInfo.Info.u8_LineId);
                        //_appcall_CallObjLineObjSet(pst_Call, pst_Line);
                    }
                    break;
                case CMBS_IE_CALLERPARTY:
                    //printf("app_OnCallEstablish: CALLER_ID address properties:0x%02x\n", st_IEInfo.Info.st_CallerParty.u8_AddressProperties);
                    if (pst_Call)
                    {
                        int i;
                        printf ( "CallerID length:%d:", st_IEInfo.Info.st_CallerParty.u8_AddressLen );
                        for ( i=0; i < st_IEInfo.Info.st_CallerParty.u8_AddressLen; i ++ )
                            printf (" 0x%02x", st_IEInfo.Info.st_CallerParty.pu8_Address[i] );
                        printf( "\n" );

                        memcpy(pst_Call->ch_CallerID, st_IEInfo.Info.st_CallerParty.pu8_Address, st_IEInfo.Info.st_CallerParty.u8_AddressLen);
                        memcpy(&pst_Call->st_CallerParty, &st_IEInfo.Info.st_CallerParty, sizeof(pst_Call->st_CallerParty));
                        pst_Call->st_CallerParty.pu8_Address = (u8*)pst_Call->ch_CallerID;
                        pst_Call->ch_CallerID[ pst_Call->st_CallerParty.u8_AddressLen ] = '\0';
						
						if (strstr(VOIP_LINE1_HSSTR, pst_Call->ch_CallerID) != NULL) {
							pst_Call->u8_LineId = 0;
						} else if (strstr(VOIP_LINE2_HSSTR, pst_Call->ch_CallerID) != NULL) {
						    pst_Call->u8_LineId = 1;
						}
						printf("Line = %d\n", pst_Call->u8_LineId);
						
                    }
                    break;

                case CMBS_IE_CALLEDPARTY:
                    //printf("app_OnCallEstablish: CALLED_ID address properties:0x%02x\n", st_IEInfo.Info.st_CalledParty.u8_AddressProperties);
                    if (pst_Call)
                    {
                        /*int i;
                        printf ( "CalledID length:%d:", st_IEInfo.Info.st_CalledParty.u8_AddressLen );
                        for ( i=0; i < st_IEInfo.Info.st_CalledParty.u8_AddressLen; i ++ )
                            printf (" 0x%02x", st_IEInfo.Info.st_CalledParty.pu8_Address[i] );
                        printf( "\n" );*/

                        memcpy(pst_Call->ch_CalledID, st_IEInfo.Info.st_CalledParty.pu8_Address, st_IEInfo.Info.st_CalledParty.u8_AddressLen);
                        memcpy(&pst_Call->st_CalledParty, &st_IEInfo.Info.st_CalledParty, sizeof(pst_Call->st_CalledParty));
                        pst_Call->st_CalledParty.pu8_Address = (u8*)pst_Call->ch_CalledID;
                        pst_Call->ch_CalledID[ pst_Call->st_CalledParty.u8_AddressLen ] = '\0';
                    }
                    break;

                case    CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;

            }

            _appmedia_CallObjMediaPropertySet(pst_Call, u16_IE, &st_IEInfo);

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (st_Response.e_Response != CMBS_RESPONSE_OK)
        {
            if (pst_Call)
            {
                printf("app_OnCallEstablish:  Response Error. Call Instance %d\n", pst_Call->u32_CallInstance);
            }
            else
            {
                printf("app_OnCallEstablish:  Response Error\n");

            }
            return;
        }

        if (! pst_Call)
        {
            printf("app_OnCallEstablish: No Call Instance\n");
            return;
        }

        /* Mark call as outgoing */
        pst_Call->b_Incoming = FALSE;

        /* reset codecs flag */
        pst_Call->b_CodecsOfferedToTarget = FALSE;

        _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_OUT_PEND);

        //if (g_cmbsappl.n_Token)
        {
            //appcmbs_ObjectSignal(NULL , 0, _appcall_CallObjIdGet(pst_Call), CMBS_EV_DEE_CALL_ESTABLISH);
            appcmbs_ObjectReport(NULL , 0, _appcall_CallObjIdGet(pst_Call), CMBS_EV_DEE_CALL_ESTABLISH);
        }

        // reply early media connect and start dial tone.
        u16_CallId = _appcall_CallObjIdGet(pst_Call);
		printf("u16_CallId = %d\n", u16_CallId);

        if (!pst_Call->st_CalledParty.u8_AddressLen ||
                ((_appcall_CheckBackwardCompatibilityLineSelection(pst_Call)) && (pst_Call->st_CalledParty.u8_AddressLen == 2)))
        {
            st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
            st_Properties.psz_Value = "CMBS_CALL_PROGR_SETUP_ACK\0";
			printf("Line2 = %d\n", pst_Call->u8_LineId);

            appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);
        }
        else
        {
            _appcall_CheckBackwardCompatibilityLineSelection(pst_Call);
			printf("Line3 = %d\n", pst_Call->u8_LineId);

            if (pst_Call->u8_LineId == APPCALL_NO_LINE)
            {
                /* use default line for HS */
                u32 u32_FPManagedLineID;
                List_GetDefaultLine(pst_Call->st_CallerParty.pu8_Address[0] - '0', &u32_FPManagedLineID);
                pst_Call->u8_LineId = (u8)u32_FPManagedLineID;
            }

            if (pst_Call->u8_LineId == APPCALL_NO_LINE)
            {
                ST_APPCALL_PROPERTIES st_Properties;
                static char s_reason[5] = {0};
                sprintf(s_reason, "%d", CMBS_REL_REASON_BUSY);
                // disconnecting call
                st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
                st_Properties.psz_Value = s_reason;
                appcall_ReleaseCall(&st_Properties, 1, _appcall_CallObjIdGet(pst_Call), NULL);
                return;
            }
            else
            {
                st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
                st_Properties.psz_Value = "CMBS_CALL_PROGR_PROCEEDING\0";
                appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);

                // Simulate network delay
                SleepMs(300);

                st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
                st_Properties.psz_Value = "CMBS_CALL_PROGR_RINGING\0";
                appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);
            }
        }

        // AG: External VoIP library integration
        // As we do not have state machines & timeouts implemented -
        // wait for '#' as a signal that digits are completed and start will a call
        if (pst_Call->st_CalledParty.u8_AddressLen && pst_Call->st_CalledParty.pu8_Address[pst_Call->st_CalledParty.u8_AddressLen - 1] == '#')
            extvoip_MakeCall(_appcall_CallObjIdGet(pst_Call));
    }

}

//  ========== app_OnCallProgress ===========
/*!
        \brief    CMBS Target signal call progres
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnCallProgress(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ      pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;
    void *         pv_IE;
    u16            u16_IE;
    //u8             u8_i;
    ST_IE_RESPONSE st_Response;
    ST_IE_CALLPROGRESS    st_CallProgress;
    ST_IE_CALLEDPARTY     st_CalledParty;

    st_Response.e_Response = CMBS_RESPONSE_OK;

    memset(&st_CalledParty, 0, sizeof(st_CalledParty));

    UNUSED_PARAMETER(pvAppRefHandle);
    printf("%s\n", __FUNCTION__);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE, CMBS_IE_CALLEDPARTY and CMBS_IE_CALLPROGRESS

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    // Todo: get call resources for this CallInstance...
                    pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
                    break;

                case CMBS_IE_CALLPROGRESS:
                    st_CallProgress = st_IEInfo.Info.st_CallProgress;

                    if (pst_Call)
                    {
                        switch (st_CallProgress.e_Progress)
                        {
                            case  CMBS_CALL_PROGR_PROCEEDING:
                                //                       _appcall_CallObjStateSet (pst_Call, E_APPCMBS_CALL_OUT_PROC );
                                extvoip_ProceedingCall(_appcall_CallObjIdGet(pst_Call));
                                break;
                            case  CMBS_CALL_PROGR_RINGING:
                                _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_INC_RING);
                                extvoip_AlertingCall(_appcall_CallObjIdGet(pst_Call));
                                break;
                            case  CMBS_CALL_PROGR_BUSY:
                                break;
                            case  CMBS_CALL_PROGR_INBAND:
                                break;
                            default:
                                break;
                        }
                    }
                    break;

                case CMBS_IE_CALLEDPARTY:
                    st_CalledParty = st_IEInfo.Info.st_CalledParty;
                    break;

                case      CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;

            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (st_Response.e_Response != CMBS_RESPONSE_OK)
        {
            if (pst_Call)
            {
                printf("app_OnCallProgress:  Response Error. Call Instance %d\n", pst_Call->u32_CallInstance);
            }
            else
            {
                printf("app_OnCallProgress:  Response Error\n");

            }
            return;
        }

        if (! pst_Call)
        {
            printf("app_OnCallProgress: ERROR: invalid Call Instance\n");
            return;
        }

        //TODO  save st_CalledParty to pst_Call  if need
        /*printf("\napp_OnCallProgress: CallInstance=0x%08X, CallProgress=%s, CalledParty[ prop=%d, pres=%d, len=%d, addr=",
               u32_CallInstance, cmbs_dbg_GetCallProgressName(st_CallProgress.e_Progress), st_CalledParty.u8_AddressProperties,
               st_CalledParty.u8_AddressPresentation, st_CalledParty.u8_AddressLen);
        for ( u8_i = 0; u8_i < st_CalledParty.u8_AddressLen; ++u8_i )
        {
            printf("%c", st_CalledParty.pu8_Address[u8_i]);
        }
        printf(" ]\n");*/


        //if (g_cmbsappl.n_Token)
        {
            /*appcmbs_ObjectSignal((void*)&st_CallProgress ,
                                 sizeof(st_CallProgress),
                                 _appcall_CallObjIdGet(pst_Call),
                                 CMBS_EV_DEE_CALL_PROGRESS);*/

            appcmbs_ObjectReport((void*)&st_CallProgress ,
                                 sizeof(st_CallProgress),
                                 _appcall_CallObjIdGet(pst_Call),
                                 CMBS_EV_DEE_CALL_PROGRESS);
        }

    }
    else
    {
        printf("app_OnCallProgress: ERROR: invalid IE list\n");
    }
}

//  ========== app_OnCallInbandInfo ===========
/*!
        \brief    CMBS Target signal inband info, e.g. digits
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void app_OnCallInbandInfo(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ  pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;
    ST_IE_RESPONSE  st_Response;
    void *    pv_IE;
    u16     u16_IE;
    printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
    st_Response.e_Response = CMBS_RESPONSE_OK;

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    // Todo: get call resources for this CallInstance...
                    pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
                    break;

                case CMBS_IE_LINE_ID:
                    //if (pst_Call)
                    //pst_Call->u8_LineId = st_IEInfo.Info.u8_LineId;
                    break;

                case CMBS_IE_CALLINFO :
                    if (pst_Call)
                    {
                        // enter to called party entry digits;

                        if (_appcall_CallObjStateGet(pst_Call) == E_APPCMBS_CALL_OUT_PEND)
                        {
                            if (_appcall_CallObjMediaGet(pst_Call) == E_APPCMBS_MEDIA_PEND)
                            {
                                appmedia_CallObjTonePlay(NULL, FALSE, _appcall_CallObjIdGet(pst_Call), NULL);
                            }

                            _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_OUT_PEND_DIAL);
                        }

                        if (_appcall_CallObjStateGet(pst_Call) < E_APPCMBS_CALL_ACTIVE)
                        {
                            if ((pst_Call->st_CalledParty.u8_AddressLen + st_IEInfo.Info.st_CallInfo.u8_DataLen) < sizeof(pst_Call->ch_CalledID))
                            {
                                memcpy(pst_Call->st_CalledParty.pu8_Address + pst_Call->st_CalledParty.u8_AddressLen,
                                       st_IEInfo.Info.st_CallInfo.pu8_Info,
                                       st_IEInfo.Info.st_CallInfo.u8_DataLen);

                                *(pst_Call->st_CalledParty.pu8_Address
                                  + pst_Call->st_CalledParty.u8_AddressLen
                                  + st_IEInfo.Info.st_CallInfo.u8_DataLen) = '\0';

                                pst_Call->st_CalledParty.u8_AddressLen = (u8)strlen((char*)pst_Call->st_CalledParty.pu8_Address);
								appmedia_CallObjTonePlay("CMBS_TONE_DIAL\0", FALSE, _appcall_CallObjIdGet(pst_Call), NULL);

                                //_appDTMFPlay(pst_Call,
                                //             st_IEInfo.Info.st_CallInfo.pu8_Info,
                                //             st_IEInfo.Info.st_CallInfo.u8_DataLen);

                                // analize handset's line selection, e.g. "*07*..."
                                if ((pst_Call->st_CalledParty.u8_AddressLen == 4)
                                        && (pst_Call->st_CalledParty.pu8_Address[0] == '*')
                                        && (pst_Call->st_CalledParty.pu8_Address[3] == '*'))
                                {
                                    u8    u8_NewLineId = APPCALL_NO_LINE;
                                    char  buf[3];
                                    buf[0] = pst_Call->st_CalledParty.pu8_Address[1];
                                    buf[1] = pst_Call->st_CalledParty.pu8_Address[2];
                                    buf[2] = '\0';

                                    u8_NewLineId = atoi(buf);

                                    if (u8_NewLineId < APPCALL_LINEOBJ_MAX)
                                        _appcall_CallObjLineObjSet(pst_Call, _appcall_LineObjGet(u8_NewLineId));
                                }

                                /* analyze handset's line selection - CAT-iq 2.0 way */
                                _appcall_CheckBackwardCompatibilityLineSelection(pst_Call);

                                if (pst_Call->u8_LineId == APPCALL_NO_LINE)
                                {
                                    /* use default line for HS */
                                    u32 u32_FPManagedLineID;
                                    List_GetDefaultLine(pst_Call->st_CallerParty.pu8_Address[0] - '0', &u32_FPManagedLineID);
                                    pst_Call->u8_LineId = (u8)u32_FPManagedLineID;
                                }

                                //release call, if there is no available line
                                if (pst_Call->u8_LineId == APPCALL_NO_LINE)
                                {
                                    ST_APPCALL_PROPERTIES st_Properties;
                                    static char s_reason[5] = {0};
                                    sprintf(s_reason, "%d", CMBS_REL_REASON_BUSY);
                                    // disconnecting call
                                    st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
                                    st_Properties.psz_Value = s_reason;
                                    appcall_ReleaseCall(&st_Properties, 1, _appcall_CallObjIdGet(pst_Call), NULL);
                                    printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
                                    return;
                                }


                                if (g_EarlyMediaAutoCfm &&
                                        ((pst_Call->st_CalledParty.u8_AddressLen > 2) ||
                                         (pst_Call->st_CalledParty.u8_AddressLen > 0 && pst_Call->st_CalledParty.pu8_Address[0] != '#')))
                                {
                                    ST_APPCALL_PROPERTIES   st_Properties;
                                    u16  u16_CallId = _appcall_CallObjIdGet(pst_Call);

                                    if (pst_Call->e_Call < E_APPCMBS_CALL_OUT_RING)
                                    {
                                        st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
                                        st_Properties.psz_Value = "CMBS_CALL_PROGR_PROCEEDING\0";
                                        appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);

                                        // Simulate network delay
                                        /*SleepMs(300);

                                        st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
                                        st_Properties.psz_Value = "CMBS_CALL_PROGR_RINGING\0";
                                        appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);*/
                                    }
                                }
                                else
                                    appmedia_CallObjTonePlay("CMBS_TONE_RING_BACK\0", TRUE, _appcall_CallObjIdGet(pst_Call), NULL);
                            }
                            else
                            {
                                printf("Digits full\n");
                            }
                        }

                        else if (_appcall_CallObjStateGet(pst_Call) == E_APPCMBS_CALL_ACTIVE)
                        {
                            // following lines are commented to pass CAT-iq 2.0 tests - no FP DTMF echo to CAT-iq 2.0 HS
                            // For GAP HS it is legal, though, but not implemented here
//                      _appDTMFPlay ( pst_Call,
//                                     st_IEInfo.Info.st_CallInfo.pu8_Info,
//                                     st_IEInfo.Info.st_CallInfo.u8_DataLen );
                        }
                    }
                    break;

                case      CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
            }

            if (st_Response.e_Response != CMBS_RESPONSE_OK)
            {
                if (pst_Call)
                {
                    printf("app_OnCallInbandInfo:  Response Error. Call Instance %d\n", pst_Call->u32_CallInstance);
                }
                else
                {
                    printf("app_OnCallInbandInfo:  Response Error\n");

                }
                printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
                return;
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
//!! todo
        printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
        appcmbs_ObjectReport(NULL , 0, _appcall_CallObjIdGet(pst_Call), CMBS_EV_DEE_CALL_INBANDINFO);
        // AG: External VoIP library integration
        // As we do not have state machines & timeouts implemented -
        // wait for '#' as a signal that digits are completed and start will a call
        if (pst_Call && pst_Call->st_CalledParty.u8_AddressLen && pst_Call->st_CalledParty.pu8_Address[pst_Call->st_CalledParty.u8_AddressLen - 1] == '#')
            extvoip_MakeCall(_appcall_CallObjIdGet(pst_Call));

    }
}

//  ========== app_OnCallAnswer ===========
/*!
        \brief    CMBS Target answer call
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnCallAnswer(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ      pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;
    ST_IE_RESPONSE st_Response;
    void *         pv_IE;
    u16            u16_IE;
	int i = 0;

    st_Response.e_Response = CMBS_RESPONSE_OK;

    UNUSED_PARAMETER(pvAppRefHandle);
    printf("%s\n", __FUNCTION__);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE, CMBS_IE_MEDIACHANNEL and CMBS_IE_MEDIADESCRIPTOR
        // Todo: Search for call instance first

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    // Todo: get call resources for this CallInstance...
                    pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
                    break;

                case CMBS_IE_CALLEDPARTY:
                    //printf("app_OnCallAnswer: CALLED_ID address properties:0x%02x\n", st_IEInfo.Info.st_CalledParty.u8_AddressProperties);
                    if (pst_Call)
                    {
                        /*int i;
                        printf ( "CalledID length: %d:", st_IEInfo.Info.st_CalledParty.u8_AddressLen );
                        for ( i=0; i < st_IEInfo.Info.st_CalledParty.u8_AddressLen; i ++ )
                            printf (" 0x%02x", st_IEInfo.Info.st_CalledParty.pu8_Address[i] );
                        printf( "\n" );*/

                        memcpy(pst_Call->ch_CalledID, st_IEInfo.Info.st_CalledParty.pu8_Address, st_IEInfo.Info.st_CalledParty.u8_AddressLen);
                        memcpy(&pst_Call->st_CalledParty, &st_IEInfo.Info.st_CalledParty, sizeof(pst_Call->st_CalledParty));
                        pst_Call->st_CalledParty.pu8_Address = (u8*)pst_Call->ch_CalledID;
                        pst_Call->ch_CalledID[ pst_Call->st_CalledParty.u8_AddressLen ] = '\0';
                    }
                    break;
                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
            }

            _appmedia_CallObjMediaPropertySet(pst_Call, u16_IE, &st_IEInfo);

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (st_Response.e_Response != CMBS_RESPONSE_OK)
        {
            if (pst_Call)
            {
                printf("app_OnCallAnswer:  Response Error. Call Instance %d\n", pst_Call->u32_CallInstance);
            }
            else
            {
                printf("app_OnCallAnswer:  Response Error\n");

            }
            return;
        }

        if (! pst_Call)
        {
            printf("app_OnCallAnswer: ERROR: No Call Instance\n");
            return;
        }

        _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_ACTIVE);

        if (g_call_automat)
        {
            appmedia_CallObjMediaStart(pst_Call->u32_CallInstance, 0, 0xFF, NULL);
        }

        appcmbs_ObjectReport(NULL , 0, _appcall_CallObjIdGet(pst_Call), CMBS_EV_DEE_CALL_ANSWER);
        // AG: External VoIP library integration
        // Answer incoming VoIP call
        extvoip_AnswerCall(_appcall_CallObjIdGet(pst_Call));

        //cmbsevent_onCallAnswered(_appcall_CallObjIdGet(pst_Call));

    }    
	for (i = 0; i < 2; i++)
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO ----------------------\n"));
        appcall_InfoCall(i);
    }
}


//  ========== app_OnCallRelease ===========
/*!
        \brief    CMBS Target release call
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnCallRelease(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ         pst_Call = NULL;
    ST_APPCMBS_IEINFO    st_IEInfo;
    void *                 pv_IE;
    u16                    u16_IE;
    u16 u16_callid;
    ST_IE_RELEASE_REASON st_Reason;

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE and CMBS_IE_CALLRELEASE_REASON

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            if (u16_IE == CMBS_IE_CALLINSTANCE)
            {
                pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
            }

            if (u16_IE == CMBS_IE_CALLRELEASE_REASON)
            {
                // we have to keep the structure for later messaging
                cmbs_api_ie_CallReleaseReasonGet(pv_IE, &st_Reason);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (! pst_Call)
        {
            printf("app_OnCallRelease: ERROR: No Call Instance\n");
            return;
        }

        /* Add Entry to call list */
        if (pst_Call->b_Incoming)
        {
            /* Incoming accepted call */
            _appcall_AddCallEntry(LIST_TYPE_INCOMING_ACCEPTED_CALLS, pst_Call->ch_CallerID, pst_Call->u8_LineId);
        }
        else
        {
            /* Outgoing call */
            _appcall_AddCallEntry(LIST_TYPE_OUTGOING_CALLS, pst_Call->ch_CalledID, pst_Call->u8_LineId);
        }

        if (_appcall_CallObjMediaGet(pst_Call) == E_APPCMBS_MEDIA_ACTIVE)
        {
            // release Media stream
            /*!\todo release media stream */
            appmedia_CallObjMediaStop(pst_Call->u32_CallInstance, 0, NULL);
        }

        //keyb_ReleaseNotify( _appcall_CallObjIdGet(pst_Call) );
        printf("ERROR: Funciton is empty\n");

        //if (g_cmbsappl.n_Token)
        if (!pst_Call->stopring)
        {
            u16_callid = _appcall_CallObjIdGet(pst_Call);
            appcmbs_ObjectReport((void*)&u16_callid , sizeof(u16_callid), pst_Call->u8_LineId, CMBS_EV_DEE_CALL_RELEASE);
        }
        else
        {
            printf("CMBS_EV_DEE_CALL_RELEASE, stop ring ch_CalledID:%s ch_CallerID lineid:%u\n", pst_Call->ch_CalledID, pst_Call->ch_CallerID, pst_Call->u8_LineId);
        }

        // AG: External VoIP library integration
        // Release VoIP call
        extvoip_DisconnectCall(_appcall_CallObjIdGet(pst_Call));

        cmbsevent_onCallReleased(_appcall_CallObjIdGet(pst_Call));

        // send Release_Complete
        cmbs_dee_CallReleaseComplete(pvAppRefHandle, pst_Call->u32_CallInstance);

        _appcall_CallObjDelete(pst_Call->u32_CallInstance, NULL);

    }
}


//  ========== app_OnCallReleaseComplete ===========
/*!
        \brief    CMBS Target reports on call relase completion
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List       IE list pointer
        \return    <none>
*/
void app_OnCallReleaseComplete(void* pvAppRefHandle, void* pv_List)
{
    PST_CALL_OBJ pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;
    void* pv_IE;
    u16 u16_IE;

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        /* collect information elements. we expect: CMBS_IE_CALLINSTANCE */

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);

        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            if (u16_IE == CMBS_IE_CALLINSTANCE)
            {
                pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (!pst_Call)
        {
            printf("app_OnCallReleaseComplete: ERROR: No Call Instance\n");
            return;
        }

        /* Add Entry to call list */
        if (pst_Call->b_Incoming)
        {
            if (pst_Call->e_Call < E_APPCMBS_CALL_ACTIVE)
            {
                /* Missed call */
                _appcall_AddCallEntry(LIST_TYPE_MISSED_CALLS, pst_Call->ch_CallerID, pst_Call->u8_LineId);
                ListChangeNotif_MissedCallListChanged(pst_Call->u8_LineId, TRUE, CMBS_ALL_RELEVANT_HS_ID);
            }
            else
            {
                /* Incoming accepted call */
                _appcall_AddCallEntry(LIST_TYPE_INCOMING_ACCEPTED_CALLS, pst_Call->ch_CallerID, pst_Call->u8_LineId);
            }
        }
        else
        {
            /* Outgoing call */
            _appcall_AddCallEntry(LIST_TYPE_OUTGOING_CALLS, pst_Call->ch_CalledID, pst_Call->u8_LineId);
        }

        if (pst_Call)
        {
            if (!pst_Call->stopring)
            {   printf("----1 CMBS_EV_DEE_CALL_RELEASE, ch_CalledID:%s ch_CallerID lineid:%u\n", pst_Call->ch_CalledID, pst_Call->ch_CallerID, pst_Call->u8_LineId);
                appcmbs_ObjectReport(NULL , 0, pst_Call->u8_LineId, CMBS_EV_DEE_CALL_RELEASECOMPLETE);
            }
            else
            {
                printf("----2 CMBS_EV_DEE_CALL_RELEASE, stop ring ch_CalledID:%s ch_CallerID lineid:%u\n", pst_Call->ch_CalledID, pst_Call->ch_CallerID, pst_Call->u8_LineId);
            }
            extvoip_DisconnectDoneCall(_appcall_CallObjIdGet(pst_Call));
        }

        _appcall_CallObjDelete(pst_Call->u32_CallInstance, NULL);
    }
    else
    {
        printf("app_OnCallReleaseComplete: ERROR: No IE List!\n");
    }
}

void        app_OnCallHold(void * pvAppRefHandle, void * pv_List)
{
    u32            u32_CallInstance = 0;
    void *         pv_IE;
    u16            u16_IE;
    u16            u16_Handset = 0;
    PST_CALL_OBJ    pst_Call = NULL;


    if (pvAppRefHandle)
    {
    };

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            if (CMBS_IE_CALLINSTANCE == u16_IE)
            {
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                pst_Call = _appcall_CallObjGet(u32_CallInstance, NULL);
            }
            else if (CMBS_IE_HANDSETS == u16_IE)
            {
                cmbs_api_ie_HandsetsGet(pv_IE, &u16_Handset);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    printf("app_OnCallHold: Call Instance = %x, HS-Mask = 0x%X\n", u32_CallInstance, u16_Handset);
    printf("\nHold ");
    if (g_HoldCfm == 1)
    {
        printf("accepted\n");
        cmbs_dee_CallHoldRes(pvAppRefHandle, u32_CallInstance, u16_Handset, CMBS_RESPONSE_OK);
        if (pst_Call)
        {
            _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_ON_HOLD);
            appcmbs_ObjectReport(NULL , 0, _appcall_CallObjIdGet(pst_Call), CMBS_EV_DEE_CALL_HOLD);
            extvoip_HoldCall(_appcall_CallObjIdGet(pst_Call));
        }
    }
    else
    {
        printf("declined\n");
        cmbs_dee_CallHoldRes(pvAppRefHandle, u32_CallInstance, u16_Handset, CMBS_RESPONSE_ERROR);
    }
}


void app_OnInternalCallTransfer(void * pvAppRefHandle, void * pv_List)
{
    ST_IE_INTERNAL_TRANSFER st_IntTransReq;
    void *         pv_IE;
    u16            u16_IE;

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            switch (u16_IE)
            {
                case CMBS_IE_INTERNAL_TRANSFER:
                    cmbs_api_ie_InternalCallTransferReqGet(pv_List, &st_IntTransReq);
                    break;

                default:
                    break;
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    /* Do nothing */
}

void app_OnCallTransfer(void * pvAppRefHandle, void * pv_List)
{
    ST_IE_CALLTRANSFERREQ st_TrfReq;
    ST_IE_RESPONSE st_Resp;
    void *         pv_RefIEList = NULL;
    PST_CFR_IE_LIST p_List;

    void *         pv_IE;
    u16            u16_IE;

    ST_APPCMBS_IEINFO st_IEInfo;

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLTRANSFERREQ:
                    cmbs_api_ie_CallTransferReqGet(pv_List, &st_TrfReq);
                    break;
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    cmbs_api_ie_CallTransferReqGet(pv_List, &st_TrfReq);

    if (g_TransferAutoCfm == 1)
    {
        st_Resp.e_Response = CMBS_RESPONSE_OK;
    }
    else
    {
        st_Resp.e_Response = CMBS_RESPONSE_ERROR;
    }

    pv_RefIEList = cmbs_api_ie_GetList();
    if (pv_RefIEList)
    {
        // Add call Instance IE
        cmbs_api_ie_CallTransferReqAdd(pv_RefIEList, &st_TrfReq);
        cmbs_api_ie_ResponseAdd(pv_RefIEList, &st_Resp);

        p_List = (PST_CFR_IE_LIST)pv_RefIEList;

        cmbs_int_EventSend(CMBS_EV_DCM_CALL_TRANSFER_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
    }
}

void app_OnCallConference(void * pvAppRefHandle, void * pv_List)
{
    ST_IE_CALLTRANSFERREQ st_TrfReq;
    ST_IE_RESPONSE st_Resp;
    void *         pv_RefIEList = NULL;
    PST_CFR_IE_LIST p_List;

    void *         pv_IE;
    u16            u16_IE;

    ST_APPCMBS_IEINFO st_IEInfo;

    UNUSED_PARAMETER(pvAppRefHandle);
    memset(&st_TrfReq, 0, sizeof(ST_IE_CALLTRANSFERREQ));

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLTRANSFERREQ:
                    cmbs_api_ie_CallTransferReqGet(pv_IE, &st_TrfReq);
                    break;
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    //cmbs_api_ie_CallTransferReqGet(u16_IE,&st_TrfReq);

    if (g_ConfAutoCfm == 1)
    {
        st_Resp.e_Response = CMBS_RESPONSE_OK;
    }
    else
    {
        st_Resp.e_Response = CMBS_RESPONSE_ERROR;
    }

    pv_RefIEList = cmbs_api_ie_GetList();
    if (pv_RefIEList)
    {
        // Add call Instance IE
        cmbs_api_ie_CallTransferReqAdd(pv_RefIEList, &st_TrfReq);
        cmbs_api_ie_ResponseAdd(pv_RefIEList, &st_Resp);

        p_List = (PST_CFR_IE_LIST)pv_RefIEList;

        cmbs_int_EventSend(CMBS_EV_DCM_CALL_CONFERENCE_RES, p_List->pu8_Buffer, p_List->u16_CurSize);
    }

    _appcall_CallObjStateSet(_appcall_CallObjGet(st_TrfReq.u32_CallInstanceFrom, NULL), E_APPCMBS_CALL_CONFERENCE);
    _appcall_CallObjStateSet(_appcall_CallObjGet(st_TrfReq.u32_CallInstanceTo, NULL), E_APPCMBS_CALL_CONFERENCE);

    extvoip_ConferenceCall(_appcall_CallObjIdGet(_appcall_CallObjGet(st_TrfReq.u32_CallInstanceFrom, NULL)), _appcall_CallObjIdGet(_appcall_CallObjGet(st_TrfReq.u32_CallInstanceTo, NULL)));
}

void        app_OnMediaOfferRes(void * pvAppRefHandle, void * pv_List)
{
    ST_IE_MEDIA_DESCRIPTOR  st_MediaDesc;
    u32                     u32_CallInstance;
    void *                  pv_IE;
    u16                     u16_IE;
    PST_CALL_OBJ            pst_Call = NULL;

    UNUSED_PARAMETER(pvAppRefHandle);
    memset(&st_MediaDesc, 0, sizeof(ST_IE_MEDIA_DESCRIPTOR));

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            if (CMBS_IE_CALLINSTANCE == u16_IE)
            {
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                pst_Call = _appcall_CallObjGet(u32_CallInstance, NULL);
            }
			
            else if (CMBS_IE_MEDIADESCRIPTOR == u16_IE)
            {
                cmbs_api_ie_MediaDescGet(pv_IE, &st_MediaDesc);
            }
		
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
		
        if (pst_Call)
        {
            if (pst_Call->e_Codec != st_MediaDesc.e_Codec)
            {
                pst_Call->e_Codec = st_MediaDesc.e_Codec;
                extvoip_MediaAckChange(_appcall_CallObjIdGet(pst_Call), (EXTVOIP_CODEC)st_MediaDesc.e_Codec);
            }

            //appmedia_CallObjMediaStart(u32_CallInstance, 0, 0xFF, 0);
        }
        
    }
}

void        app_OnHsCodecCfmFailed(void * pvAppRefHandle, void * pv_List)
{
    u32              u32_CallInstance = 0;
    void *           pv_IE;
    u16              u16_IE;
    ST_IE_RESPONSE st_Response;

    st_Response.e_Response = CMBS_RESPONSE_OK;

    if (pvAppRefHandle)
    {
    };

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            if (CMBS_IE_CALLINSTANCE == u16_IE)
            {
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
            }
            else if (CMBS_IE_RESPONSE == u16_IE)
            {
                cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    if (st_Response.e_Response != CMBS_RESPONSE_OK)
    {
        printf("app_OnHsCodecCfmFailed Recieved response not OK!. Call Instance %d\n", u32_CallInstance);
        return;
    }

    printf("\n\napp_OnHsCodecCfmFailed:  CallInstance = 0x%08X\n", u32_CallInstance);

}

void        app_OnCallResume(void * pvAppRefHandle, void * pv_List)
{
    u32            u32_CallInstance = 0;
    void *         pv_IE;
    u16            u16_IE;
    u16            u16_Handset = 0;
    PST_CALL_OBJ   pst_Call = NULL;


    if (pvAppRefHandle)
    {
    };

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            if (CMBS_IE_CALLINSTANCE == u16_IE)
            {
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                pst_Call = _appcall_CallObjGet(u32_CallInstance, NULL);
            }
            else if (CMBS_IE_HANDSETS == u16_IE)
            {
                cmbs_api_ie_HandsetsGet(pv_IE, &u16_Handset);
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    printf("app_OnCallResume: Call Instance = %x, HS-Mask = 0x%X\n", u32_CallInstance, u16_Handset);
    printf("\nHold resume ");
    if (g_HoldResumeCfm == 1)
    {
        printf("accepted\n");
        cmbs_dee_CallResumeRes(pvAppRefHandle, u32_CallInstance, u16_Handset, CMBS_RESPONSE_OK);

        if (pst_Call)
        {
            _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_ACTIVE);
            appcmbs_ObjectReport(NULL , 0, _appcall_CallObjIdGet(pst_Call), CMBS_EV_DEE_CALL_RESUME);
            extvoip_ResumeCall(_appcall_CallObjIdGet(pst_Call));
        }
    }
    else
    {
        printf("declined\n");
        cmbs_dee_CallResumeRes(pvAppRefHandle, u32_CallInstance, u16_Handset, CMBS_RESPONSE_ERROR);
    }
}


void        app_OnCallState(void * pvAppRefHandle, void * pv_List)
{
    u32              u32_CallInstance = 0;
    void *           pv_IE;
    u16              u16_IE;
    ST_IE_CALL_STATE st_CallState;
    ST_IE_RESPONSE st_Response;
    PST_CALL_OBJ pst_Call = NULL;

    memset(&st_CallState, 0, sizeof(ST_IE_CALL_STATE));
    st_Response.e_Response = CMBS_RESPONSE_OK;

    if (pvAppRefHandle)
    {
    };

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE)
        {
            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
                    break;

                case CMBS_IE_CALLSTATE:
                    cmbs_api_ie_CallStateGet(pv_IE, &st_CallState);
                    break;

                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;

            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }

    pst_Call = _appcall_CallObjGet(u32_CallInstance, NULL);

    if (pst_Call != NULL)
    {
        switch (st_CallState.e_CallStatus)
        {
            case CMBS_CALL_STATE_STATUS_CONF_CONNECTED:
                _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_CONFERENCE);
                break;
            case CMBS_CALL_STATE_STATUS_CALL_CONNECTED:
                _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_ACTIVE);
                break;
            case CMBS_CALL_STATE_STATUS_CALL_HOLD:
                _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_ON_HOLD);
                break;
            case CMBS_CALL_STATE_STATUS_CALL_SETUP_ACK:
                if (st_CallState.e_CallType == CMBS_CALL_STATE_TYPE_EXT_OUTGOING)
                {
                    if (VOIP_LINE1_HSMASK & st_CallState.u16_HandsetsMask)
                    {
                        pst_Call->u8_LineId = 0;
                    }
                    else
                    {
                        pst_Call->u8_LineId = 1;
                    }
                }
                break;
            default:
                break;
        }
    }

    if (pst_Call)
    {
        appcmbs_ObjectReport(&st_CallState , sizeof(st_CallState), _appcall_CallObjIdGet(pst_Call), CMBS_EV_DCM_CALL_STATE);
    }

    if (st_Response.e_Response != CMBS_RESPONSE_OK)
    {
        printf("app_OnCallState Recieved response not OK!. Call Instance %d\n", u32_CallInstance);
        return;
    }



    //TODO  change call object state ?
}


//  ========== app_OnCallMediaUpdate ===========
/*!
        \brief    CMBS Target announce media connectivity information
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void app_OnCallMediaUpdate(void * pv_List)
{
    PST_CALL_OBJ      pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;

    void *           pv_IE;
    u16              u16_IE;

    printf("Media Update\n");

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            if (u16_IE == CMBS_IE_CALLINSTANCE)
            {
                pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
            }

            _appmedia_CallObjMediaPropertySet(pst_Call, u16_IE, &st_IEInfo);

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (! pst_Call)
        {
            printf("app_OnCallMediaUpdate: ERROR: No Call Instance\n");
            return;
        }

        appmedia_CallObjMediaStart(pst_Call->u32_CallInstance, 0, 0xFF, NULL);
        _appcall_CallObjMediaSet(pst_Call, E_APPCMBS_MEDIA_ACTIVE);
		appmedia_CallObjTonePlay("CMBS_TONE_DIAL\0", TRUE, _appcall_CallObjIdGet(pst_Call), NULL);

        extvoip_MediaChange(_appcall_CallObjIdGet(pst_Call), pst_Call->e_Codec);
    }
}

//  ========== app_OnChannelStartRsp ===========
/*!
        \brief    CMBS Target ackknoledge channel start
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnChannelStartRsp(void * pv_List)
{
    u32              u32_CallInstance;
    void *           pv_IE;
    u16              u16_IE;

    if (pv_List)
    {
        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            // for your convenience, Call Instance IE will allways be the first
            if (u16_IE == CMBS_IE_CALLINSTANCE)
            {
                cmbs_api_ie_CallInstanceGet(pv_IE, &u32_CallInstance);
//            app_CallAnswer( &CmbsApp.u32_AppRef, u32_CallInstance );
            }
            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }
    }
}


//  ========== app_OnHsLocProgress ===========
/*!
        \brief    CMBS Target signal handset locator progress
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnHsLocProgress(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ      pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;

    void *         pv_IE;
    u16            u16_IE;
    u8             u8_i;

    u32                   u32_CallInstance = 0;
    ST_IE_CALLPROGRESS    st_CallProgress;
    ST_IE_CALLEDPARTY     st_CalledParty;
    ST_IE_RESPONSE st_Response;

    st_Response.e_Response = CMBS_RESPONSE_OK;
    memset(&st_CallProgress, 0, sizeof(st_CallProgress));
    memset(&st_CalledParty, 0, sizeof(st_CalledParty));

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE, CMBS_IE_CALLEDPARTY and CMBS_IE_CALLPROGRESS

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    u32_CallInstance = st_IEInfo.Info.u32_CallInstance;

                    pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
                    break;

                case CMBS_IE_CALLPROGRESS:
                    st_CallProgress = st_IEInfo.Info.st_CallProgress;

                    if (pst_Call)
                    {
                        switch (st_CallProgress.e_Progress)
                        {
                            case  CMBS_CALL_PROGR_PROCEEDING:
                                //                       _appcall_CallObjStateSet (pst_Call, E_APPCMBS_CALL_OUT_PROC );
                                break;
                            case  CMBS_CALL_PROGR_RINGING:
                                _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_INC_RING);
                                break;
                            case  CMBS_CALL_PROGR_BUSY:
                                break;
                            case  CMBS_CALL_PROGR_INBAND:
                                break;
                            default:
                                break;
                        }
                    }
                    break;

                case CMBS_IE_CALLEDPARTY:
                    st_CalledParty = st_IEInfo.Info.st_CalledParty;
                    break;

                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;

            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (st_Response.e_Response != CMBS_RESPONSE_OK)
        {
            printf("app_OnHsLocProgress Recieved response not OK!. Call Instance %d\n", u32_CallInstance);
            return;
        }

        //TODO  save st_CalledParty to pst_Call  if need
        printf("\napp_OnHsLocProgress: CallInstance=0x%08X, CallProgress=%s, CalledParty[ prop=%d, pres=%d, len=%d, addr=",
               u32_CallInstance, cmbs_dbg_GetCallProgressName(st_CallProgress.e_Progress), st_CalledParty.u8_AddressProperties,
               st_CalledParty.u8_AddressPresentation, st_CalledParty.u8_AddressLen);
        for (u8_i = 0; u8_i < st_CalledParty.u8_AddressLen; ++u8_i)
        {
            printf("%c", st_CalledParty.pu8_Address[u8_i]);
        }
        printf(" ]\n");
    }
    else
    {
        printf("app_OnCallProgress: ERROR: invalid IE list\n");
    }
}



//  ========== app_OnHsLocAnswer ===========
/*!
        \brief    CMBS Target signal handset locator answer
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnHsLocAnswer(void * pvAppRefHandle, void * pv_List)
{
    ST_APPCMBS_IEINFO st_IEInfo;

    void *    pv_IE;
    u16     u16_IE;
    u8     u8_i;

    u32     u32_CallInstance = 0;
    ST_IE_CALLEDPARTY st_CalledParty;
    ST_IE_RESPONSE  st_Response;

    st_Response.e_Response = CMBS_RESPONSE_OK;

    memset(&st_CalledParty, 0, sizeof(st_CalledParty));

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE and CMBS_IE_CALLEDPARTY

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    u32_CallInstance = st_IEInfo.Info.u32_CallInstance;
                    break;

                case CMBS_IE_CALLEDPARTY:
                    st_CalledParty = st_IEInfo.Info.st_CalledParty;
                    break;

                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (st_Response.e_Response != CMBS_RESPONSE_OK)
        {
            printf("app_OnHsLocAnswer Recieved response not OK!. Call Instance %d\n", u32_CallInstance);
            return;
        }

        //TODO  save st_CalledParty to pst_Call  if need
        printf("\napp_OnHsLocAnswer: CallInstance=0x%08X, CalledParty[ prop=%d, pres=%d, len=%d, addr=",
               u32_CallInstance, st_CalledParty.u8_AddressProperties,
               st_CalledParty.u8_AddressPresentation, st_CalledParty.u8_AddressLen);
        for (u8_i = 0; u8_i < st_CalledParty.u8_AddressLen; ++u8_i)
        {
            printf("%c", st_CalledParty.pu8_Address[u8_i]);
        }
        printf(" ]\n");
    }
    else
    {
        printf("app_OnHsLocAnswer: ERROR: invalid IE list\n");
    }
}


//  ========== app_OnHsLocRelease ===========
/*!
        \brief    CMBS Target signal handset locator release
        \param[in]  pvAppRefHandle   application reference pointer
        \param[in]  pv_List        IE list pointer
        \return    <none>
*/
void        app_OnHsLocRelease(void * pvAppRefHandle, void * pv_List)
{
    PST_CALL_OBJ      pst_Call = NULL;
    ST_APPCMBS_IEINFO st_IEInfo;

    void *         pv_IE;
    u16            u16_IE;
    u32            u32_CallInstance = 0;
    ST_IE_RESPONSE st_Response;

    st_Response.e_Response = CMBS_RESPONSE_OK;

    UNUSED_PARAMETER(pvAppRefHandle);

    if (pv_List)
    {
        // collect information elements. we expect:
        // CMBS_IE_CALLINSTANCE

        cmbs_api_ie_GetFirst(pv_List, &pv_IE, &u16_IE);
        while (pv_IE != NULL)
        {
            appcmbs_IEInfoGet(pv_IE, u16_IE, &st_IEInfo);

            switch (u16_IE)
            {
                case CMBS_IE_CALLINSTANCE:
                    u32_CallInstance = st_IEInfo.Info.u32_CallInstance;
                    pst_Call = _appcall_CallObjGet(st_IEInfo.Info.u32_CallInstance, NULL);
                    break;

                case CMBS_IE_RESPONSE:
                    // check response code:
                    cmbs_api_ie_ResponseGet(pv_IE, &st_Response);
                    break;
            }

            cmbs_api_ie_GetNext(pv_List, &pv_IE, &u16_IE);
        }

        if (st_Response.e_Response != CMBS_RESPONSE_OK)
        {
            printf("app_OnHsLocProgress Received response not OK!. Call Instance %d\n", u32_CallInstance);
            return;
        }

        printf("\napp_OnHsLocRelease: CallInstance=0x%08X\n", u32_CallInstance);
    }
    else
    {
        printf("app_OnHsLocRelease: ERROR: invalid IE list\n");
    }

    UNUSED_PARAMETER(pst_Call);
}

//  ========== _appcall_CallerIDSet ===========
/*!
        \brief     convert caller ID from string to IE structure

        \param[in,out]   pst_This     pointer to call object
        \param[in]           pst_Properties   pointer to exchange object
        \param[in]             n_properties     number of containing IEs in exchange object
        \param[out]        pst_CallerParty pointer to IE structure
        \return          <int>                return TRUE, if string was converted

*/
int _appcall_CallerIDSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_CALLERPARTY pst_CallerParty)
{
    int   n_Pos;

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_CALLERPARTY);
    memset(pst_Call->ch_CallerID, 0, sizeof(pst_Call->ch_CallerID));
    if (n_Pos < n_Properties)
    {
        strncpy(pst_Call->ch_CallerID, pst_Properties[n_Pos].psz_Value, (sizeof(pst_Call->ch_CallerID) - 1));
        pst_CallerParty->u8_AddressProperties     = CMBS_ADDR_PROPTYPE_INTERNATIONAL | CMBS_ADDR_PROPPLAN_E164;

        if (pst_Call->ch_CallerID[0] == 'p' || pst_Call->ch_CallerID[0] == 'r')
        {
            /* long format of CLIP: <p/r><CLI> */
            pst_CallerParty->u8_AddressPresentation   = pst_Call->ch_CallerID[0] == 'p' ?
                    CMBS_ADDR_PRESENT_ALLOW : CMBS_ADDR_PRESENT_DENIED;
            pst_CallerParty->u8_AddressLen            = (u8)strlen(pst_Call->ch_CallerID) - 1;
            pst_CallerParty->pu8_Address              = (u8*)(pst_Call->ch_CallerID + 1);
        }
        else if (pst_Call->ch_CallerID[0] == 'n')
        {
            /* format of CLIP: <n> */
            pst_CallerParty->u8_AddressPresentation = CMBS_ADDR_PRESENT_NOTAVAIL;
            pst_CallerParty->u8_AddressLen          = 0;
        }
        else if (pst_Call->ch_CallerID[0] == 'c')
        {
            /* format of CLIP: <c> */
            /* Calling Party Name from Contact list, Number from Network */
            /* Screening indicator for CNIP: User defined, verified and passed,
             * CLIP allowed as well (must be available in that case). */
            pst_CallerParty->u8_AddressPresentation   =
                CMBS_ADDR_PRESENT_ALLOW | CMBS_ADDR_PRESENT_CNIP_USER;
            pst_CallerParty->u8_AddressLen            = (u8)strlen(pst_Call->ch_CallerID) - 1;
            pst_CallerParty->pu8_Address              = (u8*)(pst_Call->ch_CallerID + 1);
        }
        else if (pst_Call->ch_CallerID[0] == 't')
        {
            pst_CallerParty->u8_AddressProperties = CMBS_ADDR_PROPTYPE_NATIONAL | CMBS_ADDR_PROPPLAN_PRIVATE;
            /* long format of CLIP: <p/r><CLI> */
            pst_CallerParty->u8_AddressPresentation   = CMBS_ADDR_PRESENT_ALLOW;
            pst_CallerParty->u8_AddressLen            = (u8)strlen(pst_Call->ch_CallerID) - 1;
            pst_CallerParty->pu8_Address              = (u8*)(pst_Call->ch_CallerID + 1);
        }
        else if (pst_Call->ch_CallerID[0] == 's')
        {
            pst_CallerParty->u8_AddressProperties = CMBS_ADDR_PROPTYPE_NATIONAL | CMBS_ADDR_PROPPLAN_NAT_STD;
            /* long format of CLIP: <p/r><CLI> */
            pst_CallerParty->u8_AddressPresentation   = CMBS_ADDR_PRESENT_ALLOW;
            pst_CallerParty->u8_AddressLen            = (u8)strlen(pst_Call->ch_CallerID) - 1;
            pst_CallerParty->pu8_Address              = (u8*)(pst_Call->ch_CallerID + 1);
        }
        else
        {
            /* short format of CLIP: <CLI> */
            pst_CallerParty->u8_AddressPresentation   = CMBS_ADDR_PRESENT_ALLOW;
            pst_CallerParty->u8_AddressLen            = (u8)strlen(pst_Call->ch_CallerID);
            pst_CallerParty->pu8_Address              = (u8*)(pst_Call->ch_CallerID);
        }


        memcpy(&pst_Call->st_CallerParty, pst_CallerParty, sizeof(pst_Call->st_CallerParty));
        pst_Call->st_CallerParty.pu8_Address   = (u8*)pst_Call->ch_CallerID;
        pst_Call->st_CallerParty.u8_AddressLen = (u8)strlen(pst_Call->ch_CallerID);

        return  TRUE;
    }

    return  FALSE;
}

//  ========== _appcall_LineIDSet ===========
/*!
        \brief     convert Line ID from string to IE structure

        \param[in,out]   pst_This     pointer to call object
        \param[in]             pst_Properties   pointer to exchange object
        \param[in]             n_properties     number of containing IEs in exchange object
        \param[out]        pu8_LineId      pointer to Line Id
        \return          <int>                return TRUE, if string was converted

*/
int _appcall_LineIDSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, u8 * pu8_LineId)
{
    int   n_Pos;
    char* psz_Value;

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_LINE_ID);

    if (n_Pos < n_Properties)
    {
        psz_Value = pst_Properties[n_Pos].psz_Value;

        *pu8_LineId = atoi(psz_Value);
        if (*pu8_LineId >= APPCALL_LINEOBJ_MAX)
        {
            printf("APP_WARNING: Line ID is out of range, and reset to O\n");
            *pu8_LineId = 0;
        }

        _appcall_CallObjLineObjSet(pst_Call, _appcall_LineObjGet(*pu8_LineId));

        return  TRUE;
    }

    return  FALSE;
}



//  ========== _appcall_CalledIDSet ===========
/*!
        \brief     convert caller ID from string to IE structure

        \param[in,out]  pst_This     pointer to call object
        \param[in]        pst_Properties   pointer to exchange object
        \param[in]        n_properties     number of containing IEs in exchange object
        \param[out]   pst_CalledParty pointer to IE structure
        \return         <int>               return TRUE, if string was converted

*/
int _appcall_CalledIDSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_CALLEDPARTY pst_CalledParty)
{
    int   n_Pos;
    char* psz_Value;

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_CALLEDPARTY);
    memset(pst_Call->ch_CalledID, 0, sizeof(pst_Call->ch_CalledID));
    if (n_Pos < n_Properties)
    {
        psz_Value = pst_Properties[n_Pos].psz_Value;
        strncpy(pst_Call->ch_CalledID, psz_Value, (sizeof(pst_Call->ch_CalledID) - 1));

        if (psz_Value[0] == 'h')
        {
            pst_CalledParty->u8_AddressProperties   = CMBS_ADDR_PROPTYPE_UNKNOWN | CMBS_ADDR_PROPPLAN_INTHS;
            pst_CalledParty->u8_AddressPresentation = CMBS_ADDR_PRESENT_ALLOW;
            pst_CalledParty->u8_AddressLen          = (u8)strlen(psz_Value) - 1;
            pst_CalledParty->pu8_Address            = (u8*)(psz_Value + 1);
        }
        else if (psz_Value[0] == 'l')
        {
            pst_CalledParty->u8_AddressProperties   = CMBS_ADDR_PROPTYPE_UNKNOWN | CMBS_ADDR_PROPPLAN_INTLINE;
            pst_CalledParty->u8_AddressPresentation = CMBS_ADDR_PRESENT_ALLOW;
            pst_CalledParty->u8_AddressLen          = 1;
            pst_CalledParty->pu8_Address            = (u8*)(psz_Value + 1);
        }

        memcpy(&pst_Call->st_CalledParty, pst_CalledParty, sizeof(pst_Call->st_CalledParty));
        pst_Call->st_CalledParty.pu8_Address   = (u8*)pst_Call->ch_CalledID;
        pst_Call->st_CalledParty.u8_AddressLen = (u8)strlen(pst_Call->ch_CalledID);

        return  TRUE;
    }

    return  FALSE;
}

//  ========== _appcall_MediaDescriptorSet ===========
/*!
        \brief     convert media descriptor from string to IE structure

        \param[in,out]  pst_This       pointer to call object
        \param[in]        pst_Properties     pointer to exchange object
        \param[in]        n_properties       number of containing IEs in exchange object
        \param[out]   pst_MediaDescr pointer to IE structure
        \return         <int>                 return TRUE, if string was converted

*/
int  _appcall_MediaDescriptorSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_MEDIA_DESCRIPTOR pst_MediaDescr)
{
    int   n_Pos;
    u32   i;
    int   n_CodecPos = 0;
    char  s_Codec[4] = {0};
    E_CMBS_AUDIO_CODEC e_Codec;

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_MEDIADESCRIPTOR);

    pst_MediaDescr->u8_CodecsLength = 0;

    if (n_Pos >= n_Properties)
        return FALSE;

    // parse codecs string with codecs numbers (format: "1,2,3")
    for (i = 0; i <= strlen(pst_Properties[n_Pos].psz_Value); i++)
    {
        if (pst_Properties[n_Pos].psz_Value[i] == ',' || i == strlen(pst_Properties[n_Pos].psz_Value))
        {
            s_Codec[n_CodecPos] = '\0';
            e_Codec = atoi(s_Codec);
            s_Codec[0] = '\0';
            n_CodecPos = 0;

            if (e_Codec > CMBS_AUDIO_CODEC_UNDEF && e_Codec < CMBS_AUDIO_CODEC_MAX)
                pst_MediaDescr->pu8_CodecsList[pst_MediaDescr->u8_CodecsLength++] = e_Codec;
        }
        else
            s_Codec[n_CodecPos++] = pst_Properties[n_Pos].psz_Value[i];
    }

    // check valid first codec
    if (!pst_MediaDescr->pu8_CodecsList[0])
		//pst_MediaDescr->pu8_CodecsList[0] = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;
		pst_MediaDescr->pu8_CodecsList[0] = CMBS_AUDIO_CODEC_PCMU;

    // set first codec as default for the call
    pst_Call->e_Codec = pst_MediaDescr->pu8_CodecsList[0];
	
	ctrl_log_print(1, __FUNCTION__, __LINE__, "_appcall_MediaDescriptorSet pst_Call->e_Codec:%d\n", pst_Call->e_Codec);
	ctrl_log_print(1, __FUNCTION__, __LINE__, "_appcall_MediaDescriptorSet pst_MediaDescr->pu8_CodecsList[0]:%d\n", pst_MediaDescr->pu8_CodecsList[0]);
	ctrl_log_print(1, __FUNCTION__, __LINE__, "_appcall_MediaDescriptorSet pst_MediaDescr->pu8_CodecsList[1]:%d\n", pst_MediaDescr->pu8_CodecsList[1]);
    pst_MediaDescr->e_Codec = pst_Call->e_Codec;

    return TRUE;

}

//  ========== _appcall_CallerNameSet ===========
/*!
        \brief     convert caller name from string to IE structure

        \param[in,out]  pst_This     pointer to call object
        \param[in]        pst_Properties   pointer to exchange object
        \param[in]        n_properties     number of containing IEs in exchange object
        \param[out]   pst_CallerName pointer to IE structure
        \return         <int>                 return TRUE, if string was converted

*/
int  _appcall_CallerNameSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_CALLERNAME pst_CallerName)
{
    int   n_Pos;

    if (pst_Call)
    {
    };

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_CALLERNAME);

    if (n_Pos < n_Properties)
    {
        pst_CallerName->pu8_Name   = (u8*)pst_Properties[n_Pos].psz_Value;
        pst_CallerName->u8_DataLen = (u8)strlen(pst_Properties[n_Pos].psz_Value);

        return TRUE;
    }

    return FALSE;
}


//  ========== _appcall_NBCodecOTASet ===========
/*!
        \brief     convert NB Codec OTA from string to IE structure

        \param[in]          pst_Properties   pointer to exchange object
        \param[in]          n_properties     number of containing IEs in exchange object
        \param[out]          pst_CallerName pointer to IE structure
        \return           <int>                 return TRUE, if string was converted

*/
int  _appcall_NBCodecOTASet(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_NB_CODEC_OTA pst_Codec)
{
    int n_Pos;

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_NB_CODEC_OTA);

    if (n_Pos < n_Properties)
    {
        pst_Codec->e_Codec = (E_CMBS_NB_CODEC_OTA_TYPE) atoi(pst_Properties[n_Pos].psz_Value);

        return TRUE;
    }

    return FALSE;
}

//  ========== _appcall_MelodySet ===========
/*!
        \brief     convert NB Codec OTA from string to IE structure

        \param[in]          pst_Properties   pointer to exchange object
        \param[in]          n_properties     number of containing IEs in exchange object
        \param[out]          pst_CallerName pointer to IE structure
        \return           <int>                 return TRUE, if string was converted

*/
int  _appcall_MelodySet(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , u8 * u8_Melody)
{
    int n_Pos;
    char* psz_Value;

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_MELODY);

    if (n_Pos < n_Properties)
    {
        psz_Value = pst_Properties[n_Pos].psz_Value;

        *u8_Melody = atoi(psz_Value);

        if ((*u8_Melody > 8) || (*u8_Melody < 1))
        {
            printf("APP_WARNING: Melody is out of range, and reset to 1\n");
            *u8_Melody = 1;
        }

        return TRUE;
    }

    return FALSE;
}

//  ========== _appcall_ReleaseReasonSet ===========
/*!
        \brief     convert realease reason from string to IE structure

        \param[in,out]  pst_This     pointer to call object
        \param[in]        pst_Properties   pointer to exchange object
        \param[in]        n_properties     number of containing IEs in exchange object
        \param[out]   pst_Reason       pointer to IE structure
        \return         <int>                 return TRUE, if string was converted

*/
int  _appcall_ReleaseReasonSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_RELEASE_REASON pst_Reason)
{
    int   n_Pos;

    if (pst_Call)
    {
    };

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_CALLRELEASE_REASON);

    if (n_Pos < n_Properties)
    {
        pst_Reason->e_Reason       = atoi(pst_Properties[n_Pos].psz_Value);
        pst_Reason->u32_ExtReason  = 0;

        return TRUE;
    }

    return FALSE;
}

//  ========== _appcall_CallProgressSet ===========
/*!
        \brief     convert call progress from string to IE structure

        \param[in,out]  pst_This     pointer to call object
        \param[in]        pst_Properties   pointer to exchange object
        \param[in]        n_properties     number of containing IEs in exchange object
        \param[out]   pst_CallProgress pointer to IE structure
        \return         <int>                 return TRUE, if string was converted

*/
int  _appcall_CallProgressSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_CALLPROGRESS pst_CallProgress)
{
    int   n_Pos;

    UNUSED_PARAMETER(pst_Call);

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_CALLPROGRESS);

    if (n_Pos < n_Properties)
    {
        pst_CallProgress->e_Progress = cmbs_dbg_String2E_CMBS_CALL_PROGR(pst_Properties[n_Pos].psz_Value);

        return  TRUE;
    }

    return FALSE;
}

//  ========== _appcall_CallDisplaySet ===========
/*!
        \brief     convert caller ID from string to IE structure

        \param[in,out]  pst_This     pointer to call object
        \param[in]        pst_Properties   pointer to exchange object
        \param[in]        n_properties     number of containing IEs in exchange object
        \param[out]   pst_CallInfo     pointer to IE structure
        \return         <int>               return TRUE, if string was converted

*/
int  _appcall_CallDisplaySet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_CALLINFO pst_CallInfo)
{
    int n_Pos;

    UNUSED_PARAMETER(pst_Call);

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_CALLINFO);

    if (n_Pos < n_Properties)
    {
        pst_CallInfo->e_Type     = CMBS_CALL_INFO_TYPE_DISPLAY;
        pst_CallInfo->pu8_Info   = (u8*)pst_Properties[n_Pos].psz_Value;
        // take care on max string length according DECT specification
        pst_CallInfo->u8_DataLen = (u8)strlen(pst_Properties[n_Pos].psz_Value);

        return  TRUE;
    }

    return FALSE;
}

//  ========== _appcall_CallDisplaySet ===========
/*!
        \brief     convert caller ID from string to IE structure

        \param[in,out]  pst_This     pointer to call object
        \param[in]        pst_Properties   pointer to exchange object
        \param[in]        n_properties     number of containing IEs in exchange object
        \param[out]   pst_CallInfo     pointer to IE structure
        \return         <int>               return TRUE, if string was converted

*/
int  _appcall_DisplayStringSet(PST_CALL_OBJ pst_Call, PST_APPCALL_PROPERTIES pst_Properties, int n_Properties , PST_IE_DISPLAY_STRING pst_DisplayString)
{
    int n_Pos;

    UNUSED_PARAMETER(pst_Call);

    n_Pos = _appcall_PropertiesIDXGet(pst_Properties, n_Properties, CMBS_IE_DISPLAY_STRING);

    if (n_Pos < n_Properties)
    {
        pst_DisplayString->pu8_Info   = (u8*)pst_Properties[n_Pos].psz_Value;
        // take care on max string length according DECT specification
        pst_DisplayString->u8_DataLen = (u8)strlen(pst_Properties[n_Pos].psz_Value);

        return  TRUE;
    }

    return FALSE;
}

//  ========== app_CallEntity ===========
/*!
        \brief    dispatcher fo call CMBS events

        \param[in]   pv_AppRef   application reference pointer
        \param[in]   e_EventID   received CMBS event
        \param[in]   pv_EventData  pointer to IE list
        \return    <int>   TRUE, if consumed

*/
int  app_CallEntity(void * pv_AppRef, E_CMBS_EVENT_ID e_EventID, void * pv_EventData)
{
    switch (e_EventID)
    {
        case CMBS_EV_DEE_CALL_ESTABLISH:
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            app_OnCallEstablish(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_PROGRESS:
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            app_OnCallProgress(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_INBANDINFO:
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            app_OnCallInbandInfo(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_ANSWER:
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            app_OnCallAnswer(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_RELEASE:
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            app_OnCallRelease(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_RELEASECOMPLETE:
            app_OnCallReleaseComplete(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_MEDIA_UPDATE:
            app_OnCallMediaUpdate(pv_EventData);
            return TRUE;

        case CMBS_EV_DEM_CHANNEL_START_RES:
            app_OnChannelStartRsp(pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_HOLD:
            app_OnCallHold(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_RESUME:
            app_OnCallResume(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DCM_CALL_STATE:
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            app_OnCallState(pv_AppRef, pv_EventData);
            printf("\n fun:%s line:%d \n", __FUNCTION__, __LINE__);
            return TRUE;

        case CMBS_EV_DCM_CALL_TRANSFER:
            app_OnCallTransfer(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DCM_INTERNAL_TRANSFER:
            app_OnInternalCallTransfer(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DCM_CALL_CONFERENCE:
            app_OnCallConference(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_CALL_MEDIA_OFFER_RES:
            app_OnMediaOfferRes(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEE_HS_CODEC_CFM_FAILED:
            app_OnHsCodecCfmFailed(pv_AppRef, pv_EventData);
            return TRUE;

        case CMBS_EV_DEM_CHANNEL_STOP_RES:
        {
            ST_IE_RESPONSE st_Response;
            st_Response.e_Response = app_ResponseCheck(pv_EventData);
            if (g_cmbsappl.n_Token)
            {
                appcmbs_ObjectSignal((void*)&st_Response , sizeof(ST_IE_RESPONSE), CMBS_IE_RESPONSE, e_EventID);
            }
            break;
        }
        return TRUE;

        default:
            return FALSE;
    }

    return FALSE;
}

//  ========== appcall_ReleaseCall ===========
/*!
        \brief   release call identified by Call ID or caller party
        \param[in]  pst_Properties pointer to exchange object
        \param[in]  n_Properties  number of containing IEs
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_CLI        Caller Party string
        \return   <int>             TRUE, if successful
*/
int      appcall_ReleaseCall(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ   pst_Call;

    if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }

    if (pst_Call)
    {
        void *         pv_RefIEList = NULL;

        ST_IE_RELEASE_REASON st_Reason;

        printf("Release Call\n");

        appmedia_CallObjMediaStop(pst_Call->u32_CallInstance, 0, NULL);

        cmbsevent_onCallReleased(_appcall_CallObjIdGet(pst_Call));

        pv_RefIEList = cmbs_api_ie_GetList();
        if (pv_RefIEList)
        {
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            // Add release reason IE
            _appcall_ReleaseReasonSet(pst_Call, pst_Properties, n_Properties , &st_Reason);

            cmbs_api_ie_CallReleaseReasonAdd(pv_RefIEList, &st_Reason);

            cmbs_dee_CallRelease(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            return TRUE;
        }
    }

    return FALSE;
}

//  ========== appcall_EstablishCall ===========
/*!
        \brief   establish incoming call
        \param[in]  pst_Properties pointer to exchange object
        \param[in]  n_Properties  number of containing IEs
        \return   <u16>                Call ID, or ~0  if failed!
*/
u16 appcall_EstablishCall(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties)
{
    PST_CALL_OBJ            pst_Call;
    void *                  pv_RefIEList = NULL;
    ST_IE_CALLEDPARTY       st_CalledParty;
    ST_IE_CALLERPARTY       st_CallerParty;
    ST_IE_CALLERNAME        st_CallerName;
    ST_IE_MEDIA_DESCRIPTOR  st_MediaDescr;
    ST_IE_NB_CODEC_OTA      st_NBCodecOTA;
    u8                      u8_Melody;
    u8                      u8_LineId = 0xFF;

    pst_Call = _appcall_CallObjNew();
    if (! pst_Call)
    {
        return ~0;
    }

    /* Mark call as incoming */
    pst_Call->b_Incoming = TRUE;

    // Create a new Call ID
    pst_Call->u32_CallInstance = cmbs_dee_CallInstanceNew(g_cmbsappl.pv_CMBSRef);

    // Initialize IE List
    pv_RefIEList = cmbs_api_ie_GetList();
    if (pv_RefIEList)
    {
        // Add call Instance IE
        cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

        // Add Line Id
        _appcall_LineIDSet(pst_Call, pst_Properties, n_Properties, &u8_LineId);
        cmbs_api_ie_LineIdAdd(pv_RefIEList, u8_LineId);

        // Add called ID IE
        _appcall_CalledIDSet(pst_Call, pst_Properties, n_Properties, &st_CalledParty);
        cmbs_api_ie_CalledPartyAdd(pv_RefIEList, &st_CalledParty);

        // Add caller ID IE
        _appcall_CallerIDSet(pst_Call, pst_Properties, n_Properties , &st_CallerParty);
        cmbs_api_ie_CallerPartyAdd(pv_RefIEList, &st_CallerParty);

        // Add caller name IE
        if (_appcall_CallerNameSet(pst_Call, pst_Properties, n_Properties , &st_CallerName))
        {
            cmbs_api_ie_CallerNameAdd(pv_RefIEList, &st_CallerName);
        }

        // Add media descriptor IE
        _appcall_MediaDescriptorSet(pst_Call, pst_Properties, n_Properties , &st_MediaDescr);
        cmbs_api_ie_MediaDescAdd(pv_RefIEList, &st_MediaDescr);

        // NB Codec OTA
        if (_appcall_NBCodecOTASet(pst_Properties, n_Properties, &st_NBCodecOTA))
        {
            cmbs_api_ie_NBOTACodecAdd(pv_RefIEList, &st_NBCodecOTA);
        }

        // Alerting pattern OTA
        if (_appcall_MelodySet(pst_Properties, n_Properties, &u8_Melody))
        {
            cmbs_api_ie_MelodyAdd(pv_RefIEList, u8_Melody);
        }

        // Establish the call now...
        cmbs_dee_CallEstablish(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

        return  _appcall_CallObjIdGet(pst_Call);
    }

    return ~0;
}

//  ========== appcall_ProgressCall ===========
/*!
        \brief   providing progress inforation of call identified by Call ID or caller party
        \param[in]  pst_Properties pointer to exchange object
        \param[in]  n_Properties  number of containing IEs
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_CLI        Caller Party string
        \return   <int>             TRUE, if successful
*/
int      appcall_ProgressCall(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ         pst_Call;
    ST_IE_CALLPROGRESS   st_CallProgress;

    if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            ST_IE_MEDIA_DESCRIPTOR st_MediaDesc;

            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);
            // answer the call: send CMBS_IE_CALLINSTANCE, CMBS_IE_MEDIACHANNEL and CMBS_IE_MEDIADESCRIPTOR

            _appcall_CallProgressSet(pst_Call, pst_Properties, n_Properties , &st_CallProgress);

            if (pst_Call->b_CodecsOfferedToTarget == FALSE)
            {
                // add media descriptor IE
                memset(&st_MediaDesc, 0, sizeof(ST_IE_MEDIA_DESCRIPTOR));
                st_MediaDesc.e_Codec = pst_Call->e_Codec;
                cmbs_api_ie_MediaDescAdd(pv_RefIEList, &st_MediaDesc);

                pst_Call->b_CodecsOfferedToTarget = TRUE;
            }

            cmbs_api_ie_CallProgressAdd(pv_RefIEList, &st_CallProgress);

            /* Add Line-Id */
            cmbs_api_ie_LineIdAdd(pv_RefIEList, pst_Call->u8_LineId);

            cmbs_dee_CallProgress(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            switch (st_CallProgress.e_Progress)
            {
                case CMBS_CALL_PROGR_SETUP_ACK:
                    _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_OUT_PEND);
                    break;
                case CMBS_CALL_PROGR_PROCEEDING:
                    _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_OUT_PROC);
                    break;
                case CMBS_CALL_PROGR_RINGING :
                    _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_OUT_RING);
                    appmedia_CallObjTonePlay("CMBS_TONE_RING_BACK\0", TRUE, _appcall_CallObjIdGet(pst_Call), NULL);
                    break;
                case CMBS_CALL_PROGR_INBAND:
                    _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_OUT_INBAND);
                    break;
                default:
                    break;
            }
        }
    }

    return TRUE;
}

//  ========== appcall_AnswerCall ===========
/*!
        \brief   answer call identified by Call ID or caller party
        \param[in]  pst_Properties pointer to exchange object
        \param[in]  n_Properties  number of containing IEs
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_CLI        Caller Party string
        \return   <int>             TRUE, if successful
*/
int      appcall_AnswerCall(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ         pst_Call;
	int i = 0;
    if (pst_Properties && n_Properties)
    {
    };

    if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }
	appmedia_CallObjTonePlay("CMBS_TONE_RING_BACK\0", FALSE, _appcall_CallObjIdGet(pst_Call), NULL);

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        cmbsevent_onCallAnswered(_appcall_CallObjIdGet(pst_Call));

        if (pv_RefIEList)
        {
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);
            // answer the call: send CMBS_IE_CALLINSTANCE, CMBS_IE_MEDIACHANNEL and CMBS_IE_MEDIADESCRIPTOR

            cmbs_dee_CallAnswer(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            _appcall_CallObjStateSet(pst_Call, E_APPCMBS_CALL_ACTIVE);

            if (g_call_automat)
            {
                appmedia_CallObjMediaStart(pst_Call->u32_CallInstance, 0, 0xFF, NULL);
            }
			for (i = 0; i < 2; i++)
			{
				APPCMBS_INFO(("APP_Dongle-CALL: INFO1 ----------------------\n"));
				appcall_InfoCall(i);
			}
			

            return  TRUE;
        }
    }
	for (i = 0; i < 2; i++)
    {
        APPCMBS_INFO(("APP_Dongle-CALL: INFO2 ----------------------\n"));
        appcall_InfoCall(i);
    }

    return FALSE;
}

//  ========== appcall_DisplayCall ===========
/*!
        \brief   provide display information of a call identified by Call ID or caller party
        \param[in]  pst_Properties pointer to exchange object
        \param[in]  n_Properties  number of containing IEs
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_CLI        Caller Party string
        \return   <int>             TRUE, if successful
*/
int      appcall_DisplayCall(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ   pst_Call;

    if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        printf("CW Display\n");

        if (pv_RefIEList)
        {
            ST_IE_CALLINFO st_CallInfo;
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            _appcall_CallDisplaySet(pst_Call, pst_Properties, n_Properties, &st_CallInfo);
            cmbs_api_ie_CallInfoAdd(pv_RefIEList, &st_CallInfo);

            cmbs_dee_CallInbandInfo(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            return  TRUE;
        }
    }

    return FALSE;
}

//  ========== appcall_DisplayString ===========
/*!
        \brief   provide display information
        \param[in]  pst_Properties pointer to exchange object
        \param[in]  n_Properties  number of containing IEs
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_Display        Display string
        \return   <int>             TRUE, if successful
*/
int      appcall_DisplayString(PST_APPCALL_PROPERTIES pst_Properties, int n_Properties, u16 u16_CallId, char * psz_Display)
{
    PST_CALL_OBJ   pst_Call;

    if (! psz_Display)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_Display);
    }

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        printf(" Display String\n");

        if (pv_RefIEList)
        {
            ST_IE_DISPLAY_STRING st_DisplayString;
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            _appcall_DisplayStringSet(pst_Call, pst_Properties, n_Properties, &st_DisplayString);
            cmbs_api_ie_DisplayStringAdd(pv_RefIEList, &st_DisplayString);

            cmbs_dee_CallInbandInfo(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            return  TRUE;
        }
    }

    return FALSE;
}

//  ========== appcall_CallInbandInfo ===========
/*!
        \brief   Host changes Caller ID (for transfer) of an active call
        \param[in]  u16_CallId    relevant Call ID
        \param[in]  psz_CLI        new Caller Party string or CNIP string
        \return   <int>             TRUE, if successful
*/
int appcall_CallInbandInfo(u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ pst_Call = g_call_obj + u16_CallId;

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            ST_IE_CALLINFO st_CallInfo;
            st_CallInfo.e_Type = CMBS_CALL_INFO_TYPE_DISPLAY;
            st_CallInfo.pu8_Info = (u8*)psz_CLI;
            st_CallInfo.u8_DataLen = 2 + strlen(psz_CLI + 2);

            cmbs_api_ie_CallInfoAdd(pv_RefIEList, &st_CallInfo);
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            cmbs_dee_CallInbandInfo(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            /* Update Host CLIP */
            if (g_call_obj[u16_CallId].b_Incoming)
            {
                strcpy(g_call_obj[u16_CallId].ch_CallerID, psz_CLI + 2);
            }
            else
            {
                strcpy(g_call_obj[u16_CallId].ch_CalledID, psz_CLI + 2);
            }

            return  TRUE;
        }
    }

    return  FALSE;
}

//  ========== appcall_CallInbandInfoCNIP ===========
/*!
        \brief   CNIP (matching Contact number) during external outgoing call establishment
        \param[in]  u16_CallId    relevant Call ID
        \param[in]  psz_CLI        new Called Party name and first name string
        \return   <int>             TRUE, if successful
*/
int appcall_CallInbandInfoCNIP(u16 u16_CallId, char * pu8_Name, char * pu8_FirstName, char * pch_cli)
{
    PST_CALL_OBJ pst_Call = g_call_obj + u16_CallId;

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            ST_IE_CALLEDNAME st_CalledName;
            ST_IE_CALLERPARTY st_CalledParty;

            if (pch_cli)
            {
                /* Called Number is available */
                // Add called ID IE
                st_CalledParty.u8_AddressProperties   = CMBS_ADDR_PROPTYPE_UNKNOWN | CMBS_ADDR_PROPPLAN_UNKNOWN;
                st_CalledParty.u8_AddressPresentation = CMBS_ADDR_PRESENT_ALLOW;
                st_CalledParty.u8_AddressLen          = (u8)strlen(pch_cli);
                st_CalledParty.pu8_Address            = (u8*)(pch_cli);

                cmbs_api_ie_CalledPartyAdd(pv_RefIEList, &st_CalledParty);
            }


            st_CalledName.u8_AlphabetAndScreening = *pu8_Name;
            st_CalledName.pu8_Name        = ++pu8_Name;
            st_CalledName.u8_DataLen      = strlen(pu8_Name);
            st_CalledName.u8_DataLenFirst = strlen(pu8_FirstName);
            st_CalledName.pu8_FirstName   = pu8_FirstName;

            cmbs_api_ie_CalledNameAdd(pv_RefIEList, &st_CalledName);

            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            cmbs_dee_CallInbandInfo(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            return  TRUE;
        }
    }

    return  FALSE;
}

//  ========== appcall_HoldCall ===========
/*!
        \brief   signal CMBS target call hold, mute media stream
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_CLI        Caller Party string
        \return   <int>             TRUE, if successful
*/
int     appcall_HoldCall(u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ   pst_Call;

    if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            cmbs_dee_CallHold(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            return  TRUE;
        }
    }

    return  FALSE;
}

//  ========== appcall_ResumeCall ===========
/*!
        \brief   signal CMBS target call resume, unmute media stream
        \param[in]  u16_CallId        relevant Call ID
        \param[in]  psz_CLI        Caller Party string
        \return   <int>             TRUE, if successful
*/
int     appcall_ResumeCall(u16 u16_CallId, char * psz_CLI)
{
    PST_CALL_OBJ   pst_Call;

    if (! psz_CLI)
    {
        // Call ID is available
        pst_Call = g_call_obj + u16_CallId;
    }
    else
    {
        pst_Call = _appcall_CallObjGet(0, psz_CLI);
    }

    if (pst_Call)
    {
        void*  pv_RefIEList = cmbs_api_ie_GetList();

        if (pv_RefIEList)
        {
            // Add call Instance IE
            cmbs_api_ie_CallInstanceAdd(pv_RefIEList, pst_Call->u32_CallInstance);

            cmbs_dee_CallResume(g_cmbsappl.pv_CMBSRef, pv_RefIEList);

            return  TRUE;
        }
    }

    return  FALSE;
}

void     appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE e_Mode)
{
    if (e_Mode == E_APPCALL_AUTOMAT_MODE_ON)
    {
        printf("Switch application to automat mode\n");
    }
    else
    {
        printf("Switch application to step mode\n");
    }

    g_call_automat = e_Mode;
}

// accept presentations templates "1", "3", "h1", "h3"
bool appcall_DestinationIsHs(u8 u8_HsNumber, ST_IE_CALLEDPARTY * pst_Caller)
{
    if (((pst_Caller->u8_AddressProperties & CMBS_ADDR_PROPPLAN_MASK) == CMBS_ADDR_PROPPLAN_INTHS) &&
            (pst_Caller->pu8_Address != NULL))
    {
        if (pst_Caller->u8_AddressLen == 1)
        {
            if ((pst_Caller->pu8_Address[0] - '0') == u8_HsNumber)
            {
                return TRUE;
            }
        }
        else if ((pst_Caller->u8_AddressLen == 2) && (pst_Caller->pu8_Address[0] == 'h'))
        {
            if ((pst_Caller->pu8_Address[1] - '0') == u8_HsNumber)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

bool appcall_IsHsInCallWithLine(u8 u8_HsNumber, u8 u8_LineId)
{
    u16 i;

    for (i = 0; i < APPCALL_CALLOBJ_MAX; i++)
    {
        if (g_call_obj[i].u32_CallInstance)
        {
            // does u8_HsNumber involved in call?
            if (appcall_DestinationIsHs(u8_HsNumber, &(g_call_obj[i].st_CallerParty)) ||
                    appcall_DestinationIsHs(u8_HsNumber, &(g_call_obj[i].st_CalledParty)))
            {
                // is this our line?
                if (g_call_obj[i].u8_LineId == u8_LineId)
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

bool  appcall_IsLineInUse(u8 u8_LineId)
{
    int i;
    for (i = 0; i < APPCALL_CALLOBJ_MAX; i++)
    {
        if ((g_call_obj[i].u32_CallInstance) && (g_call_obj[i].u8_LineId == u8_LineId))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//*/

