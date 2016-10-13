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
*   File   : zwave_ctrl.c
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
#include <errno.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "zwave_ctrl.h"
#include "hc_common.h"

static void dormir(unsigned long microsecs)
{
    struct timeval timeout;
    timeout.tv_sec = microsecs / 1000000L;
    timeout.tv_usec = microsecs - (timeout.tv_sec * 1000000L);
    select(1, 0, 0, 0, &timeout);
}

int zwave_open(char *devname)
{
    struct termio stbuf;  /* termios */
    int clocal = 0, parity = 0, bits = CS8, stopbits = 0;
    int speed = B0; /* Set to B110, B150, B300,..., B38400 */
    long senddelay = 0; /* 0/100th second character delay for sending */

    int fd = 0;

    fd = open(devname, O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY);
    if (fd < 0)
    {
        DEBUG_ERROR("Can't open device %s.\n", devname);
        return -1;
    }

    if (ioctl(fd, TCGETA, &stbuf) < 0)
    {
        close(fd);
        DEBUG_ERROR("Can't ioctl get device %s, please try again.\n", devname);
        return -1;
    }

    senddelay = 50001;
    speed     = B115200;
    clocal    = 0;
    parity    = 0;
    bits      = CS8;
    stopbits  = 0;

    bits = stbuf.c_cflag & CSIZE;
    clocal = stbuf.c_cflag & CLOCAL;
    stopbits = stbuf.c_cflag & CSTOPB;
    parity = stbuf.c_cflag & (PARENB | PARODD);
    stbuf.c_iflag &= ~(IGNCR | ICRNL | IUCLC | INPCK | IXON | IXANY | IGNPAR);
    stbuf.c_oflag &= ~(OPOST | OLCUC | OCRNL | ONLCR | ONLRET);
    stbuf.c_lflag &= ~(ICANON | XCASE | ECHO | ECHOE | ECHONL);
    stbuf.c_lflag &= ~(ECHO | ECHOE);
    stbuf.c_cc[VMIN] = 1;
    stbuf.c_cc[VTIME] = 0;
    stbuf.c_cc[VEOF] = 1;

    stbuf.c_cflag &= ~(CBAUD | CSIZE | CSTOPB | CLOCAL | PARENB);
    stbuf.c_cflag |= (speed | bits | CREAD | clocal | parity | stopbits);
    if (ioctl(fd, TCSETA, &stbuf) < 0)
    {
        close(fd);
        DEBUG_ERROR("Can't ioctl set device %s.\n", devname);
        return -1;
    }

    dormir(200000);    /* Wait a bit (DTR raise) */

    return fd;
}

void zwave_close(int fd)
{
    if (fd != -1)
    {
        close(fd);
    }
}

int zwave_write(int fd, unsigned char *buffer, int length)
{
    int res = 0;
    unsigned char *tmp = buffer;
    int total = length;
    int count = 0;

    while (1)
    {
        res = write(fd, tmp, total);
        if (res <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                if (count++ > 10)
                    return -1;
                
                continue;
            }

            DEBUG_ERROR("ZWAVE: WRITE serial failed, res is %d, errno [%d:%s].\n", res, errno, strerror(errno));
            return -1;
        }
        if (res == total)
        {
            return length;
        }
        if (res > total)
        {
            DEBUG_ERROR("ZWAVE: === BUG ===, res is %d, total is %d, errno [%d:%s].\n", res, total, errno, strerror(errno));
            return -1;
        }
        
        total -= res;
        tmp += res;
    }

    return -1;
}

int zwave_read(int fd, unsigned char *buffer, int length)
{
    int res = 0;
    int count = 0;

    while (1)
    {
        res = read(fd, buffer, length);
        if (res <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                if (count++ > 10)
                    return -1;
                
                continue;
            }

            DEBUG_ERROR("ZWAVE: READ serial failed, res is %d, errno [%d:%s].\n", res, errno, strerror(errno));
            return -1;
        }
        
        return res;
    }

    return -1;
}

