/*!
*   \file       tcxutil.c
*   \brief
*   \author     DanaKronfeld
*
*   @(#)        tcxutil.c~1
*
*******************************************************************************/
#ifdef WIN32
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "cmbs_api.h"
#include <tcx_util.h>
#include <tcx_cmbs.h>


extern ST_UART_CONFIG g_st_UartCfg;
extern ST_CMBS_DEV    g_st_DevCtl;
extern ST_CMBS_DEV    g_st_DevMedia;
extern ST_TDM_CONFIG  g_st_TdmCfg;

#define BUF_SIZE 256

void tcx_USBConfig(u8 u8_Port)
{
    // setup port and uart for control plane
    g_st_UartCfg.u8_Port = u8_Port;
    g_st_DevCtl.e_DevType = CMBS_DEVTYPE_USB;
    g_st_DevCtl.u_Config.pUartCfg = &g_st_UartCfg;

    g_st_DevMedia.e_DevType         = CMBS_DEVTYPE_USB;
    g_st_DevMedia.u_Config.pTdmCfg  = NULL;
}


u8 tcx_DetectComPort(bool interactiveMode, E_CMBS_DEVTYPE * pe_type)
{
#ifdef WIN32

    const TCHAR     szDongleHardwareId[] = TEXT(CMBS_USB_DONGLE_HARDWARE_ID);
    HINSTANCE       hSetupAPI;
    GUID            guidPortsDevClass;
    DWORD           dwItemsNum;
    DWORD           dwDeviceNumber;
    HDEVINFO        hDeviceInfo;
    SP_DEVINFO_DATA devInfoData;
    DWORD           regDataType;
    TCHAR           regData[300];
    TCHAR           descData[300];
    TCHAR           portData[300];
    HKEY            hDeviceKey;
    TCHAR           t;
    int             iPortNumber = 0;
    int             iUsbDonglePortNumber = 0;

    typedef BOOL (__stdcall * LPSETUPDICLASSGUIDSFROMNAME)(LPCTSTR, LPGUID, DWORD, PDWORD);
    typedef HDEVINFO(__stdcall * LPSETUPDIGETCLASSDEVS)(LPGUID, LPCTSTR, HWND, DWORD);
    typedef BOOL (__stdcall * LPSETUPDIGETDEVICEREGISTRYPROPERTY)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
    typedef BOOL (__stdcall * LPSETUPDIENUMDEVICEINFO)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
    typedef HKEY(__stdcall * LPSETUPDIOPENDEVREGKEY)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);
    typedef BOOL (__stdcall * LPSETUPDIDESTROYDEVICEINFOLIST)(HDEVINFO);

    LPSETUPDICLASSGUIDSFROMNAME         lpfnSetupDiClassGuidsFromName           = NULL;
    LPSETUPDIGETCLASSDEVS               lpfnSetupDiGetClassDevs                 = NULL;
    LPSETUPDIGETDEVICEREGISTRYPROPERTY  lpfnSetupDiGetDeviceRegistryProperty    = NULL;
    LPSETUPDIENUMDEVICEINFO             lpfnSetupDiEnumDeviceInfo               = NULL;
    LPSETUPDIOPENDEVREGKEY              lpfnSetupDiOpenDevRegKey                = NULL;
    LPSETUPDIDESTROYDEVICEINFOLIST      lpfnSetupDiDestroyDeviceInfoList        = NULL;

    *pe_type = CMBS_DEVTYPE_MAX; // no transport type assigned yet

    hSetupAPI = LoadLibrary(TEXT("SETUPAPI.DLL"));
    if (hSetupAPI == NULL)
    {
        return 0;
    }

#ifdef _UNICODE
    lpfnSetupDiClassGuidsFromName           = (LPSETUPDICLASSGUIDSFROMNAME)         GetProcAddress(hSetupAPI, "SetupDiClassGuidsFromNameW");
    lpfnSetupDiGetClassDevs                 = (LPSETUPDIGETCLASSDEVS)               GetProcAddress(hSetupAPI, "SetupDiGetClassDevsW");
    lpfnSetupDiGetDeviceRegistryProperty    = (LPSETUPDIGETDEVICEREGISTRYPROPERTY)  GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyW");
#else
    lpfnSetupDiClassGuidsFromName           = (LPSETUPDICLASSGUIDSFROMNAME)         GetProcAddress(hSetupAPI, "SetupDiClassGuidsFromNameA");
    lpfnSetupDiGetClassDevs                 = (LPSETUPDIGETCLASSDEVS)               GetProcAddress(hSetupAPI, "SetupDiGetClassDevsA");
    lpfnSetupDiGetDeviceRegistryProperty    = (LPSETUPDIGETDEVICEREGISTRYPROPERTY)  GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyA");
#endif
    lpfnSetupDiEnumDeviceInfo               = (LPSETUPDIENUMDEVICEINFO)             GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInfo");
    lpfnSetupDiOpenDevRegKey                = (LPSETUPDIOPENDEVREGKEY)              GetProcAddress(hSetupAPI, "SetupDiOpenDevRegKey");
    lpfnSetupDiDestroyDeviceInfoList        = (LPSETUPDIDESTROYDEVICEINFOLIST)      GetProcAddress(hSetupAPI, "SetupDiDestroyDeviceInfoList");

    if ((lpfnSetupDiClassGuidsFromName == NULL) || (lpfnSetupDiGetClassDevs == NULL) || (lpfnSetupDiGetDeviceRegistryProperty == NULL) ||
            (lpfnSetupDiEnumDeviceInfo == NULL) || (lpfnSetupDiOpenDevRegKey == NULL) || (lpfnSetupDiDestroyDeviceInfoList == NULL))
    {
        FreeLibrary(hSetupAPI);

        return 0;
    }

    if (!lpfnSetupDiClassGuidsFromName(TEXT("Ports"), &guidPortsDevClass, 1, &dwItemsNum) ||
            dwItemsNum == 0)
    {
        FreeLibrary(hSetupAPI);

        return 0;
    }

    hDeviceInfo = lpfnSetupDiGetClassDevs(&guidPortsDevClass, NULL, NULL, DIGCF_PRESENT);
    if (hDeviceInfo == INVALID_HANDLE_VALUE)
    {
        FreeLibrary(hSetupAPI);

        return 0;
    }

    if (interactiveMode)
    {
        printf("|----------------------------------|\n");
        printf("|  Available COM ports in system:  |\n");
        printf("|----------------------------------|\n");
    }

    for (dwDeviceNumber = 0; dwDeviceNumber < 65536 ; ++dwDeviceNumber)
    {
        ZeroMemory(&devInfoData, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(devInfoData);

        if (!lpfnSetupDiEnumDeviceInfo(hDeviceInfo, dwDeviceNumber, &devInfoData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
            {
                break;
            }
            else
            {
                continue;
            }
        }

        if (!lpfnSetupDiGetDeviceRegistryProperty(hDeviceInfo, &devInfoData, SPDRP_HARDWAREID, &regDataType, (LPBYTE)regData, sizeof(regData), NULL) ||
                (regDataType != REG_SZ && regDataType != REG_MULTI_SZ))
        {
            continue;
        }

        if (!lpfnSetupDiGetDeviceRegistryProperty(hDeviceInfo, &devInfoData, SPDRP_DEVICEDESC, &regDataType, (LPBYTE)descData, sizeof(descData), NULL) ||
                (regDataType != REG_SZ && regDataType != REG_MULTI_SZ))
        {
            continue;
        }

        hDeviceKey = lpfnSetupDiOpenDevRegKey(hDeviceInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
        if (hDeviceKey == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        dwItemsNum = sizeof(portData);
        if (FAILED(RegQueryValueEx(hDeviceKey, TEXT("PortName"), NULL, &regDataType, (LPBYTE)portData, &dwItemsNum)) ||
                (regDataType != REG_SZ))
        {
            RegCloseKey(hDeviceKey);

            continue;
        }

        RegCloseKey(hDeviceKey);

        portData[sizeof(portData) / sizeof(portData[0]) - 1] = TEXT('\0');
        if (_stscanf(portData, TEXT("COM%d%c"), &iPortNumber, &t) == 1 &&
                iPortNumber > 0 && iPortNumber < 256)
        {
            if (interactiveMode)
                printf("| COM%d: %s\n", iPortNumber, descData);

            if (strstr(szDongleHardwareId, regData) != 0)
            {
                iUsbDonglePortNumber = iPortNumber;
            }
        }
    }

    lpfnSetupDiDestroyDeviceInfoList(hDeviceInfo);

    FreeLibrary(hSetupAPI);

    if (iUsbDonglePortNumber > 0)
    {
        if (interactiveMode)
        {
            printf("Auto detected USB Dongle on COM%d\n", iUsbDonglePortNumber);
        }
        *pe_type = CMBS_DEVTYPE_USB;
        return iUsbDonglePortNumber;
    }

    *pe_type = CMBS_DEVTYPE_UART; // for windows return UART
    if (interactiveMode)
    {
        char InputBuffer[20] = {0};
        printf("|\n");
        printf("| Enter COM port number: ");

        memset(InputBuffer, 0, sizeof(InputBuffer));
        tcx_gets(InputBuffer, sizeof(InputBuffer));
        *pe_type = CMBS_DEVTYPE_UART;
        return (u8)atoi(InputBuffer);
    }
    else
    {
        return 0;
    }
#else
    UNUSED_PARAMETER(interactiveMode);
    // For Linux type is still unknown
    *pe_type = CMBS_DEVTYPE_MAX;
    return 0;
#endif // WIN32
}

#if defined ( __linux__ )

//  ========== tcx_getch  ===========
/*!
        \brief     re-implementation of getch function to get the right
                         behaviour under linux
        \param[in,out]   <none>
        \return     <int> return value of stdin entering

*/

int tcx_getch(void)
{
    int ch;
    struct termios old_t, tmp_t;

    // save old terminal settings
    if (tcgetattr(STDIN_FILENO, &old_t))
    {
        // return error if no stdin
        // terminal is available
        return -1;

    }

    // take old terminal settings
    // and adapt to needed onces
    memcpy(&tmp_t, &old_t, sizeof(old_t));
    // de-activate echo and switch of
    // line mode
    tmp_t.c_lflag &= ~ICANON & ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios *)&tmp_t))
    {
        return -1;
    }
    // read character and hope that
    // STDIN_FILENO handles blocked condition
    do {
        ch = getchar();
    } while (ch == EOF);
    // restore old terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios *)&old_t);

    return ch;
}

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK) == (-1))
    {
        return 0;
    }

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    if (fcntl(STDIN_FILENO, F_SETFL, oldf) == (-1))
    {
        return 0;
    }

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

#else

int tcx_getch(void)
{
    u8 value = _getch();
    printf("%c", value);
    return value;
}

int tcx_kbhit(void)
{
    return _kbhit();
}

#endif

//  ========== tcx_appClearScreen  ===========
/*!
        \brief    clear command line screen due to asc ii escape sequence
        \param[in,out]  <none>
        \return    <none>

*/

void  tcx_appClearScreen(void)
{
#if defined ( __linux__ )
    char esc = 27;
    printf("%c%s", esc, "[2J");
    printf("%c%s", esc, "[1;1H");
#elif defined ( WIN32 )
    system("cls");
#endif
}

int tcx_gets(char * buffer, int n_Length)
{
    do
    {
        fgets(buffer, n_Length, stdin);
        // remove also carriage return
        buffer[strlen(buffer) - 1] = 0;
    }
    while (buffer[0] == '\0');

    return 0;
}

void tcx_UARTConfig(u8 u8_Port)
{
    // setup port and uart for control plane
    g_st_UartCfg.u8_Port = u8_Port;
    g_st_DevCtl.e_DevType = CMBS_DEVTYPE_UART;
    g_st_DevCtl.u_Config.pUartCfg = &g_st_UartCfg;
#ifdef CMBS_UART_HW_FLOW_CTRL
    g_st_DevMedia.e_FlowCTRL = CMBS_FLOW_CTRL_RTS_CTS;
#else
    g_st_DevMedia.e_FlowCTRL = CMBS_FLOW_CTRL_NONE;
#endif //CMBS_UART_HW_FLOW_CTRL
}


#ifdef CMBS_COMA
void tcx_COMAConfig(void)
{
    g_st_DevCtl.e_DevType = CMBS_DEVTYPE_COMA;
}
#endif // CMBS_COMA

void tcx_TDMConfig(bool TDM_Type)
{
    // setup TDM interface for payload plane
    if (TDM_Type)
    {
        g_st_TdmCfg.e_Type         = CMBS_TDM_TYPE_MASTER;
    }
    else //default
    {
        g_st_TdmCfg.e_Type         = CMBS_TDM_TYPE_SLAVE;
    }

    g_st_TdmCfg.e_Speed        = CMBS_TDM_PCM_2048;
    //g_st_TdmCfg.e_Sync         = CMBS_TDM_SYNC_LONG;
    //g_st_TdmCfg.e_Sync         = CMBS_TDM_SYNC_SHORT_FR;
    //g_st_TdmCfg.e_Sync         = CMBS_TDM_SYNC_SHORT_FF;
    g_st_TdmCfg.e_Sync         = CMBS_TDM_SYNC_SHORT_LF;
    g_st_TdmCfg.u16_SlotEnable = 0xFFFF;

    g_st_DevMedia.e_DevType         = CMBS_DEVTYPE_TDM;
    g_st_DevMedia.u_Config.pTdmCfg  = &g_st_TdmCfg;
}

//    ========== tcx_fileOpen ===========
/*!
      \brief             Open the file according to the specified mode
   \param[out]        pf_file is pointer to a file pointer
   \param[in]         pu8_fileName The path to the file
   \param[in]         pu8_mode mode for opening (ie: "r+b" for reading a binary file)
      \return            CMBS_RC_OK on success, otherwise CMBS_RC_ERROR_GENERAL
*/
E_CMBS_RC tcx_fileOpen(FILE **pf_file, u8 *pu8_fileName, const u8 *pu8_mode) {

    *pf_file = fopen(pu8_fileName, pu8_mode);

    if (*pf_file == NULL)
    {
        return CMBS_RC_ERROR_GENERAL;
    }
    return CMBS_RC_OK;
}


//    ========== tcx_fileRead ===========
/*!
      \brief             Read data from file
   \param[out]        pu8_OutBuf      Pointer to output data buffer
   \param[in]         pf_file         Pointet to a file
      \param[in]         u32_Offset      Offset in the EEPROM file
      \param[in]         u32_Size        Length of the data
      \return            CMBS_RC_OK on success, otherwise CMBS_RC_ERROR_GENERAL

*/
E_CMBS_RC tcx_fileRead(FILE *pf_file, u8 *pu8_OutBuf, u32 u32_Offset, u32 u32_Size) {

    size_t  res = 0;

    if (pf_file != NULL)
    {
        if (fseek(pf_file, u32_Offset, SEEK_SET) != (-1))
        {
            res = fread(pu8_OutBuf, 1, u32_Size, pf_file);
        }
        else
        {
            printf("\n Error during tcx_fileWrite ");
            return CMBS_RC_ERROR_GENERAL;
        }
    }

    if (res != u32_Size)
    {
        printf("ERROR: Can't read file\n");
        return CMBS_RC_ERROR_GENERAL;
    }

    return CMBS_RC_OK;
}

//    ========== tcx_fileWrite ===========
/*!
      \brief             Write data to file
      \param[out]        pu8_InBuf       Pointer to input data buffer
   \param[in]         pf_file to      Pointer to a file
      \param[in]         u32_Offset      Offset in the EEPROM file
      \param[in]         u32_Size        Length of the data
      \return            CMBS_RC_OK on success, otherwise CMBS_RC_ERROR_GENERAL

*/
E_CMBS_RC tcx_fileWrite(FILE *pf_file, u8* pu8_InBuf, u32 u32_Offset, u32 u32_Size) {

    size_t  res = 0;

    if (pf_file != NULL)
    {

        if (fseek(pf_file, u32_Offset, SEEK_SET) != (-1))
        {
            res = fwrite(pu8_InBuf, 1, u32_Size, pf_file);
            fflush(pf_file);
        }
        else
        {
            printf("\n Error during tcx_fileWrite ");
            return CMBS_RC_ERROR_GENERAL;
        }
    }

    if (res != u32_Size)
    {
        printf("ERROR: Can't write file\n");
        return CMBS_RC_ERROR_GENERAL;
    }

    return CMBS_RC_OK;
}

//    ========== tcx_fileClose ===========
/*!
      \brief             Close the file
   \param[in]         pf_file to      Pointer to a file
      \return            CMBS_RC_OK

*/
E_CMBS_RC tcx_fileClose(FILE *pf_file) {

    if (pf_file != NULL)
    {
        fclose(pf_file);
    }

    return CMBS_RC_OK;
}

//    ========== tcx_fileDelete ===========
/*!
      \brief             Delete the file
   \param[in]         pFileName      Name of the file need to be deleted
      \return            CMBS_RC_OK

*/
E_CMBS_RC tcx_fileDelete(char *pFileName)
{
    int RetVal = 0;

    RetVal = remove(pFileName);
    if (RetVal == 0)
        return CMBS_RC_OK;
    else
    {
        printf("ERROR - can not delete file RetVal = %d\n", RetVal);
        return CMBS_RC_ERROR_GENERAL;
    }
}
//    ========== tcx_fileSize ===========
/*!
      \brief             Return size of the  file
   \param[in]         pf_file to      Pointer to a file
      \return            Size of the file, -1 on error

*/
u32 tcx_fileSize(FILE *pf_file) {

    s32 size = 0;
    u32 u32_ReturnVal;

    if (pf_file != NULL)
    {
        fseek(pf_file, 0, SEEK_END);
        size = ftell(pf_file);
    }
    if (size >= 0)
        u32_ReturnVal = (u32)size;
    else
        u32_ReturnVal = (u32) - 1;

    return u32_ReturnVal;
}

#if defined(_CONSOLE)

#undef scanf
int scanf(const char *format, ...)
{
    UNUSED_PARAMETER(format);
    printf("\n\n PLEAE DO NOT USE scanf(). Use tcx_scanf instead !!\n\n");
    return -1;
}


#ifdef WIN32
int vsscanf(const char *s, const char *fmt, va_list ap)
{   //wrapper for vsscanf in WIN32 since vsscanf is not avaiable in Microsoft VS2008
    void *a[20];
    int i;
    for (i = 0; i < sizeof(a) / sizeof(a[0]); i++) a[i] = va_arg(ap, void *);
    return sscanf(s, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]);
}
#endif

E_CMBS_RC tcx_scanf(const char *format, ...)
{
    int rc;
    char buf[BUF_SIZE] = {0};
    va_list args;

    tcx_gets(buf, sizeof(buf));

    va_start(args, format);
    rc = vsscanf(buf, format, args);
    va_end(args);

    if (rc > 0)
    {
        return CMBS_RC_OK;
    }
    else
    {
        return CMBS_RC_ERROR_GENERAL;
    }
}
#endif

//--------[End of file]---------------------------------------------------------------------------------------------------------------------------------
