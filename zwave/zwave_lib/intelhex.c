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
/******************************************************************************
 *    Department:
 *    Project :
 *    Block   :
 *    Creator : Roger Chen
 *    File    : intelhex.c
 *    Abstract: Z-wave Flash download SPI driver
 *    Date    : 12/29/2011
 *    $Id:$
 *
 *    Modification History:
 *    By           Date       Ver.   Modification Description
 *    -----------  ---------- -----  -----------------------------
 *    Roger.Chen   12/29/2011  0.1    Initial version
 *    Rose.kao     05/07/2014  1.0    Added hex2bin function.
 *                                    This fix intel_hex_read() analyze hex file.
 *                                    Reference hex2bin code.
 ******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "zwave_spi_ioctl.h"

#if 0
#define DEBUG(str, args...) printf(str, ## args)
#else
#define DEBUG(str, args...)
#endif

/* detect CPU endian */
char cpu_le(void)
{
    const unsigned long cpu_le_test = 0x12345678;

    return((unsigned char *) &cpu_le_test)[0] == 0x78;
}

unsigned long be_u32(const unsigned long v)
{
    if(cpu_le())
        return((v & 0xFF000000) >> 24) | ((v & 0x00FF0000) >> 8) | ((v & 0x0000FF00) << 8) | ((v & 0x000000FF) << 24);
    return v;
}

int intel_hex_read(int fd, INTEL_HEX_S *ihex)
{
    char            start_code;
    int             i;
    unsigned char   checksum;
    unsigned int    c;
    unsigned long   base = 0;
    unsigned int    last_address = 0x0;

    DEBUG("%s() L[%d]\n", __FUNCTION__, __LINE__);

    /*
     * Intel Hex format ;
     * start code : an ASCII colon ":" Byte Count : two hex digits Address : four hex
     * digits Record Type : two hex digits, 00 to 05 Data : a sequench of n byts
     * Checksum : two hex digits - the least significant byte of the 2's complement of
     * the sum of the values of all fields except fields 1 and 6 Exampe :
     * ":02002000B72205" [:][02][0020][00][B722][05]
     */
    while(read(fd, &start_code, 1) != 0)
    {
        char            buffer[9];
        unsigned int    reclen = 0, address = 0, type = 0;
        unsigned char   *record = NULL;
        unsigned int    len;

        if(start_code == '\n' || start_code == '\r') continue;
        if(start_code != ':') return INTELHEX_PARSER_ERROR;

        // Extract reclen, address, and type from each line
        buffer[8] = 0;
        if(read(fd, (char *) &buffer, 8) != 8)
        {
            return INTELHEX_PARSER_ERROR;
        }

        if(sscanf(buffer, "%2x%4x%2x", &reclen, &address, &type) != 3)
        {
            close(fd);
            return INTELHEX_PARSER_ERROR;
        }

        // 1st, add each each bytes for checksum
        checksum = reclen + ((address & 0xFF00) >> 8) + ((address & 0x00FF) >> 0) + type;

        switch(type)
        {

                // data record
            case 0:
                c = address - last_address;
                ihex->data = realloc(ihex->data, ihex->data_len + c + reclen);

                // if there is a gap, padding with 0xff and increment the length
                if(c > 0)
                {
                    memset(&ihex->data[ihex->data_len], 0xff, c);
                    ihex->data_len += c;
                }

                last_address = address + reclen;
                record = &ihex->data[ihex->data_len];
                ihex->data_len += reclen;
                break;

                // extended segment address record
            case 2:
                base = 0;
                break;

                // extended linear address record
            case 4:
                base = address;
                break;
        }

        buffer[2] = 0;
        for(i = 0; i < reclen; ++i)
        {
            if(read(fd, (char *) &buffer, 2) != 2 || sscanf(buffer, "%2x", &c) != 1)
            {
                close(fd);
                return INTELHEX_PARSER_ERROR;
            }

            // 2nd, add each bytes to the checksum
            checksum += c;
            switch(type)
            {
                case 0:
                    record[i] = c;
                    break;

                case 2:
                case 4:
                    ihex->extended_len = ihex->data_len;
                    ihex->extended = c;
                    base = (base << 8) | c;
                    break;
            }
        }

        // read, scan, and verify the checksum
        if(read(fd, (char *) &buffer, 2) != 2
                ||  sscanf(buffer, "%2x", &c) != 1
                || (unsigned char)(checksum + c) != 0x00)
        {
            close(fd);
            return INTELHEX_PARSER_ERROR;
        }

        switch(type)
        {

                // EOF
            case 1:
                close(fd);
                return INTELHEX_PARSER_SUCCESS;

                // address record
            case 2:
                base = base << 4;

            case 4:
                base = be_u32(base);

                // Reset last_address since our base changed
                last_address = 0;

                if(ihex->base == 0)
                {
                    ihex->base = base;
                    break;
                }

                // Cannot handle with files out of order
                if(base < ihex->base)
                {
                    close(fd);
                    return INTELHEX_PARSER_ERROR;
                }

                // if there is a gap, enlarge and fill with zeros
                len = base - ihex->base;
                if(len > ihex->data_len)
                {
                    ihex->data = realloc(ihex->data, len);
                    memset(&ihex->data[ihex->data_len], 0, len - ihex->data_len);
                    ihex->data_len = len;
                }
                break;
        }

        DEBUG("start_code[%c] relen[%2x], address[%4x] type[%2x] checksum[%2x]\n",
              start_code, reclen, address, type, checksum);
    }

    close(fd);
    return INTELHEX_PARSER_SUCCESS;
}



/*
 *  hex2bin converts an Intel hex file to binary.
 */

#define MAX_FILE_NAME_SIZE 81
/* The data records can contain 255 bytes: this means 512 characters. */
#define MAX_LINE_SIZE 1024
/* size in bytes */
#define MEMORY_SIZE 4096*1024

#define NO_ADDRESS_TYPE_SELECTED 0
#define LINEAR_ADDRESS 1
#define SEGMENTED_ADDRESS 2

typedef unsigned char byte;
typedef int boolean;
typedef unsigned short word;

int GetLine(char* str, FILE *in)
{
    char *result;
    int ret = 0;

    result = fgets(str, MAX_LINE_SIZE, in);
    if((result == NULL) && !feof(in))
    {
        fprintf(stderr, "Error occurred while reading from file\n");
        ret = -1;
    }

    return ret;
}


int hex2bin(char *Flnm, char *bin)
{
#ifdef DEBUG_VERIFICATIOIN
    int     numByte;
#endif
    int     j;
    int     ret = 0;
    FILE    *Filin;
    int     Pad_Byte = 0xFF;

#define CKS_8     0
#define CKS_16LE  1
#define CKS_16BE  2

    unsigned short int wCKS;
    unsigned short int w;
    unsigned int Cks_Type = CKS_8;
    unsigned int Cks_Start = 0, Cks_End = 0, Cks_Addr = 0, Cks_Value = 0;
    boolean Cks_range_set = FALSE;
    boolean Cks_Addr_set = FALSE;

    /* flag that a file was read */
    boolean Enable_Checksum_Error = FALSE;
    boolean Status_Checksum_Error = FALSE;
    boolean Max_Length_Setted = FALSE;

    /* cmd-line parameter # */
    char    *p;

    unsigned int    Max_Length = MEMORY_SIZE;
    unsigned int    Record_Nb;

    /* This will hold binary codes translated from hex file. */
    byte    *Memory_Block;

    /* Application specific */
    unsigned int Nb_Bytes;
    unsigned int First_Word, Address, Segment, Upper_Address;
    unsigned int Lowest_Address, Highest_Address, Starting_Address = 0;
    unsigned int temp;
    unsigned int Phys_Addr, Type;
    byte    Data_Str[MAX_LINE_SIZE];
    byte    Checksum = 0; // 20040617+ Added initialisation to remove GNU compiler warning about possible uninitialised usage
    int temp2;

    /* We will assume that when one type of addressing is selected, it will be valid for all the
        current file. Records for the other type will be ignored. */
    unsigned int    Seg_Lin_Select = NO_ADDRESS_TYPE_SELECTED;

    /* line inputted from file */
    char Line[MAX_LINE_SIZE];

    /* This mask is for mapping the target binary inside the
     binary buffer. If for example, we are generating a binary
     file with records starting at FFF00000, the bytes will be
     stored at the beginning of the memory buffer. */
    unsigned int Address_Mask;

    /* Just a normal file name */
    if((Filin = fopen(Flnm, "r")) == NULL)
    {
        fprintf(stderr, "%s() L[%d] Input file %s cannot be opened.\n", __FUNCTION__, __LINE__, Flnm);
        return -1;
    }

    /* allocate a buffer */
    if((Memory_Block = malloc(Max_Length)) == NULL)
    {
        fprintf(stderr, "Can't allocate memory.\n");
        fclose(Filin);
        return -1;
    }

    /* For EPROM or FLASH memory types, fill unused bytes with FF or the value specified by the p option */
    memset(Memory_Block, Pad_Byte, Max_Length);


    /* To begin, assume the lowest address is at the end of the memory.
     While reading each records, subsequent addresses will lower this number.
     At the end of the input file, this value will be the lowest address.

     A similar assumption is made for highest address. It starts at the
     beginning of memory. While reading each records, subsequent addresses will raise this number.
     At the end of the input file, this value will be the highest address. */
    Lowest_Address = Max_Length - 1;
    Highest_Address = 0;
    Segment = 0;
    Upper_Address = 0;
    Record_Nb = 0;

    /* Max length must be in powers of 2: 1,2,4,8,16,32, etc. */
    Address_Mask = Max_Length - 1;

    /* Read the file & process the lines. */
    do   /* repeat until EOF(Filin) */
    {
        unsigned int i;

        /* Read a line from input file. */
        ret = GetLine(Line, Filin);
        if(ret != 0)
            goto error;

        Record_Nb++;

        /* Remove carriage return/line feed at the end of line. */
        i = strlen(Line) - 1;

        if(Line[i] == '\n') Line[i] = '\0';

#ifdef DEBUG_VERIFICATIOIN
        printf("[%d] ", Record_Nb);
        for(j = 0; j < i; j++)
        {
            printf("%c", Line[j]);
        }
        printf("\n");
#endif
        /* Scan the first two bytes and nb of bytes.
           The two bytes are read in First_Word since its use depend on the
           record type: if it's an extended address record or a data record.
        */
        sscanf(Line, ":%2x%4x%2x%s", &Nb_Bytes, &First_Word, &Type, Data_Str);

        Checksum = Nb_Bytes + (First_Word >> 8) + (First_Word & 0xFF) + Type;

        p = (char *) Data_Str;

#ifdef DEBUG_VERIFICATIOIN
        printf("[%d] ", Record_Nb);
        printf("[%2x][%4x][%2x] [%s] [%2x]",
               Nb_Bytes, First_Word, Type, p, Checksum);
        printf("\n");
#endif

        /* If we're reading the last record, ignore it. */
        switch(Type)
        {
                /* Data record */
            case 0:
                if(Nb_Bytes == 0)
                {
                    fprintf(stderr, "0 byte length Data record ignored\n");
                    break;
                }

                Address = First_Word;

                if(Seg_Lin_Select == SEGMENTED_ADDRESS)
                    Phys_Addr = (Segment << 4) + Address;
                else
                    /* LINEAR_ADDRESS or NO_ADDRESS_TYPE_SELECTED
                       Upper_Address = 0 as specified in the Intel spec. until an extended address
                       record is read. */
                    Phys_Addr = ((Upper_Address << 16) + Address) & Address_Mask;

                /* Check that the physical address stays in the buffer's range. */
                if((Phys_Addr + Nb_Bytes) <= Max_Length)
                {
                    /* Set the lowest address as base pointer. */
                    if(Phys_Addr < Lowest_Address)
                        Lowest_Address = Phys_Addr;

                    /* Same for the top address. */
                    temp = Phys_Addr + Nb_Bytes - 1;

                    if(temp > Highest_Address)
                        Highest_Address = temp;

                    /* Read the Data bytes. */
                    /* Bytes are written in the Memory block even if checksum is wrong. */
                    i = Nb_Bytes;

                    do
                    {
                        sscanf(p, "%2x", &temp2);
                        p += 2;

                        /* Overlapping record will erase the pad bytes */
                        if(Memory_Block[Phys_Addr] != Pad_Byte) fprintf(stderr, "Overlapped record detected\n");

                        Memory_Block[Phys_Addr++] = temp2;
                        Checksum = (Checksum + temp2) & 0xFF;
                    }
                    while(--i != 0);

                    /* Read the Checksum value. */
                    sscanf(p, "%2x", &temp2);

                    /* Verify Checksum value. */
                    if((((Checksum + temp2) & 0xFF) != 0) && Enable_Checksum_Error)
                    {
                        fprintf(stderr, "Checksum error in record %d: should be %02X\n", Record_Nb, (256 - Checksum) & 0xFF);
                        Status_Checksum_Error = TRUE;
                    }
                }
                else
                {
                    if(Seg_Lin_Select == SEGMENTED_ADDRESS)
                        fprintf(stderr, "Data record skipped at %4X:%4X\n", Segment, Address);
                    else
                        fprintf(stderr, "Data record skipped at %8X\n", Phys_Addr);
                }

                break;

                /* End of file record */
            case 1:
                /* Simply ignore checksum errors in this line. */
                break;

                /* Extended segment address record */
            case 2:
                /* First_Word contains the offset. It's supposed to be 0000 so
                 we ignore it. */

                /* First extended segment address record ? */
                if(Seg_Lin_Select == NO_ADDRESS_TYPE_SELECTED)
                    Seg_Lin_Select = SEGMENTED_ADDRESS;

                /* Then ignore subsequent extended linear address records */
                if(Seg_Lin_Select == SEGMENTED_ADDRESS)
                {
                    sscanf(p, "%4x%2x", &Segment, &temp2);

                    /* Update the current address. */
                    Phys_Addr = (Segment << 4) & Address_Mask;

                    /* Verify Checksum value. */
                    Checksum = (Checksum + (Segment >> 8) + (Segment & 0xFF) + temp2) & 0xFF;

                    if((Checksum != 0) && Enable_Checksum_Error)
                        Status_Checksum_Error = TRUE;
                }
                break;

                /* Start segment address record */
            case 3:
                /* Nothing to be done since it's for specifying the starting address for
                 execution of the binary code */
                break;

                /* Extended linear address record */
            case 4:
                /* First_Word contains the offset. It's supposed to be 0000 so
                 we ignore it. */

                /* First extended linear address record ? */
                if(Seg_Lin_Select == NO_ADDRESS_TYPE_SELECTED)
                    Seg_Lin_Select = LINEAR_ADDRESS;

                /* Then ignore subsequent extended segment address records */
                if(Seg_Lin_Select == LINEAR_ADDRESS)
                {
                    sscanf(p, "%4x%2x", &Upper_Address, &temp2);

                    /* Update the current address. */
                    Phys_Addr = (Upper_Address << 16) & Address_Mask;

                    /* Verify Checksum value. */
                    Checksum = (Checksum + (Upper_Address >> 8) + (Upper_Address & 0xFF) + temp2)
                               & 0xFF;

                    if((Checksum != 0) && Enable_Checksum_Error)
                        Status_Checksum_Error = TRUE;
                }
                break;

                /* Start linear address record */
            case 5:
                /* Nothing to be done since it's for specifying the starting address for
                 execution of the binary code */
                break;
            default:
                fprintf(stderr, "Unknown record type\n");
                break;
        }

    }
    while(!feof(Filin));

    // Max_Length is set; the memory buffer is already filled with pattern before
    // reading the hex file. The padding bytes will then be added to the binary file.
    if(Max_Length_Setted == TRUE) Highest_Address = Starting_Address + Max_Length - 1;

#ifdef DEBUG_VERIFICATIOIN
    // Turn on only when you like to verify data
    numByte = 0;
    for(j = 0; j < Highest_Address; j++)
    {
        if(j % 16 == 0)
        {
            printf("\n[%08x] ", numByte);
            numByte = numByte + 16;
        }
        printf("%02x", Memory_Block[j]);
    }
    printf("\n");
#endif

    fprintf(stdout, "==============================\n");
    fprintf(stdout, "Lowest address  = %08X\n", Lowest_Address);
    fprintf(stdout, "Highest address = %08X\n", Highest_Address);
    fprintf(stdout, "Pad Byte        = %X\n",  Pad_Byte);


    /* Add a checksum to the binary file */
    wCKS = 0;
    if(!Cks_range_set)
    {
        Cks_Start = Lowest_Address;
        Cks_End = Highest_Address;
    }
    switch(Cks_Type)
    {
            unsigned int i;

        case CKS_8:

            for(i = Cks_Start; i <= Cks_End; i++)
            {
                wCKS += Memory_Block[i];
            }

            fprintf(stdout, "8-bit Checksum = %02X\n", wCKS & 0xff);
            if(Cks_Addr_set)
            {
                wCKS = Cks_Value - (wCKS - Memory_Block[Cks_Addr]);
                Memory_Block[Cks_Addr] = (byte)(wCKS & 0xff);
                fprintf(stdout, "Addr %08X set to %02X\n", Cks_Addr, wCKS & 0xff);
            }
            break;

        case CKS_16BE:

            for(i = Cks_Start; i <= Cks_End; i += 2)
            {
                w =  Memory_Block[i + 1] | ((word)Memory_Block[i] << 8);
                wCKS += w;
            }

            fprintf(stdout, "16-bit Checksum = %04X\n", wCKS);
            if(Cks_Addr_set)
            {
                w = Memory_Block[Cks_Addr + 1] | ((word)Memory_Block[Cks_Addr] << 8);
                wCKS = Cks_Value - (wCKS - w);
                Memory_Block[Cks_Addr] = (byte)(wCKS >> 8);
                Memory_Block[Cks_Addr + 1] = (byte)(wCKS & 0xff);
                fprintf(stdout, "Addr %08X set to %04X\n", Cks_Addr, wCKS);
            }
            break;

        case CKS_16LE:

            for(i = Cks_Start; i <= Cks_End; i += 2)
            {
                w =  Memory_Block[i] | ((word)Memory_Block[i + 1] << 8);
                wCKS += w;
            }

            fprintf(stdout, "16-bit Checksum = %04X\n", wCKS);
            if(Cks_Addr_set)
            {
                w = Memory_Block[Cks_Addr] | ((word)Memory_Block[Cks_Addr + 1] << 8);
                wCKS = Cks_Value - (wCKS - w);
                Memory_Block[Cks_Addr + 1] = (byte)(wCKS >> 8);
                Memory_Block[Cks_Addr] = (byte)(wCKS & 0xff);
                fprintf(stdout, "Addr %08X set to %04X\n", Cks_Addr, wCKS);
            }

        default:
            ;
    }
    fprintf(stdout, "==============================\n");
#ifdef DEBUG_VERIFICATIOIN
    // Turn on only when you like to verify data
    numByte = 0;
    for(j = 0; j < Highest_Address; j++)
    {
        if(j % 16 == 0)
        {
            printf("\n[%08x] ", numByte);
            numByte = numByte + 16;
        }
        printf("%02x", Memory_Block[j]);
    }
    printf("\n");
#endif
    for(j = 0; j <= Highest_Address; j++)
    {
        bin[j] = Memory_Block[j];
    }

    if(Status_Checksum_Error && Enable_Checksum_Error)
    {
        fprintf(stderr, "Checksum error detected.\n");
        ret = 1;
    }

error:
    free(Memory_Block);
    fclose(Filin);

    return ret;

}



