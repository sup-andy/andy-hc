/*!
*  \file       tcx_eep.h
*  \brief
*  \Author     sergiym
*
*  @(#)  %filespec: tcx_eep.h~1 %
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
*******************************************************************************/

#if   !defined( TCX_EEP_H )
#define  TCX_EEP_H


#if defined( __cplusplus )
extern "C"
{
#endif

typedef struct
{
    u8 m_Initialized;
    u8 m_DectType;
    u8 m_RF_FULL_POWER;
    u8 m_MAXUsableRSSI;
    u8 m_LowerRSSILimit;
    u8 m_PreamNormal;
    u8 m_RF19APUSupportFCC;
    u8 m_RF19APUDeviation;
    u8 m_RF19APUPA2Comp;
    u8 m_PHSScanParam;
    u8 m_JDECTLevelM62;
    u8 m_JDECTLevelM82;
    u8 m_RFPI[CMBS_PARAM_RFPI_LENGTH];
    u8 m_RXTUN[CMBS_PARAM_RXTUN_LENGTH];
    u8 m_SUBSDATA[CMBS_PARAM_SUBS_DATA_LENGTH];
    u8 m_RVREF;
    u8 m_GFSK;
} CMBS_EEPROM_DATA;

int tcx_EepOpen(char * psz_EepFileName);

int tcx_EepRead(u8* pu8_OutBuf, u32 u32_Offset, u32 u32_Size);

void tcx_EepWrite(u8* pu8_InBuf, u32 u32_Offset, u32 u32_Size);

void tcx_EepClose(void);

u32 tcx_EepSize(void);

int tcx_EepNewFile(const char * psz_EepFileName, u32 size);

#if defined( __cplusplus )
}
#endif

#endif   // TCX_EEP_H
//*/
