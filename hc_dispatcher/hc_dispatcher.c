/*******************************************************************************
 *   Copyright 2015 WondaLink CO., LTD.
 *   All Rights Reserved. This material can not be duplicated for any
 *   profit-driven enterprise. No portions of this material can be reproduced
 *   in any form without the prior written permission of WondaLink CO., LTD.
 *   Forwarding, transmitting or communicating its contents of this document is
 *   also prohibited.
 *
 *   All titles, proprietaries, trade secrets and copyrights in and related to
 *   information contained in this document are owned by WondaLink CO., LTD.
 *
 *   WondaLink CO., LTD.
 *   23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *   HSIN-CHU, TAIWAN R.O.C.
 *
 ******************************************************************************/
/******************************************************************************
*   Department:
*   Project :
*   Block   :
*   Creator : Mark Yan
*   File   : hc_dispatcher.c
*   Abstract:
*   Date   : 1/4/2015
*   $Id:$
*
*   Modification History:
*   By     Date    Ver.  Modification Description
*   -----------  ---------- -----  -----------------------------
*   Rose  06/03/2013 1.0  Initial Dev.
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include "hcapi.h"
#include "hcapi_db.h"
#include "hc_msg.h"
#include "hc_util.h"
#include "hc_common.h"

static int keep_looping = 1;

static SERV_INFO_S g_serv_info;

static void signal_handle(int signal)
{
    keep_looping = 0;
}

static void reset_client(int idx)
{
    FD_CLR(g_serv_info.clients[idx].sockfd, &(g_serv_info.rfds));
    close(g_serv_info.clients[idx].sockfd);
    g_serv_info.clients[idx].sockfd = -1;
    g_serv_info.client_num--;
}

static int client_check(CLIENT_TYPE_E client_type)
{
    int i = 0;

    for (i = 0; i < MAX_CLIENTS_NUM; i++)
    {
        if (g_serv_info.clients[i].client_type == client_type &&
                g_serv_info.clients[i].sockfd > 0)
        {
            return 0;
        }
    }
    return -1;
}

int hc_dispacher_msg_init(void)
{
    int i = 0;
    int rc = 0;
    struct sockaddr_un serverAddr;

    /* initialize global variables */
    memset(&g_serv_info, 0, sizeof(g_serv_info));
    for (i = 0; i < MAX_CLIENTS_NUM; i++)
    {
        g_serv_info.clients[i].sockfd = -1;
    }
    g_serv_info.serv_sockfd = -1;


    unlink(HC_MSG_SERVER_ADDRESS);

    int servfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (servfd == -1)
    {
        DEBUG_ERROR("socket fail!\r\n");
        return -1;
    }


    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    strncpy(serverAddr.sun_path, HC_MSG_SERVER_ADDRESS, sizeof(serverAddr.sun_path));

    rc = bind(servfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (rc != 0)
    {
        DEBUG_ERROR("bind fail!\r\n");
        close(servfd);
        return -1;
    }

    rc = listen(servfd, HC_MSG_LISTEN_BACKLOG);
    if (rc != 0)
    {
        DEBUG_ERROR("listen fail!\r\n");
        close(servfd);
        return -1;
    }

    FD_ZERO(&g_serv_info.rfds);
    FD_SET(servfd, &g_serv_info.rfds);
    g_serv_info.serv_sockfd = servfd;
    g_serv_info.maxfd = servfd;

    return servfd;
}

void hc_dispacher_msg_uninit(void)
{
    int i = 0;

    for (i = 0; i < MAX_CLIENTS_NUM; i++)
    {
        if (g_serv_info.clients[i].sockfd > 0)
        {
            close(g_serv_info.clients[i].sockfd);
            g_serv_info.clients[i].sockfd = -1;
        }
    }

    if (g_serv_info.serv_sockfd > 0)
    {
        close(g_serv_info.serv_sockfd);
        g_serv_info.serv_sockfd = -1;
    }
}

int hc_dispacher_wait_for_msg(struct timeval *timeout, HC_MSG_S **pMsg)
{
    fd_set rfds;
    int n = 0, i = 0;
    int cnt = 0;
    HC_MSG_S *hcmsg = NULL;
    HC_MSG_S *tmp = NULL;

    if (pMsg == NULL)
    {
        DEBUG_ERROR("pMsg is NULL!");
        return -1;
    }
    else
    {
        *pMsg = NULL;
    }

    while (1)
    {
        FD_ZERO(&rfds);
        rfds = g_serv_info.rfds;
        n = select(g_serv_info.maxfd + 1, &rfds, NULL, NULL, timeout);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                if (++cnt < 9999)
                    continue;
            }
            if (errno == EAGAIN)
            {
                continue;
            }
            DEBUG_ERROR("select error, errno is %d.\n", errno);
            return -1;
        }
        else if (n == 0)
        {
            //printf("select timeout. \r\n");
            return 0;
        }

        /*
         * Server socket triggered, it should be new client connecting.
         */
        if (FD_ISSET(g_serv_info.serv_sockfd, &rfds))
        {
            int client_fd = 0;
            struct sockaddr_un client_addr;
            socklen_t len = sizeof(client_addr);
            if ((client_fd = accept(g_serv_info.serv_sockfd, (struct sockaddr *)&client_addr, &len)) == -1)
            {
                DEBUG_ERROR("hc_dispacher accept error. \r\n");
                return -1;
            }

            // check the client valid

            if (g_serv_info.client_num >= MAX_CLIENTS_NUM)
            {
                // reach the max clients
                DEBUG_ERROR("hc_dispacher has connected max clients [%d]. \r\n", g_serv_info.client_num);
                return -1;
            }
            g_serv_info.client_num++;

            for (i = 0; i < MAX_CLIENTS_NUM; i++)
            {
                if (g_serv_info.clients[i].sockfd == -1)
                    break;
            }
            if (i == MAX_CLIENTS_NUM)
            {
                // no space for new client
                DEBUG_ERROR("there is no space for new client. \r\n");
                return -1;
            }
            g_serv_info.clients[i].sockfd = client_fd;
            if (g_serv_info.maxfd < client_fd)
            {
                g_serv_info.maxfd = client_fd;
            }
            FD_SET(client_fd, &(g_serv_info.rfds));

            //printf("new client connected.\r\n");

        }

        for (i = 0; i < MAX_CLIENTS_NUM; i++)
        {
            if (g_serv_info.clients[i].sockfd >  0 && FD_ISSET(g_serv_info.clients[i].sockfd, &rfds))
            {
                HC_MSG_S recv_buf;
                memset(&recv_buf, 0, sizeof(recv_buf));
                int ret = recv_ex(g_serv_info.clients[i].sockfd, (void *)&recv_buf, sizeof(recv_buf));
                if (ret <= 0)
                {
                    reset_client(i);
                    continue;
                }
                else if (ret != sizeof(HC_MSG_S))
                {
                    reset_client(i);
                    continue;
                }

                /* It's the first client info packet. */
                if (recv_buf.head.type == HC_MSG_TYPE_CLIENT_INFO)
                {
                    //printf("client type msg received. \r\n");

                    g_serv_info.clients[i].client_type = recv_buf.body.client_info.client_type;
                    g_serv_info.clients[i].socket_type = recv_buf.body.client_info.socket_type;
                    g_serv_info.clients[i].pid = recv_buf.body.client_info.pid;

                    DEBUG_INFO("New client. client type[%d] socket type[%d] socket id[%d] pid[%u]\n",
                        g_serv_info.clients[i].client_type,
                        g_serv_info.clients[i].socket_type,
                        g_serv_info.clients[i].sockfd,
                        g_serv_info.clients[i].pid);
                }
                else
                {
                    //printf("client msg received. \r\n");

                    hcmsg = calloc(1, sizeof(HC_MSG_S));
                    if (hcmsg == NULL)
                    {
                        DEBUG_ERROR("Allocate memory failed.\n");
                        return -1;
                    }

                    memcpy(hcmsg, &recv_buf, sizeof(HC_MSG_S));
                    hcmsg->head.src = g_serv_info.clients[i].client_type;
                    if (SOCKET_COMMAND == g_serv_info.clients[i].socket_type)
                    {
                        hcmsg->head.appid = g_serv_info.clients[i].sockfd;
                    }

                    /* The data_len > 0, use the member data to store more information/data. */
                    if (hcmsg->head.data_len > 0)
                    {
                        tmp = (HC_MSG_S *)realloc(hcmsg, sizeof(HC_MSG_S) + hcmsg->head.data_len);
                        if (tmp == NULL)
                        {
                            DEBUG_ERROR("Re-Allocate memory failed.\n");
                            free(hcmsg);
                            return -1;
                        }
                        hcmsg = tmp;

                        ret = recv_ex(g_serv_info.clients[i].sockfd, (void *)hcmsg->data, hcmsg->head.data_len);
                        if (ret <= 0)
                        {
                            reset_client(i);
                            free(hcmsg);
                            return -1;
                        }
                        if (ret != hcmsg->head.data_len)
                        {
                            reset_client(i);
                            free(hcmsg);
                            return -1;
                        }

                    }

                    *pMsg = hcmsg;
                    return n;

                }

            }
        }
    }

    return -1;
}

int hc_dispatcher_send_msg_to_client(HC_MSG_S *pMsg)
{
    int i = 0;
    int ret = -1;

    if (pMsg == NULL)
    {
        return -1;
    }

    /*
     * HCAPI -> Daemon
     * if couldn't find dst in clinets array, indicates the daemon
     * did not connect to dispatcher, maybe CPE is not support.
     * so just return HC_EVENT_RESP_FUNCTION_NOT_SUPPORT.
     */
    if (APPLICATION == pMsg->head.src &&
            APPLICATION != pMsg->head.dst &&
            DAEMON_ALL != pMsg->head.dst)
    {
        if (0 != client_check(pMsg->head.dst))
        {
            if (pMsg->head.appid > 0)
            {
                pMsg->head.src = pMsg->head.dst;
                pMsg->head.dst = APPLICATION;
                pMsg->head.type = HC_EVENT_RESP_FUNCTION_NOT_SUPPORT;
                pMsg->body.device_info.event_type = HC_EVENT_RESP_FUNCTION_NOT_SUPPORT;

                ret = send_ex(pMsg->head.appid, pMsg, sizeof(HC_MSG_S) + pMsg->head.data_len);

                DEBUG_INFO("Reply msg %s(%d) from %s(%d) to %s(%d) appid(%d). ret[%d]\n",
                           hc_map_msg_txt(pMsg->head.type), pMsg->head.type,
                           hc_map_client_txt(pMsg->head.src), pMsg->head.src,
                           hc_map_client_txt(pMsg->head.dst), pMsg->head.dst,
                           pMsg->head.appid, ret);

                return ret;
            }
        }
    }

    for (i = 0; i < MAX_CLIENTS_NUM; i++)
    {

        /* Broadcast this event to all daemon except DB daemon. */
        if (pMsg->head.dst == DAEMON_ALL)
        {
            if (g_serv_info.clients[i].client_type != APPLICATION)
            {
                if (g_serv_info.clients[i].sockfd > 0)
                {
                    ret = send_ex(g_serv_info.clients[i].sockfd, pMsg, sizeof(HC_MSG_S) + pMsg->head.data_len);

                    DEBUG_INFO("Broadcast signal %s(%d) from %s(%d) to %s(%d:%d) appid(%d). ret[%d]\n",
                               hc_map_msg_txt(pMsg->head.type), pMsg->head.type,
                               hc_map_client_txt(pMsg->head.src), pMsg->head.src,
                               hc_map_client_txt(g_serv_info.clients[i].client_type),
                               g_serv_info.clients[i].client_type,
                               g_serv_info.clients[i].sockfd,
                               pMsg->head.appid,
                               ret);
                
                }
            }

        }
        else if (g_serv_info.clients[i].client_type == pMsg->head.dst)
        {
            /* The event is sent to APPLICATION, there are 2 cases as below: */
            if (pMsg->head.dst == APPLICATION)
            {
                /* appid > 0 indicates this event is a response, reply to the HCAPI. */
                if (pMsg->head.appid > 0)
                {
                    if (g_serv_info.clients[i].sockfd == pMsg->head.appid)
                    {
                        ret = send_ex(g_serv_info.clients[i].sockfd, pMsg, sizeof(HC_MSG_S) + pMsg->head.data_len);

                        DEBUG_INFO("Reply msg %s(%d) from %s(%d) to %s(%d:%d) appid(%d). ret[%d]\n",
                                   hc_map_msg_txt(pMsg->head.type), pMsg->head.type,
                                   hc_map_client_txt(pMsg->head.src), pMsg->head.src,
                                   hc_map_client_txt(g_serv_info.clients[i].client_type),
                                   g_serv_info.clients[i].client_type,
                                   g_serv_info.clients[i].sockfd,
                                   pMsg->head.appid,
                                   ret);

                    }
                }
                else
                {
                    /* The event should be sent on SOCKET_EVENT socket. */
                    if (g_serv_info.clients[i].sockfd > 0 &&
                            SOCKET_EVENT == g_serv_info.clients[i].socket_type)
                    {
                        ret = send_ex(g_serv_info.clients[i].sockfd, pMsg, sizeof(HC_MSG_S) + pMsg->head.data_len);

                        DEBUG_INFO("Broadcast msg %s(%d) from %s(%d) to %s(%d:%d) appid(%d). ret[%d]\n",
                                   hc_map_msg_txt(pMsg->head.type), pMsg->head.type,
                                   hc_map_client_txt(pMsg->head.src), pMsg->head.src,
                                   hc_map_client_txt(g_serv_info.clients[i].client_type),
                                   g_serv_info.clients[i].client_type,
                                   g_serv_info.clients[i].sockfd,
                                   pMsg->head.appid,
                                   ret);
                    }
                }
            }
            /* DST is not APPLICATION && SRC is not HCAPI_DB_LIB */
            else
            {
                if (g_serv_info.clients[i].sockfd > 0 &&
                        SOCKET_DEFAULT == g_serv_info.clients[i].socket_type)
                {
                    ret = send_ex(g_serv_info.clients[i].sockfd, pMsg, sizeof(HC_MSG_S) + pMsg->head.data_len);

                    DEBUG_INFO("Forward msg %s(%d) from %s(%d) to %s(%d:%d) appid(%d). ret[%d]\n",
                               hc_map_msg_txt(pMsg->head.type), pMsg->head.type,
                               hc_map_client_txt(pMsg->head.src), pMsg->head.src,
                               hc_map_client_txt(g_serv_info.clients[i].client_type),
                               g_serv_info.clients[i].client_type,
                               g_serv_info.clients[i].sockfd,
                               pMsg->head.appid,
                               ret);
                }
            }
        }
    }

    return ret;
}



int main(int argc, char *argv[])
{
    int ret = 0;
    int i = 0;
    struct timeval tv;
    HC_MSG_S *hcmsg = NULL;
    FILE *fp = NULL;

#ifdef CTRL_LOG_DEBUG
    ctrl_log_init(HC_CTRL_NAME);
#endif

    ret = hc_dispacher_msg_init();
    if (ret == -1)
    {
        DEBUG_ERROR("Call hc_dispatcher_msg_init failed, errno is '%d'.\n", errno);
        return -1;
    }

    signal(SIGINT, signal_handle);
    signal(SIGTERM, signal_handle);

    ret = hcapi_db_related_init();
    if (ret == -1)
    {
        DEBUG_ERROR("Call hcapi_db_related_init() failed, errno is '%d'.\n", errno);
        return -1;
    }

    fp = fopen(DISPATCHER_STARTED_FILE, "w");
    if (fp == NULL)
    {
        DEBUG_ERROR("Create file %s failed, errno is '%d'.\n", DISPATCHER_STARTED_FILE, errno);
        hc_dispacher_msg_uninit();
        return -1;
    }
    fclose(fp);

    while (keep_looping)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        hcmsg = NULL;
        ret = hc_dispacher_wait_for_msg(&tv, &hcmsg);
        if (ret == 0)
        {
            //DEBUG_INFO("hc_dispatcher timeout.\n");
            continue;
        }
        else if (ret == -1)
        {
            DEBUG_ERROR("Call hc_dispacher_wait_for_msg failed, errno is '%d'.\n", errno);
            continue;
        }

        // forward msg to destination.
        ret = hc_dispatcher_send_msg_to_client(hcmsg);
        if (ret <= 0)
        {
            DEBUG_INFO("Error: Send msg %s(%d) from %s(%d) to %s(%d), errno is [%d:%s].\n",
                       hc_map_msg_txt(hcmsg->head.type), hcmsg->head.type,
                       hc_map_client_txt(hcmsg->head.src), hcmsg->head.src,
                       hc_map_client_txt(hcmsg->head.dst), hcmsg->head.dst, errno, strerror(errno));
        }

        hc_msg_free(hcmsg);
    }

    hc_dispacher_msg_uninit();

    return 0;

}

