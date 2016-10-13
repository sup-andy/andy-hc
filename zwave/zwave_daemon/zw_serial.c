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
*   File   : zw_serial.c
*   Abstract:
*   Date   : 12/18/2014
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

#include "hc_common.h"
#include "zw_serial.h"
#include "zwave_ctrl.h"
#include "zwave_api.h"


#define ZWAVE_DEVICE_NAME   "/dev/zwave"

#define SOF 0x01  /* Start Of Frame */
#define ACK 0x06  /* Acknowledge successfull frame reception */
#define NAK 0x15  /* Not Acknowledge successfull frame reception - please retransmit... */
#define CAN 0x18  /* Frame received (from host) was dropped - waiting for ACK */

#define SERIAL_BUFFER_SIZE    512

extern int keep_looping;

/* FD of serial (ttyS*) */
static int serial_fd = -1;

/* RX buffer */
static unsigned char rx_buffer[SERIAL_BUFFER_SIZE];
static int rx_count = 0;
static pthread_mutex_t read_write_mutex;


/******************** Local Functions ********************/

/* Checksum does not include SOF. */
unsigned char calculateChecksum(unsigned char *pData, int nLength)
{
    unsigned char byChecksum;
    byChecksum = 0xff;    // checksum start from 0xFF

    for (; nLength; nLength--)
    {
        byChecksum ^= *pData++;
    }

    return byChecksum;
}

static void print_cmd(int dir, unsigned char *cmd, int length)
{
    int i = 0;
    char buffer[1024] = {0};

    sprintf(buffer, "%s %02d cmd: [", (dir == 0) ? "Recv" : "Send", length);
    for (i = 0; i < length; i++)
        sprintf(buffer, "%s%02X ", buffer, cmd[i]);
    sprintf(buffer, "%s]", buffer);

    DEBUG_INFO("%s\n", buffer);
}

static int fresh_rx_buffer(int start_idx, int end_idx)
{
    int i = 0;
    int idx = start_idx;

    // if start_idx > end_idx indicates rx_buffer is empty.
    while (idx <= end_idx)
    {
        rx_buffer[i++] = rx_buffer[idx++];
    }
    rx_count = i;

    return 0;
}

static int process_rx_buffer(void)
{
    int ret = ZW_STATUS_OK;
    int i = 0;
    unsigned char *p = NULL;
    int cmd_start_idx = 0;
    int cmd_end_idx = 0;
    int cmd_length = 0;

    if (rx_count == 0)
    {
        return -1;
    }

    while (i < rx_count)
    {
        p = &rx_buffer[i];

        if (ACK == *p || NAK == *p)
        {
            // dispatch cmd to zwave protocol.
            //#ifdef SERIAL_UNIT_TEST
            //printf("ACK or NAK %02X\n", *p);
            print_cmd(0, p, 1);
            //#else
            ret = ZWapi_Dispatch(p, 1);
            if (ret != ZW_STATUS_OK)
            {
                DEBUG_ERROR("Call ZWapi_Dispatch failed.\n");
            }
            //#endif
            fresh_rx_buffer(1, rx_count - 1);
            i = 0;
            continue;
        }

        if (SOF == *p)
        {
            cmd_start_idx = i;
            if (++i < rx_count)
            {
                // SOF + cmd length + Checksum
                cmd_length = rx_buffer[i] + 2;
                cmd_end_idx = cmd_start_idx + cmd_length - 1;

                if (cmd_end_idx < rx_count)
                {
                    // dispatch cmd to zwave protocol.
                    //#ifdef SERIAL_UNIT_TEST
                    //printf("SOF %02X\n", *p);
                    print_cmd(0, p, cmd_length);
                    //#else
                    ret = ZWapi_Dispatch(p, cmd_length);
                    if (ret != ZW_STATUS_OK)
                    {
                        DEBUG_ERROR("Call ZWapi_Dispatch failed.\n");
                    }
                    //#endif
                    fresh_rx_buffer(cmd_end_idx + 1, rx_count - 1);
                    i = 0;
                    continue;
                }
            }

            // rx_buffer does not include a complete cmd, return.
            return 0;
        }

        i++;
    }

    return 0;
}


int serial_init(void)
{
    serial_fd = zwave_open(ZWAVE_DEVICE_NAME);
    if (serial_fd == -1)
    {
        return -1;
    }

    memset(rx_buffer, 0, sizeof(rx_buffer));
    rx_count = 0;

    pthread_mutex_init(&read_write_mutex, NULL);

    return 0;
}

void serial_uninit(void)
{
    if (serial_fd != -1)
    {
        zwave_close(serial_fd);
    }

    pthread_mutex_destroy(&read_write_mutex);
}

int serial_send(unsigned char *buffer, int length)
{
    int ret = 0;

    print_cmd(1, buffer, length);

    pthread_mutex_lock(&read_write_mutex);

    ret = zwave_write(serial_fd, buffer, length);

    pthread_mutex_unlock(&read_write_mutex);

    if (ret != length)
    {
        DEBUG_ERROR("ZWAVE write cmd failed, write %d bytes, actually write %d bytes.\n", length, ret);
        return -1;
    }

    return ret;
}

void * serial_handle_thread(void *arg)
{
    int ret = 0;
    int i, n, maxFd = 0;
    int errorCnt = 0;
    fd_set readFdsMaster, readFds;
    unsigned char buffer[215] = {0};

    struct timeval tv;

    /* set up all the fd stuff for select */
    FD_ZERO(&readFdsMaster);

    FD_SET(serial_fd, &readFdsMaster);

    maxFd = serial_fd;

    while (keep_looping)
    {
        readFds = readFdsMaster;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        n = select(maxFd + 1, &readFds, NULL, NULL, &tv);
        if (n < 0)
        {
            /* interrupted by signal or something, continue */
            continue;
        }
        else if (0 == n)
        {
            continue;
        }

        if (FD_ISSET(serial_fd, &readFds))
        {
            memset(buffer, 0, sizeof(buffer));

            pthread_mutex_lock(&read_write_mutex);
            ret = zwave_read(serial_fd, buffer, sizeof(buffer));
            pthread_mutex_unlock(&read_write_mutex);

            if (ret <= 0)
            {
                if (++errorCnt < 5)
                {
                    DEBUG_ERROR("read serial port %s error.\n", ZWAVE_DEVICE_NAME);
                    continue;
                }
                else
                {
                    DEBUG_ERROR("read serial port error too more times, exit.");
                    return NULL;
                }

            }

            errorCnt = 0;

            for (i = 0; i < ret; i++)
            {
                //printf("%x %d %d\n", 0xFF & buffer[i], rx_idx_in, rx_idx_out);

                if (rx_count < SERIAL_BUFFER_SIZE)
                {
                    rx_buffer[rx_count++] = buffer[i];
                }
                else
                {
                    DEBUG_ERROR("rx buffer full.\n");
                    rx_count = 0;
                }


            }

            process_rx_buffer();
        }

    }

    return NULL;
}

