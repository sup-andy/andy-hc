/******************************************************************************
*
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
*   Creator : Andy Yang
*   File   : sp_ble_serial.c
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
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include "ctrl_common_lib.h"

#include <termios.h>    // POSIX terminal control definitions
#include <sys/ioctl.h>

#include "sp_ble_serial.h"
#include "sp_ble_uart.h"
#include "sp_ble_queue.h"



extern DBusConnection* dbus_conn;
#define BLE_DEVICE_NAME   "/dev/ttyS0"


#define SERIAL_BUFFER_SIZE    1024

/* RX buffer */
#define UART_RX_BUF_SIZE	WSDP_UART_BUF_SIZE	/**< UART RX buffer size. */

static uint8_t  m_rx_msg_buf[UART_RX_BUF_SIZE];
static uint16_t m_rx_msg_buf_size = 0;


extern int keep_looping;

/* FD of serial (ttyS*) */
int serial_fd = -1;

/* RX buffer */
static pthread_mutex_t read_write_mutex;

CTRL_SC_DEVICE_INFO_S ble_info = {0};


/******************** Local Functions ********************/

static void dormir(unsigned long microsecs)
{
    struct timeval timeout;
    timeout.tv_sec = microsecs / 1000000L;
    timeout.tv_usec = microsecs - (timeout.tv_sec * 1000000L);
    select(1, 0, 0, 0, &timeout);
}

int ble_open(char *devname)
{
    int fd = 0;
	
	/* robin@wdl, For demo, rollback svn.717 */
#if 0
    struct termio stbuf;  /* termios */
    int clocal = 0, parity = 0, bits = CS8, stopbits = 0;
    int speed = B0; /* Set to B110, B150, B300,..., B38400 */
    long senddelay = 0; /* 0/100th second character delay for sending */
#endif

    fd = open(devname, O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY);
    if (fd < 0)
    {		
		ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,"Can't open device %s.\n", devname);	
        return -1;
    }

	/* robin@wdl, For demo, rollback svn.717 */
#if 0 
    if (ioctl(fd, TCGETA, &stbuf) < 0)
    {
        close(fd);
         ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Can't ioctl get device %s, please try again.\n", devname);
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
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Can't ioctl set device %s.\n", devname);
        return -1;
    }

    dormir(200000);    /* Wait a bit (DTR raise) */
#endif

    return fd;
}

void ble_close(int fd)
{
    if (fd != -1)
    {
        close(fd);
    }
}

static int ble_uart_config(int fd, int rate)
{
	int flags;
	struct termios tty;
	memset (&tty, 0, sizeof(tty));

	/* Set RTS signal to low to prevent the module
	 * from sending its plug and play string. */
	if (ioctl(fd, TIOCMGET, &flags) == 0)
	{
		flags &= ~TIOCM_RTS;
		ioctl(fd, TIOCMSET, &flags);
	}

	/* empty in and out serial buffers */
	tcflush(fd, TCIOFLUSH);

	/* get config attributes */
	if (tcgetattr (fd, &tty) != 0)
		return -1;

	/* IGNBRK: ignore BREAK condition on input
	 * IGNPAR: ignore framing errors and parity errors. */
	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;                /* RAW data mode */

	/* CS8: 8-bits character size
	 * CSTOPB: set two stop bits
	 * CREAD: enable receiver
	 * CLOCAL: ignore modem control lines */
	tty.c_cflag = CS8 | CREAD | CLOCAL;

	/* Do not echo characters because if you connect to a host it or your modem
	 * will echo characters for you.  Don't generate signals. */
	tty.c_lflag = 0;

	/* set serial port speed to 38400 bauds */
	cfsetspeed(&tty, rate);
	
	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		return -1;

	return 0;
}

static uint8_t calculate_bytes_xor(uint8_t base, uint8_t *bytes, uint16_t size)
{
	uint16_t num;
	uint8_t val = base;
	
	for (num = 0 ; num < size ; num++)
		val ^= bytes[num];
	
	return val;
}


static void print_cmd(int dir, unsigned char *cmd, int length)
{
    int i = 0;
    char buffer[1024] = {0};

    sprintf(buffer, "%s %02d cmd: [", (dir == 0) ? "Recv" : "Send", length);
    for (i = 0; i < length; i++)
        sprintf(buffer, "%s%02X ", buffer, cmd[i]);
    sprintf(buffer, "%s]", buffer);

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "%s\n", buffer);
}

int serial_init(void)
{
    serial_fd = ble_open(BLE_DEVICE_NAME);
    if (serial_fd == -1)
    {
		ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "open serial_fd failed!!!");
        return -1;
    }

	/* robin@wdl, For demo, rollback svn.717 */
	if (ble_uart_config(serial_fd, B115200) < 0)
	{
		ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "config uart failed!!!");
		close(serial_fd);
		serial_fd = -1;
		return -2;
	}

    pthread_mutex_init(&read_write_mutex, NULL);

    return 0;
}

void serial_uninit(void)
{
    if (serial_fd != -1)
    {
        ble_close(serial_fd);
    }

    pthread_mutex_destroy(&read_write_mutex);
}

int ble_write(int fd, unsigned char *buffer, int length)
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

			ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,"BLE: WRITE serial failed, res is %d, errno [%d:%s].\n", res, errno, strerror(errno)); 	
            return -1;
        }
        if (res == total)
        {
            return length;
        }
        if (res > total)
        {
			ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,"BLE: === BUG ===, res is %d, total is %d, errno [%d:%s].\n", res, total, errno, strerror(errno)); 	
			return -1;
        }
        
        total -= res;
        tmp += res;
    }

    return -1;
}

int ble_read(int fd, unsigned char *buffer, int length)
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

			ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,"BLE: READ serial failed, res is %d, errno [%d:%s].\n", res, errno, strerror(errno));	
            return -1;
        }
        
        return res;
    }

    return -1;
}


int serial_send(unsigned char *buffer, int length)
{
    int ret = 0;

    print_cmd(1, buffer, length);

    pthread_mutex_lock(&read_write_mutex);

    ret = ble_write(serial_fd, buffer, length);

    pthread_mutex_unlock(&read_write_mutex);

    if (ret != length)
    {
		ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,"BLE write cmd failed, write %d bytes, actually write %d bytes.\n", length, ret);
    }

    return ret;
}


static unsigned int GetCrc32(const char* InStr,unsigned short len)
{
	return 1;
    
    unsigned int i,j;
    unsigned int Crc;
    //  This excerpt of code is used to generate crc table.
    unsigned int Crc32Table[256] = {0};
    for (i = 0; i < 256; i++)
    {
        Crc = i;
        for (j = 0; j < 8; j++)
        {
            if (Crc & 1)
            {
                Crc = (Crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                Crc >>= 1;
            }
        }
        Crc32Table[i] = Crc;
    }
    
    
    
    Crc=0xffffffff;
    for(i = 0; i < len; i++)
    {
        Crc = (Crc >> 8)^(Crc32Table[(Crc ^ *InStr++)&0xff]);
    }
    
    Crc ^= 0xFFFFFFFF;
    return Crc;
}

void ctrl_process_user_fd_event(int fd_max, fd_set *rfds, DBusConnection* conn, DBusError *err, User_fd_EventHandle fd_handler)
{
#define FILESET_READ 1
    int ret = 0;
    CTRL_SIG_INFO_U info_s;
    int i;
    struct timeval select_timeout;
    select_timeout.tv_sec  = 0;
    select_timeout.tv_usec  = 0;

    if (!rfds)
        return;

    while (1) {
        ret = select(fd_max, rfds, NULL, NULL, &select_timeout);
        if (ret <= 0) {
            return;  // error or interrupt happened
        }
        for (i = 0; i < fd_max; i++) {
            if (FD_ISSET(i, rfds)) {
                info_s.user_file.fd = i;
                //info_s.user_file.fileset_type = FILESET_READ;
                //do something;
                ret = (*fd_handler)(info_s, conn, err);
                break;
            }
        }
    }
}

static void on_uart_packet_recv(DBusConnection * conn)
{
	WSDP_PKT_HDR_S * pkt_hdr;
	WSDP_MSG_HDR_S * msg_hdr;
	uint8_t checksum = 0x00;
	
	if (m_rx_msg_buf_size == 0)
		return;
	
	if (m_rx_msg_buf_size < (WSDP_PACKET_HDR_SIZE + WSDP_MSG_HDR_SIZE))
		return;
	
	/* Check packet header CRC */
	pkt_hdr = (WSDP_PKT_HDR_S *) m_rx_msg_buf;
	checksum = calculate_bytes_xor(0x00, (uint8_t *)pkt_hdr, WSDP_PACKET_HDR_SIZE - 1);
	if (checksum != pkt_hdr->checksum)
		return;
	
	/* Check message size */
	msg_hdr = (WSDP_MSG_HDR_S *) ((uint8_t*)pkt_hdr + WSDP_PACKET_HDR_SIZE);
	if (pkt_hdr->plen != (WSDP_MSG_HDR_SIZE + msg_hdr->length))
		return;
	
	/* Check packet payload CRC */
	if (pkt_hdr->flags & 0x01)
	{
		/* Check have payload crc byte */
		if ((m_rx_msg_buf_size - WSDP_PACKET_HDR_SIZE - WSDP_MSG_HDR_SIZE) <= msg_hdr->length)
			return;
		
		checksum = calculate_bytes_xor(0x00, (uint8_t *)msg_hdr, WSDP_MSG_HDR_SIZE + msg_hdr->length);
		if (checksum != m_rx_msg_buf[m_rx_msg_buf_size - 1])
			return;
	}
	
	/* Invoke message event handler */
	PraseBleMsg(conn, msg_hdr, (uint8_t*)msg_hdr + WSDP_MSG_HDR_SIZE);
}

static void on_uart_data_recv(DBusConnection * conn, uint8_t *buffer, int length)
{
	int i;
	for(i = 0; i < length; i++)
	{
		m_rx_msg_buf[m_rx_msg_buf_size] = buffer[i];
		switch (m_rx_msg_buf[m_rx_msg_buf_size])
		{
			/* End of the SLIP Packet */
			case WSDP_SLIP_END:
				on_uart_packet_recv(conn);
				m_rx_msg_buf_size = 0;
				break;
			
			/* Check switch byte by WSDP_SLIP_ESC_END */
			case WSDP_SLIP_ESC_END:
				if (m_rx_msg_buf_size > 0 && m_rx_msg_buf[m_rx_msg_buf_size - 1] == WSDP_SLIP_ESC)
					m_rx_msg_buf[m_rx_msg_buf_size - 1] = WSDP_SLIP_END;
				else
					m_rx_msg_buf_size++;
				break;
			
			/* Check switch byte by WSDP_SLIP_ESC_ESC */
			case WSDP_SLIP_ESC_ESC:
				if (m_rx_msg_buf_size > 0 && m_rx_msg_buf[m_rx_msg_buf_size - 1] == WSDP_SLIP_ESC)
					m_rx_msg_buf[m_rx_msg_buf_size - 1] = WSDP_SLIP_ESC;
				else
					m_rx_msg_buf_size++;
				break;
			
			/* Other byte */
			default:
				m_rx_msg_buf_size++;
				break;
		}
	}
}


int read_app_event(CTRL_SIG_INFO_U info, DBusConnection * conn, DBusError * err)
{

    uint8_t buffer[SERIAL_BUFFER_SIZE] = {0};
	//SC_BLE_MSG_INFO_S ble_msg;

    int ret = 0;
	int i;

	//memset(&ble_msg, 0 , sizeof(ble_msg));
    memset(buffer, 0, sizeof(buffer));

	pthread_mutex_lock(&read_write_mutex);
	ret = ble_read(serial_fd, buffer, sizeof(buffer));
	pthread_mutex_unlock(&read_write_mutex);
	ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,"read ret = %d.\n", ret);
	if (ret <= 0)
	{
		ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,"read serial port %s error.\n", BLE_DEVICE_NAME);
	}
		
	on_uart_data_recv(conn, buffer, ret);

	
    return ret;
}


static void PraseBleMsg(DBusConnection* dbus_conn, WSDP_MSG_HDR_S* msg_hdr, uint8_t* data)
{
	int ret, i;
	unsigned char *ptr;
	char buf[64];
	size_t data_size;
	int loop_index =0;

	switch (msg_hdr->type)
	{

		case BLE_MSG_DUMMY:	/*For debug purpose send the BLE Raw data */
		     loop_index =0;
		     ctrl_log_print(LOG_INFO,__FUNCTION__,__LINE__,"Dummy Data Length[%d]\n", msg_hdr->length);	
		     if ( msg_hdr->length >= 8)
		     {
		     	ctrl_log_print(LOG_INFO,__FUNCTION__,__LINE__,"Data: %X,%X,%X,%X,%X,%X,%X,%X\n", 
								data[0],data[1], data[2],data[3],data[4],
								data[5],data[6], data[7]);	
		     }
		     //while ( loop_index < msg_hdr->length )  	
		     //{	
		     //	 //printf("Inx[%d], data[%X]\n", loop_index, data[loop_index]);
		     //	 //printf("%x %d %d\n", 0xFF & buffer[i], rx_idx_in, rx_idx_out);
                     //
		     //   ctrl_log_print(LOG_INFO,__FUNCTION__,__LINE__,"Inx: [%d] data[%X]\n",
		     //	 						        loop_index,
		     // 	 						data[loop_index]);
		     //    loop_index = loop_index + 1 ;    	 	
		     //} 	
		     break; 	
		case BLE_MSG_DEBUG:
		     {
				char debug_str[128];
				memset(debug_str, 0, sizeof(debug_str));
				memcpy(debug_str, data, msg_hdr->length);
				ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,"debug_str = %s\n", debug_str);
				ret = ctrl_sendsignal_with_strvalue(dbus_conn, CTRL_COMMON_INTERFACE, CTRL_SIG_SC_BOARDCAST_BLE_MSG_DEBUG, debug_str);
				if (ret != 0)
				{
					ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
								   "ctrl sendsignal_with_strvalue failed, type[%s], ret[%d]\n", CTRL_SIG_SC_BOARDCAST_BLE_MSG_DEBUG, ret);
				}
				break;
			}
		
		case BLE_MSG_LE_SC_BLE_STATE:
			{
				uint8_t state;
				memset(&state, 0, sizeof(state));
				memcpy(&state, data, msg_hdr->length);
				
				ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,"ble_state = %0x\n", state);
				ctrl_sendsignal_with_intvalue(dbus_conn, CTRL_COMMON_INTERFACE, CTRL_SIG_SC_BOARDCAST_BLESTATE, state);
				if (ret != 0)
				{
					ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
								   "ctrl sendsignal_with_intvalue failed, type[%s], ret[%d]\n", CTRL_SIG_SC_BOARDCAST_BLESTATE, ret);
				}

				break;
			}
		
		case BLE_MSG_LE_SC_DEVICE_INFO:
			{
				SP_BLE_DEVICE_INFO_S device_info;
				memset(&device_info, 0, sizeof(device_info));
				memcpy(&device_info, data, msg_hdr->length);
				ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,
							   "ble_bdaddr = %02x:%02x:%02x:%02x:%02x:%02x, device_name = %s, service_uuid =%x\n",
							   device_info.bdaddr[5], device_info.bdaddr[4], device_info.bdaddr[3], 
							   device_info.bdaddr[2], device_info.bdaddr[1], device_info.bdaddr[0], 
							   device_info.device_name, device_info.service_uuid);

				ret = ctrl_sendsignal_with_struct(dbus_conn, CTRL_COMMON_INTERFACE, CTRL_SIG_SC_BOARDCAST_DEVICEINFO, &device_info, sizeof(device_info));
				if (ret != 0)
				{
					ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
								   "ctrl sendsignal, type[%s], ret[%d]\n", CTRL_SIG_SC_BOARDCAST_DEVICEINFO, ret);
				}

				break;
			}
		case BLE_MSG_LE_SC_BATTERY_LEVEL:
			{
				uint8_t level;
				memset(&level, 0, sizeof(level));
				memcpy(&level, data, msg_hdr->length);
				ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,"level = %0x\n", level);
				sp_ble_set_battery_level(level);
				ret = ctrl_sendsignal_with_intvalue(dbus_conn, CTRL_COMMON_INTERFACE, CTRL_SIG_SC_BOARDCAST_BATTERY_LEVEL, level);
				if (ret != 0)
				{
					ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
								   "ctrl sendsignal failed, type[%s], ret[%d]\n", CTRL_SIG_SC_BOARDCAST_BATTERY_LEVEL, ret);
				}

				break;
			}
		case BLE_MSG_LE_SC_CURRENT_TIME:
			{
				uint8_t time[8];
				memset(&time, 0, sizeof(time));
				memcpy(&time, data, msg_hdr->length);
				ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,"current time, len[%u], %u-%u-%u %u:%u:%u.", 
					msg_hdr->length, *(uint16_t *) &time[0], &time[2], &time[3],&time[4], &time[5], &time[6]);
				ret = ctrl_sendsignal_with_strvalue(dbus_conn, CTRL_COMMON_INTERFACE, CTRL_SIG_SC_BOARDCAST_CURRENT_TIME, (char*)time);
				if (ret != 0)
				{
					ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
								   "ctrl sendsignal failed, type[%s], ret[%d]\n", CTRL_SIG_SC_BOARDCAST_CURRENT_TIME, ret);
				}

				break;
			}
		case BLE_MSG_CTRL_SET_LED_STATUS:	
		{
				SP_LED_STATUS_S st_led;
				memset(&st_led, 0, sizeof(st_led));
				memcpy(&st_led, data, msg_hdr->length);
				
				ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,"LED State , RGB[%X], Noty_type[%X], led_state[%X], time[%X]", 
								st_led.val, st_led.notify_type, st_led.led_state, st_led.led_time);
				
				/* SEt LED State to sp_led_ctrl */
				sp_ble_send_led_state_message(st_led.val, st_led.notify_type, st_led.led_time) ;
				break;
		}
		default:
		{
			break;
		}
	
			
	}
}



