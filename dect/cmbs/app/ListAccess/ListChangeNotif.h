/*************************************************************************************************************
*** List Change Notification
**
**************************************************************************************************************/
#ifndef __LA_LIST_CHANGE_NOTIF_H__
#define __LA_LIST_CHANGE_NOTIF_H__


/*******************************************
Includes
********************************************/
#include "cmbs_api.h"

/*******************************************
Defines
********************************************/

/*******************************************
Types
********************************************/
typedef enum
{
    LINE_TYPE_EXTERNAL      = 0x00,
    LINE_TYPE_RELATING_TO   = 0x03,
    LINE_TYPE_ALL_LINES     = 0x04
} eLineType;

typedef enum
{
    LIST_CHANGE_NOTIF_LIST_TYPE_MISSED_CALLS = 1,
    LIST_CHANGE_NOTIF_LIST_TYPE_OUTGOING_CALLS,
    LIST_CHANGE_NOTIF_LIST_TYPE_INCOMING_ACCEPTED_CALLS,
    LIST_CHANGE_NOTIF_LIST_TYPE_ALL_CALLS,
    LIST_CHANGE_NOTIF_LIST_TYPE_CONTACT_LIST,
    LIST_CHANGE_NOTIF_LIST_TYPE_INTERNAL_NAMES_LIST,
    LIST_CHANGE_NOTIF_LIST_TYPE_DECT_SETTINGS_LIST,
    LIST_CHANGE_NOTIF_LIST_TYPE_LINE_SETTINGS_LIST,
    LIST_CHANGE_NOTIF_LIST_TYPE_ALL_INCOMING_CALLS,

    LIST_CHANGE_NOTIF_LIST_TYPE_MAX

} eLIST_CHANGE_NOTIF_LIST_TYPE;

/*******************************************
Globals
********************************************/

/*******************************************
List Change Notification API
********************************************/
void ListChangeNotif_MissedCallListChanged(IN u32 u32_LineId, IN bool bNewEntryAdded, IN u16 u16_HsId);

void ListChangeNotif_ListChanged(IN u32 u32_LineId, IN eLineType tLineType, IN u32 u32_HandsetMask,
                                 IN u32 u32_TotalNumOfEntries, IN eLIST_CHANGE_NOTIF_LIST_TYPE tListId);


#endif /* __LA_LIST_CHANGE_NOTIF_H__ */

/* End Of File *****************************************************************************************************************************/

