/*!
* \file  appswup.c
* \brief  handles firmware update requests
* \Author  stein
*
* @(#) %filespec: appswup.c~6.1.12.2.3 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
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

#include "cmbs_api.h"
#include "cfr_ie.h"
#include "cfr_mssg.h"
#include "cmbs_event.h"
#include "appcmbs.h"
#include "appswup.h"
#include "appsrv.h"
#include "appmsgparser.h"
#include "cmbs_int.h"


#pragma pack(push) //Store defaults
#pragma pack(1) // Byte packed

typedef struct {
    unsigned char md5[16]; // MD5 chksum
} t_swup_ChkSum;

static int    count = 0;

typedef struct
{
    unsigned int     MagicNumber;
    unsigned char    u8_ImgHeaderSize;   /* Image Header size in bytes */
    unsigned char    u8_Version[3];      /* Version in BCD coded format i.e 03.11.10 */
    unsigned int  u32_Build;          /* Version build */
    unsigned int     u32_BuildDate;      /* Build Date ddmmyyyy */
    char             u8_ImgType;                /* 1 - Handset image 0 - Base image */
    char             u8_Reserved;
    unsigned short   u16_EepromLayoutVer;       /*  Version of EEPROM Layout */
    unsigned int     u32_ImageCodeSize;  /* Code Image size in bytes */
    t_swup_ChkSum    ImageCodeChecksum;  /* 16 bytes checksum for Code Image */
    unsigned int     u32_ImageDataSize;  /* Data Image size in bytes */
    t_swup_ChkSum    ImageDataChecksum;  /* 16 bytes checksum for Data Image */
}  ST_IMAGE_HEADER, *PST_IMAGE_HEADER;
#pragma pack(pop) // Restore defaults




//#define PACKETSIZE 128 // SPI restrictions
ST_CMBS_UPGRADE_SETUP g_stSwupSetup;

#ifdef WIN32
typedef HANDLE      ST_FILE_HANDLER;
#define ST_INVALID_FILE_HANDLER  INVALID_HANDLE_VALUE
#else //(linux)
typedef int       ST_FILE_HANDLER;
#define ST_INVALID_FILE_HANDLER  (-1)
#endif

ST_FILE_HANDLER app_SwupOpenFile(const char *pchFileName)
{
#ifdef WIN32
    return CreateFile((LPCSTR)pchFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else //(linux)
    return open(pchFileName, O_RDONLY);
#endif

}

void app_SwupReadFile(ST_FILE_HANDLER hFile, char *pOutBuffer, int nSize, int *pnReadCnt)
{
#ifdef WIN32
    ReadFile(hFile, pOutBuffer, nSize, pnReadCnt, NULL);
#else
    *pnReadCnt = read(hFile, pOutBuffer, nSize);
#endif
}

void app_SwupCloseFile(ST_FILE_HANDLER hFile)
{
#ifdef WIN32
    CloseHandle(hFile);
#else //(linux)
    close(hFile);
#endif
}

E_CMBS_RC fwapp_ResponseCheck(void *pv_List)
{
    PST_CFR_IE_LIST pv_RefIEList = (PST_CFR_IE_LIST)pv_List;

    if (pv_RefIEList->u16_CurSize == 3)   /* old booters with 1-byte IE length */
        return pv_RefIEList->pu8_Buffer[2] == CMBS_RESPONSE_OK ? CMBS_RC_OK : CMBS_RC_ERROR_GENERAL;
    else                                /* new booter with 2-byte IE length */
    {
        void *pv_IE;

        if (cmbs_api_ie_GetIE(pv_List, &pv_IE, CMBS_IE_RESPONSE) == CMBS_RC_OK)
        {
            ST_IE_RESPONSE st_Response;

            // check response code:
            cmbs_api_ie_ResponseGet(pv_IE, &st_Response);

            return st_Response.e_Response;
        }
    }

    return CMBS_RC_ERROR_GENERAL;
}

void           app_OnFwUpdStartRsp(void *pv_List)
{
    char chRetCode =  fwapp_ResponseCheck(pv_List);
    appcmbs_ObjectSignal(&chRetCode, 1, 0, CMBS_EV_DSR_FW_UPD_START_RES);
}

void           app_OnFwUpdNextRsp(void *pv_List)
{
    char chRetCode =  fwapp_ResponseCheck(pv_List);
    appcmbs_ObjectSignal(&chRetCode, 1, 0, CMBS_EV_DSR_FW_UPD_PACKETNEXT_RES);
}

void           app_OnFwUpdEndRsp(void *pv_List)
{
    char chRetCode =  fwapp_ResponseCheck(pv_List);
    appcmbs_ObjectSignal(&chRetCode, 1, 0, CMBS_EV_DSR_FW_UPD_END_RES);
}

void   app_OnFWAppInvalidateRes(void *pv_List)
{
    char chRetCode =  fwapp_ResponseCheck(pv_List);
    appcmbs_ObjectSignal(&chRetCode, 1, 0, CMBS_EV_DSR_FW_APP_INVALIDATE_RES);
}

//  ========== app_SwupEntity ===========
/*!
        \brief   CMBS entity to handle response information from target side
        \param[in]  pv_AppRef   application reference
        \param[in]  e_EventID   received CMBS event
        \param[in]  pv_EventData  pointer to IE list
        \return    <int>

*/

int            app_SwupEntity(void *pv_AppRef, E_CMBS_EVENT_ID e_EventID, void *pv_EventData)
{
    // ensure that the compiler does not print out a warning
    if (pv_AppRef)
    {};

    switch (e_EventID)
    {
        case  CMBS_EV_DSR_FW_UPD_START_RES:
            app_OnFwUpdStartRsp(pv_EventData);
            break;

        case CMBS_EV_DSR_FW_UPD_PACKETNEXT_RES:
            app_OnFwUpdNextRsp(pv_EventData);
            break;

        case CMBS_EV_DSR_FW_UPD_END_RES:
            app_OnFwUpdEndRsp(pv_EventData);
            break;

        case CMBS_EV_DSR_FW_APP_INVALIDATE_RES:
            app_OnFWAppInvalidateRes(pv_EventData);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_SwupGetImageVersion(char *pFileName, u16 *u16_FwVersion)
{
    ST_FILE_HANDLER hFile = app_SwupOpenFile(pFileName);
    ST_IMAGE_HEADER stImageHeader;
    int nBytes;

    //init variables with dummy values
    *u16_FwVersion = APP_SWUP_EMPTY_VERSION;
    memset(&stImageHeader, 0xFF, sizeof(stImageHeader));

    if (hFile == ST_INVALID_FILE_HANDLER)
    {
        return CMBS_RC_ERROR_GENERAL;
    }
    app_SwupReadFile(hFile, (char *)&stImageHeader, sizeof(ST_IMAGE_HEADER), &nBytes);
    app_SwupCloseFile(hFile);

    *u16_FwVersion = ((stImageHeader.u8_Version[1] << 8) | stImageHeader.u8_Version[2]);
    return CMBS_RC_OK;
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC app_UpdateAndCheckVersion(const char *pch_FileName, u16 u16_FwVersion)
{
    ST_FILE_HANDLER   hFile  = 0;
    int      nBytes  = 0;
    char     chBuffer[APP_SWUP_MAX_PACKETSIZE];
    ST_APPCMBS_CONTAINER st_Container;
    ST_IE_FW_VERSION  stIe_FwVersion;
    ST_IMAGE_HEADER   stImageHeader;
    bool     isBooter;
    u8      u8_ComType;

    memset(&stImageHeader, 0xFF, sizeof(stImageHeader));

    // The bootloader doesn't respond to this event. Currently we assume we are not running SPI
    if ((g_CMBSInstance.u16_TargetVersion & 0xFF00) != 0x0000 &&     // Bootloader
            (g_CMBSInstance.u16_TargetVersion & 0xF000) != 0x2000 &&    // CMBS 2xxx versign (e.g. 2.99.9) - Old IE Header structure
            (g_CMBSInstance.u16_TargetVersion & 0xFF00) != 0x0200)       // CMBS 02xx versign (e.g. 2.99) - Old IE Header structure
    {
        u8_ComType = g_CMBSInstance.u8_HwComType;
    }
    else
    {
        // unable to determine HW type -> assuming SPI (do not wait for CMBS_EV_DSR_FW_UPD_END_RES)
        u8_ComType = CMBS_HW_COM_TYPE_SPI0;
    }
    // open FW image file
    hFile = app_SwupOpenFile(pch_FileName);
    if (hFile == ST_INVALID_FILE_HANDLER)
    {
        APPCMBS_ERROR(("Error: Unable to open file %s", pch_FileName));
        return CMBS_RC_ERROR_GENERAL;
    }
    //read image header
    app_SwupReadFile(hFile, chBuffer, sizeof(ST_IMAGE_HEADER), &nBytes);

    if (nBytes == 0)
    {
        APPCMBS_ERROR(("Error: Unable to read file\n"));
        app_SwupCloseFile(hFile);
        return CMBS_RC_ERROR_GENERAL;
    }

    memcpy(&stImageHeader, chBuffer, nBytes);

    isBooter = ((stImageHeader.u8_Reserved & APP_SWUP_BOOT_UPGRADE_MASK) == APP_SWUP_BOOT_UPGRADE_MASK); // marked as booter upgrade

    // send <start update event to target>
    APPCMBS_INFO(("FwUpdStart %d bytes\n", nBytes));
    appcmbs_PrepareRecvAdd(TRUE);
    cmbs_dsr_fw_UpdateStart(g_cmbsappl.pv_CMBSRef, (u8 *)chBuffer, nBytes);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_UPD_START_RES, &st_Container);

    if (CMBS_RC_OK != st_Container.ch_Info[0])
    {
        APPCMBS_ERROR(("Error: Target rejected FW UP request\nMake sure you have specified the correct BIN-file and there are no active calls\n"));
        app_SwupCloseFile(hFile);
        return (E_CMBS_RC)st_Container.ch_Info[0];
    };

    count = 0;

    // send rest of image file chunk by chunk
    do
    {
        app_SwupReadFile(hFile, chBuffer, g_stSwupSetup.u16_PacketSize, &nBytes);
        if (nBytes)
        {
            appcmbs_PrepareRecvAdd(TRUE);

            //printf("Host request = %d\n", count);
            cmbs_dsr_fw_UpdatePacketNext(g_cmbsappl.pv_CMBSRef, (u8 *)chBuffer, (u16)nBytes);
            appcmbs_WaitForContainer(CMBS_EV_DSR_FW_UPD_PACKETNEXT_RES, &st_Container);
            //printf("Host response = %d\n\n", count++);
        }
    } while ((st_Container.ch_Info[0] == CMBS_RC_OK) && (nBytes != 0));

    if (st_Container.ch_Info[0] == CMBS_RC_OK)
    {
        //send FW finish command
        appcmbs_PrepareRecvAdd(TRUE);
        cmbs_dsr_fw_UpdateEnd(g_cmbsappl.pv_CMBSRef, NULL, 0);
        // printf("\n isBooter= %d HwComType= %d\n", isBooter, u8_ComType );
        // Currently wait for END_RES has to be skipped in FWUP SPI case (Booter upgrade does support it).
        if (isBooter || ((u8_ComType != CMBS_HW_COM_TYPE_SPI0) && (u8_ComType != CMBS_HW_COM_TYPE_SPI3)))
        {
            E_CMBS_RC chRetCode;
            appcmbs_WaitForContainer(CMBS_EV_DSR_FW_UPD_END_RES, &st_Container);
            chRetCode = (E_CMBS_RC)st_Container.ch_Info[0];
            if (chRetCode != CMBS_RC_OK)
            {
                printf("\nERROR: Return Code %d. Exiting... \n", chRetCode);
                app_SwupCloseFile(hFile);
                return chRetCode;
            }
        }
    }
    else
    {
        printf("\nERROR. Exiting...");
        app_SwupCloseFile(hFile);
        return CMBS_RC_ERROR_GENERAL;
    }

    // wait until target reboots
    if (g_CMBSInstance.u8_HwComType == CMBS_HW_COM_TYPE_USB)
    {
        printf("\nReconnecting to target, please wait for %d seconds \n", APPCMBS_RECONNECT_TIMEOUT / 1000);
        if (appcmbs_ReconnectApplication(APPCMBS_RECONNECT_TIMEOUT))
        {
            app_SwupCloseFile(hFile);
            return CMBS_RC_ERROR_GENERAL;
        }
    }

    appcmbs_WaitForContainer(CMBS_EV_DSR_SYS_START_RES, NULL);

    //we successfully reconnected to target
    //ask for new version (for booter upgrade, skip this check)
    if (!isBooter)
    {
        appcmbs_PrepareRecvAdd(TRUE);
        cmbs_dsr_fw_VersionGet(g_cmbsappl.pv_CMBSRef, CMBS_MODULE_CMBS);
        appcmbs_WaitForContainer(CMBS_EV_DSR_FW_VERSION_GET_RES, &st_Container);
        memcpy(&stIe_FwVersion, st_Container.ch_Info, sizeof(stIe_FwVersion));

        //check if version is correct
        app_SwupCloseFile(hFile);
        return (u16_FwVersion == stIe_FwVersion.u16_FwVersion) ? CMBS_RC_OK : CMBS_RC_ERROR_PARAMETER;
    }
    else
    {
        app_SwupCloseFile(hFile);
        return CMBS_RC_OK;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////
// If isBooter is not set, we load the 2 application files alternatelly.
// If isBooter is set -- the cycle is:
//  booter1, application 1, check version1, booter2, application 2, check version2, etc.
void        app_SwupStartStressTests(bool isBooter)
{
    const char *pchFile;
    const char *pchBooterFile;
    u8    u8_ParserEnabled = app_get_msgparserEnabled();
    int    i = 0, nNumChars = 0;
    E_SWUP_STAGES eStage   = FW_UPGRADE_IDLE;
    FILE *fp_LogFile  = NULL;
    char   ch_LogMsg[256];
    unsigned long startTime  = appcmbs_GetTickCount(), currentTime = startTime;
    E_CMBS_RC  eResp   = CMBS_RC_OK;
    u16    u16_version;
    u16    u16_Booter_version;

    fp_LogFile  = fopen(g_stSwupSetup.logFileName, "w");

    // SW update packages should not be parsed
    app_set_msgparserEnabled(FALSE);

    while ((((currentTime - startTime) < (unsigned long)1000 * 60 * g_stSwupSetup.u16_DurationMinutes)) &&
            (CMBS_RC_OK == eResp))
    {
        // image and version for another
        if (eStage == FW_UPGRADE_FILE_1)
        {
            pchFile  = g_stSwupSetup.file_name2;
            u16_version = g_stSwupSetup.u16_Version2;
            eStage  = FW_UPGRADE_FILE_2;
            // fill also booter upgrade parameters, in case it is needed:
            pchBooterFile  = g_stSwupSetup.booter_name2;
            u16_Booter_version = g_stSwupSetup.u16_BooterVersion2;
        }
        else
        {
            pchFile  = g_stSwupSetup.file_name1;
            u16_version = g_stSwupSetup.u16_Version1;
            eStage  = FW_UPGRADE_FILE_1;
            // fill also booter upgrade parameters, in case it is needed:
            pchBooterFile  = g_stSwupSetup.booter_name1;
            u16_Booter_version = g_stSwupSetup.u16_BooterVersion1;
        }

        //print update starts to log file
        memset(ch_LogMsg, 0x0, sizeof(ch_LogMsg));
        nNumChars = sprintf(ch_LogMsg, "Staring iteration %d , version %x ... ", i, u16_version);
        if (fp_LogFile)
        {
            fwrite(ch_LogMsg, 1, nNumChars, fp_LogFile);
            fflush(fp_LogFile);
        }

        // run FW update

        if (isBooter)     // before application load also one booter and record result to the log
        {
            eResp = app_UpdateAndCheckVersion(pchBooterFile, u16_Booter_version);
            // print update results
            memset(ch_LogMsg, 0x0, sizeof(ch_LogMsg));
            if (CMBS_RC_OK == eResp)
            {
                nNumChars = sprintf(ch_LogMsg, "successfully booter done \n");
            }
            else
            {
                nNumChars = sprintf(ch_LogMsg, "error booter, reason = %d \n", eResp);
            }

            if (fp_LogFile)
            {
                fwrite(ch_LogMsg, 1, nNumChars, fp_LogFile);
                fflush(fp_LogFile);
            }
        }
        // always -- load the application
        eResp = app_UpdateAndCheckVersion(pchFile, u16_version);

        // print update results
        memset(ch_LogMsg, 0x0, sizeof(ch_LogMsg));
        if (CMBS_RC_OK == eResp)
        {
            nNumChars = sprintf(ch_LogMsg, "successfully done \n");
        }
        else
        {
            nNumChars = sprintf(ch_LogMsg, "error, reason = %d \n", eResp);
        }

        if (fp_LogFile)
        {
            fwrite(ch_LogMsg, 1, nNumChars, fp_LogFile);
            fflush(fp_LogFile);
        }

        currentTime = appcmbs_GetTickCount();
        i++;
    }

    if (fp_LogFile)
    {
        fclose(fp_LogFile);
    }

    //restore parser settings
    app_set_msgparserEnabled(u8_ParserEnabled);
}

void app_SwupApplicationInvalidate(void)
{
    ST_APPCMBS_CONTAINER st_Container;

    appcmbs_PrepareRecvAdd(TRUE);

    cmbs_dsr_fw_AppInvalidate(g_cmbsappl.pv_CMBSRef);
    appcmbs_WaitForContainer(CMBS_EV_DSR_FW_APP_INVALIDATE_RES, &st_Container);
}
//*/
