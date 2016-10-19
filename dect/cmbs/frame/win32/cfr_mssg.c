/*!
* \file   cfr_mssg.c
* \brief
* \Author  kelbch
*
* @(#) %filespec: cfr_mssg.c~2 %
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*******************************************************************************/

#include <stdio.h>
#include "cmbs_platf.h"
#include "cmbs_api.h"
#include "cfr_mssg.h"

void                  cfr_MQueueThreadIDUpdate(PST_ICOM_ENTRY pThis, u32 u32_ThreadID)
{
    pThis->u32_ThreadID = u32_ThreadID;
}

PST_ICOM_ENTRY          cfr_MQueueCreate(u32 u32_ThreadID, u32 u32_Timeout)
{
    PST_ICOM_ENTRY pThis = NULL;

    pThis = (PST_ICOM_ENTRY)malloc(sizeof(ST_ICOM_ENTRY));

    if (pThis)
    {
        pThis->u32_ThreadID = u32_ThreadID;
        pThis->u32_Timeout  = u32_Timeout;
        pThis->u32_Cleanup  = FALSE;
    }

    return pThis;
}

E_CFR_INTERPROCESS     cfr_MQueueCleanup(PST_ICOM_ENTRY pThis)
{
    MSG    msg;

    if (pThis->u32_Cleanup)
    {
        do
        {
            if (!PeekMessage(&msg, NULL, 0, CFR_CMBS_END, PM_REMOVE))
            {
                break;
            }
        } while (msg.message != CFR_CMBS_MSSG_DESTROY);

        pThis->u32_Cleanup = FALSE;

        return E_CFR_INTERPROCESS_FAILT;
    }

    return E_CFR_INTERPROCESS_MSSG;
}

void                    cfr_MQueueDestroy(PST_ICOM_ENTRY pThis)
{
    if (pThis)
    {
        pThis->u32_Cleanup = TRUE;
        cfr_MQueueSend(pThis, CFR_CMBS_MSSG_DESTROY, NULL, 0);
        // only useful, because system is synchron and not asynchronize
        cfr_MQueueCleanup(pThis);

        free(pThis);
    }
}

void                    cfr_MQueueSend(PST_ICOM_ENTRY pThis, u32 u32_MssgID, void * pv_Param, u16 u16_ParamSize)
{
    PBYTE    pbParam = NULL;

    if (u16_ParamSize)
    {
        pbParam = malloc(u16_ParamSize);

        if (pbParam)
        {
            memcpy(pbParam, (PBYTE)pv_Param, u16_ParamSize);
        }
    }

    if (pbParam || !u16_ParamSize)
    {
        if (!PostThreadMessage(pThis->u32_ThreadID, u32_MssgID, (WPARAM)u16_ParamSize, (LPARAM)pbParam))
        {
            OutputDebugString("\nPostThreadMessage failed\n\n");
        }
    }
}

E_CFR_INTERPROCESS      cfr_MQueueGet(PST_ICOM_ENTRY pThis, u32 * pu32_MssgID, void **ppv_Param, u16 * pu16_ParamSize)
{
    MSG    msg;
    if (GetMessage(&msg , NULL , CFR_CMBS_START , CFR_CMBS_END))
    {
        if (msg.message >= CFR_CMBS_START && msg.message <= CFR_CMBS_END)
        {
            *pu32_MssgID      = msg.message;
            *ppv_Param        = (void*)msg.lParam;
            *pu16_ParamSize   = (u16)msg.wParam;

            return cfr_MQueueCleanup(pThis);
        }
    }

    return E_CFR_INTERPROCESS_FAILT;
}


void                    cfr_MQueueMssgFree(PST_ICOM_ENTRY pEntry, void * pv_Param)
{
    if (pv_Param)
    {
        free(pv_Param);
    }
}
//*/

