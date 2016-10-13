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
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "hcapi.h"
#include "hc_msg.h"
#include "hc_common.h"



#define DB_DBG_FLAG     "/tmp/db.dbg"

void dump_buffer(char *str, unsigned char *buffer, int size)
{
    int loop = 0;

    if (0 != access(DB_DBG_FLAG, F_OK))
    {
        return;
    }

    printf("\n[%s]dump_buffer addr 0x%x size %d:\n",
           str, (unsigned int)buffer, size);
    for (loop = 0; loop < size; loop++)
    {
        if (loop == sizeof(HC_MSG_S))
        {
            printf("\nMSG-----------------------------------------\n");
        }
        if (loop > sizeof(HC_MSG_S) && ((loop - sizeof(HC_MSG_S)) % sizeof(HC_DEVICE_INFO_S)) == 0)
        {
            printf("\nDEV-----------------------------------------\n");
        }
        if (loop % 16 == 0)
        {
            printf("\n");
        }
        printf("%02x ", *(buffer + loop));
    }
    printf("\n");
}

int recv_ex(int sfd, void *pData, unsigned int len)
{
    int n = 0, i = 0;
    int cnt = 0;

    while (1)
    {
        n = recv(sfd, pData + i, len - i, MSG_DONTWAIT);
        if (n > 0)
        {
            i += n;
            if (i >= len)
            {
                return i;
            }
        }
        else if (n == 0)
        {
            return i;
        }
        else
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                if (++cnt > 9999)
                    break;
                continue;
            }
        }
    }

    return -1;
}

int send_ex(int sfd, void *pData, unsigned int len)
{
    int n = 0, i = 0;
    int cnt = 0;

    while (1)
    {
        n = send(sfd, pData + i, len - i, MSG_DONTWAIT);
        if (n > 0)
        {
            i += n;
            if (i >= len)
            {
                return i;
            }
        }
        else if (n <= 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                if (++cnt > 9999)
                    break;
                continue;
            }
            return -1;
        }
    }

    return -1;
}



int hc_client_msg_init(CLIENT_TYPE_E client_type, SOCKET_TYPE_E socket_type)
{
    int rc = 0;
    int sfd = -1;
    struct sockaddr_un serverAddr;

    if ((sfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        return -1;
    }

    /*
     * Set close-on-exec, even though all apps should close their
     * fd's before fork and exec.
     */
    if ((rc = fcntl(sfd, F_SETFD, FD_CLOEXEC)) != 0)
    {
        DEBUG_ERROR("set close-on-exec failed, rc=%d errno=%d.\n", rc, errno);
        close(sfd);
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    strncpy(serverAddr.sun_path, HC_MSG_SERVER_ADDRESS, sizeof(serverAddr.sun_path));

    rc = connect(sfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (rc != 0)
    {
        DEBUG_ERROR("connect to server failed, rc=%d errno=%d.\n", rc, errno);
        close(sfd);
        return -1;
    }

    /* Send the first client info packet to dispatcher, it's named register. */
    HC_MSG_S hc_msg;
    memset(&hc_msg, 0, sizeof(HC_MSG_S));
    hc_msg.head.type = HC_MSG_TYPE_CLIENT_INFO;
    hc_msg.body.client_info.client_type = client_type;
    hc_msg.body.client_info.socket_type = socket_type;
    hc_msg.body.client_info.pid = getpid();

    int n = send(sfd, &hc_msg, sizeof(hc_msg), 0);
    if (n != sizeof(hc_msg))
    {
        close(sfd);
        return -1;
    }

    DEBUG_INFO("%d[%u] initializes [%d] \n", client_type, getpid(), socket_type);

    return sfd;
}

int hc_client_wait_for_msg(int fd, struct timeval *timeout, HC_MSG_S **pMsg)
{
    int cnt = 0;
    fd_set rfds, rfdsMaster;
    int n = 0;
    int maxfd = 0;
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


    FD_ZERO(&rfdsMaster);
    FD_SET(fd, &rfdsMaster);

    maxfd = fd;

    while (1)
    {
        rfds = rfdsMaster;

        n = select(maxfd + 1, &rfds, NULL, NULL, timeout);
        if (n == 0)
        {
            return 0;
        }
        else if (n < 0)
        {
            if (errno == EINTR)
            {
                if (++cnt > 9999)
                    break;
                continue;
            }

            if (errno == EAGAIN)
            {
                continue;
            }
            return -1;
        }
        else
        {
            if (FD_ISSET(fd, &rfds))
            {
                hcmsg = calloc(1, sizeof(HC_MSG_S));
                if (hcmsg == NULL)
                {
                    DEBUG_ERROR("Allocate memory failed.\n");
                    return -1;
                }

                memset(hcmsg, 0, sizeof(HC_MSG_S));
                int ret = recv_ex(fd, (void *)hcmsg, sizeof(HC_MSG_S));
                if (ret <= 0)
                {
                    return -1;
                }
                if (ret != sizeof(HC_MSG_S))
                {
                    return -1;
                }

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

                    ret = recv_ex(fd, (void *)hcmsg->data, hcmsg->head.data_len);
                    if (ret <= 0)
                    {
                        free(hcmsg);
                        return -1;
                    }
                    if (ret != hcmsg->head.data_len)
                    {
                        free(hcmsg);
                        return -1;
                    }
                }

                *pMsg = hcmsg;

                return n;

            }
        }
    }

    return -1;
}

void hc_msg_free(HC_MSG_S *pMsg)
{
    if (pMsg)
    {
        free(pMsg);
        pMsg = NULL;
    }
}



void hc_client_unint(int sfd)
{
    if (sfd > 0)
    {
        close(sfd);
    }
}


int hc_client_send_msg_to_dispacther(int sfd, HC_MSG_S *pMsg)
{
    int ret = -1;

    if (pMsg == NULL)
    {
        return -1;
    }

    if (sfd > 0)
    {
        ret = send_ex(sfd, pMsg, sizeof(HC_MSG_S) + pMsg->head.data_len);
    }

    return ret;
}


