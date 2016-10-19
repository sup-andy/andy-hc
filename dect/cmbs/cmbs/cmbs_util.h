/*!
*  \file       cmbs_util.h
*  \brief      This file contains utility functions for CMBS API usage
*  \author     andriig
*
*  @(#)  %filespec: cmbs_util.h~DMZD53#2 %
*
*******************************************************************************/

#if   !defined( CMBS_UTIL_H )
#define  CMBS_UTIL_H

#include "cmbs_api.h"               /* CMBS API definition */

u16   cmbs_util_GetParameterLength(E_CMBS_PARAM e_Param);
E_CMBS_RC   cmbs_util_ParameterValid(E_CMBS_PARAM e_Param, u16 u16_DataLen);
bool  cmbs_util_RawPayloadEvent(u16 u16_EventID);

#endif
