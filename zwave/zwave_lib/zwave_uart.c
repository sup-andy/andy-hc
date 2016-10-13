
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>

#include "zwave_api.h"
#include "zwave_uart.h"

int g_zw_uart_fd = 0;


void print_frame(int type, BYTE *frame, BYTE len)
{
    char buff[1024] = {0};
    int n = 0, i = 0;
    
    if(type == 0)
    {
        n = snprintf(buff, sizeof(buff), "TX[%u]: ", len);
    }
    else
    {
        n = snprintf(buff, sizeof(buff), "RX[%u]: ", len);
    }

    for(i = 0; i < len; i++)
    {
        n += snprintf(buff+n, sizeof(buff), "%02x ", frame[i]);
    }
    fprintf(stderr, "%s \n", buff);
}



int read_ex(int sfd, void *pData, unsigned int len)
{
    int n = 0, i = 0;
    int cnt = 0;

    while (1)
    {
        n = read(sfd, pData + i, len - i);
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


int write_ex(int sfd, void *pData, unsigned int len)
{
    int n = 0, i = 0;
    int cnt = 0;

    while (1)
    {
        n = write(sfd, pData + i, len - i);
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


int ZW_UART_open(char *dev)
{
    struct termio stbuf;
    int clocal = 0, parity = 0, bits = CS8, stopbits = 0;
    int speed = B0;
    int fd = 0;
    
    fd = open ( dev, O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY );
    if ( fd < 0 )
    {       
        fprintf (stderr, "Can't open device %s.\n", dev);
        return -1;
    }
    
    if ( ioctl ( fd, TCGETA, &stbuf ) < 0 )
    {
        close ( fd ); 
        fprintf (stderr, "Can't ioctl get device, please try again.\n");
        return -1;
    }
    
    speed     = B115200;
    bits = stbuf.c_cflag & CSIZE;
    clocal = stbuf.c_cflag & CLOCAL;
    stopbits = stbuf.c_cflag & CSTOPB;
    parity = stbuf.c_cflag & ( PARENB | PARODD );
    stbuf.c_iflag &= ~ ( IGNCR | ICRNL | IUCLC | INPCK | IXON | IXANY | IGNPAR );
    stbuf.c_oflag &= ~ ( OPOST | OLCUC | OCRNL | ONLCR | ONLRET );
    stbuf.c_lflag &= ~ ( ICANON | XCASE | ECHO | ECHOE | ECHONL );
    stbuf.c_lflag &= ~ ( ECHO | ECHOE );
    stbuf.c_cc[VMIN] = 1;
    stbuf.c_cc[VTIME] = 0; 
    stbuf.c_cc[VEOF] = 1;
    stbuf.c_cflag &= ~ ( CBAUD | CSIZE | CSTOPB | CLOCAL | PARENB );
    stbuf.c_cflag |= ( speed | bits | CREAD | clocal | parity | stopbits );
    
    if ( ioctl ( fd, TCSETA, &stbuf ) < 0 )
    {
        close ( fd );
        fprintf (stderr,  "Can't ioctl set device.\n");
        return -1;
    }
    
    usleep ( 200000 );
    g_zw_uart_fd = fd;
    return fd;
}

int ZW_UART_tx(BYTE *data, int len)
{
    
    fd_set wfds;
    int ret;
    int n = 0;

    print_frame(0, data, len);

    FD_ZERO(&wfds);
    FD_SET(g_zw_uart_fd, &wfds);
    ret = select(g_zw_uart_fd + 1, NULL, &wfds, NULL, NULL);
    if(ret > 0)
    {
        if(FD_ISSET(g_zw_uart_fd, &wfds))
        {
            n = write_ex(g_zw_uart_fd, data, len);
            return n;
        }
    }

    return ret;
}


void *ZW_UART_rx(void *args)
{
    int fd1 = *(int *)args;

    fd_set rfds;
    int ret = -1;
    BYTE buf[256];
    int n = -1;

    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(fd1, &rfds);
        ret = select(fd1 + 1, &rfds, NULL, NULL, NULL);
        if(ret < 0)
        {
            continue;
        }
        if(FD_ISSET(fd1, &rfds))
        {
            memset(buf, 0, sizeof(buf));
            n = read_ex(fd1, buf, 1);
            if(n > 0)
            {
                if(buf[0] == 0x06
                    ||buf[0] == 0x15
                    ||buf[0] == 0x18)
                {
                    print_frame(1, buf, 1);
                    ZWapi_Dispatch(buf, 1);
                }
                else if(buf[0] == 0x01)
                {
                    n = read_ex(fd1, &(buf[1]), 1);
                    if(n > 0)
                    {
                        n = read_ex(fd1, &(buf[2]), buf[1]);
                    }
                    print_frame(1, buf, buf[1]+2);
                    ZWapi_Dispatch(buf, buf[1]+2);
                }
                else
                {
                    fprintf(stderr, "receive error data. \n");
                    continue;
                }
                
            }
        }
    }

}


