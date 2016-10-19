#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include "pthread.h"
#include <semaphore.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#include <fcntl.h>
#include "gmep-us.h"
#include "suota.h"

#ifdef SUOTA_LOCAL_FILE
#define CURL_STATICLIB
#include "curl.h"
#endif

#ifndef CMBS_API
#include "common.h"
#endif

#ifdef CMBS_SUOTA_DEBUG
#define SUOTA_INFO printf
#else
#define SUOTA_INFO
#endif

#define SUOTA_ERROR printf

#define RTP_THREAD_STACKSIZE 16*1024

#define PORT      9976
#define max_sessions     2
#define max_app_sessions   1
#define FULL_HTTP_SUPPORT   4
#define SUOTA_APPLICATION_ID 0x437
#define MSG_BUF_SIZE    4096
#define Suota_SESSION_NOT_CREATED  0
#define Suota_SESSION_CREATED    1
#define FACILITY_PAYLOAD_SIZE   52

static u32 u32_NumOfBytesRead = 0;
static u32 u32_TotalFileSize = 0;


static const char* CMBS_SUOTA_LOCAL_FILE_NAME = "ITCM_H";

typedef enum
{
    Gmep_session_not_created = 0,
    Gmep_session_created = 1

} t_gmepstate;

typedef enum
{
    SUOTA_STATE_IDLE,
    SUOTA_STATE_PUSH_MODE_SEND,
    SUOTA_STATE_HS_VER_IND_RX,
    SUOTA_STATE_HS_VER_AVAIL_TX,
    SUOTA_STATE_URL_TX,
    SUOTA_STATE_URL_RX,
    SUOTA_STATE_GMEP_SESSION_OPEN
} t_SuotaState;

typedef enum
{
    SUOTA_GMEP_SESSION_IDLE = 1,
    SUOTA_GMEP_SESSION_ACTIVE,
    SUOTA_GMEP_SESSION_DOWNLOAD
} t_SuotaGmepSessionState;


t_Suota_GmepSessionTbl suotaSessionTbl[GMEP_MAX_SESSIONS];



typedef enum
{
    Free,
    Busy
} status;

typedef struct
{
    u16 HS;
    u16 sessionid;
    status s;
} t_st_suota_multiple_session;

t_st_suota_multiple_session    psuota_session[5];

typedef struct
{
    u16 hs;
    //t_SuotaState Suotastate;
    status s;
    u32 sessionid;
} t_st_cplanesessiontable;

t_st_cplanesessiontable cplanesession[5];

typedef struct
{
    u16 hs;
    t_gmepstate gmepstate;
    u32 sessionid;
    int socketfd;
} t_st_httpsessiontable;

t_st_httpsessiontable httpsession[5];

typedef struct
{
    u8 appId;
    u8 sessionId;
    int gtstRdFd;
    int gtstWrFd;
} t_st_SessionTbl;
t_st_SessionTbl suotaapp[max_app_sessions];

typedef struct
{
    t_SuotaState suotaState;
    u8   noUrl2;
    char inputchar;
} t_st_SuotaSt;
t_st_SuotaSt g_SuotaInfo;

typedef struct
{
    u16 hsno;
    u8 furl_total;
    u8 furl_sent;
    u8 furl_link[3][40];
    u8 furl_linkLen[3];
} t_st_CplaneUrlInfo;
t_st_CplaneUrlInfo UrlInfo ;

typedef struct
{
    t_st_GmepSuotaHsVerAvail hsVerAvail;
    t_st_GmepSuotaUrlInd hsVerUrl;
} t_st_SuotaMgmtConf;
t_st_SuotaMgmtConf gSuotaMgmtConf;

char line[503] = {0};
int appCnt = 0;
/* set of fifo read filedescriptors for select() */
fd_set suotaR;
/* set of fifo error filedescriptors for select() */

fd_set suotaE;
int loopCnt = 0;
int filedescr;
int readfd, writefd;


static int g_PushMode = 0;
static u16 g_PushModeHsID = 0xff;


void   suota_CallBack(t_GmepCmd , void *, u32);
void   cplane_CmdRecvdCB(u8, t_GmepCmd ,  void *);
void   cplane_ProcessHandVerInd(t_st_GmepSuotaHsVerInd * , u16) ;
void   cplane_processNegack(t_st_GmepSuotaNegAck *, u16);
void   cplane_SendHandVerAvail(t_st_GmepSuotaHsVerAvail *, u16);
void   cplane_ProcessUrl(t_st_GmepSuotaUrlInd *);
void   cplane_SendUrl(t_st_GmepSuotaUrlInd * , u16);
void   Cplane_FillurlInfo(u16, int);

void   cplane_SendNegAck(t_st_GmepSuotaNegAck  *);
void   suota_httpRequestThread(void *);
void   Suota_DataSend(t_st_GmepSdu*);
char   *get_ip(char *);
void    suota_CoreInit();
void    http_Init(void);
void    cplane_Init(void);
int     close(int);
int     create_tcp_socket();
void    *core(void *);
void   cplane_FacilityCB(u8 handset, u8 bFacilitySent);
struct hostent *gethostbyname(const char *);
int suotaWrFd, suotaRdFd;
static  int fifo_open_write(char *);
static int fifo_open_read(char *);
static int fifo_read(int , char *, int);
static int fifo_write(int , const void *, int);
#ifdef SUOTA_LOCAL_FILE
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
#endif //SUOTA_LOCAL_FILE

#define SUOTA_PROXY_IP_ADDR_LEN 16
char  suotaProxyIp[SUOTA_PROXY_IP_ADDR_LEN];
u16  suotaProxyPort;
u8  suotaRdCfg = 0;


int suota_ReadConfig(char * pBuffer)
{
    FILE *fp;
    int index = 0, rc = 0, i = 0;

    fp = fopen("/etc/suota_mgmt.cfg", "r");

    if (fp == NULL)
    {
        SUOTA_ERROR(" SUOTA: Error:!!! Can not open /etc/suota_mgmt.cfg ... \n");
        return -1;
    }

    for (index = 0; ((rc = fgetc(fp)) != EOF); ++index)
    {
        if (rc == '#')
        {   // skip the commented part . i.e. skip full line followed by "#"
            for (i = 0; ((rc = fgetc(fp)) != EOF); ++i)
            {
                if (rc == '\n')
                    break;
            }
        }
        if (rc == '\n' || rc == '\r')
            --index;
        else
            pBuffer[index] = rc;
    }
    fclose(fp);

    return 0;
}

int suota_ParseCfgFile()
{
    char buffer[2000];
    char *tempstr;

    memset(&gSuotaMgmtConf, 0, sizeof(t_st_SuotaMgmtConf));

    if (-1 == suota_ReadConfig(buffer))
        return -1;

    tempstr = strtok(buffer, ";");
    gSuotaMgmtConf.hsVerAvail.st_Header.u16_delayInMin = atoi(tempstr);

    tempstr = strtok(NULL, ";");
    gSuotaMgmtConf.hsVerAvail.st_Header.u8_URLStoFollow = atoi(tempstr);

    tempstr = strtok(NULL, ";");
    gSuotaMgmtConf.hsVerAvail.st_Header.u8_Spare = atoi(tempstr);

    tempstr = strtok(NULL, ";");
    gSuotaMgmtConf.hsVerAvail.st_Header.u8_UserInteraction = atoi(tempstr);

    tempstr = strtok(NULL, ";");
    strncpy((char*)gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer, tempstr, sizeof(gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer) - 1);

    tempstr = strtok(NULL, ";");
    gSuotaMgmtConf.hsVerUrl.u8_URLStoFollow = atoi(tempstr);

    tempstr = strtok(NULL, ";");
    strncpy((char*)gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL, tempstr, sizeof(gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL) - 1);

    SUOTA_INFO("\n SUOTA: CFg Mgmt Configuation: \n");
    SUOTA_INFO(" gSuotaMgmtConf.hsVerAvail.st_Header.u16_delayInMin - %d\n", gSuotaMgmtConf.hsVerAvail.st_Header.u16_delayInMin);
    SUOTA_INFO(" gSuotaMgmtConf.hsVerAvail.st_Header.u8_URLStoFollow- %d\n", gSuotaMgmtConf.hsVerAvail.st_Header.u8_URLStoFollow);
    SUOTA_INFO(" gSuotaMgmtConf.hsVerAvail.st_Header.u8_Spare -%d\n", gSuotaMgmtConf.hsVerAvail.st_Header.u8_Spare);
    SUOTA_INFO(" gSuotaMgmtConf.hsVerAvail.st_Header.u8_UserInteraction - %d\n", gSuotaMgmtConf.hsVerAvail.st_Header.u8_UserInteraction);
    SUOTA_INFO(" gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer - %s, - %zu\n", gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer, strlen((char*)gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer));
    SUOTA_INFO(" gSuotaMgmtConf.hsVerUrl.u8_URLStoFollow - %u\n", gSuotaMgmtConf.hsVerUrl.u8_URLStoFollow);
    SUOTA_INFO(" gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL - %s, %zu\n", gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL, strlen((char*)gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL));

    return 0;
}

u8 getNumOfURLMsgs(char* pch_URL)
{
    u8 u8_URL_Len = strlen((char *)pch_URL);
    u8 u8_NumOfMsgs;
    u8_NumOfMsgs = u8_URL_Len / FACILITY_PAYLOAD_SIZE;
    if (u8_URL_Len % FACILITY_PAYLOAD_SIZE)
    {
        ++u8_NumOfMsgs; // one more message for the residue
    }
    return u8_NumOfMsgs;
}

#ifdef CMBS_API
pthread_t g_soutaProcessThreadId;
void *soutaThread(void *args)
{
    SUOTA_INFO("SUOTA: soutaThread Process Started g_PushMode<%d>\n", g_PushMode);
    u16 requestId = 1;
    UNUSED_PARAMETER(args);
    if (-1 == suota_ParseCfgFile())
        return NULL;

    suota_CoreInit();

    if (g_PushMode)
    {
        //gmep_us_PushModeSend();
        cmbs_dsr_suota_SendSWUpdateInd(NULL, g_PushModeHsID , SUOTA_GE00_SU_FW_UPGRAGE, requestId);
    }

    suota_app();

    return NULL;
}


/**
 *
 *
 * @brief
 *
 * @return unsigned long
 *  0  =  success
 */
unsigned long suota_createThread(char *pIpAddr, u16 portNumber, int pushMode, u16 u16_Handset)
{
    int ret;
    pthread_attr_t  attr;

    strncpy(suotaProxyIp, pIpAddr, SUOTA_PROXY_IP_ADDR_LEN);
    suotaProxyIp[SUOTA_PROXY_IP_ADDR_LEN - 1] = 0;
    suotaProxyPort = portNumber;

    g_PushMode = pushMode;
    g_PushModeHsID = u16_Handset;

    SUOTA_INFO("SUOTA: suota_createThread PrxyIP<%s> Port<%d> g_PushMode<%d>\n", suotaProxyIp, suotaProxyPort, g_PushMode);
    pthread_attr_init(&attr);
    ret = pthread_create(&g_soutaProcessThreadId, &attr, soutaThread, NULL);
    pthread_attr_destroy(&attr);

    return ret;
}

unsigned long suota_quitThread()
{
    SUOTA_INFO("suota_quitThread\n");
    if (pthread_cancel(g_soutaProcessThreadId) != 0)
    {
        SUOTA_ERROR("suota_quitThread Error Couldnt pthread_cancel  \n");
    }
    if (pthread_join(g_soutaProcessThreadId, NULL) != 0)
    {
        SUOTA_ERROR("suota_quitThread Error Couldnt pthread_join  \n");
    }
    return 0;
}
#endif

#ifndef CMBS_API
void main(int argc, char * argv[])
{
    int i;


    if (argc < 4)
    {
        SUOTA_INFO(" \n SUOTA returned error: SUOTA Usage:\n");
        SUOTA_INFO(" ./suota  ProxyIpAddress  ProxyPort ReadCfg) & \n");
        SUOTA_INFO("\t ReadCfg = 0; do not read from CFG file for HTTP request, process HTTP request from HS\n");
        SUOTA_INFO("\t ReadCfg = 1; read HTTP request from CFG file, drop HTTP request from HS\n");
        return;
    }

    strcpy(suotaProxyIp, argv[1]);
    suotaProxyPort = atoi(argv[2]);
    suotaRdCfg = atoi(argv[3]);
    SUOTA_INFO("\n SUOTA: Proxy Ip - %s, Port Number - %d \n", suotaProxyIp, suotaProxyPort);
    /*Initialization of http,gmep,cplane*/

    if (-1 == suota_ParseCfgFile())
        return;

    suota_CoreInit();

    /*C plane processing and File download Process */
    //gmep_us_PushModeSend();
    suota_app();
}
#endif

/*c plane processing  call back function.*/
void  cplane_CmdRecvdCB(u8 Handsetno, t_GmepCmd suotaCommand, void * pData)
{
    void *pdatarecvd;
    u16 hsno;
    hsno = (u16)Handsetno;

    switch (suotaCommand)
    {
        case GMEP_US_HS_VER_IND:
        {
            g_SuotaInfo.suotaState = SUOTA_STATE_HS_VER_IND_RX;
            pdatarecvd = (t_st_GmepSuotaHsVerInd *)pData;
            cplane_ProcessHandVerInd(pdatarecvd, hsno) ;
            break;
        }
        case GMEP_US_NACK:
        {
            pdatarecvd = (t_st_GmepSuotaNegAck *)pData;
            //SUOTA_INFO("\n");
            cplane_processNegack(pdatarecvd, hsno);
            break;
        }
        case GMEP_US_URL_IND:
        {
            break;
        }
        default:
            SUOTA_INFO(" SUOTA:cplane_CmdRecvdCB: incorrect suota command\n");
            break;
    }
}

//Recieves negative acknowledgement from Gmep test stub
void cplane_processNegack(t_st_GmepSuotaNegAck *pdatanegack, u16 handsetno)
{
    UNUSED_PARAMETER(handsetno);
    SUOTA_INFO("SUOTA:negack_handsetno is %d\n", pdatanegack->hsNo);
    SUOTA_INFO("SUOTA:negack_failurereason %d\n", pdatanegack->rejReason);
}

// Recieves Handset Version Indication Request from Gmep test stub.
//compares HS current sw version with Three versions..
void cplane_ProcessHandVerInd(t_st_GmepSuotaHsVerInd  * hsVerInd1, u16 hsno)
{
    //int noUrl2 = 0;
    char temp1[100], temp2[100];
    g_SuotaInfo.noUrl2 = 0;

    SUOTA_INFO("SUOTA:Handset Version Indication Recieved\n");
    SUOTA_INFO("SUOTA:handsetno - %d \n", hsno);
    SUOTA_INFO("SUOTA:Handset Version Emc is %d\n", hsVerInd1->st_Header.u16_EMC);
    SUOTA_INFO("SUOTA:Handset Version urlstofollow is %d\n", hsVerInd1->st_Header.u8_URLStoFollow);
    SUOTA_INFO("SUOTA:Handset Version filenumber is %d\n", hsVerInd1->st_Header.u8_FileNum);
    SUOTA_INFO("SUOTA:Handset Version spare is %d\n", hsVerInd1->st_Header.u8_Spare);
    SUOTA_INFO("SUOTA:Handset Version flags is %d\n", hsVerInd1->st_Header.u8_Flags);
    SUOTA_INFO("SUOTA:Handset Version reasonis %d\n", hsVerInd1->st_Header.u8_Reason);
    SUOTA_INFO("SUOTA:handsetswverlength - %d \n", hsVerInd1->st_SwVer.u8_BuffLen);
    memcpy(temp1, hsVerInd1->st_SwVer.u8_Buffer, hsVerInd1->st_SwVer.u8_BuffLen);
    temp1[hsVerInd1->st_SwVer.u8_BuffLen] = '\0';
    SUOTA_INFO("SUOTA:suotahandsetswversion is %s \n", temp1);
    SUOTA_INFO("SUOTA:handsethwverlength - %d \n", hsVerInd1->st_HwVer.u8_BuffLen);
    memcpy(temp2, hsVerInd1->st_HwVer.u8_Buffer, hsVerInd1->st_HwVer.u8_BuffLen);
    temp2[hsVerInd1->st_HwVer.u8_BuffLen] = '\0';
    SUOTA_INFO("SUOTA:suotahandsethwversion is %s \n", temp2);

    if ((!strncmp((char*)hsVerInd1->st_SwVer.u8_Buffer, "1.1", hsVerInd1->st_SwVer.u8_BuffLen))
            || (!strncmp((char*)hsVerInd1->st_SwVer.u8_Buffer, "1.2", hsVerInd1->st_SwVer.u8_BuffLen))
            || (!strncmp((char*)hsVerInd1->st_SwVer.u8_Buffer, "1.3", hsVerInd1->st_SwVer.u8_BuffLen))
            || (!strncmp((char*)hsVerInd1->st_SwVer.u8_Buffer, "1.4", hsVerInd1->st_SwVer.u8_BuffLen)))
    {
        //if HS is already downloaded with latest version
        if (!strncmp((char*)hsVerInd1->st_SwVer.u8_Buffer, "1.4", hsVerInd1->st_SwVer.u8_BuffLen))

        {
            g_SuotaInfo.noUrl2 = 1;
        }
        //if previous file download failed
        if ((hsVerInd1->st_Header.u8_Reason) != 0)
        {
            g_SuotaInfo.noUrl2 = 1;
        }
        //Cplane_FillurlInfo(hsVerInd1->hsNo, noUrl2);
    }
    Cplane_FillurlInfo(hsno, g_SuotaInfo.noUrl2);

}

// Handset version available command is sent with latest version updated.Assuming each software version is associated with 3 files(urls).
void  Cplane_FillurlInfo(u16 handsetno, int noUrl2)
{
    UNUSED_PARAMETER(noUrl2);
    SUOTA_INFO("SUOTA:Cplane_FillurlInfo \n");
    UrlInfo.hsno = handsetno;
    if (UrlInfo.hsno == handsetno)
    {
        // filling HS version available structure
        t_st_GmepSuotaHsVerAvail hsVerAvail;

        hsVerAvail.st_Header.u16_delayInMin = gSuotaMgmtConf.hsVerAvail.st_Header.u16_delayInMin;
        hsVerAvail.st_Header.u8_URLStoFollow = gSuotaMgmtConf.hsVerAvail.st_Header.u8_URLStoFollow;
        hsVerAvail.st_Header.u8_Spare = gSuotaMgmtConf.hsVerAvail.st_Header.u8_Spare;
        hsVerAvail.st_Header.u8_UserInteraction = gSuotaMgmtConf.hsVerAvail.st_Header.u8_UserInteraction;
        hsVerAvail.st_SwVer.u8_BuffLen = strlen((char*)gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer);
        strncpy((char*)&hsVerAvail.st_SwVer.u8_Buffer, (char*)gSuotaMgmtConf.hsVerAvail.st_SwVer.u8_Buffer, hsVerAvail.st_SwVer.u8_BuffLen);
        cplane_SendHandVerAvail(&hsVerAvail, UrlInfo.hsno) ;

    }
}

//filling of url indication command

void cplane_FillUrlMsg(u8 hs)
{

    t_st_GmepSuotaUrlInd hsVerUrl;
    u16 handset = (u16)hs;
    static u8 u8_NumOfMsgs = 0;
    static u8 u8_Index = 0;
    static u8 u8_URL_Len = 0;
    static e_URLSendState e_SendingState = CMBS_URL_SENDING_STATE_IDLE;

    if (e_SendingState == CMBS_URL_SENDING_STATE_IDLE)
    {
        // First time we enter - calculate the number of messages
        u8_URL_Len = strlen((char *)gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL);

        // split the URL into several FACILITY messages

        // calculate number of FACILITY messages needed
        u8_NumOfMsgs = getNumOfURLMsgs((char *)gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL);


        e_SendingState = CMBS_URL_SENDING_STATE_IN_PROGRESS;
    }

    if (e_SendingState == CMBS_URL_SENDING_STATE_IN_PROGRESS)
    {
        if (u8_NumOfMsgs == 0)
        {
            e_SendingState = CMBS_URL_SENDING_STATE_IDLE;
            u8_NumOfMsgs = 0;
            u8_Index = 0;
            u8_URL_Len = 0;
            return;
        }

        // URL to follow
        hsVerUrl.u8_URLStoFollow = --u8_NumOfMsgs;

        // URL len
        hsVerUrl.st_URL.u8_URLLen = u8_URL_Len > FACILITY_PAYLOAD_SIZE ? FACILITY_PAYLOAD_SIZE : u8_URL_Len;
        u8_URL_Len -= hsVerUrl.st_URL.u8_URLLen;

        // URL data
        strncpy((char *)hsVerUrl.st_URL.u8_URL, (char *) & (gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL[u8_Index]), hsVerUrl.st_URL.u8_URLLen);
        u8_Index += hsVerUrl.st_URL.u8_URLLen;

        // send
        cplane_SendUrl(&hsVerUrl, handset);
    }
}

void suota_push_mode_send()
{
    gmep_us_PushModeSend();
#if 0
    t_st_GmepSuotaPushMode push_mode;
    char str3[200], str4[200];
    strcpy(push_mode.st_SwVer.u8_Buffer, "1.5");
    push_mode.st_SwVer.u8_BuffLen = strlen("1.5");
    strcpy(str3, push_mode.st_SwVer.u8_Buffer);
    str3[push_mode.st_SwVer.u8_BuffLen] = '\0';
    SUOTA_INFO("\n SUOTA:latest sw version is %s \n", str3);
    strcpy(push_mode.st_HwVer.u8_Buffer, "1.6");
    push_mode.st_HwVer.u8_BuffLen = strlen("1.5");
    strcpy(str4, push_mode.st_HwVer.u8_Buffer);
    str4[push_mode.st_HwVer.u8_BuffLen] = '\0';
    SUOTA_INFO("\n SUOTA:latest hw version is %s \n", str4);
#endif
}

//filling up negative acknowledgement command structure
void cplane_Fillnegack()
{
    t_st_GmepSuotaNegAck negack;
    negack.hsNo = UrlInfo.hsno;
    negack.rejReason = SUOTA_FILE_DOESNOT_EXIST;
    SUOTA_INFO("SUOTA:NegativeAcknowledgement reason is %d\n", negack.rejReason);
    SUOTA_INFO("SUOTA:Requested sw version is not in MGT server\n");
    cplane_Sendnegack(&negack);
}

#if 0
void cplane_FacilityCB(u8 handset, u8 bFacilitySent)
{
    switch (hsno)
    {
            switch (state)
                for (int i = 0; i < 5; i++)
                {
                    switch (cplanesession[i].suotaState)
                    {
                        case SUOTA_STATE_HS_VER_AVAIL_TX:
                        {
                            if (1 == bFacilitySent && cplanesession[i].hsno == handset)
                            {

                                SUOTA_INFO("\n SUOTA: cplane_FacilityCB: SUOTA_STATE_HS_VER_AVAIL_TX");
                                //g_SuotaInfo.suotaState = SUOTA_STATE_URL_TX;
                                cplanesession[i].suotastate = SUOTA_STATE_URL_TX;
                                cplane_FillUrlMsg();
                            }
                            else if (cplanesession[i].


                        }
                    case SUOTA_STATE_URL_TX:
                    {

                        if (1 == bFacilitySent)
                            {

                                g_SuotaInfo.suotaState = SUOTA_STATE_IDLE;
                                SUOTA_INFO("\n SUOTA: cplane_FacilityCB: SUOTA_STATE_URL_TX");


                            }

                        }

                        default:
                            break;
                    }
                }

    }
}
#endif

void cplane_FacilityCB(u8 handset, u8 bFacilitySent)
{
    switch (g_SuotaInfo.suotaState)
    {
#if 0
        case SUOTA_STATE_PUSH_MODE_SEND:
        {
            SUOTA_INFO("\n SUOTA: FacilityCallback : SUOTA_STATE_PUSH_MODE_SEND");
            SUOTA_INFO("\n Handset Recieved Facility %d", handset);
            //prakash; debug purpose
            SUOTA_INFO(" SUOTA:cplane_ProcessHandVerInd returned  $$$$ \n ");
            g_SuotaInfo.suotaState = SUOTA_STATE_HS_VER_AVAIL_TX;
        }
#endif
        case SUOTA_STATE_HS_VER_AVAIL_TX:
        {
            if (1 == bFacilitySent)
            {
                SUOTA_INFO("\n SUOTA: cplane_FacilityCB: SUOTA_STATE_HS_VER_AVAIL_TX");
                g_SuotaInfo.suotaState = SUOTA_STATE_URL_TX;
                cplane_FillUrlMsg(handset);
            }
        }
        break;
        case SUOTA_STATE_URL_TX:
        {
            if (1 == bFacilitySent)
            {
                g_SuotaInfo.suotaState = SUOTA_STATE_IDLE;
                SUOTA_INFO("\n SUOTA: cplane_FacilityCB: SUOTA_STATE_URL_TX");
            }
        }
        break;
        default:
            SUOTA_INFO("\n SUOTA: cplane_FacilityCB(): Invalid command ");
            break;
    }

}

//sending HS version available comand to stub.
void cplane_SendHandVerAvail(t_st_GmepSuotaHsVerAvail *hsverAvail1, u16 hsno)
{

#ifdef CMBS_API
    ST_SUOTA_UPGRADE_DETAILS  st_HSVerAvail;
    u16 u16_RequestId;
    ST_VERSION_BUFFER st_SwVersion;

    SUOTA_INFO("SUOTA:cplane_SendHandVerAvail \n");
    g_SuotaInfo.suotaState = SUOTA_STATE_HS_VER_AVAIL_TX;

    st_HSVerAvail.u16_delayInMin = hsverAvail1->st_Header.u16_delayInMin;
    st_HSVerAvail.u8_URLStoFollow = getNumOfURLMsgs((char *)gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL);
    st_HSVerAvail.u8_UserInteraction = hsverAvail1->st_Header.u8_UserInteraction;
    st_HSVerAvail.u8_Spare = hsverAvail1->st_Header.u8_Spare;

    st_SwVersion.u8_VerLen = hsverAvail1->st_SwVer.u8_BuffLen;
    if (hsverAvail1->st_SwVer.u8_BuffLen > CMBS_MAX_VERSION_LENGTH)
    {
        SUOTA_INFO("SUOTA:cplane_SendHandVerAvail Error - buf length 2 long \n");
    }
    else
        memcpy(st_SwVersion.pu8_VerBuffer, hsverAvail1->st_SwVer.u8_Buffer, hsverAvail1->st_SwVer.u8_BuffLen);

    u16_RequestId = 1;

    st_SwVersion.type = CMBS_SUOTA_SW_VERSION;

    cmbs_dsr_suota_SendHSVersionAvail(NULL, st_HSVerAvail, hsno, &st_SwVersion, u16_RequestId);
#else
    SUOTA_INFO("SUOTA:handset version available command sent to GMEP\n");
    g_SuotaInfo.suotaState = SUOTA_STATE_HS_VER_AVAIL_TX;
    gmep_us_SendHandVerAvail(hsverAvail1, hsno, &cplane_FacilityCB);
#endif

}

//sending Url indication command to stub
void cplane_SendUrl(t_st_GmepSuotaUrlInd  *url, u16 handset)
{
#ifdef CMBS_API
    ST_URL_BUFFER pst_Url;
    u16 u16_RequestId;

    SUOTA_INFO("cplane_SendUrl\n");

    pst_Url.u8_UrlLen = url->st_URL.u8_URLLen;
    if (url->st_URL.u8_URLLen <= CMBS_MAX_URL_SIZE)
    {
        memcpy(pst_Url.pu8_UrlBuffer, url->st_URL.u8_URL, url->st_URL.u8_URLLen);
    }
    else
    {
        pst_Url.u8_UrlLen = 0;
    }

    u16_RequestId = 1;
    cmbs_dsr_suota_SendURL(NULL, handset, url->u8_URLStoFollow, &pst_Url, u16_RequestId);
#else
    gmep_us_SendUrl(handset, url, &cplane_FacilityCB);
#endif
}

//sending negative acknowledgement to stub
void cplane_Sendnegack(t_st_GmepSuotaNegAck *Nack)
{
    SUOTA_INFO("SUOTA:negative ack sent to GMEP\n");
    gmep_us_SendNegAck(Nack->hsNo, Nack, &cplane_FacilityCB);
}

int suota_RebuildFdSets(void)
{
    int maxfd, i;
    /* prepare fdsets for select */

    FD_ZERO(&suotaR);
    FD_ZERO(&suotaE);
    maxfd = 0;

    for (i = 0; i < max_app_sessions; i++)
    {
        FD_SET(suotaapp[i].gtstRdFd, &suotaR);
        FD_SET(suotaapp[i].gtstRdFd, &suotaE);
        maxfd = MAX(maxfd, suotaapp[i].gtstRdFd);
    }

    FD_SET(suotaRdFd, &suotaR);
    FD_SET(suotaRdFd, &suotaE);
    maxfd = MAX(maxfd, suotaRdFd);

    //SUOTA_INFO("\n suota_RebuildFdSets %d",maxfd);

    return maxfd;

}

int suota_ProcessAppMsgs(t_st_GmepAppMsgCmd *pMsgCmd, int len)
{
    UNUSED_PARAMETER(len);
    SUOTA_INFO("\n SUOTA: Data received from App Id / session Id - %d - %d \n", pMsgCmd->appSessionId, pMsgCmd->bufLen);
    suota_PrintHex(pMsgCmd->buf, pMsgCmd->bufLen);
    return 0;
}

#if 0
void suota_PrintHex(u8 *buf, u32 len)
{
    u32 i;
    SUOTA_INFO("\nSUOTA_PrintHex 1 - %d - ", len);
    for (i = 0; i < len; i++)
        SUOTA_INFO(" 0x%x ", *(buf + i));
    SUOTA_INFO("  SUOTA_PrintHex 2 \n");
}
#endif

void suota_InitFifo()
{
    char buffer1[CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1];
    int ret;

    snprintf(buffer1, (CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1), "/tmp/psuota-%05d",  getpid());
    ret = mkfifo(buffer1, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

    if (ret < 0)
    {
        SUOTA_ERROR("SUOTA: ERROR: couldn't mkfifo ");
        exit(1);
    }

    suotaRdFd = fifo_open_read(buffer1);
    suotaWrFd = fifo_open_write(buffer1);

    return;
}

u32 suota_CheckFileSize()
{
    // return the number of bytes in the file
    static FILE* fd = NULL;
    u32 u32_FileSize = 0;

    if ((fd = fopen(CMBS_SUOTA_LOCAL_FILE_NAME, "rb")) == NULL)
    {
        SUOTA_ERROR("\n fopen() Error!!!\n");
    }

    //get the size of the file
    fseek(fd, 0, SEEK_END);

    u32_FileSize = ftell(fd);
    fclose(fd);

    return u32_FileSize;
}

int suota_ReadFile(u32 u32_NumOfBytes, u32* u32_Offset)
{
    static FILE* fd = NULL;

    u32 u32_content_length = 0;


    if (NULL == fd)
    {
        fd = fopen(CMBS_SUOTA_LOCAL_FILE_NAME, "rb");
    }

    // start reading from the place we stoped last time. u32_NumOfBytesRead is the number of bytes read till now
    if (fseek(fd, u32_NumOfBytesRead, SEEK_SET) == (-1))
        return -1;

    if (u32_NumOfBytesRead < u32_TotalFileSize)
    {
        // if we still didn't reach the end of the file read another chunck of data
        u32_content_length = fread(u32_Offset, 1, u32_NumOfBytes, fd);

        if (u32_content_length <= 0)
        {
            SUOTA_ERROR("Could not read from file!! ERROR = %d\n", u32_content_length);
            return u32_content_length;
        }
        // increase u32_NumOfBytesRead by the value fread returns
        u32_NumOfBytesRead += u32_content_length;
        // check that with the new u32_NumOfBytesRead we didnot reach the end of the file
        // if yes, close the file
        if (u32_NumOfBytesRead == u32_TotalFileSize)
        {
            fclose(fd);
            fd = NULL;
        }

    }
    return u32_content_length;

}

void suota_ConstructMessage(t_st_httpResponse* st_Response)
{
    // This function construct the message to be sent to the BS
    u32 u32_count = 0;
    int resp;

    // Calculate the content length to be read from a file and sent
    if (u32_NumOfBytesRead == 0)
    {
        // On first request we read only image header
        u32_TotalFileSize = suota_CheckFileSize();
        st_Response->content_length = SUOTA_IMAGE_HEADER_SIZE;
    }

    else if ((u32_NumOfBytesRead + SUOTA_MAX_PAYLOAD_SIZE) <= u32_TotalFileSize)
    {
        st_Response->content_length = SUOTA_MAX_PAYLOAD_SIZE;
    }

    else
    {
        // last packet may be smaller than SUOTA_MAX_PAYLOAD_SIZE, calculate it
        st_Response->content_length = ((u32_TotalFileSize - SUOTA_IMAGE_HEADER_SIZE) % SUOTA_MAX_PAYLOAD_SIZE);
    }

    // Build the artificial HTTP header
    u32_count += sprintf((char*)st_Response->responseBuff, "HTTP/1.1 206 Partial Content\r\n");
    u32_count += sprintf((char*)st_Response->responseBuff + u32_count, "Content-Length: %d\r\n", st_Response->content_length);
    u32_count += sprintf((char*)st_Response->responseBuff + u32_count, "Content-Range: bytes %d-%d/%d\r\n", u32_NumOfBytesRead, (u32_NumOfBytesRead + st_Response->content_length), u32_TotalFileSize);
    u32_count += sprintf((char*)st_Response->responseBuff + u32_count, "Content-Type: application/octet-stream\r\n\r\n");

    // read chunck of file and add it contents to response
    resp = suota_ReadFile(st_Response->content_length, (u32*) & (st_Response->responseBuff[u32_count]));

    // calculate response length
    st_Response->responseBuffLen = u32_count + st_Response->content_length;

}

// Suota initialization

void suota_CoreInit()
{
    int ret;
    u32 i;

    memset(&g_SuotaInfo, 0, sizeof(t_st_SuotaSt));
    g_SuotaInfo.suotaState = SUOTA_STATE_IDLE;
    for (i = 0; i < max_app_sessions; i++)
    {
        suotaapp[i].gtstRdFd = -1;
        suotaapp[i].gtstWrFd = -1;
    }

    ret = gmep_us_Init(&suotaapp[0].gtstRdFd, &suotaapp[0].gtstWrFd, SUOTA_APPLICATION_ID);

    SUOTA_INFO(" SUOTA: GMEP Librarty initialization - %d\n", ret);

    http_Init();

    cplane_Init();

    gmep_us_RegAppCB(NULL, SUOTA_APPLICATION_ID);
    SUOTA_INFO("SUOTA: Core initialization\n");
    suota_InitFifo();

    for (i = 0; i < GMEP_MAX_SESSIONS; i++)
    {
        suotaSessionTbl[i].sessionId  = -1;
        suotaSessionTbl[i].sesStatus = SUOTA_GMEP_SESSION_IDLE;
        suotaSessionTbl[i].threadRdFd = -1;
        suotaSessionTbl[i].threadWrFd = -1;
    }

    return;
}

void suota_app()
{
    int ret;
    char buf[MSG_BUF_SIZE];
    t_st_GmepAppMsgCmd *pGmepMsgResp;
    int i  = 0, maxfd;
    int j = 0;
    int session_id = 0;

    //DownloadFullBinary();

    while (1)
    {
        maxfd = suota_RebuildFdSets();
        ret = select(maxfd + 1, &suotaR, NULL, &suotaE, NULL);
        if (ret < 0)
        {
            SUOTA_ERROR("SUOTA:  select()'s error %s\n", strerror(ret));
            exit(EXIT_FAILURE);
        }

        for (j = 0; j < max_app_sessions; j++)
        {
            if (FD_ISSET(suotaapp[j].gtstRdFd, &suotaR))
            {
                //ret = msg_fifo_read(suotaapp[j].gtstRdFd, buf, sizeof(t_st_GmepAppMsgCmd));
                ret = read(suotaapp[j].gtstRdFd, buf, sizeof(t_st_GmepAppMsgCmd));
                if (ret < 0)
                {
                    SUOTA_ERROR(" SUOTA: MSGSWD ERROR!! msg_fifo_read(ctrl) returned %d : %s\n",
                                ret, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                pGmepMsgResp = (t_st_GmepAppMsgCmd *)buf;
                //sessionid=pGmepMsgResp->appSessionId;
                SUOTA_INFO("SUOTA: Message type rxed - %d \n", pGmepMsgResp->cmd);

                switch (pGmepMsgResp->cmd)
                {
                    case GMEP_US_SESSION_CREATE_ACK:
                    {
                        //u8 sessionId;
                        SUOTA_INFO(" SUOTA: suota_app:GMEP_US_SESSION_CREATE_ACK \n");
                        //memcpy(&sessionId, &pGmepMsgResp->buf[2], 1);
                        //suota_CallBack(GMEP_US_SESSION_CREATE_ACK, NULL, sessionId);
                        suota_CallBack(GMEP_US_SESSION_CREATE_ACK, NULL, pGmepMsgResp->appSessionId);
                        break;
                    }
                    case GMEP_US_SESSION_CLOSE_ACK:
                    {
                        u8 sessionId;
                        SUOTA_INFO(" SUOTA: suota_app:GMEP_US_SESSION_CLOSE_ACK \n");
                        memcpy(&sessionId, &pGmepMsgResp->buf[0], 1);
                        //SUOTA_INFO(" SUOTA : GMEP_US_SESSION_CLOSE_ACK 1 \n");
                        suota_CallBack(GMEP_US_SESSION_CLOSE_ACK, NULL, sessionId);
                        break;
                    }
                    case GMEP_US_OPEN_SESSION:
                    {
                        break;
                    }
                    case GMEP_US_DATA_RECV:
                    {
                        t_st_GmepSdu sduData;

                        SUOTA_INFO(" SUOTA: GMEP_US_DATA_RECV  buf len %d\n", pGmepMsgResp->bufLen);
                        i = 0;
                        sduData.appSduLen = pGmepMsgResp->bufLen;
                        //sduData.appSdu = (u8 *)malloc(pGmepMsgResp->bufLen);
                        //memcpy(sduData.appSdu, &pGmepMsgResp->buf[i], pGmepMsgResp->bufLen);
                        sduData.appSdu = &pGmepMsgResp->buf[i];
                        //if(gmepSessionTable.pGmepAppCallback)
                        //gmepSessionTable.pGmepAppCallback(SUOTA_DATA_RECV, &sduData);
                        suota_CallBack(GMEP_US_DATA_RECV, &sduData, pGmepMsgResp->appSessionId);
                        //free(sduData.appSdu);
                        break;
                    }
                    case GMEP_US_DATA_SEND_ACK:
                    {
                        u8 result;

                        SUOTA_INFO(" SUOTA: GMEP_US_DATA_SEND_ACK \n");

                        memcpy(&result, &pGmepMsgResp->buf[0], 1);
                        suota_CallBack(GMEP_US_DATA_SEND_ACK, &result, pGmepMsgResp->appSessionId);
                        break;
                    }
                    case GMEP_US_HS_VER_IND:
                    {

                        SUOTA_INFO(" SUOTA: suota_app:GMEP_US_HS_VER_IND\n");

                        t_st_GmepSuotaHsVerInd hsVerInd;
                        u16 hsno;
                        memset(&hsVerInd, 0, sizeof(hsVerInd));
                        i = 0;
                        memcpy(&hsno/*&hsVerInd.hsNo*/, &pGmepMsgResp->buf[i], 2);
                        i = i + 2;
                        memcpy(&hsVerInd.st_Header.u16_EMC, &pGmepMsgResp->buf[i], 2);
                        i  = i + 2;
                        hsVerInd.st_Header.u8_URLStoFollow = pGmepMsgResp->buf[i];
                        i = i + 1;
                        hsVerInd.st_Header.u8_FileNum = pGmepMsgResp->buf[i];
                        i = i  + 1;
                        hsVerInd.st_Header.u8_Spare = pGmepMsgResp->buf[i];
                        i = i  + 1;
                        hsVerInd.st_Header.u8_Flags = pGmepMsgResp->buf[i];
                        i = i  + 1;
                        hsVerInd.st_Header.u8_Reason = pGmepMsgResp->buf[i];
                        i = i  + 1;
                        hsVerInd.st_SwVer.u8_BuffLen = pGmepMsgResp->buf[i];
                        i = i  + 1;
                        hsVerInd.st_SwVer.u8_BuffLen = MIN(MAX_SW_VER_ID_SIZE, hsVerInd.st_SwVer.u8_BuffLen);
                        memcpy(&hsVerInd.st_SwVer.u8_Buffer, &pGmepMsgResp->buf[i], hsVerInd.st_SwVer.u8_BuffLen);
                        i = i + hsVerInd.st_SwVer.u8_BuffLen;
                        hsVerInd.st_HwVer.u8_BuffLen = pGmepMsgResp->buf[i];
                        i = i  + 1;
                        hsVerInd.st_HwVer.u8_BuffLen = MIN(MAX_SW_VER_ID_SIZE, hsVerInd.st_HwVer.u8_BuffLen);
                        memcpy(&hsVerInd.st_HwVer.u8_Buffer, &pGmepMsgResp->buf[i], hsVerInd.st_HwVer.u8_BuffLen);
                        i = i + hsVerInd.st_HwVer.u8_BuffLen;

                        cplanesession[session_id].hs = hsno;
                        cplanesession[session_id].sessionid = pGmepMsgResp->appSessionId;
                        cplanesession[session_id].s = Busy;

                        //pGmepMsgResp->bufLen=i;
                        //if(gmepSessionTable.pGmepCPlaneCmdsCb)
                        //gmepSessionTable.pGmepCPlaneCmdsCb(SUOTA_HS_VER_IND, &hsVerInd);
                        cplane_CmdRecvdCB(hsno, GMEP_US_HS_VER_IND, &hsVerInd);
                        break;
                    }
                    case GMEP_US_SESSION_CLOSE:
                    {
                        break;
                    }
                    case GMEP_US_URL_IND:
                    {

                        t_st_GmepSuotaUrlInd hsUrlInd;
                        i = 0;
                        u16 hsno;
                        memcpy(&hsno, &pGmepMsgResp->buf[i], 2);
                        i = i + 2;
                        hsUrlInd.u8_URLStoFollow = pGmepMsgResp->buf[i];
                        i = i + 1;
                        hsUrlInd.st_URL.u8_URLLen = pGmepMsgResp->buf[i];
                        i  = i  + 1;
                        hsUrlInd.st_URL.u8_URLLen = MIN(MAX_URL_SIZE, hsUrlInd.st_URL.u8_URLLen);
                        memcpy(&hsUrlInd.st_URL.u8_URL, &pGmepMsgResp->buf[i], hsUrlInd.st_URL.u8_URLLen);
                        i  = i + hsUrlInd.st_URL.u8_URLLen;



                        hsUrlInd.st_URL.u8_URL[hsUrlInd.st_URL.u8_URLLen] = 0;
                        SUOTA_INFO(" SUOTA: suota_app:GMEP_US_URL_IND %s\n", hsUrlInd.st_URL.u8_URL);

                        //if(gmepSessionTable.pGmepCPlaneCmdsCb)
                        //gmepSessionTable.pGmepCPlaneCmdsCb(SUOTA_URL_IND, &hsUrlInd);
                        cplane_CmdRecvdCB(hsno, GMEP_US_URL_IND, &hsUrlInd);
                        break;
                    }
                    case GMEP_US_NACK:
                    {
                        t_st_GmepSuotaNegAck hsNegAckInd;
                        i = 0;
                        memcpy(&hsNegAckInd.hsNo, &pGmepMsgResp->buf[i], 2);
                        i = i + 2;
                        hsNegAckInd.rejReason = pGmepMsgResp->buf[i];
                        i = i + sizeof(t_Suota_RejectReason);
                        //if(gmepSessionTable.pGmepCPlaneCmdsCb)
                        //gmepSessionTable.pGmepCPlaneCmdsCb(SUOTA_NACK, &hsNegAckInd);
                        cplane_CmdRecvdCB(hsNegAckInd.hsNo, GMEP_US_NACK, &hsNegAckInd);
                        break;
                    }
                    case GMEP_US_FACILITY_CB:
                    {
                        u16 hsNo;
                        u8 bFacilitySent;
                        int i = 0;
                        memcpy(&hsNo, &pGmepMsgResp->buf[i], 2);
                        i = i + 2;
                        memcpy(&bFacilitySent, &pGmepMsgResp->buf[i], 1);
                        cplane_FacilityCB(hsNo, bFacilitySent);
                        break;
                    }


                    case GMEP_US_HS_AVAIL_ACK:
                    {
                        int i = 0;
                        u16 hsNo;
                        memcpy(&hsNo, &pGmepMsgResp->buf[i], 2);
                        SUOTA_INFO("SUOTA: Rcvd From Target GMEP_US_HS_AVAIL_ACK hsNo %d \n", hsNo);
                        cplane_FillUrlMsg(hsNo);
                    }
                    break;

                    case GMEP_US_URL_IND_ACK:
                    {
                        int i = 0;
                        u16 hsNo;
                        memcpy(&hsNo, &pGmepMsgResp->buf[i], 2);
                        SUOTA_INFO("SUOTA:  case GMEP_US_URL_IND_ACK hsNo %d \n", hsNo);
                        cplane_FillUrlMsg((u8)hsNo);
                    }
                    break;

                    default:
                    {
                        break;
                    }
                }
            }
        }
        if (FD_ISSET(suotaapp[j].gtstRdFd, &suotaE))
        {
            //SUOTA_ERROR("SUOTA: MSGSWD ERROR fd_ctrl\n ");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(suotaRdFd, &suotaR))
        {
            SUOTA_INFO("SUOTA: Read From HTTP Thread Buffer  \n");
            suota_send_gmep();
        }
        if (FD_ISSET(suotaRdFd, &suotaE))
        {
            SUOTA_ERROR("SUOTA: ERROR suotaRdFd \n ");
            exit(EXIT_FAILURE);
        }
    }
}



//GMEP_LOG(" Message - %s, len -  %d \n", pGmepMsgResp->buf, pGmepMsgResp->bufLen);
//gmepSessionTable.pGmepCPlaneCmdsCb(pGmepMsgResp->cmd, NULL);
//ptemp = (unsigned int*)buf;
//GMEP_LOG("\n MSGSWD GMEP Reqt len %d Data %u,%u,%u,%u\n", ret,*ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3));
//return 0;


//c plane initialization
void  cplane_Init(void)
{
    //register a callback
    gmep_us_RegCPlaneCmdsCB(NULL, SUOTA_APPLICATION_ID);
    SUOTA_INFO("SUOTA:C Plane Initialization\n");
}

//http initialization
void   http_Init()
{
    SUOTA_INFO("SUOTA: HTTP Initialization\n");
}

void suota_PrintHex(u8 *buf, u32 len)
{
    u32 i;
    SUOTA_INFO("\nFile content starts \n");
    for (i = 0; i < len; i++)
        SUOTA_INFO(" %x ", *(buf + i));
    SUOTA_INFO("\nFile content ends \n");
}

void suota_PrintChar(u8 *buf, u32 len)
{
    u32 i;
    SUOTA_INFO("\n");
    for (i = 0; i < len; i++)
        SUOTA_INFO(" %c ", *(buf + i));
    SUOTA_INFO("\n");
}

void substring(int i, int j, char *ch)
{
    SUOTA_INFO("The substring is: %.*s\n", j - i, &ch[i]);
}

// Suota callback for HTTP request and response
void suota_CallBack(t_GmepCmd cmd, void *pData, u32 sessionid)
{
    char buffer[CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1];

    switch (cmd)
    {
        case GMEP_US_SESSION_CREATE_ACK:
        {
            int i, ret;
            t_Suota_GmepSessionTbl *pSession = NULL;
            char buffer1[CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1];
            char str5[100];
            pthread_attr_t  attr;
            SUOTA_INFO(" SUOTA:GMEP_US_SESSION_CREATE_ACK \n");
            for (i = 0; i < GMEP_MAX_SESSIONS; i++)
            {
                if (suotaSessionTbl[i].sesStatus == SUOTA_GMEP_SESSION_IDLE)
                {
                    suotaSessionTbl[i].sessionId = sessionid;
                    suotaSessionTbl[i].sesStatus = SUOTA_GMEP_SESSION_ACTIVE;
                    pSession = &suotaSessionTbl[i];
                    break;
                }
            }

            if (pSession != NULL)
            {

                pSession->sesStatus = SUOTA_GMEP_SESSION_DOWNLOAD;
                snprintf(buffer1, (CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1), "/tmp/phread-%05d", (getpid() + sessionid));
                ret = mkfifo(buffer1, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

                if (ret < 0)
                {
                    SUOTA_ERROR("SUOTA: creadting pthread ERROR: couldn't mkfifo - %d, %s\n", ret, buffer1);
                    exit(1);
                }
                pSession->threadRdFd = fifo_open_read(buffer1);
                pSession->threadWrFd = fifo_open_write(buffer1);
                pSession->sessionId = sessionid;
                strncpy(pSession->local, str5, sizeof(pSession->local) - 1);
                pSession->httpRequest.appSduLen = 0;//pHttpReq->appSduLen;
                pSession->httpRequest.appSdu = NULL;//malloc(pSession->httpRequest.appSduLen);
                //memcpy(pSession->httpRequest.appSdu, pHttpReq->appSdu, pSession->httpRequest.appSduLen);
                pthread_attr_init(&attr);
                //pthread_attr_setstacksize(&attr, RTP_THREAD_STACKSIZE);
                ret = pthread_create(&pSession->receiveThread, &attr, (void*)(*suota_httpRequestThread), pSession);

                pthread_attr_destroy(&attr);
                if (ret < 0)
                {
                    fprintf(stderr, "%s(): could not create suota receiver thread\n",
                            __FUNCTION__);
                }
            }
            else
                SUOTA_INFO("SUOTA: Invalid data received from session \n ");

            break;
        }
        case GMEP_US_SESSION_CLOSE_ACK:
        {
            int i;
            // Initialize parameters
            u32_NumOfBytesRead = 0;
            u32_TotalFileSize = 0;
            //delete created file here
            tcx_fileDelete(CMBS_SUOTA_LOCAL_FILE_NAME);

            //SUOTA_INFO(" SUOTA : GMEP_US_SESSION_CLOSE_ACK 2 \n");
            for (i = 0; i < GMEP_MAX_SESSIONS; i++)
            {
                if ((suotaSessionTbl[i].sesStatus >= SUOTA_GMEP_SESSION_ACTIVE) && (suotaSessionTbl[i].sessionId == (s32)sessionid))
                {
                    close(suotaSessionTbl[i].threadRdFd);
                    close(suotaSessionTbl[i].threadWrFd);
                    close(suotaSessionTbl[i].socket);
                    snprintf(buffer, (CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1), "/tmp/phread-%05d", (getpid() + suotaSessionTbl[i].sessionId));
                    if (remove(buffer) == (-1))
                        SUOTA_ERROR("SUOTA: Error to remove buffer \n");
                    pthread_cancel(suotaSessionTbl[i].receiveThread);
                    pthread_join(suotaSessionTbl[i].receiveThread, NULL);
                    suotaSessionTbl[i].sessionId = 0;;
                    suotaSessionTbl[i].sesStatus = SUOTA_GMEP_SESSION_IDLE;
                    suotaSessionTbl[i].threadRdFd = -1;
                    suotaSessionTbl[i].threadWrFd = -1;
                    SUOTA_INFO("SUOTA: HTTP File download is completed \n");
                    break;
                }
            }
            break;
        }

        case GMEP_US_DATA_RECV:
        {
            t_st_GmepSdu     * pHttpReq;
            //t_st_GmepSdu httpReq;
            int i;
            FILE *fp2;
            //pthread_attr_t  attr;
            char lineRd[500];
            //t_Suota_GmepSessionTbl *pSession = NULL;
            SUOTA_INFO(" SUOTA: suota_CallBack :SUOTA_DATA_RECV \n");
            //char *pch;
            //char *p2ch;
            int offset;
            //char *p1ch;
            //char *p3ch;
            t_st_GmepAppMsgCmd threadMsgReq;
            //char str4[100],str5[100],str6[100];
            pHttpReq = (t_st_GmepSdu *)pData;
            //char buffer1[CMBS_SUOTA_PATH_MAX+CMBS_SUOTA_NAME_MAX+1];
            if (suotaRdCfg == 1)
            {
                fp2 = fopen("/etc/suota.cfg", "r");
                if (fp2 == NULL)
                {
                    SUOTA_ERROR("SUOTA : /etc/suota.cfg file does not exist \n");
                    exit(1);
                }
                offset = 0;
                while (fgets(lineRd, (500 - offset), fp2) != NULL)
                {
                    strncpy((line + offset), lineRd, (strlen(lineRd) - 1));
                    offset = offset + strlen(lineRd) - 1;
                    strncpy((line + offset), "\r\n", strlen("\r\n") + 1);
                    offset = offset + strlen("\r\n");
                }
                strncpy((line + offset), "\r\n", strlen("\r\n") + 1);
                offset = offset + strlen("\r\n");
                *(line + offset) = '\0';
                fclose(fp2);
                pHttpReq->appSduLen = strlen(line);
                memcpy(pHttpReq->appSdu, line, pHttpReq->appSduLen);
            }
#if 0
            for (i = 0; i < GMEP_MAX_SESSIONS; i++)
            {
                if (suotaSessionTbl[i].sessionId == sessionid)
                {
                    SUOTA_INFO(" SUOTA: 1 \n");
                    if (suotaSessionTbl[i].sesStatus != SUOTA_GMEP_SESSION_IDLE)
                    {
                        SUOTA_INFO(" SUOTA: 2 \n");
                        while (suotaSessionTbl[i].sesStatus == SUOTA_GMEP_SESSION_DOWNLOAD)
                        {
                            usleep(1);
                            SUOTA_INFO(" SUOTA:3 \n");
                        }
                        SUOTA_INFO(" SUOTA: 4 \n");
                        pSession = &suotaSessionTbl[i];
                    }
                    break;
                }
            }
            SUOTA_INFO(" SUOTA: GMEP_US_DATA_RECV - %d \n", sessionid);
            if (pSession != NULL)
            {
                //SUOTA_INFO(" SUOTA: 5 \n");
                snprintf(buffer1, (CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1), "/tmp/phread-%05d", (getpid() + sessionid));
                ret = mkfifo(buffer1, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
                //SUOTA_INFO(" SUOTA: 6 \n");
                if (ret < 0)
                {
                    SUOTA_ERROR("SUOTA: creating pthread ERROR: couldn't mkfifo -%d, %s", ret, buffer1);
                    exit(1);
                }
                SUOTA_INFO(" SUOTA: 7 \n");
                pSession->threadRdFd = fifo_open_read(buffer1);
                pSession->threadWrFd = fifo_open_write(buffer1);
                pSession->sessionId = sessionid;
                strcpy(pSession->local, str5);
                //SUOTA_INFO(" SUOTA: 8 \n");
                pSession->httpRequest.appSduLen = pHttpReq->appSduLen;
                pSession->httpRequest.appSdu = malloc(pSession->httpRequest.appSduLen);
                SUOTA_INFO(" SUOTA: 9 \n");
                memcpy(pSession->httpRequest.appSdu, pHttpReq->appSdu, pSession->httpRequest.appSduLen);
                pthread_attr_init(&attr);
                //pthread_attr_setstacksize(&attr, RTP_THREAD_STACKSIZE);
                ret = pthread_create(&pSession->receiveThread, &attr, suota_httpRequestThread, pSession);
                pthread_attr_destroy(&attr);
                if (ret < 0)
                {
                    fprintf(stderr, "%s(): could not create suota receiver thread\n",
                            __FUNCTION__);
                    return ret;
                }
                SUOTA_INFO(" SUOTA: 10 \n");
            }
            else
                SUOTA_INFO("SUOTA: Invalid data received from session \n ");
#else
            memset(&threadMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
            for (i = 0; i < GMEP_MAX_SESSIONS; i++)
            {
                if ((suotaSessionTbl[i].sesStatus == SUOTA_GMEP_SESSION_DOWNLOAD) && (suotaSessionTbl[i].sessionId == (s32)sessionid))
                {
                    threadMsgReq.cmd = GMEP_US_DATA_RECV;
                    threadMsgReq.appSessionId = sessionid;
                    memcpy(&threadMsgReq.buf[0], pHttpReq->appSdu, pHttpReq->appSduLen);
                    threadMsgReq.bufLen = pHttpReq->appSduLen;

                    SUOTA_INFO(" SUOTA: suota_CallBack :SUOTA_DATA_RECV to FIFO http thread len %d", threadMsgReq.bufLen);
                    Suota_fifo_send(suotaSessionTbl[i].threadWrFd, &threadMsgReq);
                    break;
                }
            }
#endif
            break;
        }

        case GMEP_US_DATA_SEND_ACK:
        {
            int i;
            t_st_GmepAppMsgCmd threadMsgReq;
            memset(&threadMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));

            SUOTA_INFO(" SUOTA: suota_CallBack :GMEP_US_DATA_SEND_ACK \n ");

            //while(1);
            for (i = 0; i < GMEP_MAX_SESSIONS; i++)
            {
                if ((suotaSessionTbl[i].sesStatus == SUOTA_GMEP_SESSION_DOWNLOAD) && (suotaSessionTbl[i].sessionId == (s32)sessionid))
                {
                    threadMsgReq.cmd = GMEP_US_DATA_SEND_ACK;
                    threadMsgReq.appSessionId = sessionid;
                    memcpy(&threadMsgReq.buf[0], pData, 1);
                    threadMsgReq.bufLen = 1;
                    SUOTA_INFO(" SUOTA: suota_CallBack :GMEP_US_DATA_SEND_ACK send ack to http thread \n");
                    Suota_fifo_send(suotaSessionTbl[i].threadWrFd, &threadMsgReq);
                    break;
                }
            }
            //SUOTA_INFO(" SUOTA: GMEP_US_DATA_SEND_ACK - %d \n", sessionid);
            break;
        }
        case GMEP_US_SESSION_CLOSE:
        {
            break;
        }
        default:
        {
            SUOTA_INFO("SUOTA: Incorrect HTTP Command\n");
            break;
        }
    }
}


/**
 *
 *
 * @brief - Handset can request any length in HTTP Get Request -
 *     we modify length to adjust to CMBS capabilities.
 *
 * @return int
 *  0 on Success
 */
int modifyRangeHttpHdr(char *pRequestBuffer)
{
    char *pRangeStart = 0;
    char  *pReqPtr = 0;
    //char *pToBytePtr=0;
    int   ret = -1;
    int  from = 0, to, fromLength = 0;
    char newRange[100];
    char fromStr[10];
    int  inx;
    int  oldRangeSize, newRangeSize;

    char *pRangeString = "Range: bytes=";

    pRangeStart = strstr(pRequestBuffer, pRangeString);
    if (pRangeStart)
    {
        pReqPtr = pRangeStart + strlen(pRangeString);
        inx = 0;
        while (*pReqPtr != '\r' && *pReqPtr != '\n')// loop until range ends
        {
            fromStr[inx] = *pReqPtr;
            if (*pReqPtr == '-') //End of From Byte Number Chunk
            {
                fromStr[inx] = 0;
                from = atoi(fromStr);
                fromLength = strlen(fromStr);
                inx = 0;
            }
            else
                inx++;
            pReqPtr++;
        }
        fromStr[inx] = 0;
        to = atoi(fromStr);

        SUOTA_INFO("\nSUOTA: modifyRangeHttpHdr from<%d>  to<%d>\n", from, to);
        oldRangeSize = fromLength + strlen(fromStr);
        if (to - from > (MAX_GMEP_SDU_SIZE / 2))
        {
            to = from + (MAX_GMEP_SDU_SIZE / 2);
            SUOTA_INFO("SUOTA: modifyRangeHttpHdr NEW range from<%d>  to<%d>\n", from, to);
        }
        sprintf(fromStr, "%d", to); // new to size
        newRangeSize = fromLength + strlen(fromStr);
        SUOTA_INFO("SUOTA: modifyRangeHttpHdr newRangeSize <%d>  oldRangeSize<%d>\n", newRangeSize, oldRangeSize);

        if (newRangeSize < oldRangeSize)
        {
            sprintf(newRange, "Range: bytes=%d-%d ", from, to);
        }
        else
            sprintf(newRange, "Range: bytes=%d-%d", from, to);

        memcpy(pRangeStart, newRange, strlen(newRange)); // copy New Range
        SUOTA_INFO(" SUOTA: modifyRangeHttpHdr After range manipulation <%s> \n", pRequestBuffer);
        ret = 0;
    }
    return ret;
}


/**
 *
 *
 * @brief
 *  Create a new request ( modify range to adapt for CMBS ) and
 *  get the HTTP response... forward towards Target
 *
 * @param pMsgRx
 * @param pSession
 * @param pHttpresp
 */
void getNextHttpChunk(t_st_GmepAppMsgCmd *pMsgRx, t_Suota_GmepSessionTbl *pSession, t_st_GmepSdu *pHttpresp)
{
#ifndef SUOTA_LOCAL_FILE
    int ret;
    int cnt;
    int tmpres;
    int len;
    char *get;
    int resp;
    char str3[2000];
    char buf[MAX_GMEP_SDU_SIZE * 2];
    char *pch;
    char *p2ch;
    int  size_offset;
    int  offset;
    int  file_size = 0;
    char *ip ;
    char *host = NULL;
    struct sockaddr_in remote;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));

    do
    {


        ret = 0;
        cnt = 0;
        len = pMsgRx->bufLen;
        get = (char *)malloc(len);

        modifyRangeHttpHdr((char*)pMsgRx->buf);
        strncpy(get, (char*)pMsgRx->buf, len);
        pSession->httpRequest.appSduLen = pMsgRx->bufLen;
        pSession->httpRequest.appSdu = pMsgRx->buf;

        int sent = 0;
        while (sent < len)
        {
            tmpres = send(pSession->socket, get + sent, len - sent, 0);
            if (tmpres == -1)
            {
                perror("Can't send query \n");
                SUOTA_ERROR("SUOTA: getNextHttpChunk  ERROR Sending GMEP_US_DATA_RECV pMsgRx bufLen %d \n", pMsgRx->bufLen);
                exit(1);
            }
            sent += tmpres;
        }
        free(get);

        // e.t. change
        //usleep(1000);
        //resp=recv(pSession->socket, buf, sizeof(buf), MSG_WAITALL);
        resp = recv(pSession->socket, buf, sizeof(buf), 0);
        if (resp > 0)
        {
            pHttpresp->appSduLen = resp;
            SUOTA_INFO("SUOTA: getNextHttpChunk allocating  %d \n", pHttpresp->appSduLen);
            pHttpresp->appSdu = (u8 *)malloc(pHttpresp->appSduLen);
            memcpy(pHttpresp->appSdu, buf, pHttpresp->appSduLen);
            SUOTA_INFO(" SUOTA: suota_httpRequestThread  1 GMEP_US_DATA_RECV appSduLen  %d \n", pHttpresp->appSduLen);
            memcpy(str3, pHttpresp->appSdu, pHttpresp->appSduLen);
            str3[pHttpresp->appSduLen] = '\0';
            SUOTA_INFO(" SUOTA: getNextHttpChunk  GMEP_US_DATA_RECV appSduLen %d \n", pHttpresp->appSduLen);

            if ((strstr(str3, "HTTP/1.1 200 OK") != NULL) || (strstr(str3, "HTTP/1.0 200 OK") != NULL) || (strstr(str3, "HTTP/1.1 206") != NULL) || (strstr(str3, "HTTP/1.0 206") != NULL))
            {
                p2ch = strstr(str3, "GMT");
                offset = p2ch - str3;
                offset = offset + 3;
                cnt = cnt + (pHttpresp->appSduLen - offset);
                pch = strstr(str3, "Content-Length:");
                size_offset = pch - str3;
                size_offset = size_offset + 16;
                sscanf((str3 + size_offset), "%d", &file_size);
                //SUOTA_INFO(" ## SUOTA 1 ##\n");
                //SUOTA_INFO(" SUOTA: Send HTTP packet to HS : 1 - len - %d \n", pHttpresp.appSduLen);
                //suota_PrintHex(pHttpresp.appSdu, pHttpresp.appSduLen);
                thread_send_suota(pSession, pHttpresp);
                free(pHttpresp->appSdu);
            }
            else
            {
                SUOTA_ERROR(" SUOTA: getNextHttpChunk RESPONSE ERROR str3 %s\n", str3);
                cnt = cnt + (pHttpresp->appSduLen);
                thread_send_suota(pSession, pHttpresp);
                free(pHttpresp->appSdu);
            }
        }
        else
        {   /* recv returned an ERROR */
            SUOTA_INFO(" SUOTA: getNextHttpChunk recv returned error errno %d   resp %d\n", errno, resp);
            if (errno == EINTR)
            {
                SUOTA_INFO(" SUOTA: getNextHttpChunk recv returned EINTR \n");
            }
            close(pSession->socket);
            pSession->socket = create_tcp_socket();
            ip = get_ip(host);
            remote.sin_family = AF_INET;
            tmpres = inet_pton(AF_INET, ip, (void *)(&(remote.sin_addr.s_addr)));
            if (tmpres < 0)
            {
                perror("Can't set remote->sin_addr.s_addr");
                exit(1);
            }
            else if (tmpres == 0)
            {
                fprintf(stderr, "%s is not a valid IP address\n", ip);
                exit(1);
            }
            remote.sin_port = htons(suotaProxyPort);
            free(ip);

            if (connect(pSession->socket, (struct sockaddr *)&remote, sizeof(struct sockaddr)) < 0)
            {
                SUOTA_INFO(" SUOTA: getNextHttpChunk: Could not connect \n");
                exit(1);
            }
            ret = -1;


        }
    } while (ret != 0);
#else //SUOTA_LOCAL_FILE

    CURL *curl;
    CURLcode res;
    FILE *fpHSBinary = NULL;
    char *url = gSuotaMgmtConf.hsVerUrl.st_URL.u8_URL;

    if (tcx_fileOpen(&fpHSBinary, CMBS_SUOTA_LOCAL_FILE_NAME, "r") != CMBS_RC_OK)
    {
        // Indicates that this is the first request from the HS. In this case - download the whole file.
        SUOTA_INFO("This is the first request from HS. Download the file\n");
        curl = curl_easy_init();
        if (curl)
        {
            if (tcx_fileOpen(&fpHSBinary, CMBS_SUOTA_LOCAL_FILE_NAME, "wb") != CMBS_RC_OK)
            {
                SUOTA_ERROR("Can't open file ITCM_H\n");
                return;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fpHSBinary);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            tcx_fileClose(fpHSBinary);
        }
    }
    else
    {
        tcx_fileClose(fpHSBinary);
    }
    getNextChunk(pMsgRx, pSession, pHttpresp);
#endif //SUOTA_LOCAL_FILE
}

void getNextChunk(t_st_GmepAppMsgCmd *pMsgRx, t_Suota_GmepSessionTbl *pSession, t_st_GmepSdu *pHttpresp)
{
    // read file and construct message to send
    t_st_httpResponse st_Response;

    memset(&st_Response, 0, sizeof(t_st_httpResponse));

    suota_ConstructMessage(&st_Response);
    pSession->httpRequest.appSduLen = st_Response.responseBuffLen;
    pSession->httpRequest.appSdu = st_Response.responseBuff;
    pHttpresp->appSduLen = st_Response.responseBuffLen;
    SUOTA_INFO("SUOTA: getNextChunk allocating  %d \n", pHttpresp->appSduLen);
    pHttpresp->appSdu = st_Response.responseBuff;
    thread_send_suota(pSession, pHttpresp);
}

//#if 0
//recieves HTTP request from stub and connects to ffproxy and recieves response  and sends file content along wih response to stub.
//stores the response locally in "suotafile.txt"
void  suota_httpRequestThread(void *arg/*t_st_GmepSdu *Httpreq,u32 sessionid,char *str6*/)
{
    char str3[2000];
    char buf[MAX_GMEP_SDU_SIZE * 2];
    int tmpres;
    int resp;
    int file_size = 0;
    int cnt = 0 ;
    char *ip ;
    char *host = NULL;
    struct sockaddr_in remote;
    u8 resAck1;
    t_st_GmepAppMsgCmd msgRx;
    int retFifo;
    t_st_GmepSdu pHttpresp;

    t_Suota_GmepSessionTbl *pSession = (t_Suota_GmepSessionTbl *)arg;

    pSession->sesStatus = SUOTA_GMEP_SESSION_DOWNLOAD;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));

    SUOTA_INFO("suota_httpRequestThread created\n");

    pSession->socket = create_tcp_socket();
    ip = get_ip(host);
    remote.sin_family = AF_INET;
    tmpres = inet_pton(AF_INET, ip, (void *)(&(remote.sin_addr.s_addr)));
    if (tmpres < 0)
    {
        perror("Can't set remote->sin_addr.s_addr");
        exit(1);
    }
    else if (tmpres == 0)
    {
        fprintf(stderr, "%s is not a valid IP address\n", ip);
        exit(1);
    }
    remote.sin_port = htons(suotaProxyPort);
    free(ip);

    if (connect(pSession->socket, (struct sockaddr *)&remote, sizeof(struct sockaddr)) < 0)
    {
        SUOTA_ERROR(" SUOTA: suota_httpRequestThread: Could not connect \n");
        exit(1);
    }
    SUOTA_INFO("SUOTA: suota_httpRequestThread: tcp connection success \n");
    SUOTA_INFO("SUOTA: HTTP File download is started \n");
    while (1)
    {
        memset(&msgRx, 0, sizeof(t_st_GmepAppMsgCmd));
        retFifo = read(pSession->threadRdFd, &msgRx, sizeof(t_st_GmepAppMsgCmd));
        if (retFifo < 0)
        {
            SUOTA_INFO(" SUOTA: suota_httpRequestThread ERROR msg_fifo_read from SUOTA process \n");
            return;
        }
        switch (msgRx.cmd)
        {

            case GMEP_US_DATA_RECV:
            {
                getNextHttpChunk(&msgRx, pSession, &pHttpresp);
                break;
            }

            case GMEP_US_DATA_SEND_ACK:
            {
                SUOTA_INFO("SUOTA: suota_httpRequestThread GMEP_US_DATA_SEND_ACK  \n");

                resAck1 = 1;
                memcpy(&resAck1, &msgRx.buf[0], 1);
                if (resAck1 != 0)
                {
                    SUOTA_INFO("\n SUOTA:HTTP1: sent is not success and aborting download !!!!! \n");
                    pSession->sesStatus = SUOTA_GMEP_SESSION_ACTIVE;
                }
                else
                {
                    if ((cnt == file_size) || (cnt > file_size))
                    {
                    }
                    else
                    {
#ifndef SUOTA_LOCAL_FILE

                        resp = recv(pSession->socket, buf, sizeof(buf), MSG_WAITALL);

                        SUOTA_INFO("SUOTA: suota_httpRequestThread GMEP_US_DATA_SEND_ACK  recv return with %d \n", resp);
                        if (resp > 0)
                        {
                            pHttpresp.appSduLen = resp;
                            pHttpresp.appSdu = (u8 *)malloc(pHttpresp.appSduLen);
                            memcpy(pHttpresp.appSdu, buf, pHttpresp.appSduLen);
                            memcpy(str3, pHttpresp.appSdu, pHttpresp.appSduLen);
                            str3[pHttpresp.appSduLen] = '\0';
                            cnt = cnt + (pHttpresp.appSduLen);
                            thread_send_suota(pSession, &pHttpresp);
                            free(pHttpresp.appSdu);
                        }
                        else
                        {
                            t_st_GmepAppMsgCmd threadMsgReq;
                            memset(&threadMsgReq, 0, sizeof(t_st_GmepAppMsgCmd)); // e.t. change
                            threadMsgReq.cmd = GMEP_US_SESSION_CLOSE;
                            threadMsgReq.appSessionId = pSession->sessionId;
                            threadMsgReq.bufLen = 0;
                            Thread_Fifo_Send(&threadMsgReq);
                            SUOTA_INFO(" SUOTA: read error from tcp socket, breaks file download - middle - %d \n ", resp);
                        }
#endif
                    }
                }
                break;
            }

            default:
            {
                SUOTA_INFO(" SUOTA:suota_httpRequestThread: Incorrect SUOTA command: \n");
                break;
            }
        }
    }

    return;
}

void Thread_Fifo_Send(t_st_GmepAppMsgCmd   *httpMsg)
{
    int ret1;

    ret1 = fifo_write(suotaWrFd, httpMsg, sizeof(t_st_GmepAppMsgCmd));
    if (ret1 < 0)
    {
        SUOTA_ERROR("Thread_Fifo_Send error in writing\n");
        exit(1);
    }
    SUOTA_INFO("SUOTA: Thread_Fifo_Send t_st_GmepAppMsgCmd httpMsg wrote len %d\n", ret1);
}

void thread_send_suota(t_Suota_GmepSessionTbl *pSession, t_st_GmepSdu *HttpResp)
{
    t_st_GmepAppMsgCmd threadMsgReq;

    memset(&threadMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    threadMsgReq.cmd = GMEP_US_DATA_SEND;
    threadMsgReq.appSessionId = pSession->sessionId;
    threadMsgReq.bufLen = HttpResp->appSduLen;

    SUOTA_INFO("SUOTA: thread_send_suota sending threadMsgReq.bufLen %d\n", threadMsgReq.bufLen);
    memcpy(&threadMsgReq.buf, HttpResp->appSdu, HttpResp->appSduLen);
    Thread_Fifo_Send(&threadMsgReq);
}

void suota_send_gmep()
{
    int ret;
    char buf[sizeof(t_st_GmepAppMsgCmd)];
    //t_st_GmepSdu HttpResp;
    t_st_GmepAppMsgCmd *pGmepMsg;

    SUOTA_INFO("suota_send_gmep \n");
    ret = fifo_read(suotaRdFd, buf, (sizeof(t_st_GmepAppMsgCmd)));
    if (-1 == ret)
        return;

    pGmepMsg = (t_st_GmepAppMsgCmd*)buf;
    switch (pGmepMsg->cmd)
    {
        case GMEP_US_DATA_SEND:
        {
            u16 requestId = 1;
            SUOTA_INFO("suota_send_gmep GMEP_US_DATA_SEND len %d\n", pGmepMsg->bufLen);
#ifdef CMBS_API
            //HttpResp.appSdu = &pGmepMsg->buf[0];
            cmbs_dsr_suota_DataSend(NULL, pGmepMsg->appSessionId, pGmepMsg->appSessionId,
                                    (char*)pGmepMsg->buf, pGmepMsg->bufLen, requestId);
#else
            HttpResp.appSduLen = pGmepMsg->bufLen;
            HttpResp.appSdu = &pGmepMsg->buf[0];
            gmep_us_GmepSduSend(pGmepMsg->appSessionId, &HttpResp);
#endif
            break;
        }
        case GMEP_US_SESSION_CLOSE:
        {
            gmep_us_SessionClose(pGmepMsg->appSessionId);
            break;
        }
        default:
        {
            SUOTA_INFO(" SUOTA:suota_send_gmep: Invalid cmd from thread \n");
            break;
        }
    }
    return;
}

void Suota_fifo_send(int fd, t_st_GmepAppMsgCmd *pMsg)
{
    int ret;

    ret = fifo_write(fd, pMsg, sizeof(t_st_GmepAppMsgCmd));
    if (ret < 0)
    {
        SUOTA_ERROR("SUOTA:Suota_fifo_send: error in writing \n");
        exit(1);
    }
}

char *get_ip(char *host1)
{
    struct hostent *hent;
    int iplen = 15; //XXX.XXX.XXX.XXX
    char *ip = (char *)malloc(iplen + 1);

    UNUSED_PARAMETER(host1);
    memset(ip, 0, iplen + 1);

    SUOTA_INFO("SUOTA: get_ip ip %s  port %d\n", suotaProxyIp, suotaProxyPort);
    if ((hent = gethostbyname(suotaProxyIp)) == NULL)
    {
        herror("SUOTA: Can't get IP\n");
        exit(1);
    }

    if (inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
    {
        perror("SUOTA: Can't resolve host\n");
        exit(1);
    }
    return ip;
}

int create_tcp_socket()
{
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("SUOTA: Can't create TCP socket\n");
        exit(1);
    }

    return sock;
}

//open fifo for writing
int fifo_open_write(char *name)
{
    int filed;

    char buffer1[MSG_BUF_SIZE];
    snprintf(buffer1, MSG_BUF_SIZE, "%s",  name);

    filed = open(buffer1, O_WRONLY);

    if (filed < 0)
    {

        if (errno == ENXIO)
            SUOTA_INFO("SUOTA: ERROR: client %s dead already?\n", name);
        else
            SUOTA_INFO("SUOTA:ERROR: couldn't open %s\n", buffer1);
    }
    return filed;

}

int fifo_open_read(char *name)
{
    int filed;
    char buffer1[MSG_BUF_SIZE];

    snprintf(buffer1, MSG_BUF_SIZE, "%s",  name);
    //open for write
    filed = open(buffer1, O_RDWR);

    if (filed < 0)

    {

        if (errno == ENXIO)
            SUOTA_ERROR("SUOTA: ERROR: client %s dead already?\n", name);

        else
            SUOTA_ERROR("SUOTA:ERROR: couldn't open %s\n", buffer1);
    }
    return filed;
}

//read fn
int fifo_read(int fd, char *buf, int len)
{
    int ret;
    ret = read(fd, buf, len);
    return ret;
}

//write fn
int fifo_write(int fd, const void *buf, int len)
{
    int ret;
    ret = write(fd, buf, len);
    return ret;
}
#ifdef SUOTA_LOCAL_FILE
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}
#endif //SUOTA_LOCAL_FILE
