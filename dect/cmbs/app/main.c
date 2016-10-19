#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/msg.h>

#include "cmbs_api.h"
#include "cmbs_int.h"

#include "tcx_util.h"

#include "appcmbs.h"
#include "appswup.h"
#include "appsrv.h"

#define FILENAME_MAX_SIZE           64          // max size of filename
#define PACKET_SIZE_DEFAULT         256         // default size of packet for fw upgrade
#define PACKET_SIZE_MIN             32          // min size of packet for fw upgrade
#define PACKET_SIZE_MAX             512         // max size of packet for fw upgrade (maximum of Flash Page size)

ST_CMBS_DEV                         g_st_DevMedia;
ST_CMBS_DEV                         g_st_DevCtl;
ST_UART_CONFIG                      g_st_UartCfg;
ST_TDM_CONFIG                       g_st_TdmCfg;

// flag if eeprom file is on host
u8 g_EepromOnHost = 0;

extern ST_CB_LOG_BUFFER             pfn_log_buffer_Cb;
extern ST_CMBS_UPGRADE_SETUP        g_stSwupSetup;

#ifdef HAN_SERVER_DEMO

unsigned long han_createThread(char *pInterfaceName, unsigned short port);

char * psz_interfaceName = "eth0";
u16      u16_BindPort = 3490;

u16 u16_Han_Server = FALSE;

bool s_b_HanThreadExists = FALSE;

void han_serverStart(void)
{
    u16_Han_Server = TRUE;
    if (s_b_HanThreadExists)
    {
        printf("han thread already exists\n");
    }
    else
    {
        s_b_HanThreadExists = TRUE;
        han_createThread(psz_interfaceName, u16_BindPort);
    }
}

#endif

int cmbs_init(E_CMBS_HW_COM_TYPE com_type, u8 com_port)
{
    int ret = CMBS_RC_OK;

    // detect Com Port
    if (com_port == 0) {
        /* interactive mode */
        com_port = tcx_DetectComPort(TRUE, &g_st_DevCtl.e_DevType);
    }
    printf("COM PORT: %d\n", com_port);

    // Config Device Type
    if (com_type == CMBS_HW_COM_TYPE_USB)
        tcx_USBConfig(com_port);

    // Start Init
    ret = appcmbs_Initialize(NULL, &g_st_DevCtl, &g_st_DevMedia, pfn_log_buffer_Cb);
    if (ret != CMBS_RC_OK) {
        printf("call appcmbs_Initialize failed!\n");
        return ret;
    }

    return CMBS_RC_OK;
}

int cmbs_firmware_upgrade_start(char *filename, int packet_size)
{
    int ret = CMBS_RC_OK;
    u16 version = 0x0000;

    g_stSwupSetup.u16_PacketSize = (packet_size / 32) * 32;
    if (!g_stSwupSetup.u16_PacketSize)
        g_stSwupSetup.u16_PacketSize = PACKET_SIZE_DEFAULT;

    // wait 0.2 seconds
    SleepMs(200);

    // get firmware version
    ret = app_SwupGetImageVersion(filename, &version);
    if (ret != CMBS_RC_OK) {
        printf("can't get version of firmware file!\n");
        return ret;
    }

    // perform firmware upgrade
    printf("Starting FW Upgrade: File: %s, PacketSize: %d, version:0x%2x \n" , filename, packet_size, version);
    ret = app_UpdateAndCheckVersion(filename, version);
    if (ret != CMBS_RC_OK) {
        printf("call app_UpdateAndCheckVersion failed!\n");
        return ret;
    }

    return CMBS_RC_OK;
}

void help_print()
{
    printf("usage: cmbs_fwup -f [filename] -s [packet_size]\n");
    printf("\tFirmware upgrade tools for DSPG DECT module.\n");
}
#if 0
int main(int argc, char *argv[])
{
    int flag = 0, ret = 0;
    char filename[FILENAME_MAX_SIZE] = {0};
    int packet_size = PACKET_SIZE_DEFAULT;

    // parameters input
    while ((flag = getopt(argc, argv, "f:s:?")) != -1) {
        switch (flag) {
            case 'f' :
                strncpy(filename, optarg, FILENAME_MAX_SIZE - 1);
                break;
            case 's' :
                packet_size = atoi(optarg);
                break;
            case '?' :
                help_print();
                return -1;
                break;
        }
    }

    // parameters check
    if (packet_size < PACKET_SIZE_MIN || packet_size > PACKET_SIZE_MAX) {
        printf("PacketSize only between %d to %d bytes!\n", PACKET_SIZE_MIN, PACKET_SIZE_MAX);
        return -1;
    }

    if (strlen(filename) == 0) {
        printf("Firmware filename is required!\n");
        return -1;
    }

    // cmbs init
    ret = cmbs_init(CMBS_HW_COM_TYPE_USB, 0);
    if (ret != CMBS_RC_OK) {
        printf("CMBS Init failed!\n");
        return -1;
    }

    // Try to get HW Chip version
    // robin@tecom, it will initialize some global variable for firmware upgrade needed
    if ((g_CMBSInstance.u16_TargetVersion & 0xFF00) != 0x0000 &&   // Bootloader
            (g_CMBSInstance.u16_TargetVersion & 0xF000) != 0x2000 && // CMBS 2xxx version (e.g. 2.99.9) - Old IE Header structure
            (g_CMBSInstance.u16_TargetVersion & 0xFF00) != 0x0200)   // CMBS 02xx version (e.g. 2.99) - Old IE Header structure
        app_SrvHWVersionGet(TRUE);

    // Start DECT model
    ret = appcmbs_CordlessStart(NULL);
    if (ret == FALSE) {
        printf("CMBS CordlessStart failed!\n");
        return -1;
    }

    // start firmware upgrade
    ret = cmbs_firmware_upgrade_start(filename, packet_size);
    if (ret != CMBS_RC_OK) {
        printf("call cmbs_firmware_upgrade_start failed!\n");
        return -1;
    }

    // exit app
    return 0;
}
#endif
