/*************************************************************************************************************
*** List Change Notification
**
**************************************************************************************************************/

/*******************************************
Includes
********************************************/
#include "ListChangeNotif.h"
#include "cfr_mssg.h"
#include "appcmbs.h"
#include "ListsApp.h"
#include "appsrv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************
Defines
********************************************/

/*******************************************
Types
********************************************/

/*******************************************
Globals
********************************************/

/*******************************************
Auxiliary
********************************************/

/* ***************** Auxiliary end ***************** */

/*******************************************
List Change Notification API

 u16_HsId may be a single HS ID or all handsets which are attached to the given line (CMBS_ALL_RELEVANT_HS_ID)

********************************************/
void ListChangeNotif_MissedCallListChanged(IN u32 u32_LineId, IN bool bNewEntryAdded, IN u16 u16_HsId)
{
    u32 u32_HandsetMask, u32_NumOfRead, u32_NumOfUnread;

    //printf("ListChangeNotif_MissedCallListChanged LineID %d u16_HsId %d \n",u32_LineId, u16_HsId );

    List_GetAttachedHs(u32_LineId, &u32_HandsetMask);

    printf("ListChangeNotif_MissedCallListChanged u32_HandsetMask1 %x \n", u32_HandsetMask);

    // if only one HS has to be notified, mask other handsets:
    if (u16_HsId != CMBS_ALL_RELEVANT_HS_ID)
    {
        u32_HandsetMask &= (1 << (u16_HsId - 1));
    }

    //printf("ListChangeNotif_MissedCallListChanged u32_HandsetMask2 %x \n",u32_HandsetMask );

    if (u32_HandsetMask)
    {
        List_GetMissedCallsNumOfEntries(u32_LineId, &u32_NumOfRead, &u32_NumOfUnread);

        cmbs_dsr_gen_SendMissedCalls(g_cmbsappl.pv_CMBSRef, 0, u32_LineId, u32_HandsetMask, u32_NumOfUnread, bNewEntryAdded, u32_NumOfUnread + u32_NumOfRead);
    }
}

void ListChangeNotif_ListChanged(IN u32 u32_LineId, IN eLineType tLineType, IN u32 u32_HandsetMask,
                                 IN u32 u32_TotalNumOfEntries, IN eLIST_CHANGE_NOTIF_LIST_TYPE tListId)
{
    cmbs_dsr_gen_SendListChanged(g_cmbsappl.pv_CMBSRef, 0, u32_HandsetMask, tListId, u32_TotalNumOfEntries, u32_LineId, tLineType);
}

/* End Of File *****************************************************************************************************************************/

