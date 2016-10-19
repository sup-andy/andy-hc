#include "cmbs_event.h"

#define ARGUSED(x) ((void)(x))

char dbg[5000] = {0};

/*
* sends debug info
*/
void cmbsevent_printf(char* ss)
{
    ARGUSED(ss);
}

/*
* clear HS list information
*/
void cmbsevent_clearHsList()
{

}

/*
* Add HS to the list
*/
void cmbsevent_addHs2List(int handsetNumber, const char* name)
{
    ARGUSED(handsetNumber);
    ARGUSED(name);
}

/*
* Handset registered event
*/
void cmbsevent_OnHandsetRegistered(int handsetNumber)
{
    ARGUSED(handsetNumber);
}

/*
* Handset deleted event
*/
void cmbsevent_OnHandsetDeleted(int handsetNumber)
{
    ARGUSED(handsetNumber);
}

/*
* Handset In range event
*/
void cmbsevent_OnHandsetInRange(int handsetNumber)
{
    ARGUSED(handsetNumber);
}

/*
* Handset link released
*/
void cmbsevent_OnHandsetLinkReleased(int handsetNumber)
{
    ARGUSED(handsetNumber);
}


/*
* Handset registration opened
*/
void cmbsevent_OnRegistrationOpened()
{

}

/*
* Handset registration closed
*/
void cmbsevent_OnRegistrationClosed()
{

}

/*
* Handset page opened
*/
void cmbsevent_OnPageStarted()
{

}

/*
* Handset page closed
*/
void cmbsevent_OnPageStoped()
{

}

/*
* Handset list was updated
*/
void cmbsevent_OnHSListUpdated()
{

}

/*
* VoIP line connection status changed
*/
void cmbsevent_onLineStatus(int status)
{
    ARGUSED(status);
}

/*
* Contact list was synchronized with external VoIP
*/
void cmbsevent_onSyncContacts(int completed)
{
    ARGUSED(completed);
}

/*
* Call established and started
*/
void cmbsevent_onCallStarted(int callID, int isIncoming)
{
    ARGUSED(callID);
    ARGUSED(isIncoming);
}

/*
* Call finished
*/
void cmbsevent_onCallFinished(int callID)
{
    ARGUSED(callID);
}

/*
* Contact list was changed
*/
void cmbsevent_onContactListUpdated()
{

}

void cmbsevent_onCallListUpdated()
{

}

void cmbsevent_onSwUpdateProgress(int completed)
{
    ARGUSED(completed);
}

void cmbsevent_onSwUpdateCompleted()
{

}

void cmbsevent_onCallEstablishing(int callID)
{
    ARGUSED(callID);
}

void cmbsevent_onCallAnswered(int callID)
{
    ARGUSED(callID);
}

void cmbsevent_onCallReleased(int callID)
{
    ARGUSED(callID);
}
