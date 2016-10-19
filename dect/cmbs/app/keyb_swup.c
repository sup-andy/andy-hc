/*!
* \file  keyb_swup.c
* \brief  firmware update test
* \Author  stein
*
* @(#) %filespec: keyb_swup.c~6 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if ! defined ( WIN32 )
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/msg.h>
#else
#include <conio.h>
#include <io.h>
#endif

#include <fcntl.h>
#include <errno.h>

#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appswup.h"
#include "tcx_keyb.h"
#include "cmbs_int.h"
#include <tcx_util.h>
extern ST_CMBS_UPGRADE_SETUP    g_stSwupSetup;

//  ========== keyb_SwupStart ===========
/*!
        \brief         starts firmware update
        \param[in,ou]  <none>
        \return

*/
extern E_CMBS_RC      app_SrvParamGet(E_CMBS_PARAM e_Param, u32 u32_Token);
extern E_CMBS_RC      app_SrvParamSet(E_CMBS_PARAM e_Param, u8 *pu8_Data, u16 u16_Length, u32 u32_Token);
void keyb_SwupStart(char *pszFileName, int piPacketSize)
{
    char szFileName[64] = { 0 }, cRestoreSubs = 0;

    memset(szFileName, 0, sizeof(szFileName));
    if (pszFileName)
    {
        strncpy(szFileName, pszFileName, (sizeof(szFileName) - 1));
        g_stSwupSetup.u16_PacketSize = (piPacketSize / 32) * 32;
        if (!g_stSwupSetup.u16_PacketSize)
        {
            g_stSwupSetup.u16_PacketSize = 32;
            cRestoreSubs = 0;
        }
    }
    else
    {
        printf("\nEnter firmware binary file name:\n");
        tcx_gets(szFileName, sizeof(szFileName));

        printf("Enter packet size, possible options listed below :\n");
        printf(" press 0 for - 32 \n");
        printf(" press 1 for - 64 \n");
        printf(" press 2 for - 128\n");
        printf(" press 3 for - 256\n");
        printf(" press 4 for - 512\n");
        g_stSwupSetup.u16_PacketSize = tcx_getch();
        g_stSwupSetup.u16_PacketSize -= '0';
        if (g_stSwupSetup.u16_PacketSize > 4)
        {
            g_stSwupSetup.u16_PacketSize = 4;
        }

        g_stSwupSetup.u16_PacketSize = 1 << (5 + g_stSwupSetup.u16_PacketSize);


        printf("\nRestore EEPROM parameters after the upgrade [y/n]? ");
        cRestoreSubs = tcx_getch();
        cRestoreSubs = cRestoreSubs == 'y' ? 1 : 0; // convert y/n to 1/0 (easier to work with)
        printf("\n");
    }

    if (cRestoreSubs)
    {
        // Get subscription data
        app_SrvEEPROMBackupCreate();
    }

    // perform firmware upgrade
    app_UpdateAndCheckVersion(szFileName, 0x0);

    if (cRestoreSubs)
    {
        // Set subscription data
        app_SrvEEPROMBackupRestore();
        printf("\nEEPROM data set successfully!...\n");
        app_SrvLocateSuggest(CMBS_ALL_HS_MASK); //Page all HSs
        tcx_getch();
    }
}

E_CMBS_RC keyb_SwupGetFileVersion(char *pFileName, u16 *pu16_Version)
{
    if (CMBS_RC_OK != app_SwupGetImageVersion(pFileName, pu16_Version))
    {
        return CMBS_RC_ERROR_GENERAL;
    }

    if ((*pu16_Version) == APP_SWUP_EMPTY_VERSION)
    {
        printf("Enter file version(in hex) : ");
        tcx_scanf("%hx", pu16_Version);
    }
    return CMBS_RC_OK;
}

/*
    Following data packet sizes can be set for FW update procedure – 32, 64, 128, 256, and 512.
    There are following restriction on choosing data packet size:
        for SPI  booter 1.2     –128 bytes (or less)
        for SPI booter  >= 1.3  – 512 bytes(or less).
        for UART                – 512 bytes(or less)
        Some USB boards require 32 bytes packet size only.
        for regular FW update used 32 bytes packets.
*/

void        keyb_SwupStartStressTests(bool isBooter)
{
    char chInput;

    printf("\nEnter 1-st Application file name       : ");
    tcx_scanf("%s", g_stSwupSetup.file_name1);
    if (CMBS_RC_OK != keyb_SwupGetFileVersion(g_stSwupSetup.file_name1, &(g_stSwupSetup.u16_Version1)))
    {
        printf("Error: Can't open file %s, press any key ... \n", g_stSwupSetup.file_name1);
        tcx_getch();
        return;
    }

    printf("Enter 2-nd Application file name       : ");
    tcx_scanf("%s", g_stSwupSetup.file_name2);

    if (CMBS_RC_OK != keyb_SwupGetFileVersion(g_stSwupSetup.file_name2, &(g_stSwupSetup.u16_Version2)))
    {
        printf("Error: Can't open file %s, press any key ... \n", g_stSwupSetup.file_name2);
        tcx_getch();
        return;
    }

    if (isBooter)     // Get additional 2 booter files
    {
        printf("Enter 1-st booter file name  : ");
        tcx_scanf("%s", g_stSwupSetup.booter_name1);
        if (CMBS_RC_OK != keyb_SwupGetFileVersion(g_stSwupSetup.booter_name1, &(g_stSwupSetup.u16_BooterVersion1)))
        {
            printf("Error: Can't open file %s, press any key ... \n", g_stSwupSetup.booter_name1);
            tcx_getch();
            return;
        }

        printf("Enter 2-nd booter file name  : ");
        tcx_scanf("%s", g_stSwupSetup.booter_name2);
        if (CMBS_RC_OK != keyb_SwupGetFileVersion(g_stSwupSetup.booter_name2, &(g_stSwupSetup.u16_BooterVersion2)))
        {
            printf("Error: Can't open file %s, press any key ... \n", g_stSwupSetup.booter_name2);
            tcx_getch();
            return;
        }

    }

    printf("Enter packet size, possible options listed below :\n");
    printf(" press 0 for - 32 \n");
    printf(" press 1 for - 64 \n");
    printf(" press 2 for - 128\n");
    printf(" press 3 for - 256\n");
    printf(" press 4 for - 512\n");
    tcx_scanf("%hu", &(g_stSwupSetup.u16_PacketSize));
    if (g_stSwupSetup.u16_PacketSize > 4)
    {
        g_stSwupSetup.u16_PacketSize = 4;
    }

    g_stSwupSetup.u16_PacketSize = 1 << (5 + g_stSwupSetup.u16_PacketSize);

    printf("Enter tests duration, in minutes: ");
    tcx_scanf("%hu", &(g_stSwupSetup.u16_DurationMinutes));

    printf("Enter log file name             : ");
    tcx_scanf("%s", g_stSwupSetup.logFileName);

    printf("\n ******************************************************* ");
    printf("\n ******************************************************* ");
    printf("\n ******************************************************* ");
    printf("\n Application file 1   = %s", g_stSwupSetup.file_name1);
    printf("\n Application version 1= %x", g_stSwupSetup.u16_Version1);
    printf("\n Application file 2   = %s", g_stSwupSetup.file_name2);
    printf("\n Application version 2= %x", g_stSwupSetup.u16_Version2);
    if (isBooter)
    {
        printf("\n Booter file 1   = %s", g_stSwupSetup.booter_name1);
        printf("\n Booter version 1= %x", g_stSwupSetup.u16_BooterVersion1);
        printf("\n Booter file 2   = %s", g_stSwupSetup.booter_name2);
        printf("\n Booter version 2= %x", g_stSwupSetup.u16_BooterVersion2);
    }
    printf("\n Packet size   = %d", g_stSwupSetup.u16_PacketSize);
    printf("\n Test duration = %d", g_stSwupSetup.u16_DurationMinutes);
    printf("\n Log File      = %s", g_stSwupSetup.logFileName);
    printf("\n Press Enter to start, or q to exit  \n\n");

#ifdef __linux__
    tcx_getch();
#endif

    chInput = tcx_getch();

    if ((chInput == '\n') || (chInput == '\r'))
    {
        app_SwupStartStressTests(isBooter);
    }
}

void keyb_SwupWithInvalidation(void)
{
    app_SwupApplicationInvalidate();

    //reboot system
    app_SrvSystemReboot();

    tcx_appClearScreen();
    //check target version - if it is booter version - make FW upgrade
    if ((g_CMBSInstance.u16_TargetVersion & 0xFF00) == 0x0)
        keyb_SwupStart(NULL, 0);
    else
        printf("ERROR!!! Expected Booter version number - received %d", g_CMBSInstance.u16_TargetVersion);
}
//  ========== keyb_SRVLoop ===========
/*!
        \brief         keyboard loop of test application
      \param[in,ou]  <none>
        \return

*/
void        keyb_SwupLoop(void)
{
    int n_Keep = TRUE;

    while (n_Keep)
    {
        //      tcx_appClearScreen();
        printf("-----------------------------  \n");
        printf("1 => Start FW update           \n");
        printf("2 => Start stress tests     \n");
        printf("3 => Start FW update (application-booter mismatch, [not for DCX81])\n");
        printf("4 => Start Booter stress tests \n");
        printf("- - - - - - - - - - - - - - -  \n");
        printf("q => Return to Interface Menu  \n");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case '1':
                keyb_SwupStart(NULL, 0);
                break;

            case '2':
                keyb_SwupStartStressTests(FALSE);
                break;

            case '3':
                keyb_SwupWithInvalidation();
                break;

            case '4':
                keyb_SwupStartStressTests(TRUE);
                break;

            case 'q':
                n_Keep = FALSE;
                break;

            default:
                break;
        }
    }
}

//*/
