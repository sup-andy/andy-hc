/*!
*  \file       cmbs_fifo.c
*  \brief      Fifo message queue for CMBS host and CMBS target
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                      \n
*  ---------------------------------------------------------------------------\n
*
*******************************************************************************/
#include <string.h>

#include "cmbs_api.h"
#include "cmbs_int.h"
#include "cfr_ie.h"
#include "cfr_debug.h"
#include "cmbs_fifo.h"
#include "cfr_mssg.h"

#if defined(CMBS_API_TARGET)
#include "csys0reg.h"
#include "cos00int.h"     // Needed for critical section

#if defined (CSS)
#include "plicu.h"
#include "priorities.h"
#endif

#endif

// Brief
//------------
// Init the FIFO
void              cmbs_util_FifoInit(ST_CMBS_FIFO *p_Fifo,
                                     void *pv_Buffer,
                                     const u8 u8_ElementSize,
                                     const u8 u8_Size,
                                     CFR_CMBS_CRITICALSECTION p_cSection)
{
    //printf("cmbs_util_FifoInit-->\n");
    p_Fifo->u8_Count    = 0;
    p_Fifo->u8_ElemSize = u8_ElementSize;

    p_Fifo->p_Read  = p_Fifo->p_Write = pv_Buffer;
    p_Fifo->u8_Size = p_Fifo->u8_Read2End = p_Fifo->u8_Write2End = u8_Size;
    p_Fifo->p_cSection = p_cSection;

    CFR_CMBS_INIT_CRITICALSECTION(p_Fifo->p_cSection);
}


// Brief
//------------
// Push a message to FIFO
u8                cmbs_util_FifoPush(ST_CMBS_FIFO *p_Fifo, void *pv_Element)
{
    u8             u8_Write2End;
    u8 *p_Write;

    //printf("cmbs_util_FifoPush-->\n");

    CFR_CMBS_ENTER_CRITICALSECTION(p_Fifo->p_cSection);

    if (p_Fifo->u8_Count >= p_Fifo->u8_Size)
    {
        CFR_DBG_WARN("cmbs_util_FifoPush: FIFO full\n");

        CFR_CMBS_LEAVE_CRITICALSECTION(p_Fifo->p_cSection);
        return 0;
    }

    p_Write = p_Fifo->p_Write;

    memcpy(p_Write, pv_Element, p_Fifo->u8_ElemSize);

    p_Write += p_Fifo->u8_ElemSize;

    u8_Write2End = p_Fifo->u8_Write2End;

    if (--u8_Write2End == 0)
    {
        u8_Write2End = p_Fifo->u8_Size;
        p_Write -= u8_Write2End * p_Fifo->u8_ElemSize;
    }

    p_Fifo->u8_Write2End = u8_Write2End;
    p_Fifo->p_Write = p_Write;

    p_Fifo->u8_Count++;

    CFR_CMBS_LEAVE_CRITICALSECTION(p_Fifo->p_cSection);

    return 1;
}


// Brief
//------------
// Get a message from FIFO
void*            cmbs_util_FifoGet(ST_CMBS_FIFO *p_Fifo)
{
    //printf("cmbs_util_FifoGet-->\n");

    CFR_CMBS_ENTER_CRITICALSECTION(p_Fifo->p_cSection);

    if (p_Fifo->u8_Count == 0)
    {
        //      CFR_DBG_WARN( "cmbs_util_FifoGet: FIFO empty\n" );
        CFR_CMBS_LEAVE_CRITICALSECTION(p_Fifo->p_cSection);
        return NULL;
    }


    CFR_CMBS_LEAVE_CRITICALSECTION(p_Fifo->p_cSection);

    return p_Fifo->p_Read;
}

// Brief
//------------
// Pop (extract) a message to FIFO
void*            cmbs_util_FifoPop(ST_CMBS_FIFO *p_Fifo)
{
    u8             u8_Read2End;
    u8 *p_Read;
    void *p_Element;

    //printf("cmbs_util_FifoPop-->\n");

    CFR_CMBS_ENTER_CRITICALSECTION(p_Fifo->p_cSection);

    if (p_Fifo->u8_Count == 0)
    {
        CFR_DBG_WARN("cmbs_util_FifoGet: FIFO empty\n");
        CFR_CMBS_LEAVE_CRITICALSECTION(p_Fifo->p_cSection);
        return NULL;
    }

    u8_Read2End = p_Fifo->u8_Read2End;
    p_Read      = p_Fifo->p_Read;
    p_Element   = p_Read;

    p_Read += p_Fifo->u8_ElemSize;

    if (--u8_Read2End == 0)
    {
        u8_Read2End = p_Fifo->u8_Size;
        p_Read -= u8_Read2End * p_Fifo->u8_ElemSize;
    }

    p_Fifo->p_Read = p_Read;
    p_Fifo->u8_Read2End = u8_Read2End;

    p_Fifo->u8_Count--;

    CFR_CMBS_LEAVE_CRITICALSECTION(p_Fifo->p_cSection);

    return p_Element;
}

/*[----------- End Of File -------------------------------------------------------------------------------------------------------------]*/
