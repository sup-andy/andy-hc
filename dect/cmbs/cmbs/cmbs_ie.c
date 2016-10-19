/*!
* \file  cmbs_ie.c
* \brief  Information Elements List functions
* \Author  CMBS Team
*
* @(#) %filespec: cmbs_ie.c~DMZD53#14.10 %
*
*******************************************************************************
* \par History
* \n==== History =============================================================\n
* date   name  version  action
* ----------------------------------------------------------------------------
*
*  27-May-14  ronenw  --- GIT--  Move U_Buffer to cfr_ie.h
*  27-Jan-14  tcmc_asa  --- GIT--  solve checksum failure issues
*  13-Jan-2014  tcmc_asa --  GIT--   take checksum changes from 3.46.x to 3_x main (3.5x)
*  20-Dec-2013  tcmc_asa -- GIT--  added Checksum support
*  12-Jun-2013  tcmc_asa -  GIT ---  take changes form 2.99.9 to 3.x branch
*  28-Jan-2013  tcmc_asa  14.1.8.1.6.1.13 added cmbs_api_ie_CalledNameAdd/Get
*/


#include "cfr_ie.h"
#include "cfr_debug.h"
#include "cmbs_han_ie.h"
#include "cmbs_int.h"
#if defined(__arm)
#include "tclib.h"
#include "embedded.h"
#else
#include <string.h>
#endif

#if defined ( CMBS_API_TARGET )
#define CMBS_IE_GET_BUFFER(NAME)            \
 u8 *u8_Buffer = (u8 *)&s_Buffer.NAME;         \
 memset(u8_Buffer,0,sizeof(s_Buffer.NAME));
#else
#define CMBS_IE_GET_BUFFER(NAME)            \
 u8 * u8_Buffer;               \
 u8 u8_ThreadIdx = cfr_ie_getThreadIdx();         \
 if ( u8_ThreadIdx == CMBS_UNKNOWN_THREAD )         \
 {                  \
    CFR_DBG_ERROR("Unknown Thread Id!!");        \
    return CMBS_RC_ERROR_GENERAL;          \
 }                  \
 else                 \
 {                  \
  u8_Buffer = (u8 *)&(s_Buffer[u8_ThreadIdx].NAME);       \
 }                                                           \
    memset(u8_Buffer,0,sizeof(s_Buffer[u8_ThreadIdx].NAME));
#endif

/*****************************************************************************
 * Information Elements List functions
 *
 * Information element structure:
 *
 * | 2 bytes  | 2 bytes           | IE Payload Length bytes |
 * |----------|-------------------|-------------------------|
 * | IE Type  | IE Payload Length | IE Payload ...          |
 *
 *****************************************************************************/



#if defined ( CMBS_API_TARGET )
U_Buffer s_Buffer;
#else
U_Buffer s_Buffer[CMBS_NUM_OF_HOST_THREADS];
#endif

void* cmbs_api_ie_GetList(void)
{
    return cfr_ie_ListNew(NULL, 0);
}

E_CMBS_RC cmbs_api_ie_GetFirst(void *pv_RefIEList, void **ppv_RefIE, u16 *pu16_IEType)
{
    // set pointer to first IE element
    *ppv_RefIE = cfr_ie_ItemFirstGet((PST_CFR_IE_LIST)pv_RefIEList);

    // IE type stored in first 2 bytes
    cfr_ie_dser_u16((u8 *)*ppv_RefIE + CFR_IE_TYPE_POS, pu16_IEType);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_GetNext(void *pv_RefIEList, void **ppv_RefIE, u16 *pu16_IEType)
{
    // get next IE element
    *ppv_RefIE = cfr_ie_ItemNextGet((PST_CFR_IE_LIST)pv_RefIEList);

    // IE type stored in first 2 bytes
    if (*ppv_RefIE)
    {
        cfr_ie_dser_u16((u8 *)*ppv_RefIE + CFR_IE_TYPE_POS, pu16_IEType);
    }
    else
    {
        pu16_IEType = 0;
    }

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_GetIE(void *pv_RefIEList, void **ppv_RefIE, u16 pu16_IEType)
{
    void *pv_IE;
    u16      u16_IE;

    cmbs_api_ie_GetFirst(pv_RefIEList, &pv_IE, &u16_IE);

    while (pv_IE != NULL)
    {
        if (u16_IE == pu16_IEType)
        {
            *ppv_RefIE = pv_IE;
            return CMBS_RC_OK;
        }
        cmbs_api_ie_GetNext(pv_RefIEList, &pv_IE, &u16_IE);
    }

    return CMBS_RC_ERROR_PARAMETER;
}

/*****************************************************************************
 * Information Elements ADD / GET functions
 *****************************************************************************/

E_CMBS_RC cmbs_api_ie_ByteValueAdd(void *pv_RefIEList, u8 u8_Value, E_CMBS_IE_TYPE e_IETYPE)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferByteValueAdd);

    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, e_IETYPE);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, sizeof(u8));
    u8_Buffer[u16_Size++] = u8_Value;

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ByteValueGet(void *pv_RefIE, u8 *pu8_Value, E_CMBS_IE_TYPE e_IETYPE)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;

    CHECK_IE_TYPE(pu8_Buffer, e_IETYPE);

    *pu8_Value = pu8_Buffer[CFR_IE_HEADER_SIZE];

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ShortValueAdd(void *pv_RefIEList, u16 u16_Value, E_CMBS_IE_TYPE e_IETYPE)
{
    u16  u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferShortValueAdd);

    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, e_IETYPE);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, sizeof(u16));
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE, u16_Value);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ShortValueGet(void *pv_RefIE, u16 *pu16_Value, E_CMBS_IE_TYPE e_IETYPE)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;

    CHECK_IE_TYPE(pu8_Buffer, e_IETYPE);

    cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE, pu16_Value);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_u32ValueAdd(void *pv_RefIEList, u32 u32_Value, E_CMBS_IE_TYPE e_IEType)
{
    u16  u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferByteU32ValueAdd);

    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, e_IEType);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, sizeof(u32));
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE, u32_Value);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_u32ValueGet(void *pv_RefIE, u32 *pu32_Value, E_CMBS_IE_TYPE e_IEType)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;

    CHECK_IE_TYPE(pu8_Buffer, e_IEType);

    cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE, pu32_Value);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_IntValueAdd(void *pv_RefIEList, u32 u32_Value)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIEList, u32_Value, CMBS_IE_INTEGER_VALUE);
}

E_CMBS_RC cmbs_api_ie_IntValueGet(void *pv_RefIE, u32 *pu32_Value)
{
    return cmbs_api_ie_u32ValueGet(pv_RefIE, pu32_Value, CMBS_IE_INTEGER_VALUE);
}

E_CMBS_RC cmbs_api_ie_LineIdAdd(void *pv_RefIEList, u8 u8_LineId)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, u8_LineId, CMBS_IE_LINE_ID);
}

E_CMBS_RC cmbs_api_ie_LineIdGet(void *pv_RefIE, u8 *pu8_LineId)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, pu8_LineId, CMBS_IE_LINE_ID);
}

E_CMBS_RC cmbs_api_ie_MelodyAdd(void *pv_RefIEList, u8 u8_Melody)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, u8_Melody, CMBS_IE_MELODY);
}

E_CMBS_RC cmbs_api_ie_MelodyGet(void *pv_RefIE, u8 *pu8_Melody)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, pu8_Melody, CMBS_IE_MELODY);
}

E_CMBS_RC cmbs_api_ie_CallInstanceAdd(void *pv_RefIEList, u32 u32_CallInstance)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIEList, u32_CallInstance, CMBS_IE_CALLINSTANCE);
}

E_CMBS_RC cmbs_api_ie_CallInstanceGet(void *pv_RefIE, u32 *pu32_CallInstance)
{
    return cmbs_api_ie_u32ValueGet(pv_RefIE, pu32_CallInstance, CMBS_IE_CALLINSTANCE);
}

E_CMBS_RC cmbs_api_ie_RequestIdAdd(void *pv_RefIE, u16 pu16_RequestId)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, pu16_RequestId, CMBS_IE_REQUEST_ID);
}

E_CMBS_RC cmbs_api_ie_RequestIdGet(void *pv_RefIE, u16 *pu16_RequestId)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_RequestId, CMBS_IE_REQUEST_ID);
}

E_CMBS_RC cmbs_api_ie_HandsetsAdd(void *pv_RefIE, u16  u16_Handsets)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_Handsets, CMBS_IE_HANDSETS);
}

E_CMBS_RC cmbs_api_ie_HandsetsGet(void *pv_RefIE, u16 *pu16_Handsets)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_Handsets, CMBS_IE_HANDSETS);
}

E_CMBS_RC cmbs_api_ie_HsNumberAdd(void *pv_RefIEList, u8 u8_HsNumber)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, u8_HsNumber, CMBS_IE_HS_NUMBER);
}

E_CMBS_RC cmbs_api_ie_HsNumberGet(void *pv_RefIE, u8 *pu8_HsNumber)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, pu8_HsNumber, CMBS_IE_HS_NUMBER);
}

E_CMBS_RC cmbs_api_ie_GpioAdd(void *pv_RefIEList, u16 u16_Gpio)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIEList, u16_Gpio, CMBS_IE_SHORT_VALUE);
}

E_CMBS_RC cmbs_api_ie_GpioGet(void *pv_RefIE, u16 *pu16_Gpio)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_Gpio, CMBS_IE_SHORT_VALUE);
}

E_CMBS_RC cmbs_api_ie_CallTransferReqAdd(void *pv_RefIEList, PST_IE_CALLTRANSFERREQ pst_CallTrf)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferReqAdd);

    // put data
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallTrf->u32_CallInstanceFrom);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallTrf->u32_CallInstanceTo);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallTrf->u8_TermInstance);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLTRANSFERREQ);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallTransferReqGet(void *pv_RefIE, PST_IE_CALLTRANSFERREQ pst_CallTrf)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_CallTrf, 0, sizeof(ST_IE_CALLTRANSFERREQ));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLTRANSFERREQ);

    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallTrf->u32_CallInstanceFrom);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallTrf->u32_CallInstanceTo);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallTrf->u8_TermInstance);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_InternalCallTransferReqAdd(void *pv_RefIEList, PST_IE_INTERNAL_TRANSFER pst_InternalTransfer)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferInternalReqAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_InternalTransfer->u16_HsNum);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_InternalTransfer->eTransferToType);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_InternalTransfer->u32_TransferToID);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_InternalTransfer->eTransferFromType);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_InternalTransfer->u32_TransferFromID);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_INTERNAL_TRANSFER);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_InternalCallTransferReqGet(void *pv_RefIE, PST_IE_INTERNAL_TRANSFER pst_InternalTransfer)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_TransferToType = 0;
    u8  u8_TransferFromType = 0;

    memset(pst_InternalTransfer, 0, sizeof(ST_IE_INTERNAL_TRANSFER));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_INTERNAL_TRANSFER);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_InternalTransfer->u16_HsNum);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_TransferToType);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_InternalTransfer->u32_TransferToID);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_TransferFromType);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_InternalTransfer->u32_TransferFromID);

    pst_InternalTransfer->eTransferToType = (E_CMBS_CALL_LEG_TYPE)u8_TransferToType;
    pst_InternalTransfer->eTransferFromType = (E_CMBS_CALL_LEG_TYPE)u8_TransferFromType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallerPartyAdd(void *pv_RefIEList, ST_IE_CALLERPARTY *pst_CallerParty)
{
    u16 u16_Size = 0;
    u8  u8_DataLength = MIN(pst_CallerParty->u8_AddressLen, CMBS_CALLER_NUM_MAX_LEN);

    CMBS_IE_GET_BUFFER(u8_BufferCallerPartyAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallerParty->u8_AddressProperties);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallerParty->u8_AddressPresentation);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallerParty->pu8_Address, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLERPARTY);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallerPartyGet(void *pv_RefIE, ST_IE_CALLERPARTY *pst_CallerParty)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_CallerParty, 0, sizeof(ST_IE_CALLERPARTY));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLERPARTY);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallerParty->u8_AddressProperties);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallerParty->u8_AddressPresentation);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallerParty->u8_AddressLen);
    pst_CallerParty->pu8_Address = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CalledPartyAdd(void *pv_RefIEList, ST_IE_CALLEDPARTY *pst_CalledParty)
{
    u16 u16_Size = 0;
    u8  u8_DataLength = MIN(pst_CalledParty->u8_AddressLen, CMBS_CALLER_NUM_MAX_LEN);

    CMBS_IE_GET_BUFFER(u8_BufferCalledPartyAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CalledParty->u8_AddressProperties);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CalledParty->u8_AddressPresentation);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CalledParty->pu8_Address, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLEDPARTY);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CalledPartyGet(void *pv_RefIE, ST_IE_CALLEDPARTY *pst_CalledParty)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_CalledParty, 0, sizeof(ST_IE_CALLEDPARTY));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLEDPARTY);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CalledParty->u8_AddressProperties);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CalledParty->u8_AddressPresentation);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CalledParty->u8_AddressLen);
    pst_CalledParty->pu8_Address = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallerNameAdd(void *pv_RefIEList, ST_IE_CALLERNAME *pst_CallerName)
{
    u16 u16_Size = 0;

    u8  u8_DataLength = MIN(pst_CallerName->u8_DataLen, CMBS_CALLER_NAME_MAX_LEN);

    CMBS_IE_GET_BUFFER(u8_BufferCallerNameAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallerName->pu8_Name, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLERNAME);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallerNameGet(void *pv_RefIE, ST_IE_CALLERNAME *pst_CallerName)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_CallerName, 0, sizeof(ST_IE_CALLERNAME));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLERNAME);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallerName->u8_DataLen);
    pst_CallerName->pu8_Name = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    return CMBS_RC_OK;
}


E_CMBS_RC cmbs_api_ie_CalledNameAdd(void *pv_RefIEList, ST_IE_CALLEDNAME *pst_CalledName)
{
    u16 u16_Size;
    u8  u8_NameLength = pst_CalledName->u8_DataLen;
    u8  u8_FirstNameLength = pst_CalledName->u8_DataLenFirst;

    CMBS_IE_GET_BUFFER(u8_BufferCalledNameAdd);

    if (u8_NameLength > CMBS_CALLED_NAME_MAX_LEN)
    {
        u8_NameLength = CMBS_CALLED_NAME_MAX_LEN;
    }

    if (u8_FirstNameLength > CMBS_CALLED_NAME_MAX_LEN)
    {
        u8_FirstNameLength = CMBS_CALLED_NAME_MAX_LEN;
    }

    u8_Buffer[CFR_IE_TYPE_POS] = (u8)CMBS_IE_CALLEDNAME;
    /* Screening indicator + NameLength + Name + FirstNameLength + FristName */
    u8_Buffer[CFR_IE_SIZE_POS] = 3 * sizeof(u8) + u8_NameLength + u8_FirstNameLength;

    u8_Buffer[CFR_IE_HEADER_SIZE] = pst_CalledName->u8_AlphabetAndScreening;
    u8_Buffer[CFR_IE_HEADER_SIZE + 1] = u8_NameLength;
    cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + 2 * sizeof(u8), pst_CalledName->pu8_Name, u8_NameLength);

    u8_Buffer[CFR_IE_HEADER_SIZE + 2 * sizeof(u8) + u8_NameLength] = u8_FirstNameLength;
    cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u8_NameLength + 3 * sizeof(u8), pst_CalledName->pu8_FirstName, u8_FirstNameLength);

    u16_Size = u8_Buffer[CFR_IE_SIZE_POS] + CFR_IE_HEADER_SIZE;

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, u16_Size);

    return CMBS_RC_OK;
}


E_CMBS_RC cmbs_api_ie_CalledNameGet(void *pv_RefIE, ST_IE_CALLEDNAME *pst_CalledName)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;

    if (*pu8_Buffer == CMBS_IE_CALLEDNAME)
    {
        pst_CalledName->u8_AlphabetAndScreening = pu8_Buffer[CFR_IE_HEADER_SIZE];

        pst_CalledName->u8_DataLen      = pu8_Buffer[CFR_IE_HEADER_SIZE +  sizeof(u8)];
        pst_CalledName->pu8_Name        = pu8_Buffer + CFR_IE_HEADER_SIZE + 2 * sizeof(u8);

        pst_CalledName->u8_DataLenFirst = pu8_Buffer[CFR_IE_HEADER_SIZE + pst_CalledName->u8_DataLen + 2 * sizeof(u8)];
        pst_CalledName->pu8_FirstName   = pu8_Buffer + CFR_IE_HEADER_SIZE + pst_CalledName->u8_DataLen + 3 * sizeof(u8);

        return CMBS_RC_OK;
    }

    return CMBS_RC_ERROR_PARAMETER;
}


E_CMBS_RC cmbs_api_ie_CallProgressAdd(void *pv_RefIEList, ST_IE_CALLPROGRESS *pst_CallProgress)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_CallProgress->e_Progress, CMBS_IE_CALLPROGRESS);
}

E_CMBS_RC cmbs_api_ie_CallProgressGet(void *pv_RefIE, ST_IE_CALLPROGRESS *pst_CallProgress)
{
    u8 u8_Progress = 0;

    memset(pst_CallProgress, 0, sizeof(ST_IE_CALLPROGRESS));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_Progress, CMBS_IE_CALLPROGRESS);

    pst_CallProgress->e_Progress = (E_CMBS_CALL_PROGRESS)u8_Progress;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallInfoAdd(void *pv_RefIEList, ST_IE_CALLINFO *pst_CallInfo)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferCallInfoAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallInfo->e_Type);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallInfo->u8_DataLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallInfo->pu8_Info, pst_CallInfo->u8_DataLen);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLINFO);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}


E_CMBS_RC cmbs_api_ie_CallInfoGet(void *pv_RefIE, ST_IE_CALLINFO *pst_CallInfo)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Type;

    memset(pst_CallInfo, 0, sizeof(ST_IE_CALLINFO));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLINFO);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Type);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallInfo->u8_DataLen);
    pst_CallInfo->pu8_Info = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    pst_CallInfo->e_Type = (E_CMBS_CALL_INFO_TYPE)u8_Type;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DisplayStringAdd(void *pv_RefIEList, ST_IE_DISPLAY_STRING *pst_DisplayString)
{
    u16 u16_Size = 0;
    u8  u8_DataLength = MIN(pst_DisplayString->u8_DataLen, CMBS_DISPLAY_STRING_LENGTH);

    CMBS_IE_GET_BUFFER(u8_BufferDisplayStringAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DisplayString->pu8_Info, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_DISPLAY_STRING);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DisplayStringGet(void *pv_RefIE, ST_IE_DISPLAY_STRING *pst_DisplayString)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_DisplayString, 0, sizeof(ST_IE_DISPLAY_STRING));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_DISPLAY_STRING);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DisplayString->u8_DataLen);
    pst_DisplayString->pu8_Info = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallReleaseReasonAdd(void *pv_RefIEList, ST_IE_RELEASE_REASON *pst_RelReason)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferReleaseReasonAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RelReason->e_Reason);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RelReason->u32_ExtReason);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLRELEASE_REASON);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallReleaseReasonGet(void *pv_RefIE, ST_IE_RELEASE_REASON *pst_RelReason)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Reason = 0;

    memset(pst_RelReason, 0, sizeof(ST_IE_RELEASE_REASON));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLRELEASE_REASON);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Reason);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RelReason->u32_ExtReason);

    pst_RelReason->e_Reason = (E_CMBS_REL_REASON)u8_Reason;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallStateAdd(void *pv_RefIEList, ST_IE_CALL_STATE *pst_CallState)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferCallStateAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallState->u8_ActCallID);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallState->e_CallType);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallState->e_CallStatus);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallState->u16_HandsetsMask);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_CallState->u8_LinesMask);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALLSTATE);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallStateGet(void *pv_RefIE, ST_IE_CALL_STATE *pst_CallState)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_CallType = 0;
    u8  u8_CallStatus = 0;

    memset(pst_CallState, 0, sizeof(ST_IE_CALL_STATE));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALLSTATE);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallState->u8_ActCallID);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_CallType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_CallStatus);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallState->u16_HandsetsMask);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_CallState->u8_LinesMask);

    pst_CallState->e_CallType = (E_CMBS_CALL_STATE_TYPE)u8_CallType;
    pst_CallState->e_CallStatus = (E_CMBS_CALL_STATE_STATUS)u8_CallStatus;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_MediaChannelAdd(void *pv_RefIEList, ST_IE_MEDIA_CHANNEL *pst_MediaChannel)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferChannelAdd);

    // put data
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaChannel->u32_ChannelID);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaChannel->u32_ChannelParameter);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaChannel->e_Type);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_MEDIACHANNEL);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}


E_CMBS_RC cmbs_api_ie_MediaChannelGet(void *pv_RefIE, ST_IE_MEDIA_CHANNEL *pst_MediaChannel)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Type = 0;

    memset(pst_MediaChannel, 0, sizeof(ST_IE_MEDIA_CHANNEL));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_MEDIACHANNEL);

    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_MediaChannel->u32_ChannelID);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_MediaChannel->u32_ChannelParameter);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Type);

    pst_MediaChannel->e_Type = (E_CMBS_MEDIA_TYPE)u8_Type;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_MediaICAdd(void *pv_RefIEList, ST_IE_MEDIA_INTERNAL_CONNECT *pst_MediaIC)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferICAdd);

    // put data
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaIC->u32_ChannelID);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaIC->u32_NodeId);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaIC->e_Type);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_MEDIA_INTERNAL_CONNECT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_MediaICGet(void *pv_RefIE, ST_IE_MEDIA_INTERNAL_CONNECT *pst_MediaIC)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Type = 0;

    memset(pst_MediaIC, 0, sizeof(ST_IE_MEDIA_INTERNAL_CONNECT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_MEDIA_INTERNAL_CONNECT);

    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_MediaIC->u32_ChannelID);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_MediaIC->u32_NodeId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Type);

    pst_MediaIC->e_Type = (E_CMBS_MEDIA_IC_TYPE)u8_Type;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_MediaDescAdd(void *pv_RefIEList, ST_IE_MEDIA_DESCRIPTOR *pst_MediaDesc)
{
    u16 u16_Size = 0;
    u8  u8_DataLength = MIN(pst_MediaDesc->u8_CodecsLength, CMBS_AUDIO_CODEC_MAX);

    CMBS_IE_GET_BUFFER(u8_BufferDescAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaDesc->e_Codec);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaDesc->pu8_CodecsList, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_MEDIADESCRIPTOR);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_MediaDescGet(void *pv_RefIE, ST_IE_MEDIA_DESCRIPTOR *pst_MediaDesc)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Codec = 0;

    memset(pst_MediaDesc, 0, sizeof(ST_IE_MEDIA_DESCRIPTOR));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_MEDIADESCRIPTOR);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Codec);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_MediaDesc->u8_CodecsLength);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_MediaDesc->pu8_CodecsList, MIN(pst_MediaDesc->u8_CodecsLength, CMBS_AUDIO_CODEC_MAX));

    pst_MediaDesc->e_Codec = (E_CMBS_AUDIO_CODEC)u8_Codec;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ToneAdd(void *pv_RefIEList, ST_IE_TONE *pst_Tone)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_Tone->e_Tone, CMBS_IE_TONE);
}

E_CMBS_RC cmbs_api_ie_ToneGet(void *pv_RefIE, ST_IE_TONE *pst_Tone)
{
    u8 u8_Tone = 0;

    memset(pst_Tone, 0, sizeof(ST_IE_TONE));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_Tone, CMBS_IE_TONE);

    pst_Tone->e_Tone = (E_CMBS_TONE)u8_Tone;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_TimeAdd(void *pv_RefIEList, ST_IE_TIMEOFDAY *pst_TimeOfDay)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIEList, pst_TimeOfDay->u32_Timestamp, CMBS_IE_TIMEOFDAY);
}

E_CMBS_RC cmbs_api_ie_TimeGet(void *pv_RefIE, ST_IE_TIMEOFDAY *pst_TimeOfDay)
{
    memset(pst_TimeOfDay, 0, sizeof(ST_IE_TIMEOFDAY));
    return cmbs_api_ie_u32ValueGet(pv_RefIE, &pst_TimeOfDay->u32_Timestamp, CMBS_IE_TIMEOFDAY);
}

E_CMBS_RC cmbs_api_ie_HandsetInfoAdd(void *pv_RefIEList, ST_IE_HANDSETINFO *pst_HandsetInfo)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferInfoAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u8_Hs);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u8_State);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->e_Type);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u32_ModelNr);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u8_IPEI, CMBS_IPEI_SIZE);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u8_EMC, CMBS_EMC_SIZE);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HANDSETINFO);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_HandsetInfoGet(void *pv_RefIE, ST_IE_HANDSETINFO *pst_HandsetInfo)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Type = 0;

    memset(pst_HandsetInfo, 0, sizeof(ST_IE_HANDSETINFO));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HANDSETINFO);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HandsetInfo->u8_Hs);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HandsetInfo->u8_State);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Type);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HandsetInfo->u32_ModelNr);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u8_IPEI, CMBS_IPEI_SIZE);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HandsetInfo->u8_EMC, CMBS_EMC_SIZE);

    pst_HandsetInfo->e_Type = (E_CMBS_HS_TYPE)u8_Type;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ParameterAdd(void *pv_RefIEList, ST_IE_PARAMETER *pst_Param)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferParameterAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->e_Param);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->e_ParamType);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u16_DataLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->pu8_Data, pst_Param->u16_DataLen);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_PARAMETER);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ParameterGet(void *pv_RefIE, ST_IE_PARAMETER *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Param = 0;
    u8  u8_ParamType = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_PARAMETER));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_PARAMETER);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Param);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ParamType);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u16_DataLen);
    pst_Parameter->pu8_Data = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    pst_Parameter->e_Param = (E_CMBS_PARAM)u8_Param;
    pst_Parameter->e_ParamType = (E_CMBS_PARAM_TYPE)u8_ParamType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SubscribedHSListAdd(void *pv_RefIEList, ST_IE_SUBSCRIBED_HS_LIST *pst_SubscribedHsList)
{
    u16 u16_Size = 0;
    u8  u8_Index = 0;

    CMBS_IE_GET_BUFFER(u8_BufferHsListAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u16_HsID);
    if (pst_SubscribedHsList->u16_NameLength > CMBS_HS_NAME_MAX_LENGTH)
    {
        pst_SubscribedHsList->u16_NameLength = CMBS_HS_NAME_MAX_LENGTH;
    }
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u16_NameLength);
    for (u8_Index = 0; u8_Index < pst_SubscribedHsList->u16_NameLength; u8_Index++) u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u8_HsName[u8_Index]);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u16_HsCapabilities);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u8_FXS_ExNumLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u8_FXS_ExNum, pst_SubscribedHsList->u8_FXS_ExNumLen);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u8_Spare);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_SUBSCRIBED_HS_LIST);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SubscribedHSListGet(void *pv_RefIE, ST_IE_SUBSCRIBED_HS_LIST *pst_SubscribedHsList)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Index = 0;

    memset(pst_SubscribedHsList, 0, sizeof(ST_IE_SUBSCRIBED_HS_LIST));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_SUBSCRIBED_HS_LIST);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SubscribedHsList->u16_HsID);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SubscribedHsList->u16_NameLength);
    for (u8_Index = 0; u8_Index < pst_SubscribedHsList->u16_NameLength; u8_Index++) u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SubscribedHsList->u8_HsName[u8_Index]);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SubscribedHsList->u16_HsCapabilities);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SubscribedHsList->u8_FXS_ExNumLen);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SubscribedHsList->u8_FXS_ExNum, CMBS_FXS_NUMBER_MAX_LENGTH);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SubscribedHsList->u8_Spare);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LineSettingsListAdd(void *pv_RefIEList, ST_IE_LINE_SETTINGS_LIST *pst_LineSettingsList)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferSettingsAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LineSettingsList->u8_Line_Id);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LineSettingsList->u8_Call_Intrusion);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LineSettingsList->u8_Multiple_Calls);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LineSettingsList->u16_Attached_HS);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LineSettingsList->e_LineType);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_LINE_SETTINGS_LIST);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}


E_CMBS_RC cmbs_api_ie_LineSettingsListGet(void *pv_RefIE, ST_IE_LINE_SETTINGS_LIST *pst_LineSettingsList)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_LineType = 0;

    memset(pst_LineSettingsList, 0, sizeof(ST_IE_LINE_SETTINGS_LIST));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_LINE_SETTINGS_LIST);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LineSettingsList->u8_Line_Id);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LineSettingsList->u8_Call_Intrusion);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LineSettingsList->u8_Multiple_Calls);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LineSettingsList->u16_Attached_HS);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_LineType);

    pst_LineSettingsList->e_LineType = (E_CMBS_LINE_TYPE)u8_LineType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_FwVersionAdd(void *pv_RefIEList, ST_IE_FW_VERSION *pst_FwVersion)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferVersionAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_FwVersion->e_SwModule);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_FwVersion->u16_FwVersion);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_FW_VERSION);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_FwVersionGet(void *pv_RefIE, ST_IE_FW_VERSION *pst_FwVersion)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_SwModule = 0;

    memset(pst_FwVersion, 0, sizeof(ST_IE_FW_VERSION));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_FW_VERSION);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_SwModule);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_FwVersion->u16_FwVersion);

    pst_FwVersion->e_SwModule = (E_CMBS_MODULE)u8_SwModule;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ResponseAdd(void *pv_RefIE, ST_IE_RESPONSE *pst_Response)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, pst_Response->e_Response, CMBS_IE_RESPONSE);
}

E_CMBS_RC cmbs_api_ie_ResponseGet(void *pv_RefIE, ST_IE_RESPONSE *pst_Response)
{
    u8 u8_Response = 0;

    memset(pst_Response, 0, sizeof(ST_IE_RESPONSE));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_Response, CMBS_IE_RESPONSE);

    pst_Response->e_Response = (E_CMBS_RESPONSE)u8_Response;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SysStatusAdd(void *pv_RefIEList, ST_IE_SYS_STATUS *pst_SysStatus)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_SysStatus->e_ModuleStatus, CMBS_IE_STATUS);
}

E_CMBS_RC cmbs_api_ie_SysStatusGet(void *pv_RefIE, ST_IE_SYS_STATUS *pst_SysStatus)
{
    u8 u8_ModuleStatus = 0;

    memset(pst_SysStatus, 0, sizeof(ST_IE_SYS_STATUS));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_ModuleStatus, CMBS_IE_STATUS);

    pst_SysStatus->e_ModuleStatus = (E_CMBS_MODULE_STATUS)u8_ModuleStatus;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SysLogAdd(void *pv_RefIEList, ST_IE_SYS_LOG *pst_SysLog)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferLogAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SysLog->u8_DataLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SysLog->u8_Data, pst_SysLog->u8_DataLen);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_SYS_LOG);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SysLogGet(void *pv_RefIE, ST_IE_SYS_LOG *pst_SysLog)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_SysLog, 0, sizeof(ST_IE_SYS_LOG));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_SYS_LOG);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SysLog->u8_DataLen);
    pst_SysLog->u8_DataLen = MIN(pst_SysLog->u8_DataLen, CMBS_SYS_LOG_MAX_SIZE);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SysLog->u8_Data, pst_SysLog->u8_DataLen);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ParameterAreaAdd(void *pv_RefIEList, ST_IE_PARAMETER_AREA *pst_ParamArea)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAreaAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_ParamArea->e_AreaType);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_ParamArea->u32_Offset);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_ParamArea->u16_DataLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_ParamArea->pu8_Data, pst_ParamArea->u16_DataLen);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_PARAMETER_AREA);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ParameterAreaGet(void *pv_RefIE, ST_IE_PARAMETER_AREA *pst_ParameterArea)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_AreaType = 0;

    memset(pst_ParameterArea, 0, sizeof(ST_IE_PARAMETER_AREA));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_PARAMETER_AREA);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AreaType);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_ParameterArea->u32_Offset);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_ParameterArea->u16_DataLen);
    pst_ParameterArea->pu8_Data = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    pst_ParameterArea->e_AreaType = (E_CMBS_PARAM_AREA_TYPE)u8_AreaType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_GenEventAdd(void *pv_RefIEList, ST_IE_GEN_EVENT *pst_GenEvent)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferGenEventAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_GenEvent->u8_SubType);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_GenEvent->u16_MultiPlicity);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_GenEvent->u8_LineId);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_GEN_EVENT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_GenEventGet(void *pv_RefIE, ST_IE_GEN_EVENT *pst_GenEvent)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_GenEvent, 0, sizeof(ST_IE_GEN_EVENT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_GEN_EVENT);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_GenEvent->u8_SubType);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_GenEvent->u16_MultiPlicity);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_GenEvent->u8_LineId);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_PropEventAdd(void *pv_RefIEList, ST_IE_PROP_EVENT *pst_PropEvent)
{
    u16 u16_Size = 0;
    u8 u8_DataLength = MIN(pst_PropEvent->u8_DataLen, CMBS_PROP_EVENT_LENGTH);

    CMBS_IE_GET_BUFFER(u8_BufferPropEventAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_PropEvent->u16_PropEvent);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_PropEvent->u8_Data, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_PROP_EVENT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_PropEventGet(void *pv_RefIE, ST_IE_PROP_EVENT *pst_PropEvent)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_PropEvent, 0, sizeof(ST_IE_PROP_EVENT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_PROP_EVENT);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_PropEvent->u16_PropEvent);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_PropEvent->u8_DataLen);
    pst_PropEvent->u8_DataLen = MIN(pst_PropEvent->u8_DataLen, CMBS_PROP_EVENT_LENGTH);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_PropEvent->u8_Data, pst_PropEvent->u8_DataLen);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DateTimeAdd(void *pv_RefIEList, ST_IE_DATETIME *pst_DateTime)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferDateAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->e_Coding);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->e_Interpretation);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Year);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Month);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Day);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Hours);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Mins);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Secs);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DateTime->u8_Zone);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_DATETIME);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DateTimeGet(void *pv_RefIE, ST_IE_DATETIME *pst_DateTime)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Coding = 0;
    u8  u8_Interpretation = 0;

    memset(pst_DateTime, 0, sizeof(ST_IE_DATETIME));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_DATETIME);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Coding);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Interpretation);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Year);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Month);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Day);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Hours);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Mins);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Secs);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DateTime->u8_Zone);

    pst_DateTime->e_Coding = (E_CMBS_DATETIME_CODING)u8_Coding;
    pst_DateTime->e_Interpretation = (E_CMBS_DATETIME_INTERPRETATION)u8_Interpretation;


    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DataSessionIdAdd(void *pv_RefIE, u16 pu16_DataSessionId)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, pu16_DataSessionId, CMBS_IE_DATA_SESSION_ID);
}

E_CMBS_RC cmbs_api_ie_DataSessionIdGet(void *pv_RefIE, u16 *pu16_DataSessionId)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_DataSessionId, CMBS_IE_DATA_SESSION_ID);
}

E_CMBS_RC cmbs_api_ie_DataSessionTypeAdd(void *pv_RefIEList, ST_IE_DATA_SESSION_TYPE *pst_DataSessionType)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferSessionTypeAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DataSessionType->e_ChannelType);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DataSessionType->e_ServiceType);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_DATA_SESSION_TYPE);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DataSessionTypeGet(void *pv_RefIE, ST_IE_DATA_SESSION_TYPE *pst_DataSessionType)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_ChannelType = 0;
    u8  u8_ServiceType = 0;

    memset(pst_DataSessionType, 0, sizeof(ST_IE_DATA_SESSION_TYPE));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_DATA_SESSION_TYPE);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ChannelType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ServiceType);

    pst_DataSessionType->e_ChannelType = (E_CMBS_DATA_CHANNEL_TYPE)u8_ChannelType;
    pst_DataSessionType->e_ServiceType = (E_CMBS_DATA_SERVICE_TYPE)u8_ServiceType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DataAdd(void *pv_RefIEList, ST_IE_DATA *pst_Data)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferDataAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Data->u16_DataLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Data->pu8_Data, pst_Data->u16_DataLen);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_DATA);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DataGet(void *pv_RefIE, ST_IE_DATA *pst_Data)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_Data, 0, sizeof(ST_IE_DATA));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_DATA);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Data->u16_DataLen);
    pst_Data->pu8_Data = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LASessionIdAdd(void *pv_RefIE, u16 pu16_LASessionId)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, pu16_LASessionId, CMBS_IE_LA_SESSION_ID);
}

E_CMBS_RC cmbs_api_ie_LASessionIdGet(void *pv_RefIE, u16 *pu16_LASessionId)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_LASessionId, CMBS_IE_LA_SESSION_ID);
}

E_CMBS_RC cmbs_api_ie_LAListIdAdd(void *pv_RefIE, u16 u16_LAListId)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_LAListId, CMBS_IE_LA_LIST_ID);
}

E_CMBS_RC cmbs_api_ie_LAListIdGet(void *pv_RefIE, u16 *pu16_LAListId)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_LAListId, CMBS_IE_LA_LIST_ID);
}

E_CMBS_RC cmbs_api_ie_LAFieldsAdd(void *pv_RefIEList, ST_IE_LA_FIELDS *pst_LAFields, E_CMBS_IE_TYPE e_IEType)
{
    u16 u16_Size = 0;
    u8  u8_Count = 0;

    CMBS_IE_GET_BUFFER(u8_BufferFieldsAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LAFields->u16_Length);
    for (u8_Count = 0; u8_Count < CMBS_LA_MAX_FLD_COUNT; u8_Count++) u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LAFields->pu16_FieldId[u8_Count]);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, e_IEType);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LAFieldsGet(void *pv_RefIE, ST_IE_LA_FIELDS *pst_LAFields)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_Count = 0;

    memset(pst_LAFields, 0, sizeof(ST_IE_LA_FIELDS));
    // CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_DATA);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LAFields->u16_Length);
    for (u8_Count = 0; u8_Count < CMBS_LA_MAX_FLD_COUNT; u8_Count++) u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LAFields->pu16_FieldId[u8_Count]);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LASearchCriteriaAdd(void *pv_RefIEList, ST_IE_LA_SEARCH_CRITERIA *pst_LASearchCriteria)
{
    u16 u16_Size = 0;
    u8  u8_DataLength = MIN(LA_SEARCH_CRITERIA_PATTERN_MAX_SIZE, pst_LASearchCriteria->u8_PatternLength);

    CMBS_IE_GET_BUFFER(u8_BufferCriteriaAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LASearchCriteria->e_MatchingType);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LASearchCriteria->u8_CaseSensitive);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LASearchCriteria->u8_Direction);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LASearchCriteria->u8_MarkEntriesReq);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LASearchCriteria->pu8_Pattern, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_LA_SEARCH_CRITERIA);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LASearchCriteriaGet(void *pv_RefIE, ST_IE_LA_SEARCH_CRITERIA *pst_LASearchCriteria)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_MatchingType = 0;

    memset(pst_LASearchCriteria, 0, sizeof(ST_IE_LA_SEARCH_CRITERIA));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_LA_SEARCH_CRITERIA);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_MatchingType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LASearchCriteria->u8_CaseSensitive);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LASearchCriteria->u8_Direction);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LASearchCriteria->u8_MarkEntriesReq);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_LASearchCriteria->u8_PatternLength);
    pst_LASearchCriteria->u8_PatternLength = MIN(LA_SEARCH_CRITERIA_PATTERN_MAX_SIZE, pst_LASearchCriteria->u8_PatternLength);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_LASearchCriteria->pu8_Pattern, pst_LASearchCriteria->u8_PatternLength);

    pst_LASearchCriteria->e_MatchingType = (E_CMBS_LA_SEARCH_MATCHING)u8_MatchingType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LAEntryIdAdd(void *pv_RefIE, u16 u16_LAEntryId)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_LAEntryId, CMBS_IE_LA_ENTRY_ID);
}

E_CMBS_RC cmbs_api_ie_LAEntryIdGet(void *pv_RefIE, u16 *pu16_LAEntryId)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_LAEntryId, CMBS_IE_LA_ENTRY_ID);
}

E_CMBS_RC cmbs_api_ie_LAEntryIndexAdd(void *pv_RefIE, u16 u16_LAEntryIndex)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_LAEntryIndex, CMBS_IE_LA_ENTRY_INDEX);
}

E_CMBS_RC cmbs_api_ie_LAEntryIndexGet(void *pv_RefIE, u16 *pu16_LAEntryIndex)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_LAEntryIndex, CMBS_IE_LA_ENTRY_INDEX);
}

E_CMBS_RC cmbs_api_ie_LAEntryCountAdd(void *pv_RefIE, u16 u16_LAEntryCount)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_LAEntryCount, CMBS_IE_LA_ENTRY_COUNT);
}

E_CMBS_RC cmbs_api_ie_LAEntryCountGet(void *pv_RefIE, u16 *pu16_LAEntryCount)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_LAEntryCount, CMBS_IE_LA_ENTRY_COUNT);
}

E_CMBS_RC cmbs_api_ie_LAIsLastAdd(void *pv_RefIE, u8 u8_LAIsLast)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_LAIsLast, CMBS_IE_LA_IS_LAST);
}

E_CMBS_RC cmbs_api_ie_LAIsLastGet(void *pv_RefIE, u8 *pu8_LAIsLast)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, pu8_LAIsLast, CMBS_IE_LA_IS_LAST);
}

E_CMBS_RC cmbs_api_ie_LARejectReasonAdd(void *pv_RefIE, u8 u8_LARejectReason)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_LARejectReason, CMBS_IE_LA_REJECT_REASON);
}

E_CMBS_RC cmbs_api_ie_LARejectReasonGet(void *pv_RefIE, u8 *pu8_LARejectReason)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, pu8_LARejectReason, CMBS_IE_LA_REJECT_REASON);
}

E_CMBS_RC cmbs_api_ie_LANrOfEntriesAdd(void *pv_RefIE, u16 u16_LANrOfEntries)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_LANrOfEntries, CMBS_IE_LA_NR_OF_ENTRIES);
}

E_CMBS_RC cmbs_api_ie_LANrOfEntriesGet(void *pv_RefIE, u16 *pu16_LANrOfEntries)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_LANrOfEntries, CMBS_IE_LA_NR_OF_ENTRIES);
}

E_CMBS_RC cmbs_api_ie_ReadDirectionAdd(void *pv_RefIEList, ST_IE_READ_DIRECTION *pst_ReadDirection)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_ReadDirection->e_ReadDirection, CMBS_IE_LA_READ_DIRECTION);
}

E_CMBS_RC cmbs_api_ie_ReadDirectionGet(void *pv_RefIE, ST_IE_READ_DIRECTION *pst_ReadDirection)
{
    u8 u8_ReadDirection = 0;

    memset(pst_ReadDirection, 0, sizeof(ST_IE_READ_DIRECTION));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_ReadDirection, CMBS_IE_LA_READ_DIRECTION);

    pst_ReadDirection->e_ReadDirection = (E_CMBS_READ_DIRECTION)u8_ReadDirection;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_MarkRequestAdd(void *pv_RefIEList, ST_IE_MARK_REQUEST *pst_MarkRequest)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_MarkRequest->e_MarkRequest, CMBS_IE_LA_MARK_REQUEST);
}

E_CMBS_RC cmbs_api_ie_MarkRequestGet(void *pv_RefIE, ST_IE_MARK_REQUEST *pst_MarkRequest)
{
    u8 u8_MarkRequest = 0;

    memset(pst_MarkRequest, 0, sizeof(ST_IE_MARK_REQUEST));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_MarkRequest, CMBS_IE_LA_MARK_REQUEST);

    pst_MarkRequest->e_MarkRequest = (E_CMBS_MARK_REQUEST)u8_MarkRequest;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ATESettingsAdd(void *pv_RefIEList, ST_IE_ATE_SETTINGS *pst_AteSettings)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferATESettingsAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->e_ATESlotType);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->e_ATEType);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_Instance);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_Slot);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_Carrier);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_Ant);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_Pattern);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_NormalPreamble);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_PowerLevel);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_Gpio);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_BERFEREnabled);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_BERFERFrameCount);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_AteSettings->u8_DeviceIdentity);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_ATE_SETTINGS);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ATESettingsGet(void *pv_RefIE, ST_IE_ATE_SETTINGS *pst_AteSettings)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_ATESlotType = 0;
    u8  u8_ATEType = 0;

    memset(pst_AteSettings, 0, sizeof(ST_IE_ATE_SETTINGS));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_ATE_SETTINGS);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ATESlotType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ATEType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_Instance);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_Slot);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_Carrier);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_Ant);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_Pattern);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_NormalPreamble);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_PowerLevel);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_Gpio);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_BERFEREnabled);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_BERFERFrameCount);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_AteSettings->u8_DeviceIdentity);

    pst_AteSettings->e_ATESlotType = (E_CMBS_ATE_SLOT_TYPE)u8_ATESlotType;
    pst_AteSettings->e_ATEType = (E_CMBS_ATE_TYPE)u8_ATEType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LineSubtypeAdd(void *pv_RefIEList, u8 value)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, value, CMBS_IE_LINE_SUBTYPE);
}

E_CMBS_RC cmbs_api_ie_LineSubtypeGet(void *pv_RefIE, u8 *value)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, value, CMBS_IE_LINE_SUBTYPE);
}

E_CMBS_RC cmbs_api_ie_VersionAvailAdd(void *pv_RefIEList, ST_SUOTA_UPGRADE_DETAILS *st_HSVerAvail)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferVersionAvailAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerAvail->u16_delayInMin);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerAvail->u8_URLStoFollow);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerAvail->u8_Spare);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerAvail->u8_UserInteraction);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AVAIL_VERSION_DETAILS);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_VersionAvailGet(void *pv_RefIEList, ST_SUOTA_UPGRADE_DETAILS *pst_HSVerAvail)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIEList;
    u16  u16_Size = 0;

    memset(pst_HSVerAvail, 0, sizeof(ST_SUOTA_UPGRADE_DETAILS));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AVAIL_VERSION_DETAILS);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerAvail->u16_delayInMin);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerAvail->u8_URLStoFollow);
    pst_HSVerAvail->u8_Spare = pu8_Buffer[CFR_IE_HEADER_SIZE + u16_Size++];
    pst_HSVerAvail->u8_UserInteraction = pu8_Buffer[CFR_IE_HEADER_SIZE + u16_Size++];

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_VersionBufferAdd(void *pv_RefIEList, ST_VERSION_BUFFER *pst_SwVersion)
{
    u16 u16_Size = 0;
    u8 u8_DataLength = MIN(CMBS_MAX_VERSION_LENGTH, pst_SwVersion->u8_VerLen);

    CMBS_IE_GET_BUFFER(u8_BufferVersionBufAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SwVersion->pu8_VerBuffer, u8_DataLength);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SwVersion->type);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HS_VERSION_BUFFER);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_VersionBufferGet(void *pv_RefIEList, ST_VERSION_BUFFER *pst_SwVersion)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIEList;
    u16  u16_Size = 0;
    u8 u8_Temp;

    memset(pst_SwVersion, 0, sizeof(ST_VERSION_BUFFER));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HS_VERSION_BUFFER);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_SwVersion->u8_VerLen);
    pst_SwVersion->u8_VerLen = MIN(CMBS_MAX_VERSION_LENGTH, pst_SwVersion->u8_VerLen);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_SwVersion->pu8_VerBuffer, pst_SwVersion->u8_VerLen);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Temp);
    pst_SwVersion->type = u8_Temp;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_VersionIndAdd(void *pv_RefIEList, ST_SUOTA_HS_VERSION_IND *st_HSVerInd)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferVersionIndAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerInd->u16_EMC);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerInd->u8_URLStoFollow);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerInd->u8_FileNumber);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerInd->u8_Flags);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_HSVerInd->u8_Reason);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HS_VERSION_DETAILS);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_VersionIndGet(void *pv_RefIEList, ST_SUOTA_HS_VERSION_IND *pst_HSVerInd)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIEList;
    u16  u16_Size = 0;

    memset(pst_HSVerInd, 0, sizeof(ST_SUOTA_HS_VERSION_IND));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HS_VERSION_DETAILS);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerInd->u16_EMC);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerInd->u8_URLStoFollow);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerInd->u8_FileNumber);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerInd->u8_Flags);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HSVerInd->u8_Reason);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SuSubtypeAdd(void *pv_RefIEList, u8 value)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, value, CMBS_IE_SU_SUBTYPE);
}

E_CMBS_RC cmbs_api_ie_SuSubtypeGet(void *pv_RefIE, u8 *value)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, value, CMBS_IE_SU_SUBTYPE);
}

E_CMBS_RC cmbs_api_ie_UrlAdd(void *pv_RefIEList, ST_URL_BUFFER *pst_Url)
{
    u16 u16_Size = 0;
    u8 u8_DataLength = MIN(CMBS_MAX_URL_SIZE, pst_Url->u8_UrlLen);

    CMBS_IE_GET_BUFFER(u8_BufferURLAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Url->pu8_UrlBuffer, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_URL);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_UrlGet(void *pv_RefIEList, ST_URL_BUFFER *pst_Url)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIEList;
    u16  u16_Size = 0;

    memset(pst_Url, 0, sizeof(ST_URL_BUFFER));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_URL);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Url->u8_UrlLen);
    pst_Url->u8_UrlLen = MIN(CMBS_MAX_URL_SIZE, pst_Url->u8_UrlLen);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Url->pu8_UrlBuffer, pst_Url->u8_UrlLen);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_NumOfUrlsAdd(void *pv_RefIEList, u8 value)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, value, CMBS_IE_NUM_OF_URLS);
}

E_CMBS_RC cmbs_api_ie_NumOfUrlsGet(void *pv_RefIE, u8 *value)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, value, CMBS_IE_NUM_OF_URLS);
}

E_CMBS_RC cmbs_api_ie_RejectReasonAdd(void *pv_RefIEList, u8 value)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, value, CMBS_IE_REJECT_REASON);
}

E_CMBS_RC cmbs_api_ie_RejectReasonGet(void *pv_RefIE, u8 *value)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, value, CMBS_IE_REJECT_REASON);
}

E_CMBS_RC cmbs_api_ie_NBOTACodecAdd(void *pv_RefIEList, PST_IE_NB_CODEC_OTA pst_Codec)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_Codec->e_Codec, CMBS_IE_NB_CODEC_OTA);
}

E_CMBS_RC cmbs_api_ie_NBOTACodecGet(void *pv_RefIE, PST_IE_NB_CODEC_OTA pst_Codec)
{
    u8 u8_Codec = 0;

    memset(pst_Codec, 0, sizeof(ST_IE_NB_CODEC_OTA));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_Codec, CMBS_IE_NB_CODEC_OTA);

    pst_Codec->e_Codec = (E_CMBS_NB_CODEC_OTA_TYPE)u8_Codec;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_TargetListChangeNotifAdd(void *pv_RefIEList, PST_IE_TARGET_LIST_CHANGE_NOTIF pst_Notif)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferChangeNotifAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Notif->e_ListId);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Notif->u32_NumOfEntries);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Notif->u32_EntryId);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Notif->e_ChangeType);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_TARGET_LIST_CHANGE_NOTIF);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_TargetListChangeNotifGet(void *pv_RefIE, PST_IE_TARGET_LIST_CHANGE_NOTIF pst_Notif)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_ListId = 0;
    u8  u8_ChangeType = 0;

    memset(pst_Notif, 0, sizeof(ST_IE_TARGET_LIST_CHANGE_NOTIF));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_TARGET_LIST_CHANGE_NOTIF);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ListId);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Notif->u32_NumOfEntries);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Notif->u32_EntryId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_ChangeType);

    pst_Notif->e_ListId = (E_CMBS_LA_LIST_ID)u8_ListId;
    pst_Notif->e_ChangeType = (E_CMBS_LIST_CHANGE_TYPE)u8_ChangeType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_HwVersionAdd(void *pv_RefIEList, ST_IE_HW_VERSION *pst_HwVersion)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferHwVersionAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HwVersion->u8_HwChip);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HwVersion->u8_HwChipVersion);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HwVersion->u8_HwBoard);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HwVersion->u8_HwComType);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HW_VERSION);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_HwVersionGet(void *pv_RefIE, ST_IE_HW_VERSION *pst_HwVersion)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_HwChip = 0;
    u8  u8_HwChipVersion = 0;
    u8  u8_HwBoard = 0;
    u8  u8_HwComType = 0;

    memset(pst_HwVersion, 0, sizeof(ST_IE_HW_VERSION));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HW_VERSION);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_HwChip);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_HwChipVersion);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_HwBoard);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_HwComType);

    pst_HwVersion->u8_HwChip = (E_CMBS_HW_CHIP)u8_HwChip;
    pst_HwVersion->u8_HwChipVersion = (E_CMBS_HW_CHIP_VERSION)u8_HwChipVersion;
    pst_HwVersion->u8_HwBoard = (E_CMBS_HW_BOARD)u8_HwBoard;
    pst_HwVersion->u8_HwComType = (E_CMBS_HW_COM_TYPE)u8_HwComType;


    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_DectSettingsListAdd(void *pv_RefIEList, ST_IE_DECT_SETTINGS_LIST *pst_DectSettings)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferDectSettingsAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u16_ClockMaster);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u16_BaseReset);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u16_EmissionMode);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u8_IPAddressType);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressValue, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressSubnetMask, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressGateway, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressDNSServer, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_FirmwareVersion, CMBS_MAX_DECT_SETTINGS_VERSION_LENGTH);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u8_FirmwareVersionLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_EepromVersion, CMBS_MAX_DECT_SETTINGS_VERSION_LENGTH);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u8_EepromVersionLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_HardwareVersion, CMBS_MAX_DECT_SETTINGS_VERSION_LENGTH);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u8_HardwareVersionLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_PinCode, CMBS_MAX_PIN_CODE_LENGTH);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->u16_FieldsMask);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_DECT_SETTINGS_LIST);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}


E_CMBS_RC cmbs_api_ie_DectSettingsListGet(void *pv_RefIE, ST_IE_DECT_SETTINGS_LIST *pst_DectSettings)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_DectSettings, 0, sizeof(ST_IE_DECT_SETTINGS_LIST));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_DECT_SETTINGS_LIST);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u16_ClockMaster);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u16_BaseReset);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u16_EmissionMode);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u8_IPAddressType);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressValue, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressSubnetMask, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressGateway, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_IPAddressDNSServer, CMBS_MAX_IP_ADDRESS_LENGTH);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_FirmwareVersion, CMBS_MAX_DECT_SETTINGS_VERSION_LENGTH);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u8_FirmwareVersionLength);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_EepromVersion, CMBS_MAX_DECT_SETTINGS_VERSION_LENGTH);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u8_EepromVersionLength);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_HardwareVersion, CMBS_MAX_DECT_SETTINGS_VERSION_LENGTH);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u8_HardwareVersionLength);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_DectSettings->pu8_PinCode, CMBS_MAX_PIN_CODE_LENGTH);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_DectSettings->u16_FieldsMask);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTPSessionInformationAdd(void *pv_RefIEList, const ST_IE_RTP_SESSION_INFORMATION *pst_RTPSessionInformation)
{
    u16 u16_Size = 0;
    u8  i = 0;

    CMBS_IE_GET_BUFFER(u8_BufferSessionInfoAdd);

    // put data
    for (i = 0; i < CMBS_MAX_NUM_OF_CODECS; ++i)
    {
        u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_RTPSessionInformation->ast_RxCodecList[i].sz_CodecStr, CMBS_MAX_DYNAMIC_CODEC_LEN);
        u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->ast_RxCodecList[i].u8_CodecPt);
    }
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u8_RxCodecEventPt);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_RTPSessionInformation->st_TxCodec.sz_CodecStr, CMBS_MAX_DYNAMIC_CODEC_LEN);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->st_TxCodec.u8_CodecPt);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u8_TxCodecEventPt);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u32_Capabilities);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_RTPSessionInformation->sz_SDesName, CMBS_RTCP_MAX_SDES);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u16_Duration);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u32_CurrentTime);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u32_Timestamp);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u32_SSRC);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u8_JBMinLen);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u8_JBMaxLen);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->e_JBMode);
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u32_DTMFEndPackets);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->e_MediaLoopLevel);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u16_T38LsRedundancy);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u16_T38HsRedundancy);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->u8_T38EcnOn);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPSessionInformation->e_AudioMode);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_RTP_SESSION_INFORMATION);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTPSessionInformationGet(void *pv_RefIE, ST_IE_RTP_SESSION_INFORMATION *pst_RTPSessionInformation)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  i = 0;
    u8  u8_JBMode = 0;
    u8  u8_MediaLoopLevel = 0;
    u8  u8_AudioMode = 0;

    memset(pst_RTPSessionInformation, 0, sizeof(ST_IE_RTP_SESSION_INFORMATION));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_RTP_SESSION_INFORMATION);

    for (i = 0; i < CMBS_MAX_NUM_OF_CODECS; ++i)
    {
        u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_RTPSessionInformation->ast_RxCodecList[i].sz_CodecStr, CMBS_MAX_DYNAMIC_CODEC_LEN);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->ast_RxCodecList[i].u8_CodecPt);
    }
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u8_RxCodecEventPt);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_RTPSessionInformation->st_TxCodec.sz_CodecStr, CMBS_MAX_DYNAMIC_CODEC_LEN);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->st_TxCodec.u8_CodecPt);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u8_TxCodecEventPt);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u32_Capabilities);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_RTPSessionInformation->sz_SDesName, CMBS_RTCP_MAX_SDES);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u16_Duration);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u32_CurrentTime);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u32_Timestamp);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u32_SSRC);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u8_JBMinLen);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u8_JBMaxLen);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_JBMode);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u32_DTMFEndPackets);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_MediaLoopLevel);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u16_T38LsRedundancy);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u16_T38HsRedundancy);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPSessionInformation->u8_T38EcnOn);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AudioMode);

    pst_RTPSessionInformation->e_JBMode = (E_CMBS_RTP_JB_MODE)u8_JBMode;
    pst_RTPSessionInformation->e_MediaLoopLevel = (E_CMBS_VOICE_LOOP)u8_MediaLoopLevel;
    pst_RTPSessionInformation->e_AudioMode = (E_CMBS_AUDIO_MODE)u8_AudioMode;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTCPIntervalAdd(void *pv_RefIEList, u32 u32_RTCPInterval)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIEList, u32_RTCPInterval, CMBS_IE_RTCP_INTERVAL);
}

E_CMBS_RC cmbs_api_ie_RTCPIntervalGet(void *pv_RefIE, u32 *pu32_RTCPInterval)
{
    return cmbs_api_ie_u32ValueGet(pv_RefIE, pu32_RTCPInterval, CMBS_IE_RTCP_INTERVAL);
}

E_CMBS_RC cmbs_api_ie_RTPDTMFEventAdd(void *pv_RefIEList, const ST_IE_RTP_DTMF_EVENT *pst_RTPDTMFEvent)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferDTMFEventAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPDTMFEvent->u8_Event);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPDTMFEvent->u16_Volume);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPDTMFEvent->u16_Duration);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_RTP_DTMF_EVENT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTPDTMFEventGet(void *pv_RefIE, ST_IE_RTP_DTMF_EVENT *pst_RTPDTMFEvent)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_RTPDTMFEvent, 0, sizeof(ST_IE_RTP_DTMF_EVENT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_RTP_DTMF_EVENT);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPDTMFEvent->u8_Event);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPDTMFEvent->u16_Volume);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPDTMFEvent->u16_Duration);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTPDTMFEventInfoAdd(void *pv_RefIEList, const ST_IE_RTP_DTMF_EVENT_INFO *pst_RTPDTMFEventInfo)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferDTMFEventInfoAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPDTMFEventInfo->u16_EventDuration);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RTPDTMFEventInfo->u16_MaxEventDuration);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_RTP_DTMF_EVENT_INFO);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTPDTMFEventInfoGet(void *pv_RefIE, ST_IE_RTP_DTMF_EVENT_INFO *pst_RTPDTMFEventInfo)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_RTPDTMFEventInfo, 0, sizeof(ST_IE_RTP_DTMF_EVENT_INFO));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_RTP_DTMF_EVENT_INFO);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPDTMFEventInfo->u16_EventDuration);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RTPDTMFEventInfo->u16_MaxEventDuration);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RTPFaxToneTypeAdd(void *pv_RefIEList, E_CMBS_FAX_TONE_TYPE e_FaxToneType)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, e_FaxToneType, CMBS_IE_RTP_FAX_TONE_TYPE);
}

E_CMBS_RC cmbs_api_ie_RTPFaxToneTypeGet(void *pv_RefIE, E_CMBS_FAX_TONE_TYPE *pe_FaxToneType)
{
    u8 u8_FaxToneType = 0;

    memset(pe_FaxToneType, 0, sizeof(E_CMBS_FAX_TONE_TYPE));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_FaxToneType, CMBS_IE_RTP_FAX_TONE_TYPE);

    *pe_FaxToneType = (E_CMBS_FAX_TONE_TYPE)u8_FaxToneType;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LAPropCmdAdd(void *pv_RefIE, ST_LA_PROP_CMD *pst_Cmd)
{
    u16 u16_Size = 0;
    u16 u16_DataLength = MIN(CMBS_LA_MAX_CMD_LEN, pst_Cmd->u16_Length);

    CMBS_IE_GET_BUFFER(u8_BufferPropCmdAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Cmd->pu8_data, u16_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_LA_PROP_CMD);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_LAPropCmdGet(void *pv_RefIE, ST_LA_PROP_CMD *pst_Cmd)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_Cmd, 0, sizeof(ST_LA_PROP_CMD));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_LA_PROP_CMD);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Cmd->u16_Length);
    pst_Cmd->u16_Length = MIN(CMBS_LA_MAX_CMD_LEN, pst_Cmd->u16_Length);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Cmd->pu8_data, pst_Cmd->u16_Length);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_BaseNameAdd(void *pv_RefIE, ST_IE_BASE_NAME *pst_BaseName)
{
    u16 u16_Size = 0;
    u8 u8_DataLength = MIN(CMBS_BS_NAME_MAX_LENGTH, pst_BaseName->u8_BaseNameLen);

    CMBS_IE_GET_BUFFER(u8_BufferBaseNameAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_DataLength);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_BaseName->u8_BaseName, u8_DataLength);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_BASE_NAME);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_BaseNameGet(void *pv_RefIE, ST_IE_BASE_NAME *pst_BaseName)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_BaseName, 0, sizeof(ST_IE_BASE_NAME));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_BASE_NAME);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_BaseName->u8_BaseNameLen);
    pst_BaseName->u8_BaseNameLen = MIN(CMBS_BS_NAME_MAX_LENGTH, pst_BaseName->u8_BaseNameLen);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_BaseName->u8_BaseName, pst_BaseName->u8_BaseNameLen);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_RegCloseReasonAdd(void *pv_RefIEList, ST_IE_REG_CLOSE_REASON *pst_Reg_Close_Reason)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_Reg_Close_Reason->e_Reg_Close_Reason, CMBS_IE_REG_CLOSE_REASON);
}

E_CMBS_RC cmbs_api_ie_RegCloseReasonGet(void *pv_RefIE, ST_IE_REG_CLOSE_REASON *st_Reg_Close_Reason)
{
    u8 u8_Value = 0;

    memset(st_Reg_Close_Reason, 0, sizeof(ST_IE_REG_CLOSE_REASON));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_Value, CMBS_IE_REG_CLOSE_REASON);

    st_Reg_Close_Reason->e_Reg_Close_Reason = (E_CMBS_REG_CLOSE_REASON)u8_Value;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_EEPROMVersionAdd(void *pv_RefIEList, ST_IE_EEPROM_VERSION *pst_EEPROMVersion)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIEList, pst_EEPROMVersion->u32_EEPROM_Version, CMBS_IE_EEPROM_VERSION);
}

E_CMBS_RC   cmbs_api_ie_EEPROMVersionGet(void *pv_RefIE, ST_IE_EEPROM_VERSION *pst_EEPROMVersion)
{
    return cmbs_api_ie_u32ValueGet(pv_RefIE, &pst_EEPROMVersion->u32_EEPROM_Version, CMBS_IE_EEPROM_VERSION);
}

E_CMBS_RC cmbs_api_ie_DCRejectReasonAdd(void *pv_RefIE, ST_IE_DC_REJECT_REASON *st_DC_Reject_Reason)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, st_DC_Reject_Reason->e_DC_Reject_Reason, CMBS_IE_DC_REJECT_REASON);
}

E_CMBS_RC cmbs_api_ie_DCRejectReasonGet(void *pv_RefIE, ST_IE_DC_REJECT_REASON *st_DC_Reject_Reason)
{
    u8 u8_Value = 0;

    memset(st_DC_Reject_Reason, 0, sizeof(ST_IE_DC_REJECT_REASON));

    cmbs_api_ie_ByteValueGet(pv_RefIE, &u8_Value, CMBS_IE_DC_REJECT_REASON);

    st_DC_Reject_Reason->e_DC_Reject_Reason = (E_CMBS_DC_REJECT_REASON)u8_Value;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SuotaAppIdAdd(void *pv_RefIE, u32 u32_SuotaAppId)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIE, u32_SuotaAppId, CMBS_IE_SUOTA_APP_ID);
}

E_CMBS_RC cmbs_api_ie_SuotaAppIdGet(void *pv_RefIE, u32 *u32_SuotaAppId)
{
    return cmbs_api_ie_u32ValueGet(pv_RefIE, u32_SuotaAppId, CMBS_IE_SUOTA_APP_ID);
}

E_CMBS_RC cmbs_api_ie_SuotaSessionIdAdd(void *pv_RefIE, u32 u32_SuotaSessionId)
{
    return cmbs_api_ie_u32ValueAdd(pv_RefIE, u32_SuotaSessionId, CMBS_IE_SUOTA_SESSION_ID);
}

E_CMBS_RC cmbs_api_ie_SuotaSessionIdGet(void *pv_RefIE, u32 *u32_SuotaSessionId)
{
    return cmbs_api_ie_u32ValueGet(pv_RefIE, u32_SuotaSessionId, CMBS_IE_SUOTA_SESSION_ID);
}

E_CMBS_RC cmbs_api_ie_HsPropEventAdd(void *pv_RefIEList, ST_IE_HS_PROP_EVENT *pst_Param)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferHsPropEventAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u8_HsNumber);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u8_PropEvent);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u8_EMC, CMBS_EMC_SIZE);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u8_LineId);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u8_LineIdSubType);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->u8_DataLen);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Param->pu8_Data, pst_Param->u8_DataLen);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HS_PROP_EVENT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_HsPropEventGet(void *pv_RefIE, ST_IE_HS_PROP_EVENT *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_HS_PROP_EVENT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HS_PROP_EVENT);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u8_HsNumber);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u8_PropEvent);
    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->u8_EMC, CMBS_EMC_SIZE);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u8_LineId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u8_LineIdSubType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u8_DataLen);
    pst_Parameter->pu8_Data = pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SYPOSpecificationGet(void *pv_RefIE, ST_IE_SYPO_SPECIFICATION *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_SYPO_SPECIFICATION));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_SYPO_SPECIFICATION);

    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u32_WaitForSync);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Parameter->u8_GPIO);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_SYPOSpecificationAdd(void *pv_RefIE, ST_IE_SYPO_SPECIFICATION *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferSYPOSpecificationAdd);

    // put data
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->u32_WaitForSync);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->u8_GPIO);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_SYPO_SPECIFICATION);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointConnectionGet(void *pv_RefIE, ST_IE_AFE_ENDPOINTS_CONNECT *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_AFEEndpointIn = 0;
    u8  u8_AFEEndpointOut = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_AFE_ENDPOINTS_CONNECT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AFE_ENDPOINT_CONNECT);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AFEEndpointIn);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AFEEndpointOut);

    pst_Parameter->e_AFEEndPointIN = (E_CMBS_AFE_CHANNEL)u8_AFEEndpointIn;
    pst_Parameter->e_AFEEndPointOUT = (E_CMBS_AFE_CHANNEL)u8_AFEEndpointOut;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointConnectionAdd(void *pv_RefIE, ST_IE_AFE_ENDPOINTS_CONNECT *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAFEEndpointConnectionAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->e_AFEEndPointIN);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->e_AFEEndPointOUT);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AFE_ENDPOINT_CONNECT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointGet(void *pv_RefIE, ST_IE_AFE_ENDPOINT *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8   u8_AFEChannel = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_AFE_ENDPOINT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AFE_ENDPOINT);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AFEChannel);
    pst_Parameter->e_AFEChannel = (E_CMBS_AFE_CHANNEL)u8_AFEChannel;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointAdd(void *pv_RefIE, ST_IE_AFE_ENDPOINT *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAFEEndpointAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->e_AFEChannel);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AFE_ENDPOINT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointGainGet(void *pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16   u16_Size = 0;
    u8   u8_AFEChannel = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_AFE_ENDPOINT_GAIN));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AFE_ENDPOINT_GAIN);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AFEChannel);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u16 *)&pst_Parameter->s16_NumOfSteps);
    pst_Parameter->e_AFEChannel = (E_CMBS_AFE_CHANNEL)u8_AFEChannel;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointGainAdd(void *pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAFEEndpointGainAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->e_AFEChannel);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u16)pst_Parameter->s16_NumOfSteps);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AFE_ENDPOINT_GAIN);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointGainDBGet(void *pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN_DB *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8   u8_AFEChannel = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_AFE_ENDPOINT_GAIN_DB));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AFE_ENDPOINT_GAIN_DB);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AFEChannel);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u16 *)&pst_Parameter->s16_GainInDB);
    pst_Parameter->e_AFEChannel = (E_CMBS_AFE_CHANNEL)u8_AFEChannel;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_EndpointGainDBAdd(void *pv_RefIE, ST_IE_AFE_ENDPOINT_GAIN_DB *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAFEEndpointGainDBAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->e_AFEChannel);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u16)pst_Parameter->s16_GainInDB);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AFE_ENDPOINT_GAIN_DB);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureSettingsGet(void *pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_SETTINGS *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8  u8_AUX_Input = 0, u8_Temp;

    memset(pst_Parameter, 0, sizeof(ST_IE_AFE_AUX_MEASUREMENT_SETTINGS));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AFE_AUX_MEASUREMENT_SETTINGS);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_AUX_Input);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Temp);

    pst_Parameter->e_AUX_Input = (E_CMBS_AUX_INPUT)u8_AUX_Input;
    pst_Parameter->b_Bmp = u8_Temp;
    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureSettingsAdd(void *pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_SETTINGS *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAFEAUXMeasureSettingsAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->e_AUX_Input);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Parameter->b_Bmp);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AFE_AUX_MEASUREMENT_SETTINGS);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureResultGet(void *pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_RESULT *pst_Parameter)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16   u16_Size = 0;

    memset(pst_Parameter, 0, sizeof(ST_IE_AFE_AUX_MEASUREMENT_RESULT));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_AFE_AUX_MEASUREMENT_RESULT);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u16 *)&pst_Parameter->s16_Measurement_Result);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_AUXMeasureResultAdd(void *pv_RefIE, ST_IE_AFE_AUX_MEASUREMENT_RESULT *pst_Parameter)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferAFEAUXMeasureResultAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u16)pst_Parameter->s16_Measurement_Result);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_AFE_AUX_MEASUREMENT_RESULT);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_AFE_ResourceTypeAdd(void *pv_RefIE, u8 u8_ResourceType)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_ResourceType, CMBS_IE_AFE_RESOURCE_TYPE);
}

E_CMBS_RC cmbs_api_ie_AFE_ResourceTypeGet(void *pv_RefIE, u8 *u8_ResourceType)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_ResourceType, CMBS_IE_AFE_RESOURCE_TYPE);
}

E_CMBS_RC cmbs_api_ie_AFE_InstanceNumAdd(void *pv_RefIE, u8 u8_InstanceNum)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_InstanceNum, CMBS_IE_AFE_INSTANCE_NUM);
}

E_CMBS_RC cmbs_api_ie_AFE_InstanceNumGet(void *pv_RefIE, u8 *u8_InstanceNum)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_InstanceNum, CMBS_IE_AFE_INSTANCE_NUM);
}

E_CMBS_RC cmbs_api_ie_DHSGValueAdd(void *pv_RefIE, u8 u8_DHSGValue)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_DHSGValue, CMBS_IE_DHSG_VALUE);
}

E_CMBS_RC cmbs_api_ie_DHSGValueGet(void *pv_RefIE, u8 *u8_DHSGValue)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_DHSGValue, CMBS_IE_DHSG_VALUE);
}

E_CMBS_RC cmbs_api_ie_GPIOIDGet(void *pv_RefIE, PST_IE_GPIO_ID pst_GPIOID)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;
    u8 u8_GPIOBank = 0;
    memset(pst_GPIOID, 0, sizeof(ST_IE_GPIO_ID));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_GPIO_ID);

    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_GPIOID->u32_GPIO);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_GPIOBank);

    pst_GPIOID->e_GPIOBank = (E_CMBS_GPIO_BANK)u8_GPIOBank;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_GPIOIDAdd(void *pv_RefIE, PST_IE_GPIO_ID st_GPIOID)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferGPIOIDAdd);

    // put data
    u16_Size += cfr_ie_ser_u32(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_GPIOID->u32_GPIO);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_GPIOID->e_GPIOBank);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_GPIO_ID);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_GPIOModeGet(void *pv_RefIE, u8 *u8_Mode)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_Mode, CMBS_IE_GPIO_MODE);
}
E_CMBS_RC cmbs_api_ie_GPIOModeAdd(void *pv_RefIE, u8  u8_Mode)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_Mode, CMBS_IE_GPIO_MODE);
}
E_CMBS_RC cmbs_api_ie_GPIOValueGet(void *pv_RefIE, u8 *u8_Value)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_Value, CMBS_IE_GPIO_VALUE);
}
E_CMBS_RC cmbs_api_ie_GPIOValueAdd(void *pv_RefIE, u8  u8_Value)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_Value, CMBS_IE_GPIO_VALUE);
}
E_CMBS_RC cmbs_api_ie_GPIOPullTypeGet(void *pv_RefIE, u8 *u8_PullType)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_PullType, CMBS_IE_GPIO_PULL_TYPE);
}
E_CMBS_RC cmbs_api_ie_GPIOPullTypeAdd(void *pv_RefIE, u8 u8_PullType)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_PullType, CMBS_IE_GPIO_PULL_TYPE);
}
E_CMBS_RC cmbs_api_ie_GPIOPullEnaGet(void *pv_RefIE, u8 *u8_PullEna)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_PullEna, CMBS_IE_GPIO_PULL_ENA);
}
E_CMBS_RC cmbs_api_ie_GPIOPullEnaAdd(void *pv_RefIE, u8 u8_PullEna)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_PullEna, CMBS_IE_GPIO_PULL_ENA);
}
E_CMBS_RC cmbs_api_ie_GPIOEnaGet(void *pv_RefIE, u8 *u8_Ena)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_Ena, CMBS_IE_GPIO_ENA);
}
E_CMBS_RC cmbs_api_ie_GPIOEnaAdd(void *pv_RefIE, u8 u8_Ena)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_Ena, CMBS_IE_GPIO_ENA);
}
E_CMBS_RC cmbs_api_ie_ExtIntNumAdd(void *pv_RefIE, u8 u8_IntNum)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, u8_IntNum, CMBS_IE_EXT_INT_NUM);
}
E_CMBS_RC cmbs_api_ie_ExtIntNumGet(void *pv_RefIE, u8 *u8_IntNum)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, u8_IntNum, CMBS_IE_EXT_INT_NUM);
}
E_CMBS_RC cmbs_api_ie_ExtIntConfigurationAdd(void *pv_RefIE, PST_IE_INT_CONFIGURATION st_INTConfiguration)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferExtINTConfigAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_INTConfiguration->u8_INTPolarity);
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, st_INTConfiguration->u8_INTType);
    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_EXT_INT_CONFIGURATION);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}
E_CMBS_RC cmbs_api_ie_ExtIntConfigurationGet(void *pv_RefIE, PST_IE_INT_CONFIGURATION st_INTConfiguration)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(st_INTConfiguration, 0, sizeof(ST_IE_INT_CONFIGURATION));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_EXT_INT_CONFIGURATION);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &st_INTConfiguration->u8_INTPolarity);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &st_INTConfiguration->u8_INTType);
    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_TerminalCapabilitiesAdd(void *pv_RefIE, PST_IE_TERMINAL_CAPABILITIES pst_TermCapability)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferTerminalCapabilitiesAdd);

    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, (u8 *)pst_TermCapability, sizeof(ST_IE_TERMINAL_CAPABILITIES));

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_TERMINAL_CAPABILITIES);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_TerminalCapabilitiesGet(void *pv_RefIE, PST_IE_TERMINAL_CAPABILITIES pst_TermCapability)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_TermCapability, 0, sizeof(ST_IE_TERMINAL_CAPABILITIES));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_TERMINAL_CAPABILITIES);

    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE, (u8 *)pst_TermCapability, sizeof(ST_IE_TERMINAL_CAPABILITIES));

    return CMBS_RC_OK;
}

#ifdef CHECKSUM_SUPPORT

E_CMBS_RC cmbs_api_ie_ChecksumErrorAdd(void *pv_RefIEList, PST_IE_CHECKSUM_ERROR  pst_CheckSumError)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BufferChecksumErrorAdd);

    // u8_Buffer[CFR_IE_TYPE_POS] = (u8)CMBS_IE_CHECKSUM_ERROR;
    // u8_Buffer[CFR_IE_SIZE_POS] = sizeof(u16) + sizeof(u8);

    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CHECKSUM_ERROR);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, sizeof(u16) + sizeof(u8));

    // Error type:
    // u8_Buffer[CFR_IE_HEADER_SIZE] = (u8)pst_CheckSumError->e_CheckSumError;
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE, pst_CheckSumError->e_CheckSumError);

    // errorness received event
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + 1, pst_CheckSumError->u16_ReceivedEvent);

    // u16_Size = u8_Buffer[CFR_IE_SIZE_POS] + CFR_IE_HEADER_SIZE;

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, u16_Size);
    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_ChecksumErrorGet(void *pv_RefIE, PST_IE_CHECKSUM_ERROR  pst_CheckSumError)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u8 u8_Temp;

    memset(pst_CheckSumError, 0, sizeof(ST_IE_CHECKSUM_ERROR));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CHECKSUM_ERROR);

    cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE, &u8_Temp);
    cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + 1, &pst_CheckSumError->u16_ReceivedEvent);

    pst_CheckSumError->e_CheckSumError = u8_Temp;

    return CMBS_RC_OK;
}
#endif

E_CMBS_RC cmbs_api_ie_CallHoldReasonGet(void *pv_RefIE, PST_IE_CALL_HOLD_REASON pst_Reason)
{
    u8 *pu8_Buffer = (u8 *)pv_RefIE;
    u8 u8_Temp;

    memset(pst_Reason, 0, sizeof(ST_IE_CALL_HOLD_REASON));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_CALL_HOLD_REASON);

    cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE, &u8_Temp);
    pst_Reason->eReason = u8_Temp;

    return CMBS_RC_OK;
}

E_CMBS_RC cmbs_api_ie_CallHoldReasonAdd(void *pv_RefIE, PST_IE_CALL_HOLD_REASON pst_Reason)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(u8_BuffeCallHoldReasonAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Reason->eReason);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_CALL_HOLD_REASON);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}
/*----------------[End of file]------------------------------------------------------------------------------------------------------------------*/
