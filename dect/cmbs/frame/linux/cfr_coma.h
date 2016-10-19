/*!
*  \file       cfr_coma.h
*  \brief
*  \author
*******************************************************************************/

#if   !defined( CFR_COMA_H )
#define  CFR_COMA_H

/*! current packet transmission size */
#define     CFR_BUFFER_WINDOW_SIZE  3

/*! identifier for receive path */
#define  CFR_BUFFER_COMA_REC  0
/*! identifier for transmit path */
#define  CFR_BUFFER_COMA_TRANS 1

#if defined( __cplusplus )
extern "C"
{
#endif
void  *      cfr_comaThread(void * pVoid);
int          cfr_comaInitialize(void *p);
int    cfr_comaPacketPartWrite(u8* pu8_Buffer, u16 u16_Size);
void         cfr_comaPacketWriteFinish(u8 u8_BufferIDX);
CFR_E_RETVAL cfr_comaPacketPrepare(u16 u16_size);
void   cfr_comaDataTransmitKick(void);
void         cfr_comaStop(void);

#if defined( __cplusplus )
}
#endif

#endif   // CFR_COMA_H
//*/

