/*!
* \file   keyb_call.c
* \brief
* \Author  kelbch
*
* @(#) %filespec: keyb_call.c~NBGD53#48 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================
* date      name     rev     action
* --------------------------------------------------------------------------
* 07-Mar-2014 tcmc_asa  ---GIT--   added CMBS_INBAND_ITALY
* 12-Jun-2013 tcmc_asa - GIT - merge 2.99.x changes to 3.x branch
* 28-Jan-2013 tcmc_asa 40      added keyb_CallInbandCNIP() PR 3615
* 24-Jan-2013 tcmc_asa 39      added keyb_SetHostCodec(E_APPCALL_SWITCH_RECEIVED_CODEC)
* 24-May-2013 tcmc_asa 48      added Congestion tone (swiss)
* 03-May-2013 tcmc_asa 47      change inband tone handling to new layout
* 30-Apr-2013 tcmc_asa 46      added swiss tones
* 23-Jul-2012 tcmc_asa 42      added further OUTBAND tones, PR 3320
* 24-May-2012 tcmc_asa 38      added CMBS_TONE_CALL_WAITING_OUTBAND, PR 3205
* 22-May-2012 tcmc_asa 37      added CLIP type 's' in keyb_CallEstablish
* 22-May-2012 tcmc_asa 36      added keyb_MediaAutoSwitch()
* 16-Feb-2012 tcmc_asa 28      added CLIP type 't' in keyb_CallEstablish
* 13-Feb-2012 tcmc_asa 27      added further french and polish tones
* 02-Feb-2012 tcmc_asa 26      added CMBS_TONE_CALL_WAITING_FT_FR
* 02-Feb-2012 tcmc_asa 25      added CNIP from Contact list case (only new printf)
* 12-Dec-2011 tcmc_asa 22      added CLIR changes
* 18-sep-09  Kelbch  pj1029-479    add quick call demonstration component
*******************************************************************************/

#if !defined( KEYB_CALL_H )
#define KEYB_CALL_H


#if defined( __cplusplus )
extern "C"
{
#endif

#if defined( __cplusplus )
}
#endif

#endif // KEYB_CALL_H
//*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cmbs_int.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "tcx_keyb.h"
#include <tcx_util.h>

#ifdef WIN32
#define kbhit _kbhit
#else
extern int kbhit(void);
#endif

#define KEYB_CALL_INVALID_CALLID 0xFFFF

/***************************************************************
*
* external defined function for a quick demonstration
*
****************************************************************/

extern void appcall_InfoCall(int n_Call);

extern u8 g_HoldResumeCfm;
extern u8 g_HoldCfm;

extern u8 g_TransferAutoCfm;
extern u8 g_ConfAutoCfm;

extern u8 g_EarlyMediaAutoCfm;
extern E_APPCALL_PREFERRED_CODEC g_HostCodec;

extern PST_CALL_OBJ _appcall_CallObjGetById(u16 u16_CallId);

extern void appmedia_CallObjMediaInternalConnect(int channel, int context, int connect);

extern void  keyb_AddPhoneABToContactList(void);

extern void  keyb_AddLine0ToLineSettingsList(void);

void keyb_CallAnswer(void);

/***************************************************************
*
* global variable
*
****************************************************************/

u16     g_u16_DemoCallId = APPCALL_NO_CALL;


u16     g_u16_PhoneACallId = APPCALL_NO_CALL;
u16     g_u16_PhoneBCallId = APPCALL_NO_CALL;
u16     g_u16_PhoneCCallId = APPCALL_NO_CALL;

extern u8 g_u8_ToneType;

void  keyb_ReleaseNotify(u16 u16_CallId)
{
if (u16_CallId == g_u16_DemoCallId)
{
    g_u16_DemoCallId = APPCALL_NO_CALL;
}
if (u16_CallId == g_u16_PhoneACallId)
{
    g_u16_PhoneACallId = APPCALL_NO_CALL;
}
if (u16_CallId == g_u16_PhoneBCallId)
{
    g_u16_PhoneBCallId = APPCALL_NO_CALL;
}
if (u16_CallId == g_u16_PhoneCCallId)
{
    g_u16_PhoneCCallId = APPCALL_NO_CALL;
}

}

//  ==========  keyb_CallInfo ===========
/*!
  \brief     print the call information for demonstration line

  \param[in,out]   <none>

  \return     <none>

*/

void    keyb_CallInfo(void)
{
if (g_u16_DemoCallId != APPCALL_NO_CALL)
{
    appcall_InfoCall(g_u16_DemoCallId);
}
}

//  ==========  keyb_IncCallWB ===========
/*!
  \brief     starts an incoming WB call for demonstration line\n
         in a active call it launch codec change to WB

  \param[in,out]   <none>

  \return     <none>

   The demonstration line uses the automat of appcall component\n
  CLI            : 1234
   Ringing Pattern: standard
   Handsets       : 12345
   CNAME     : CMBS WB
*/

void keyb_IncCallWB(void)
{
ST_APPCALL_PROPERTIES st_Properties[6];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};

// put codecs priority (WB, NB)
sprintf(s_codecs, "%d,%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_WB), (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_DemoCallId == APPCALL_NO_CALL)
{

    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "1234\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "Call WB\0",
                     st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "1\0";
    st_Properties[5].e_IE      = CMBS_IE_MELODY;
    st_Properties[5].psz_Value = "1\0";

    g_u16_DemoCallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_DemoCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Switch to wide band audio\n");
    appmedia_CallObjMediaOffer(g_u16_DemoCallId, 'w');
}
}

//  ==========  keyb_IncCallNB ===========
/*!
  \brief     starts an incoming NB call for demonstration line\n
         in a active call it launch codec change to NB

  \param[in,out]   <none>

  \return     <none>

   The demonstration line uses the automat of appcall component\n
  CLI            : 5678
   Ringing Pattern: standard
   Handsets       : 12345
   CNAME     : CMBS NB
*/

void keyb_IncCallNB(void)
{
ST_APPCALL_PROPERTIES st_Properties[6];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_DemoCallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "p5678\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "Call NB\0";
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "1\0";
    st_Properties[5].e_IE      = CMBS_IE_MELODY;
    st_Properties[5].psz_Value = "1\0";

    g_u16_DemoCallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_DemoCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Switch to wideband audio\n");
    appmedia_CallObjMediaOffer(g_u16_DemoCallId, 'n');
}

}

//  ==========  keyb_IncCallNB_G711A_OTA ===========
/*!
  \brief      starts an incoming NB call using G.711 A-law OTA (requires support in PP)\n
     in a active call it launch codec change to NB and G.711 A-law OTA

  \param[in,out]   <none>

  \return     <none>
*/
void keyb_IncCallNB_G711A_OTA(void)
{
ST_APPCALL_PROPERTIES st_Properties[7];
char s_codecs[2] = {0};
char s_codecsOTA[2] = {0};
sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));
sprintf(s_codecsOTA, "%d", (CMBS_NB_CODEC_OTA_G711A));

if (g_u16_DemoCallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "p5678\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "G.711 OTA\0";
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "0\0";
    st_Properties[5].e_IE      = CMBS_IE_NB_CODEC_OTA;
    st_Properties[5].psz_Value = s_codecsOTA;
    st_Properties[6].e_IE      = CMBS_IE_MELODY;
    st_Properties[6].psz_Value = "1\0";

    g_u16_DemoCallId = appcall_EstablishCall(st_Properties, 6);
    if (g_u16_DemoCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Switch to NB Linear PCM16 with G.711 A-law OTA\n");
    appmedia_CallObjMediaOffer(g_u16_DemoCallId, 'g');
}
}

//  ==========  keyb_IncCallWB_aLAW_mLAW ===========
/*!
  \brief     starts an incoming WB call for demonstration line\n
         in a active call it launch codec change to WB

  \param[in,out]   <none>

  \return     <none>

   The demonstration line uses the automat of appcall component\n
  CLI            : 1234
   Ringing Pattern: standard
   Handsets       : 12345
   CNAME     : CMBS WB
*/

void    keyb_IncCallWB_aLAW_mLAW(void)
{
ST_APPCALL_PROPERTIES st_Properties[5];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};

// put codecs priority (WB, NB)
sprintf(s_codecs, "%d,%d", (CMBS_AUDIO_CODEC_PCMA_WB), (CMBS_AUDIO_CODEC_PCMU_WB));

if (g_u16_DemoCallId == APPCALL_NO_CALL)
{

    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "p1234\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "Call WB_aLaw_mLaw\0";
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "1\0";

    g_u16_DemoCallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_DemoCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Switch to wide band alaw\\mlaw audio\n");
    appmedia_CallObjMediaOffer(g_u16_DemoCallId, 'a');
}

}

//  ==========  keyb_IncCallRelease ===========
/*!
  \brief     release call on demonstration line

  \param[in,out]   <none>

  \return     <none>

*/

void    keyb_IncCallRelease(void)
{
ST_APPCALL_PROPERTIES st_Properties;

if (g_u16_DemoCallId != APPCALL_NO_CALL)
{
    st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
    st_Properties.psz_Value = "0\0";

    appcall_ReleaseCall(&st_Properties, 1, g_u16_DemoCallId, NULL);
    g_u16_DemoCallId = APPCALL_NO_CALL;
}
}


//  ==========  keyb_IncCall_DA_PhA_WB ===========
/*!
  \brief     starts an incoming WB call for demonstration line\n
         in a active call it launch codec change to WB

  \param[in,out]   <none>

  \return     <none>

   The demonstration line uses the automat of appcall component\n
  CLI            : 1234
   Ringing Pattern: standard
   Handsets       : 12345
   CNAME     : CMBS WB
*/

void    keyb_IncCall_DA_PhA_WB(void)
{
ST_APPCALL_PROPERTIES st_Properties[5];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};

// put codecs priority (WB, NB)
sprintf(s_codecs, "%d,%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_WB), (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_PhoneACallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "121\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = "h12\0";
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "Phone A\0";
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "0\0";

    g_u16_PhoneACallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_PhoneACallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Phone A already in a call!\n");
}
}


void    keyb_IncCall_DA_PhA_NB(void)
{
ST_APPCALL_PROPERTIES st_Properties[5];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_PhoneACallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "121\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "Phone A\0";
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "0\0";

    g_u16_PhoneACallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_PhoneACallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Phone A already in a call!\n");
}

}

void keyb_IncCall_DA_PhB_WB(void)
{
ST_APPCALL_PROPERTIES st_Properties[5];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
char s_PhoneBCNIP[] = {0x50, 0x68, 0x6F, 0x6E, 0x65, 0x20, 0x42, 0x20, 0xC3, 0xA9, 0xC3, 0xA0, 0xC3, 0xBC, 0x00}; //Phone B + (UTF-8 characters)

// put codecs priority (WB, NB)
sprintf(s_codecs, "%d,%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_WB), (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_PhoneBCallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "120\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = s_PhoneBCNIP;
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "0\0";

    g_u16_PhoneBCallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_PhoneBCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Phone A already in a call!\n");
}
}


void keyb_IncCall_DA_PhB_NB(void)
{
ST_APPCALL_PROPERTIES st_Properties[5];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
char s_PhoneBCNIP[] = {0x50, 0x68, 0x6F, 0x6E, 0x65, 0x20, 0x42, 0x20, 0xC3, 0xA9, 0xC3, 0xA0, 0xC3, 0xBC, 0x00}; //Phone B + (UTF-8 characters)
sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_PhoneBCallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "120\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = s_PhoneBCNIP;
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "1\0";

    g_u16_PhoneBCallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_PhoneBCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Phone A already in a call!\n");
}
}

void    keyb_IncCall_DA_PhC_NB(void)
{
ST_APPCALL_PROPERTIES st_Properties[5];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));

if (g_u16_PhoneCCallId == APPCALL_NO_CALL)
{
    appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

    st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
    st_Properties[0].psz_Value = "1002\0";
    st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
    st_Properties[1].psz_Value = ALL_HS_STRING;
    st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
    st_Properties[2].psz_Value = s_codecs;
    st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
    st_Properties[3].psz_Value = "Phone C\0";
    st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
    st_Properties[4].psz_Value = "0\0";

    g_u16_PhoneCCallId = appcall_EstablishCall(st_Properties, 5);
    if (g_u16_PhoneCCallId == APPCALL_NO_CALL)
    {
        printf("Call can not be set-up!\n");
    }
} else
{
    printf("Phone A already in a call!\n");
}
}

void    keyb_IncCallRelease_PhA(void)
{
ST_APPCALL_PROPERTIES st_Properties;

if (g_u16_PhoneACallId != APPCALL_NO_CALL)
{
    st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
    st_Properties.psz_Value = "0\0";

    appcall_ReleaseCall(&st_Properties, 1, g_u16_PhoneACallId, NULL);
    g_u16_PhoneACallId = APPCALL_NO_CALL;
}
}

void    keyb_IncCallRelease_PhB(void)
{
ST_APPCALL_PROPERTIES st_Properties;

if (g_u16_PhoneBCallId != APPCALL_NO_CALL)
{
    st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
    st_Properties.psz_Value = "0\0";

    appcall_ReleaseCall(&st_Properties, 1, g_u16_PhoneBCallId, NULL);
    g_u16_PhoneBCallId = APPCALL_NO_CALL;
}
}

void    keyb_IncCallRelease_PhC(void)
{
ST_APPCALL_PROPERTIES st_Properties;

if (g_u16_PhoneCCallId != APPCALL_NO_CALL)
{
    st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
    st_Properties.psz_Value = "0\0";

    appcall_ReleaseCall(&st_Properties, 1, g_u16_PhoneCCallId, NULL);
    g_u16_PhoneCCallId = APPCALL_NO_CALL;
}
}

//      ========== keyb_SetHostCodec===========
/*!
  \brief              set host codec
  \param[in,out]      <prefered codec>
  \return             <none>

*/
void    keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC e_PreferredCodec)
{
g_HostCodec = e_PreferredCodec;
}



//  ==========  keyb_DandATests ===========
/*!
  \brief     Helps to Perform D&A TESTS

  \param[in,out]   <none>

  \return     <none>

*/

void    keyb_DandATests(void)
{
int n_Keep = TRUE;

while (n_Keep)
{
    //      tcx_appClearScreen();

    printf("\n======Dosch & Amand testing menu==========================\n\n");
    printf("Registration Window %s\n\n", g_cmbsappl.RegistrationWindowStatus ? "OPEN" : "CLOSED");
    appcall_InfoPrint();
    printf("\n======Call Control===============================\n\n");
    printf("a => G.722 call from Phone A\n");
    printf("s => G.726 call from Phone A\n");
    printf("b => G.722 call from Phone B\n");
    printf("n => G.726 call from Phone B\n");
    printf("c => G.726 call from Phone C\n");

    printf("A => release call from Phone A\n");
    printf("B => release call from Phone B\n");
    printf("C => release call from Phone C\n");

    printf("e => Call Answer\n");

    printf("r => Open Registration Window\n");

    printf("\n------------------------------------------\n\n");
    printf("i => call/line infos \n");
    printf("I => Insert Phone A, Phone B to contact list \n");
    printf("L => Insert Line 0 to line settings list \n");
    printf("------------------------------------------\n");
    printf("Set host preferred CODEC \n");
    printf("   6 => switch received codec \n");
    printf("   7 => use received codec \n");
    printf("   8 => WB \n");
    printf("   9 => NB \n");
    printf("------------------------------------------\n");
    printf("Set CODEC of call Id 0/1\n");
    printf("   1 => change codec to NB, call id 0\n");
    printf("   2 => change codec to WB, call id 0\n");
    printf("   3 => change codec to NB, call id 1\n");
    printf("   4 => change codec to WB, call id 1\n");
    printf("------------------------------------------\n");
    printf("q => return to interface \n");

    switch (tcx_getch())
    {
        case ' ':
            tcx_appClearScreen();
            break;

        case 'a':
            keyb_IncCall_DA_PhA_WB();
            break;
        case 's':
            keyb_IncCall_DA_PhA_NB();
            break;

        case 'b':
            keyb_IncCall_DA_PhB_WB();
            break;

        case 'n':
            keyb_IncCall_DA_PhB_NB();
            break;

        case 'c':
            keyb_IncCall_DA_PhC_NB();
            break;

        case 'A':
            keyb_IncCallRelease_PhA();
            break;

        case 'B':
            keyb_IncCallRelease_PhB();
            break;

        case 'C':
            keyb_IncCallRelease_PhC();
            break;

        case 'i':
            // call/line info
            tcx_appClearScreen();
            appcall_InfoPrint();
            printf("Press Any Key\n ");
            tcx_getch();
            break;

        case 'I':
            keyb_AddPhoneABToContactList();
            break;

        case 'L':
            keyb_AddLine0ToLineSettingsList();
            break;

        case 'e':
            keyb_CallAnswer();
            break;

        case 'r':
            app_SrvSubscriptionOpen(120);
            break;

        case '6':
            keyb_SetHostCodec(E_APPCALL_SWITCH_RECEIVED_CODEC);
            break;

        case '7':
            keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC_RECEIVED_CODEC);
            break;

        case '8':
            keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC_WB);
            break;

        case '9':
            keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC_NB);
            break;

        case '1':
            appmedia_CallObjMediaOffer(0, 'n');
            break;

        case '2':
            appmedia_CallObjMediaOffer(0, 'w');
            break;

        case '3':
            appmedia_CallObjMediaOffer(0, 'n');
            break;

        case '4':
            appmedia_CallObjMediaOffer(0, 'w');
            break;

        case 'q':
            n_Keep = FALSE;
            break;
    }
}
}



//  ========== _keyb_LineIdInput  ===========
/*!
  \brief input Line ID

  \param[in]   <none>

  \return   <u16> return Line ID in binary form

*/
u16   _keyb_LineIdInput(void)
{
u32 u32_LineID;

printf("Enter Line ID - Line ID 0xFF means no line ID selected ");
tcx_scanf("%u", &u32_LineID);

return (u16)u32_LineID;
}

//  ========== _keyb_CallIdInput  ===========
/*!
  \brief input Call ID

  \param[in]   <none>

  \return   <u16> return Call ID in binary form

*/
u16   _keyb_CallIdInput(void)
{
u32 u32_CallId;
PST_CALL_OBJ         pst_Call;

printf("Enter Call ID ");
tcx_scanf("%u", &u32_CallId);

pst_Call = _appcall_CallObjGetById(u32_CallId);
if (pst_Call)
    if (pst_Call->u32_CallInstance)
        return (u16)u32_CallId;

return KEYB_CALL_INVALID_CALLID;
}

//  ========== _keyb_SlotInput  ===========
/*!
  \brief Input IOM Slot Number, 8 bits as one slot

  \param[in]   <none>

  \return   <u16> return Slot Number

*/
u16   _keyb_SlotInput(void)
{
u16 u16_Slot;
char InputBuffer[4];

printf("Enter IOM Slot Number (8Bits as a slot):");

memset(InputBuffer, 0, sizeof(InputBuffer));
tcx_gets(InputBuffer, sizeof(InputBuffer));
u16_Slot = (u16)atoi(InputBuffer);
return u16_Slot;
}

char ch_caller_party[20];

//  ========== keyb_CallEstablish ===========
/*!
  \brief     establish a call from Host to Target {INCOMING Call from view of CMBS Target}
  \param[in,out]   <none>
  \return     <none>

   CMBS_IE_CALLERPARTY, CMBS_CALLEDPARTY, CMBS_IE_MEDIADESCRIPTOR

*/
void     keyb_CallEstablish(void)
{
ST_APPCALL_PROPERTIES st_Properties[6];
int  n_Prop = 0;
char ch_cli[40];
char ch_cni[40];
char ch_cld[20];
char ch_clineid[20];
char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
char codec;
char ch_melody[20];

printf("Enter Properties\n");
printf("CLIP (Presentation: \n");
printf("        r = Restricted\n");
printf("        p = Allowed \n");
printf("        s = Allowed - National number, National Std. plan \n");
printf("        t = Allowed - National number, Private Plan \n");
printf("        n = not available\n");
printf("        c = CNIP from contact list, using CLIP\n");
printf("   CLIP:");
tcx_gets(ch_cli, sizeof(ch_cli));

printf("CNIP (0 = not available):");
tcx_gets(ch_cni, sizeof(ch_cni));

printf("LineID:");
tcx_gets(ch_clineid, sizeof(ch_clineid));

printf("Handsets [{h}{destinations}] (e.g. for Handsets 1 and 3 enter h13 - for HS10 enter A) :");
tcx_gets(ch_cld, sizeof(ch_cld));

printf("Audio [{w/n/8/a/u/A/U}]:");
codec = tcx_getch();

// printf( "Melody [{m}{1...8}] (e.g. for alerting pattern 3 (0x43) enter m3) :" );
printf("\nMelody [{1...8}] (e.g. for alerting pattern 3 (0x43) enter 3) :");
tcx_gets(ch_melody, sizeof(ch_melody));
// ch_melody = tcx_getch();

switch (codec)
{
    case 'w':
        sprintf(s_codecs, "%d,%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_WB), (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));
        break;
    case 'a':
        sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCMA));
        break;
    case 'u':
        sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCMU));
        break;
    case '8':
        sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM8));
        break;
    case 'A':
        sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCMA_WB));
        break;
    case 'U':
        sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCMU_WB));
        break;

    case 'n':
    default:
        sprintf(s_codecs, "%d", (CMBS_AUDIO_CODEC_PCM_LINEAR_NB));
}

st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
st_Properties[0].psz_Value = ch_cli;
st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
st_Properties[1].psz_Value = ch_cld;
st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
st_Properties[2].psz_Value = s_codecs;
st_Properties[3].e_IE      = CMBS_IE_LINE_ID;
st_Properties[3].psz_Value = ch_clineid;

if (strlen(ch_cni) && (ch_cni[0] != '0'))
{
    st_Properties[4].e_IE      = CMBS_IE_CALLERNAME ;
    st_Properties[4].psz_Value = ch_cni;
    n_Prop = 5;
} else
{
    n_Prop = 4;
}
st_Properties[n_Prop].e_IE      = CMBS_IE_MELODY;
st_Properties[n_Prop].psz_Value = ch_melody;
n_Prop++;

appcall_EstablishCall(st_Properties, n_Prop);
}

//  ========== keyb_CallRelease ===========
/*!
  \brief    Release a call from Host to Target side
  \param[in,out]  <none>
  \return    <none>

   CMBS_IE_CALLRELEASE_REASON
*/
void     keyb_CallRelease(void)
{
u16                   u16_CallId;
char ch_Reason[2];
ST_APPCALL_PROPERTIES st_Properties;

u16_CallId = _keyb_CallIdInput();
if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{
    printf("\nReason \n");
    printf("0 => Normal\t 1 => Abnormal\n");
    printf("2 => Busy\t 3 => unknown Number\n");

    ch_Reason[0] = tcx_getch();
    ch_Reason[1] = 0;

    st_Properties.e_IE      = CMBS_IE_CALLRELEASE_REASON;
    st_Properties.psz_Value = ch_Reason;

    appcall_ReleaseCall(&st_Properties, 1, u16_CallId, NULL);

    keyb_ReleaseNotify(u16_CallId);
}
}

//  ========== keyb_CallInbandInfo ===========
/*!
  \brief    Host changes Caller ID of an active call (for transfer)
  \param[in,out]  <none>
  \return    <none>

   CMBS_IE_CALLPROGRESS
*/
void keyb_CallInbandInfo()
{
u16 u16_CallId;
char sCLIP[32];

u16_CallId = _keyb_CallIdInput();
if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{
    /* for CLIP update demo, we will use constant presentation indicator (will not prompt user) */
    sCLIP[0] = CMBS_ADDR_PROPTYPE_NATIONAL;
    sCLIP[1] = CMBS_ADDR_PRESENT_ALLOW;

    printf("\nNew Caller ID: ");
    tcx_gets(sCLIP + 2, sizeof(sCLIP) - 2);

    appcall_CallInbandInfo(u16_CallId, sCLIP);
}
}

//  ========== keyb_CallInbandCNIP ===========
/*!
        \brief    Host sends CNIP during active call (matching dial number in Contact list)
        \param[in,out]  <none>
        \return    <none>

      CMBS_IE_CALLPROGRESS
*/
void keyb_CallInbandCNIP(void)
{
u16 u16_CallId;
char sCalledName[CMBS_CALLED_NAME_MAX_LEN + 1];
char sCalledFirstName[CMBS_CALLED_NAME_MAX_LEN];
char ch_cli[40];

ch_cli[0] = 0;

u16_CallId = _keyb_CallIdInput();

printf("\nType (c= Contact List, n = Network provided): ");
switch (tcx_getch())
{
    case 'c':
        // Scrrening indicator 'User provided'
        sCalledName[0] = CMBS_ADDR_SCREENING_UTF8_USER;
        printf("Contact List");
        break;

    case 'n':
        // Scrrening indicator 'Network provided'
        // NOTE: In that case CLIP is needed additionally, thus this use case is not fully covered here
        sCalledName[0] = CMBS_ADDR_SCREENING_UTF8_NWK;

        printf("Network provided");
        printf("\nCalled Party Number: ");
        tcx_gets(ch_cli, sizeof(ch_cli));
        break;
}

printf("\nCalled Party Sir Name  : ");
tcx_gets(sCalledName + 1, sizeof(sCalledName) - 1);

printf("Called Party First Name: ");
tcx_gets(sCalledFirstName, sizeof(sCalledFirstName));

/* combine the full message with Screening indicator, name and first name */
if (sCalledName[0] == CMBS_ADDR_SCREENING_UTF8_NWK)
{
    appcall_CallInbandInfoCNIP(u16_CallId, sCalledName, sCalledFirstName, ch_cli);
}
else
{
    appcall_CallInbandInfoCNIP(u16_CallId, sCalledName, sCalledFirstName, NULL);
}
}


//  ========== keyb_CallProgress ===========
/*!
  \brief    call progress information from Host to Target side
  \param[in,out]  <none>
  \return    <none>

   CMBS_IE_CALLPROGRESS
*/
void     keyb_CallProgress(char *psz_Value)
{
u16      u16_CallId;
ST_APPCALL_PROPERTIES   st_Properties;

u16_CallId = _keyb_CallIdInput();
if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{
    st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;
    st_Properties.psz_Value = psz_Value;
    appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);
}
}
//  ========== keyb_CallAnswer ===========
/*!
  \brief    call answer from Host to Target side
  \param[in,out]  <none>
  \return    <none>

*/
void     keyb_CallAnswer(void)
{
u16      u16_CallId;
u8       u8_WBCodec = 0;
int i;
ST_APPCALL_PROPERTIES   st_Properties;
PST_CALL_OBJ         pst_Call;

// get call ID
u16_CallId = _keyb_CallIdInput();

// get call object
pst_Call = _appcall_CallObjGetById(u16_CallId);
if (!pst_Call)
{
    printf("NO SUCH CALL WITH ID %d\n", u16_CallId);
    return;
}
// check WB support
for (i = 0; i < pst_Call->u8_CodecsLength; i++)
{
    if (pst_Call->pu8_CodecsList[i] == CMBS_AUDIO_CODEC_PCM_LINEAR_WB)
    {
        u8_WBCodec = 1;
        break;
    }
}

// answer call
memset(&st_Properties, 0, sizeof(st_Properties));
appcall_AnswerCall(&st_Properties, 0, u16_CallId, NULL);

// check do we need codec switch
if (pst_Call->e_Codec == CMBS_AUDIO_CODEC_PCM_LINEAR_WB && u8_WBCodec == 0)
{
    appmedia_CallObjMediaStop(0, u16_CallId, NULL);

    // wait for CMBS_EV_DEM_CHANNEL_STOP_RES
    SleepMs(200);          // wait 0.2 seconds
    appmedia_CallObjMediaOffer(u16_CallId, 'n');

    // wait for CMBS_EV_DEM_CHANNEL_OFFER_RES
    SleepMs(200);          // wait 0.2 seconds

    appmedia_CallObjMediaStart(0, u16_CallId, 0xFF, NULL);
} else if (pst_Call->e_Codec == CMBS_AUDIO_CODEC_PCM_LINEAR_NB && u8_WBCodec == 1)
{
    appmedia_CallObjMediaStop(0, u16_CallId, NULL);

    // wait for CMBS_EV_DEM_CHANNEL_STOP_RES
    SleepMs(200);          // wait 0.2 seconds
    appmedia_CallObjMediaOffer(u16_CallId, 'w');

    // wait for CMBS_EV_DEM_CHANNEL_OFFER_RES
    SleepMs(200);          // wait 0.2 seconds

    appmedia_CallObjMediaStart(0, u16_CallId, 0xFF, NULL);
}
}

//      ========== keyb_HoldResumeCfm ===========
/*!
  \brief              set automatic accept for hold/resume
  \param[in,out]      <none>
  \return             <none>

*/
void    keyb_HoldResumeCfm(void)
{
u8 u8_Answ;
printf("\nEnter automatic confirm for hold [Y/N]: ");
u8_Answ = tcx_getch();
if (u8_Answ == 'y' || u8_Answ == 'Y')
{
    g_HoldCfm = 1;
} else
{
    g_HoldCfm = 0;
}

printf("\nEnter automatic confirm for resume hold [Y/N]: ");
u8_Answ = tcx_getch();
if (u8_Answ == 'y' || u8_Answ == 'Y')
{
    g_HoldResumeCfm = 1;
} else
{
    g_HoldResumeCfm = 0;
}
}

//      ========== keyb_EarlyMediaOnOffCfm ===========
/*!
  \brief              enables/disables auto early media
  \param[in,out]      <none>
  \return             <none>

*/
void    keyb_EarlyMediaOnOffCfm(void)
{
u8 u8_Answ;
printf("\nEnter automatic early media [Y/N]: ");
u8_Answ = tcx_getch();
if (u8_Answ == 'y' || u8_Answ == 'Y')
{
    g_EarlyMediaAutoCfm = 1;
} else
{
    g_EarlyMediaAutoCfm = 0;
}
}

//      ========== keyb_TransferAutoCfm ===========
/*!
  \brief              set automatic accept for call transfer
  \param[in,out]      <none>
  \return             <none>

*/
void    keyb_TransferAutoCfm(void)
{
u8 u8_Answ;
printf("\nEnter automatic confirm for transfer request [Y/N]: ");
u8_Answ = tcx_getch();
if (u8_Answ == 'y' || u8_Answ == 'Y')
{
    g_TransferAutoCfm = 1;
} else
{
    g_TransferAutoCfm = 0;
}
}

//      ========== keyb_ConferenceAutoCfm ===========
/*!
  \brief              set automatic accept for conference creation
  \param[in,out]      <none>
  \return             <none>

*/
void    keyb_ConferenceAutoCfm(void)
{
u8 u8_Answ;
printf("\nEnter automatic confirm for conference request [Y/N]: ");
u8_Answ = tcx_getch();
if (u8_Answ == 'y' || u8_Answ == 'Y')
{
    g_ConfAutoCfm = 1;
} else
{
    g_ConfAutoCfm = 0;
}
}


//  ========== keyb_MediaSwitch ===========
/*!
  \brief    Media channel start or stop
  \param[in,out]  bo_On    TRUE to start, FALSE to stop
  \return    <none>

*/
void     keyb_MediaSwitch(int bo_On)
{
u16  u16_CallId;
u16  u16_Slot;

u16_CallId = _keyb_CallIdInput();
if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{

    if (bo_On)
    {
        u16_Slot = _keyb_SlotInput();
        appmedia_CallObjMediaStart(0, u16_CallId, u16_Slot, NULL);
    } else
    {
        appmedia_CallObjMediaStop(0, u16_CallId, NULL);
    }

}
}

void keyb_MediaInernalCall(void)
{
char  ch_Channel;
char  ch_Context;
char  ch_Operation;

printf("\nChannel ID? [0..3]:");
ch_Channel = tcx_getch();

if ((ch_Channel >= '0') && (ch_Channel <= '3'))
{
    ch_Channel -= '0';
} else
{
    printf("\nUnavailble Channel ID, Must be 0 to 3, Press any key to continue");
    tcx_getch();
    return;
}

printf("\nContext/Node ID? [0..1]:");
ch_Context = tcx_getch();

if ((ch_Context >= '0') && (ch_Context <= '1'))
{
    ch_Context -= '0';
} else
{
    printf("\nUnavailble Context/Node ID, Must be 0 or 1, Press any key to continue");
    tcx_getch();
    return;
}


printf("\nConnect or Disconnect? [1, Connect; 0, Disconnect]:");
ch_Operation = tcx_getch();

if ((ch_Operation >= '0') && (ch_Operation <= '1'))
{
    ch_Operation -= '0';
} else
{
    printf("\nError Command, Must be 0 or 1, Press any key to continue");
    tcx_getch();
    return;
}

appmedia_CallObjMediaInternalConnect(ch_Channel, ch_Context, ch_Operation);

}

void     keyb_MediaOffer(void)
{
u16   u16_CallId;
char  ch_Audio;

u16_CallId = _keyb_CallIdInput();

if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{

    printf("\nSelect codec:\n");
    printf("\tw - WB Linear PCM16  (16 KHz, 16Bit / Sample)\n");
    printf("\tn - NB Linear PCM16  (8 KHz,  16Bit / Sample)\n");
    printf("\ta - NB G.711 A-law   (8 KHz,  8Bit  / Sample)\n");
    printf("\tu - NB G.711 u-law   (8 KHz,  8Bit  / Sample)\n");
    printf("\t8 - NB Linear PCM8   (8 KHz,  8Bit  / Sample)\n");
    printf("\tg - NB Linear PCM16 with G.711 A-law OTA\n");
    ch_Audio = tcx_getch();

    appmedia_CallObjMediaOffer(u16_CallId, ch_Audio);
}
}

//  ========== keyb_MediaAutoSwitch ===========
/*!
        \brief    Switch codec betwen G722 (WB) and G726 (NB) every 4 seconds
        \                   for testing slot switch failures
        \param[in,out]  <none>
        \return    <none>

*/
void     keyb_MediaAutoSwitch(void)
{
u16   u16_CallId;

u16_CallId = _keyb_CallIdInput();

printf("\nSwitch codec betwen G722 (WB) and G726 (NB) every 4 seconds until key pressed \n");

while (!kbhit())
{
    printf("\nSwitch codec to G726 \n");
    appmedia_CallObjMediaOffer(u16_CallId, 'n');
    SleepMs(4000);        // wait 4 seconds

    if (!kbhit())
    {
        printf("\nSwitch codec to G722 \n");
        appmedia_CallObjMediaOffer(u16_CallId, 'w');
        SleepMs(4000);        // wait 4 seconds
    }
}
}

//  ========== keyb_TonePlay ===========
/*!
  \brief    Play/Stop a tone on Media channel
  \param[in,out]  psz_Value  enumeration string of CMBS tone
  \param[in,out]  bo_On    TRUE to start, FALSE to stop
  \return    <none>

*/
void     keyb_TonePlay(char *psz_Value, int no_On)
{
u16      u16_CallId;

u16_CallId = _keyb_CallIdInput();
if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
}
else
{
    appmedia_CallObjTonePlay(psz_Value, no_On, u16_CallId, NULL);
}
}

//
//  ========== keyb_TonePlaySelect ===========
/*!
  \brief    Play/Stop a tone on Media channel
  \param[in,out]  psz_Value  enumeration string of CMBS tone
  \param[in,out]  bo_On    TRUE to start, FALSE to stop
  \return    <none>

*/
void     keyb_TonePlaySelect(void)
{
u8    u8_Value;

printf("\n Tones provided: \n");
printf("0  : Tone OFF \n");
printf("1  : Dial tone\n");
printf("2  : Ring-back tone\n");
printf("3  : Busy tone\n");
printf("4  : Call waiting tone\n");
printf("5  : Message waiting dial tone\n");
printf("6  : Call Forward dial tone (Swiss only)\n");
printf("6  : Confirm tone           (italy only)\n");
printf("7  : Congestion tone        (Swiss only)\n");
printf("\n  Country selection: \n");
printf("l  : default (0)\n");
printf("m  : french  (1)\n");
printf("n  : polish  (2)\n");
printf("o  : swiss   (3)\n");
printf("p  : italian (4)\n");
printf("v  : toggle 'use TOG to audio node' (not IOM) actual 0x%2x \n", g_u8_ToneType);
printf("\n OLD STYLE implementation for french and polish tones:\n");
printf(" FRENCH tones: \n");
printf("8  : Dial tone                 - French style \n");
printf("9  : Message waiting dial tone - French style \n");
printf("a  : Ring-back tone            - French style \n");
printf("b  : Busy tone                 - French style \n");
printf("c  : Call waiting tone         - French style \n");
printf(" POLISH tones: \n");
printf("d  : Dial tone                 - Polish style \n");
printf("e  : Message waiting dial tone - Polish style \n");
printf("f  : Ring-back tone            - Polish style \n");
printf("g  : Busy tone                 - Polish style \n");
printf("h  : Call waiting tone         - Polish style \n");
printf(" OUTBAND tones (<<SIGNAL>> IE to PP): \n");
printf("r  : Dial tone                 - outband \n");
printf("s  : Ring-back tone            - outband \n");
printf("t  : Busy tone                 - outband \n");
printf("u  : Call waiting tone         - outband \n");
printf("x  : tone off                  - outband \n");
printf("\n  select Tone Style:  \n");

switch (tcx_getch())
{
    case '0':
        keyb_TonePlay(NULL, FALSE | g_u8_ToneType);
        break;
    case '1':
        keyb_TonePlay("CMBS_TONE_DIAL\0", TRUE | g_u8_ToneType);
        break;
    case '2':
        keyb_TonePlay("CMBS_TONE_RING_BACK\0", TRUE | g_u8_ToneType);
        break;
    case '3':
        keyb_TonePlay("CMBS_TONE_BUSY\0", TRUE | g_u8_ToneType);
        break;
    case '4':
        keyb_TonePlay("CMBS_TONE_CALL_WAITING\0", TRUE | g_u8_ToneType);
        break;
    case '5':
        keyb_TonePlay("CMBS_TONE_MWI\0", TRUE | g_u8_ToneType);
        break;
    case '6':
        keyb_TonePlay("CMBS_TONE_DIAL_CALL_FORWARD\0", TRUE | g_u8_ToneType);
        break;
    case '7':
        keyb_TonePlay("CMBS_TONE_MWI_OR_CONGESTION\0", TRUE | g_u8_ToneType);
        break;

    case 'l':
        u8_Value = CMBS_INBAND_DEFAULT;
        app_SrvParamSet(CMBS_PARAM_INBAND_COUNTRY, &u8_Value, sizeof(u8_Value), 1);
        break;
    case 'm':
        u8_Value = CMBS_INBAND_FRENCH;
        app_SrvParamSet(CMBS_PARAM_INBAND_COUNTRY, &u8_Value, sizeof(u8_Value), 1);
        break;
    case 'n':
        u8_Value = CMBS_INBAND_POLISH;
        app_SrvParamSet(CMBS_PARAM_INBAND_COUNTRY, &u8_Value, sizeof(u8_Value), 1);
        break;
    case 'o':
        u8_Value = CMBS_INBAND_SWISS;
        app_SrvParamSet(CMBS_PARAM_INBAND_COUNTRY, &u8_Value, sizeof(u8_Value), 1);
        break;

    case 'p':
        u8_Value = CMBS_INBAND_ITALY;
        app_SrvParamSet(CMBS_PARAM_INBAND_COUNTRY, &u8_Value, sizeof(u8_Value), 1);
        break;

    case 'v':
        g_u8_ToneType ^= 0x80;
        printf(" Tonetype = 0x%2x\n", g_u8_ToneType);
        break;

    case '8':
        keyb_TonePlay("CMBS_TONE_DIAL_FT_FR\0", TRUE);
        break;
    case '9':
        keyb_TonePlay("CMBS_TONE_MWI_FT_FR\0", TRUE);
        break;
    case 'a':
        keyb_TonePlay("CMBS_TONE_RING_BACK_FT_FR\0", TRUE);
        break;
    case 'b':
        keyb_TonePlay("CMBS_TONE_BUSY_FT_FR\0", TRUE);
        break;
    case 'c':
        keyb_TonePlay("CMBS_TONE_CALL_WAITING_FT_FR\0", TRUE);
        break;
    case 'd':
        keyb_TonePlay("CMBS_TONE_DIAL_FT_PL\0", TRUE);
        break;
    case 'e':
        keyb_TonePlay("CMBS_TONE_MWI_FT_PL\0", TRUE);
        break;
    case 'f':
        keyb_TonePlay("CMBS_TONE_RING_BACK_FT_PL\0", TRUE);
        break;
    case 'g':
        keyb_TonePlay("CMBS_TONE_BUSY_FT_PL\0", TRUE);
        break;
    case 'h':
        keyb_TonePlay("CMBS_TONE_CALL_WAITING_FT_PL\0", TRUE);
        break;

    case 'r':
        keyb_TonePlay("CMBS_TONE_DIAL_OUTBAND\0", TRUE);
        break;
    case 's':
        keyb_TonePlay("CMBS_TONE_RING_BACK_OUTBAND\0", TRUE);
        break;
    case 't':
        keyb_TonePlay("CMBS_TONE_BUSY_OUTBAND\0", TRUE);
        break;
    case 'u':
        keyb_TonePlay("CMBS_TONE_CALL_WAITING_OUTBAND\0", TRUE);
        break;
    case 'x':
        keyb_TonePlay("CMBS_TONE_OFF_OUTBAND\0", TRUE);
        break;
}
}


//   ========== keyb_CallWaiting ===========
/*!
  \brief    start/stop Call waiting from host to target.
  \param[in,out]  <none>
  \return    <none>

   \note             Host is in responsibility of stop call waiting and
      provide the old CLI to CMBS target.
*/
void     keyb_CallWaiting(void)
{
ST_APPCALL_PROPERTIES st_Properties;

u16      u16_CallId;
int      n_On = FALSE;
char     ch_cli[30];

u16_CallId = _keyb_CallIdInput();
if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{

    printf("Call Waiting On {y/n/f = Yes/No/French tone}:\n");
    n_On = tcx_getch();

    printf("Caller Party :");
    tcx_gets(ch_cli, sizeof(ch_cli));
    if (n_On == 'y')
    {
        appmedia_CallObjTonePlay("CMBS_TONE_CALL_WAITING\0", TRUE, u16_CallId, NULL);
    } else if (n_On == 'f')
    {
        appmedia_CallObjTonePlay("CMBS_TONE_CALL_WAITING_FT_FR\0", TRUE, u16_CallId, NULL);
    } else
    {
        appmedia_CallObjTonePlay(NULL, FALSE, u16_CallId, NULL);
    }
    st_Properties.e_IE = CMBS_IE_CALLINFO;
    st_Properties.psz_Value = ch_cli;

    appcall_DisplayCall(&st_Properties, 1, u16_CallId, NULL);

}
}
//  ========== keyb_CallHold  ===========
/*!
  \brief   signal to CMBS Target, that the call is hold or resumed.
      the CMBS Target enable/disable the audio stream.
  \param[in]    <none>
  \return   <none>
*/

void  keyb_CallHold(void)
{
u16 u16_CallId;
int n_On;

u16_CallId = _keyb_CallIdInput();

if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{
    printf("\nCall Hold? [y/n] : ");
    n_On = tcx_getch();
    if (n_On == 'y')
    {
        appcall_HoldCall(u16_CallId, NULL);
    } else
    {
        appcall_ResumeCall(u16_CallId, NULL);
    }
}
}


//  ========== keyb_SendCallStatusUpdateToPP  ===========
/*!
  \brief         Send an updated call status to the PP
  \param[in]     <none>
  \return      <none>
*/
void keyb_SendCallStatusUpdateToPP(void)
{
u16                     u16_CallId;
u32                     u32_Temp;
ST_APPCALL_PROPERTIES   st_Properties;

st_Properties.e_IE      = CMBS_IE_CALLPROGRESS;

u16_CallId = _keyb_CallIdInput();

if (u16_CallId == KEYB_CALL_INVALID_CALLID)
{
    printf(" \n Invalid Caller ID - Retry with the correct ID \n");
} else
{

    printf("\n Call Status:\n");
    printf("================\n");
    printf("DISCONNECTING:\t0\n");
    printf("IDLE:\t1\n");
    printf("HOLD:\t2\n");
    printf("CALL-CONNECTED:\t3\n");

    tcx_scanf("%u", &u32_Temp);

    switch (u32_Temp)
    {
        case 0:
            st_Properties.psz_Value = "CMBS_CALL_PROGR_DISCONNECTING";
            break;

        case 1:
            st_Properties.psz_Value = "CMBS_CALL_PROGR_IDLE";
            break;

        case 2:
            st_Properties.psz_Value = "CMBS_CALL_PROGR_HOLD";
            break;

        case 3:
            st_Properties.psz_Value = "CMBS_CALL_PROGR_CONNECT";
            break;

        default:
            printf("\nERROR %d, ignoring...\n", u32_Temp);
            return;
    }

    appcall_ProgressCall(&st_Properties, 1, u16_CallId, NULL);
}
}


//  ========== keyb_CallStressTests  ===========
/*!
        \brief         Call control stress tests
        \param[in]     <none>
        \return      <none>
*/
void keyb_CallStressTests(void)
{
tcx_appClearScreen();
printf("Call control stress tests\n");
printf("==========================\n");
printf("m\tMissed call stress test\n");
printf("q\tback to previous menu\n");

while (1)
{
    switch (tcx_getch())
    {
        case 'm':
        {
            // loop parameters
            u32 u32_NumOfCalls, u32_DelayBetweenCallsMs, u32_Counter;

            // call ID
            u16 u16_CallId;

            // call parameters
            ST_APPCALL_PROPERTIES st_Properties[6], st_RelProperties;
            int  n_Prop = 6;
            char ch_cli[] = "1234567890000";
            char ch_cni[] = "MissedCallsTests";
            char ch_cld[] = ALL_HS_STRING;
            char ch_clineid[] = "0";
            char s_codecs[] = "5,6"; //WB and NB
            char ch_melody[] = "3";

            st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
            st_Properties[0].psz_Value = ch_cli;
            st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
            st_Properties[1].psz_Value = ch_cld;
            st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
            st_Properties[2].psz_Value = s_codecs;
            st_Properties[3].e_IE      = CMBS_IE_LINE_ID;
            st_Properties[3].psz_Value = ch_clineid;
            st_Properties[4].e_IE      = CMBS_IE_CALLERNAME;
            st_Properties[4].psz_Value = ch_cni;
            st_Properties[5].e_IE      = CMBS_IE_MELODY;
            st_Properties[5].psz_Value = ch_melody;

            // release parameters
            st_RelProperties.e_IE = CMBS_IE_CALLRELEASE_REASON;
            st_RelProperties.psz_Value = "0";

            printf("\n\nMissed calls:\nEnter number of calls... ");
            tcx_scanf("%d", &u32_NumOfCalls);
            printf("\nEnter delay between calls (seconds)... ");
            tcx_scanf("%d", &u32_DelayBetweenCallsMs);

            // stress test loop
            for (u32_Counter = 0; u32_Counter < u32_NumOfCalls; ++u32_Counter)
            {
                tcx_appClearScreen();

                printf("\n\nPerforming call %d out of %d...\n\n", u32_Counter + 1, u32_NumOfCalls);

                // perform IC call
                u16_CallId = appcall_EstablishCall(st_Properties, n_Prop);

                // wait
                SleepMs(u32_DelayBetweenCallsMs * 1000);

                // release the call
                appcall_ReleaseCall(&st_RelProperties, 1, u16_CallId, NULL);
            }
        }
        break;

        case 'q':
        default:
            return;
    }
}
}

//  ========== keyb_CallLoop ===========
/*!
  \brief    call management loop to control the call activities
  \param[in,out]  <none>
  \return    <none>

*/

void     keyb_CallLoop(void)
{
int n_Keep = TRUE;

while (n_Keep)
{
    //      tcx_appClearScreen();

    printf("\n======Call Objects===============================\n\n");
    appcall_InfoPrint();
    printf("\n======Call Control===============================\n\n");
    printf("l => automat on             L => automat off\n");
    printf("e => establish call         r => release call\n");
    printf("c => change codec           h => hold/resume\n");
    printf("w => call waiting           k => ringing\n");
    printf("a => answer call            f => early connect\n");
    printf("g => setup-ack              t => automatic confirm for hold/resume...\n");
    printf("G => call-proceeding        C => Change caller ID of an active call\n");
    printf("u => transfer auto cfm...   o => conference auto cfm...\n");
    printf("x => automatic codec switch G726/G722 every 10 seconds \n");
    printf("b => Send Call Status update to PP...\n");
    printf("v => Send calling party name (CNIP) during a call.\n");
    printf("s => Stress tests ...\n");
    printf("\n======Media===============================\n\n");
    printf("1 => Dial Tone      2 => Ring Tone\n");
    printf("3 => Busy Tone      0 => Off Tone \n");
    printf("6 => Other tones by further selection \n");
    printf("m => Media on       n => Media off\n");
    printf("p => Media Loopback\n");
    printf("d => early media on/off\n");
    printf("------------------------------------------\n");
    printf("Set host preferred CODEC \n");
    printf("   7 => use received codec \n");
    printf("   8 => WB \n");
    printf("   9 => NB \n");
    printf("------------------------------------------\n\n");
    printf("i => call/line infos \n");
    printf("q => return to interface \n");

    switch (tcx_getch())
    {
        case ' ':
            tcx_appClearScreen();
            break;

        case 'l':
            appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);
            break;
        case 'L':
            appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_OFF);
            break;

        case 'e':
            // establish
            keyb_CallEstablish();
            break;
        case 'r':
            // release call
            keyb_CallRelease();
            break;
        case 'c':
            // change codec
            keyb_MediaOffer();
            break;
        case 'x':
            // change codec between G726 and G722 every 10 seconds
            keyb_MediaAutoSwitch();
            break;
        case 'h':
            // Call Hold/Resume
            keyb_CallHold();
            break;
        case 'w':
            // call waiting
            keyb_CallWaiting();
            break;
        case 'k':
            // ringback-tone
            keyb_CallProgress((char*)"CMBS_CALL_PROGR_RINGING\0");
            break;
        case 'a':
            // answer call
            keyb_CallAnswer();
            break;
        case 'f':
            // early media
            keyb_CallProgress((char*)"CMBS_CALL_PROGR_INBAND\0");
            break;
        case 'g':
            keyb_CallProgress((char*)"CMBS_CALL_PROGR_SETUP_ACK\0");
            break;

        case 'G':
            keyb_CallProgress((char*)"CMBS_CALL_PROGR_PROCEEDING\0");
            break;

        case '1':
            keyb_TonePlay("CMBS_TONE_DIAL\0", TRUE);
            break;
        case '2':
            keyb_TonePlay("CMBS_TONE_RING_BACK\0", TRUE);
            break;
        case '3':
            keyb_TonePlay("CMBS_TONE_BUSY\0", TRUE);
            break;
        case '6':
            keyb_TonePlaySelect();
            break;
        case 'm':
            keyb_MediaSwitch(TRUE);
            break;
        case 'n':
            keyb_MediaSwitch(FALSE);
            break;
        case 'p':
            keyb_MediaInernalCall();
            break;
        case '0':
            keyb_TonePlay(NULL, FALSE | g_u8_ToneType);
            break;
        case 'i':
            // call/line info
            tcx_appClearScreen();
            appcall_InfoPrint();
            printf("Press Any Key\n ");
            tcx_getch();
            break;

        case 't':
            keyb_HoldResumeCfm();
            break;

        case 'd':
            keyb_EarlyMediaOnOffCfm();
            break;

        case '7':
            keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC_RECEIVED_CODEC);
            break;

        case '8':
            keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC_WB);
            break;

        case '9':
            keyb_SetHostCodec(E_APPCALL_PREFERRED_CODEC_NB);
            break;

        case 'u':
            keyb_TransferAutoCfm();
            break;

        case 'v':
            keyb_CallInbandCNIP();
            break;

        case 'o':
            keyb_ConferenceAutoCfm();
            break;

        case 'q':
            n_Keep = FALSE;
            break;

        case 'C':
            keyb_CallInbandInfo();
            break;

        case 'b':
            keyb_SendCallStatusUpdateToPP();
            break;

        case 's':
            keyb_CallStressTests();
            break;

        default:
            break;
    }
}
}

extern u32 g_u32_UsedSlots;

void keyb_AudioTestCall(E_CMBS_AUDIO_CODEC codec)
{
int n_Keep = TRUE;

while (n_Keep)
{
    char value = 0;
    int i = 0, startSlot = 0, slotMask = 0xF, item = 0;
    int slotSize = codec == CMBS_AUDIO_CODEC_PCM_LINEAR_WB ? 4 : 2;
    int slotShift = 2;
    int audioSlaveMenuIndex = 1;
    char audioSlaveMenuIndexName[] = "W16";

    switch (codec)
    {
        case CMBS_AUDIO_CODEC_PCM_LINEAR_WB:
            slotSize = 4;
            slotShift = 2;
            slotMask = 0xF;
            audioSlaveMenuIndex = 1;
            strcpy(audioSlaveMenuIndexName, "W16");
            break;
        case CMBS_AUDIO_CODEC_PCM_LINEAR_NB:
            slotSize = 2;
            slotShift = 2;
            slotMask = 0x3;
            audioSlaveMenuIndex = 2;
            strcpy(audioSlaveMenuIndexName, "N16");
            break;
        case CMBS_AUDIO_CODEC_PCMA:
            slotSize = 1;
            slotShift = 1;
            slotMask = 0x1;
            audioSlaveMenuIndex = 3;
            strcpy(audioSlaveMenuIndexName, "N8a");
            break;
        case CMBS_AUDIO_CODEC_PCMU:
            slotSize = 1;
            slotShift = 1;
            slotMask = 0x1;
            audioSlaveMenuIndex = 4;
            strcpy(audioSlaveMenuIndexName, "N8u");
            break;
        case CMBS_AUDIO_CODEC_PCMA_WB:
            slotSize = 2;
            slotShift = 1;
            slotMask = 0x3;
            audioSlaveMenuIndex = 5;
            strcpy(audioSlaveMenuIndexName, "W8a");
            break;
        case CMBS_AUDIO_CODEC_PCMU_WB:
            slotSize = 2;
            slotShift = 1;
            slotMask = 0x3;
            audioSlaveMenuIndex = 6;
            strcpy(audioSlaveMenuIndexName, "W8u");
            break;

        default:
            slotSize = 1;
            slotShift = 1;
            slotMask = 0x1;
    }

    //      tcx_appClearScreen();
    printf("\nHint: Caller number contains digits sequence for TDM Master!");
    printf("\n----------------------------  \n");

    // print slot selection menu
    while (startSlot + slotSize <= 12 && item < 9)
    {
        printf("%d => Use slots 0x%.4X (slots %d-%d) \n" , ++item, slotMask, startSlot, startSlot + slotSize - 1);
        slotMask = slotMask << slotShift;
        startSlot += slotShift;
    }

    printf("- - - - - - - - - - - - - - -  \n");
    keyb_CallInfo();
    printf("r => Release test call  \n");
    printf("q => Return to Interface Menu  \n");

    value = tcx_getch();
    switch (value)
    {
        case 'r':
            keyb_IncCallRelease();
            break;
        case 'q':
            n_Keep = FALSE;
            break;
        default:
            if (value >= '1' && value <= '9')
            {
                int index = value - '1';
                startSlot = index * slotShift;

                printf("\n------------------------------  \n");
                printf("-- TDM MASTER INSTRUCTIONS --  \n");
                printf("------------------------------  \n");
                printf("- PRESS '0' : to mute all channels \n");
                printf("- PRESS '8' : to go into settings \n");
                printf("- PRESS '%d' : to choose %s \n", audioSlaveMenuIndex, audioSlaveMenuIndexName);
                printf("- PRESS '%s%d' : to choose %d start slot \n", startSlot < 10 ? "0" : "", startSlot, startSlot);
                printf("------------------------------  \n");

                // mark other slots as busy
                g_u32_UsedSlots = 0;
                for (i = 0; i < startSlot; i++)
                    g_u32_UsedSlots |= 0x1 << i;

                {
                    ST_APPCALL_PROPERTIES st_Properties[6];
                    char s_codecs[CMBS_AUDIO_CODEC_MAX * 2] = {0};
                    char s_callername[CMBS_CALLER_NAME_MAX_LEN] = {0};
                    char s_callerparty[CMBS_CALLER_NAME_MAX_LEN] = {0};

                    sprintf(s_codecs, "%d", (codec));
                    sprintf(s_callername, "%s, Slots %d-%d\0", audioSlaveMenuIndexName, startSlot, startSlot + slotSize - 1);
                    sprintf(s_callerparty, "08%d%s%d\0", audioSlaveMenuIndex, startSlot < 10 ? "0" : "", startSlot);

                    if (g_u16_DemoCallId == APPCALL_NO_CALL)
                    {
                        appcall_AutomatMode(E_APPCALL_AUTOMAT_MODE_ON);

                        st_Properties[0].e_IE      = CMBS_IE_CALLERPARTY;
                        st_Properties[0].psz_Value = s_callerparty;
                        st_Properties[1].e_IE      = CMBS_IE_CALLEDPARTY;
                        st_Properties[1].psz_Value = ALL_HS_STRING;
                        st_Properties[2].e_IE      = CMBS_IE_MEDIADESCRIPTOR;
                        st_Properties[2].psz_Value = s_codecs;
                        st_Properties[3].e_IE      = CMBS_IE_CALLERNAME;
                        st_Properties[3].psz_Value = s_callername;
                        st_Properties[4].e_IE      = CMBS_IE_LINE_ID;
                        st_Properties[4].psz_Value = "1\0";
                        st_Properties[5].e_IE      = CMBS_IE_MELODY;
                        st_Properties[5].psz_Value = "1\0";

                        g_u16_DemoCallId = appcall_EstablishCall(st_Properties, 5);
                        if (g_u16_DemoCallId == APPCALL_NO_CALL)
                        {
                            printf("Call can not be set-up!\n");
                        }
                    } else
                    {
                        printf("Please, release existing test call first\n");
                    }
                }
                n_Keep = FALSE;
            }
    }
}
}

void keyb_AudioTest()
{
int n_Keep = TRUE;

while (n_Keep)
{
    tcx_appClearScreen();
    printf("-----------------------------  \n");
    printf("1 => Incoming PCM_LINEAR_WB 16 bit 16kHz call \n");
    printf("2 => Incoming PCM_LINEAR_NB 16 bit  8kHz call \n");
    printf("3 => Incoming PCMA_WB        8 bit 16kHz call \n");
    printf("4 => Incoming PCMU_WB        8 bit 16kHz call \n");
    printf("5 => Incoming PCMA           8 bit  8kHz call \n");
    printf("6 => Incoming PCMU           8 bit  8kHz call \n");
    printf("7 => Incoming PCM8           8 bit  8kHz call \n");
    printf("- - - - - - - - - - - - - - -  \n");
    keyb_CallInfo();
    printf("r => Release test call  \n");
    printf("- - - - - - - - - - - - - - -  \n");
    printf("q => Return to Interface Menu  \n");

    switch (tcx_getch())
    {
        case '1':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCM_LINEAR_WB);
            break;
        case '2':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCM_LINEAR_NB);
            break;
        case '3':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCMA_WB);
            break;
        case '4':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCMU_WB);
            break;
        case '5':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCMA);
            break;
        case '6':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCMU);
            break;
        case '7':
            keyb_AudioTestCall(CMBS_AUDIO_CODEC_PCM8);
            break;
        case 'r':
            keyb_IncCallRelease();
            break;
        case 'q':
            n_Keep = FALSE;
            break;

        default:
            break;
    }
}
}
