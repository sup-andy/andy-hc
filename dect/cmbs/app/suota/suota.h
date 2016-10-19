

#ifndef SUOTA_PROCESS
#define SUOTA_PROCESS

#include "gmep-us.h"

typedef struct
{
    int    threadWrFd;
    int    threadRdFd;
    int     sessionId;
    int    socket;
    pthread_t  receiveThread;
    char    local[32];
    t_st_GmepSdu httpRequest;
    u8    sesStatus;
} t_Suota_GmepSessionTbl;

#define SUOTA_MAX_HEADER_SIZE       140
#define SUOTA_MAX_PAYLOAD_SIZE      (MAX_GMEP_SDU_SIZE - SUOTA_MAX_HEADER_SIZE)
#define SUOTA_IMAGE_HEADER_SIZE     60

typedef struct
{
    u8 responseBuff[MAX_GMEP_SDU_SIZE];
    u32 content_length;
    u32 responseBuffLen;
} t_st_httpResponse;


typedef enum
{
    CMBS_URL_SENDING_STATE_IDLE,
    CMBS_URL_SENDING_STATE_IN_PROGRESS,

    CMBS_URL_SENDING_STATE_MAX
} e_URLSendState;

unsigned long suota_createThread(char *pIpAddr, u16 portNumber, int pushMode, u16 u16_Handset);
unsigned long suota_quitThread();
void suota_send_gmep();
void Thread_Fifo_Send(t_st_GmepAppMsgCmd   *httpMsg);
void Suota_fifo_send(int fd, t_st_GmepAppMsgCmd *pMsg);
void cplane_Sendnegack(t_st_GmepSuotaNegAck *Nack);
void suota_PrintHex(u8 *buf, u32 len);
void suota_app();
void thread_send_suota(t_Suota_GmepSessionTbl *pSession, t_st_GmepSdu *HttpResp);
void getNextChunk(t_st_GmepAppMsgCmd *pMsgRx, t_Suota_GmepSessionTbl *pSession, t_st_GmepSdu *pHttpresp);
#endif


