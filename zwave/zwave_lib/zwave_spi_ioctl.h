/*******************************************************************************
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
#ifndef __ZWAVE_SPI_IOCTL_H__
#define __ZWAVE_SPI_IOCTL_H__

/* ZWAVE_SPI Device Major Number */
#define ZWAVE_SPI_MAJOR_NUM         220

#define  ZW010x   0x01
#define  ZW020x   0x02
#define  ZW030x   0x03
#define  ZW040x   0x04                          /*ZW040x Id in PC to Programmer interface*/
#define  ZW050x   0x05                          /*ZW050x Id in PC to Programmer interface*/

#define ZW_PAGE_MAX_NUM             128
#define ZW_PAGE_SIZE                256    /* Same size for ZW020x/030x */
#define FIRMWARE_MAX_NAME           128
#define ZWAVE_FW_MAX_SIZE           ZW_PAGE_MAX_NUM * ZW_PAGE_SIZE
#define ZWAVE_FW_MAX_SIZE_ZW5XX           ZW_PAGE_MAX_NUM * ZW_PACKET_SIZE_ZW5XX
#define INTELHEX_PARSER_SUCCESS     1
#define INTELHEX_PARSER_ERROR       0
#define TRUE                        1
#define FALSE                       0

#define ZW_PACKET_SIZE_ZW5XX      1024

#define ZW_NVR_SIZE 112


/* IOCTL MAGIC */
static const unsigned char ZWAVESPI_MAGIC = 'z' | 'w' | 'a' | 'v' | 'e' | 's' | 'p' | 'i';
/* IOCTL parameters */
#define ZWAVE_SPI_IOCTL_ZW0x0x_PROG_ENABLE           _IO(ZWAVESPI_MAGIC, 0)
#define ZWAVE_SPI_IOCTL_ZW0x0x_PROG_RELEASE          _IO(ZWAVESPI_MAGIC, 1)
#define ZWAVE_SPI_IOCTL_ZW0x0x_CHIP_ERASE            _IO(ZWAVESPI_MAGIC, 2)
#define ZWAVE_SPI_IOCTL_ZW0x0x_READ_PAGE             _IO(ZWAVESPI_MAGIC, 3)
#define ZWAVE_SPI_IOCTL_ZW0x0x_WRITE_PAGE            _IO(ZWAVESPI_MAGIC, 4)
#define ZWAVE_SPI_IOCTL_ZW0x0x_READ_NVR              _IO(ZWAVESPI_MAGIC, 5)
#define ZWAVE_SPI_IOCTL_ZW0x0x_SET_NVR               _IO(ZWAVESPI_MAGIC, 6)
#define ZWAVE_SPI_IOCTL_ZW0x0x_READ_SRAM             _IO(ZWAVESPI_MAGIC, 7)

/* IOCTL structure */
typedef struct _zwave_page_s
{
    unsigned int pageNo;
    unsigned char data[ZW_PAGE_SIZE];
} ZWAVE_PAGE_S;

typedef struct _zwave_read_s
{
    unsigned int pageNo;
    unsigned int all;
    int error;
    unsigned char data[ZWAVE_FW_MAX_SIZE_ZW5XX];
} ZWAVE_READ_S;

typedef struct _zwave_write_s
{
    unsigned char chipType;
    int error;
    unsigned char data[ZWAVE_FW_MAX_SIZE_ZW5XX];
} ZWAVE_WRITE_S;

typedef struct _intel_hex_s
{
    size_t          extended_len;
    size_t          data_len;
    size_t          offset;
    unsigned char   *data;
    unsigned char   base;
    unsigned char   extended;
} INTEL_HEX_S;

typedef struct _zwave_read_nvr_s
{
    unsigned int addr;
    int error;
    unsigned char data[ZW_NVR_SIZE];
} ZWAVE_READ_NVR_S;

typedef struct _zwave_set_nvr_s
{
    unsigned int addr;
    int error;
    unsigned char data[ZW_NVR_SIZE];
} ZWAVE_SET_NVR_S;

typedef struct _zwave_read_sram_s
{
    unsigned int addr;
    int error;
    unsigned char data[ZW_PACKET_SIZE_ZW5XX * 4];
} ZWAVE_READ_SRAM_S;

int intel_hex_read(int fd, INTEL_HEX_S *ihex);
#endif
