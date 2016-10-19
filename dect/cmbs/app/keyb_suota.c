/*!
* \file  keyb_suota.c
* \brief  cat-iq 2.0 data services tests
* \Author  stein
*
* @(#) %filespec: keyb_suota.c~7 %
*
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
#include "appsuota.h"
#include "tcx_keyb.h"
#include <tcx_util.h>

/** CMBS Changes  *   */
#include "suota/suota.h"

extern u16 app_HandsetMap(char * psz_Handsets);


void keyb_SuotaSendVersionAvailable(void)
{
    char InputBuffer[100];

    ST_SUOTA_UPGRADE_DETAILS  st_HSVerAvail;
    u16 u16_Handset = 0xFF, u16_RequestId;
    ST_VERSION_BUFFER st_SwVersion;

    printf("\n SUOTA Version Available command");

    // read HS number
    printf("\nEnter HS number: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_Handset = app_HandsetMap(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read delay in minutes
    printf("Enter delay in minutes: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    st_HSVerAvail.u16_delayInMin = atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // no settings for user interaction and URL nums
    st_HSVerAvail.u8_UserInteraction    = 0;
    st_HSVerAvail.u8_URLStoFollow       = 1;

    // read version strings
    printf("Enter SW version string(max 20 chars) : ");
    tcx_gets(InputBuffer, CMBS_MAX_VERSION_LENGTH);

    st_SwVersion.u8_VerLen = (u8)strlen(InputBuffer);

    st_SwVersion.type = CMBS_SUOTA_SW_VERSION;

    if (st_SwVersion.u8_VerLen <= CMBS_MAX_VERSION_LENGTH)
    {
        memcpy(st_SwVersion.pu8_VerBuffer, InputBuffer, st_SwVersion.u8_VerLen);
    }
    else
    {
        st_SwVersion.u8_VerLen = 0;
    }

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    app_SoutaSendHSVersionAvail(st_HSVerAvail, u16_Handset, &st_SwVersion, u16_RequestId);
}
//////////////////////////////////////////////////////////////////////////
void keyb_SuotaSendNewVersionInd(void)
{
    char    InputBuffer[20];
    u16     u16_Handset, u16_RequestId;

    printf("\nSUOTA New Version Ind command");

    // read HS number
    printf("\nEnter HS number: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_Handset = app_HandsetMap(InputBuffer);

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    app_SoutaSendNewVersionInd(u16_Handset, SUOTA_GE00_SU_FW_UPGRAGE, u16_RequestId);
}
//////////////////////////////////////////////////////////////////////////
void keyb_SuotaSendURL(void)
{
    ST_URL_BUFFER st_Url;
    char    InputBuffer[100];
    u16     u16_Handset, u16_RequestId ;
    u8      u8_URLToFollow;

    printf("\nSUOTA Send URL command");

    // read HS number
    printf("\nEnter HS number: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_Handset = app_HandsetMap(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read num of URLs
    printf("\nEnter num of URLs to follow: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_URLToFollow = atoi(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read URL string
    printf("\nEnter URL addres (max %d chars):\n", CMBS_MAX_URL_SIZE);
    tcx_gets(InputBuffer, CMBS_MAX_URL_SIZE);

    st_Url.u8_UrlLen    = (u8)strlen(InputBuffer);
    if (st_Url.u8_UrlLen <= CMBS_MAX_URL_SIZE)
    {
        memcpy(st_Url.pu8_UrlBuffer, InputBuffer, st_Url.u8_UrlLen);
    }
    else
    {
        st_Url.u8_UrlLen = 0;
    }

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    app_SoutaSendURL(u16_Handset, u8_URLToFollow, &st_Url, u16_RequestId);
}

void keyb_SuotaSendNack(void)
{
    char    InputBuffer[20];
    u16     u16_Handset, u16_RequestId;
    E_SUOTA_RejectReason enReason;

    printf("\nSUOTA Send NACK command");

    // read HS number
    printf("\nEnter HS number:");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_Handset = app_HandsetMap(InputBuffer);
    memset(InputBuffer, 0, sizeof(InputBuffer));

    // read reason
    printf("\nEnter reason:");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    enReason = atoi(InputBuffer);

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    app_SoutaSendNack(u16_Handset, enReason, u16_RequestId);
}


void keyb_SuotaQuit(void)
{
    // suota_quitThread();
}


void keyb_SuotaThread(int pushMode)
{
    char InputBuffer[60];
    char InputHsBuffer[20];
    u16 portNumber = 0;
    u16 u16_Handset = 0;


    static bool threadCreate = 0;

    printf("\nEnter Port Number xxxx: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    portNumber = atoi(InputBuffer);

    // Read Server IP Address
    printf("\nEnter IP Address xxx.xxx.xxx.xxx: ");
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    if (pushMode)
    {
        printf("\nEnter HS number: ");
        tcx_gets(InputHsBuffer, sizeof(InputHsBuffer));
        u16_Handset = app_HandsetMap(InputHsBuffer);
    }
    if (!threadCreate)
    {
        suota_createThread(InputBuffer, portNumber, pushMode, u16_Handset);
        threadCreate = 1;
    }
}


void keyb_SuotaLoop(void)
{
    int n_Keep = TRUE;

    while (n_Keep)
    {
        printf("------------------------------------\n");
        printf("1 => Send HS Version Available Ind  \n");
        printf("2 => Send New Version Ind           \n");
        printf("3 => Send URL indication         \n");
        printf("4 => Send NACK       \n");
        printf("5 => Start SUOTA Process     \n");
        printf("6 => Start Suota Push Mode    \n");
        printf("- - - - - - - - - - - - - - -       \n");
        printf("q => Return to Interface Menu       \n");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case '1':
                keyb_SuotaSendVersionAvailable();
                break;

            case '2':
                keyb_SuotaSendNewVersionInd();
                break;

            case '3':
                keyb_SuotaSendURL();
                break;

            case '4':
                keyb_SuotaSendNack();
                break;

            case '5':
                keyb_SuotaThread(0);
                break;

            case '6':
                keyb_SuotaThread(1);
                break;

            case 'q':
                keyb_SuotaQuit();
                n_Keep = FALSE;
                break;
        }
    }

}

/*---------[End Of File]---------------------------------------------------------------------------------------------------------------------------*/
