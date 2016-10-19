/*!
*  \file       cfr_cmbs.c
*  \brief      Target side
*  \author     stein
*
*  @(#)  %filespec: cfr_cmbs.c~DMZD53#5 %
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
* 14-feb-09 R.Stein   1   Initialize \n
* 14-feb-09 D.Kelbch  2   Project integration - VONE \n
* 14-dec-09 sergiym   ?   Add init for critical section \n
*******************************************************************************/

#include "tclib.h"
#include "embedded.h"
#include "cmbs_int.h"  /* internal API structure and defines */
#include "cfr_uart.h"  /* packet handler */

/* Globals */
ST_CMBS_API_INST g_CMBSInstance;

/*****************************************************************************
 * API Internal functions
 *****************************************************************************/

E_CMBS_RC         cmbs_int_EnvCreate(E_CMBS_API_MODE e_Mode, ST_CMBS_DEV * pst_DevCtl, ST_CMBS_DEV * pst_DevMedia)
{
    static u8 u8_AlreadyCreated = 0;

    if (u8_AlreadyCreated != 0)
    {
        return CMBS_RC_OK;
    }
    u8_AlreadyCreated = 1;

    if (e_Mode) {}       // not used
    if (pst_DevCtl) {}   // not used
    if (pst_DevMedia) {} // not used

    memset(&g_CMBSInstance, 0, sizeof(g_CMBSInstance));   // endianess on target is little

    g_CMBSInstance.u16_TargetVersion = CMBS_API_TARGET_VERSION;
    g_CMBSInstance.u16_TargetBuild = CMBS_TARGET_BUILD;

    CFR_CMBS_INIT_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);

    cfr_uartInitalize();

    // If EEPROM on RAM, this will be called later
#ifndef EEPROM_ON_RAM
    cfr_uartInitalize_from_eeprom();
#endif

    return CMBS_RC_OK;
}

E_CMBS_RC         cmbs_int_EnvDestroy(void)
{
    CFR_CMBS_DELETE_CRITICALSECTION(g_CMBSInstance.h_CriticalSectionTransmission);

    return   CMBS_RC_OK;
}

//EOF
