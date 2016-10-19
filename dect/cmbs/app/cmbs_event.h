/*
* A proxy file between CMBS API (on C) and hi-layer
* Used to send events notifications from CMBS API
*/

#if !defined( CMBS_EVENT_H )
#define CMBS_EVENT_H

#if defined( __cplusplus )
extern "C"
{
#endif

// for debug
void cmbsevent_printf(char* str);

// handsets information
void cmbsevent_clearHsList();
void cmbsevent_addHs2List(int handsetNumber, const char* name);
void cmbsevent_OnHSListUpdated();

// events
void cmbsevent_OnHandsetRegistered(int handsetNumber);
void cmbsevent_OnHandsetDeleted(int handsetNumber);
void cmbsevent_OnHandsetInRange(int handsetNumber);
void cmbsevent_OnHandsetLinkReleased(int handsetNumber);
void cmbsevent_OnRegistrationOpened();
void cmbsevent_OnRegistrationClosed();
void cmbsevent_OnPageStarted();
void cmbsevent_OnPageStoped();
void cmbsevent_onLineStatus(int status);
void cmbsevent_onSyncContacts(int completed);
void cmbsevent_onSwUpdateProgress(int completed);
void cmbsevent_onSwUpdateCompleted();

void cmbsevent_onCallStarted(int callID, int isIncoming);
void cmbsevent_onCallFinished(int callID);
void cmbsevent_onCallEstablishing(int callID);
void cmbsevent_onCallAnswered(int callID);
void cmbsevent_onCallReleased(int callID);

void cmbsevent_onContactListUpdated();

void cmbsevent_onCallListUpdated();

#if defined( __cplusplus )
}
#endif

#endif
