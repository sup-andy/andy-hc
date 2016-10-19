/*!
* \file   tcx_keyb.h
* \brief
* \Author  kelbch
*
* @(#) %filespec: tcx_keyb.h~3 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/

#if !defined( TCX_KEYB_H )
#define TCX_KEYB_H


#if defined( __cplusplus )
extern "C"
{
#endif

void  keyboard_loop(void);

void  keyb_SRVLoop(void);
void  keyb_CallLoop(void);
void  keyb_SwupLoop(void);

typedef struct
{
    u8 u8_prot_disc ;
    u8 u8_desc_type;
    u8 u8_EMC_high;
    u8 u8_EMC_low;
    u8 u8_length;
    u8 u8_command;
    u8 u8_total_len_high;
    u8 u8_total_len_low;
} st_DataCall_Header;

#define PROTOCOL_DISCRIMINATOR  0xC0
#define DESCRIMINATOR_TYPE   0x81
#define EMC_HIGH     0x3
#define EMC_LOW      0x4
#define HEADER_SIZE     sizeof(st_DataCall_Header)
#define FIRST_PACKET_COMMAND        0x31
#define NEXT_PACKET_COMMAND         0x30
#if defined( __cplusplus )
}
#endif

#endif // TCX_KEYB_H
//*/
