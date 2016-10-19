/*!
* \file  tcx_keyb.c
* \brief  test command line generator for manual testing
* \Author  kelbch
*
* @(#) %filespec: keyb_srv.c~48.9.1.7 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                \n
* ----------------------------------------------------------------------------\n
* 13-Jan-2014 tcmc_asa  -- GIT--  take checksum changes from 3.46.x to 3_x main (3.5x)
* 20-Dec-2013 tcmc_asa                 added CHECKSUM_SUPPORT
*
*   16-apr-09  kelbch  1.0   Initialize \n
*   14-dec-09  sergiym   ?   Add start/stop log menu \n
*   21-Oct-2011 tcmc_asa   48.1.11 Added Fixed carrier selection
*   26-Oct-2011 tcmc_asa   48.1.12 merged NBG 48.1.11 with BLR 48.1.12
*   17-Jan-2013 tcmc_asa   48.1.16.1.6.1.3  PR3613/3614 Internal call/name blocking
* 12-Jun-2013 tcmc_asa - GIT - add 2.99.x changes to 3.3x
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "cmbs_int.h"
#include "appcmbs.h"
#include "appsrv.h"
#include "appcall.h"
#include "tcx_log.h"
#include "tcx_keyb.h"
#include "ListsApp.h"
#include "appmsgparser.h"
#include "applog.h"
#include "tcx_eep.h"
#include "cmbs_int.h"
#include <tcx_util.h>
#ifdef WIN32
#define kbhit _kbhit
#else
extern int kbhit(void);
#endif

#define CMBS_IWU_DATA_MAX_TRANSMIT_LENGTH    59
#define CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH   51 //CMBS_IWU_DATA_MAX_TRANSMIT_LENGTH - size of header
#define CMBS_IWU_DATA_MAX_NEXT_PAYLOAD_LENGTH   53 //header is smaller for next messages because total length is not needed

void keyb_ParamAreaGetBySegments(u32 u32_Pos, u16 u16_Length, u8 *pu8_Data, u16 packet_max_size);
void keyb_ParamAreaSetBySegments(u32 u32_Pos, u16 u16_Length, u8 *pu8_Data, u16 packet_max_size);

#define HAN_DECT_SUBS_LENGTH 4096
#define HAN_ULE_SUBS_LENGTH 2048
#define HAN_FUN_SUBS_LENGTH 2048
#define MAX_PATH_SIZE 50
#define MAX_OVERNIGHT_TESTS 1000
extern st_AFEConfiguration AFEconfig;
extern u32 g_u32_UsedSlots;
extern ST_CMBS_API_INST g_CMBSInstance;
extern u8 g_EepromOnHost;

typedef enum {
    OK,
    NO_INPUT,
    TOO_LONG
} E_CMBS_RetVal;

/*****************************************************************************
*
*        Service Keyboard loop
*
******************************************************************************/
//  ========== keyb_EEPROMReset  ===========
/*!
\brief     Send CMBS_PARAM_RESET_ALL and wait for reconnection

\param[in,out]   <none>

\return    <none>

*/

void keyb_EEPROMReset(void)
{
    u8 u8_Dummy = 0;
    ST_APPCMBS_CONTAINER st_Container;

    app_SrvParamSet(CMBS_PARAM_RESET_ALL, &u8_Dummy, sizeof(u8_Dummy), 1);

    if (g_EepromOnHost)
    {
        // Wait till target finishes writing in eeprom file the flag for factory reset
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
    }

    appcmbs_WaitForContainer(CMBS_EV_DSR_SYS_START_RES, NULL);

}

void keyb_EEPROMBackupCreate(void)
{
    app_SrvEEPROMBackupCreate();

}

void     keyb_ParamFlexGet(void)
{
    char  buffer[12];
    u16   u16_Location;

    printf("Enter Location        (dec): ");
    tcx_gets(buffer, sizeof(buffer));
    u16_Location = atoi(buffer);

    printf("Enter Length (dec. max 512): ");
    tcx_gets(buffer, sizeof(buffer));

    app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM, u16_Location, (u16)atoi(buffer), 1);

}

//  ========== keyb_RFPISet ===========
/*!
\brief     set the RFPI parameter
\param[in,out]   <none>
\return    <none>
*/
void     keyb_RFPISet(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    char buffer[12];
    u8  u8_RFPI[CMBS_PARAM_RFPI_LENGTH];
    int  i, j = 0;
    u8  u8_Value[CMBS_PARAM_RFPI_LENGTH] = { 0 };


    // save parser state as we want to disable it temporary
    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    // get current RFPI
    app_SrvParamGet(CMBS_PARAM_RFPI, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u8_Value[0], st_Container.ch_Info,  CMBS_PARAM_RFPI_LENGTH);
    printf("Current RFPI: \t %.2X%.2X%.2X%.2X%.2X\n", u8_Value[0], u8_Value[1], u8_Value[2], u8_Value[3], u8_Value[4]);

    // ask for a new RFPI
    printf("Enter New RFPI: ");
    tcx_gets(buffer, sizeof(buffer));

    for (i = 0; i < 10; i += 2)
    {
        u8_RFPI[j] = app_ASC2HEX(buffer + i);
        j++;
    }

    // save new RFPI
    app_SrvParamSet(CMBS_PARAM_RFPI, u8_RFPI, sizeof(u8_RFPI), 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    app_SrvSystemReboot();

    // restore parser state
    app_set_msgparserEnabled(msgparserEnabled);
}

//  ========== _EcoMode2Str ===========
/*!
\brief     converts E_CMBS_ECO_MODE_TYPE value into string
\param[in,out]   u8  u8_Value
\return     char*
*/
char* _EcoMode2Str(u8  u8_Value)
{
    switch (u8_Value)
    {
        case CMBS_ECO_MODE_TYPE_LOW_POWER:
            return "ECO mode enabled low power (CMBS_ECO_MODE_TYPE_LOW_POWER)";
        case CMBS_ECO_MODE_TYPE_LOWEST_POWER:
            return "ECO mode enabled lowest power (CMBS_ECO_MODE_TYPE_LOWEST_POWER)";
        case CMBS_ECO_MODE_TYPE_NONE:
            return "ECO mode disabled (CMBS_ECO_MODE_TYPE_NONE)";
    }

    return "UNKNOWN";
}

//  ========== keyb_ECOModeSet ===========
/*!
\brief     set the ECO_MODE parameter
\param[in,out]   <none>
\return    <none>
*/
void     keyb_ECOModeSet(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    u8                   u8_Value = CMBS_ECO_MODE_TYPE_NONE;
    u32      u32_Value;

    // get current ECO MODE
    app_ProductionParamGet(CMBS_PARAM_ECO_MODE, CMBS_PARAM_ECO_MODE_LENGTH);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    SleepMs(100);
    printf("Current ECO Mode is: \t 0x%X  %s", st_Container.ch_Info[0], _EcoMode2Str(st_Container.ch_Info[0]));

    printf("\nEnter: \n%d-%s \n%d-%s \n%d-%s \n",
           CMBS_ECO_MODE_TYPE_NONE,  _EcoMode2Str(CMBS_ECO_MODE_TYPE_NONE),
           CMBS_ECO_MODE_TYPE_LOW_POWER, _EcoMode2Str(CMBS_ECO_MODE_TYPE_LOW_POWER),
           CMBS_ECO_MODE_TYPE_LOWEST_POWER, _EcoMode2Str(CMBS_ECO_MODE_TYPE_LOWEST_POWER));

    tcx_scanf("%d", &u32_Value);
    u8_Value = (u8)u32_Value;

    if (u8_Value < CMBS_ECO_MODE_TYPE_MAX)
    {
        app_ProductionParamSet(CMBS_PARAM_ECO_MODE, &u8_Value, sizeof(u8), 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
    }

    // get current ECO MODE
    app_ProductionParamGet(CMBS_PARAM_ECO_MODE, CMBS_PARAM_ECO_MODE_LENGTH);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    SleepMs(100);
    printf("Current ECO Mode is: \t 0x%X  %s", st_Container.ch_Info[0], _EcoMode2Str(st_Container.ch_Info[0]));

    printf("\nPress Any Key!\n");
    tcx_getch();

}

//  ========== _DectType2Str ===========
/*!
\brief     converts E_CMBS_DECT_TYPE value into string
\param[in,out]   u8  u8_Value
\return     char*
*/
char* _DectType2Str(u8  u8_Value)
{
    switch (u8_Value)
    {
        case CMBS_DECT_TYPE_EU:
            return "EU DECT (CMBS_DECT_TYPE_EU)";
        case CMBS_DECT_TYPE_US:
            return "US DECT (CMBS_DECT_TYPE_US)";
        case CMBS_DECT_TYPE_US_FCC:
            return "US FCC DECT (CMBS_DECT_TYPE_US_FCC)";
        case CMBS_DECT_TYPE_JAPAN:
            return "JAPAN DECT (CMBS_DECT_TYPE_JAPAN)";
        case CMBS_DECT_TYPE_JAPAN_FCC:
            return "JAPAN FCC DECT (CMBS_DECT_TYPE_JAPAN_FCC)";
    }

    return "UNKNOWN";
}

//  ========== keyb_DectTypeSet ===========
/*!
\brief     set the DECT TYPE parameter
\param[in,out]   <none>
\return    <none>
*/
void     keyb_DectTypeSet(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    u8  u8_Value;
    u32 u32_Value;

    // get current DECT Type
    app_ProductionParamGet(CMBS_PARAM_DECT_TYPE, CMBS_PARAM_DECT_TYPE_LENGTH);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    SleepMs(100);
    printf("Current DECT type is: 0x%X %s ", st_Container.ch_Info[0], _DectType2Str(st_Container.ch_Info[0]));

    printf("\n\nEnter DECT type: \n%d-%s \n%d-%s \n%d-%s \n%d-%s \n%d-%s \n ",
           CMBS_DECT_TYPE_EU,   _DectType2Str(CMBS_DECT_TYPE_EU),
           CMBS_DECT_TYPE_US,   _DectType2Str(CMBS_DECT_TYPE_US),
           CMBS_DECT_TYPE_US_FCC,  _DectType2Str(CMBS_DECT_TYPE_US_FCC),
           CMBS_DECT_TYPE_JAPAN,  _DectType2Str(CMBS_DECT_TYPE_JAPAN),
           CMBS_DECT_TYPE_JAPAN_FCC, _DectType2Str(CMBS_DECT_TYPE_JAPAN_FCC));

    tcx_scanf("%X", &u32_Value);
    u8_Value = (u8)u32_Value;

    if (u8_Value < CMBS_DECT_TYPE_MAX)
    {
        app_ProductionParamSet(CMBS_PARAM_DECT_TYPE, &u8_Value, sizeof(u8_Value), 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
    }
    if (st_Container.n_Info == TRUE)
    {
        // reboot module only in case the set was successful
        printf("Rebooting module...Please wait...\n");
        app_SrvSystemReboot();

        // get current DECT Type
        app_ProductionParamGet(CMBS_PARAM_DECT_TYPE, CMBS_PARAM_DECT_TYPE_LENGTH);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        SleepMs(100);
        printf("Current DECT type is: 0x%X %s ", st_Container.ch_Info[0], _DectType2Str(st_Container.ch_Info[0]));
    }
    else
    {
        printf("\n ERROR, could not set DECT type !!!");
        if ((u8_Value == CMBS_DECT_TYPE_JAPAN) || (u8_Value == CMBS_DECT_TYPE_JAPAN_FCC))
        {
            printf(" In case you wish to switch to Japan-DECT, first make sure to disable NEMo!\n");
        }
    }
    printf("Press Any Key!\n");
    tcx_getch();
}

//  ========== keyb_ChipSettingsSet ===========
/*!
\brief     set the tuning parameter of the chipset

\param[in,out]   <none>

\return    <none>

*/
void     keyb_ChipSettingsSet(void)
{
    char  buffer[21];
    u8    u8_Value;
    ST_APPCMBS_CONTAINER st_Container;

    u8 u8_RFPI[CMBS_PARAM_RFPI_LENGTH] = { 0 };
    bool isVegaOne = FALSE;
    u8 u8_RVBG = 0;
    u8 u8_RVREF = 0;
    u8 u8_RXTUN = 0;
    u8 u8_TestMode = 0;
    u32 u32_BG = 0;
    u32 u32_ADC = 0;
    u32 u32_PMU = 0;


    if (g_CMBSInstance.u8_HwChip ==  CMBS_HW_CHIP_VEGAONE)
        isVegaOne = TRUE;

    if (!isVegaOne)
    {
        app_SrvParamGet(CMBS_PARAM_AUXBGPROG, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&u32_BG, st_Container.ch_Info,  CMBS_PARAM_AUXBGPROG_LENGTH);

        app_SrvParamGet(CMBS_PARAM_ADC_MEASUREMENT, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&u32_ADC, st_Container.ch_Info,  CMBS_PARAM_ADC_MEASUREMENT_LENGTH);

        app_SrvParamGet(CMBS_PARAM_PMU_MEASUREMENT, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(&u32_PMU, st_Container.ch_Info,  CMBS_PARAM_PMU_MEASUREMENT_LENGTH);
    }
    else
    {
        app_SrvParamGet(CMBS_PARAM_RVBG, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_RVBG = st_Container.ch_Info[0];

        app_SrvParamGet(CMBS_PARAM_RVREF, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_RVREF = st_Container.ch_Info[0];
    }

    app_SrvParamGet(CMBS_PARAM_RXTUN, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_RXTUN = st_Container.ch_Info[0];

    app_SrvParamGet(CMBS_PARAM_TEST_MODE, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_TestMode = st_Container.ch_Info[0];

    app_SrvParamGet(CMBS_PARAM_RFPI, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(u8_RFPI, st_Container.ch_Info,  CMBS_PARAM_RFPI_LENGTH);

    tcx_appClearScreen();
    printf("-- Current chipset values: \n");
    if (!isVegaOne)
    {
        printf("DCIN2 ADC VALUE: \t%d mV\n", u32_ADC);
        printf("PMU VALUE:       \t%d mV\n", u32_PMU);
        printf("BG_Calibrate:    \t0x%X\n", u32_BG);
    }
    else
    {
        printf("RVBG:     \t%X\n", u8_RVBG);
        printf("RVREF:    \t%X\n", u8_RVREF);
    }
    printf("RXTUN:    \t%X\n", u8_RXTUN);
    printf("RFPI:     \t%.2X%.2X%.2X%.2X%.2X\n", u8_RFPI[0], u8_RFPI[1], u8_RFPI[2], u8_RFPI[3], u8_RFPI[4]);
    printf("TestMode: \t");
    switch (u8_TestMode)
    {
        case CMBS_TEST_MODE_NORMAL:
            printf("None\n");
            break;
        case CMBS_TEST_MODE_TBR6:
            printf("TBR6\n");
            break;
        case CMBS_TEST_MODE_TBR10:
            printf("TBR10\n");
            break;
    }

    printf("------------------------\n");

    if (isVegaOne)
    {
        printf("Enter RVBG: ");
        tcx_gets(buffer, sizeof(buffer));
        if (strlen(buffer))
        {
            u8_Value = app_ASC2HEX(buffer);
            app_SrvParamSet(CMBS_PARAM_RVBG, &u8_Value, sizeof(u8_Value), 1);

            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }

        printf("\nEnter RVREF: ");
        tcx_gets(buffer, sizeof(buffer));
        if (strlen(buffer))
        {
            u8_Value = app_ASC2HEX(buffer);
            app_SrvParamSet(CMBS_PARAM_RVREF, &u8_Value, sizeof(u8_Value), 1);

            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }
    }

    printf("\nEnter RXTUN: ");
    tcx_gets(buffer, sizeof(buffer));
    {
        u8_Value = app_ASC2HEX(buffer);
        app_SrvParamSet(CMBS_PARAM_RXTUN, &u8_Value, sizeof(u8_Value), 1);

        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
    }

    if (isVegaOne)
    {
        printf("\nEnter GFSK (10 bytes): ");
        tcx_gets(buffer, sizeof(buffer));
        {
            if (strlen(buffer) == 20)
            {
                u8 i;
                for (i = 0; i < 10; ++i)
                {
                    buffer[i] = app_ASC2HEX(&buffer[i * 2]);
                }

                app_SrvParamSet(CMBS_PARAM_GFSK, (u8 *)buffer, 10, 1);

                appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
            }
        }
    }
}
//  ========== keyb_ChipSettingsGet ===========
/*!
\brief     Shows the tuning parameter of the CMBS module

\param[in,out]   <none>

\return    <none>

*/

void     keyb_ChipSettingsGet(void)
{
    u8 u8_Value[13], i;
    ST_APPCMBS_CONTAINER st_Container;

    app_SrvParamGet(CMBS_PARAM_RVBG, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_Value[0] = st_Container.ch_Info[0];

    app_SrvParamGet(CMBS_PARAM_RVREF, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_Value[1] = st_Container.ch_Info[0];

    app_SrvParamGet(CMBS_PARAM_RXTUN, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_Value[2] = st_Container.ch_Info[0];

    app_SrvParamGet(CMBS_PARAM_GFSK, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u8_Value[3], st_Container.ch_Info,  10);

    tcx_appClearScreen();


    printf("Chipset settings\n");
    printf("RVBG: %02x\n", u8_Value[0]);
    printf("RVREF: %02x\n", u8_Value[1]);
    printf("RXTUN: %02x\n", u8_Value[2]);
    printf("GFSK: ");
    for (i = 0; i < 10; ++i)
    {
        printf("%X ", u8_Value[3 + i]);
    }

}
//  ========== keypb_EEPromParamGet ===========
/*!
\brief         Handle EEProm Settings get
\param[in,ou]  <none>
\return

*/

void keypb_param_test(void)
{
    int i;
    ST_APPCMBS_CONTAINER        st_Container;

    memset(&st_Container, 0, sizeof(st_Container));

    for (i = 0; i < 30; i++)
    {
        app_SrvParamGet(CMBS_PARAM_RFPI, 1);
        // wait for CMBS target message
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);

        /*        memcpy( &st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen );
        if(st_Resp.e_Response != CMBS_RESPONSE_OK)
        {
        printf("******** ERRROR CMBS_PARAM_RFPI!!!  on %d try ********\n",i);
        }
        */

        memset(&st_Container, 0, sizeof(st_Container));
        app_SrvParamGet(CMBS_PARAM_AUTH_PIN, 1);
        // wait for CMBS target message
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        /*        memcpy( &st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen );
        if(st_Resp.e_Response != CMBS_RESPONSE_OK)
        {
        printf("******** ERRROR CMBS_PARAM_AUTH_PIN!!!  on %d try ********\n",i);
        }
        */
        memset(&st_Container, 0, sizeof(st_Container));
        app_SrvFWVersionGet(CMBS_MODULE_CMBS, 1);
        // wait for CMBS target message
        appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
        /*        memcpy( &st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen );
        if(st_Resp.e_Response != CMBS_RESPONSE_OK)
        {
        printf("******** ERRROR CMBS_EV_DSR_FW_VERSION_GET_RES!!!  on %d try ********\n",i);
        }
        */
        memset(&st_Container, 0, sizeof(st_Container));
        app_SrvLineSettingsGet(0xFF, 1);
        // wait for CMBS target message
        appcmbs_WaitForContainer(CMBS_EV_DSR_LINE_SETTINGS_LIST_GET_RES, &st_Container);
        /*        memcpy( &st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen );
        if(st_Resp.e_Response != CMBS_RESPONSE_OK)
        {
        printf("******** ERRROR CMBS_EV_DSR_LINE_SETTINGS_LIST_GET_RES!!!  on %d try ********\n",i);
        }
        */
        memset(&st_Container, 0, sizeof(st_Container));
        app_SrvRegisteredHandsets(0xFFFF, 1);
        // wait for CMBS target message
        appcmbs_WaitForContainer(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES, &st_Container);
        /*        memcpy( &st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen );
        if(st_Resp.e_Response != CMBS_RESPONSE_OK)
        {
        printf("******** ERRROR CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES!!!  on %d try ********\n",i);
        }
        */
    }
}

void keyb_GetEntireEEPROM(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    char     file_name[FILENAME_MAX];
    u16      u16_EepromSize;
    u16      u16_Index = 0;
    FILE     *pFile = NULL;
    static const u8         u8_PacketSize = 128;

    printf("\nThis menu dumps entire eeprom to given file");
    printf("\nEnter file name: ");
    tcx_gets(file_name, sizeof(file_name));

    // open output file
    pFile = fopen(file_name, "wb");

    if (pFile == NULL)
    {
        printf("\n Can't open file %s ", file_name);
        return;
    }

    //get eeprom size
    app_SrvGetEepromSize();
    appcmbs_WaitForContainer(CMBS_EV_DSR_EEPROM_SIZE_GET_RES, &st_Container);

    //extract eeprom size
    memcpy(&u16_EepromSize, st_Container.ch_Info, sizeof(u16_EepromSize));
    printf("\n Eeprom size = %d ", u16_EepromSize);

    //read eeprom by 128 bytes
    while (u16_Index < u16_EepromSize)
    {
        appcmbs_PrepareRecvAdd(TRUE);
        app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM, u16_Index, MIN(u8_PacketSize, u16_EepromSize - u16_Index), 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container);
        u16_Index += u8_PacketSize;

        printf("\n packet size = %d ", st_Container.n_InfoLen);
        fwrite(st_Container.ch_Info, 1, st_Container.n_InfoLen, pFile);
        fflush(pFile);
    }

    fclose(pFile);
}

void  keypb_EEPromParamGet(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    int   nEvent;


    memset(&st_Container, 0, sizeof(st_Container));

    printf("Select EEProm Param:\n");
    printf("1 => RFPI\n");
    printf("2 => PIN\n");
    printf("3 => Chipset settings\n");
    printf("4 => TEST Mode\n");
    printf("5 => Master PIN\n");
    printf("6 => Flex EEprom get\n");
    printf("7 => Subscription data get\n");
    printf("8 => Generate eeprom file \n");
    printf("9 => PORBGCFG value \n");
    printf("a => AUXBGPROG value\n");
    printf("b => HAN DECT subscription DB start\n");
    printf("c => HAN DECT subscription DB end\n");
    printf("d => HAN ULE subscription DB start\n");
    printf("e => HAN ULE subscription DB end\n");
    printf("f => HAN FUN subscription DB start\n");
    printf("g => HAN FUN subscription DB end\n");
    printf("h => HAN next TPUI\n");
    printf("i => Pream Normal\n");
    printf("j => Full Power\n");
    printf("k => Low Power\n");
    printf("l => Lowest Power\n");
    printf("m => RF19APU MLSE\n");
    printf("n => RF19APU KCALOVR\n");
    printf("o => RF19APU_KCALOVR_LINEAR\n");
    printf("p => RF19APU Support FCC\n");
    printf("q => RF19APU Deviation\n");
    printf("r => RF19APU PA2 compatibility\n");
    printf("s => RFIC Selection\n");
    printf("t => MAX usable RSSI\n");
    printf("u => Lower RSSI Limit\n");
    printf("v => PHS Scan Parameter\n");
    printf("w => L1 - minus 82 dBm RSSI threshold for Japan regulation\n");
    printf("z => L2 - minus 62 dBm RSSI threshold for Japan regulation\n");

    printf("y => Maximum number of active calls of 1 PT\n");


    switch (tcx_getch())
    {
        case '1':
            app_SrvParamGet(CMBS_PARAM_RFPI, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case '2':
            app_SrvParamGet(CMBS_PARAM_AUTH_PIN, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case '3':
            keyb_ChipSettingsGet();
            printf("\nPress Any Key!\n");
            tcx_getch();
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            return;

        case '4':
            app_SrvParamGet(CMBS_PARAM_TEST_MODE, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case '5':
            app_SrvParamGet(CMBS_PARAM_MASTER_PIN, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case '6':
            keyb_ParamFlexGet();
            nEvent = CMBS_EV_DSR_PARAM_AREA_GET_RES;
            break;

        case '7':
            app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case '8':
            keyb_GetEntireEEPROM();
            nEvent = 0;
            break;

        case '9':
            app_SrvParamGet(CMBS_PARAM_PORBGCFG, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'a':
            app_SrvParamGet(CMBS_PARAM_AUXBGPROG_DIRECT, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'b':
            app_SrvParamGet(CMBS_PARAM_HAN_DECT_SUB_DB_START, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'c':
            app_SrvParamGet(CMBS_PARAM_HAN_DECT_SUB_DB_END, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'd':
            app_SrvParamGet(CMBS_PARAM_HAN_ULE_SUB_DB_START, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'e':
            app_SrvParamGet(CMBS_PARAM_HAN_ULE_SUB_DB_END, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'f':
            app_SrvParamGet(CMBS_PARAM_HAN_FUN_SUB_DB_START, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'g':
            app_SrvParamGet(CMBS_PARAM_HAN_FUN_SUB_DB_END, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'h':
            app_SrvParamGet(CMBS_PARAM_HAN_ULE_NEXT_TPUI, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'i':
            app_SrvParamGet(CMBS_PARAM_PREAM_NORM, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'j':
            app_SrvParamGet(CMBS_PARAM_RF_FULL_POWER, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'k':
            app_SrvParamGet(CMBS_PARAM_RF_LOW_POWER, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'l':
            app_SrvParamGet(CMBS_PARAM_RF_LOWEST_POWER, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'm':
            app_SrvParamGet(CMBS_PARAM_RF19APU_MLSE, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'n':
            app_SrvParamGet(CMBS_PARAM_RF19APU_KCALOVR, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'o':
            app_SrvParamGet(CMBS_PARAM_RF19APU_KCALOVR_LINEAR, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'p':
            app_SrvParamGet(CMBS_PARAM_RF19APU_SUPPORT_FCC, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'q':
            app_SrvParamGet(CMBS_PARAM_RF19APU_DEVIATION, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'r':
            app_SrvParamGet(CMBS_PARAM_RF19APU_PA2_COMP, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 's':
            app_SrvParamGet(CMBS_PARAM_RFIC_SELECTION, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 't':
            app_SrvParamGet(CMBS_PARAM_MAX_USABLE_RSSI, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'u':
            app_SrvParamGet(CMBS_PARAM_LOWER_RSSI_LIMIT, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'v':
            app_SrvParamGet(CMBS_PARAM_PHS_SCAN_PARAM, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'w':
            app_SrvParamGet(CMBS_PARAM_JDECT_LEVEL1_M82, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case 'z':
            app_SrvParamGet(CMBS_PARAM_JDECT_LEVEL2_M62, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        case '0':
            keypb_param_test();
            printf("Press Any Key!\n");
            tcx_getch();
            return;
            break;

        case 'y':
            app_SrvParamGet(CMBS_PARAM_MAX_NUM_ACT_CALLS_PT, 1);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;

        default:
            return;
    }
    // wait for CMBS target message
    if (nEvent != 0)
    {
        appcmbs_WaitForContainer(nEvent, &st_Container);
    }

    printf("Press Any Key!\n");
    tcx_getch();
}

//  ========== keypb_ProductionParamGet ===========
/*!
\brief         Handle Production Settings get
\param[in,ou]  <none>
\return

*/

void  keypb_ProductionParamGet(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    int   nEvent;
    u8 u8_parameter;

    memset(&st_Container, 0, sizeof(st_Container));

    printf("\nSelect Production Param:\n");
    printf("1 => ECO mode\n");
    printf("2 => Get DECT Type (EU, US, Japan, etc...)\n");

    u8_parameter = tcx_getch();
    switch (u8_parameter)
    {
        case '1':
            app_ProductionParamGet(CMBS_PARAM_ECO_MODE, CMBS_PARAM_ECO_MODE_LENGTH);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;
        case '2':
            app_ProductionParamGet(CMBS_PARAM_DECT_TYPE, CMBS_PARAM_DECT_TYPE_LENGTH);
            nEvent = CMBS_EV_DSR_PARAM_GET_RES;
            break;
        default:
            return;
    }

    // wait for CMBS target message
    appcmbs_WaitForContainer(nEvent, &st_Container);

    // give time to print debug output
    SleepMs(100);

    switch (u8_parameter)
    {
        case '1':   // CMBS_PARAM_ECO_MODE
            printf("Current ECO Mode is: \t 0x%X  %s", st_Container.ch_Info[0], _EcoMode2Str(st_Container.ch_Info[0]));
            break;
        default: // case '2':   // CMBS_PARAM_DECT_TYPE
            printf("Current DECT type is: 0x%X %s ", st_Container.ch_Info[0], _DectType2Str(st_Container.ch_Info[0]));
            break;
    }

    printf("\nPress Any Key!\n");
    tcx_getch();
}

void  keyb_ParamFlexSet(void)
{
    char buffer[256] = { 0 };
    u8  u8_Value[512] = { 0 };
    u8  u8_Data[512] = { 0 };
    u16 u16_Len = 0;
    u16 u16_Location, i, x;
    ST_APPCMBS_CONTAINER st_Container;

    // save parser state as we want to disable it temporary
    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    printf("Enter Location (dec): ");
    tcx_gets(buffer, sizeof(buffer));
    u16_Location = atoi(buffer);

    printf("Enter Length (dec. max 512): ");
    tcx_gets(buffer, sizeof(buffer));
    u16_Len = (u16)atoi(buffer);
    if (u16_Len > 512)
        u16_Len = 512;

    // read current value before writing a new one
    app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM, u16_Location, (u16)u16_Len, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container);
    memcpy(&u8_Value[0], st_Container.ch_Info,  u16_Len);

    tcx_appClearScreen();

    // display current data
    printf("CURRENT VALUE: \n");
    for (i = 0; i < u16_Len; i++) printf("%02x ", u8_Value[i]);
    printf("\n\n");

    // restore parser state
    app_set_msgparserEnabled(msgparserEnabled);

    // ask for new data
    printf("Enter New Data (hexadecimal): \n");
    tcx_gets(buffer, sizeof(buffer));

    for (i = 0, x = 0; i < u16_Len; i++)
    {
        u8_Data[i] = app_ASC2HEX(buffer + x);
        x += 2;
    }

    // set new data
    app_SrvParamAreaSet(CMBS_PARAM_AREA_TYPE_EEPROM, u16_Location, u16_Len, u8_Data, 1);

    // wait for response
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_SET_RES, &st_Container);

    // save parser state as we want to disable it temporary
    msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    // read current value before writing a new one
    app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM, u16_Location, (u16)u16_Len, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container);
    memcpy(&u8_Value[0], st_Container.ch_Info,  u16_Len);

    tcx_appClearScreen();

    // display current data
    printf("CURRENT VALUE: \n");
    for (i = 0; i < u16_Len; i++) printf("%02x ", u8_Value[i]);
    printf("\n\n");

    // restore parser state
    app_set_msgparserEnabled(msgparserEnabled);

    printf("\nPress any key...");
    tcx_getch();
}


void     keyb_RxtnGpioConnect(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    u32 u32_GPIO = 0;

    printf("Enter GPIO (0x00 -0x1B): ");
    tcx_scanf("%X", &u32_GPIO);

    appcmbs_PrepareRecvAdd(1);
    cmbs_dsr_RxtunGpioConnect(g_cmbsappl.pv_CMBSRef, (u16)u32_GPIO);
    appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_CONNECT_RES, &st_Container);
}

void     keyb_RxtnGpioDisconnect(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    appcmbs_PrepareRecvAdd(1);
    cmbs_dsr_RxtunGpioDisconnect(g_cmbsappl.pv_CMBSRef);
    appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_DISCONNECT_RES, &st_Container);
}

void     keyb_StartATETest(void)
{
    ST_IE_ATE_SETTINGS  st_AteSettings;
    u8                  u8_Ans;
    bool                isVegaOne = FALSE;
    char                hexbuffer[2] = { 0 };
    ST_APPCMBS_CONTAINER st_Container;

    // save parser state as we want to disable it temporary
    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);


    if (g_CMBSInstance.u8_HwChip ==  CMBS_HW_CHIP_VEGAONE)
        isVegaOne = TRUE;

    SleepMs(20);

    st_AteSettings.u8_NormalPreamble = 0;
    st_AteSettings.u8_PowerLevel = 0;
    st_AteSettings.u8_Pattern = 0;
    st_AteSettings.u8_Slot = 0;

    tcx_appClearScreen();
    printf("ATE test:");

    printf("\nEnter device identity (00-09):\n\t00 - 01 22 33 00 00\n\t01 - 01 22 33 11 10\n\t02 - 01 22 33 22 20\n\t03 - 01 22 33 33 30\n\t04 - 01 22 33 22 40\n\t05 - 01 22 33 22 50\n\t06 - 01 22 33 22 60\n\t07 - 01 22 33 22 70\n\t08 - 01 22 33 22 80\n\t09 - 01 22 33 22 90\nOR FF for default:\n");
    hexbuffer[0] = tcx_getch();
    hexbuffer[1] = tcx_getch();
    st_AteSettings.u8_DeviceIdentity = app_ASC2HEX(hexbuffer);
    printf("\nChosen device identity is = %X\n", st_AteSettings.u8_DeviceIdentity);

    printf("\nEnter slot type:\n\t0 - slot\n\t1 - double slot\n\t2 - long slot\n: ");
    u8_Ans = tcx_getch();
    switch (u8_Ans)
    {
        case '0':
            st_AteSettings.e_ATESlotType = CMBS_ATE_SLOT_TYPE;
            break;

        case '1':
            st_AteSettings.e_ATESlotType = CMBS_ATE_SLOT_TYPE_DOUBLE;
            break;

        case '2':
            st_AteSettings.e_ATESlotType = CMBS_ATE_SLOT_TYPE_LONG;
            break;

        default:
            printf("\nError parameter !\n");
            tcx_getch();
            return;
    }
    printf("\nEnter ATE test type:\n\t0 - RX\n\t1 - continuous RX\n\t2 - TX\n");
    if (!isVegaOne)
    {
        // currently continuous TX is not supported in V1
        printf("\t3 - continuous TX\n: ");
    }

    u8_Ans = tcx_getch();
    switch (u8_Ans)
    {
        case '0':
            st_AteSettings.e_ATEType = CMBS_ATE_TYPE_RX;
            break;

        case '1':
            st_AteSettings.e_ATEType = CMBS_ATE_TYPE_CONTINUOUS_RX;
            break;

        case '2':
            st_AteSettings.e_ATEType = CMBS_ATE_TYPE_TX;
            break;

        case '3':
            st_AteSettings.e_ATEType = CMBS_ATE_TYPE_CONTINUOUS_TX;
            break;

        default:
            printf("\nError parameter !\n");
            tcx_getch();
            return;
    }

    if (st_AteSettings.e_ATEType == CMBS_ATE_TYPE_CONTINUOUS_RX || st_AteSettings.e_ATEType == CMBS_ATE_TYPE_RX)
    {
        u8 quit = 0;
        // GPIO connect
        do
        {
            printf("\nInsert the GPIO (two hex digits, 03 - 1B) to use for measurement (FF for None): ");

            hexbuffer[0] = tcx_getch();
            hexbuffer[1] = tcx_getch();
            st_AteSettings.u8_Gpio = app_ASC2HEX(hexbuffer);

            if (st_AteSettings.u8_Gpio >= 0x03 && st_AteSettings.u8_Gpio <= 0x1B)
                quit = 1;
            else if (st_AteSettings.u8_Gpio == 0xFF)
                quit = 1;
        } while (!quit);
    }

    printf("\nEnter Instance (0..9): ");
    st_AteSettings.u8_Instance = tcx_getch() - '0';

    if (st_AteSettings.e_ATEType == CMBS_ATE_TYPE_RX)
    {
        printf("\nSync pattern: 0-FP, 1-PP: ");
        st_AteSettings.u8_Pattern = tcx_getch() - '0';
        printf("\nEnter Slot (two digits, 0..11): ");
        hexbuffer[0] = tcx_getch();
        hexbuffer[1] = tcx_getch();
        st_AteSettings.u8_Slot = app_ASC2HEX(hexbuffer);
    }
    printf("\nEnter Carrier 0..9 for ETSI DECT, 0..94 for WDCT, 23..27 for US DECT (decimal): ");
    hexbuffer[0] = tcx_getch();
    hexbuffer[1] = tcx_getch();
    st_AteSettings.u8_Carrier = atoi(hexbuffer);

    printf("\nEnter Ant (0,1): ");
    st_AteSettings.u8_Ant = tcx_getch() - '0';
    if (st_AteSettings.e_ATEType == CMBS_ATE_TYPE_TX)
    {
        printf("\nEnter Slot (two digits, 0..11): ");
        hexbuffer[0] = tcx_getch();
        hexbuffer[1] = tcx_getch();
        st_AteSettings.u8_Slot = app_ASC2HEX(hexbuffer);

        printf("\nEnter Pattern(0..4: [0,0x22,0xF0,0xDD,Fig41]): ");
        st_AteSettings.u8_Pattern = tcx_getch() - '0';

        printf("\nEnter Power Level (0,1 or 2): ");
        st_AteSettings.u8_PowerLevel = tcx_getch() - '0';

        printf("\nEnter Normal Preamble(y/n): ");
        u8_Ans = tcx_getch();
        switch (u8_Ans)
        {
            case 'y':
            case 'Y':
                st_AteSettings.u8_NormalPreamble = 1;
                break;

            case 'n':
            case 'N':
            default:
                st_AteSettings.u8_NormalPreamble = 0;
                break;
        }
    }
    else if (st_AteSettings.e_ATEType == CMBS_ATE_TYPE_RX)
    {
        printf("\nEnable BER-FER measurement (y/n): ");
        u8_Ans = tcx_getch();
        switch (u8_Ans)
        {
            case 'y':
            case 'Y':
            {
                char hexbuffer[10];
                printf("\nNumber of frames for ber measurement (hex, >0x64 for manual test):");
                hexbuffer[0] = tcx_getch();
                hexbuffer[1] = tcx_getch();
                st_AteSettings.u8_BERFERFrameCount = app_ASC2HEX(hexbuffer);
            }
            st_AteSettings.u8_BERFEREnabled = 1;
            break;

            case 'n':
            case 'N':
            default:
                st_AteSettings.u8_BERFEREnabled = 0;
                break;
        }
    }

    appcmbs_PrepareRecvAdd(TRUE);
    if (cmbs_dsr_ATETestStart(g_cmbsappl.pv_CMBSRef, &st_AteSettings) == CMBS_RC_OK)
    {
        appcmbs_WaitForEvent(CMBS_EV_DSR_ATE_TEST_START_RES);

        if (st_AteSettings.e_ATEType == CMBS_ATE_TYPE_RX)
        {
            if (st_AteSettings.u8_BERFEREnabled)
            {
                u8 u8_Buffer[CMBS_PARAM_BERFER_VALUE_LENGTH];

                u32 u32_Value = 0;

                printf("FER       BER     (per 0x%x frames)\n", st_AteSettings.u8_BERFERFrameCount);

                while (!kbhit())
                {
                    app_SrvParamGet(CMBS_PARAM_BERFER_VALUE, 1);
                    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
                    memcpy(&u8_Buffer, st_Container.ch_Info,  CMBS_PARAM_BERFER_VALUE_LENGTH);
                    memcpy(&u32_Value, u8_Buffer, 4); // Copy BERFER values

                    printf("BERFER: ");
                    if (u32_Value & 0xFF000000)
                    {
                        u16 u16_BERcnt;
                        u8 u8_FERcnt;

                        u16_BERcnt = u32_Value & 0xFFFF;
                        u8_FERcnt = (u32_Value >> 16) & 0xFF;

                        if (st_AteSettings.u8_BERFERFrameCount > 100)
                        {
                            u32 u32_Temp = 0;
                            if (u8_FERcnt != 100)
                                u32_Temp = (u16_BERcnt * 100000) / (320 * (100 - u8_FERcnt));

                            if (u32_Temp < 1000)
                            {
                                printf("FER(*10E-2) = %03d\tBER(*10E-5) = %4d:\n", u8_FERcnt, u32_Temp);
                            }
                            else if (u32_Temp < 10000)
                            {
                                printf("FER(*10E-2) = %03d\tBER(*10E-4) = %4d:\n", u8_FERcnt, (u32_Temp / 10));
                            }
                            else if (u32_Temp < 100000)
                            {
                                printf("FER(*10E-2) = %03d\tBER(*10E-3) = %4d:\n", u8_FERcnt, (u32_Temp / 100));
                            }
                            else if (u32_Temp < 1000000)
                            {
                                printf("FER(*10E-2) = %03d\tBER(*10E-2) = %4d:\n", u8_FERcnt, (u32_Temp / 1000));
                            }
                        }

                        else
                        {
                            printf("%03x\t%4x\n", u8_FERcnt, u16_BERcnt);
                        }

                        printf("RSSI MAX = %X, RSSI MIN = %X", u8_Buffer[4], u8_Buffer[5]);
                        SleepMs(1000);
                    }
                    else
                    {
                        printf("...\n");
                    }

                    SleepMs(200);
                }
            }
            else
            {
                u32 u32_ADC = 0;

                while (!kbhit())
                {
                    app_SrvParamGet(CMBS_PARAM_RSSI_VALUE, 1);
                    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
                    memcpy(&u32_ADC, st_Container.ch_Info,  CMBS_PARAM_RSSI_VALUE_LENGTH);

                    printf("\n\nRSSI: %d\n\n", u32_ADC);

                    SleepMs(200);
                }
            }
        }
    }
    else
        printf("Test was not started as parameters are wrong");

    app_set_msgparserEnabled(msgparserEnabled);

    printf("\nPress any key !\n");
    tcx_getch();
}

void     keyb_LeaveATETest(void)
{
    cmbs_dsr_ATETestLeave(g_cmbsappl.pv_CMBSRef);
    SleepMs(20);
    printf("\nPress any key !\n");
    tcx_getch();
}


void keyb_ListAccess_DeleteList(void)
{
    u8 u8_Ans;

    tcx_appClearScreen();

    printf("List Access Delete List\n");
    printf("========================\n\n");
    printf("select list:\n");
    printf("1: Missed calls List\n");
    printf("2: Outgoing calls List\n");
    printf("3: Incoming accepted calls List\n");
    printf("4: Line Settings List\n");
    printf("5: Contact List\n");

    printf("\n");

    u8_Ans = tcx_getch();

    switch (u8_Ans)
    {
        case '1':
        {
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_MISSED_CALLS);

            List_DeleteAllEntries(LIST_TYPE_MISSED_CALLS);

            printf("Missed calls list cleared!");
        }
        break;

        case '2':
        {
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_OUTGOING_CALLS);

            List_DeleteAllEntries(LIST_TYPE_OUTGOING_CALLS);

            printf("Outgoing calls list cleared!");
        }
        break;

        case '3':
        {
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_INCOMING_ACCEPTED_CALLS);

            List_DeleteAllEntries(LIST_TYPE_INCOMING_ACCEPTED_CALLS);

            printf("Incoming Accepted calls list cleared!");
        }
        break;

        case '4':
        {
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_LINE_SETTINGS_LIST);

            List_DeleteAllEntries(LIST_TYPE_LINE_SETTINGS_LIST);

            printf("Line Settings list cleared!");
        }
        break;

        case '5':
        {
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_CONTACT_LIST);

            List_DeleteAllEntries(LIST_TYPE_CONTACT_LIST);

            printf("Contact list cleared!");
        }
        break;

        default:
            printf("\nError parameter !\n");
            return;
    }
}

void keyb_ListAccess_DumpEntry(LIST_TYPE eListType, void *pv_Entry)
{
    const char *sTrue = "TRUE", *sFalse = "FALSE";

    switch (eListType)
    {
        case LIST_TYPE_CONTACT_LIST:
        {
            stContactListEntry *pst_Entry = pv_Entry;
            const char *sNum1Type, *sNum2Type, *sWork = "WORK", *sMobile = "MOBILE", *sFixed = "FIXED";

            switch (pst_Entry->cNumber1Type)
            {
                case NUM_TYPE_WORK:
                    sNum1Type = sWork;
                    break;
                case NUM_TYPE_MOBILE:
                    sNum1Type = sMobile;
                    break;
                default:
                    sNum1Type = sFixed;
                    break;
            }

            switch (pst_Entry->cNumber2Type)
            {
                case NUM_TYPE_WORK:
                    sNum2Type = sWork;
                    break;
                case NUM_TYPE_MOBILE:
                    sNum2Type = sMobile;
                    break;
                default:
                    sNum2Type = sFixed;
                    break;
            }

            printf("Entry Id            = %d\n", pst_Entry->u32_EntryId);
            printf("Last Name           = %s\n", pst_Entry->sLastName);
            printf("First Name          = %s\n", pst_Entry->sFirstName);
            printf("Number 1            = %s\n", pst_Entry->sNumber1);
            printf("Number 1 Type       = %s\n", sNum1Type);
            printf("Number 1 Default    = %s\n", pst_Entry->bNumber1Default ? sTrue : sFalse);
            printf("Number 1 Own        = %s\n", pst_Entry->bNumber1Own ? sTrue : sFalse);
            printf("Number 2            = %s\n", pst_Entry->sNumber2);
            printf("Number 2 Type       = %s\n", sNum2Type);
            printf("Number 2 Default    = %s\n", pst_Entry->bNumber2Default ? sTrue : sFalse);
            printf("Number 2 Own        = %s\n", pst_Entry->bNumber2Own ? sTrue : sFalse);
            printf("Associated Melody   = %d\n", pst_Entry->u32_AssociatedMelody);
            printf("Line Id             = %d\n", pst_Entry->u32_LineId);
        }
        break;

        case LIST_TYPE_LINE_SETTINGS_LIST:
        {
            stLineSettingsListEntry *pst_Entry = pv_Entry;
            printf("Entry Id                            = %d\n", pst_Entry->u32_EntryId);
            printf("Line Name                           = %s\n", pst_Entry->sLineName);
            printf("Line Id                             = %d\n", pst_Entry->u32_LineId);
            printf("Attached Hs Mask                    = 0x%X\n", pst_Entry->u32_AttachedHsMask);
            printf("Dial Prefix                         = %s\n", pst_Entry->sDialPrefix);
            printf("FP Melody                           = %d\n", pst_Entry->u32_FPMelody);
            printf("FP Volume                           = %d\n", pst_Entry->u32_FPVolume);
            printf("Blocked Number                      = %s\n", pst_Entry->sBlockedNumber);
            printf("Multi-Call Enabled                  = %s\n", pst_Entry->bMultiCalls ? sTrue : sFalse);
            printf("Intrusion Enabled                   = %s\n", pst_Entry->bIntrusionCall ? sTrue : sFalse);
            printf("Permanent CLIR set                  = %s\n", pst_Entry->bPermanentCLIR ? sTrue : sFalse);
            printf("Permanent CLIR Activation code      = %s\n", pst_Entry->sPermanentCLIRActCode);
            printf("Permanent CLIR DeActivation code    = %s\n", pst_Entry->sPermanentCLIRDeactCode);
            printf("Call Fwd Unconditional Enabled      = %s\n", pst_Entry->bCallFwdUncond ? sTrue : sFalse);
            printf("Call Fwd Uncond Activation code     = %s\n", pst_Entry->sCallFwdUncondActCode);
            printf("Call Fwd Uncond DeActivation code   = %s\n", pst_Entry->sCallFwdUncondDeactCode);
            printf("Call Fwd Uncond Number              = %s\n", pst_Entry->sCallFwdUncondNum);
            printf("Call Fwd No Answer Enabled          = %s\n", pst_Entry->bCallFwdNoAns ? sTrue : sFalse);
            printf("Call Fwd No Ans Activation code     = %s\n", pst_Entry->sCallFwdNoAnsActCode);
            printf("Call Fwd No Ans DeActivation code   = %s\n", pst_Entry->sCallFwdNoAnsDeactCode);
            printf("Call Fwd No Answer Number           = %s\n", pst_Entry->sCallFwdNoAnsNum);
            printf("Call Fwd No Answer Timeout          = %d\n", pst_Entry->u32_CallFwdNoAnsTimeout);
            printf("Call Fwd Busy Enabled               = %s\n", pst_Entry->bCallFwdBusy ? sTrue : sFalse);
            printf("Call Fwd Busy Activation code       = %s\n", pst_Entry->sCallFwdBusyActCode);
            printf("Call Fwd Busy DeActivation code     = %s\n", pst_Entry->sCallFwdBusyDeactCode);
            printf("Call Fwd Busy Number                = %s\n", pst_Entry->sCallFwdBusyNum);
        }
        break;

        default:
        {
            stCallsListEntry *pst_Entry = pv_Entry;
            const char *sCallType;
            struct tm *pst_Time;
            pst_Time = localtime(&(pst_Entry->t_DateAndTime));

            switch (pst_Entry->cCallType)
            {
                case CALL_TYPE_MISSED:
                    sCallType = "Missed Call";
                    break;
                case CALL_TYPE_OUTGOING:
                    sCallType = "Outgoing Call";
                    break;
                default:
                    sCallType = "Incoming Call";
                    break;
            }

            printf("Entry Id      = %d\n", pst_Entry->u32_EntryId);
            printf("Number        = %s\n", pst_Entry->sNumber);
            printf("Time - Year   = %d\n", pst_Time->tm_year + 1900);
            printf("Time - Month  = %d\n", pst_Time->tm_mon + 1);
            printf("Time - Day    = %d\n", pst_Time->tm_mday);
            printf("Time - Hour   = %d\n", pst_Time->tm_hour);
            printf("Time - Min    = %d\n", pst_Time->tm_min);
            printf("Time - Sec    = %d\n", pst_Time->tm_sec);
            printf("Line name     = %s\n", pst_Entry->sLineName);
            printf("Line ID       = %d\n", pst_Entry->u32_LineId);
            printf("Last name     = %s\n", pst_Entry->sLastName);
            printf("First name    = %s\n", pst_Entry->sFirstName);

            if (eListType == LIST_TYPE_MISSED_CALLS || eListType == LIST_TYPE_ALL_INCOMING_CALLS)
            {
                printf("Read          = %s\n", pst_Entry->bRead ? sTrue : sFalse);
                printf("Num Of Calls  = %d\n", pst_Entry->u32_NumOfCalls);
            }
            else if (eListType == LIST_TYPE_ALL_CALLS)
            {
                printf("Call Type     = %s\n", sCallType);
            }
        }
        break;
    }
}

void keyb_ListAccess_AddToSuppList(void)
{
    ST_APPCMBS_CONTAINER st_Container;

    printf("\nAdding Outgoing calls list...\n");
    cmbs_dsr_la_AddSupportedList(NULL, CMBS_LA_LIST_OUTGOING_CALLS);
    appcmbs_WaitForContainer(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS_RES, &st_Container);
    printf("\ndone.\n");

    printf("\nAdding All calls list...\n");
    cmbs_dsr_la_AddSupportedList(NULL, CMBS_LA_LIST_ALL_CALLS);
    appcmbs_WaitForContainer(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS_RES, &st_Container);
    printf("\ndone.\n");

    printf("\nAdding All incoming calls list...\n");
    cmbs_dsr_la_AddSupportedList(NULL, CMBS_LA_LIST_ALL_INCOMING_CALLS);
    appcmbs_WaitForContainer(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS_RES, &st_Container);
    printf("\ndone.\n");
}

void keyb_ListAccess_SpecificFields(void)
{
    u8 u8_Ans;
    LIST_RC rc;
    u32 u32_LineId, u32_AttachedHsMask;
    char ps8_Buffer[10];

    tcx_appClearScreen();

    printf("List Access Specifc Fields\n");
    printf("==========================\n\n");
    printf("1: Get Line Attached Handsets\n");
    printf("2: Set Line Attached Handsets\n");

    printf("\n");

    u8_Ans = tcx_getch();

    switch (u8_Ans)
    {
        case '1':
        {
            printf("Enter Line ID...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            u32_LineId = atoi(ps8_Buffer);

            rc = List_GetAttachedHs(u32_LineId, &u32_AttachedHsMask);
            if (rc != LIST_RC_OK)
            {
                printf("error: rc = %d\n", rc);
            }
            else
            {
                printf("Attached Hs Mask  = 0x%X\n", u32_AttachedHsMask);
            }
        }
        break;
        case '2':
        {
            printf("Enter Line ID...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            u32_LineId = atoi(ps8_Buffer);

            printf("Enter Attahced HS Mask (1 or 2 bytes)...\n");
            tcx_scanf("%x", &u32_AttachedHsMask);
            rc = List_SetAttachedHs(u32_LineId, u32_AttachedHsMask);
            if (rc != LIST_RC_OK)
            {
                printf("error: rc = %d\n", rc);
            }
            else
            {
                List_GetAttachedHs(u32_LineId, &u32_AttachedHsMask);
                printf("Attached Hs Mask  = 0x%X\n", u32_AttachedHsMask);
                printf("done.\n");
            }
        }
        break;
        default:
            printf("\nError parameter !\n");

            return;

    }
}

void keyb_ListAccess_FillContactList(void)
{
    stContactListEntry st_Entry;
    u32 pu32_FiledIDs[5], u32_EntryId;
    u32 i = 0, j = 0;

    pu32_FiledIDs[0] = FIELD_ID_LAST_NAME;
    pu32_FiledIDs[1] = FIELD_ID_FIRST_NAME;
    pu32_FiledIDs[2] = FIELD_ID_CONTACT_NUM_1;
    pu32_FiledIDs[3] = FIELD_ID_ASSOCIATED_MELODY;
    pu32_FiledIDs[4] = FIELD_ID_LINE_ID;

    st_Entry.u32_AssociatedMelody = 0;
    st_Entry.u32_LineId = 0;
    st_Entry.bNumber1Default = TRUE;
    st_Entry.bNumber1Own = FALSE;
    st_Entry.cNumber1Type = NUM_TYPE_FIXED;

    printf("\nFilling database with entries... \n");

    for (; i < 26; ++i)
    {
        for (j = 0; j < 10; ++j)
        {
            sprintf(st_Entry.sFirstName,  "%c%d", 'a' + i, j);
            sprintf(st_Entry.sLastName,  "%c%d", 'A' + i, j * 10);
            sprintf(st_Entry.sNumber1,  "%d", i * 10 + j * 100 + i);

            List_InsertEntry(LIST_TYPE_CONTACT_LIST, &st_Entry, pu32_FiledIDs, 5, &u32_EntryId);

            printf("\r%d%%", 100 * (10 * i + j + 1) / 260);
        }
    }

    printf("\nDone!\n");
}

void keyb_ListAccess_DumpList(void)
{
    u8 u8_Ans, pu8_Entry[LIST_ENTRY_MAX_SIZE];
    u32 u32_Count, u32_Index, pu32_Fields[FIELD_ID_MAX], u32_FieldsSize, u32_SortField, u32_SortField2 = FIELD_ID_INVALID, u32_NumOfEntries = 1;
    LIST_TYPE eListType;
    const char *sListName;

    tcx_appClearScreen();

    printf("List Access Display List Contents\n");
    printf("=================================\n\n");
    printf("select list:\n");
    printf("1: Contact List\n");
    printf("2: Line Settings List\n");
    printf("3: Missed calls List\n");
    printf("4: Outgoing Calls List\n");
    printf("5: Incoming Accepted Calls List\n");
    printf("6: All Calls List\n");
    printf("7: All Incoming Calls List\n");
    printf("\n");

    u8_Ans = tcx_getch();

    switch (u8_Ans)
    {
        case '1':
        {
            eListType = LIST_TYPE_CONTACT_LIST;
            sListName = "Contacts";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_LAST_NAME;
            pu32_Fields[2] = FIELD_ID_FIRST_NAME;
            pu32_Fields[3] = FIELD_ID_CONTACT_NUM_1;
            pu32_Fields[4] = FIELD_ID_CONTACT_NUM_2;
            pu32_Fields[5] = FIELD_ID_ASSOCIATED_MELODY;
            pu32_Fields[6] = FIELD_ID_LINE_ID;
            u32_FieldsSize = 7;

            u32_SortField   = FIELD_ID_LAST_NAME;
            u32_SortField2  = FIELD_ID_FIRST_NAME;
        }
        break;

        case '2':
        {
            eListType = LIST_TYPE_LINE_SETTINGS_LIST;
            sListName = "Line Settings";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_LINE_NAME;
            pu32_Fields[2] = FIELD_ID_LINE_ID;
            pu32_Fields[3] = FIELD_ID_ATTACHED_HANDSETS;
            pu32_Fields[4] = FIELD_ID_DIALING_PREFIX;
            pu32_Fields[5] = FIELD_ID_FP_MELODY;
            pu32_Fields[6] = FIELD_ID_FP_VOLUME;
            pu32_Fields[7] = FIELD_ID_BLOCKED_NUMBER;
            pu32_Fields[8] = FIELD_ID_MULTIPLE_CALLS_MODE;
            pu32_Fields[9] = FIELD_ID_INTRUSION_CALL;
            pu32_Fields[10] = FIELD_ID_PERMANENT_CLIR;
            pu32_Fields[11] = FIELD_ID_CALL_FWD_UNCOND;
            pu32_Fields[12] = FIELD_ID_CALL_FWD_NO_ANSWER;
            pu32_Fields[13] = FIELD_ID_CALL_FWD_BUSY;
            u32_FieldsSize = 14;

            u32_SortField = FIELD_ID_LINE_ID;
        }
        break;

        case '3':
        {
            eListType = LIST_TYPE_MISSED_CALLS;
            sListName = "Missed Calls";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_NUMBER;
            pu32_Fields[2] = FIELD_ID_LAST_NAME;
            pu32_Fields[3] = FIELD_ID_FIRST_NAME;
            pu32_Fields[4] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[5] = FIELD_ID_READ_STATUS;
            pu32_Fields[6] = FIELD_ID_LINE_NAME;
            pu32_Fields[7] = FIELD_ID_LINE_ID;
            pu32_Fields[8] = FIELD_ID_NUM_OF_CALLS;
            u32_FieldsSize = 9;

            u32_SortField = FIELD_ID_DATE_AND_TIME;
        }
        break;

        case '4':
        {
            eListType = LIST_TYPE_OUTGOING_CALLS;
            sListName = "Outgoing Calls";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_NUMBER;
            pu32_Fields[2] = FIELD_ID_LAST_NAME;
            pu32_Fields[3] = FIELD_ID_FIRST_NAME;
            pu32_Fields[4] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[5] = FIELD_ID_LINE_NAME;
            pu32_Fields[6] = FIELD_ID_LINE_ID;
            u32_FieldsSize = 7;

            u32_SortField = FIELD_ID_DATE_AND_TIME;
        }
        break;

        case '5':
        {
            eListType = LIST_TYPE_INCOMING_ACCEPTED_CALLS;
            sListName = "Incoming Accepted";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_NUMBER;
            pu32_Fields[2] = FIELD_ID_LAST_NAME;
            pu32_Fields[3] = FIELD_ID_FIRST_NAME;
            pu32_Fields[4] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[5] = FIELD_ID_LINE_NAME;
            pu32_Fields[6] = FIELD_ID_LINE_ID;
            u32_FieldsSize = 7;

            u32_SortField = FIELD_ID_DATE_AND_TIME;
        }
        break;

        case '6':
        {
            eListType = LIST_TYPE_ALL_CALLS;
            sListName = "All Calls";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_CALL_TYPE;
            pu32_Fields[2] = FIELD_ID_NUMBER;
            pu32_Fields[3] = FIELD_ID_LAST_NAME;
            pu32_Fields[4] = FIELD_ID_FIRST_NAME;
            pu32_Fields[5] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[6] = FIELD_ID_LINE_NAME;
            pu32_Fields[7] = FIELD_ID_LINE_ID;
            u32_FieldsSize = 8;

            u32_SortField = FIELD_ID_DATE_AND_TIME;
        }
        break;

        case '7':
        {
            eListType = LIST_TYPE_ALL_INCOMING_CALLS;
            sListName = "All Incoming Calls";

            pu32_Fields[0] = FIELD_ID_ENTRY_ID;
            pu32_Fields[1] = FIELD_ID_NUMBER;
            pu32_Fields[2] = FIELD_ID_LAST_NAME;
            pu32_Fields[3] = FIELD_ID_FIRST_NAME;
            pu32_Fields[4] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[5] = FIELD_ID_READ_STATUS;
            pu32_Fields[6] = FIELD_ID_LINE_NAME;
            pu32_Fields[7] = FIELD_ID_LINE_ID;
            pu32_Fields[8] = FIELD_ID_NUM_OF_CALLS;
            u32_FieldsSize = 9;

            u32_SortField = FIELD_ID_DATE_AND_TIME;
        }
        break;

        default:
            printf("\nError parameter !\n");
            return;
    }

    tcx_appClearScreen();

    printf("Contents of %s List:\n", sListName);
    printf("====================\n");

    /* Create if not exist */
    List_CreateList(eListType);

    List_GetCount(eListType, &u32_Count);
    printf("Total Num Of Entries = %d\n", u32_Count);

    for (u32_Index = 0; u32_Index < u32_Count; ++u32_Index)
    {
        u32 u32_StartIdx = u32_Index + 1;
        List_ReadEntries(eListType, &u32_StartIdx, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, u32_FieldsSize,
                         u32_SortField, u32_SortField2, pu8_Entry, &u32_NumOfEntries);

        printf("Entry #%d of %d:\n", u32_Index + 1, u32_Count);
        printf("===================\n");
        keyb_ListAccess_DumpEntry(eListType, pu8_Entry);
        printf("**********************************************\n");
    }
}

void keyb_ListAccess_InsertEntry(void)
{
    u8 u8_Ans;
    char ps8_Buffer[10];
    stCallsListEntry         st_CallListEntry;
    stLineSettingsListEntry  st_LineSettingsListEntry;
    stContactListEntry       st_ContactEntry;
    time_t t_Time;

    memset(&st_CallListEntry,           0, sizeof(st_CallListEntry));
    memset(&st_LineSettingsListEntry,   0, sizeof(st_LineSettingsListEntry));

    tcx_appClearScreen();

    printf("List Access Insert Entry\n");
    printf("========================\n\n");
    printf("select list:\n");
    printf("1: Missed calls List\n");
    printf("2: Outgoing calls List\n");
    printf("3: Incoming accepted calls List\n");
    printf("4: Line Settings List\n");
    printf("5: Contact List\n");

    printf("\n");

    u8_Ans = tcx_getch();

    switch (u8_Ans)
    {
        case '1':
        {
            u32 pu32_Fields[5], u32_FieldsNum = 5, u32_EntryId;
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_MISSED_CALLS);

            printf("Enter Number...\n");
            tcx_gets(st_CallListEntry.sNumber, sizeof(st_CallListEntry.sNumber));

            time(&t_Time);
            printf("Using current time as Date and Time...\n");
            st_CallListEntry.t_DateAndTime = t_Time;

            printf("Enter Line ID...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_CallListEntry.u32_LineId = atoi(ps8_Buffer);

            printf("Enter Number of calls...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_CallListEntry.u32_NumOfCalls = atoi(ps8_Buffer);

            printf("Enter Read Status (0 - Unread, 1 - Read)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_CallListEntry.bRead = atoi(ps8_Buffer);

            pu32_Fields[0] = FIELD_ID_NUMBER;
            pu32_Fields[1] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[2] = FIELD_ID_LINE_ID;
            pu32_Fields[3] = FIELD_ID_NUM_OF_CALLS;
            pu32_Fields[4] = FIELD_ID_READ_STATUS;

            List_InsertEntry(LIST_TYPE_MISSED_CALLS, &st_CallListEntry, pu32_Fields, u32_FieldsNum, &u32_EntryId);

            printf("Entry inserted. Entry Id = %d\n", u32_EntryId);
        }
        break;

        case '2':
        case '3':
        {
            u32 pu32_Fields[4], u32_FieldsNum = 3, u32_EntryId;
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList((u8_Ans == '2') ? LIST_TYPE_OUTGOING_CALLS : LIST_TYPE_INCOMING_ACCEPTED_CALLS);

            printf("Enter Number...\n");
            tcx_gets(st_CallListEntry.sNumber, sizeof(st_CallListEntry.sNumber));

            time(&t_Time);
            printf("Using current time as Date and Time...\n");
            st_CallListEntry.t_DateAndTime = t_Time;

            printf("Enter Line ID...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_CallListEntry.u32_LineId = atoi(ps8_Buffer);

            pu32_Fields[0] = FIELD_ID_NUMBER;
            pu32_Fields[1] = FIELD_ID_DATE_AND_TIME;
            pu32_Fields[2] = FIELD_ID_LINE_ID;

            List_InsertEntry((u8_Ans == '2') ? LIST_TYPE_OUTGOING_CALLS : LIST_TYPE_INCOMING_ACCEPTED_CALLS,
                             &st_CallListEntry, pu32_Fields, u32_FieldsNum, &u32_EntryId);

            printf("Entry inserted. Entry Id = %d\n", u32_EntryId);
        }
        break;

        case '4':
        {
            u32 pu32_Fields[13], u32_FieldsNum = 13, u32_EntryId;
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_LINE_SETTINGS_LIST);

            printf("Enter Line name...\n");
            tcx_gets(st_LineSettingsListEntry.sLineName, sizeof(st_LineSettingsListEntry.sLineName));

            printf("Enter Line ID...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.u32_LineId = atoi(ps8_Buffer);

            printf("Enter Attahced HS Mask (1 or 2 bytes)... (HEX)\n");
            tcx_scanf("%x", &st_LineSettingsListEntry.u32_AttachedHsMask);

            printf("Enter Dialing prefix...(any number or 'none')\n");
            tcx_gets(st_LineSettingsListEntry.sDialPrefix, sizeof(st_LineSettingsListEntry.sDialPrefix));
            if (strcmp(st_LineSettingsListEntry.sDialPrefix, "none") == 0)
            {
                st_LineSettingsListEntry.sDialPrefix[0] = 0;
            }

            printf("Enter FP Melody...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.u32_FPMelody = atoi(ps8_Buffer);

            printf("Enter FP Volume...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.u32_FPVolume = atoi(ps8_Buffer);

            printf("Enter Blocked telephone number...\n");
            tcx_gets(st_LineSettingsListEntry.sBlockedNumber, sizeof(st_LineSettingsListEntry.sBlockedNumber));

            printf("Enter Multicall enable (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.bMultiCalls = atoi(ps8_Buffer);

            printf("Enter Intrusion enable (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.bIntrusionCall = atoi(ps8_Buffer);

            printf("Enter Permanent CLIR enable (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.bPermanentCLIR = atoi(ps8_Buffer);

            printf("Enter Permanent CLIR Activation code...\n");
            tcx_gets(st_LineSettingsListEntry.sPermanentCLIRActCode, sizeof(st_LineSettingsListEntry.sPermanentCLIRActCode));

            printf("Enter Permanent CLIR Deactivation code...\n");
            tcx_gets(st_LineSettingsListEntry.sPermanentCLIRDeactCode, sizeof(st_LineSettingsListEntry.sPermanentCLIRDeactCode));

            printf("Enter Call Fwd Uncond enable (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.bCallFwdUncond = atoi(ps8_Buffer);

            printf("Enter Call Fwd Uncond Activation code...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdUncondActCode, sizeof(st_LineSettingsListEntry.sCallFwdUncondActCode));

            printf("Enter Call Fwd Uncond Deactivation code...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdUncondDeactCode, sizeof(st_LineSettingsListEntry.sCallFwdUncondDeactCode));

            printf("Enter Call Fwd Uncond Number...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdUncondNum, sizeof(st_LineSettingsListEntry.sCallFwdUncondNum));

            printf("Enter Call Fwd No Answer enable (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.bCallFwdNoAns = atoi(ps8_Buffer);

            printf("Enter Call Fwd No Answer Activation code...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdNoAnsActCode, sizeof(st_LineSettingsListEntry.sCallFwdNoAnsActCode));

            printf("Enter Call Fwd No Answer Deactivation code...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdNoAnsDeactCode, sizeof(st_LineSettingsListEntry.sCallFwdNoAnsDeactCode));

            printf("Enter Call Fwd No Answer Number...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdNoAnsNum, sizeof(st_LineSettingsListEntry.sCallFwdNoAnsNum));

            printf("Enter Call Fwd No Answer Timeout [0..64]...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.u32_CallFwdNoAnsTimeout = atoi(ps8_Buffer);

            printf("Enter Call Fwd Busy enable (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_LineSettingsListEntry.bCallFwdBusy = atoi(ps8_Buffer);

            printf("Enter Call Fwd Busy Activation code...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdBusyActCode, sizeof(st_LineSettingsListEntry.sCallFwdBusyActCode));

            printf("Enter Call Fwd Busy Deactivation code...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdBusyDeactCode, sizeof(st_LineSettingsListEntry.sCallFwdBusyDeactCode));

            printf("Enter Call Fwd Busy Number...\n");
            tcx_gets(st_LineSettingsListEntry.sCallFwdBusyNum, sizeof(st_LineSettingsListEntry.sCallFwdBusyNum));

            pu32_Fields[0] = FIELD_ID_LINE_NAME;
            pu32_Fields[1] = FIELD_ID_LINE_ID;
            pu32_Fields[2] = FIELD_ID_ATTACHED_HANDSETS;
            pu32_Fields[3] = FIELD_ID_DIALING_PREFIX;
            pu32_Fields[4] = FIELD_ID_FP_VOLUME;
            pu32_Fields[5] = FIELD_ID_FP_MELODY;
            pu32_Fields[6] = FIELD_ID_BLOCKED_NUMBER;
            pu32_Fields[7] = FIELD_ID_MULTIPLE_CALLS_MODE;
            pu32_Fields[8] = FIELD_ID_INTRUSION_CALL;
            pu32_Fields[9] = FIELD_ID_PERMANENT_CLIR;
            pu32_Fields[10] = FIELD_ID_CALL_FWD_UNCOND;
            pu32_Fields[11] = FIELD_ID_CALL_FWD_NO_ANSWER;
            pu32_Fields[12] = FIELD_ID_CALL_FWD_BUSY;

            List_InsertEntry(LIST_TYPE_LINE_SETTINGS_LIST, &st_LineSettingsListEntry, pu32_Fields, u32_FieldsNum, &u32_EntryId);

            /* Need to update target with Line Settings */
            {
                ST_IE_LINE_SETTINGS_LIST st_LineSettingsList;

                st_LineSettingsList.u16_Attached_HS     = (u16)st_LineSettingsListEntry.u32_AttachedHsMask;
                st_LineSettingsList.u8_Call_Intrusion   = st_LineSettingsListEntry.bIntrusionCall;
                st_LineSettingsList.u8_Line_Id          = (u8)st_LineSettingsListEntry.u32_LineId;
                st_LineSettingsList.u8_Multiple_Calls   = st_LineSettingsListEntry.bMultiCalls ? 4 : 0;
                st_LineSettingsList.e_LineType          = st_LineSettingsListEntry.bMultiCalls ? CMBS_LINE_TYPE_VOIP_PARALLEL_CALL : CMBS_LINE_TYPE_VOIP_DOUBLE_CALL;

                app_SrvLineSettingsSet(&st_LineSettingsList, 1);
            }

            printf("Entry inserted. Entry Id = %d\n", u32_EntryId);
        }
        break;

        case '5':
        {
            u32 pu32_Fields[5], u32_FieldsNum = 5, u32_EntryId;
            tcx_appClearScreen();

            /* Create if not exist */
            List_CreateList(LIST_TYPE_CONTACT_LIST);

            printf("Enter Last name...\n");
            tcx_gets(st_ContactEntry.sLastName, sizeof(st_ContactEntry.sLastName));

            printf("Enter First name...\n");
            tcx_gets(st_ContactEntry.sFirstName, sizeof(st_ContactEntry.sFirstName));

            printf("Enter Number...\n");
            tcx_gets(st_ContactEntry.sNumber1, sizeof(st_ContactEntry.sNumber1));

            printf("Enter Number Type: 0=Fixed, 1=Mobile, 2=Work...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_ContactEntry.cNumber1Type = atoi(ps8_Buffer);

            printf("Enter Is Number Default (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_ContactEntry.bNumber1Default = atoi(ps8_Buffer);

            printf("Enter Is Number Own (0 / 1)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_ContactEntry.bNumber1Own = atoi(ps8_Buffer);

            printf("Enter Associated Melody (0-255)...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_ContactEntry.u32_AssociatedMelody = atoi(ps8_Buffer);

            printf("Enter Line ID...\n");
            tcx_gets(ps8_Buffer, sizeof(ps8_Buffer));
            st_ContactEntry.u32_LineId = atoi(ps8_Buffer);

            pu32_Fields[0] = FIELD_ID_LAST_NAME;
            pu32_Fields[1] = FIELD_ID_FIRST_NAME;
            pu32_Fields[2] = FIELD_ID_CONTACT_NUM_1;
            pu32_Fields[3] = FIELD_ID_ASSOCIATED_MELODY;
            pu32_Fields[4] = FIELD_ID_LINE_ID;

            List_InsertEntry(LIST_TYPE_CONTACT_LIST, &st_ContactEntry, pu32_Fields, u32_FieldsNum, &u32_EntryId);

            printf("Entry inserted. Entry Id = %d\n", u32_EntryId);
        }
        break;

        default:
            printf("\nError parameter !\n");
            return;
    }
}

void keyb_ListAccess(void)
{
    u8 u8_Ans;

    tcx_appClearScreen();

    printf("List Access Operation\n");
    printf("=====================\n\n");
    printf("select option:\n");
    printf("1: Insert Entry to List\n");
    printf("2: Delete List\n");
    printf("3: Display List contents\n");
    printf("4: Fill contact list with entries\n");
    printf("5: Add optional lists to list of supporeted list\n");
    printf("6: List Access Specific fields\n");
    printf("\n");

    u8_Ans = tcx_getch();

    switch (u8_Ans)
    {
        case '1':
            keyb_ListAccess_InsertEntry();
            break;

        case '2':
            keyb_ListAccess_DeleteList();
            break;

        case '3':
            keyb_ListAccess_DumpList();
            break;

        case '4':
            keyb_ListAccess_FillContactList();
            break;

        case '5':
            keyb_ListAccess_AddToSuppList();
            break;

        case '6':
            keyb_ListAccess_SpecificFields();
            break;

        default:
            printf("\nError parameter !\n");
            return;
    }

    u8_Ans = tcx_getch();
    tcx_appClearScreen();
}

//  ========== keyb_BGCalibrationSettings ===========
/*!
\brief     Set Bandgap calibration arguments. DCIN & Resistor divider can be set other than SW defaults.
\param[in,out]   u8_BG_DCIN - The DCIN index used for BG calibration
\return    <none>
*/

void keyb_BGCalibrationSettings(u8 u8_BG_DCIN)
{
    ST_APPCMBS_CONTAINER st_Container;
    u8 u8_BG_ResistorFactor = 0;
    u8 u8_Getch;
    bool ValidSelect = FALSE;


    app_SrvParamGet(CMBS_PARAM_AUXBGP_RESISTOR_FACTOR, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_BG_ResistorFactor = st_Container.ch_Info[0];

    do
    {
        //    tcx_appClearScreen();
        printf("\n-----------------------------------------\n");
        printf("------ Aux BG Calibration Settings    -----\n");
        printf("-------------------------------------------\n");
        printf("Auxiliary Calibration DCIN  = DCIN%d\n", u8_BG_DCIN);
        printf("Auxiliary Calibration Resistor Factor  = %d\n", u8_BG_ResistorFactor);
        printf("-------------------------------------------\n");
        printf("a) Change Settings \n");
        printf("q) Keep Settings W/O changes \n");
        printf("\n Please Select!\n");

        u8_Getch = tcx_getch();
        switch (u8_Getch)
        {
            case 'a':
                printf("Select DCIN input\n");
                printf("0) DCIN0 \n");
                printf("1) DCIN1 \n");
                printf("2) DCIN2 \n");
                printf("3) DCIN3 \n");
                do
                {
                    printf("\nPlease select value between 0-3 \n");
                    u8_Getch = tcx_getch();
                } while ((u8_Getch < '0') || (u8_Getch > '3'));
                u8_BG_DCIN = u8_Getch - '0';

                do
                {
                    printf("\nPlease select Resistor Factor (1 - 4):\n");
                    u8_Getch = tcx_getch();
                } while ((u8_Getch < '1') || (u8_Getch > '4'));
                u8_BG_ResistorFactor = u8_Getch - '0';


                // u8 msgparserEnabled = app_get_msgparserEnabled();
                // app_set_msgparserEnabled(0);

                app_SrvParamSet(CMBS_PARAM_AUXBGP_DCIN, &u8_BG_DCIN, CMBS_PARAM_AUXBGP_DCIN_LENGTH, 1);
                appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

                app_SrvParamSet(CMBS_PARAM_AUXBGP_RESISTOR_FACTOR, &u8_BG_ResistorFactor, CMBS_PARAM_AUXBGP_RESISTOR_FACTOR_LENGTH, 1);
                appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

                // app_set_msgparserEnabled(msgparserEnabled);

                ValidSelect = TRUE;
                break;
            case 'q':
                ValidSelect = TRUE;
                break;
            default:
                break;
        }
    } while (!ValidSelect);

    SleepMs(100);
    printf("-------------------------------------------\n");
    printf("Auxiliary Calibration DCIN  = DCIN%d\n", u8_BG_DCIN);
    printf("Auxiliary Calibration Resistor Factor  = %d\n", u8_BG_ResistorFactor);
    printf("-------------------------------------------\n\n");

}

//  ========== keyb_ChipSettingsCalibrate_BG ===========
/*!
\brief     Calibration of BG
\param[in,out]   <none>
\return    <none>
*/

void keyb_ChipSettingsCalibrate_BG(void)
{
    u8 u8_Value[2] = { 0 };
    char buffer[16] = { 0 };
    u32 ValueVREF = 0;
    ST_APPCMBS_CONTAINER st_Container;

    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    printf("Enter Supply Volt [mV]\n");
    tcx_gets(buffer, sizeof(buffer));
    ValueVREF = atoi(buffer);
    memcpy(u8_Value, &ValueVREF, CMBS_PARAM_AUXBGPROG_LENGTH);

    printf("\nRunning Calibration: Input supply %d.  vref_1v1_FromDCDC (11010)\n", ValueVREF);

    app_SrvParamSet(CMBS_PARAM_AUXBGPROG, u8_Value, CMBS_PARAM_AUXBGPROG_LENGTH, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    app_set_msgparserEnabled(msgparserEnabled);
}



//  ========== keyb_ChipSettingsCalibrate_RVBG ===========
/*!
\brief     Calibration of RVBG
\param[in,out]   <none>
\return    <none>
*/

void keyb_ChipSettingsCalibrate_RVBG(void)
{
    u8 quit = 0;

    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    do
    {
        u8 u8_Value;
        u8 u8_Index = 0;
        u8 u8_Max = 0x7F;
        ST_APPCMBS_CONTAINER st_Container;

        u8 const u8_AllowedValue[] =
        {
            0x14, /* -99 mV */0x17, /* -93 mV */0x16, /* -87 mV */0x1C, /* -84 mV */
            0x11, /* -81 mV */0x1F, /* -78 mV */0x10, /* -75 mV */0x24, /* -74 mV */
            0x1E, /* -72 mV */0x13, /* -69 mV */0x27, /* -68 mV */0x19, /* -66 mV */
            0x12, /* -63 mV */0x26, /* -62 mV */0x18, /* -60 mV */0x2C, /* -59 mV */
            0x15, /* -57 mV */0x21, /* -56 mV */0x1B, /* -54 mV */0x2F, /* -53 mV */
            0x20, /* -50 mV */0x34, /* -49 mV */0x1A, /* -48 mV */0x2E, /* -47 mV */
            0x1D, /* -45 mV */0x23, /* -44 mV */0x37, /* -43 mV */0x29, /* -41 mV */
            0x22, /* -38 mV */0x36, /* -37 mV */0x28, /* -35 mV */0x3C, /* -34 mV */
            0x25, /* -32 mV */0x31, /* -31 mV */0x2B, /* -29 mV */0x3F, /* -28 mV */
            0x30, /* -25 mV */0x4, /* -24 mV */0x2A, /* -23 mV */0x3E, /* -22 mV */
            0x2D, /* -20 mV */0x33, /* -19 mV */0x7, /* -18 mV */0x39, /* -16 mV */
            0x32, /* -13 mV */0x6, /* -12 mV */0x38, /* -10 mV */0xC, /*  -9 mV */
            0x35, /*  -7 mV */0x1, /*  -6 mV */0x3B, /*  -4 mV */0xF, /*  -3 mV */
            0x0, /*   0 mV */0x44, /*   1 mV */0x3A, /*   2 mV */0xE, /*   3 mV */
            0x3D, /*   5 mV */0x3, /*   6 mV */0x47, /*   7 mV */0x9, /*   9 mV */
            0x2, /*  12 mV */0x46, /*  13 mV */0x8, /*  15 mV */0x4C, /*  16 mV */
            0x5, /*  18 mV */0x41, /*  19 mV */0xB, /*  21 mV */0x4F, /*  22 mV */
            0x40, /*  25 mV */0x54, /*  26 mV */0xA, /*  27 mV */0x4E, /*  28 mV */
            0xD, /*  30 mV */0x43, /*  31 mV */0x57, /*  32 mV */0x49, /*  34 mV */
            0x42, /*  37 mV */0x56, /*  38 mV */0x48, /*  40 mV */0x5C, /*  41 mV */
            0x45, /*  43 mV */0x51, /*  44 mV */0x4B, /*  46 mV */0x5F, /*  47 mV */
            0x50, /*  50 mV */0x64, /*  51 mV */0x4A, /*  52 mV */0x5E, /*  53 mV */
            0x4D, /*  55 mV */0x53, /*  56 mV */0x67, /*  57 mV */0x59, /*  59 mV */
            0x52, /*  62 mV */0x66, /*  63 mV */0x58, /*  65 mV */0x6C, /*  66 mV */
            0x55, /*  68 mV */0x61, /*  69 mV */0x5B, /*  71 mV */0x6F, /*  72 mV */
            0x60, /*  75 mV */0x74, /*  76 mV */0x5A, /*  77 mV */0x6E, /*  78 mV */
            0x5D, /*  80 mV */0x63, /*  81 mV */0x77, /*  82 mV */0x69, /*  84 mV */
            0x62, /*  87 mV */0x76, /*  88 mV */0x68, /*  90 mV */0x7C, /*  91 mV */
            0x65, /*  93 mV */0x71, /*  94 mV */0x6B, /*  96 mV */0x7F, /*  97 mV */
            0x70, /* 100 mV */0x6A, /* 102 mV */0x7E, /* 103 mV */0x6D, /* 105 mV */
            0x73, /* 106 mV */0x79, /* 109 mV */0x72, /* 112 mV */0x78, /* 115 mV */
            0x75, /* 118 mV */0x7B, /* 121 mV */0x7A, /* 127 mV */0x7D, /* 130 mV */
        };

        app_SrvParamGet(CMBS_PARAM_RVBG, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_Value = st_Container.ch_Info[0];

        /*find index*/
        for (u8_Index = 0; u8_Index < (u8_Max + 1); u8_Index++)
        {
            if (u8_Value == u8_AllowedValue[u8_Index])
                break;
        }

        if (u8_Index == (u8_Max + 1))
        {
            /* Value was not found in the list. i.e. illegal value */
            /* so force to value that is in both lists */
            u8_Index = 0;
        }

        printf("\nUse < or > to adjust the value, 'q' to quit the menu:\n");
        printf("\nRVBG: 0x%X\n", u8_Value);

        switch (tcx_getch())
        {
            case '>':
                if (u8_Index < u8_Max)
                    u8_Index++;
                break;
            case '<':
                if (u8_Index > 0)
                    u8_Index--;
                break;
            case 'q':
                quit = 1;
                break;
        }

        u8_Value = u8_AllowedValue[u8_Index];

        if (!quit)
        {
            app_SrvParamSet(CMBS_PARAM_RVBG, &u8_Value, sizeof(u8_Value), 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }

    } while (!quit);

    app_set_msgparserEnabled(msgparserEnabled);
}

//  ========== keyb_ChipSettingsCalibrate_RVREF ===========
/*!
\brief     Calibration of RVREF
\param[in,out]   <none>
\return    <none>
*/
void keyb_ChipSettingsCalibrate_RVREF(void)
{
    u8 quit = 0;

    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    do
    {
        u8 u8_Value;
        u8 u8_Max = 0x3F;
        ST_APPCMBS_CONTAINER st_Container;

        app_SrvParamGet(CMBS_PARAM_RVREF, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_Value = st_Container.ch_Info[0];

        printf("\nUse < or > to adjust the value, 'q' to quit the menu:\n");
        printf("\nRVREF: 0x%X\n", u8_Value);

        switch (tcx_getch())
        {
            case '>':
                if (u8_Value < u8_Max)
                    u8_Value++;
                break;
            case '<':
                if (u8_Value > 0)
                    u8_Value--;
                break;
            case 'q':
                quit = 1;
                break;
        }

        if (!quit)
        {
            app_SrvParamSet(CMBS_PARAM_RVREF, &u8_Value, sizeof(u8_Value), 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }

    } while (!quit);

    app_set_msgparserEnabled(msgparserEnabled);
}

//  ========== keyb_ChipSettingsCalibrate_RXTUN ===========
/*!
\brief     Calibration of RXTUN
\param[in,out]   <none>
\return    <none>
*/
void keyb_ChipSettingsCalibrate_RXTUN(void)
{
    u8 quit = 0;
    u32 u32_GPIO = 0;

    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    // GPIO connect
    do
    {
        ST_APPCMBS_CONTAINER st_Container;
        printf("Insert the GPIO (0x00 - 0x1B) to use for measurement (0xFF for None): ");
        tcx_scanf("%X", &u32_GPIO);

        if (u32_GPIO <= 0x1B)
        {
            printf("Connecting GPIO 0x%X...\n", u32_GPIO);
            appcmbs_PrepareRecvAdd(1);
            cmbs_dsr_RxtunGpioConnect(g_cmbsappl.pv_CMBSRef, (u16)u32_GPIO);
            appcmbs_WaitForContainer(CMBS_EV_DSR_GPIO_CONNECT_RES, &st_Container);
            quit = 1;
        }

        else if (u32_GPIO == 0xFF)
            quit = 1;
    } while (!quit);

    // Value calibration
    quit = 0;
    do
    {
        u8 u8_Value;
        ST_APPCMBS_CONTAINER st_Container;

        app_SrvParamGet(CMBS_PARAM_RXTUN, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_Value = st_Container.ch_Info[0];

        printf("\nUse < or > to adjust the value, 'q' to quit the menu:\n");
        printf("\nRXTUN: 0x%X\n", u8_Value);

        switch (tcx_getch())
        {
            case '>':
                u8_Value++;
                break;
            case '<':
                u8_Value--;
                break;
            case 'q':
                quit = 1;
                break;
        }

        if (!quit)
        {
            app_SrvParamSet(CMBS_PARAM_RXTUN, &u8_Value, sizeof(u8_Value), 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }

    } while (!quit);

    printf("Disconnecting GPIO 0x%X...\n", u32_GPIO);
    cmbs_dsr_RxtunGpioDisconnect(g_cmbsappl.pv_CMBSRef);

    app_set_msgparserEnabled(msgparserEnabled);
}

//  ========== keyb_ChipSettingsCalibrate ===========
/*!
\brief     provides calibration menu
\param[in,out]   <none>
\return    <none>
*/
void     keyb_ChipSettingsCalibrate(void)
{
    u8 quit = 0;

    do
    {
        u8 u8_RFPI[CMBS_PARAM_RFPI_LENGTH] = { 0 };
        bool isVegaOne = FALSE;
        u8 u8_RVBG = 0;
        u8 u8_RVREF = 0;
        u8 u8_RXTUN = 0;
        u8 u8_TestMode = CMBS_TEST_MODE_NORMAL;
        u8 u8_DectType = CMBS_DECT_TYPE_EU;
        u32 u32_BG = 0;
        u32 u32_ADC = 0;
        u32 u32_PMU = 0;
        u8  u8_AntSwitch = CMBS_ANT_SWITCH_NONE;
        u8 u8_BG_DCIN = 2; //DCIN2 is the default DCIN. But this is anyway updated from target side.
        ST_APPCMBS_CONTAINER st_Container;


        if (g_CMBSInstance.u8_HwChip ==  CMBS_HW_CHIP_VEGAONE)
            isVegaOne = TRUE;

        if (!isVegaOne)
        {
            app_SrvParamGet(CMBS_PARAM_AUXBGPROG, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(&u32_BG, st_Container.ch_Info,  CMBS_PARAM_AUXBGPROG_LENGTH);

            app_SrvParamGet(CMBS_PARAM_ADC_MEASUREMENT, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(&u32_ADC, st_Container.ch_Info,  CMBS_PARAM_ADC_MEASUREMENT_LENGTH);

            app_SrvParamGet(CMBS_PARAM_PMU_MEASUREMENT, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(&u32_PMU, st_Container.ch_Info,  CMBS_PARAM_PMU_MEASUREMENT_LENGTH);
        }
        else
        {
            app_SrvParamGet(CMBS_PARAM_RVBG, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            u8_RVBG = st_Container.ch_Info[0];

            app_SrvParamGet(CMBS_PARAM_RVREF, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            u8_RVREF = st_Container.ch_Info[0];
        }

        app_SrvParamGet(CMBS_PARAM_AUXBGP_DCIN, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_BG_DCIN = st_Container.ch_Info[0];


        app_SrvParamGet(CMBS_PARAM_RXTUN, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_RXTUN = st_Container.ch_Info[0];

        app_SrvParamGet(CMBS_PARAM_TEST_MODE, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_TestMode = st_Container.ch_Info[0];

        if (u8_TestMode == CMBS_TEST_MODE_TBR6)
        {
            app_SrvParamGet(CMBS_PARAM_ANT_SWITCH_MASK, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            u8_AntSwitch = st_Container.ch_Info[0];
        }

        app_SrvParamGet(CMBS_PARAM_RFPI, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        memcpy(u8_RFPI, st_Container.ch_Info,  CMBS_PARAM_RFPI_LENGTH);

        // get current DECT Type
        app_ProductionParamGet(CMBS_PARAM_DECT_TYPE, CMBS_PARAM_DECT_TYPE_LENGTH);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
        u8_DectType = st_Container.ch_Info[0];


        tcx_appClearScreen();
        printf("\n-------------------------\n");
        printf("-- CALIBRATION MENU    --\n");
        printf("-------------------------\n");
        printf("Select Parameter:\n");
        if (!isVegaOne)
        {
            printf("DCIN%d ADC VALUE: \t%d mV\n", u8_BG_DCIN, u32_ADC);
            printf("PMU VALUE:       \t%d mV\n", u32_PMU);
            printf("a) BG_Calibrate: \t0x%X\n", u32_BG);
        }
        else
        {
            printf("a) RVBG:     \t%X\n", u8_RVBG);
            printf("b) RVREF:    \t%X\n", u8_RVREF);
        }
        printf("c) RXTUN:    \t%X\n", u8_RXTUN);
        printf("d) RFPI:     \t%.2X%.2X%.2X%.2X%.2X\n", u8_RFPI[0], u8_RFPI[1], u8_RFPI[2], u8_RFPI[3], u8_RFPI[4]);
        printf("e) TestMode: \t");
        switch (u8_TestMode)
        {
            case CMBS_TEST_MODE_NORMAL:
                printf("None\n");
                break;
            case CMBS_TEST_MODE_TBR6:
                printf("TBR6\n");
                break;
            case CMBS_TEST_MODE_TBR10:
                printf("TBR10\n");
                break;
        }

        // Antenna switch for TBR6
        if (u8_TestMode == CMBS_TEST_MODE_TBR6)
        {
            printf("n) Antenna:\t%X (%s)\n", u8_AntSwitch, u8_AntSwitch ? (u8_AntSwitch == CMBS_ANT_SWITCH_ANT0 ? "Antenna 1" : "Antenna 2") : "None");
        }

        printf("f) DECT Type: \t0x%X %s\n\n", u8_DectType, _DectType2Str(u8_DectType));

        printf("s) Start ATE tests\n");
        printf("u) Stop ATE tests\n\n");

        printf("x) EEPROM set\n");
        printf("z) EEPROM reset\n\n");

        printf("r) Reboot target\n\n");

        printf("q) Quit\n");

        switch (tcx_getch())
        {
            case 'a':
                if (!isVegaOne)
                {
                    keyb_BGCalibrationSettings(u8_BG_DCIN);
                    keyb_ChipSettingsCalibrate_BG();
                }
                else
                {
                    keyb_ChipSettingsCalibrate_RVBG();
                }
                break;
            case 'b':
                keyb_ChipSettingsCalibrate_RVREF();
                break;
            case 'c':
                keyb_ChipSettingsCalibrate_RXTUN();
                break;
            case 'd':
                keyb_RFPISet();
                break;
            case 'e':
            {
                u8 u8_Value = CMBS_TEST_MODE_NORMAL;

                if (u8_TestMode == CMBS_TEST_MODE_NORMAL)
                    u8_Value = CMBS_TEST_MODE_TBR6;
                else if (u8_TestMode == CMBS_TEST_MODE_TBR6)
                    u8_Value = CMBS_TEST_MODE_TBR10;

                app_SrvParamSet(CMBS_PARAM_TEST_MODE, &u8_Value, sizeof(u8_Value), 1);
                appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

                // timeout to apply changes on target side
                SleepMs(3000);

                app_SrvSystemReboot();

                break;
            }
            case 'n':
            {
                u8 u8_Value = CMBS_ANT_SWITCH_NONE;
                if (u8_AntSwitch == CMBS_ANT_SWITCH_NONE)
                    u8_Value = CMBS_ANT_SWITCH_ANT0;
                else if (u8_AntSwitch == CMBS_ANT_SWITCH_ANT0)
                    u8_Value = CMBS_ANT_SWITCH_ANT1;

                app_SrvParamSet(CMBS_PARAM_ANT_SWITCH_MASK, &u8_Value, sizeof(u8_Value), 1);
                appcmbs_WaitForEvent(CMBS_EV_DSR_PARAM_SET_RES);

                SleepMs(3000);

                break;
            }
            break;
            case 'f':
                keyb_DectTypeSet();
                break;
            case 's':
                keyb_StartATETest();
                break;
            case 'u':
                keyb_LeaveATETest();
                break;
            case 'x':
                keyb_ParamFlexSet();
                break;
            case 'z':
                keyb_EEPROMReset();
                break;
            case 'r':
                app_SrvSystemReboot();
                break;
            case 'q':
                quit = 1;
                break;
        }

    } while (!quit);
}


//  ========== keypb_EEPromParamSet ===========
/*!
\brief         Handle EEProm Settings set
\param[in,ou]  <none>
\return

*/
void  keypb_EEPromParamSet(void)
{
    char buffer[16];
    int i = 0;
    int j = 0;
    u8 u8_Getch;

    // get current settings
    //keyb_ChipSettingsGet();

    // show menu
    printf("Select Parameter:\n");
    printf("1 => RFPI\n");
    printf("2 => PIN\n");
    printf("3 => Chipset parameters\n");
    printf("4 => RXTUN GPIO connect\n");
    printf("5 => RXTUN GPIO disconnect\n");
    printf("6 => Test mode\n");
    printf("7 => Flex EEprom set\n");
    printf("8 => PORBGCFG\n");
    printf("9 => AUXBGPROG\n");
    printf("0 => Reset EEprom\n");
    printf("* => Overnight EEPROM reset test\n");
    printf("a => HAN next TPUI\n");
    printf("i => Pream Normal\n");
    printf("j => Full Power\n");
    printf("k => Low Power\n");
    printf("l => Lowest Power\n");
    printf("m => RF19APU MLSE\n");
    printf("n => RF19APU KCALOVR\n");
    printf("o => RF19APU_KCALOVR_LINEAR\n");
    printf("p => RF19APU Support FCC\n");
    printf("q => RF19APU Deviation\n");
    printf("r => RF19APU PA2 compatibility\n");
    printf("s => RFIC Selection\n");
    printf("t => MAX usable RSSI\n");
    printf("u => Lower RSSI Limit\n");
    printf("v => PHS Scan Parameter\n");
    printf("w => L1 - minus 82 dBm RSSI threshold for Japan regulation\n");
    printf("z => L2 - minus 62 dBm RSSI threshold for Japan regulation\n");
    printf("x => Chipset Tunes\n");
    printf("y => Maximum number of call per HS\n");

    u8_Getch = tcx_getch();
    switch (u8_Getch)
    {
        case '1':
            keyb_RFPISet();
            break;

        case '2':
            printf("\n");
            printf("New PIN Code (4-digit): ");
            tcx_gets(buffer, sizeof(buffer));

            /* change to FFFFxxxx form (Expected by app_SrvPINCodeSet) */
            for (i = 0; i < 4; ++i)
            {
                buffer[i + 4] = buffer[i];
                buffer[i] = 'f';
            }
            buffer[8] = 0;

            app_SrvPINCodeSet(buffer);
            printf("Press Any Key!\n");
            tcx_getch();
            break;

        case '3':
            keyb_ChipSettingsSet();
            break;

        case '4':
            keyb_RxtnGpioConnect();
            break;

        case '5':
            keyb_RxtnGpioDisconnect();
            break;

        case '6':
        {
            ST_APPCMBS_CONTAINER st_Container;
            u8 u8_Value;


            printf("New Test mode value (hex, 00=>disabled, 81=>TBR6, 82=>TBR10): ");

            memset(buffer, 0, sizeof(buffer));
            tcx_gets(buffer, sizeof(buffer));

            if (strlen(buffer))
            {
                u8_Value = app_ASC2HEX(buffer);
                app_SrvParamSet(CMBS_PARAM_TEST_MODE, &u8_Value, sizeof(u8_Value), 1);
                appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

                // timeout to apply changes on target side
                SleepMs(3000);
            }
        }
        break;

        case '7':
            keyb_ParamFlexSet();
            break;

        case '8':
        {
            ST_APPCMBS_CONTAINER st_Container;
            u8 u8_Value = 0;
            u32 u32_Value;

            printf("\n");
            printf("New value for PORBGCFG: ");
            tcx_scanf("%X", &u32_Value);
            u8_Value = (u8)u32_Value;

            app_SrvParamSet(CMBS_PARAM_PORBGCFG, &u8_Value, sizeof(u8_Value), 1);

            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }
        break;

        case '9':
        {
            ST_APPCMBS_CONTAINER st_Container;
            char auxbg_buffer[10];
            char auxbg_buffer_hex[CMBS_PARAM_AUXBGPROG_LENGTH];

            printf("\n");
            printf("New value for AUXBGPROG (1 byte): ");
            tcx_gets(auxbg_buffer, sizeof(auxbg_buffer));

            for (i = 0; i < CMBS_PARAM_AUXBGPROG_LENGTH * 2; i += 2)
            {
                auxbg_buffer_hex[j] = app_ASC2HEX(auxbg_buffer + i);
                j++;
            }
            app_SrvParamSet(CMBS_PARAM_AUXBGPROG_DIRECT, (u8 *)auxbg_buffer_hex, sizeof(auxbg_buffer_hex), 1);

            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }
        break;

        case '*':
        {
            u32 u32_NumOfTests = 0, u32_Current = 0;
            printf("\nEEPROM reset overnight tests");
            printf("\nPlease enter number of tests... (max 1000)");
            tcx_gets(buffer, sizeof(buffer));
            u32_NumOfTests = (u32)atoi(buffer);
            u32_NumOfTests = MIN(MAX_OVERNIGHT_TESTS, u32_NumOfTests);
            while (++u32_Current <= u32_NumOfTests)
            {
                printf("\nRunning test # %d out of requested %d...", u32_Current, u32_NumOfTests);
                keyb_EEPROMReset();
            }

            printf("\nCompleted. # of tests performed = %d. Hit any key to continue...\n", u32_NumOfTests);
            tcx_getch();
        }
        break;

        case '0':
        {
            keyb_EEPROMReset();
        }
        break;

        case 'a':
        {

            ST_APPCMBS_CONTAINER st_Container;
            u8 buff[16] = { 0 };
            u8 buff_hex[CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH] = { 0 };
            u8 u8_Value[CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH] = { 0 };
            u8 i, j = 0;

            app_SrvParamGet(CMBS_PARAM_HAN_ULE_NEXT_TPUI, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(&u8_Value[0], st_Container.ch_Info,  CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH);
            printf("Current TPUI: \t %.2X%.2X%.2X\n", u8_Value[0], u8_Value[1], u8_Value[2]);

            printf("\n");
            printf("HAN Next TPUI (6-digit): ");
            tcx_gets((char *)buff, sizeof(buff));

            for (i = 0; i < CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH * 2; i += 2)
            {
                buff_hex[j] = app_ASC2HEX((char *)buff + i);
                j++;
            }

            app_SrvParamSet(CMBS_PARAM_HAN_ULE_NEXT_TPUI, buff_hex, CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

        }
        break;

        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'z':
        {
            ST_APPCMBS_CONTAINER st_Container;
            u8 u8_Value = 0;
            u32 u32_Value;
            E_CMBS_PARAM e_cmbsParam;

            switch (u8_Getch)
            {
                case 'i':
                    e_cmbsParam = CMBS_PARAM_PREAM_NORM;
                    break;
                case 'j':
                    e_cmbsParam = CMBS_PARAM_RF_FULL_POWER;
                    break;
                case 'k':
                    e_cmbsParam = CMBS_PARAM_RF_LOW_POWER;
                    break;
                case 'l':
                    e_cmbsParam = CMBS_PARAM_RF_LOWEST_POWER;
                    break;
                case 'm':
                    e_cmbsParam = CMBS_PARAM_RF19APU_MLSE;
                    break;
                case 'n':
                    e_cmbsParam = CMBS_PARAM_RF19APU_KCALOVR;
                    break;
                case 'o':
                    e_cmbsParam = CMBS_PARAM_RF19APU_KCALOVR_LINEAR;
                    break;
                case 'p':
                    e_cmbsParam = CMBS_PARAM_RF19APU_SUPPORT_FCC;
                    break;
                case 'q':
                    e_cmbsParam = CMBS_PARAM_RF19APU_DEVIATION;
                    break;
                case 'r':
                    e_cmbsParam = CMBS_PARAM_RF19APU_PA2_COMP;
                    break;
                case 's':
                    e_cmbsParam = CMBS_PARAM_RFIC_SELECTION;
                    break;
                case 't':
                    e_cmbsParam = CMBS_PARAM_MAX_USABLE_RSSI;
                    break;
                case 'u':
                    e_cmbsParam = CMBS_PARAM_LOWER_RSSI_LIMIT;
                    break;
                case 'v':
                    e_cmbsParam = CMBS_PARAM_PHS_SCAN_PARAM;
                    break;
                case 'w':
                    e_cmbsParam = CMBS_PARAM_JDECT_LEVEL1_M82;
                    break;
                case 'z':
                    e_cmbsParam = CMBS_PARAM_JDECT_LEVEL2_M62;
                    break;
            }

            printf("\n");
            printf("New value (in Hex): ");
            tcx_scanf("%X", &u32_Value);
            u8_Value = (u8)u32_Value;

            app_SrvParamSet(e_cmbsParam, &u8_Value, sizeof(u8_Value), 1);

            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }
        break;

        case 'x':
            keyb_ChipSettingsCalibrate();
            break;

        case 'y':
        {
            ST_APPCMBS_CONTAINER st_Container;
            u8 u8_Value = 0;
            u32 u32_Value;

            printf("\n");
            printf("New value for max number of call per HS is: ");
            tcx_scanf("%X", &u32_Value);
            u8_Value = (u8)u32_Value;

            app_SrvParamSet(CMBS_PARAM_MAX_NUM_ACT_CALLS_PT, &u8_Value, sizeof(u8_Value), 1);

            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
        }
        break;

    }
}

//  ========== keypb_ProductionParamSet ===========
/*!
\brief         Handle Production Settings set
\param[in,ou]  <none>
\return

*/
void  keypb_ProductionParamSet(void)
{
    printf("Select EEProm Param:\n");
    printf("1 => ECO mode\n");
    printf("2 => Set DECT Type(Japan, ...)\n");

    switch (tcx_getch())
    {
        case '1':
            keyb_ECOModeSet();
            break;
        case '2':
            keyb_DectTypeSet();
            break;
            break;

        default:
            return;
    }
}

//  ========== keypb_HandsetPage ===========
/*!
\brief         Page a handset or all handsets
\param[in,ou]  <none>
\return

*/
void  keypb_HandsetPage(void)
{
    /*char buffer[20];  //Host of CMBS supports today HS Locator to all handlets only.

    printf( "Enter handset mask ( e.g. 1,2,3,4 or none or all):\n" );
    tcx_gets( buffer, sizeof(buffer) );

    app_SrvHandsetPage( "all");*/

    cmbs_dsr_hs_Page(g_cmbsappl.pv_CMBSRef, CMBS_ALL_HS_MASK); // use maximal possible mask
}

//      ========== keypb_HandsetStopPaging ===========
/*!
\brief         Stop page handsets
\param[in,ou]  <none>
\return

*/
void  keypb_HandsetStopPaging(void)
{

    app_SrvHandsetStopPaging();

}


//  ========== keypb_HandsetDelete ===========
/*!
\brief         Delete a handset or all handsets
\param[in,ou]  <none>
\return

*/
void  keypb_HandsetDelete(void)
{
    char buffer[20];

    printf("Enter handset mask ( e.g. 1,2,3,4,....,max hs or none or all):\n");
    tcx_gets(buffer, sizeof(buffer));

    app_SrvHandsetDelete(buffer);

}

//  ========== keypb_SYSRegistrationMode ===========
/*!
\brief         Subscription on/off
\param[in,ou]  <none>
\return

*/
void  keypb_SYSRegistrationMode(void)
{
    u32 u32_Timeout_Value;

    printf("\n1 => Registration open\n");
    printf("2 => Registration close\n");

    switch (tcx_getch())
    {
        case '1':
            printf("\nSet timeout in seconds\n");
            tcx_scanf("%d", &u32_Timeout_Value);

            app_SrvSubscriptionOpen(u32_Timeout_Value);
            break;
        case '2':
            app_SrvSubscriptionClose();
            break;
    }
}

//  ========== keypb_SYSEncryptionMode ===========
/*!
\brief         Encryption on/off
\param[in,ou]  <none>
\return

*/
void  keypb_SYSEncryptionMode(void)
{
    printf("\n1 => Disable Encryption\n");
    printf("2 => Enable Encryption\n");

    switch (tcx_getch())
    {
        case '1':
            app_SrvEncryptionDisable();
            break;
        case '2':
            app_SrvEncryptionEnable();
            break;
    }
}

//  ========== keypb_SYSFPCustomFeatures ===========
/*!
\brief         Internal call and ConfCall enable/disable
\param[in,ou]  <none>
\return

*/

void  keypb_SYSFPCustomFeatures(void)
{
    u8 u8_Value = 0;
    u8 u8_IntCall;
    u8 u8_ConfCall;
    s8 s8_Char;
    ST_APPCMBS_CONTAINER st_Container;

    /* get actual setting from target */
    app_SrvParamGet(CMBS_PARAM_FP_CUSTOM_FEATURES, 1);

    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    u8_Value = st_Container.ch_Info[0];

    /* print and change setting until exit */
    do
    {
        u8_IntCall  = u8_Value & 0x01;
        u8_ConfCall = u8_Value & 0x02;

        printf("\n actual status: \n");
        if (u8_IntCall)
        {
            printf("  Internal call disabled\n");
        }
        else
        {
            printf("  Internal call enabled\n");
        }
        if (u8_ConfCall)
        {
            printf("  2-Line Call transfer disabled\n");
        }
        else
        {
            printf("  2-Line Call transfer enabled\n");
        }

        printf("\n1 => Disable Internal call\n");
        printf("2 => Enable  Internal call\n");
        printf("3 => Disable 2-Line Conference call\n");
        printf("4 => Enable  2-Line Conference call\n");
        printf("\ns => Save new setting and exit (send to target)\n");
        printf("q => exit without saving changes \n");
        printf("Select: ...  ");

        s8_Char = tcx_getch(); //  atoi
        printf("\n");

        switch (s8_Char)
        {
            case '1':
                u8_Value |= 0x01;
                break;

            case '2':
                u8_Value &= ~0x01;
                break;

            case '3':
                u8_Value |= 0x02;
                break;

            case '4':
                u8_Value &= ~0x02;
                break;
        }

    } while ((s8_Char != 's') && (s8_Char != 'q'));

    if (s8_Char == 's')
    {
        /* send new setting to target */
        app_SrvParamSet(CMBS_PARAM_FP_CUSTOM_FEATURES, &u8_Value, sizeof(u8_Value), 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);
    }
}

//  ========== keypb_SYSFixedCarrierSet ===========
/*!
\brief         Set or reset Fixed carrier for RF testing
\param[in,ou]  <none>
\return

*/
void  keypb_SYSFixedCarrierSet(void)
{
    char buffer[10];
    u8 u8_Value;

    printf("\nSet fixed Carrier for TX measurement (2 digit hex)");
    printf("\n                e.g. for 2 use 02, FF for disable: ");

    memset(buffer, 0, sizeof(buffer));
    tcx_gets(buffer, sizeof(buffer));

    if (strlen(buffer))
    {
        u8_Value = app_ASC2HEX(buffer);

        // 0x80 for enable)
        if (u8_Value == 0xFF)
        {
            // disable
            u8_Value = 0;
        }
        else
        {
            // add MSB (0x80) for enable
            u8_Value |= 0x80;
        }


        // app_SrvParamSet( CMBS_PARAM_FIXED_CARRIER, &u8_Value, sizeof(u8_Value), 1 );
        app_SrvFixedCarrierSet(u8_Value);
    }
}

//  ========== keypb_SYSRestart ===========
/*!
\brief         Reboot CMBS Target
\param[in,ou]  <none>
\return

*/
void  keypb_SYSRestart(void)
{
    app_SrvSystemReboot();
}

//  ========== keypb_SYSFWVersionGet ===========
/*!
\brief         Get current CMBS Target version
\param[in,ou]  <none>
\return

*/
void  keypb_SYSFWVersionGet(void)
{
    ST_APPCMBS_CONTAINER st_Container;

    u16 u16_FwModule = 0;
    u16 u16_FwDect = 0;
    u16 u16_FwDSP = 0;
    u16 u16_FwEEPROM = 0;
    u16 u16_FwUSB = 0;
    u16 u16_FwBuild = 0;
    u16 u16_FwBooter = 0;

    u8 parserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    app_SrvFWVersionGet(CMBS_MODULE_CMBS, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwModule = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messages

    app_SrvFWVersionGet(CMBS_MODULE_DECT, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwDect = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messages

    app_SrvFWVersionGet(CMBS_MODULE_DSP, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwDSP = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messages

    app_SrvFWVersionGet(CMBS_MODULE_EEPROM, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwEEPROM = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messages

    app_SrvFWVersionGet(CMBS_MODULE_USB, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwUSB = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messages

    app_SrvFWVersionGet(CMBS_MODULE_BUILD, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwBuild = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messagesV

    app_SrvFWVersionGet(CMBS_MODULE_BOOTER, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
    u16_FwBooter = ((PST_IE_FW_VERSION)st_Container.ch_Info)->u16_FwVersion;
    SleepMs(10);    // skip logging messages

    printf("\n\n**\n** FIRMWARE Version:\n");
    printf("** CMBS  : %X\n", u16_FwModule);
    printf("** DECT  : %X\n", u16_FwDect);
    printf("** DSP   : %X\n", u16_FwDSP);
    printf("** EEPROM: %X\n", u16_FwEEPROM);
    printf("** USB   : %X\n", u16_FwUSB);
    printf("** Build : %X\n", u16_FwBuild);
    printf("** Booter: %X\n", u16_FwBooter);

    app_set_msgparserEnabled(parserEnabled);
}

//  ========== keypb_SYSEEPROMVersionGet ===========
/*!
\brief         Get current CMBS Target EEPROM version
\param[in,ou]  <none>
\return

*/

void keypb_SYSEEPROMVersionGet(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    PST_IE_EEPROM_VERSION  p_IE;

    u8 parserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    app_SrvEEPROMVersionGet(TRUE);

    appcmbs_WaitForContainer(CMBS_EV_DSR_EEPROM_VERSION_GET_RES, &st_Container);

    p_IE = (PST_IE_EEPROM_VERSION)st_Container.ch_Info;

    SleepMs(10);    // skip logging messages

    printf("\n\n**\n** EEPROM Version:\n** ");
    printf("VER_%04x", p_IE->u32_EEPROM_Version);
    printf("\n**\n\n");

    app_set_msgparserEnabled(parserEnabled);

}


//  ========== keypb_SYSHWVersionGet ===========
/*!
\brief         Get current CMBS Target hardware version
\param[in,ou]  <none>
\return

*/
void  keypb_SYSHWVersionGet(void)
{

    app_SrvHWVersionGet(TRUE);

    printf("\n\nPress Any Key!\n");
    tcx_getch();
}

//  ========== keypb_DectSettingsGet ===========
/*!
\brief         Get DECT settings from target
\param[in,ou]  <none>
\return

*/
void  keypb_DectSettingsGet(void)
{
    ST_APPCMBS_CONTAINER  st_Container;
    PST_IE_DECT_SETTINGS_LIST st_DectSettings;
    char str[128] = { 0 };
    int i = 0;

    app_SrvDectSettingsGet(TRUE);

    appcmbs_WaitForContainer(CMBS_EV_DSR_DECT_SETTINGS_LIST_GET_RES, &st_Container);

    st_DectSettings = (PST_IE_DECT_SETTINGS_LIST)st_Container.ch_Info;

    SleepMs(10);    // skip logging messages
    printf("\n--- DECT SETTINGS: ---\n");
    printf("ClockMaster: %d\n", st_DectSettings->u16_ClockMaster);
    printf("EmissionMode: %d\n", st_DectSettings->u16_EmissionMode);
    printf("IP address type: %s\n", st_DectSettings->u8_IPAddressType ? "Static" : "DHCP");
    printf("IP address: %d.%d.%d.%d\n", st_DectSettings->pu8_IPAddressValue[0], st_DectSettings->pu8_IPAddressValue[1], st_DectSettings->pu8_IPAddressValue[2], st_DectSettings->pu8_IPAddressValue[3]);
    printf("Subnet mask: %d.%d.%d.%d\n", st_DectSettings->pu8_IPAddressSubnetMask[0], st_DectSettings->pu8_IPAddressSubnetMask[1], st_DectSettings->pu8_IPAddressSubnetMask[2], st_DectSettings->pu8_IPAddressSubnetMask[3]);
    printf("Gateway: %d.%d.%d.%d\n", st_DectSettings->pu8_IPAddressGateway[0], st_DectSettings->pu8_IPAddressGateway[1], st_DectSettings->pu8_IPAddressGateway[2], st_DectSettings->pu8_IPAddressGateway[3]);
    printf("DNS Server: %d.%d.%d.%d\n", st_DectSettings->pu8_IPAddressDNSServer[0], st_DectSettings->pu8_IPAddressDNSServer[1], st_DectSettings->pu8_IPAddressDNSServer[2], st_DectSettings->pu8_IPAddressDNSServer[3]);

    strncpy(str, (const char *)st_DectSettings->pu8_FirmwareVersion, st_DectSettings->u8_FirmwareVersionLength);
    printf("FirmwareVersion: %s\n", str);
    memset(&str, 0, sizeof(str));

    strncpy(str, (const char *)st_DectSettings->pu8_EepromVersion, st_DectSettings->u8_EepromVersionLength);
    printf("EepromVersion: %s\n", str);
    memset(&str, 0, sizeof(str));

    strncpy(str, (const char *)st_DectSettings->pu8_HardwareVersion, st_DectSettings->u8_HardwareVersionLength);
    printf("HardwareVersion: %s\n", str);

    printf("Pin Code: ");
    for (i = 0; i < CMBS_MAX_PIN_CODE_LENGTH; i++)
    {
        if ((st_DectSettings->pu8_PinCode[i] & 0xF0) != 0xF0)
            printf("%d", st_DectSettings->pu8_PinCode[i] >> 4);
        if ((st_DectSettings->pu8_PinCode[i] & 0xF) != 0xF)
            printf("%d", st_DectSettings->pu8_PinCode[i] & 0xF);
    }
    printf("\n");

    tcx_getch();
}


//  ========== keypb_DectSettingsSet ===========
/*!
\brief         Set DECT settings to target
\param[in,ou]  <none>
\return

*/
void  keypb_DectSettingsSet(void)
{
    ST_APPCMBS_CONTAINER  st_Container;
    ST_IE_DECT_SETTINGS_LIST st_DectSettings;
    PST_IE_RESPONSE    st_Response;

    memset(&st_DectSettings, 0, sizeof(st_DectSettings));
    st_DectSettings.u16_FieldsMask = 0;

    // fill with dummy data
    /*
    st_DectSettings.u16_BaseReset = 0x1;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_BASE_RESET;
    */

    st_DectSettings.u16_ClockMaster = 0x1;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_CLCK_MASTER;

    st_DectSettings.u16_EmissionMode = 0x0;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_EMISSION_MODE;

    st_DectSettings.u8_IPAddressType = 0x1;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_IP_ADDR_TYPE;

    st_DectSettings.pu8_IPAddressValue[0] = 192;
    st_DectSettings.pu8_IPAddressValue[1] = 168;
    st_DectSettings.pu8_IPAddressValue[2] = 2;
    st_DectSettings.pu8_IPAddressValue[3] = 5;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_IP_ADDR_VAL;

    st_DectSettings.pu8_IPAddressSubnetMask[0] = 255;
    st_DectSettings.pu8_IPAddressSubnetMask[1] = 255;
    st_DectSettings.pu8_IPAddressSubnetMask[2] = 255;
    st_DectSettings.pu8_IPAddressSubnetMask[3] = 0;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_IP_ADDR_SUBNET_MASK;

    st_DectSettings.pu8_IPAddressGateway[0] = 192;
    st_DectSettings.pu8_IPAddressGateway[1] = 168;
    st_DectSettings.pu8_IPAddressGateway[2] = 2;
    st_DectSettings.pu8_IPAddressGateway[3] = 1;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_IP_GATEWAY;

    st_DectSettings.pu8_IPAddressDNSServer[0] = 8;
    st_DectSettings.pu8_IPAddressDNSServer[1] = 8;
    st_DectSettings.pu8_IPAddressDNSServer[2] = 8;
    st_DectSettings.pu8_IPAddressDNSServer[3] = 8;
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_IP_DNS_SERVER;

    strcpy((char *)st_DectSettings.pu8_EepromVersion, "5588");
    st_DectSettings.u8_EepromVersionLength = strlen((const char *)st_DectSettings.pu8_EepromVersion);
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_EEPROM_VERSION;

    strcpy((char *)st_DectSettings.pu8_FirmwareVersion, "8855");
    st_DectSettings.u8_FirmwareVersionLength = strlen((const char *)st_DectSettings.pu8_FirmwareVersion);
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_FIRMWARE_VERSION;

    strcpy((char *)st_DectSettings.pu8_HardwareVersion, "AA55");
    st_DectSettings.u8_HardwareVersionLength = strlen((const char *)st_DectSettings.pu8_HardwareVersion);
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_HARDWARE_VERSION;

    /* New PIN code: 1212 */
    /*
    st_DectSettings.pu8_PinCode[0] = 0xFF; // mark as not used
    st_DectSettings.pu8_PinCode[1] = 0xFF; // mark as not used
    st_DectSettings.pu8_PinCode[2] = 0x12; // first two digits
    st_DectSettings.pu8_PinCode[3] = 0x12;  // last two digits
    st_DectSettings.u16_FieldsMask |= 1 << CMBS_DECT_SETTINGS_FIELD_PIN_CODE;
    */

    app_SrvDectSettingsSet(&st_DectSettings, 1);

    appcmbs_WaitForContainer(CMBS_EV_DSR_DECT_SETTINGS_LIST_SET_RES, &st_Container);

    st_Response = (PST_IE_RESPONSE)st_Container.ch_Info;
    printf("Dect Settings SET Result: %s\n", st_Response->e_Response == CMBS_RESPONSE_OK ? "OK" : "FAIL");

    tcx_getch();
}

//  ========== keypb_SysLogStart ===========
/*!
\brief         Get current content of log buffer
\param[in,ou]  <none>
\return

*/
void  keypb_SysLogStart(void)
{
    app_SrvLogBufferStart();

    printf("SysLog started\n");
}

//  ========== keypb_SysLogStop ===========
/*!
\brief         Get current content of log buffer
\param[in,ou]  <none>
\return

*/
void  keypb_SysLogStop(void)
{
    app_SrvLogBufferStop();

    printf("SysLog stopped\n");
}

//  ========== keypb_SysLogRead ===========
/*!
\brief         Get current content of log buffer
\param[in,ou]  <none>
\return

*/
void  keypb_SysLogRead(void)
{
    ST_APPCMBS_CONTAINER st_Container;

    app_SrvLogBufferRead(TRUE);

    appcmbs_WaitForContainer(CMBS_EV_DSR_SYS_LOG, &st_Container);

    printf("Press Any Key!\n");
    tcx_getch();
}

/* == ALTDV == */

//      ========== keyb_SYSPowerOff ===========
/*!
\brief         Power Off CMBS Target
\param[in,ou]  <none>
\return

*/
void  keypb_SYSPowerOff(void)
{
    app_SrvSystemPowerOff();
    printf("\n");
    printf("Power Off CMBS Target\n");
}

//      ========== keyb_RF_Control ===========
/*!
\brief         Suspend/Resume RF on CMBS Target
\param[in,ou]  <none>
\return

*/
void keypb_RF_Control(void)
{
    printf("\n");
    printf("1 => RF Suspend\n");
    printf("2 => RF Resume\n");
    printf("3 => Use fixed frequency (carrier) for Test purpose\n");

    switch (tcx_getch())
    {
        case '1':
            app_SrvRFSuspend();
            break;
        case '2':
            app_SrvRFResume();
            break;
        case '3':
            keypb_SYSFixedCarrierSet();
            break;
    }
}


//      ========== keypb_TurnOn_NEMo_mode ===========
/*!
\brief         Turn On/Off NEMo mode on CMBS Target
\param[in,ou]  <none>
\return

*/
void keypb_Turn_On_Off_NEMo_mode(void)
{
    printf("\n");
    printf("1 => Turn On NEMo mode\n");
    printf("2 => Turn Off NEMo mode\n");

    switch (tcx_getch())
    {
        case '1':
            app_SrvTurnOnNEMo();
            break;
        case '2':
            app_SrvTurnOffNEMo();
            break;
    }
}

//      ========== keyb_RegisteredHandsets ===========
/*!
\brief         Get List of subscribed handsets
\param[in,ou]  <none>
\return

*/
void  keypb_RegisteredHandsets(void)
{
    ST_APPCMBS_CONTAINER        st_Container;
    ST_IE_RESPONSE              st_Resp;

    memset(&st_Container, 0, sizeof(st_Container));

    app_SrvRegisteredHandsets(0xFFFF, 1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES, &st_Container);

    memcpy(&st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen);

    printf("\nSubscribed list get %s\n", st_Resp.e_Response == CMBS_RESPONSE_OK ? "Success" : "Fail");
    printf("Press Any Key!\n");
    tcx_getch();
}


//      ========== keyb_SetNewHandsetName ===========
/*!
\brief         Set List of subscribed handsets
\param[in,ou]  <none>
\return

*/
void  keyb_SetNewHandsetName(void)
{
    u16     u16_HsID = 0;
    u8      u8_HsName[CMBS_HS_NAME_MAX_LENGTH + 1];
    u8      u8_Index = 0;
    u16     u16_NameSize = 0;

    memset(u8_HsName, 0, sizeof(u8_HsName));

    printf("\nSet new HS name: ");
    printf("\nEnter HS/Extension number (1-12): ");
    tcx_scanf("%hu", &u16_HsID);
    if (u16_HsID < 1 || u16_HsID > 12)
    {
        printf("\n HS number not in range 1-12 ");
        return;
    }
    printf("\nEnter name (max 32 symbols): ");
    tcx_scanf("%s", u8_HsName);

    u16_NameSize = (u16)strlen((char *)u8_HsName);

    if (u16_NameSize > CMBS_HS_NAME_MAX_LENGTH)
    {
        printf("\n HS name too long ");
        return;
    }

    app_SrvSetNewHandsetName(u16_HsID, u8_HsName, u16_NameSize, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_SET_RES, NULL);

    // Display the list so the user can verify change is OK
    printf("\nList of registered handsets:\n");
    app_SrvRegisteredHandsets(0xFFFF, 1);
}

//  ========== keypb_AddNewExtension ===========
/*!
\brief         Add new extension to internal names list (FXS)
\param[in,ou]  <none>
\return

*/
void  keypb_AddNewExtension(void)
{
    u8 u8_NumberSize;
    u8 u8_Number[CMBS_FXS_NUMBER_MAX_LENGTH + 1];

    u16 u16_NameSize;
    u8 u8_Name[CMBS_HS_NAME_MAX_LENGTH + 1];

    memset(u8_Name, 0, sizeof(u8_Name));
    memset(u8_Number, 0, sizeof(u8_Number));


    printf("\nEnter Extension Number(max 4 symbols): \n");
    tcx_scanf("%s", u8_Number);

    u8_NumberSize = strlen((char *)u8_Number);
    if (u8_NumberSize < 1 || u8_NumberSize > 4)
    {
        printf("\n Wrong Extension Number length");
        return;
    }

    printf("\nEnter Extension Name(max 31 symbols): \n");
    tcx_scanf("%s", u8_Name);

    u16_NameSize = strlen((char *)u8_Name);

    if (u16_NameSize >= CMBS_HS_NAME_MAX_LENGTH)
    {
        printf("\n HS name too long ");
        return;
    }

    app_SrvAddNewExtension(u8_Name, u16_NameSize, u8_Number, u8_NumberSize, 1);
}

//      ========== keypb_SetBaseName ===========
/*!
\brief         Set Base name
\param[in,ou]  <none>
\return

*/
void keypb_SetBaseName(void)
{
    u8 u8_BaseNameSize;
    u8   u8_BaseName[CMBS_BS_NAME_MAX_LENGTH + 1];

    printf("\nEnter new base name (max 13 symbols): \n");
    tcx_scanf("%s", u8_BaseName);

    u8_BaseNameSize = strlen((char *)u8_BaseName);

    app_SrvSetBaseName(u8_BaseName, u8_BaseNameSize, 1);

}

void keypb_GetBaseName(void)
{
    ST_APPCMBS_CONTAINER        st_Container;
    ST_IE_BASE_NAME            st_BaseName;

    app_SrvGetBaseName(1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_GET_BASE_NAME_RES, &st_Container);

    memcpy(&st_BaseName.u8_BaseName, &st_Container.ch_Info, st_Container.n_InfoLen);

    printf("\nBase Name %s\n", st_BaseName.u8_BaseName);
    printf("Press Any Key!\n");
    tcx_getch();
}

/*=== keyb_CommStress  =======================================================*/
/*                                                                            */
/* FUNCTIONAL DESCRIPTION:                                                    */
/*                                                                            */
/* This function is called when a stress test is required for the communication
 *  channel between host and target.
 *
 * PARAMETERS
 *
 *                                                                            */
/* INTERFACE DECLARATION:                                                     */
void keyb_CommStress(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    u8 pu8_SubsDataRx[CMBS_PARAM_SUBS_DATA_LENGTH], pu8_SubsDataTx[CMBS_PARAM_SUBS_DATA_LENGTH], pu8_Buffer[8];
    u8 i;
    u32 u32_Counter, u32_Errors = 0;
    u8 u8_TestSelect;
    u8 szPinCode[CMBS_PARAM_PIN_CODE_LENGTH] = { 0xFF, 0xFF, 0, 0 };
    u8 szPinCodeRx[CMBS_PARAM_PIN_CODE_LENGTH];

    // save parser state as we want to disable it temporary
    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    // Clear screen
    tcx_appClearScreen();

    printf("This test performs stress test for the communication channel with 2 options:\n");
    printf(" s - by setting and getting PP Subscription data (200 bytes).\n");
    printf(" a - by getting PP Subscription data (200 bytes) and setting the AUTH PIN\n");
    printf(" Select test to perform: (s or a): ");
    u8_TestSelect = tcx_getch();

    printf("\n Enter number of iterations to perform: max 1000");
    tcx_gets((char *)pu8_Buffer, sizeof(pu8_Buffer));
    u32_Counter = (u32)atoi((const char *)pu8_Buffer);
    u32_Counter = MIN(MAX_OVERNIGHT_TESTS, u32_Counter);
    // get Subscription data
    app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);

    // copy to internal buffers
    memcpy(pu8_SubsDataRx, st_Container.ch_Info, st_Container.n_InfoLen);
    memcpy(pu8_SubsDataTx, st_Container.ch_Info, st_Container.n_InfoLen);

    if (u8_TestSelect == 's')
    {

        for (; u32_Counter > 0; --u32_Counter)
        {
            // Set Subscription data
            app_SrvParamSet(CMBS_PARAM_SUBS_DATA, pu8_SubsDataTx, CMBS_PARAM_SUBS_DATA_LENGTH, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

            // Get Subscription data
            app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(pu8_SubsDataRx, st_Container.ch_Info, st_Container.n_InfoLen);

            // compare
            if (memcmp(pu8_SubsDataRx, pu8_SubsDataTx, sizeof(pu8_SubsDataRx)))
            {
                printf("\nCommunication error!\n");
                ++u32_Errors;
            }
        }
    }
    else
    {
        /* use FFFFxxxx form (Expected by app_SrvPINCodeSet)
        for ( i = 0; i < 4; ++i )
        {
            PINCode[i] = 'f';
            PINCode[i + 4] = '0';
        }
        PINCode[8] = 0;
        PINCodeRx[8] = 0; */

        // AUTH_PIN stresstest combined with read of subcsriptiondata
        for (; u32_Counter > 0; --u32_Counter)
        {
            // Get Subscription data and compare it with initial data (first read)
            app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(pu8_SubsDataRx, st_Container.ch_Info, st_Container.n_InfoLen);

            // compare
            if (memcmp(pu8_SubsDataRx, pu8_SubsDataTx, sizeof(pu8_SubsDataRx)))
            {
                printf("\nCommunication error Subsdata!\n");
                ++u32_Errors;
            }
            // Set AUTH PIN data
            // app_SrvParamSet( CMBS_PARAM_AUTH_PIN, , 1 );
            // ASA: I don't understand why appcmbs_WaitForContainer doesn't work in that case !!!!
            // app_SrvPINCodeSet( PINCode );

            // do it different:
            app_SrvParamSet(CMBS_PARAM_AUTH_PIN, szPinCode, CMBS_PARAM_PIN_CODE_LENGTH, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);


            app_SrvParamGet(CMBS_PARAM_AUTH_PIN, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
            memcpy(szPinCodeRx, st_Container.ch_Info, 4);
            printf(" Rec. PIN:\n");
            for (i = 0; i < 4; i++)
            {
                printf(" %x", szPinCodeRx[i]);
                if (szPinCodeRx[i] != szPinCode[i])
                {
                    ++u32_Errors;
                    printf(" AUTH_PIN ERROR !!! \n");
                }

            }
            printf("\n");
        }
    }

    // restore parser state
    app_set_msgparserEnabled(msgparserEnabled);

    printf("\nAll done with %d errors.\n", u32_Errors);

    tcx_getch();
}

//  ========== keypb_SubsHSTest ===========
/*!
\brief         Get subscribed HS Data, perform EEPROM reset, restore subscribed HS Data
\param[in,out]  <none>
\return void
*/
void keyb_SubsHSTest(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    u8 pu8_SubsData[CMBS_PARAM_SUBS_DATA_LENGTH];
    u8 pu8_RFPI[CMBS_PARAM_RFPI_LENGTH] = { 0 };
    u8 pu8_ULENextTPUI[CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH] = { 0 };
    u8 pu8_HAN_DECT_DATA[HAN_DECT_SUBS_LENGTH] = { 0 };
    u8 pu8_HAN_ULE_DATA[HAN_ULE_SUBS_LENGTH] = { 0 };
    u8 pu8_HAN_FUN_DATA[HAN_FUN_SUBS_LENGTH] = { 0 };
    u16 u16_ULE_dect_start, u16_ULE_dect_end, u16_ULE_dect_size;
    u16 u16_ULE_start, u16_ULE_end, u16_ULE_size;
    u16 u16_FUN_start, u16_FUN_end, u16_FUN_size;
    s8 s8_i;

    // save parser state as we want to disable it temporary
    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    // Clear screen
    tcx_appClearScreen();

    // get current RFPI
    printf("Getting Current RFPI...\n");
    app_SrvParamGet(CMBS_PARAM_RFPI, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&pu8_RFPI[0], st_Container.ch_Info,  CMBS_PARAM_RFPI_LENGTH);


    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");

    printf("Current RFPI: \t %.2X%.2X%.2X%.2X%.2X\n", pu8_RFPI[0], pu8_RFPI[1], pu8_RFPI[2], pu8_RFPI[3], pu8_RFPI[4]);

    SleepMs(3000);

    // Get current subscribe data
    printf("\nGetting SUBS Data... \n");

    app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);

    // copy to internal buffer
    memcpy(pu8_SubsData, st_Container.ch_Info, st_Container.n_InfoLen);

    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.");

    // Handle ULE Subscription restore

    // Get ULE DECT parameters
    printf("Getting ULE DECT DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_DECT_SUB_DB_START, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_dect_start, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DECT DB start address: \t 0x%X\n", u16_ULE_dect_start);

    // Get ULE DECT parameters
    printf("Getting ULE DECT DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_DECT_SUB_DB_END, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_dect_end, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DECT DB end address: \t 0x%X\n", u16_ULE_dect_end);

    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.");
    printf("Getting ULE DECT DB address...done\n");

    //Calculate the needed size
    u16_ULE_dect_size = u16_ULE_dect_end - u16_ULE_dect_start;

    printf("ULE DECT DB size: \t 0x%X\n", u16_ULE_dect_size);

    //Get data from Target
    if (u16_ULE_dect_size < HAN_DECT_SUBS_LENGTH)
    {
        keyb_ParamAreaGetBySegments(u16_ULE_dect_start, u16_ULE_dect_size, pu8_HAN_DECT_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Error in DECT DB size - received 0x%X\n", u16_ULE_dect_size);
        u16_ULE_dect_size = 0;
    }


    // Get ULE  parameters
    printf("Getting ULE DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_ULE_SUB_DB_START, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_start, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DB start address: \t 0x%X\n", u16_ULE_start);


    // Get ULE parameters
    printf("Getting ULE  DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_ULE_SUB_DB_END, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_end, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DB end address: \t 0x%X\n", u16_ULE_end);

    //Calculate the needed size
    u16_ULE_size = u16_ULE_end - u16_ULE_start;

    printf("ULE DB size: \t 0x%X\n", u16_ULE_size);

    //Get data from Target
    if (u16_ULE_size < HAN_ULE_SUBS_LENGTH)
    {
        keyb_ParamAreaGetBySegments(u16_ULE_start, u16_ULE_size, pu8_HAN_ULE_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Error in ULE  DB size - received 0x%X\n", u16_ULE_size);
        u16_ULE_size = 0;
    }


    // Get FUN parameters
    printf("Getting FUN DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_FUN_SUB_DB_START, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_FUN_start, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE FUN DB start address: \t 0x%X\n", u16_FUN_start);


    // Get FUN parameters
    printf("Getting FUN  DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_FUN_SUB_DB_END, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_FUN_end, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE FUN DB end address: \t 0x%X\n", u16_FUN_end);

    //Calculate the needed size
    u16_FUN_size = u16_FUN_end - u16_FUN_start;

    printf("HAN FUN DB size: \t 0x%X\n", u16_FUN_size);

    //Get data from Target
    if (u16_FUN_size < HAN_FUN_SUBS_LENGTH)
    {
        keyb_ParamAreaGetBySegments(u16_FUN_start, u16_FUN_size, pu8_HAN_FUN_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Error in FUN  DB size - received 0x%X\n", u16_FUN_size);
        u16_FUN_size = 0;
    }

    //Get next ULE TPUI
    printf("Getting Next TPUI...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_ULE_NEXT_TPUI, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(pu8_ULENextTPUI, st_Container.ch_Info,  CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH);

    // perform EEPROM reset
    printf("\nPerforming EEPROM reset... \n");

    keyb_EEPROMReset();

    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.\n");
    printf("Getting ULE HAN DB address...done\n");
    printf("Performing EEPROM reset... done.\n");

    // restore SUBS Data
    printf("Restoring SUBS Data... \n");

    app_SrvParamSet(CMBS_PARAM_SUBS_DATA, pu8_SubsData, CMBS_PARAM_SUBS_DATA_LENGTH, 1);

    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.\n");
    printf("Getting ULE HAN DB...done\n");
    printf("Performing EEPROM reset... done.\n");
    printf("Restoring SUBS Data... done.\n");



    // restore RFPI
    printf("\nRestoring RFPI... \n");
    app_SrvParamSet(CMBS_PARAM_RFPI, pu8_RFPI, sizeof(pu8_RFPI), 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);


    // restore HAN Data
    printf("Restoring DECT Data... \n");

    // Check the DECT DB size
    if (u16_ULE_dect_size)
    {
        keyb_ParamAreaSetBySegments(u16_ULE_dect_start, u16_ULE_dect_size, pu8_HAN_DECT_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Fail !");
    }


    // restore ULE Data
    printf("Restoring ULE Data... \n");
    if (u16_ULE_size)
    {
        keyb_ParamAreaSetBySegments(u16_ULE_start, u16_ULE_size, pu8_HAN_ULE_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Fail !");
    }

    // restore FUN Data
    printf("Restoring FUN Data... \n");
    if (u16_FUN_size)
    {
        keyb_ParamAreaSetBySegments(u16_FUN_start, u16_FUN_size, pu8_HAN_FUN_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Fail !");
    }
    // restore ULE next TPUI
    printf("\nRestoring ULE Next TPUI... \n");
    app_SrvParamSet(CMBS_PARAM_HAN_ULE_NEXT_TPUI, pu8_ULENextTPUI, sizeof(pu8_ULENextTPUI), 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);


    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.\n");
    printf("Getting ULE HAN DB address...done\n");
    printf("Performing EEPROM reset... done.\n");
    printf("Restoring SUBS Data... done.\n");
    printf("Restoring ULE Subscription Data... done.\n");
    printf("Restoring RFPI... done.\n");
    printf("Restoring ULE HAN DB ...done.\n");

    for (s8_i = 20; s8_i >= 0; --s8_i)
    {
        printf("\rWaiting for target to finish writing to EEPROM... %02d", s8_i);
        SleepMs(1000);
    }

    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.\n");
    printf("Performing EEPROM reset... done.\n");
    printf("Restoring SUBS Data... done.\n");
    printf("Restoring RFPI... done.\n");
    printf("Restoring ULE Subscription Data... done.\n");

    // system restart
    printf("Restarting system... \n");
    app_SrvSystemReboot();

    tcx_appClearScreen();
    printf("Getting Current RFPI... done.\n");
    printf("Getting SUBS Data... done.\n");
    printf("Performing EEPROM reset... done.\n");
    printf("Restoring SUBS Data... done.\n");
    printf("Restoring RFPI... done.\n");
    printf("Restoring ULE Subscription Data... done.\n");
    printf("Restarting system... done.\n");

    printf("All done! press any key...");

    // restore parser state
    app_set_msgparserEnabled(msgparserEnabled);

    tcx_getch();
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

//This is utility function for copying big buffers to the EEPROM in the target.
//This function is doing the fragmentation for packets of packet_max_size.
//This function is blocking and waiting for the packet acknoledge.

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void keyb_ParamAreaSetBySegments(u32 u32_Pos, u16 u16_Length, u8 *pu8_Data, u16 packet_max_size)
{

    u8 i, u8_number_of_packet = 0;

    ST_APPCMBS_CONTAINER st_Container;
    u16 u16_offset, u16_remainder = 0;

    if (u16_Length < packet_max_size)
    {
        printf("Setting Data ...");
        app_SrvParamAreaSet(CMBS_PARAM_AREA_TYPE_EEPROM,  u32_Pos,  u16_Length, pu8_Data, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_SET_RES, &st_Container);
    }
    else
    {
        u8_number_of_packet = (u16_Length / packet_max_size);
        u16_remainder = (u16_Length % packet_max_size);
        u16_offset = 0;
        for (i = 0; i < u8_number_of_packet; i++)
        {
            printf("Setting Data Packet %d... \n", i + 1);
            app_SrvParamAreaSet(CMBS_PARAM_AREA_TYPE_EEPROM,  u32_Pos + u16_offset,  packet_max_size, pu8_Data + u16_offset, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_SET_RES, &st_Container);
            u16_offset += packet_max_size;
        }
        if (u16_remainder)
        {
            printf("Setting Data Packet %d... \n", i + 1);
            app_SrvParamAreaSet(CMBS_PARAM_AREA_TYPE_EEPROM,  u32_Pos + u16_offset,  u16_remainder, pu8_Data + u16_offset, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_SET_RES, &st_Container);
            u16_offset += u16_remainder;
        }
    }

    printf("Done!");


}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

//This is utility function for getting big buffers from the EEPROM in the target.
//This function is doing the fragmentation for packets of packet_max_size.
//This function is blocking and waiting for the packet.

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void keyb_ParamAreaGetBySegments(u32 u32_Pos, u16 u16_Length, u8 *pu8_Data, u16 packet_max_size)
{
    u8 i, u8_number_of_packet = 0;

    ST_APPCMBS_CONTAINER st_Container;
    u16 u16_offset, u16_remainder = 0;

    if (u16_Length < packet_max_size)
    {
        printf("Getting Data ...");
        app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM,  u32_Pos,  u16_Length, 1);
        appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container);
        memcpy(pu8_Data, st_Container.ch_Info,  u16_Length);
    }
    else
    {
        u8_number_of_packet = (u16_Length / packet_max_size);
        u16_remainder = (u16_Length % packet_max_size);
        u16_offset = 0;

        for (i = 0; i < u8_number_of_packet; i++)
        {
            printf("Getting Data Packet %d... \n", i + 1);
            app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM,  u32_Pos + u16_offset,  packet_max_size, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container);
            memcpy(pu8_Data + u16_offset, st_Container.ch_Info,  packet_max_size);
            u16_offset += packet_max_size;
        }
        if (u16_remainder)
        {
            printf("Getting Data Packet %d... \n", i + 1);
            app_SrvParamAreaGet(CMBS_PARAM_AREA_TYPE_EEPROM,  u32_Pos + u16_offset,  u16_remainder, 1);
            appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_AREA_GET_RES, &st_Container);
            memcpy(pu8_Data + u16_offset, st_Container.ch_Info,  u16_remainder);
            u16_offset += u16_remainder;
        }
    }

    printf("Done!");


}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

//    Writing a binary file content into a buffer
//    Parameters: a buffer to write into, a file path
//    Assumption: file size <= buffer size
//    Return value: 1 on success, 0 on faliure

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
E_CMBS_RC keyb_ParamAreaGetFromFile(u8 *pu8_Data, u8 *pu8_path)
{

    FILE *fp_binFile = 0;
    u32 u32_status = 0;
    u32 u32_size = 0;
    E_CMBS_RC e_status;

    // Open binary file for read
    e_status = tcx_fileOpen(&fp_binFile, pu8_path, "r+b");

    if (e_status != CMBS_RC_OK)
    {
        printf("Failed to open file - %s\n", pu8_path);
        return CMBS_RC_ERROR_GENERAL;
    }

    // get the size of the file in bytes
    u32_size = tcx_fileSize(fp_binFile);

    if ((s32)u32_size < 0)
    {
        printf("Failed to get file size - %s\n", pu8_path);
        tcx_fileClose(fp_binFile);
        return CMBS_RC_ERROR_GENERAL;
    }

    // Read u32_size bytes from binFile
    e_status = tcx_fileRead(fp_binFile, pu8_Data, 0, u32_size);

    if (e_status != CMBS_RC_OK)
    {
        printf("Failed to read from file into EEprom buffer\n");
        tcx_fileClose(fp_binFile);
        return CMBS_RC_ERROR_GENERAL;
    }

    // Close file
    tcx_fileClose(fp_binFile);

    return CMBS_RC_OK;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

//This is utility function for getting input line from the user

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
static  E_CMBS_RetVal getLine(u8 *pu8_prmpt, u8 *pu8_buff, u32 pu8_sz)
{

    u32 u32_ch, u32_extra;

    // Get line with buffer overrun protection
    if (pu8_prmpt != NULL)
    {
        printf("%s", pu8_prmpt);
        fflush(stdout);
    }

    if (fgets((char *)pu8_buff, pu8_sz, stdin) == NULL)
        return NO_INPUT;

    // If it was too long, there'll be no newline. In that case, we flush
    // to end of line so that excess doesn't affect the next call.
    if (pu8_buff[strlen((char *)pu8_buff) - 1] != '\n')
    {
        u32_extra = 0;
        while (((u32_ch = getchar()) != '\n') && (u32_ch != (u32)EOF)) u32_extra = 1;
        return (u32_extra == 1) ? TOO_LONG : OK;
    }

    // Otherwise remove newline and give string back to caller.
    pu8_buff[strlen((char *)pu8_buff) - 1] = '\0';
    return OK;
}


//  ========== keyb_ULEDevicesAutoSubscription ===========
/*!
\brief         Get subscribed Data from binary files, updates subscribed Data from files
\param[in,out]  <none>
\return void
*/
void keyb_ULEDevicesAutoSubscription(void)
{
    E_CMBS_RC e_status;
    E_CMBS_RetVal e_statusLine;
    ST_APPCMBS_CONTAINER st_Container;
    u8 pu8_RFPI[CMBS_PARAM_RFPI_LENGTH] = { 0 };
    u8 pu8_ULENextTPUI[CMBS_PARAM_HAN_ULE_NEXT_TPUI_LENGTH] = { 0 };
    u8 pu8_HAN_DECT_DATA[HAN_DECT_SUBS_LENGTH] = { 0 };
    u8 pu8_HAN_ULE_DATA[HAN_ULE_SUBS_LENGTH] = { 0 };
    u8 pu8_HAN_FUN_DATA[HAN_FUN_SUBS_LENGTH] = { 0 };
    u16 u16_ULE_dect_start, u16_ULE_dect_end, u16_ULE_dect_size;
    u16 u16_ULE_start, u16_ULE_end, u16_ULE_size;
    u16 u16_FUN_start, u16_FUN_end, u16_FUN_size;
    u8 pu8_path_RFPI[MAX_PATH_SIZE], pu8_path_TPUI[MAX_PATH_SIZE], pu8_path_DECT[MAX_PATH_SIZE],
    pu8_path_ULE[MAX_PATH_SIZE], pu8_path_FUN[MAX_PATH_SIZE];
    u8 pu8_buff[MAX_PATH_SIZE];
    u32 u32_fileStatus;
    FILE *f_binFile = 0;

    // Binary file names
    const u8 pu8_filename_RFPI[] = "RFPI.bin"; //RFPI file name
    const u8 pu8_filename_DECT[] = "DECT.bin"; //DECT file name
    const u8 pu8_filename_ULE[] = "ULE.bin"; //ULE file name
    const u8 pu8_filename_FUN[] = "FUN.bin"; //FUN file name
    const u8 pu8_filename_TPUI[] = "Next_TPUI.bin"; //Next TPUI file name

    // Clear screen
    tcx_appClearScreen();

    // Get directory from the user
    e_statusLine = getLine("Enter a directory path for the location of the binary files > ", pu8_buff, sizeof(pu8_buff));

    switch (e_statusLine)
    {
        case NO_INPUT:
            printf("\nError: path is empty!\nExiting...");
            tcx_getch();
            return;
        case TOO_LONG:
            printf("\nError: path is too long!\nExiting...");
            tcx_getch();
            return;
        default:
            break;
    }

    // Get RFPI.bin full path
    pu8_path_RFPI[0] = '\0';
    strncat((char *)pu8_path_RFPI, (const char *)pu8_buff, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_RFPI) + 1);
    strncat((char *)pu8_path_RFPI, (const char *)pu8_filename_RFPI, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_RFPI) + 1);

    // Get DECT.bin full path
    pu8_path_DECT[0] = '\0';
    strncat((char *)pu8_path_DECT, (const char *)pu8_buff, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_DECT) + 1);
    strncat((char *)pu8_path_DECT, (const char *)pu8_filename_DECT, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_DECT) + 1);

    // Get ULE.bin full path
    pu8_path_ULE[0] = '\0';
    strncat((char *)pu8_path_ULE, (const char *)pu8_buff, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_ULE) + 1);
    strncat((char *)pu8_path_ULE, (const char *)pu8_filename_ULE, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_ULE) + 1);

    // Get FUN.bin full path
    pu8_path_FUN[0] = '\0';
    strncat((char *)pu8_path_FUN, (const char *)pu8_buff, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_FUN) + 1);
    strncat((char *)pu8_path_FUN, (const char *)pu8_filename_FUN, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_FUN) + 1);

    // Get Next_TPUI.bin full path
    pu8_path_TPUI[0] = '\0';
    strncat((char *)pu8_path_TPUI, (const char *)pu8_buff, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_TPUI) + 1);
    strncat((char *)pu8_path_TPUI, (const char *)pu8_filename_TPUI, strlen((char *)pu8_buff) + strlen((char *)pu8_filename_TPUI) + 1);

    printf("\nUsing Files:\n");
    printf("============\n%s\n%s\n%s\n%s\n%s\n", pu8_path_RFPI, pu8_path_DECT, pu8_path_ULE, pu8_path_FUN, pu8_path_TPUI);

    u32_fileStatus = 1;

    // Check binary files exist
    e_status = tcx_fileOpen(&f_binFile, pu8_path_RFPI, "r+b");
    if (e_status != CMBS_RC_OK)
    {
        u32_fileStatus = 0;
        printf("\nError: Can't Open File: %s", pu8_path_RFPI);
    }
    else
    {
        tcx_fileClose(f_binFile);
    }

    e_status = tcx_fileOpen(&f_binFile, pu8_path_DECT, "r+b");
    if (e_status != CMBS_RC_OK)
    {
        u32_fileStatus = 0;
        printf("\nError: Can't Open File: %s", pu8_path_DECT);
    }
    else
    {
        tcx_fileClose(f_binFile);
    }

    e_status = tcx_fileOpen(&f_binFile, pu8_path_ULE, "r+b");
    if (e_status != CMBS_RC_OK)
    {
        u32_fileStatus = 0;
        printf("\nError: Can't Open File: %s", pu8_path_ULE);
    }
    else
    {
        tcx_fileClose(f_binFile);
    }

    e_status = tcx_fileOpen(&f_binFile, pu8_path_FUN, "r+b");
    if (e_status != CMBS_RC_OK)
    {
        u32_fileStatus = 0;
        printf("\nError: Can't Open File: %s", pu8_path_FUN);
    }
    else
    {
        tcx_fileClose(f_binFile);
    }

    e_status = tcx_fileOpen(&f_binFile, pu8_path_TPUI, "r+b");
    if (e_status != CMBS_RC_OK)
    {
        u32_fileStatus = 0;
        printf("\nError: Can't Open File: %s", pu8_path_TPUI);
    }
    else
    {
        tcx_fileClose(f_binFile);
    }

    // Exit in case at least one file could not be opened
    if (!u32_fileStatus)
    {
        printf("\n\nAborting...");
        printf("\nPress any key to exit...");
        tcx_getch();
        printf("\n\n");
        return;
    }

    // Get current RFPI from file
    printf("\nGetting RFPI from file - %s...\n", pu8_path_RFPI);
    e_status = keyb_ParamAreaGetFromFile(pu8_RFPI, pu8_path_RFPI);
    if (e_status == CMBS_RC_ERROR_GENERAL)
    {
        printf("Failed to read RFPI data from file %s into buffer\nExiting...", pu8_path_RFPI);
        return;
    }

    // Handle ULE Auto Subscription

    // Get ULE DECT parameters
    printf("\n********************************************\n");
    printf("\nGetting ULE DECT DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_DECT_SUB_DB_START, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_dect_start, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DECT DB start address: \t 0x%X\n", u16_ULE_dect_start);

    // Get ULE DECT parameters
    printf("\nGetting ULE DECT DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_DECT_SUB_DB_END, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_dect_end, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DECT DB end address: \t 0x%X\n", u16_ULE_dect_end);

    // Calculate the needed size
    u16_ULE_dect_size = u16_ULE_dect_end - u16_ULE_dect_start;

    printf("ULE DECT DB size: \t 0x%X\n", u16_ULE_dect_size);
    printf("\n********************************************\n");

    printf("\nGetting ULE DECT block from file - %s...\n", pu8_path_DECT);


    // Get data from file
    if (u16_ULE_dect_size < HAN_DECT_SUBS_LENGTH)
    {
        e_status = keyb_ParamAreaGetFromFile(pu8_HAN_DECT_DATA, pu8_path_DECT);

        if (e_status == CMBS_RC_ERROR_GENERAL)
        {
            printf("Failed to read DECT data from file %s into buffer\n", pu8_path_DECT);
            u16_ULE_dect_size = 0;
            return;
        }
    }
    else
    {
        printf("Error in DECT DB size - received 0x%X\n", u16_ULE_dect_size);
        u16_ULE_dect_size = 0;
    }

    // Get ULE  parameters
    printf("\nGetting ULE DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_ULE_SUB_DB_START, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_start, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DB start address: \t 0x%X\n", u16_ULE_start);

    // Get ULE parameters
    printf("Getting ULE  DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_ULE_SUB_DB_END, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_ULE_end, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE DB end address: \t 0x%X\n", u16_ULE_end);

    //Calculate the needed size
    u16_ULE_size = u16_ULE_end - u16_ULE_start;

    printf("ULE DB size: \t 0x%X\n", u16_ULE_size);
    printf("\n********************************************\n");

    printf("\nGetting ULE block from file - %s...\n", pu8_path_ULE);

    // Get data from file
    if (u16_ULE_size < HAN_ULE_SUBS_LENGTH)
    {
        e_status = keyb_ParamAreaGetFromFile(pu8_HAN_ULE_DATA, pu8_path_ULE);
        if (e_status == CMBS_RC_ERROR_GENERAL)
        {
            printf("Failed to read ULE data from file %s into buffer\nExiting...", pu8_path_ULE);
            u16_ULE_size = 0;
            return;
        }
    }
    else
    {
        printf("Error in ULE  DB size - received 0x%X\n", u16_ULE_size);
        u16_ULE_size = 0;
    }

    // Get FUN parameters
    printf("Getting FUN DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_FUN_SUB_DB_START, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_FUN_start, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE FUN DB start address: \t 0x%X\n", u16_FUN_start);

    // Get FUN parameters
    printf("Getting FUN  DB address...\n");
    app_SrvParamGet(CMBS_PARAM_HAN_FUN_SUB_DB_END, 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);
    memcpy(&u16_FUN_end, st_Container.ch_Info,  CMBS_PARAM_HAN_DB_ADDR_LENGTH);
    printf("ULE FUN DB end address: \t 0x%X\n", u16_FUN_end);

    //Calculate the needed size
    u16_FUN_size = u16_FUN_end - u16_FUN_start;

    printf("HAN FUN DB size: \t 0x%X\n", u16_FUN_size);
    printf("\n********************************************\n");

    printf("\nGetting FUN block from file - %s...\n", pu8_path_FUN);

    // Get data from file
    if (u16_FUN_size < HAN_FUN_SUBS_LENGTH)
    {
        e_status = keyb_ParamAreaGetFromFile(pu8_HAN_FUN_DATA, pu8_path_FUN);
        if (e_status == CMBS_RC_ERROR_GENERAL)
        {
            printf("Failed to read FUN data from file %s into buffer\nExiting...", pu8_path_FUN);
            u16_FUN_size = 0;
            return;
        }
    }
    else
    {
        printf("Error in FUN DB size - received 0x%X\n", u16_FUN_size);
        u16_FUN_size = 0;
    }



    // Get next ULE TPUI from file
    printf("Getting Next TPUI from file - %s...\n", pu8_path_TPUI);
    e_status = keyb_ParamAreaGetFromFile(pu8_ULENextTPUI, pu8_path_TPUI);
    if (e_status == CMBS_RC_ERROR_GENERAL)
    {
        printf("Failed to read ULE Next TPUI from file %s into buffer\nExiting...", pu8_path_TPUI);
        return;
    }

    // Setting RFPI
    printf("Setting RFPI... \n");
    app_SrvParamSet(CMBS_PARAM_RFPI, pu8_RFPI, sizeof(pu8_RFPI), 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    // Setting HAN Data
    printf("Setting DECT Data... \n");

    // Check the DECT DB size
    if (u16_ULE_dect_size)
    {
        keyb_ParamAreaSetBySegments(u16_ULE_dect_start, u16_ULE_dect_size, pu8_HAN_DECT_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Fail !");
    }


    // Setting ULE Data
    printf("Setting ULE Data... \n");
    if (u16_ULE_size)
    {
        keyb_ParamAreaSetBySegments(u16_ULE_start, u16_ULE_size, pu8_HAN_ULE_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Fail !");
    }

    // Setting FUN Data
    printf("Setting FUN Data... \n");
    if (u16_FUN_size)
    {
        keyb_ParamAreaSetBySegments(u16_FUN_start, u16_FUN_size, pu8_HAN_FUN_DATA, CMBS_PARAM_AREA_MAX_SIZE);
    }
    else
    {
        printf("Fail !");
    }
    // Setting ULE next TPUI
    printf("Setting ULE Next TPUI... \n");
    app_SrvParamSet(CMBS_PARAM_HAN_ULE_NEXT_TPUI, pu8_ULENextTPUI, sizeof(pu8_ULENextTPUI), 1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_SET_RES, &st_Container);

    printf("\ndone!\n\n");

    // System restart
    printf("\n\nRestarting system... ");
    app_SrvSystemReboot();

    printf("done.\n\n");

    tcx_appClearScreen();
    printf("All done! press any key to exit...");

    tcx_getch();
}


void keyb_DisplaySubscriptionData(void)
{
    ST_APPCMBS_CONTAINER st_Container;
    ST_CMBS_SUBS_DATA Subs_Data[CMBS_HS_SUBSCRIBED_MAX_NUM];
    u8 u8_j;
    u8 u8_i;

    // save parser state as we want to disable it temporary
    u8 msgparserEnabled = app_get_msgparserEnabled();
    app_set_msgparserEnabled(0);

    // Get current subscribe data
    printf("\nGetting SUBS Data... \n");

    app_SrvParamGet(CMBS_PARAM_SUBS_DATA, 1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_PARAM_GET_RES, &st_Container);

    // copy to internal buffer
    memcpy(Subs_Data, st_Container.ch_Info, st_Container.n_InfoLen);

    // Clear screen
    tcx_appClearScreen();

    // go over the buffer and parse data
    for (u8_j = 0; u8_j < CMBS_HS_SUBSCRIBED_MAX_NUM; ++u8_j)
    {
        if (Subs_Data[u8_j].u8_SUB_STATUS == 1)   // if the hadnset is subscribed
        {
            printf("\nSubscription Data for HS number %d\n", (Subs_Data[u8_j].u8_HANDSET_NR) & 0x0F);
            printf("\nIPUI: ");
            for (u8_i = 0; u8_i < CMBS_IPUI_LEN; ++u8_i)
            {
                printf("%02X ", Subs_Data[u8_j].pu8_IPUI[u8_i]);
            }

            printf("\nTPUI: ");
            for (u8_i = 0; u8_i < CMBS_TPUI_LEN; ++u8_i)
            {
                printf("%02X ", Subs_Data[u8_j].pu8_TPUI[u8_i]);
            }

            printf("\nDCK: ");
            for (u8_i = 0; u8_i < CMBS_DCK_LEN; ++u8_i)
            {
                printf("%02X ", Subs_Data[u8_j].pu8_DCK[u8_i]);
            }

            printf("\nUAK: ");
            for (u8_i = 0; u8_i < CMBS_UAK_LEN; ++u8_i)
            {
                printf("%02X ", Subs_Data[u8_j].pu8_UAK[u8_i]);
            }

            printf("\nAuthentication code ");
            for (u8_i = 0; u8_i < CMBS_AC_LEN; ++u8_i)
            {
                printf("%02X ", Subs_Data[u8_j].pu8_AC[u8_i]);
            }

            printf("\n\n UAK authorization: %02X", Subs_Data[u8_j].u8_UAK_AUTH);
            printf("\n Status: %02X", Subs_Data[u8_j].u8_SUB_STATUS);
            printf("\n HS number: %02X", (Subs_Data[u8_j].u8_HANDSET_NR) & 0x0F);
            printf("\n DCK assigned: %02X", Subs_Data[u8_j].u8_DCK_ASSIGNED);
            printf("\n Cipher Key Length: %02X", Subs_Data[u8_j].u8_CK_LEN);
            printf("\n Handset ULE feature support: %02X\n", Subs_Data[u8_j].u8_FEATURES);
            printf("\n---------------------\n");
        }
    }

    tcx_getch();
    // restore parser state
    app_set_msgparserEnabled(msgparserEnabled);

}

//      ========== keypb_LineSettingsGet ===========
/*!
\brief         Get List of Lines settings
\param[in,ou]  <none>
\return

*/
void  keypb_LineSettingsGet(void)
{
    ST_APPCMBS_CONTAINER        st_Container;
    ST_IE_RESPONSE              st_Resp;
    u16                         u16_LinesMask = 0;

    memset(&st_Container, 0, sizeof(st_Container));

    printf("\nEnter Mask of lines in hex (FF for all): ");
    tcx_scanf("%hX", &u16_LinesMask);

    app_SrvLineSettingsGet(u16_LinesMask, 1);

    // wait for CMBS target message
    appcmbs_WaitForContainer(CMBS_EV_DSR_LINE_SETTINGS_LIST_GET_RES, &st_Container);

    memcpy(&st_Resp, &st_Container.ch_Info, st_Container.n_InfoLen);

    printf("\nLines settings list get %s\n", st_Resp.e_Response == CMBS_RESPONSE_OK ? "Success" : "Fail");
    printf("Press Any Key!\n");
    tcx_getch();
}


//      ========== keypb_LineSettingsSet ===========
/*!
\brief         Set List of Lines settings
\param[in,ou]  <none>
\return

*/
void  keypb_LineSettingsSet(void)
{
    u16 u16_Line_Id = 0, u16_Temp;
    ST_IE_LINE_SETTINGS_LIST    st_LineSettingsList;
    memset(&st_LineSettingsList, 0, sizeof(st_LineSettingsList));

    /* Line ID */
    printf("\nEnter line ID (in decimal): ");
    tcx_scanf("%hd", &u16_Line_Id);
    st_LineSettingsList.u8_Line_Id = (u8)u16_Line_Id;

    /* Attahced HS Mask */
    printf("\nEnter Hs mask in hex: ");
    tcx_scanf("%hX", &st_LineSettingsList.u16_Attached_HS);

    /* Call intrusion en / dis */
    printf("\nCall Intrusion: Enter 0 - disabled, 1 - enabled: ");
    tcx_scanf("%hd", &u16_Temp);
    st_LineSettingsList.u8_Call_Intrusion = (u8)u16_Temp;

    /* Multicall mode en / dis */
    printf("\nMulticall mode: Enter number of parallel calls (0 = multicalls disabled): ");
    tcx_scanf("%hd", &u16_Temp);
    st_LineSettingsList.u8_Multiple_Calls = (u8)u16_Temp;

    /* PSTN / VoIP Line */
    printf("\nLine Type: Enter\n0 - PSTN_DOUBLE_CALL\n1 - PSTN_PARALLEL_CALL\n2 - VOIP_DOUBLE_CALL\n3 - VOIP_PARALLEL_CALL: ");
    tcx_scanf("%hd", &u16_Temp);
    st_LineSettingsList.e_LineType = (E_CMBS_LINE_TYPE)u16_Temp;

    app_SrvLineSettingsSet(&st_LineSettingsList, 1);
}

void  keyb_AddPhoneABToContactList(void)
{
    stContactListEntry st_Entry;
    u32 pu32_FiledIDs[5], u32_EntryId;
    char s_PhoneBCNIP[] = { 0x50, 0x68, 0x6F, 0x6E, 0x65, 0x20, 0x42, 0x20, 0xC3, 0xA9, 0xC3, 0xA0, 0xC3, 0xBC, 0x00 }; //Phone B + (UTF-8 characters)

    pu32_FiledIDs[0] = FIELD_ID_LAST_NAME;
    pu32_FiledIDs[1] = FIELD_ID_FIRST_NAME;
    pu32_FiledIDs[2] = FIELD_ID_CONTACT_NUM_1;
    pu32_FiledIDs[3] = FIELD_ID_ASSOCIATED_MELODY;
    pu32_FiledIDs[4] = FIELD_ID_LINE_ID;

    strcpy(st_Entry.sLastName, "Phone A");
    strcpy(st_Entry.sFirstName, "x");
    strcpy(st_Entry.sNumber1, "121");
    st_Entry.cNumber1Type = NUM_TYPE_FIXED;
    st_Entry.bNumber1Default = TRUE;
    st_Entry.bNumber1Own = FALSE;
    st_Entry.u32_AssociatedMelody = 1;
    st_Entry.u32_LineId = 0;

    /* Insert Phone A */
    List_InsertEntry(LIST_TYPE_CONTACT_LIST, &st_Entry, pu32_FiledIDs, 5, &u32_EntryId);

    strcpy(st_Entry.sLastName, s_PhoneBCNIP);
    strcpy(st_Entry.sNumber1, "120");

    /* Insert Phone B */
    List_InsertEntry(LIST_TYPE_CONTACT_LIST, &st_Entry, pu32_FiledIDs, 5, &u32_EntryId);
}


void  keyb_AddLine0ToLineSettingsList(void)
{
    stLineSettingsListEntry st_Entry;
    u32 pu32_FiledIDs[12], u32_EntryId;

    pu32_FiledIDs[0] = FIELD_ID_LINE_NAME;
    pu32_FiledIDs[1] = FIELD_ID_LINE_ID;
    pu32_FiledIDs[2] = FIELD_ID_ATTACHED_HANDSETS;
    pu32_FiledIDs[3] = FIELD_ID_FP_MELODY;
    pu32_FiledIDs[4] = FIELD_ID_FP_VOLUME;
    pu32_FiledIDs[5] = FIELD_ID_BLOCKED_NUMBER;
    pu32_FiledIDs[6] = FIELD_ID_MULTIPLE_CALLS_MODE;
    pu32_FiledIDs[7] = FIELD_ID_INTRUSION_CALL;
    pu32_FiledIDs[8] = FIELD_ID_PERMANENT_CLIR;
    pu32_FiledIDs[9] = FIELD_ID_CALL_FWD_UNCOND;
    pu32_FiledIDs[10] = FIELD_ID_CALL_FWD_BUSY;
    pu32_FiledIDs[11] = FIELD_ID_CALL_FWD_NO_ANSWER;

    strcpy(st_Entry.sLineName, "Line-0");
    st_Entry.u32_LineId = 0;
    st_Entry.u32_AttachedHsMask = 0x03;
    st_Entry.u32_FPMelody = 1;
    st_Entry.u32_FPVolume = 1;
    strcpy(st_Entry.sBlockedNumber, "555");
    st_Entry.bMultiCalls = TRUE;
    st_Entry.bIntrusionCall = TRUE;
    st_Entry.bPermanentCLIR = FALSE;
    strcpy(st_Entry.sPermanentCLIRActCode, "333");
    strcpy(st_Entry.sPermanentCLIRDeactCode, "444");
    st_Entry.bCallFwdUncond = FALSE;
    strcpy(st_Entry.sCallFwdUncondActCode, "555");
    strcpy(st_Entry.sCallFwdUncondDeactCode, "666");
    strcpy(st_Entry.sCallFwdUncondNum, "123456");
    st_Entry.bCallFwdNoAns = FALSE;
    strcpy(st_Entry.sCallFwdNoAnsActCode, "888");
    strcpy(st_Entry.sCallFwdNoAnsDeactCode, "999");
    strcpy(st_Entry.sCallFwdNoAnsNum, "654321");
    st_Entry.u32_CallFwdNoAnsTimeout = 30;
    st_Entry.bCallFwdBusy = FALSE;
    strcpy(st_Entry.sCallFwdBusyActCode, "777");
    strcpy(st_Entry.sCallFwdBusyDeactCode, "222");
    strcpy(st_Entry.sCallFwdBusyNum, "98765432");

    List_InsertEntry(LIST_TYPE_LINE_SETTINGS_LIST, &st_Entry, pu32_FiledIDs, 12, &u32_EntryId);

    /* Need to update target with Line Settings */
    {
        ST_IE_LINE_SETTINGS_LIST st_LineSettingsList;

        st_LineSettingsList.u16_Attached_HS     = (u16)st_Entry.u32_AttachedHsMask;
        st_LineSettingsList.u8_Call_Intrusion   = st_Entry.bIntrusionCall;
        st_LineSettingsList.u8_Line_Id          = (u8)st_Entry.u32_LineId;
        st_LineSettingsList.u8_Multiple_Calls   = st_Entry.bMultiCalls ? 4 : 0;
        st_LineSettingsList.e_LineType          = CMBS_LINE_TYPE_VOIP_PARALLEL_CALL;

        app_SrvLineSettingsSet(&st_LineSettingsList, 1);
    }
}


//  ========== keypb_SYSTestModeGet ===========
/*!
\brief         Get current CMBS Target test mode state
\param[in,ou]  <none>
\return

*/
void  keypb_SYSTestModeGet(void)
{
    app_SrvTestModeGet(TRUE);
}

//  ========== keypb_SYSTestModeSet ===========
/*!
\brief         enable TBR 6 mode
\param[in,ou]  <none>
\return

*/
void  keypb_SYSTestModeSet(void)
{
    app_SrvTestModeSet();
}
//////////////////////////////////////////////////////////////////////////
void keyb_StartCmbsLogger(void)
{
    char strFileName[100];
    if (tcx_IsLoggerEnabled())
    {
        printf("Logger already started\n");
        return;
    }
    printf("Enter log file name:\n");
    tcx_scanf("%s", strFileName);
    if (tcx_LogOpenLogfile(strFileName))
    {
        printf("Logger started successfully\n");
    }
    else
    {
        printf("Failed to open log file \n");
    }

}
//////////////////////////////////////////////////////////////////////////
void keyb_StopCmbsLogger(void)
{
    tcx_LogCloseLogfile();
    printf("CMBS logger stopped \n");
}
//////////////////////////////////////////////////////////////////////////
void              keyb_DECTLoggerStart(void)
{
    app_LogStartDectLogger();
}
//////////////////////////////////////////////////////////////////////////
void              keyb_DECTLoggerStop(void)
{
    app_LogStoptDectLoggerAndRead();
}

void keyb_DataCall_CallEstablish(void)
{
    u16 u16_HSNumber;
    u32 u32_CallInstance;
    printf("\n Enter Destination HS number\n");
    tcx_scanf("%hd", &u16_HSNumber);
    u32_CallInstance = cmbs_dee_CallInstanceNew(g_cmbsappl.pv_CMBSRef);
    printf("\n Assigned Call Instance is 0x%X", u32_CallInstance);
    printf("\n Press any key to continue \n");
    tcx_getch();
    cmbs_dsr_dc_SessionStart(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance);
}
void keyb_DataCall_CallConclude(void)
{
    u16 u16_HSNumber;
    u32 u32_CallInstance;
    printf("\n Enter Destination HS number\n");
    tcx_scanf("%hd", &u16_HSNumber);
    printf("\n Enter Call Instance (in hex format)\n");
    tcx_scanf("%x", &u32_CallInstance);
    cmbs_dsr_dc_SessionStop(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance);
}

void keyb_DataCall_SendData(void)
{
    u32 u32_CallInstance;
    u8 pu8_IWUData[1000] = {0}; //two ASCII characters consume one byte
    u8 pu8_IWUData_HEX[CMBS_IWU_DATA_MAX_TRANSMIT_LENGTH];
    ST_APPCMBS_CONTAINER st_Container;
    u16 u16_DataLength;
    u16 u16_HSNumber;
    u8 u8_NumOfMessages = 0;
    u16 u16_total_len;
    st_DataCall_Header st_Header;
    int i;

    printf("\n Enter Call Instance  (in hex format)\n");
    tcx_scanf("%x", &u32_CallInstance);
    printf("\n Enter HS number\n");
    tcx_scanf("%hd", &u16_HSNumber);
    printf("\n Enter Hex Data (maximum 500 bytes)\n");
    tcx_gets((char *)pu8_IWUData, sizeof(pu8_IWUData));
    u16_DataLength = strlen((char *)pu8_IWUData) / 2;
    printf("u16_DataLength = %d\n", u16_DataLength);
    if (u16_DataLength <= CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH)
    {
        u8_NumOfMessages = 1;
    }
    else
    {
        u8_NumOfMessages = (u16_DataLength - CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH) / CMBS_IWU_DATA_MAX_NEXT_PAYLOAD_LENGTH + 1;
        if (u16_DataLength % CMBS_IWU_DATA_MAX_NEXT_PAYLOAD_LENGTH)
            u8_NumOfMessages++;
    }

    printf("u8_NumOfMessages = %d\n", u8_NumOfMessages);

    // Prepare header
    st_Header.u8_prot_disc = PROTOCOL_DISCRIMINATOR;
    st_Header.u8_desc_type = DESCRIMINATOR_TYPE;
    st_Header.u8_EMC_high = EMC_HIGH;
    st_Header.u8_EMC_low = EMC_LOW;
    st_Header.u8_command = FIRST_PACKET_COMMAND;
    st_Header.u8_length = (MIN(u16_DataLength,  CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH) + 4); //data length + 4 bytes reserved for protocol discriminator, descriptor type and EMC
    u16_total_len = u16_DataLength + 2 + u8_NumOfMessages * 6;
    printf("u16_total_len = %d\n", u16_total_len);
    st_Header.u8_total_len_low = (u16_total_len) & 0x00FF;
    st_Header.u8_total_len_high = (u16_total_len << 8) & 0xFF00;
    memcpy(pu8_IWUData_HEX, &st_Header, HEADER_SIZE);

    for (i = 0; i < (CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH * 2); i += 2)
    {
        pu8_IWUData_HEX[i / 2 + HEADER_SIZE] = app_ASC2HEX((char *)pu8_IWUData + i);
    }

    appcmbs_PrepareRecvAdd(1);
    cmbs_dsr_dc_DataSend(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance, pu8_IWUData_HEX, st_Header.u8_length + 4);
    appcmbs_WaitForContainer(CMBS_EV_DSR_DC_DATA_SEND_RES, &st_Container);
    u16_DataLength -= CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH;
    u8_NumOfMessages--;
    while (u8_NumOfMessages > 0)
    {
        st_Header.u8_command = NEXT_PACKET_COMMAND;
        st_Header.u8_length = (MIN(u16_DataLength,  CMBS_IWU_DATA_MAX_NEXT_PAYLOAD_LENGTH)) + 2;
        memcpy(pu8_IWUData_HEX, &st_Header, HEADER_SIZE - 2);
        for (i = 0; i < (CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH * 2); i += 2)
        {
            pu8_IWUData_HEX[i / 2 + HEADER_SIZE - 2] = app_ASC2HEX((char *)pu8_IWUData + i);
        }
        appcmbs_PrepareRecvAdd(1);
        cmbs_dsr_dc_DataSend(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance, pu8_IWUData_HEX, st_Header.u8_length + 4);
        appcmbs_WaitForContainer(CMBS_EV_DSR_DC_DATA_SEND_RES, &st_Container);
        u16_DataLength -= CMBS_IWU_DATA_MAX_FIRST_PAYLOAD_LENGTH;
        u8_NumOfMessages--;
    }
}

void keyb_DataCall_StressTest(void)
{

    ST_APPCMBS_CONTAINER        st_Container;
    u16 u16_HSNumber;
    u32 u32_CallInstance;
    u16 u16_Delay;
    u16 u16_Iterations;
    u16 u16_i;
    u8 pu8_IWUData_HEX[CMBS_IWU_DATA_MAX_TRANSMIT_LENGTH] =
    {
        0xC0, 0x81, 0x03, 0x04, 0x37, 0x31, 0x00, 0x3B,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
        0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54,
        0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x61, 0x62, 0x63, 0x64,
        0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
        0x6F, 0x70, 0x61, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79
    };

    printf("\n Enter Destination HS number\n");
    tcx_scanf("%hd", &u16_HSNumber);
    printf("\n Enter delay (in msec) between transmissions\n");
    tcx_scanf("%hd", &u16_Delay);
    printf("\n Enter number of iterations\n");
    tcx_scanf("%hd", &u16_Iterations);

    u32_CallInstance = cmbs_dee_CallInstanceNew(g_cmbsappl.pv_CMBSRef);
    printf("\n Assigned Call Instance is 0x%X", u32_CallInstance);
    printf("\n Press any key to continue \n");

    cmbs_dsr_dc_SessionStart(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance);

    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_DC_SESSION_START_RES, &st_Container);


    for (u16_i = 0; u16_i < u16_Iterations; u16_i++)
    {
        SleepMs(u16_Delay);
        cmbs_dsr_dc_DataSend(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance, pu8_IWUData_HEX, CMBS_IWU_DATA_MAX_TRANSMIT_LENGTH);
        appcmbs_PrepareRecvAdd(1);

        // wait for CMBS target message
        appcmbs_WaitForContainer(CMBS_EV_DSR_DC_DATA_SEND_RES, &st_Container);
    }

    cmbs_dsr_dc_SessionStop(g_cmbsappl.pv_CMBSRef, u16_HSNumber, u32_CallInstance);

}

void keypb_DataCallLoop(void)
{
    u8 u8_Ans = 0;
    tcx_appClearScreen();
    while (u8_Ans != 'q')
    {
        printf("Data Call Operation\n");
        printf("=====================\n\n");
        printf("Select option:\n");
        printf("1: Establish Data Call\n");
        printf("2: Conclude Data Call\n");
        printf("3: Send Data\n");
        printf("4: Stress Test Data Call\n ");
        printf("\n Press 'q' to return to menu\n");
        printf("\n");
        u8_Ans = tcx_getch();
        switch (u8_Ans)
        {
            case '1':
                keyb_DataCall_CallEstablish();
                break;
            case '2':
                keyb_DataCall_CallConclude();
                break;
            case '3':
                keyb_DataCall_SendData();
                break;
            case '4':
                keyb_DataCall_StressTest();
                break;
            default:
                printf("\nError parameter !\n");
                break;
        }
    }
    tcx_appClearScreen();
}

void keyb_AFESelectSlotAndCodec(void)
{
    char c_Codec = 0;
    u32 StartSlot = 0;
    char tmp[5];

    printf("Enter start slot:\n");

    tcx_gets(tmp, sizeof(tmp));
    StartSlot = (u32)atoi(tmp);


    printf("Enter codec ('n' for LINEAR NB or 'w' for LINEAR WB) ):\n");
    tcx_scanf("%c", &c_Codec);

    if ((c_Codec == 'n') || (c_Codec == 'N'))
    {
        AFEconfig.e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_NB;
        AFEconfig.u32_SlotMask = 0x3 << StartSlot;

    }
    else if ((c_Codec == 'w') || (c_Codec == 'W'))
    {
        AFEconfig.e_Codec = CMBS_AUDIO_CODEC_PCM_LINEAR_WB;
        AFEconfig.u32_SlotMask = 0xF << StartSlot;
    }

    g_u32_UsedSlots |= AFEconfig.u32_SlotMask;

    printf("\nEnter resource type: '0' for DAC/ADC, '1' for DCLASS: \n");
    tcx_scanf("%hhX", &AFEconfig.u8_Resource);
}

void keyb_AFEAllocateChannel(void)
{
    appsrv_AFEAllocateChannel();
}

void keyb_AFESelectEndpoints(void)
{
    u32 u32_temp;
    u8 u8_Offset = 0;
    u8 u8_Endpoint = 0;
    printf("\nChoose Audio sink: \n");
    printf("0.\tAMP_OUT0\n");
    printf("1.\tAMP_OUT1\n");
    printf("2.\tAMP_OUT2\n");
    printf("3.\tAMP_OUT3\n");
    // printf("4.\tCMBS_AFE_AMP_SINGIN0\n");
    // printf("5.\tCMBS_AFE_AMP_SINGIN1\n");
    // printf("6.\tCMBS_AFE_AMP_DIFFIN0\n");
    // printf("7.\tCMBS_AFE_AMP_DIFFIN1\n");
    printf("4.\tADC0\n");
    printf("5.\tADC1\n");

    tcx_scanf("%u", &u32_temp);
    AFEconfig.st_AFEEndpoints.e_AFEEndPointOUT = u32_temp;
    u8_Endpoint = AFEconfig.st_AFEEndpoints.e_AFEEndPointOUT;
    if (AFEconfig.st_AFEEndpoints.e_AFEEndPointOUT >= 4)
    {
        AFEconfig.st_AFEEndpoints.e_AFEEndPointOUT += 4;
    }
    AFEconfig.st_AFEEndpoints.e_AFEEndPointOUT += CMBS_AFE_AMP_START; // add CMBS_AFE_AMP_START to each chosen number

    printf("\nChoose Audio source: \n");
    // printf("0.\tCMBS_AFE_CHANNEL_IN_SINGIN0\n");
    // printf("1.\tCMBS_AFE_CHANNEL_IN_SINGIN1\n");
    if (u8_Endpoint >= 4)
    {
        printf("0.\tDIFFIN0\n");
        printf("1.\tDIFFIN1\n");
        printf("2.\tSINGIN0_MINUS_SINGIN1\n");
        printf("3.\tVREF_OFFSET\n");
        printf("4.\tMUTE_VREF\n");
        printf("5.\tMUTE_FLOAT\n");
        printf("6.\tDIGMIC1\n");
        printf("7.\tDIGMIC2\n");
        u8_Offset = CMBS_AFE_ADC_IN_DIFFIN0;
    }
    else
    {
        printf("0.\tDAC0\n");
        printf("1.\tDAC1\n");
        printf("2.\tDAC0_INV\n");
        printf("3.\tDAC1_INV\n");
        printf("4.\tSINGIN\n");
        printf("5.\tDIFFIN0\n");
        printf("6.\tDIFFIN1\n");
        printf("7.\tMUTE\n");
        u8_Offset = CMBS_AFE_AMPOUT_IN_START;
    }
    tcx_scanf("%u", &u32_temp);
    AFEconfig.st_AFEEndpoints.e_AFEEndPointIN = u32_temp;
    AFEconfig.st_AFEEndpoints.e_AFEEndPointIN += u8_Offset;     //CMBS_AFE_ADC_IN_START  CMBS_AFE_AMPOUT_IN_START

    app_AFEConnectEndpoints();
}
#define CMBS_AFE_AMP_END_OFFSET 7
#define CMBS_AFE_MIC_START_OFFSET (CMBS_AFE_AMP_END_OFFSET + 1)
#define CMBS_AFE_MIC_END_OFFSET 9
#define CMBS_AFE_DIGMIC_START_OFFSET (CMBS_AFE_MIC_END_OFFSET + 1)
#define CMBS_AFE_DIGMIC_END_OFFSET 11

void keyb_AFE_EnableDisableEndpoints(void)
{
    ST_IE_AFE_ENDPOINT st_Endpoint;
    u16 u16_Enable = 0;

    memset(&st_Endpoint, 0, sizeof(st_Endpoint));

    printf("\nChoose AFE end point: \n");
    printf("0. AMP_OUT0\n");
    printf("1. AMP_OUT1\n");
    printf("2. AMP_OUT2\n");
    printf("3. AMP_OUT3\n");
    printf("4. SINGIN0\n");
    printf("5. SINGIN1\n");
    printf("6. DIFFIN0\n");
    printf("7. DIFFIN1\n");
    printf("8. MIC0\n");
    printf("9. MIC1\n");
    printf("10. DIGMIC1\n");
    printf("11. DIGMIC2\n");

    tcx_scanf("%hd", &u16_Enable);
    st_Endpoint.e_AFEChannel = u16_Enable;

    if (st_Endpoint.e_AFEChannel <= CMBS_AFE_AMP_END_OFFSET)
    {
        st_Endpoint.e_AFEChannel += CMBS_AFE_AMP_START;
    }
    else if ((st_Endpoint.e_AFEChannel > CMBS_AFE_AMP_END_OFFSET) && (st_Endpoint.e_AFEChannel <= CMBS_AFE_MIC_END_OFFSET))
    {
        st_Endpoint.e_AFEChannel += (CMBS_AFE_MIC_START - CMBS_AFE_MIC_START_OFFSET);
    }
    else if ((st_Endpoint.e_AFEChannel >= CMBS_AFE_DIGMIC_START_OFFSET) && (st_Endpoint.e_AFEChannel <= CMBS_AFE_DIGMIC_END_OFFSET))
    {
        st_Endpoint.e_AFEChannel += (CMBS_DIGMIC_START - CMBS_AFE_DIGMIC_START_OFFSET);
    }
    printf("\nChoose '1' to Enable or '0' to disable: \n");
    tcx_scanf("%hd", &u16_Enable);

    app_AFEEnableDisableEndpoint(&st_Endpoint, u16_Enable);


}


void keyb_AFESetEndpointGain(void)
{
    ST_IE_AFE_ENDPOINT_GAIN  st_AFEEndpointGain;
    u16 u16_Selection;
    u16 u16_tmp;

    printf("\nChoose '1' to set Input gain or '0' to set Output gain: \n");
    tcx_scanf("%hd", &u16_Selection);

    printf("Choose endpoint: \n");
    printf("0.\tAMP_OUT0\n");
    printf("1.\tAMP_OUT1\n");
    printf("2.\tAMP_OUT2\n");
    printf("3.\tAMP_OUT3\n");
    printf("4.\tSINGIN0\n");
    printf("5.\tSINGIN1\n");
    printf("6.\tDIFFIN0\n");
    printf("7.\tDIFFIN1\n");
    tcx_scanf("%hd", &u16_tmp);
    st_AFEEndpointGain.e_AFEChannel = u16_tmp + CMBS_AFE_AMP_START;

    if (u16_Selection == TRUE)
    {
        //Select Input gain
        printf("Select Input gain:\n");
        printf("0.\tMINUS_4DB\n");
        printf("1.\tMINUS_2DB\n");
        printf("2.\t0DB\n");
        printf("3.\t11KRIN\n");
        printf("4.\t2DB\n");
        printf("5.\t4DB\n");
        printf("6.\t6DB\n");
        printf("7.\t22_5KRIN\n");
        printf("8.\t8DB\n");
        printf("9.\t10DB\n");
        printf("10.\t12B\n");
        printf("11.\t45KRIN\n");
        printf("12.\t14DB\n");
        printf("13.\t16DB\n");
        printf("14.\t18DB\n");
        printf("15.\t90KRIN\n");
        printf("16.\t20DB\n");
        printf("17.\t22DB\n");
        printf("18.\t24DB\n");
        printf("19.\t180KRIN\n");
        printf("20.\t26DB\n");
        printf("21.\t28DB\n");
        printf("22.\t30DB\n");
        printf("23.\t360KRIN\n");
        printf("24.\t32DB\n");
        printf("25.\t34DB\n");
        printf("26.\t36DB\n");
        printf("27.\t720KRIN\n");
        printf("28.\tOLOOP1\n");
        printf("29.\tOLOOP2\n");
        printf("30.\tOLOOP3\n");
        printf("31.\tOLOOP4\n");

        tcx_scanf("%hd", &u16_tmp);
        st_AFEEndpointGain.s16_NumOfSteps = u16_tmp;

    }
    else
    {
        // Select Output gain
        printf("\nSelect Output gain:\n");
        printf("0.0DB\n");
        printf("1.MINUS_6DB\n");
        printf("2.MINUS_12DB\n");
        printf("3.MINUS_18DB\n");
        printf("4.MINUS_24DB\n");
        tcx_scanf("%hd", &u16_tmp);
        st_AFEEndpointGain.s16_NumOfSteps = u16_tmp + CMBS_AFE_OUT_GAIN_START;
    }

    appsrv_AFESetEndpointGain(&st_AFEEndpointGain, u16_Selection);
}
void keyb_AFEAUXMeasurement(void)
{
    u16 scanfInput = 0;

    ST_APPCMBS_CONTAINER         st_Container;
    ST_IE_AFE_AUX_MEASUREMENT_SETTINGS st_AFEMeasurementSettings;

    printf("\nEnter AUX Input to measure  (in HEX): ");
    tcx_scanf("%hX", &scanfInput);
    st_AFEMeasurementSettings.e_AUX_Input = (E_CMBS_AUX_INPUT)scanfInput;

    // printf("\nEnter '1' to measure via BMP, '0' to measure manually: ");
    st_AFEMeasurementSettings.b_Bmp = 0;

    cmbs_dsr_AFE_AUXMeasurement(g_cmbsappl.pv_CMBSRef, &st_AFEMeasurementSettings);

    appcmbs_PrepareRecvAdd(1);
    appcmbs_WaitForContainer(CMBS_EV_DSR_AFE_AUX_MEASUREMENT_RES, &st_Container);
}

void keyb_AFEOpenAudioChannel(void)
{
    appsrv_AFEOpenAudioChannel();
}

void keyb_AFECloseAudioChannel(void)
{
    u32 u32_ChannelID;

    printf("Enter Channel ID:\n");
    tcx_scanf("%X", &u32_ChannelID);

    g_u32_UsedSlots &= ~(AFEconfig.u32_SlotMask);

    appsrv_AFECloseAudioChannel(u32_ChannelID);
}

void keyb_AFEDHSGValueSend(void)
{
    u8  u8_Value;
    u32 u32_Value;

    printf("\nEnter byte value:\n");

    tcx_scanf("%X", &u32_Value);
    u8_Value = (u8)u32_Value;

    appsrv_AFEDHSGSendByte(u8_Value);
}

void keyb_AFEDHSGEnable(void)
{
    u8  u8_Value;
    u32 u32_Value;


    do
    {
        printf("\nSelect 1-Enable 0-Ignore:\n");
        tcx_scanf("%d", &u32_Value);
        u8_Value = (u8)u32_Value;
    } while ((u32_Value != 0) && (u32_Value != 1));

    if (u8_Value)
    {
        app_SrvParamSet(CMBS_PARAM_DHSG_ENABLE, &u8_Value, CMBS_PARAM_DHSG_ENABLE_LENGTH, 1);
    }
}



void keyb_GPIOControl(void)
{
    u32 u32_temp;
    u8 u8_Selection;
    ST_IE_GPIO_ID st_GPIOId;
    tcx_appClearScreen();

    memset(&st_GPIOId, 0, sizeof(ST_IE_GPIO_ID));

    printf("Select action:\n");
    printf("1.\tEnable GPIO\n");
    printf("2.\tDisable GPIO\n");
    printf("3.\tControl GPIO Settings\n");
    printf("4.\tGet GPIO Settings\n");
    u8_Selection = tcx_getch();

    switch (u8_Selection)
    {
        case '1':
        {
            printf("\nEnter GPIO BANK (0 - 4) [A=0...E=4]: \n");
            tcx_scanf("%u", &u32_temp);
            st_GPIOId.e_GPIOBank = u32_temp;
            if (st_GPIOId.e_GPIOBank >= CMBS_GPIO_BANK_MAX)
                printf("ERROR! Invalid GPIO BANK\n");
            printf("\nEnter GPIO #: \n");
            tcx_scanf("%d", &st_GPIOId.u32_GPIO);
            app_SrvGPIOEnable(&st_GPIOId);
        }
        break;
        case '2':
        {
            printf("\nEnter GPIO BANK (0 - 4) [A=0...E=4]: \n");
            tcx_scanf("%u", &u32_temp);
            st_GPIOId.e_GPIOBank = u32_temp;
            if (st_GPIOId.e_GPIOBank >= CMBS_GPIO_BANK_MAX)
                printf("ERROR! Invalid GPIO BANK\n");
            printf("\nEnter GPIO #: \n");
            tcx_scanf("%d", &st_GPIOId.u32_GPIO);
            app_SrvGPIODisable(&st_GPIOId);
        }
        break;
        case '3':
        {
            ST_GPIO_Properties st_Prop[4];

            printf("\nEnter GPIO BANK (0 - 4) [A=0...E=4]: \n");
            tcx_scanf("%u", &u32_temp);
            st_GPIOId.e_GPIOBank = u32_temp;
            if (st_GPIOId.e_GPIOBank >= CMBS_GPIO_BANK_MAX)
                printf("ERROR! Invalid GPIO BANK\n");
            printf("Enter GPIO #: \n");
            tcx_scanf("%d", &st_GPIOId.u32_GPIO);
            printf("Enter Mode ('0' for GPIO_MODE_INPUT, '1' for GPIO_MODE_OUTPUT, 'F' to skip configuration)\n");
            tcx_scanf("%u", &u32_temp);
            st_Prop[0].u8_Value = u32_temp;
            st_Prop[0].e_IE = CMBS_IE_GPIO_MODE;
            printf("Enter Pull Type ('0' for GPIO_PULL_DN, '1' for GPIO_PULL_UP, 'F' to skip configuration)\n");
            tcx_scanf("%u", &u32_temp);
            st_Prop[1].u8_Value = u32_temp;
            st_Prop[1].e_IE = CMBS_IE_GPIO_PULL_TYPE;
            printf("Enter Pull Enable ('0' for GPIO_PULL_DIS, '1' GPIO_PULL_ENA, 'F' to skip configuration)\n");
            tcx_scanf("%u", &u32_temp);
            st_Prop[2].u8_Value = u32_temp;
            st_Prop[2].e_IE = CMBS_IE_GPIO_PULL_ENA;
            printf("Enter Value ('0' for DATA CLR, '1' for DATA SET, 'F' to skip configuration)\n");
            tcx_scanf("%u", &u32_temp);
            st_Prop[3].u8_Value = u32_temp;
            st_Prop[3].e_IE = CMBS_IE_GPIO_VALUE;

            app_SrvGPIOSet(&st_GPIOId, st_Prop);
        }
        break;
        case '4':
        {
            ST_GPIO_Properties st_Prop[4];
            u8 u8_Ans = 0;
            u8 u8_Idx;
            for (u8_Idx = 0; u8_Idx < 4; u8_Idx++)
            {
                st_Prop[u8_Idx].e_IE = 0xF;
            }
            printf("Enter GPIO BANK (0 - 4) [A=0...E=4]: \n");
            tcx_scanf("%u", &u32_temp);
            st_GPIOId.e_GPIOBank = u32_temp;
            if (st_GPIOId.e_GPIOBank >= CMBS_GPIO_BANK_MAX)
                printf("ERROR! Invalid GPIO BANK\n");
            printf("Enter GPIO #: \n");
            tcx_scanf("%d", &st_GPIOId.u32_GPIO);

            tcx_appClearScreen();

            while (u8_Ans != 'q')
            {
                printf("Choose parameters:\n");
                printf("1. GPIO mode (input/output)\n");
                printf("2. GPIO value \n");
                printf("3. GPIO pull type \n");
                printf("4. GPIO pull enable\n");
                printf("Press 'q' to send the message\n ");

                u8_Ans = tcx_getch();

                switch (u8_Ans)
                {
                    case '1':
                        st_Prop[0].e_IE = CMBS_IE_GPIO_MODE;
                        break;

                    case '2':
                        st_Prop[1].e_IE = CMBS_IE_GPIO_VALUE;
                        break;

                    case '3':
                        st_Prop[2].e_IE = CMBS_IE_GPIO_PULL_TYPE;
                        break;

                    case '4':
                        st_Prop[3].e_IE = CMBS_IE_GPIO_PULL_ENA;
                        break;

                    default:
                        break;
                }
            }
            app_SrvGPIOGet(&st_GPIOId, st_Prop);
        }

        break;

        default:
            break;
    }
}
void keypb_AFESettingsMenu(void)
{
    u8 u8_Ans = 0;
    tcx_appClearScreen();
    memset(&AFEconfig, 0, sizeof(AFEconfig));

    while (u8_Ans != 'q')
    {
        printf("=====================\n");
        printf("Chosen AFE Settings:\n");
        printf("=====================\n\n");

        printf("Preffered Codec: %s \n", app_GetCodecString(AFEconfig.e_Codec));
        printf("Slot Mask: %2X  \n", g_u32_UsedSlots);
        printf("Resource: %s\n", AFEconfig.u8_Resource == CMBS_AFE_RESOURCE_DACADC ? "DAC/ADC" : "DCLASS");
        printf("Instance Number (received from target): %x\n", AFEconfig.u8_InstanceNum);
        printf("Channel ID (received from target): %x\n", AFEconfig.u32_ChannelID);
        printf("Point: Input: %s Output: %s \n", app_GetAFEEndpointString(AFEconfig.st_AFEEndpoints.e_AFEEndPointIN), app_GetAFEEndpointString(AFEconfig.st_AFEEndpoints.e_AFEEndPointOUT));


        printf("=====================\n\n");
        printf("Select option:\n");
        printf("1: Select slot and codec\n");
        printf("2: Allocate channel\n");
        printf("3: Connect AFE Endpoints\n");
        printf("4: Enable or Disable AFE Endpoint\n");
        printf("5: Set Endpoint gain\n");
        printf("6: AFE AUX measurement\n");
        printf("7: AFE Open Audio Channel\n");
        printf("8: AFE Close Audio Channel\n");

        printf("\n Press 'q' to return to menu\n");
        printf("\n");
        u8_Ans = tcx_getch();
        printf("\n");

        switch (u8_Ans)
        {
            case '1':
                keyb_AFESelectSlotAndCodec();
                break;
            case '2':
                keyb_AFEAllocateChannel();
                break;
            case '3':
                keyb_AFESelectEndpoints();
                break;
            case '4':
                keyb_AFE_EnableDisableEndpoints();
                break;
            case '5':
                keyb_AFESetEndpointGain();
                break;
            case '6':
                keyb_AFEAUXMeasurement();
                break;
            case '7':
                keyb_AFEOpenAudioChannel();
                break;
            case '8':
                keyb_AFECloseAudioChannel();
                break;
            default:
                break;
        }
    }
}


#ifdef CHECKSUM_SUPPORT
//  ========== keyb_SrvChecksumTest ===========
/*!
        \brief         keyboard loop for checksum test
      \param[in,ou]  <none>
        \return <none>

*/
void keyb_SrvChecksumTest(void)
{
    int n_Keep = TRUE;
    char c;

    while (n_Keep)
    {
        printf("\n-----------------------------\n");
        printf("Choose Checksum Test:\n");
        printf("1 => Send Event with wrong checksum \n");
        printf("2 => Send Event without checksum\n");
        printf("3 => Send event with distroyed CHECKSUM IE\n\n");

        printf("5 => send CMBS_EV_CHECKSUM_FAILURE with CMBS_CHECKSUM_ERROR\n");
        printf("6 => send CMBS_EV_CHECKSUM_FAILURE with CMBS_CHECKSUM_NO_EVENT_ID\n");

        printf("- - - - - - - - - - - - - - - \n");
        printf("q => Return to Interface Menu\n");

        c = tcx_getch();

        switch (c)
        {
            case '1':
                // Send wrong checksum in send message
            case '2':
                // Send event without checksum
            case '3':
                // Send event with distroyed CHECKSUM IE

                cmbs_int_SimulateChecksumError(c - 0x30);
                // send any event with an IE included
                app_SrvTestModeSet();
                // reset error simulation
                cmbs_int_SimulateChecksumError(0);

                break;

            case '5':
                // send CMBS_EV_CHECKSUM_FAILURE with CMBS_CHECKSUM_ERROR
            case '6':
                // send CMBS_EV_CHECKSUM_FAILURE with CMBS_CHECKSUM_NO_EVENT_ID
                cmbs_int_SimulateChecksumError(c - 0x30);
                break;

            case 'q':
                n_Keep = FALSE;
                break;
        }
    }
}
#endif


void keyb_ExternalInterruptsControl(void)
{
    u32 u32_temp;
    u8 u8_Ans = 0;
    u16 u16_Ans = 0;
    u8 u8_IntNumber = 0;
    ST_IE_INT_CONFIGURATION st_INTConfiguration;
    ST_IE_GPIO_ID st_GPIOId;

    tcx_appClearScreen();

    memset(&st_GPIOId, 0, sizeof(ST_IE_GPIO_ID));
    memset(&st_INTConfiguration, 0, sizeof(ST_IE_INT_CONFIGURATION));


    while (u8_Ans != 'q')
    {
        printf("=====================\n\n");
        printf("Select option:\n");
        printf("1: External Interrupt Configure\n");
        printf("2: External Interrupt Enable\n");
        printf("3: External Interrupt Disable\n");
        printf("\n Press 'q' to return to menu\n");
        printf("\n");
        u8_Ans = tcx_getch();

        switch (u8_Ans)
        {
            case '1':
            {
                printf("\nEnter Interrupt Number:\n");
                tcx_scanf("%hd", &u16_Ans);
                u8_IntNumber = (u8)u16_Ans;

                printf("\nEnter GPIO BANK (0 - 4) [A=0...E=4]:\n");
                tcx_scanf("%u", &u32_temp);
                st_GPIOId.e_GPIOBank = u32_temp;
                if (st_GPIOId.e_GPIOBank >= CMBS_GPIO_BANK_MAX)
                {
                    printf("ERROR! Invalid GPIO BANK\n");
                    break;
                }
                printf("\nEnter GPIO #: \n");
                tcx_scanf("%d", &st_GPIOId.u32_GPIO);

                printf("\nEnter Interrupt polarity: (active_low=0 / active_high=1)\n");
                tcx_scanf("%hd", &u16_Ans);
                st_INTConfiguration.u8_INTPolarity = (u8)u16_Ans;

                printf("\nEnter Interrupt Type: (Mode: Level=0 / edge=1)\n");
                tcx_scanf("%hd", &u16_Ans);
                st_INTConfiguration.u8_INTType = (u8)u16_Ans;

                app_SrvExtIntConfigure(&st_GPIOId, &st_INTConfiguration, u8_IntNumber);
            }
            break;


            case '2':
            {
                printf("\nEnter Interrupt Number:\n");
                tcx_scanf("%hd", &u16_Ans);
                u8_IntNumber = (u8)u16_Ans;
                app_SrvExtIntEnable(u8_IntNumber);
            }
            break;

            case '3':
            {
                printf("\nEnter Interrupt Number:\n");
                tcx_scanf("%hd", &u16_Ans);
                u8_IntNumber = (u8)u16_Ans;
                app_SrvExtIntDisable(u8_IntNumber);
            }
            break;

            default:
                break;
        }
    }

}

void keypb_HWControlMenu(void)
{
    u8 u8_Ans = 0;
    tcx_appClearScreen();

    while (u8_Ans != 'q')
    {
        printf("=====================\n\n");
        printf("Select option:\n");
        printf("1: AFE Settings Menu\n");
        printf("2: GPIO Control\n");
        printf("3: External Interrupts Control\n");
        printf("4: Send DHSG Value\n");
        printf("5: Enable DHSG (once only)\n");
        printf("\n Press 'q' to return to menu\n");
        printf("\n");
        u8_Ans = tcx_getch();
        switch (u8_Ans)
        {
            case '1':
                keypb_AFESettingsMenu();
                break;
            case '2':
                keyb_GPIOControl();
                break;
            case '3':
                keyb_ExternalInterruptsControl();
                break;
            case '4':
                keyb_AFEDHSGValueSend();
                break;
            case '5':
                keyb_AFEDHSGEnable();

            default:
                break;
        }
    }
}



void  keyb_SRVLoop(void)
{
    int n_Keep = TRUE;

    while (n_Keep)
    {
        //      tcx_appClearScreen();
        printf("-----------------------------\n");
        printf("Choose service:\n");
        printf("1 => EEProm  Param Get\n");
        printf("2 => EEProm  Param Set\n");
        printf("3 => Production  Param Get\n");
        printf("4 => Production  Param Set\n");
        printf("5 => Handset Page\n");
        printf("6 => Stop Paging\n");
        printf("7 => Handset Delete\n");
        printf("8 => System  Registration Mode\n");
        printf("9 => System  Restart\n");
        printf("A => System  FW/HW/EEPROM Version Get\n");
        printf("B => System  Testmode Get\n");
        printf("C => System  Testmode Set\n");
        printf("D => SysLog  Start\n");
        printf("E => SysLog  Stop\n");
        printf("F => SysLog  Read\n");
        printf("G => List Of Registered Handsets\n");
        printf("H => Set new handset name\n");
        printf("I => RF Control and carrier fix\n");
        printf("J => Turn On/Off NEMo mode\n");
        printf("K => System  Power Off\n");
        printf("L => Get Line Settings\n");
        printf("M => Set Line Settings\n");
        printf("N => ATE test start\n");
        printf("O => ATE test leave\n");
        printf("P => List Access\n");
        printf("R => Start CMBS messages logging\n");
        printf("S => Stop CMBS logging\n");
        printf("t/T => Get/Set DECT settings\n");
        printf("u => Add New Extension\n");
        printf("V => Get subs. HS, EEPROM reset, Set subs. HS\n");
        printf("v => Target <-> Host communication stress test\n");
        printf("W => Encryption Disable/Enable if feature supported\n");
        printf("w => FP Customer settings (Int call, ConfCall) change \n");
        printf("x => Set Base Name\n");
        printf("z => Get Base Name\n");
        printf("Z => HW Control menu (AFE, GPIO, External Interrupts configuration)\n");
        printf("+ => Start DECT logger \n");
        printf("- => Stop and read DECT logs \n");
        printf("y => Data Call\n");
        printf("Y => Display subscription Data\n");
        printf(". => Ping\n");
        printf("/ => Create EEPROM backup file\n");
        printf("# => ULE Auto Subscription\n");

        printf("- - - - - - - - - - - - - - - \n");
        printf("q => Return to Interface Menu\n");

        switch (tcx_getch())
        {
            case ' ':
                tcx_appClearScreen();
                break;

            case '1':
                keypb_EEPromParamGet();
                break;
            case '2':
                keypb_EEPromParamSet();
                break;
            case '3':
                keypb_ProductionParamGet();
                break;
            case '4':
                keypb_ProductionParamSet();
                break;
            case '5':
                keypb_HandsetPage();
                break;
            case '6':
                keypb_HandsetStopPaging();
                break;
            case '7':
                keypb_HandsetDelete();
                break;
            case '8':
                keypb_SYSRegistrationMode();
                break;
            case '9':
                keypb_SYSRestart();
                break;
            case 'a':
            case 'A':
                keypb_SYSFWVersionGet();
                keypb_SYSEEPROMVersionGet();
                keypb_SYSHWVersionGet();
                break;
            case 'b':
            case 'B':
                keypb_SYSTestModeGet();
                break;
            case 'c':
            case 'C':
                keypb_SYSTestModeSet();
                break;
            case 'd':
            case 'D':
                keypb_SysLogStart();
                break;
            case 'e':
            case 'E':
                keypb_SysLogStop();
                break;
            case 'f':
            case 'F':
                keypb_SysLogRead();
                break;
            case 'g':
            case 'G':
                keypb_RegisteredHandsets();
                break;
            case 'h':
            case 'H':
                keyb_SetNewHandsetName();
                break;
            case 'i':
            case 'I':
                keypb_RF_Control();
                break;
            case 'j':
            case 'J':
                keypb_Turn_On_Off_NEMo_mode();
                break;
            case 'k':
            case 'K':
                keypb_SYSPowerOff();
                break;
            case 'l':
            case 'L':
                keypb_LineSettingsGet();
                break;
            case 'm':
            case 'M':
                keypb_LineSettingsSet();
                break;

            case 'n':
            case 'N':
                keyb_StartATETest();
                break;
            case 'o':
            case 'O':
                keyb_LeaveATETest();
                break;

            case 'p':
            case 'P':
                keyb_ListAccess();
                break;

            case 'r':
            case 'R':
                keyb_StartCmbsLogger();
                break;

            case 's':
            case 'S':
                keyb_StopCmbsLogger();
                break;

            case 't':
                keypb_DectSettingsGet();
                break;

            case 'T':
                keypb_DectSettingsSet();
                break;

            case 'u':
                keypb_AddNewExtension();
                break;

            case 'V':
                keyb_SubsHSTest();
                break;

            case 'v':
                keyb_CommStress();
                break;

            case 'W':
                keypb_SYSEncryptionMode();
                break;

            case 'w':
                keypb_SYSFPCustomFeatures();
                break;

            case 'x':
                keypb_SetBaseName();
                break;

            case 'z':
                keypb_GetBaseName();
                break;

            case 'Z':
                keypb_HWControlMenu();
                break;

            case '+':
                keyb_DECTLoggerStart();
                break;

            case '-':
                keyb_DECTLoggerStop();
                break;
            case 'y':
                keypb_DataCallLoop();
                break;
            case 'Y':
                keyb_DisplaySubscriptionData();
                break;
            case '.':
                cmbs_dsr_Ping(g_cmbsappl.pv_CMBSRef);
                break;
            case '/':
                keyb_EEPROMBackupCreate();
                break;
            case '#':
                keyb_ULEDevicesAutoSubscription();
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
