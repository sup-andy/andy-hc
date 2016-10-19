
/*
   CPhysicalPort.cpp


   Implementation of class CPhysicalPort

   Methods to configure, write to and read from physical com port


*/

//#include  <StdAfx.h>
//#include <afxwin.h>         // MFC-Kern- und -Standardkomponenten
#include <stdio.h>
#include "cmbs_platf.h"
#include "cfr_debug.h"  /* debug handling */

#include    <Mmsystem.h>
#include   "cphysicalport.h"


#define TARGET_RESOLUTION 1
#define TRACE //CFR_DBG_OUT

BOOL    CPhysicalPort::openPort(LPSTR lpstr_Port)
{
    CFR_DBG_OUT("Open Port: %s\n", lpstr_Port);

    if (strlen(lpstr_Port) < sizeof(m_chPort))
    {
        strcpy(m_chPort, lpstr_Port);
    }

    return open();
}

/*                ========== CPhysicalPort::open ==========

   Opens the COM Port with the specified parameters

   Args:

     MS_RLSD_ON

   Return: TRUE if the port was successfully opened, FALSE if it failed

*/

BOOL  CPhysicalPort::open(void)
{
    BOOL     fRetVal;
    HANDLE   hCommWatchThread;
    DWORD    dwThreadID;

    m_hPort = NULL;
    if ((m_hPort = CreateFileA(m_port, GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL |
                               FILE_FLAG_OVERLAPPED,
                               NULL)) == INVALID_HANDLE_VALUE)
    {
        DWORD dw_Error;
        dw_Error = GetLastError();
        CFR_DBG_ERROR("Can't open %s => error %d %x\n", m_port, dw_Error, dw_Error);

        return FALSE;
    }
    else
    {
        BOOL Ret;
        CFR_DBG_OUT("%s opened\n", m_port);
        // get any early notifications

        fRetVal = SetCommMask(m_hPort, EV_RXCHAR);

        // setup device buffers

        fRetVal = SetupComm(m_hPort, 4096, 4096); // 8192

        // purge any information in the buffer

        fRetVal = PurgeComm(m_hPort, PURGE_TXABORT | PURGE_RXABORT |
                            PURGE_TXCLEAR | PURGE_RXCLEAR);

        // set up for overlapped I/O

        // No read timeout, return whatever might be in buffer
        m_ComTimeOuts.ReadIntervalTimeout         = MAXDWORD;
        m_ComTimeOuts.ReadTotalTimeoutMultiplier  = 0;
        m_ComTimeOuts.ReadTotalTimeoutConstant    = 0;  //1000 ;
        m_ComTimeOuts.WriteTotalTimeoutMultiplier = 0;
        m_ComTimeOuts.WriteTotalTimeoutConstant   = 4000;
        Ret = SetCommTimeouts(m_hPort, &m_ComTimeOuts);
        if (!Ret)
        {
            CFR_DBG_ERROR("SetCommTimeouts has returned Error: 0x%04x", GetLastError());
        }
    }  // else

    // set flow control to hardware flow control RTS/CTS
    //m_bFlowCtrl           = FC_RTSCTS;

    m_PortCfg.BaudRate   = CBR_115200;
    m_PortCfg.ByteSize   = 8;
    m_PortCfg.Parity     = NOPARITY;
    m_PortCfg.StopBits   = ONESTOPBIT;

    fRetVal = setConfig(m_PortCfg);


    if (fRetVal)
    {
        m_fPortConnected = TRUE;

        // Create a secondary thread
        // to watch for an event.

        if (NULL == (hCommWatchThread =
                         CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                      0,
                                      (LPTHREAD_START_ROUTINE)ThreadEntry,
                                      (LPVOID)this,   // this
                                      0, &dwThreadID)))

        {
            m_fPortConnected = FALSE;
            CloseHandle(m_hPort);
            fRetVal = FALSE;
            CFR_DBG_OUT("COMPort FALSE1\n");
        }
        else
        {
            m_dwThreadID   = dwThreadID;
            m_hWatchThread = hCommWatchThread;

            // assert DTR

            // EscapeCommFunction( m_hPort, SETDTR ) ;
        }
    }  // if (fRetVal)
    else
    {
        m_fPortConnected = FALSE;
        CloseHandle(m_hPort);
        CFR_DBG_OUT("COMPort FALSE2\n");
    }

    return (fRetVal);

}  // open



//************************************************************************
//  DWORD CommWatchProc( LPSTR lpData )
//
//  Description:
//     A secondary thread that will watch for COMM events.
//
//  Parameters:
//     LPSTR lpData
//        32-bit pointer argument
//
//  Win-32 Porting Issues:
//     - Added this thread to watch the communications device and
//       post notifications to the associated window.
//
//************************************************************************

DWORD CPhysicalPort::CommWatchProc(void)  //  PVOID pReference
{
    DWORD          dwEvtMask;
    OVERLAPPED     os;
    DWORD          dwModemState;
    UINT           mmTimerID;
    TIMECAPS       tc;
    UINT           wTimerRes;

    TRACE("CommWatchProc \n");

    memset(&os, 0, sizeof(OVERLAPPED));

    // create I/O event used for overlapped read

    os.hEvent = CreateEvent(NULL,    // no security
                            TRUE,    // explicit reset req
                            FALSE,   // initial event reset
                            NULL); // no name
    if (os.hEvent == NULL)
    {
        CFR_DBG_OUT("COMPort FALSE3\n");
        return (FALSE);

    }

    if (!SetCommMask(m_hPort, EV_RXCHAR | EV_CTS | EV_DSR | EV_RING | EV_RLSD))
    {
        CFR_DBG_OUT("COMPort FALSE4\n");
        return (FALSE);
    }
    GetCommModemStatus(m_hPort, &dwModemState);

    TRACE("CommState %x\n", dwModemState);

    setModemState(dwModemState);



    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
    {
        // Error; application can't continue.
    }

    wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
    timeBeginPeriod(wTimerRes);

    mmTimerID = timeSetEvent(
                    30,
                    wTimerRes,
                    (LPTIMECALLBACK)TimeProc,
                    (DWORD)this,
                    TIME_PERIODIC
                );

    while (m_fPortConnected)
    {
        dwEvtMask = 0;

        WaitCommEvent(m_hPort, &dwEvtMask, NULL);

        TRACE("EVENT: %x\n", dwEvtMask);

        if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR)
        {
            //OutputDebugString("CPhysicalPort::Read Character \n");


            if (m_pfnDataReceived != NULL)
                m_pfnDataReceived(NULL);
            TRACE("Received Data\n");
            //do
            //{

            // if (nLength = read( (LPSTR) abIn, 1 ))
            // {
            // Write received data to DAPI
            //    TRACE( "%02x ",*abIn );

            // }
            //}
            //while ( nLength > 0 ) ;

        }
        //else
        if ((dwEvtMask & EV_BREAK) == EV_BREAK)
        {
            TRACE("Break\n");

        }
        //else
        if ((dwEvtMask & EV_CTS) == EV_CTS)         // Clear To Send
        {
            //       m_MsrBits |= MSR_DCTS;
            // m_MsrBits ^=MSR_CTS;
            //         printf("CommWatchProc: CTS changed\n");
            TRACE("CTS chan\n");

        }
        //else
        if ((dwEvtMask & EV_DSR) == EV_DSR)         // Data Set Ready
        {
            //    m_MsrBits |= MSR_DDSR;
            //    m_MsrBits ^=MSR_DSR;
            //         printf("CommWatchProc: DSR changed\n");
            TRACE("DSR chan\n");
        }
        //else
        if ((dwEvtMask & EV_RING) == EV_RING)       // Ring Indicator
        {
            //    m_MsrBits |= MSR_DRI;
            //       m_MsrBits ^=MSR_RI;
            //         printf("CommWatchProc: RI changed\n");
            TRACE("RIN chan\n");
        }
        //else
        if ((dwEvtMask & EV_ERR) == EV_ERR)
        {
            TRACE("ERR chan\n");
        }
        //else
        if ((dwEvtMask & EV_RLSD) == EV_RLSD)       // DCD   -  Data Carrier Detect
        {
            //       m_MsrBits |= MSR_DDCD;
            //       m_MsrBits ^= MSR_DCD;
            //         printf("CommWatchProc: DCD changed\n");
        }
        //else
        if ((dwEvtMask & EV_RXFLAG) == EV_RXFLAG)
        {
            TRACE("RX chan\n");


        }
        //else
        if ((dwEvtMask & EV_TXEMPTY) == EV_TXEMPTY)
        {
            TRACE("TX EMPTY chan\n");
        }

        // Send MSR changes to client
        //    if (m_MsrBits & (MSR_DDCD | MSR_DRI | MSR_DDSR | MSR_DCTS))
        //          m_DapiServer->WriteDapi((PVOID)NULL,0);
    }  // while (m_fPortConnected)

    // get rid of event handle
    TRACE("OUT COMPORT\n");

    CloseHandle(os.hEvent);


    if (mmTimerID)                  // is timer event pending?
    {
        timeKillEvent(mmTimerID);  // cancel the event
        mmTimerID = 0;
        timeEndPeriod(wTimerRes);
    }

    // clear information in structure (kind of a "we're done flag")

    m_dwThreadID   = 0;
    m_hWatchThread = NULL;


    //   printf("CommWatchProc finished!\n");
    CFR_DBG_OUT("CPhysicalPort::Finish\n");

    return (TRUE);

} // end of CommWatchProc()



/*                ========== CPhysicalPort::TimerProc ==========

   Function that is called if the timer has elapsed

   Args:


   Return: none

*/

void CPhysicalPort::TimerProc(void)
{
    // int            nLength ;
    // BYTE           abIn[ MAXBLOCK + 1];

}  // TimerProc


/*                ========== CPhysicalPort::close ==========

   Closes the com port

   Args:


   Return: TRUE if the function succeeds, false if it failed

*/
BOOL  CPhysicalPort::close(void)
{
    m_fPortConnected = FALSE;

    // disable event notification and wait for thread to halt

    SetCommMask(m_hPort, 0);

    while (m_dwThreadID != 0);

    //EscapeCommFunction(m_hPort,CLRDTR);

    PurgeComm(m_hPort, PURGE_TXABORT | PURGE_RXABORT |
              PURGE_TXCLEAR | PURGE_RXCLEAR);

    CFR_DBG_OUT("CPhysicalPort::close\n");

    return CloseHandle(m_hPort);
}  // close

/*                ========== CPhysicalPort::write ==========

   Writes a block of data to the COM port specified in the associated
   structure.
   Writes the specified data into the comport

   Args:


   Return: TRUE if the function succeeds, false if it failed

*/

BOOL CPhysicalPort::write(LPSTR lpByte, DWORD dwBytesToWrite)
{

    BOOL        fWriteStat;
    DWORD       dwBytesWritten;
    DWORD       dwErrorFlags;
    //DWORD       dwError;
    COMSTAT     ComStat;
    char        szError[10];
    OVERLAPPED  st_Overlap = { 0 };

    st_Overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    //   OutputDebugString("CPhysicalPort::Write\n");


    fWriteStat = WriteFile(m_hPort, lpByte, dwBytesToWrite,
                           &dwBytesWritten, &st_Overlap);

    // Note that normally the code will not execute the following
    // because the driver caches write operations. Small I/O requests
    // (up to several thousand bytes) will normally be accepted
    // immediately and WriteFile will return true even though an
    // overlapped operation was specified

    if (!fWriteStat)
    {
        //      OutputDebugString("CPhysicalPort::Write not completed\n");

        if (GetLastError() == ERROR_IO_PENDING)
        {
            BOOL bo_RET;
            int  y = 0;
            //char  bBuf[50];

            //      OutputDebugString("CPhysicalPort::IO_PENDING Issue\n");
            // We should wait for the completion of the write operation
            // so we know if it worked or not

            // This is only one way to do this. It might be beneficial to
            // the to place the writing operation in a separate thread
            // so that blocking on completion will not negatively
            // affect the responsiveness of the UI

            // If the write takes long enough to complete, this
            // function will timeout according to the
            // CommTimeOuts.WriteTotalTimeoutConstant variable.
            // At that time we can check for errors and then wait
            // some more.
            bo_RET = GetOverlappedResult(m_hPort, &st_Overlap, &dwBytesWritten, TRUE);
            //sprintf( bBuf, "Write ret %d\n", bo_RET );
            //OutputDebugString((char*)bBuf);
        }
        else
        {
            // some other error occurred

            ClearCommError(m_hPort, &dwErrorFlags, &ComStat);
            if ((dwErrorFlags > 0) && m_fDisplayErrors)
            {
                wsprintfA(szError, "<CE-%u>", dwErrorFlags);
            }
            CloseHandle(st_Overlap.hEvent);

            return (FALSE);
        }
    }

    CloseHandle(st_Overlap.hEvent);

    return (TRUE);

} // end of write()

/*                ========== CPhysicalPort::read ==========

     Reads a block from the COM port and stuffs it into
     the provided buffer.


   Args: pUserData   Pointer to the buffer where the data is stored

         nMaxLength  max length of block to read

   Return: TRUE if the function succeeds, false if it failed

*/

int CPhysicalPort::read(LPSTR lpszBlock, int nMaxLength)
{
    BOOL       fReadStat;
    COMSTAT    ComStat;
    DWORD      dwErrorFlags;
    DWORD      dwLength = 0;
    DWORD      dwError;
    char       szError[10];
    OVERLAPPED  st_Overlap = { 0 };

    st_Overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // only try to read number of bytes in queue
    ClearCommError(m_hPort, &dwErrorFlags, &ComStat);
    dwLength = min((DWORD)nMaxLength, ComStat.cbInQue);

    if (dwLength > 0)
    {
        fReadStat = ReadFile(m_hPort, lpszBlock,
                             dwLength, &dwLength, &st_Overlap);
        if (!fReadStat)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                //            OutputDebugString("\n\rIO Pending");
                // We have to wait for read to complete.
                // This function will timeout according to the
                // CommTimeOuts.ReadTotalTimeoutConstant variable
                // Every time it times out, check for port errors
                while (!GetOverlappedResult(m_hPort,
                                            &st_Overlap, &dwLength, TRUE))
                {
                    dwError = GetLastError();
                    if (dwError == ERROR_IO_INCOMPLETE)
                        // normal result if not finished
                        continue;
                    else
                    {
                        // an error occurred, try to recover
                        wsprintfA(szError, "<CE-%u>", dwError);
                        // error output: WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
                        ClearCommError(m_hPort, &dwErrorFlags, &ComStat);
                        if ((dwErrorFlags > 0) && m_fDisplayErrors)
                        {
                            wsprintfA(szError, "<CE-%u>", dwErrorFlags);
                        }
                        break;
                    }  // else

                }  // while

            }
            else
            {
                // some other error occurred

                dwLength = 0;
                ClearCommError(m_hPort, &dwErrorFlags, &ComStat);
                if ((dwErrorFlags > 0) && m_fDisplayErrors)
                {
                    wsprintfA(szError, "<CE-%u>", dwErrorFlags);

                }
            }  // else
        }
    }

    CloseHandle(st_Overlap.hEvent);
    return (dwLength);

} // end of read()





/*                ========== CPhysicalPort::setConfig ==========

   This routines sets up the DCB based on settings in the
   class structure and performs a SetCommState().
   Sets the configuration parameters of the com port

   Args:
         portCfg  structure that contains the parameters for the comports

   Return: TRUE if the function succeeds, false if it failed

* /


BOOL  CPhysicalPort::setConfig (P_DCOM_SERPARAM_MSSG serParam)
{
   PORTCFG portCfg;

   printf("CPhysicalPort::setConfig %d\n",serParam->dwBaudRate);
   portCfg.BaudRate  = serParam->dwBaudRate ;
   portCfg.ByteSize  = serParam->dwByteSize ;
   portCfg.Parity    = serParam->dwParity ;
   portCfg.StopBits  = serParam->dwStopBits ;

   return setConfig(portCfg);
}  // setConfig


/*

*/

void    CPhysicalPort::setCMBSSet(PORTCFG portCfg)
{
    BOOL  fRetVal;
    DCB   dcb;
    DWORD LastError = 0;

    dcb.DCBlength = sizeof(DCB);

    GetCommState(m_hPort, &dcb);

    dcb.BaudRate   = m_PortCfg.BaudRate = 115200;
    dcb.ByteSize   = m_PortCfg.ByteSize = 8;
    dcb.Parity     = m_PortCfg.Parity   = 0;
    dcb.StopBits   = m_PortCfg.StopBits = 1;

    // setup hardware flow control

    m_bFlowCtrl = 0;
    dcb.fOutxDsrFlow = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    // setup software flow control

    dcb.fInX = FALSE;

    // other various settings

    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

    fRetVal = SetCommState(m_hPort, &dcb);
    LastError = GetLastError();
    //   return ( fRetVal ) ;
}
/*                ========== CPhysicalPort::setConfig ==========

   This routines sets up the DCB based on settings in the
   class structure and performs a SetCommState().
   Sets the configuration parameters of the com port

   Args:
         portCfg  structure that contains the parameters for the comports

   Return: TRUE if the function succeeds, false if it failed

*/


BOOL  CPhysicalPort::setConfig(PORTCFG portCfg)
{
    BOOL  fRetVal;
    BYTE  bSet;
    DCB      dcb;
    DWORD LastError = 0;

    dcb.DCBlength = sizeof(DCB);

    GetCommState(m_hPort, &dcb);

    dcb.BaudRate   = m_PortCfg.BaudRate = portCfg.BaudRate;
    dcb.ByteSize   = m_PortCfg.ByteSize = portCfg.ByteSize;
    dcb.Parity     = m_PortCfg.Parity   = portCfg.Parity;
    dcb.StopBits   = m_PortCfg.StopBits = portCfg.StopBits;

    // setup hardware flow control

    bSet = (BYTE)((m_bFlowCtrl & FC_DTRDSR) != 0);
    dcb.fOutxDsrFlow = bSet;
    if (bSet)
        dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
    else
        dcb.fDtrControl = DTR_CONTROL_ENABLE;

    bSet = (BYTE)((m_bFlowCtrl & FC_RTSCTS) != 0);
    dcb.fOutxCtsFlow = bSet;
    if (bSet)
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    else
        dcb.fRtsControl = RTS_CONTROL_ENABLE;

    // setup software flow control

    bSet = (BYTE)((m_bFlowCtrl & FC_XONXOFF) != 0);

    dcb.fInX = dcb.fOutX = bSet;
    dcb.XonChar = ASCII_XON;
    dcb.XoffChar = ASCII_XOFF;
    dcb.XonLim = 100;
    dcb.XoffLim = 100;

    // other various settings

    dcb.fBinary = TRUE;
    dcb.fParity = TRUE;

    fRetVal = SetCommState(m_hPort, &dcb);
    LastError = GetLastError();
    return (fRetVal);
}  // setConfig


/*                ========== CPhysicalPort::getConfig ==========

   Gets the configuration parameters of the com port

   Args:

   Return: TRUE if the function succeeds, false if it failed

*/
PORTCFG  CPhysicalPort::getConfig(void)
{
    PORTCFG portCfg;

    memset(&portCfg, 0, sizeof(PORTCFG)); // not implemented at the moment

    return portCfg;
}  // getConfig

/*                ========== CPhysicalPort::setModemState ==========

   Set the Modem State (map from DWORD to BYTE)

   Args: dwModemState   Modem state as defined in winbase.h

   Return:

*/

void     CPhysicalPort::setModemState(DWORD dwModemState)
{
    /* if ((dwModemState & MS_CTS_ON) == MS_CTS_ON)
       {m_MsrBits |= MSR_CTS;} // The CTS (clear-to-send) signal is on.
       if ((dwModemState & MS_DSR_ON) == MS_DSR_ON)
       {m_MsrBits |= MSR_DSR;} // The DSR (data-set-ready) signal is on.
       if ((dwModemState & MS_RING_ON) == MS_RING_ON)
       {m_MsrBits |= MSR_RI;}  // The ring indicator signal is on.
       if ((dwModemState & MS_RLSD_ON) == MS_RLSD_ON)
       {m_MsrBits |= MSR_DCD;} // The RLSD (receive-line-signal-detect) signal is on.
    */
}
// setModemState


/*                ========== CPhysicalPort::setModemState ==========

   Set the Modem State (map from DWORD to BYTE)

   Args: dwModemState   Modem state as defined in winbase.h

   Return:

*/

void     CPhysicalPort::setMSRBits(BYTE bModemState)
{
    m_MsrBits = bModemState;
}  // setModemState

/*                ========== CPhysicalPort::setMCR ==========

   Set the Modem Control Register DTR and RTS lines

   Args: bModemState Modem Control byte

   Return: TRUE, if the Escape character was successfully send, else FALSE

*/

BOOL  CPhysicalPort::setMCR(BYTE bModemState)
{
    /* if ((bModemState & MCR_DDTR) == MCR_DDTR)    // Data Terminal Ready Line was changed
       {
          if ((bModemState & MCR_DTR) == MCR_DTR)
          {
             printf("SETDTR \n");
             return EscapeCommFunction(m_hPort,SETDTR);
          }
          else
          {
             printf("CLRDTR \n");
             return EscapeCommFunction(m_hPort,CLRDTR);
          }
       }
       else
       if ((bModemState & MCR_DRTS) == MCR_DRTS)    // Request to Send Line was changed
       {
          if ((bModemState & MCR_RTS) == MCR_RTS)
          {
             printf("SETRTS \n");
             return EscapeCommFunction(m_hPort,SETRTS);
          }
          else
          {
             printf("CLRRTS \n");
             return EscapeCommFunction(m_hPort,CLRRTS);
          }
       }
       else return FALSE;
    */
    return FALSE;
}   // setMSR

/*                ========== CPhysicalPort::setRTS ==========

   Sets and clears the RTS line

   Args: bRTS

   Return: none

*/

void  CPhysicalPort::setRTS(BYTE bRTS)
{
    if (bRTS)
        EscapeCommFunction(m_hPort, SETRTS);
    else
        EscapeCommFunction(m_hPort, CLRRTS);
}  // setRTS
