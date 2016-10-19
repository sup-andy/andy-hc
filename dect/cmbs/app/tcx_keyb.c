/*!
* \file  tcx_keyb.c
* \brief  test command line generator for manual testing
* \Author  kelbch
*
* @(#) %filespec: tcx_keyb.c~7.1.12.1.7.2.1 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                \n
* ----------------------------------------------------------------------------\n
* 13-Jan-2014 tcmc_asa  -- GIT--  take checksum changes from 3.46.x to 3_x main (3.5x)
* 20-Dec-2013 tcmc_asa                 added CHECKSUM_SUPPORT
*   16-apr-09  kelbch  1.0       Initialize \n
*   18-sep-09  kelbch  pj1029-479  add quick demonstration menu \n
*******************************************************************************/
#include <stdio.h>
#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "tcx_keyb.h"
#include <tcx_util.h>
#ifdef WIN32
#include "cmbs_voipline_skype.h"
extern ST_EXTVOIPLINE voiplineSkype;
#define kbhit _kbhit
#endif

/***************************************************************
*
* external defined function for a quick demonstration
*
****************************************************************/

extern void keyb_CatIqLoop(void);
extern void keyb_DataLoop(void);
extern void keyb_IncCallWB(void);
extern void keyb_IncCallNB(void);
extern void keyb_IncCallNB_G711A_OTA(void);
extern void keyb_IncCallRelease(void);
extern void keyb_DandATests(void);
extern void keyb_CallInfo(void);
extern void keyb_SuotaLoop(void);
extern void keyb_IncCallWB_aLAW_mLAW(void);
extern void keyb_ChipSettingsCalibrate(void);
extern void keyb_RTPTestLoop(void);
extern void keyb_HanLoop(void);
extern void keyb_AudioTest();
extern void keyb_SrvChecksumTest(void);
extern u16   tcx_ApplVersionGet(void);
extern int   tcx_ApplVersionBuildGet(void);
extern void     appcmbs_VersionGet(char * pc_Version);

//  ========== keyboard_loop  ===========
/*!
        \brief     line command keyboard loop of test application
        \param[in,out]   <none>
        \return     <none>
*/
void  keyboard_loop(void)
{
    char ch_Version[80];
    int    n_Run = TRUE;

    while (n_Run)
    {
        appcmbs_VersionGet(ch_Version);
        printf("#############################################################\n");
        printf("#\tVersion:\n");
        printf("#\tApplication\t: Version %02x.%02x - Build %x\n", (tcx_ApplVersionGet() >> 8), (tcx_ApplVersionGet() & 0xFF), tcx_ApplVersionBuildGet());
        printf("#\tTarget     \t: %s\n", ch_Version);
        printf("#\tRegistration Window %s\n", g_cmbsappl.RegistrationWindowStatus ? "OPEN" : "CLOSED");
        printf("#\n");
        printf("#############################################################\n");
        printf("#\tChoose Option\n");
        printf("#\te => Open Registration Window\n");
        printf("#\ts => Service, system\n");
        printf("#\tc => Call management\n");
        printf("#\tf => Firmware update menu\n");
        printf("#\ti => Facility requests\n");
        printf("#\tj => Light Data Service\n");
        printf("#\td => D&A Tester Menu\n");
#ifndef WIN32
        printf("#\tu => Suota test Menu\n");
#endif
        printf("#\tx => Calibration Menu\n");
        printf("#\tt => RTP Test Menu\n");
#ifdef CHECKSUM_SUPPORT
        printf("#\tg => Checksum Error Test Menu\n");
#endif
        printf("#\th => HAN Test Menu\n");
        printf("#\tU => Audio Test Menu\n");
        printf("#\t----------------------------------------------------------\n");
        printf("#\tw => Incoming wideband CLI: 1234, CNAME: Call WB\n");
        printf("#\t     Active Call >>> Codec change to wideband \n");
        printf("#\tn => Incoming narrow band CLI: 6789, CNAME: Call NB\n");
        printf("#\t     Active Call >>> Codec change to narrow band \n");
        printf("#\ta => Incoming wideband  aLaw CLI: 3456, CNAME: Call WB aLaw \n");
        printf("#\t     Active Call >>> Codec change to wideband aLaw \n");
        printf("#\to => Incoming NB Linear PCM with G.711 A-law used OTA\n");
        printf("#\t     Active Call >>> Codec change to NB with G.711 A-law OTA \n");
        printf("#\tr => Release call\n");
        printf("#\t----------------------------------------------------------\n");
#ifdef WIN32
        if (voiplineSkype.fncSetupCall)
        {
            printf("#\tp => Show Skype Phonebook\n");
            printf("#\t----------------------------------------------------------\n");
        }
#endif
        printf("#\tq => Quit\n\n");
        printf("#############################################################\n");
        keyb_CallInfo();
        printf("\nChoose:");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case 's':
                keyb_SRVLoop();
                break;

            case 'c':
                keyb_CallLoop();
                break;

            case 'f':
            case 'F':
                keyb_SwupLoop();
                break;

            case 'i':
                keyb_CatIqLoop();
                break;

            case 'j':
                keyb_DataLoop();
                break;

            case 'w':
                keyb_IncCallWB();
                break;

            case 'n':
                keyb_IncCallNB();
                break;

            case 'a':
                keyb_IncCallWB_aLAW_mLAW();
                break;

            case 'o':
                keyb_IncCallNB_G711A_OTA();
                break;

            case 'r':
                keyb_IncCallRelease();
                break;

            case 'd':
                keyb_DandATests();
                break;

#ifdef WIN32
            case 'p':
                extvoip_DumpContacts();
                break;
#endif
#ifndef WIN32
            case 'u':
                keyb_SuotaLoop();
                break;
#endif
            case 'x':
                keyb_ChipSettingsCalibrate();
                break;

            case 't':
                keyb_RTPTestLoop();
                break;

            case 'e':
                app_SrvSubscriptionOpen(120);
                break;

#ifdef CHECKSUM_SUPPORT
            case 'g':
                keyb_SrvChecksumTest();
                break;
#endif

            case 'h':
            case 'H':
                keyb_HanLoop();
                break;

            case 'U':
                keyb_AudioTest();
                break;

            case 'q':
                n_Run = FALSE;
                break;
        }
    }
}

//*/
