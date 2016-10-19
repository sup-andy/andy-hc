/*
 * GMEP US API: C file
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>


#ifndef CMBS_API
#include "netlink.h"
#else
#include "cmbs_api.h"
#include "cmbs_int.h"
#include "cfr_ie.h"
#endif

#include <errno.h>
#include <dirent.h>
#include <unistd.h>

#include "gmep-us.h"
//#include "msgswitchd.h"

int appRdFrmGmepdFd = -1;
int appWrToGmepdWFd  = -1;

static cbFacility g_gmepUsFacilityCB = NULL;

int gmep_us_Write(t_st_GmepAppMsgCmd *pData)
{
#ifndef CMBS_API
    int ret;

    ret = msg_fifo_write(appWrToGmepdWFd, (char *)pData, sizeof(t_st_GmepAppMsgCmd));
    if (ret < 0)
    {
        GMEP_LOG(" msg_fifo_write() failed -%d  ", ret);
        return -1;
    }
#else
    PST_CFR_IE_LIST p_List;
    E_CMBS_EVENT_ID e_EventID;
    ST_IE_DATA      st_Data;

    p_List = (PST_CFR_IE_LIST)cmbs_api_ie_GetList();
    e_EventID = pData->cmd;
    cmbs_api_ie_u32ValueAdd(p_List, pData->appId, CMBS_IE_SUOTA_APP_ID);
    cmbs_api_ie_u32ValueAdd(p_List, pData->appSessionId, CMBS_IE_SUOTA_SESSION_ID);
    st_Data.u16_DataLen = pData->bufLen;
    st_Data.pu8_Data    = pData->buf;
    cmbs_api_ie_DataAdd(p_List, &st_Data);
    cmbs_int_EventSend(e_EventID, p_List->pu8_Buffer, p_List->u16_CurSize);
#endif
    return 0;

}

int gmep_us_Read(t_st_GmepAppMsgCmd *pData)
{
#ifndef CMBS_API
    int ret;

    ret = msg_fifo_read(appRdFrmGmepdFd, (char *)pData, sizeof(t_st_GmepAppMsgCmd));
    if (ret < 0)
    {
        GMEP_LOG("  msg_fifo_read(GMEP) returned %d: ", ret);
        return -1;
    }
#else
    UNUSED_PARAMETER(pData);
#endif
    return 0;
}

#if 0
int cplane_CmdRecvdCB(t_GmepCmd suotaCommand,  void * pData)
{

    GMEP_LOG(" cplane_CmdRecvdCB called !!.. \n");
    return 0;
}


u8 gmep_us_msg_fifo_read(char *buf, int len)
{
    int ret;
    u8 result = 1;
    t_st_GmepAppMsgCmd *pGmepMsgResp;
    ret = msg_fifo_read(gmepSessionTable.gmepReadFd, buf, len);
    if (ret < 0)
    {
        GMEP_LOG("  msg_fifo_read(GMEP) returned %d: ", ret);
        exit(0);
    }
    pGmepMsgResp = (t_st_GmepAppMsgCmd *)buf;

    switch (pGmepMsgResp->cmd)
    {
        case GMEP_US_DATA_SEND_ACK:
        {
            GMEP_LOG("\ngmep_us_msg_fifo_read : MGMEP_US_DATA_SEND_ACK - %d \n", pGmepMsgResp->cmd);
            memcpy(&result, pGmepMsgResp->buf, 1);
            break;
        }

        default:
            GMEP_LOG("  default: Message type rxed - %d \n", pGmepMsgResp->cmd);
            break;
    }

    return result;
}
#endif



#if 0
int gmep_us_Callback()
{
    int ret;
    char buf[MSG_BUF_SIZE];
    unsigned int *ptemp = NULL;
    t_st_GmepAppMsgCmd *pGmepMsgResp;
    char str[20];
    int i  = 0;

    ret = msg_fifo_read(gmepSessionTable.gmepReadFd, buf, MSG_BUF_SIZE);
    if (ret < 0)
    {
        GMEP_LOG("  msg_fifo_read(GMEP) returned %d: ", ret);
        exit(0);
    }

    pGmepMsgResp = (t_st_GmepAppMsgCmd *)buf;
    GMEP_LOG(" Message type rxed - %d \n", pGmepMsgResp->cmd);

    switch (pGmepMsgResp->cmd)
    {
        case GMEP_US_SESSION_CREATE:
        {

            break;
        }

        case GMEP_US_OPEN_SESSION:
        {

            break;
        }
        case GMEP_US_DATA_RECV:
        {
            t_st_GmepSdu sduData;
            i = 0;

            sduData.appSduLen = pGmepMsgResp->bufLen;
            sduData.appSdu = (u8 *)malloc(pGmepMsgResp->bufLen);
            memcpy(sduData.appSdu, &pGmepMsgResp->buf[i], pGmepMsgResp->bufLen);

            //GMEP_LOG("\n GMEP-US: %d %d %d  &&&& ", pGmepMsgResp->buf[3999], pGmepMsgResp->buf[3998], pGmepMsgResp->buf[3997]);

            if (gmepSessionTable.pGmepAppCallback)
                gmepSessionTable.pGmepAppCallback(SUOTA_DATA_RECV, &sduData);

            free(sduData.appSdu);
            break;
        }

        case GMEP_US_DATA_SEND_ACK:
        {
            u8 result;

            memcpy(&result, pGmepMsgResp->buf, 1);
            if (gmepSessionTable.pGmepAppCallback)
                gmepSessionTable.pGmepAppCallback(SUOTA_DATA_SEND_ACK, &result);

            break;
        }

        case GMEP_US_HS_VER_IND:
        {
            t_st_GmepSuotaHsVerInd hsVerInd;
            u16 hsno;


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
            memcpy(&hsVerInd.st_SwVer.u8_Buffer, &pGmepMsgResp->buf[i], hsVerInd.st_SwVer.u8_BuffLen);
            i = i + hsVerInd.st_SwVer.u8_BuffLen;
            hsVerInd.st_HwVer.u8_BuffLen = pGmepMsgResp->buf[i];
            i = i  + 1;
            memcpy(&hsVerInd.st_HwVer.u8_Buffer, &pGmepMsgResp->buf[i], hsVerInd.st_HwVer.u8_BuffLen);
            i = i + hsVerInd.st_HwVer.u8_BuffLen;

            //pGmepMsgResp->bufLen=i;



            if (gmepSessionTable.pGmepCPlaneCmdsCb)
                gmepSessionTable.pGmepCPlaneCmdsCb(hsno, SUOTA_HS_VER_IND, &hsVerInd);

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
            memcpy(&hsUrlInd.st_URL.u8_URL, &pGmepMsgResp->buf[i], hsUrlInd.st_URL.u8_URLLen);
            i  = i + hsUrlInd.st_URL.u8_URLLen;

            if (gmepSessionTable.pGmepCPlaneCmdsCb)
                gmepSessionTable.pGmepCPlaneCmdsCb(hsno, SUOTA_URL_IND, &hsUrlInd);

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

            if (gmepSessionTable.pGmepCPlaneCmdsCb)
                gmepSessionTable.pGmepCPlaneCmdsCb(hsNegAckInd.hsNo, SUOTA_NACK, &hsNegAckInd);

            break;
        }

        case GMEP_US_FACILITY_CB:
        {
            i = 0;
            u16 hsNo;
            u8 bFacilitySent;

            memcpy(&hsNo, &pGmepMsgResp->buf[i], 2);
            i = i + 2;
            memcpy(&bFacilitySent, &pGmepMsgResp->buf[i], 1);
            i = i + 1;

            g_gmepUsFacilityCB(hsNo, bFacilitySent);

            break;
        }
        default:
        {
            break;
        }
    }

    //GMEP_LOG(" Message - %s, len -  %d \n", pGmepMsgResp->buf, pGmepMsgResp->bufLen);
    //gmepSessionTable.pGmepCPlaneCmdsCb(pGmepMsgResp->cmd, NULL);
    //ptemp = (unsigned int*)buf;
    //GMEP_LOG("\n MSGSWD GMEP Reqt len %d Data %u,%u,%u,%u\n", ret,*ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3));
    return 0;
}
#endif

int gmep_us_RegCPlaneCmdsCB(cbSuotaCmd pcbSuotaCb, u32 appId)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    UNUSED_PARAMETER(pcbSuotaCb);

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));

    cfr_ie_ser_u32((u8 *)&gmepMsgReq.appId, appId);

    gmepMsgReq.cmd = GMEP_US_REG_CPLANE_CB;

    gmep_us_Write(&gmepMsgReq);
    return 0;
}

int gmep_us_RegAppCB(gmepAppCB pcbAppCb, u32 appId)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    UNUSED_PARAMETER(pcbAppCb);
    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_REG_APP_CB;
    cfr_ie_ser_u32(&gmepMsgReq.appId, appId);
    gmep_us_Write(&gmepMsgReq);
    return 0;
}

u8  gmep_us_SessionCreate(u32 appId, u16 hsNo)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_SESSION_CREATE;

    gmepMsgReq.appId = appId;

    memcpy(&gmepMsgReq.buf[0], &hsNo, 2);
    gmepMsgReq.bufLen = 2;
    gmep_us_Write(&gmepMsgReq);

    return 0;
}

int gmep_us_SessionClose(u32 sessionId)
{
    t_st_GmepAppMsgCmd gmepMsgReq;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_SESSION_CLOSE;

    gmepMsgReq.appSessionId = sessionId;

    gmepMsgReq.bufLen = 0;
    gmep_us_Write(&gmepMsgReq);
    return 0;
}

int gmep_us_test(void)
{
#if 0
    t_st_GmepSuotaHsVerInd hsVerAvail;
    u8 swVer[50];
    u8 hwVer[50];

    strcpy(swVer, "SOFTWARE");
    strcpy(hwVer, "HARDWARE");

    hsVerAvail.hsNo = 1;
    hsVerAvail.emc = 2;
    hsVerAvail.urlsToFollow = 10;
    hsVerAvail.fileNumber = 4;
    hsVerAvail.flags =  5;
    hsVerAvail.reason = 6;
    hsVerAvail.swVerLen = strlen(swVer) + 1;
    hsVerAvail.pSwVer = swVer;
    hsVerAvail.hwVerLen = strlen(hwVer) + 1;
    hsVerAvail.pHwVer = hwVer;

    gmep_us_SendHandVerAvail(&hsVerAvail);
#endif
    return 0;
}

int gmep_us_SendHandVerAvail(t_st_GmepSuotaHsVerAvail *pData, u16 u16_Handset, cbFacility pFacilityCB)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    int i = 0;
    //u16 hsno;
    //hsno=(u16)u16_Handset;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_HS_AVAIL;

    memcpy(&gmepMsgReq.buf[i], &u16_Handset/*&hsno &pData->hsNo*/, 2);
    i = i + 2;
    memcpy(&gmepMsgReq.buf[i], &pData->st_Header.u16_delayInMin, 2);
    i = i + 2;
    memcpy(&gmepMsgReq.buf[i], &pData->st_Header.u8_URLStoFollow, 1);
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], &pData->st_Header.u8_Spare, 1);
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], &pData->st_Header.u8_UserInteraction, 1);
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], &pData->st_SwVer.u8_BuffLen, 1);
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], pData->st_SwVer.u8_Buffer, pData->st_SwVer.u8_BuffLen);
    i = i + pData->st_SwVer.u8_BuffLen;

    gmepMsgReq.bufLen = i;

    g_gmepUsFacilityCB = pFacilityCB;
    GMEP_LOG(" buf len - %d ", gmepMsgReq.bufLen);
    gmep_us_Write(&gmepMsgReq);

    return 0;
}

int gmep_us_PushModeSend(void)
{
    t_st_GmepAppMsgCmd gmepMsgReq;


    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_PUSH_MODE;
    gmep_us_Write(&gmepMsgReq);

    return 0;
}

int gmep_us_SendUrl(u16 u16_Handset, t_st_GmepSuotaUrlInd *pData, cbFacility pFacilityCB)
{

    t_st_GmepAppMsgCmd gmepMsgReq;
    int i = 0;

    //u16 u16_hsno;
    //u16_hsno=(u16)u16_Handset;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));

    // CMBS REMOVE
    //gmepMsgReq.cmd = GMEP_US_SEND_URL_IND; //GMEP_US_URL_IND;
    gmepMsgReq.cmd = GMEP_US_URL_IND; //;

    memcpy(&gmepMsgReq.buf[i], &u16_Handset/*&pData->hsNo*/, 2);
    i = i + 2;
    memcpy(&gmepMsgReq.buf[i], &pData->u8_URLStoFollow, 1);
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], &pData->st_URL.u8_URLLen, 1);
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], pData->st_URL.u8_URL, pData->st_URL.u8_URLLen);
    i = i + pData->st_URL.u8_URLLen;

    gmepMsgReq.bufLen = i;
    GMEP_LOG(" buf len - %d ", gmepMsgReq.bufLen);
    g_gmepUsFacilityCB = pFacilityCB;
    gmep_us_Write(&gmepMsgReq);
    return 0;
}

int gmep_us_SendNegAck(u16 u16_Handset, t_st_GmepSuotaNegAck *negAck, cbFacility pFacilityCB)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    int i = 0;

    //u16 hsno;
    //hsno=(u16)u16_Handset;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_NACK;

    memcpy(&gmepMsgReq.buf[i], &u16_Handset/*&negAck->hsNo*/, 2);
    i = i + 2;
    memcpy(&gmepMsgReq.buf[i], &negAck->rejReason, 4);
    i = i + sizeof(t_Suota_RejectReason);

    gmepMsgReq.bufLen = i;
    GMEP_LOG(" buf len - %d ", gmepMsgReq.bufLen);
    g_gmepUsFacilityCB = pFacilityCB;
    gmep_us_Write(&gmepMsgReq);
    return 0;

}


int gmep_us_GmepSduSend(u32 sessionId, t_st_GmepSdu *pGmepSdu)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_DATA_SEND;
    gmepMsgReq.appSessionId = sessionId;

    GMEP_LOG("gmep-us gmep_us_GmepSduSend pGmepSdu->appSduLen %d \n", pGmepSdu->appSduLen);
    gmepMsgReq.bufLen = pGmepSdu->appSduLen;
    memcpy(gmepMsgReq.buf, pGmepSdu->appSdu, pGmepSdu->appSduLen);

    gmep_us_Write(&gmepMsgReq);
    return 0;

}

int gmep_us_UpdateControlSet(u32 sessionId, t_st_GmepControlSet *pControlSet)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    int i = 0;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));

    gmepMsgReq.cmd = GMEP_US_CONTROL_SET;
    gmepMsgReq.appSessionId = sessionId;

    gmepMsgReq.buf[i] = pControlSet->spare;
    i = i + 1;
    gmepMsgReq.buf[i] = pControlSet->chop;
    i = i + 1;
    gmepMsgReq.buf[i] = pControlSet->optionalGrp;
    i = i + 1;
    gmepMsgReq.buf[i] = pControlSet->operationalCode;
    i = i + 1;
    gmepMsgReq.buf[i] = pControlSet->cntrlseText;
    i = i + 1;
    gmepMsgReq.buf[i] = pControlSet->gmci;
    i = i + 1;
    gmepMsgReq.buf[i] = pControlSet->seqNo;
    i = i + 1;
    memcpy(&gmepMsgReq.buf[i], &pControlSet->pid, 2);
    i = i + 2;

    gmepMsgReq.bufLen = i;
    GMEP_LOG(" buf len - %d ", gmepMsgReq.bufLen);
    gmep_us_Write(&gmepMsgReq);
    return 0;
}

int gmep_us_ResetControlSet(u32 sessionId)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    int i = 0;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));
    gmepMsgReq.cmd = GMEP_US_CONTROL_SET;
    gmepMsgReq.appSessionId = sessionId;


    gmepMsgReq.bufLen = i;
    GMEP_LOG(" buf len - %d ", gmepMsgReq.bufLen);
    gmep_us_Write(&gmepMsgReq);
    return 0;
}


int gmep_us_UpdateOptionalGroup(u32 sessionId, t_st_GmepOptionalGroup *pGmepOS)
{
    t_st_GmepAppMsgCmd gmepMsgReq;
    int i = 0;

    memset(&gmepMsgReq, 0, sizeof(t_st_GmepAppMsgCmd));

    gmepMsgReq.cmd = GMEP_US_UPDATE_OPTIONAL_GRP;
    gmepMsgReq.appSessionId = sessionId;

    memcpy(&gmepMsgReq.buf[i], &pGmepOS->ipSource, 4);
    i = i + 4;
    memcpy(&gmepMsgReq.buf[i], &pGmepOS->ipDestination, 4);
    i = i + 4;
    memcpy(&gmepMsgReq.buf[i], &pGmepOS->portNumSrc, 2);
    i = i + 2;
    memcpy(&gmepMsgReq.buf[i], &pGmepOS->portNumDest, 2);
    i = i + 2;

    gmepMsgReq.bufLen = i;
    GMEP_LOG(" buf len - %d ", gmepMsgReq.bufLen);
    gmep_us_Write(&gmepMsgReq);
    return 0;
}

#if 0
int fifo_open_write(char *name)
{
    int filed;
    char buffer1[max_buf_size];

    snprintf(buffer1, max_buf_size, "%s",  name);

    filed = open(buffer1, O_WRONLY | O_NONBLOCK);
    if (filed < 0)
    {
        if (errno == ENXIO)
            GMEP_LOG("ERROR: client %s dead already?", name);
        else
            GMEP_LOG("ERROR: couldn't open %s", buffer1);
    }

    return filed;
}

int fifo_open_read(char *name)
{
    int filed;
    char buffer1[max_buf_size];

    snprintf(buffer1, max_buf_size, "%s",  name);

    filed = open(buffer1, O_RDWR | O_SYNC);
    if (filed < 0)
    {
        if (errno == ENXIO)
            GMEP_LOG("ERROR: client %s dead already?", name);
        else
            GMEP_LOG("ERROR: couldn't open %s", buffer1);
    }

    return filed;
}

int fifo_read(int fd, char *buf, int len)
{
    int ret;
    ret = read(fd, buf, len);
    return ret;
}


int fifo_write(int fd, const void *buf, int len)
{
    int ret;
    ret = write(fd, buf, len);
    return ret;
}
#endif

int gmep_us_MsgFifoInit(const char *fifo_name)
{
    int ret;
    char buf[CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1];

    /* mkfifo */
    snprintf(buf, CMBS_SUOTA_PATH_MAX + CMBS_SUOTA_NAME_MAX + 1, "%s/%s", GMEPD_DIR, fifo_name);
    if (mkdir(GMEPD_DIR, S_IRWXG | S_IRWXU | S_IRWXO) == (-1))
    {
        GMEP_LOG("ERROR: couldn't mkdir \n");
    }
    ret = mkfifo(buf, S_IRWXU | S_IRWXG);

    if ((ret < 0) && (errno != EEXIST))
    {
        GMEP_LOG("ERROR: couldn't mkfifo(\"%s\"): %s", buf, strerror(errno));
        return ret;
    }

    /* open and return a reader file handle *
     * for some reason we must open(RDWR) here,
     * with RDONLY select() always returns with 0 data */
    ret = open(buf, O_RDWR | O_SYNC);
    if (ret < 0)
        GMEP_LOG("ERROR: couldn't open(\"%s\"): %s", buf, strerror(errno));

    return ret;
}

int gmep_us_Init(int *readFd, int *writeFd, u32 appId)
{
#ifndef CMBS_API
    t_st_GmepAppMsgCmd regMsgCmd;
    u32 pid;
    int ret = -1;
#endif
    char buf[GMPED_APP_FIFO_CLIENT_FMT_LEN];
    int errno;
    u32 u32_SerializedAppId;

    g_gmepUsFacilityCB = NULL;
    GMEP_LOG("gmep_us_Init  sending GMEPD_APP_REGISTER \n");


    cfr_ie_ser_u32((u8 *)&u32_SerializedAppId, appId);
    sprintf(buf, GMEPD_APP_FIFO_CLIENT_FMT, (getpid() + u32_SerializedAppId));
    appRdFrmGmepdFd = gmep_us_MsgFifoInit(buf);
    if (appRdFrmGmepdFd < 0)
    {
        GMEP_LOG(" ERR: fd_gmep_recv %d \n", appRdFrmGmepdFd);
        return -1;
    }

#ifndef CMBS_API
    // CMBS will be used to write from CMBS to Select part of SUOTA
    appWrToGmepdWFd = open(GMEPD_APP_FIFO, O_WRONLY | O_NONBLOCK);
    if (appWrToGmepdWFd  < 0)
    {
        if (errno == ENXIO)
            fprintf(stderr, "GMEP: msgswitchd not running\n");
        GMEP_LOG(" ERR: fd_gmep_send %d \n", appWrToGmepdWFd);
        return -1;
    }


    regMsgCmd.cmd  = GMEPD_APP_REGISTER;
    regMsgCmd.appSessionId = u32_SerializedAppId;
    pid = getpid();
    regMsgCmd.bufLen = sizeof(pid);
    memcpy(&regMsgCmd.buf[0], &pid, regMsgCmd.bufLen);

    do
    {
        ret = msg_fifo_write(appWrToGmepdWFd, (char *)&regMsgCmd, sizeof(regMsgCmd));
    } while (ret < 0 && errno == EAGAIN && usleep(10000));

    if (ret < 0)
        return -1;
#endif

    *readFd = appRdFrmGmepdFd;
    *writeFd = appWrToGmepdWFd;

    return 0;
}

/*---------[End of file]-------------------------------------------------------------*/
