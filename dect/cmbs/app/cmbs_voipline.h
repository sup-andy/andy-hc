/*!
*  \file       cmbs_voipline.h
*  \brief
*  \Author     andriig
*
*  @(#)  %filespec: cmbs_voipline.h~8 %
*
*  This API is provided for DEMO purposes
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
*******************************************************************************/

#if   !defined( CMBS_VOIPLINE_H )
#define  CMBS_VOIPLINE_H

#include "cmbs_voip_api.h"
#include "ListsApp.h"
#include "appcall.h"
#include "cmbs_event.h"

/*
 * VoIP Line structure
 * Has pointers to a correspond functions implementation in VoIP application
 */

typedef struct ExtVoipLine
{
    // line ID
    int LineID;

    char LineName[LIST_NAME_MAX_LEN];

    // function for call setup
    voipapp_SetupCallFunc   fncSetupCall;
    // function for call proceeding
    voipapp_ProceedingCallFunc  fncProceedingCall;
    // function for call proceeding
    voipapp_AlertingCallFunc  fncAlertingCall;
    // function for call answer
    voipapp_AnswerCallFunc   fncAnswerCall;
    // function for call disconnect
    voipapp_DisconnectCallFunc  fncDisconnectCall;
    // function for call disconnect done
    voipapp_DisconnectCallDoneFunc fncDisconnectCallDone;
    // function for call hold
    voipapp_HoldCallFunc   fncHoldCall;
    // function for call hold acknowledge
    voipapp_HoldCallAckFunc   fncHoldCallDone;
    // function for call resume
    voipapp_ResumeCallFunc   fncResumeCall;
    // function for call conference
    voipapp_ConferenceCallFunc  fncConferenceCall;
    // function for call resume acknowledge
    voipapp_ResumeCallAckFunc  fncResumeCallDone;
    // function for call codec change
    voipapp_MediaChangeFunc   fncMediaChange;
    // function for call codec change acknowledge
    voipapp_MediaChangeAckFunc  fncMediaChangeAck;
    // function to send digits
    voipapp_SendDigitsCallFunc  fncSendDigits;
    // function to retrieve caller ID
    voipapp_GetCallerIDFunc   fncGetCallerID;
    // function to retrieve called ID
    voipapp_GetCalledIDFunc   fncGetCalledID;
    // function to set audio channel
    voipapp_SetAudioChannelFunc  fncSetAudioChannel;
    // function to retrieve cotacts size
    voipapp_GetContactsCountFunc fncContactsCount;
    // function to retrieve contact entry
    voipapp_GetContactEntryFunc  fncContactsEntry;

} ST_EXTVOIPLINE;

/*
 * Common functions to register/unregister VoIP Applications and supported lines
 */

// Initializes voip applications array
void extvoip_InitVoiplines();

// Registers VoIP application on desired lineID
EXTVOIP_RC extvoip_RegisterVoipline(int lineID, ST_EXTVOIPLINE* voipline);

// Unregisters VoIP application
EXTVOIP_RC extvoip_UnRegisterVoipline(ST_EXTVOIPLINE* voipline);

/*
 * Helpers
 */

// Retrieves the correspond VoIP application using lineID
ST_EXTVOIPLINE* extvoip_GetVoIPAllicationByLineID(int LineID);

// Retrieves the correspond VoIP application using callID
ST_EXTVOIPLINE* extvoip_GetVoIPAllicationByCallID(int cmbs_callID);

/*
 * VoIP application functions
 */

// Make VoIP call
void extvoip_MakeCall(u32 cmbs_callID);

// Proceeding VoIP call
void extvoip_ProceedingCall(u32 cmbs_callID);

// Alerting VoIP call
void extvoip_AlertingCall(u32 cmbs_callID);

// Answer VoIP call
void extvoip_AnswerCall(u32 cmbs_callID);

// Release VoIP call
void extvoip_DisconnectCall(u32 cmbs_callID);

// Release Ack VoIP call
void extvoip_DisconnectDoneCall(u32 cmbs_callID);

// Hold VoIP call
void extvoip_HoldCall(u32 cmbs_callID);

// Hold ack VoIP call
void extvoip_HoldAckCall(u32 cmbs_callID);

// Resume VoIP call
void extvoip_ResumeCall(u32 cmbs_callID);

// Conference VoIP call
void extvoip_ConferenceCall(u32 cmbs_callID, u32 cmbs_callID2);

// Resume Ack VoIP call
void extvoip_ResumeAckCall(u32 cmbs_callID);

// Media change for VoIP call
void extvoip_MediaChange(u32 cmbs_callID, E_CMBS_AUDIO_CODEC codec);

// Media change acknowledge for VoIP call
void extvoip_MediaAckChange(u32 cmbs_callID, E_CMBS_AUDIO_CODEC codec);

// Send digits for VoIP call
void extvoip_SendDigits(u32 cmbs_callID, const char* digits);

/*
 * VoIP Callbacks
 */

// On Incoming call
EXTVOIP_RC extvoip_OnSetupCall(IN int lineID, IN int calledHandset, IN const char* callerNumber, IN const char* callerName, IN EXTVOIP_CODEC* codecList, IN int codecListLength, OUT int* cmbsCallID);

// On call proceeding
EXTVOIP_RC extvoip_OnProceedingCall(IN int cmbsCallID);

// On call alerting
EXTVOIP_RC extvoip_OnAlertingCall(IN int cmbsCallID);

// On call answered
EXTVOIP_RC extvoip_OnAnswerCall(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// On call released
EXTVOIP_RC extvoip_OnDisconnectCall(IN int cmbsCallID, IN EXTVOIP_RELEASE_REASON releaseReason);

// On call release done
EXTVOIP_RC extvoip_OnDisconnectCallDone(IN int cmbsCallID);

// On call hold
EXTVOIP_RC extvoip_OnHoldCall(IN int cmbsCallID);

// On call hold ack
EXTVOIP_RC extvoip_OnHoldCallAck(IN int cmbsCallID);

// On call resume
EXTVOIP_RC extvoip_OnResumeCall(IN int cmbsCallID);

// On call resume ack
EXTVOIP_RC extvoip_OnResumeCallAck(IN int cmbsCallID);

// On media change
EXTVOIP_RC extvoip_OnMediaChange(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// On media change ack
EXTVOIP_RC extvoip_OnMediaChangeAck(IN int cmbsCallID);

// On send digits
EXTVOIP_RC extvoip_OnSendDigits(IN int cmbsCallID, IN const char* digits);

// On line status
EXTVOIP_RC voipapp_OnLineStatus(IN EXTVOIP_LINE_STATUS status);

// Sync contacts
void extvoip_SyncContacts(ST_EXTVOIPLINE* voipline);

void extvoip_DumpContacts();

// helpers

// convert cmbs audio codec to voip codec
EXTVOIP_CODEC cmbsCodec2VoipCodec(E_CMBS_AUDIO_CODEC codec);

// convert voip audio codec to cmbs codec
E_CMBS_AUDIO_CODEC voipCodec2CmbsCodec(EXTVOIP_CODEC codec);

// Convert voip release reason to cmbs release reason
E_CMBS_REL_REASON voipReleaseReason2CmbsReleaseReason(EXTVOIP_RELEASE_REASON val);

// update Line data in LineSettingsList
void _extvoip_updateLineSettingsList(ST_EXTVOIPLINE* voipline);

#endif   // CMBS_VOIPLINE_H
//*/
