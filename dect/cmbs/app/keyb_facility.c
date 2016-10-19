/*!
* \file  keyb_catiq.c
* \brief  firmware update test
* \Author  stein
*
* @(#) %filespec: keyb_facility.c~9.1.1 %
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
#include <time.h>

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
#include "appfacility.h"
#include "tcx_keyb.h"
#include <tcx_util.h>

#define CMBS_PROP_EVENT_MAX_PAYLOAD_SIZE 20

void keyb_CatIqMWI(void)
{
    u16               u16_RequestId, u16_Messages;
    u8                u8_LineId, u8_Type;
    E_CMBS_MWI_TYPE   eType;
    char              InputBuffer[20];

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    printf("Enter Line ID            (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_LineId = atoi(InputBuffer);

    printf("Enter number of messages (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_Messages = atoi(InputBuffer);

    printf("Enter message type: 0-Voice, 1-SMS, 2-Email: ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_Type = atoi(InputBuffer);
    switch (u8_Type)
    {
        case 1:
            eType = CMBS_MWI_SMS;
            break;

        case 2:
            eType = CMBS_MWI_EMAIL;
            break;

        default:
            eType = CMBS_MWI_VOICE;
            break;
    }

    printf("Enter handset mask ( e.g. 1,2,3,4 or none or all):\n");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    app_FacilityMWI(u16_RequestId, u8_LineId, u16_Messages, InputBuffer, eType);
}


void keyb_CatIqMissedCalls(void)
{
    u16            u16_RequestId, u16_Messages, u16_TotalNumMessages;
    u8             u8_LineId;
    bool           bNewMissedCall;
    char           InputBuffer[20];

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    printf("Enter Line ID            (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_LineId = atoi(InputBuffer);

    printf("Enter number of unread calls    (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_Messages = atoi(InputBuffer);

    printf("Enter number of total (read + unread) calls    (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_TotalNumMessages = atoi(InputBuffer);

    printf("Enter 1 - New missed call, 0 - No new missed calls: ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    bNewMissedCall = atoi(InputBuffer);

    printf("Enter handset mask ( e.g. 1,2,3,4 or none or all):\n");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    app_FacilityMissedCalls(u16_RequestId, u8_LineId, u16_Messages, InputBuffer, bNewMissedCall, u16_TotalNumMessages);
}


void keyb_CatIqListChanged(void)
{
    u16            u16_RequestId;
    u8             u8_ListId, u8_ListEntries, u8_LineId, u8_LineSubType;
    char           InputBuffer[20];

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    printf("Enter List ID            (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_ListId = atoi(InputBuffer);

    printf("Enter number of list entries  : ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_ListEntries = atoi(InputBuffer);

    printf("Enter Line ID (255 for all lines)  : ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_LineId = atoi(InputBuffer);

    printf("Enter Line Subtype (0 - Line identifier for external call, 3 - Relating-to line identifier, 4 - All lines  : ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_LineSubType = atoi(InputBuffer);

    printf("Enter handset mask ( e.g. 1,2,3,4 or none or all):\n");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    app_FacilityListChanged(u16_RequestId, u8_ListId, u8_ListEntries, InputBuffer, u8_LineId, u8_LineSubType);
}


void              keyb_CatIqWebContent(void)
{
    u16            u16_RequestId;
//   u8             u8_LineId;
    u8             u8_WebContents;
    char           InputBuffer[20];

    printf("Enter Request ID         (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    /*
       printf( "Enter Line ID            (dec): " );
       memset( InputBuffer, 0, sizeof(InputBuffer) );
       tcx_gets( InputBuffer, sizeof(InputBuffer) );
       u8_LineId = atoi( InputBuffer );
    */
    printf("Enter number of web contents  : ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_WebContents = atoi(InputBuffer);

    printf("Enter handset mask ( e.g. 1,2,3,4 or none or all):\n");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    app_FacilityWebContent(u16_RequestId, u8_WebContents, InputBuffer);
}


void              keyb_CatIqPropEvent(void)
{
    u16            u16_RequestId, u16_PropEvent = 0;
    u8             u8_DataLen, u8_Data[20];
    char           InputBuffer[64];
    int            i, j;

    printf("Enter Request ID          (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);
    /*
       printf( "Enter Proprietary Event   (hex): " );
       memset( InputBuffer, 0, sizeof(InputBuffer) );
       memset( u8_Data, 0, sizeof(u8_Data) );
       tcx_gets( InputBuffer, sizeof(InputBuffer) );
       for( i=0, j=0; i < 4; i++ )
       {
          u8_Data[i] = app_ASC2HEX( InputBuffer+j );
          j += 2;
       }
       u16_PropEvent = (u8_Data[0] << 8) | u8_Data[1];
    */
    printf("Enter Data Length (dec. max 20): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u8_DataLen = (u8)atoi(InputBuffer);
    u8_DataLen = MIN(CMBS_PROP_EVENT_MAX_PAYLOAD_SIZE, u8_DataLen);
    printf("Enter Data ( currently this raw data will be transmitted) (hex): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    memset(u8_Data, 0, sizeof(u8_Data));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    for (i = 0, j = 0; i < u8_DataLen; i++)
    {
        u8_Data[i] = app_ASC2HEX(InputBuffer + j);
        j += 2;
    }

    printf("Enter handset mask ( e.g. 1,2,3,4 or none or all):\n");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    app_FacilityPropEvent(u16_RequestId, u16_PropEvent, u8_Data, u8_DataLen, InputBuffer);
}


void              keyb_CatIqTimeUpdate(void)
{
    u16            u16_RequestId;
    char           InputBuffer[32];
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

    printf("Sending Date:20%02d-%02d-%02d Time:%02d:%02d:%02d Zone:0x%02d\n",
           st_DateAndTime.u8_Year, st_DateAndTime.u8_Month, st_DateAndTime.u8_Day,
           st_DateAndTime.u8_Hours, st_DateAndTime.u8_Mins, st_DateAndTime.u8_Secs,
           st_DateAndTime.u8_Zone);

    printf("Enter Request ID          (dec): ");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));
    u16_RequestId = atoi(InputBuffer);

    printf("Enter handset mask ( e.g. 1,2,3,4 or none or all):\n");
    memset(InputBuffer, 0, sizeof(InputBuffer));
    tcx_gets(InputBuffer, sizeof(InputBuffer));

    app_FacilityDateTime(u16_RequestId, &st_DateAndTime, InputBuffer);
}


//  ========== keyb_CatIqLoop ===========
/*!
  \brief         keyboard loop for CAT-iq menu
      \param[in,ou]  <none>
  \return

*/
void              keyb_CatIqLoop(void)
{
    int            n_Keep = TRUE;

    while (n_Keep)
    {
//      tcx_appClearScreen();
        printf("-----------------------------\n");
        printf("1 => Send Message Waiting Indication\n");
        printf("2 => Send Missed Calls Notification\n");
        printf("3 => Send List Changed Notification\n");
        printf("4 => Send Web Content Notification\n");
        printf("- - - - - - - - - - - - - - - \n");
        printf("5 => Send Proprietary Event\n");
        printf("6 => Send Date_Time Update Event\n");
        printf("- - - - - - - - - - - - - - - \n");
        printf("q => Return to Interface Menu\n");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case '1':
                keyb_CatIqMWI();
                break;

            case '2':
                keyb_CatIqMissedCalls();
                break;

            case '3':
                keyb_CatIqListChanged();
                break;

            case '4':
                keyb_CatIqWebContent();
                break;

            case '5':
                keyb_CatIqPropEvent();
                break;

            case '6':
                keyb_CatIqTimeUpdate();
                break;

            case 'q':
                n_Keep = FALSE;
                break;
        }
    }
}

//*/
