/*!
*  \file       cmbs_han_ie.c
*  \brief      Information Elements List functions for HAN
*  \author     andriig
*
*  @(#)  %filespec: cmbs_han_ie.c~BLRD53#9 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
* 27-05-14 ronenw  GIT  moved cmbs_han_ie_params_Union members into common union U_Buffer @ cfr_ie.h
*
*******************************************************************************/

#include "cfr_ie.h"
#include "cfr_debug.h"
#include "cmbs_han_ie.h"
#include "cmbs_han.h"
#include "cmbs_int.h"

#if defined(__arm)
#include "tclib.h"
#include "embedded.h"
#else
#include <string.h>
#endif

#if defined ( CMBS_API_TARGET )
extern U_Buffer s_Buffer;
#else
extern U_Buffer s_Buffer[CMBS_NUM_OF_HOST_THREADS];
#endif

#if defined ( CMBS_API_TARGET )
#define CMBS_IE_GET_BUFFER(NAME)            \
 u8 *pu8_Buffer = (u8 *)&s_Buffer.NAME;         \
 memset(pu8_Buffer,0,sizeof(s_Buffer.NAME));
#else
#define CMBS_IE_GET_BUFFER(NAME)            \
 u8 * pu8_Buffer;               \
 u8 u8_ThreadIdx = cfr_ie_getThreadIdx();         \
 if ( u8_ThreadIdx == CMBS_UNKNOWN_THREAD )         \
 {                  \
    CFR_DBG_ERROR("Unknown Thread Id!!");        \
    return CMBS_RC_ERROR_GENERAL;          \
 }                  \
 else                 \
 {                  \
  pu8_Buffer = (u8 *)&(s_Buffer[u8_ThreadIdx].NAME);       \
 }                                                           \
    memset(pu8_Buffer,0,sizeof(s_Buffer[u8_ThreadIdx].NAME));
#endif



//////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HanCfgAdd(void * pv_RefIEList, ST_IE_HAN_CONFIG* pst_Cfg)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIEList, pst_Cfg->st_HanCfg.u8_HANServiceConfig, CMBS_IE_HAN_CFG);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HanCfgGet(void * pv_RefIEList, ST_IE_HAN_CONFIG* pst_Cfg)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIEList, &pst_Cfg->st_HanCfg.u8_HANServiceConfig, CMBS_IE_HAN_CFG);
}

E_CMBS_RC    cmbs_api_ie_HanUleBaseInfoAdd(void * pv_RefIEList, ST_IE_HAN_BASE_INFO* pst_HanBaseInfo)
{
    u16 u16_Size = 0;

    CMBS_IE_GET_BUFFER(HanBaseInfoAdd);

    // put data
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HanBaseInfo->u8_UleAppProtocolId);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HanBaseInfo->u8_UleAppProtocolVersion);

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_BASE_INFO);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HanUleBaseInfoGet(void * pv_RefIEList, ST_IE_HAN_BASE_INFO* pst_HanBaseInfo)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIEList;
    u16  u16_Size = 0;

    memset(pst_HanBaseInfo, 0, sizeof(ST_IE_HAN_BASE_INFO));

    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_BASE_INFO);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HanBaseInfo->u8_UleAppProtocolId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HanBaseInfo->u8_UleAppProtocolVersion);

    return CMBS_RC_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC  cmbs_ie_HanDeviceTableBriefAdd(void * pv_RefIEList, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8* pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE)
{
    u16 u16_Size = 0;
    int Devices;
    int Units;
    ST_HAN_BRIEF_DEVICE_INFO* BriefEntries = (ST_HAN_BRIEF_DEVICE_INFO*)pArrayPtr;
    CMBS_IE_GET_BUFFER(HanTableAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_NumOfEntries);
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_IndexOfFirstEntry);

    UNUSED_PARAMETER(u8_entrySize);
    for (Devices = 0; Devices < u16_NumOfEntries; Devices++)
    {

        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].u16_DeviceId);
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].u8_RegistrationStatus);
        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].u16_RequestedPageTime);
        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].u16_PageTime);   //final page time, after negotiation
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].u8_NumberOfUnits);

        for (Units = 0; Units < BriefEntries[Devices].u8_NumberOfUnits; Units++)
        {
            u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].st_UnitsInfo[Units].u8_UnitId);
            u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, BriefEntries[Devices].st_UnitsInfo[Units].u16_UnitType);
        }

    }

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, e_IE);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General implementation
static E_CMBS_RC  cmbs_ie_HanTableAdd(void * pv_RefIEList, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8* pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE)
{
    u8  u8_Buffer[ CFR_IE_HEADER_SIZE + CMBS_HAN_IE_TABLE_SIZE] = {0};
    u16 u16_Size = 0;

    // put data
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_NumOfEntries);
    u16_Size += cfr_ie_ser_u16(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_IndexOfFirstEntry);
    u16_Size += cfr_ie_ser_pu8(u8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pArrayPtr, u16_NumOfEntries * u8_entrySize);

    // put header
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_TYPE_POS, e_IE);
    cfr_ie_ser_u16(u8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, u8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static E_CMBS_RC cmbs_ie_HanTableGet(void * pv_RefIEList, u16* pu16_NumOfEntries, u16* pu16_IndexOfFirstEntry, u8 * pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIEList;
    u16  u16_Size = 0;
    u16  u16_Length = 0;

    CHECK_IE_TYPE(pu8_Buffer, e_IE);

    cfr_ie_dser_u16(pu8_Buffer + CFR_IE_SIZE_POS, &u16_Length);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pu16_NumOfEntries);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pu16_IndexOfFirstEntry);

    if (u16_Length != (u16_Size + u8_entrySize * (*pu16_NumOfEntries)))
        return CMBS_RC_ERROR_PARAMETER;

    u16_Size += cfr_ie_dser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pArrayPtr, (*pu16_NumOfEntries) * u8_entrySize);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC  cmbs_ie_HanDeviceTableExtendedAdd(void * pv_RefIEList, u16 u16_NumOfEntries, u16 u16_IndexOfFirstEntry, u8* pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE)
{
// u8  u8_Buffer[ CFR_IE_HEADER_SIZE + CMBS_HAN_IE_TABLE_SIZE] = {0};
    u16 u16_Size = 0;
    int Devices, Units, Interfaces;
    ST_HAN_EXTENDED_DEVICE_INFO* ExtendedEntries = (ST_HAN_EXTENDED_DEVICE_INFO*)pArrayPtr;
    CMBS_IE_GET_BUFFER(HanTableAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_NumOfEntries);
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u16_IndexOfFirstEntry);
    UNUSED_PARAMETER(u8_entrySize);

    for (Devices = 0; Devices < u16_NumOfEntries; Devices++)
    {

        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u16_DeviceId);

        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_IPUI[0]);
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_IPUI[1]);
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_IPUI[2]);
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_IPUI[3]);
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_IPUI[4]);

        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_RegistrationStatus);
        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u16_RequestedPageTime);
        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u16_PageTime);   //final page time, after negotiation
        u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u16_DeviceEMC);
        u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].u8_NumberOfUnits);

        for (Units = 0; Units < ExtendedEntries[Devices].u8_NumberOfUnits; Units++)
        {
            u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].st_UnitsInfo[Units].u8_UnitId);
            u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].st_UnitsInfo[Units].u16_UnitType);
            u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].st_UnitsInfo[Units].u16_NumberOfOptionalInterfaces);
            for (Interfaces = 0; Interfaces < ExtendedEntries[Devices].st_UnitsInfo[Units].u16_NumberOfOptionalInterfaces; Interfaces++)
            {
                u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, ExtendedEntries[Devices].st_UnitsInfo[Units].u16_OptionalInterfaces[Interfaces]);
            }
        }

    }

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, e_IE);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static E_CMBS_RC cmbs_ie_HanDeviceTableBriefGet(void * pv_RefIEList, u16* pu16_NumOfEntries, u16* pu16_IndexOfFirstEntry, u8 * pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIEList;
    u16  u16_Size = 0;
    u16  u16_Length = 0;
    int  Devices;
    int  Units;
    ST_HAN_BRIEF_DEVICE_INFO* BriefEntries = (ST_HAN_BRIEF_DEVICE_INFO*)pArrayPtr;

    CHECK_IE_TYPE(pu8_Buffer, e_IE);

    cfr_ie_dser_u16(pu8_Buffer + CFR_IE_SIZE_POS, &u16_Length);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pu16_NumOfEntries);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pu16_IndexOfFirstEntry);
    UNUSED_PARAMETER(u8_entrySize);

    for (Devices = 0; Devices < *pu16_NumOfEntries; Devices++)
    {
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].u16_DeviceId);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].u8_RegistrationStatus);
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].u16_RequestedPageTime);
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].u16_PageTime);   //final page time, after negotiation
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].u8_NumberOfUnits);

        for (Units = 0; Units < BriefEntries[Devices].u8_NumberOfUnits; Units++)
        {
            u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].st_UnitsInfo[Units].u8_UnitId);
            u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &BriefEntries[Devices].st_UnitsInfo[Units].u16_UnitType);
        }

    }

    if (u16_Length != u16_Size)
        return CMBS_RC_ERROR_PARAMETER;
    else
        return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static E_CMBS_RC cmbs_ie_HanDeviceTableExtendedGet(void * pv_RefIEList, u16* pu16_NumOfEntries, u16* pu16_IndexOfFirstEntry, u8 * pArrayPtr, u8 u8_entrySize, E_CMBS_IE_HAN_TYPE e_IE)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIEList;
    u16  u16_Size = 0;
    u16  u16_Length = 0;
    int  Devices, Units, Interfaces;
    ST_HAN_EXTENDED_DEVICE_INFO* ExtendedEntries = (ST_HAN_EXTENDED_DEVICE_INFO*)pArrayPtr;

    CHECK_IE_TYPE(pu8_Buffer, e_IE);

    cfr_ie_dser_u16(pu8_Buffer + CFR_IE_SIZE_POS, &u16_Length);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pu16_NumOfEntries);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pu16_IndexOfFirstEntry);
    UNUSED_PARAMETER(u8_entrySize);

    for (Devices = 0; Devices < *pu16_NumOfEntries; Devices++)
    {
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u16_DeviceId);

        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_IPUI[0]);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_IPUI[1]);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_IPUI[2]);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_IPUI[3]);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_IPUI[4]);

        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_RegistrationStatus);
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u16_RequestedPageTime);
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u16_PageTime);   //final page time, after negotiation
        u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u16_DeviceEMC);
        u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].u8_NumberOfUnits);

        for (Units = 0; Units < ExtendedEntries[Devices].u8_NumberOfUnits; Units++)
        {
            u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].st_UnitsInfo[Units].u8_UnitId);
            u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].st_UnitsInfo[Units].u16_UnitType);
            u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].st_UnitsInfo[Units].u16_NumberOfOptionalInterfaces);

            for (Interfaces = 0; Interfaces < ExtendedEntries[Devices].st_UnitsInfo[Units].u16_NumberOfOptionalInterfaces; Interfaces++)
            {
                u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &ExtendedEntries[Devices].st_UnitsInfo[Units].u16_OptionalInterfaces[Interfaces]);
            }

        }

    }

    if (u16_Length != u16_Size)
        return CMBS_RC_ERROR_PARAMETER;
    else
        return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANDeviceTableBriefAdd(void * pv_RefIEList, ST_IE_HAN_BRIEF_DEVICE_ENTRIES* pst_HANDeviceEntries)
{
    return cmbs_ie_HanDeviceTableBriefAdd(pv_RefIEList,
                                          pst_HANDeviceEntries->u16_NumOfEntries,
                                          pst_HANDeviceEntries->u16_StartEntryIndex,
                                          (u8*)(pst_HANDeviceEntries->pst_DeviceEntries),
                                          sizeof(ST_HAN_DEVICE_ENTRY),
                                          CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANDeviceTableBriefGet(void * pv_RefIEList, ST_IE_HAN_BRIEF_DEVICE_ENTRIES* pst_HANDeviceEntries)
{
    return cmbs_ie_HanDeviceTableBriefGet(pv_RefIEList,
                                          &(pst_HANDeviceEntries->u16_NumOfEntries),
                                          &(pst_HANDeviceEntries->u16_StartEntryIndex),
                                          (u8*)pst_HANDeviceEntries->pst_DeviceEntries,
                                          sizeof(ST_HAN_BRIEF_DEVICE_INFO),
                                          CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES);
}

//////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANDeviceTableExtendedAdd(void * pv_RefIEList, ST_IE_HAN_EXTENDED_DEVICE_ENTRIES* pst_HANExtendedDeviceEntries)
{
    return cmbs_ie_HanDeviceTableExtendedAdd(pv_RefIEList,
            pst_HANExtendedDeviceEntries->u16_NumOfEntries,
            pst_HANExtendedDeviceEntries->u16_StartEntryIndex,
            (u8*)(pst_HANExtendedDeviceEntries->pst_DeviceEntries),
            sizeof(ST_HAN_DEVICE_ENTRY),
            CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANDeviceTableExtendedGet(void * pv_RefIEList, ST_IE_HAN_EXTENDED_DEVICE_ENTRIES* pst_HANExtendedDeviceEntries)
{
    return cmbs_ie_HanDeviceTableExtendedGet(pv_RefIEList,
            &(pst_HANExtendedDeviceEntries->u16_NumOfEntries),
            &(pst_HANExtendedDeviceEntries->u16_StartEntryIndex),
            (u8*)pst_HANExtendedDeviceEntries->pst_DeviceEntries,
            sizeof(ST_HAN_EXTENDED_DEVICE_INFO),
            CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANBindTableAdd(void * pv_RefIEList, ST_IE_HAN_BIND_ENTRIES*  pst_HanBinds)
{
    UNUSED_PARAMETER(pv_RefIEList);
    UNUSED_PARAMETER(pst_HanBinds);
    //printf("cmbs_api_ie_HANBindTableAdd not implemented yet!!!!!!!!\n");

    // Using a general implementation
    return cmbs_ie_HanTableAdd(pv_RefIEList,
                               pst_HanBinds->u16_NumOfEntries,
                               pst_HanBinds->u16_StartEntryIndex,
                               (u8*)pst_HanBinds->pst_BindEntries,
                               sizeof(ST_HAN_BIND_ENTRY),
                               CMBS_IE_HAN_BIND_TABLE_ENTRIES);
    //return CMBS_RC_ERROR_NOT_SUPPORTED;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANBindTableGet(void * pv_RefIEList, ST_IE_HAN_BIND_ENTRIES*   pst_HanBinds)
{
    UNUSED_PARAMETER(pv_RefIEList);
    UNUSED_PARAMETER(pst_HanBinds);
    //printf("cmbs_api_ie_HANBindTableGet not implemented yet!!!!!!!!\n");

    // Using a general implementation
    return cmbs_ie_HanTableGet(pv_RefIEList,
                               &(pst_HanBinds->u16_NumOfEntries),
                               &(pst_HanBinds->u16_StartEntryIndex),
                               (u8*)pst_HanBinds->pst_BindEntries,
                               sizeof(ST_HAN_BIND_ENTRY),
                               CMBS_IE_HAN_BIND_TABLE_ENTRIES);
    //return CMBS_RC_ERROR_NOT_SUPPORTED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANGroupTableAdd(void * pv_RefIEList, ST_IE_HAN_GROUP_ENTRIES*  pst_HanGroups)
{
    UNUSED_PARAMETER(pv_RefIEList);
    UNUSED_PARAMETER(pst_HanGroups);
    //printf("cmbs_api_ie_HANGroupTableAdd not implemented yet!!!!!!!!\n");

    // Using a general implementation
    return cmbs_ie_HanTableAdd(pv_RefIEList,
                               pst_HanGroups->u16_NumOfEntries,
                               pst_HanGroups->u16_StartEntryIndex,
                               (u8*)pst_HanGroups->pst_GroupEntries,
                               sizeof(ST_HAN_GROUP_ENTRY),
                               CMBS_IE_HAN_GROUP_TABLE_ENTRIES);
    //return CMBS_RC_ERROR_NOT_SUPPORTED;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HANGroupTableGet(void * pv_RefIEList, ST_IE_HAN_GROUP_ENTRIES*  pst_HanGroups)
{
    UNUSED_PARAMETER(pv_RefIEList);
    UNUSED_PARAMETER(pst_HanGroups);
    //printf("cmbs_api_ie_HANGroupTableGet not implemented yet!!!!!!!!\n");

    // Using a general implementation
    return cmbs_ie_HanTableGet(pv_RefIEList,
                               &(pst_HanGroups->u16_NumOfEntries),
                               &(pst_HanGroups->u16_StartEntryIndex),
                               (u8*)pst_HanGroups->pst_GroupEntries,
                               sizeof(ST_HAN_GROUP_ENTRY),
                               CMBS_IE_HAN_GROUP_TABLE_ENTRIES);
    //return CMBS_RC_ERROR_NOT_SUPPORTED;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HanMsgRegInfoAdd(void * pv_RefIEList, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo)
{
    //u8  u8_Buffer[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_MSG_REG_INFO)] = {0};
    u16 u16_Size = 0;
    CMBS_IE_GET_BUFFER(RegInfoAdd);
    // put data
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HANMsgRegInfo->u8_UnitId);
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_HANMsgRegInfo->u16_InterfaceId);

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_MSG_REG_INFO);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC    cmbs_api_ie_HanMsgRegInfoGet(void * pv_RefIEList, ST_HAN_MSG_REG_INFO * pst_HANMsgRegInfo)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIEList;
    u16  u16_Size = 0;

    memset(pst_HANMsgRegInfo, 0, sizeof(ST_HAN_MSG_REG_INFO));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_MSG_REG_INFO);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HANMsgRegInfo->u8_UnitId);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_HANMsgRegInfo->u16_InterfaceId);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANConnectionStatusAdd(void * pv_RefIE, u16 pu16_ConnectionStatus)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, pu16_ConnectionStatus, CMBS_IE_HAN_DEVICE_CONNECTION_STATUS);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANConnectionStatusGet(void * pv_RefIE, u16* pu16_ConnectionStatus)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_ConnectionStatus, CMBS_IE_HAN_DEVICE_CONNECTION_STATUS);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANDeviceAdd(void * pv_RefIE, u16 pu16_HANDevice)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, pu16_HANDevice, CMBS_IE_HAN_DEVICE);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANDeviceGet(void * pv_RefIE, u16* pu16_HANDevice)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_HANDevice, CMBS_IE_HAN_DEVICE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANRegErrorReasonAdd(void * pv_RefIE, u16 u16_Reason)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_Reason, CMBS_IE_HAN_DEVICE_REG_ERROR_REASON);
}

E_CMBS_RC cmbs_api_ie_HANRegErrorReasonGet(void * pv_RefIE, u16* pu16_Reason)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_Reason, CMBS_IE_HAN_DEVICE_REG_ERROR_REASON);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANForcefulDeRegErrorReasonAdd(void * pv_RefIE, u16 u16_Reason)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_Reason, CMBS_IE_HAN_DEVICE_FORCEFUL_DELETE_ERROR_STATUS_REASON);
}

E_CMBS_RC cmbs_api_ie_HANForcefulDeRegErrorReasonGet(void * pv_RefIE, u16* pu16_Reason)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_Reason, CMBS_IE_HAN_DEVICE_FORCEFUL_DELETE_ERROR_STATUS_REASON);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANSendErrorReasonAdd(void * pv_RefIE, u16 u16_Reason)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_Reason, CMBS_IE_HAN_SEND_FAIL_REASON);
}

E_CMBS_RC cmbs_api_ie_HANSendErrorReasonGet(void * pv_RefIE, u16* pu16_Reason)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_Reason, CMBS_IE_HAN_SEND_FAIL_REASON);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANTxEndedReasonAdd(void * pv_RefIE, u16 u16_Reason)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_Reason, CMBS_IE_HAN_TX_ENDED_REASON);
}

E_CMBS_RC cmbs_api_ie_HANTxEndedReasonGet(void * pv_RefIE, u16* pu16_Reason)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_Reason, CMBS_IE_HAN_TX_ENDED_REASON);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANReqIDAdd(void * pv_RefIE, u16 u16_RequestID)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIE, u16_RequestID, CMBS_IE_REQUEST_ID);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANReqIDGet(void * pv_RefIE, u16* pu16_RequestID)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIE, pu16_RequestID, CMBS_IE_REQUEST_ID);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANMsgCtlAdd(void * pv_RefIE,  PST_IE_HAN_MSG_CTL pst_MessageControl)
{
    return cmbs_api_ie_ByteValueAdd(pv_RefIE, *(u8*)pst_MessageControl, CMBS_IE_HAN_MSG_CONTROL);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANMsgCtlGet(void * pv_RefIE, PST_IE_HAN_MSG_CTL pst_MessageControl)
{
    return cmbs_api_ie_ByteValueGet(pv_RefIE, (u8*)pst_MessageControl, CMBS_IE_HAN_MSG_CONTROL);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANMsgAdd(void * pv_RefIE, ST_IE_HAN_MSG * pst_HANMsg)
{
    //u8  u8_Buffer[ CFR_IE_HEADER_SIZE + sizeof(ST_IE_HAN_MSG) + CMBS_HAN_MAX_MSG_LEN] = {0};
    u16 u16_Size = 0;
    u8 u8_Temp;
    CMBS_IE_GET_BUFFER(HanMsgAdd);
    // put data
    // check size
    if (pst_HANMsg->u16_DataLen > CMBS_HAN_MAX_MSG_LEN)
        return CMBS_RC_ERROR_OUT_OF_MEM;

    // st_SrcDeviceUnit
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               pst_HANMsg->u16_SrcDeviceId);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                              pst_HANMsg->u8_SrcUnitId);

    // st_DstDeviceUnit
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               pst_HANMsg->u16_DstDeviceId);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                              pst_HANMsg->u8_DstUnitId);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                              pst_HANMsg->u8_DstAddressType);
    // st_MsgTransport
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               pst_HANMsg->st_MsgTransport.u16_Reserved);

    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                              pst_HANMsg->u8_MsgSequence);

    u8_Temp = pst_HANMsg->e_MsgType;
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_Temp);

    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                              pst_HANMsg->u8_InterfaceType);
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               pst_HANMsg->u16_InterfaceId);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                              pst_HANMsg->u8_InterfaceMember);
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               pst_HANMsg->u16_DataLen);
    u16_Size += cfr_ie_ser_pu8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               pst_HANMsg->pu8_Data, pst_HANMsg->u16_DataLen);

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_MSG);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANMsgGet(void * pv_RefIE, ST_IE_HAN_MSG * pst_HANMsg)
{
    u8 *pu8_Buffer = (u8*)pv_RefIE;
    u16 u16_Size = 0;
    u8 u8_Temp;

    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_MSG);
    pst_HANMsg->e_MsgType = 0;

    // st_SrcDeviceUnit
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                                &pst_HANMsg->u16_SrcDeviceId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               &pst_HANMsg->u8_SrcUnitId);

    // st_DstDeviceUnit
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                                &pst_HANMsg->u16_DstDeviceId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               &pst_HANMsg->u8_DstUnitId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               &pst_HANMsg->u8_DstAddressType);
    // st_MsgTransport
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                                &pst_HANMsg->st_MsgTransport.u16_Reserved);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               &pst_HANMsg->u8_MsgSequence);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_Temp);
    pst_HANMsg->e_MsgType = u8_Temp;

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               &pst_HANMsg->u8_InterfaceType);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                                &pst_HANMsg->u16_InterfaceId);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                               &pst_HANMsg->u8_InterfaceMember);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
                                &pst_HANMsg->u16_DataLen);

    if (pst_HANMsg->u16_DataLen && pst_HANMsg->pu8_Data)
    {
        memcpy(pst_HANMsg->pu8_Data,
               pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size,
               pst_HANMsg->u16_DataLen);
    }
    return CMBS_RC_OK;
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANNumOfEntriesAdd(void * pv_RefIEList, u16 u16_NumOfEntries)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIEList, u16_NumOfEntries, CMBS_IE_HAN_NUM_OF_ENTRIES);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANNumOfEntriesGet(void * pv_RefIEList, u16 * pu16_NumOfEntries)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIEList, pu16_NumOfEntries, CMBS_IE_HAN_NUM_OF_ENTRIES);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANIndex1stEntryAdd(void * pv_RefIEList, u16 u16_IndexOfFirstEntry)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIEList, u16_IndexOfFirstEntry, CMBS_IE_HAN_INDEX_1ST_ENTRY);
}
//////////////////////////////////////////////////////////////////////////
E_CMBS_RC cmbs_api_ie_HANIndex1stEntryGet(void * pv_RefIEList, u16 * pu16_IndexOfFirstEntry)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIEList, pu16_IndexOfFirstEntry, CMBS_IE_HAN_INDEX_1ST_ENTRY);
}
//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANTableUpdateInfoAdd(void * pv_RefIE,
        ST_IE_HAN_TABLE_UPDATE_INFO * pst_HANTableUpdateInfo)
{
    //u8  pu8_Buffer[ CFR_IE_HEADER_SIZE + sizeof(ST_IE_HAN_TABLE_UPDATE_INFO)] = {0};
    u16 u16_Size = 0;
    u8 u8_TableId = pst_HANTableUpdateInfo->e_Table;
    CMBS_IE_GET_BUFFER(UpdateInfoAdd);

    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, u8_TableId);

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_TABLE_UPDATE_INFO);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIE, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANTableUpdateInfoGet(void * pv_RefIE,
        ST_IE_HAN_TABLE_UPDATE_INFO * pst_HANTableUpdateInfo)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIE;
    u16 u16_Size = 0;
    u8  u8_TableId = 0;

    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_TABLE_UPDATE_INFO);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &u8_TableId);
    pst_HANTableUpdateInfo->e_Table = u8_TableId;

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANRegStage1OKResParamsAdd(void * pv_RefIEList, ST_HAN_REG_STAGE_1_STATUS* pst_RegStatus)
{
    //u8  u8_Buffer[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_REG_STAGE_1_STATUS)] = {0};
    u16 u16_Size = 0;
    CMBS_IE_GET_BUFFER(Stage1ParamsAdd);

    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u8_IPUI[0]);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u8_IPUI[1]);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u8_IPUI[2]);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u8_IPUI[3]);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u8_IPUI[4]);

    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_DEVICE_REG_STAGE1_OK_STATUS_PARAMS);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANUnknownDeviceContactedParamsAdd(void * pv_RefIEList, ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS* pst_Params)
{
    //u8  u8_Buffer[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_REG_STAGE_1_STATUS)] = {0};
    u16 u16_Size = 0;
    CMBS_IE_GET_BUFFER(UnknownDeviceParamsAdd);

    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Params->SetupType);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Params->NodeResponse);

    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Params->u8_IPUI[0]);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Params->u8_IPUI[1]);
    u16_Size += cfr_ie_ser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_Params->u8_IPUI[2]);


    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_UNKNOWN_DEVICE_CONTACTED_PARAMS);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANRegStage2OKResParamsAdd(void * pv_RefIEList, ST_HAN_REG_STAGE_2_STATUS* pst_RegStatus)
{
    //u8  u8_Buffer[ CFR_IE_HEADER_SIZE + sizeof(ST_HAN_REG_STAGE_2_STATUS)] = {0};
    u16 u16_Size = 0;
    CMBS_IE_GET_BUFFER(Stage2ParamsAdd);

    // put data
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u16_UleVersion);
    u16_Size += cfr_ie_ser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u16_FunVersion);
    u16_Size += cfr_ie_ser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u32_OriginalDevicePagingInterval);
    u16_Size += cfr_ie_ser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, pst_RegStatus->u32_ActualDevicePagingInterval);
    // put header
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_TYPE_POS, CMBS_IE_HAN_DEVICE_REG_STAGE2_OK_STATUS_PARAMS);
    cfr_ie_ser_u16(pu8_Buffer + CFR_IE_SIZE_POS, u16_Size);

    cfr_ie_ItemAdd(pv_RefIEList, pu8_Buffer, CFR_IE_HEADER_SIZE + u16_Size);

    return CMBS_RC_OK;
}

//////////////////////////////////////////////////////////////////////////


E_CMBS_RC  cmbs_api_ie_HANRegStage2OKResParamsGet(void * pv_RefIE, ST_HAN_REG_STAGE_2_STATUS* pst_RegStatus)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_RegStatus, 0, sizeof(ST_HAN_REG_STAGE_2_STATUS));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_DEVICE_REG_STAGE2_OK_STATUS_PARAMS);

    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u16_UleVersion);
    u16_Size += cfr_ie_dser_u16(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u16_FunVersion);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u32_OriginalDevicePagingInterval);
    u16_Size += cfr_ie_dser_u32(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u32_ActualDevicePagingInterval);
    return CMBS_RC_OK;

}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANRegStage1OKResParamsGet(void * pv_RefIE, ST_HAN_REG_STAGE_1_STATUS* pst_RegStatus)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIE;
    u16  u16_Size = 0;

    memset(pst_RegStatus, 0, sizeof(ST_HAN_REG_STAGE_1_STATUS));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_DEVICE_REG_STAGE1_OK_STATUS_PARAMS);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u8_IPUI[0]);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u8_IPUI[1]);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u8_IPUI[2]);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u8_IPUI[3]);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_RegStatus->u8_IPUI[4]);

    return CMBS_RC_OK;

}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANUnknownDeviceContactedParamsGet(void * pv_RefIEList, ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS* pst_Params)
{
    u8 *    pu8_Buffer = (u8*)pv_RefIEList;
    u16  u16_Size = 0;

    memset(pst_Params, 0, sizeof(ST_HAN_UNKNOWN_DEVICE_CONTACT_PARAMS));
    CHECK_IE_TYPE(pu8_Buffer, CMBS_IE_HAN_UNKNOWN_DEVICE_CONTACTED_PARAMS);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Params->SetupType);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Params->NodeResponse);

    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Params->u8_IPUI[0]);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Params->u8_IPUI[1]);
    u16_Size += cfr_ie_dser_u8(pu8_Buffer + CFR_IE_HEADER_SIZE + u16_Size, &pst_Params->u8_IPUI[2]);

    return CMBS_RC_OK;

}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANGeneralStatusAdd(void * pv_RefIEList, ST_HAN_GENERAL_STATUS* pst_Status)
{
    return cmbs_api_ie_ShortValueAdd(pv_RefIEList, pst_Status->u16_Status, CMBS_IE_HAN_GENERAL_STATUS);
}

//////////////////////////////////////////////////////////////////////////

E_CMBS_RC cmbs_api_ie_HANGeneralStatusGet(void * pv_RefIEList, ST_HAN_GENERAL_STATUS* pst_Staus)
{
    return cmbs_api_ie_ShortValueGet(pv_RefIEList, &pst_Staus->u16_Status, CMBS_IE_HAN_GENERAL_STATUS);

}

//////////////////////////////////////////////////////////////////////////
