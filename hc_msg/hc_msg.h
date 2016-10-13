/**********************************************************************
 *    Copyright 2015 WondaLink CO., LTD.
 *    All Rights Reserved. This material can not be duplicated for any
 *    profit-driven enterprise. No portions of this material can be reproduced
 *    in any form without the prior written permission of WondaLink CO., LTD.
 *    Forwarding, transmitting or communicating its contents of this document is
 *    also prohibited.
 *
 *    All titles, proprietaries, trade secrets and copyrights in and related to
 *    information contained in this document are owned by WondaLink CO., LTD.
 *
 *    WondaLink CO., LTD.
 *    23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *    HSIN-CHU, TAIWAN R.O.C.
 *
 ******************************************************************************/
/******************************************************************************
 *    Department:
 *    Project :
 *    Block   :
 *    Creator :
 *    File    :
 *    Abstract:
 *    Date    :
 *    $Id:$
 *
 *    Modification History:
 *    By           Date       Ver.   Modification Description
 *    -----------  ---------- -----  -----------------------------
 ******************************************************************************/

#ifndef HC_MSG_H
#define HC_MSG_H

#include <sys/time.h>
#include <ctrl_common_lib.h>
#include <hcapi.h>

#ifdef __cplusplus
extern "C"
{
#endif

//#pragma pack(1)

#define HC_MSG_TYPE_CLIENT_INFO  HC_EVENT_MAX

//#define HC_MSG_LISTEN_PORT  55555
#define HC_MSG_SERVER_ADDRESS   "/var/hc_msg_server_address"
#define HC_MSG_LISTEN_BACKLOG   3

#define MAX_CLIENTS_NUM  128

typedef enum
{
    APPLICATION = 1,
    DAEMON_ZWAVE,
    DAEMON_ZIGBEE,
    DAEMON_ULE,
    DAEMON_LPRF,
    DAEMON_CAMERA,
    DAEMON_VOIP,
    DAEMON_ALLJOYN,
    // Send msg to all daemon except DAEMON_DB.
    DAEMON_ALL
} CLIENT_TYPE_E;

typedef enum
{
    SOCKET_DEFAULT = 1,
    SOCKET_EVENT,
    SOCKET_COMMAND
} SOCKET_TYPE_E;

typedef struct client_info_s
{
    CLIENT_TYPE_E client_type;
    SOCKET_TYPE_E socket_type;
    int sockfd;
    unsigned int pid;
} CLIENT_INFO_S;

typedef struct hc_option_info_s
{
    union {
        unsigned int timeout;
        unsigned int id;
    } u;
} HC_OPTION_INFO_S;

typedef struct serv_info_s
{
    int client_num;
    CLIENT_INFO_S clients[MAX_CLIENTS_NUM];
    fd_set rfds;
    int serv_sockfd;
    int maxfd;
} SERV_INFO_S;

typedef struct hc_msg_hdr_s
{
    int type;
    unsigned int seqnum;
    /*
     * The appid just used by the command socket (SOCKET_COMMAND),
     * when dispatcher received msg on this socket, will set its fd to
     * header field appid, and then forward to daemon (dst indicated).
     * after daemon finish process, reply msg to dispatcher, dispatcher
     * use appid to get who is the sender.
     */
    int appid;
    CLIENT_TYPE_E src;
    CLIENT_TYPE_E dst;
    unsigned int dev_id;
    HC_DEVICE_TYPE_E dev_type;
    DB_ACT_TYPE_E db_act;
    int db_resp_none;
    int db_act_ret;
    int dev_count;
    int data_len;
} HC_MSG_HDR_S;

typedef union hc_msg_body_u
{
    CLIENT_INFO_S client_info;
    HC_DEVICE_INFO_S device_info;
    HC_OPTION_INFO_S option_info;
    HC_UPGRADE_INFO_S upgrade_info;
} HC_MSG_BODY_U;

typedef struct hc_msg_s
{
    HC_MSG_HDR_S head;
    HC_MSG_BODY_U body;
    /*
     * The most of cases no needs use below member data, just get
     * all devices from DB use it now.
     * When use this memeber, the data_len > 0, the msg buffer
     * have to reallocate to store data, and whole msg size is:
     * (sizeof(HC_MSG_S) + pMsg->head.data_len)
     */
    unsigned char data[0];
} HC_MSG_S;

extern void dump_buffer(char *str, unsigned char *buffer, int size);
extern int recv_ex(int sfd, void *pData, unsigned int len);
extern int send_ex(int sfd, void *pData, unsigned int len);
extern int hc_client_msg_init(CLIENT_TYPE_E client_type, SOCKET_TYPE_E socket_type);
extern int hc_client_wait_for_msg(int fd, struct timeval *timeout, HC_MSG_S **pMsg);
extern void hc_msg_free(HC_MSG_S *pMsg);
extern void hc_client_unint(int sfd);
extern int hc_client_send_msg_to_dispacther(int sfd, HC_MSG_S *pMsg);


#ifdef __cplusplus
}
#endif

#endif
