/*!
*  \file       cmbs_dbg.h
*  \brief      This file contains debug functions for CMBS API
*  \author     andriig
*
*  @(#)  %filespec: cmbs_dbg.h~DMZD53#6 %
*
*******************************************************************************/

#if   !defined( CMBS_DBG_H )
#define  CMBS_DBG_H

#include "cmbs_api.h"               /* CMBS API definition */
#include "cmbs_han.h"

const char* cmbs_dbg_GetEventName(E_CMBS_EVENT_ID id);
const char* cmbs_dbg_GetCommandName(CMBS_CMD id);
const char* cmbs_dbg_GetParamName(E_CMBS_PARAM e_ParamId);
const char* cmbs_dbg_GetIEName(E_CMBS_IE_TYPE e_IE);
const char* cmbs_dbg_GetHsTypeName(E_CMBS_HS_TYPE e_HsType);
const char* cmbs_dbg_GetToneName(E_CMBS_TONE e_Tone);
const char* cmbs_dbg_GetCallProgressName(E_CMBS_CALL_PROGRESS e_Prog);
const char* cmbs_dbg_GetCallTypeName(E_CMBS_CALL_STATE_TYPE    e_CallType);
const char* cmbs_dbg_GetCallStateName(E_CMBS_CALL_STATE_STATUS  e_CallStatus);
E_CMBS_PARAM cmbs_dbg_String2E_CMBS_PARAM(const char *psz_Value);
E_CMBS_TONE cmbs_dbg_String2E_CMBS_TONE(const char *psz_Value);
E_CMBS_CALL_PROGRESS cmbs_dbg_String2E_CMBS_CALL_PROGR(const char *psz_Value);

void    cmbs_dbg_DumpIEList(u8 *pu8_Buffer, u16 u16_Size);
void    cmbs_dbg_CmdTrace(const char *message, ST_CMBS_SER_MSGHDR *pst_Hdr, u8 *pu8_Buffer);
void cmbs_dbg_getTimestampString(char *buffer);

/* Parse IE Function */
typedef u32(*pfn_cmbs_dbg_ParseIEFunc)(char *pOutput, u32 u32_OutputSize, void *pv_IE, u16 u16_IE);
void cmbs_dbg_SetParseIEFunc(pfn_cmbs_dbg_ParseIEFunc);


#define CMBS_MAX_IE_PRINT_SIZE  4000

#endif
