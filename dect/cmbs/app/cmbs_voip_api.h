/*!
*  \file       voip_api.h
*  \brief
*  \Author     andriig
*
*  @(#)  %filespec: cmbs_voip_api.h~5 %
*
*  This API is provided for DEMO purposes
*
*******************************************************************************
*  \par  History
*  \n==== History ============================================================\n
*  date        name     version   action                                          \n
*  ----------------------------------------------------------------------------\n
*******************************************************************************/

#if   !defined( CMBS_VOIP_API_H )
#define  CMBS_VOIP_API_H

#include <stdio.h>

#if defined( __cplusplus )
extern "C"
{
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif


/*
 * Common enumerations
 */

typedef enum
{
    EXTVOIP_CODEC_UNKNOWN,
    EXTVOIP_CODEC_PCMU,
    EXTVOIP_CODEC_PCMA,
    EXTVOIP_CODEC_PCMU_WB,
    EXTVOIP_CODEC_PCMA_WB,
    EXTVOIP_CODEC_PCM_LINEAR_WB,
    EXTVOIP_CODEC_PCM_LINEAR_NB,
    EXTVOIP_CODEC_PCM8,
    EXTVOIP_CODEC_MAX
}
EXTVOIP_CODEC;

typedef enum
{
    EXTVOIP_REASON_NORMAL,          /*!< Normal reason*/
    EXTVOIP_REASON_ABNORMAL,        /*!< Any not specified reason */
    EXTVOIP_REASON_BUSY,            /*!< Destination side is busy */
    EXTVOIP_REASON_UNKNOWN_NUMBER,  /*!< Destination is unknown */
    EXTVOIP_REASON_FORBIDDEN,       /*!< Network access denied */
    EXTVOIP_REASON_UNSUPPORTED_MEDIA,/*!< The requested media is not supported */
    EXTVOIP_REASON_NO_RESOURCE,     /*!< No internal resources are available */
    EXTVOIP_REASON_REDIRECT,   /*Redirecting the call to a different destination*/
    EXTVOIP_REASON_MAX
} EXTVOIP_RELEASE_REASON;

typedef enum
{
    EXTVOIP_RC_OK,
    EXTVOIP_RC_FAIL
} EXTVOIP_RC;

typedef enum
{
    EXTVOIP_LINE_STATUS_DISCONNECTED,
    EXTVOIP_LINE_STATUS_CONNECTING,
    EXTVOIP_LINE_STATUS_CONNECTED
} EXTVOIP_LINE_STATUS;


/*
 * Types definitions: Initial VoIP functions
 */

// Init instance
typedef EXTVOIP_RC(*voipapp_InitFunc)();

// Unload instance
typedef EXTVOIP_RC(*voipapp_UnloadFunc)();

/*
 * Types definitions: Call control VoIP functions
 */

// Make a call
typedef EXTVOIP_RC(*voipapp_SetupCallFunc)(IN int cmbsCallID, IN const char* calledID, IN int callerHandset, IN int lineID, IN EXTVOIP_CODEC* codecList, IN int codecListLength);

// Proceeding
typedef EXTVOIP_RC(*voipapp_ProceedingCallFunc)(IN int cmbsCallID);

// Alerting
typedef EXTVOIP_RC(*voipapp_AlertingCallFunc)(IN int cmbsCallID);

// Answer call
typedef EXTVOIP_RC(*voipapp_AnswerCallFunc)(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// Release active call
typedef EXTVOIP_RC(*voipapp_DisconnectCallFunc)(IN int cmbsCallID, IN EXTVOIP_RELEASE_REASON disconnectReason);

// Release active call ack
typedef EXTVOIP_RC(*voipapp_DisconnectCallDoneFunc)(IN int cmbsCallID);

// Hold call
typedef EXTVOIP_RC(*voipapp_HoldCallFunc)(IN int cmbsCallID);

// Hold call ack
typedef EXTVOIP_RC(*voipapp_HoldCallAckFunc)(IN int cmbsCallID);

// Resume call
typedef EXTVOIP_RC(*voipapp_ResumeCallFunc)(IN int cmbsCallID);

// Resume call ack
typedef EXTVOIP_RC(*voipapp_ResumeCallAckFunc)(IN int cmbsCallID);

// Conference call
typedef EXTVOIP_RC(*voipapp_ConferenceCallFunc)(IN int cmbsCallID, IN int cmbsCallID2);

// Change codec
typedef EXTVOIP_RC(*voipapp_MediaChangeFunc)(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// Change codec ack
typedef EXTVOIP_RC(*voipapp_MediaChangeAckFunc)(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// Send digits
typedef EXTVOIP_RC(*voipapp_SendDigitsCallFunc)(IN int cmbsCallID, IN const char* digits);



/*
 * Types definitions: Callbacks
 */

// Type definition for incoming call callback function
typedef EXTVOIP_RC(*voipapp_SetupCallCallbackFunc)(IN int calledHandset, IN const char* callerNumber, IN const char* callerName, IN EXTVOIP_CODEC* codecList, IN int codecListLength, OUT int* cmbsCallID);

// Type definition for proceeding callback function
typedef EXTVOIP_RC(*voipapp_ProceedingCallCallbackFunc)(IN int cmbsCallID);

// Type definition for alerting callback function
typedef EXTVOIP_RC(*voipapp_AlertingCallCallbackFunc)(IN int cmbsCallID);

// Type definition for answer call callback function
typedef EXTVOIP_RC(*voipapp_AnswerCallCallbackFunc)(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// Type definition for disconnect call callback function
typedef EXTVOIP_RC(*voipapp_DisconnectCallCallbackFunc)(IN int cmbsCallID, IN EXTVOIP_RELEASE_REASON releaseReason);

// Type definition for disconnect call done callback function
typedef EXTVOIP_RC(*voipapp_DisconnectDoneCallbackFunc)(IN int cmbsCallID);

// Type definition for hold call callback function
typedef EXTVOIP_RC(*voipapp_HoldCallCallbackFunc)(IN int cmbsCallID);

// Type definition for hold call ack callback function
typedef EXTVOIP_RC(*voipapp_HoldCallAckCallbackFunc)(IN int cmbsCallID);

// Type definition for resume call callback function
typedef EXTVOIP_RC(*voipapp_ResumeCallCallbackFunc)(IN int cmbsCallID);

// Type definition for resume call ack callback function
typedef EXTVOIP_RC(*voipapp_ResumeCallAckCallbackFunc)(IN int cmbsCallID);

// Type definition for codec change callback function
typedef EXTVOIP_RC(*voipapp_MediaChangeCallbackFunc)(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// Type definition for codec change ack callback function
typedef EXTVOIP_RC(*voipapp_MediaChangeAckCallbackFunc)(IN int cmbsCallID, IN EXTVOIP_CODEC codec);

// Type definition for send digits callback function
typedef EXTVOIP_RC(*voipapp_SendDigitsCallbackFunc)(IN int cmbsCallID, IN const char* digits);

// Type definition for line status callback function
typedef EXTVOIP_RC(*voipapp_LineStatusCallbackFunc)(IN EXTVOIP_LINE_STATUS status);


/*
 * Helpers
 */

// Retrieve call caller id
typedef EXTVOIP_RC(*voipapp_GetCallerIDFunc)(IN int cmbsCallID, OUT char* callerID);

// Retrieve call called id
typedef EXTVOIP_RC(*voipapp_GetCalledIDFunc)(IN int cmbsCallID, OUT char* calledID);


/*
 * Audio config
 */

// Set audio channel
typedef EXTVOIP_RC(*voipapp_SetAudioChannelFunc)(IN int cmbsCallID, IN int mediaChannelID);


/*
 * Callbacks registration
 */

// Register OnSetupCall callback
typedef EXTVOIP_RC(*voipapp_RegisterOnSetupCallCallbackFunc)(IN voipapp_SetupCallCallbackFunc func);

// Register OnAlerting callback
typedef EXTVOIP_RC(*voipapp_RegisterOnAlertingCallbackFunc)(IN voipapp_AlertingCallCallbackFunc func);

// Register OnProceeding callback
typedef EXTVOIP_RC(*voipapp_RegisterOnProceedingCallbackFunc)(IN voipapp_ProceedingCallCallbackFunc func);

// Register OnAnswerCall callback
typedef EXTVOIP_RC(*voipapp_RegisterOnAnswerCallCallbackFunc)(IN voipapp_AnswerCallCallbackFunc func);

// Register OnDisconnectCall callback
typedef EXTVOIP_RC(*voipapp_RegisterOnDisconnectCallCallbackFunc)(IN voipapp_DisconnectCallCallbackFunc func);

// Register OnDisconnectCallDone callback
typedef EXTVOIP_RC(*voipapp_RegisterOnDisconnectCallDoneCallbackFunc)(IN voipapp_DisconnectDoneCallbackFunc func);

// Register OnHoldCall callback
typedef EXTVOIP_RC(*voipapp_RegisterOnHoldCallCallbackFunc)(IN voipapp_HoldCallCallbackFunc func);

// Register OnHoldCallAck callback
typedef EXTVOIP_RC(*voipapp_RegisterOnHoldCallAckCallbackFunc)(IN voipapp_HoldCallAckCallbackFunc func);

// Register OnResumeCall callback
typedef EXTVOIP_RC(*voipapp_RegisterOnResumeCallCallbackFunc)(IN voipapp_ResumeCallCallbackFunc func);

// Register OnResumeCallAck callback
typedef EXTVOIP_RC(*voipapp_RegisterOnResumeCallAckCallbackFunc)(IN voipapp_ResumeCallAckCallbackFunc func);

// Register OnMediaChange callback
typedef EXTVOIP_RC(*voipapp_RegisterOnMediaChangeCallbackFunc)(IN voipapp_MediaChangeCallbackFunc func);

// Register OnMediaChangeAck callback
typedef EXTVOIP_RC(*voipapp_RegisterOnMediaChangeAckCallbackFunc)(IN voipapp_MediaChangeAckCallbackFunc func);

// Register OnSendDigits callback
typedef EXTVOIP_RC(*voipapp_RegisterOnSendDigitsCallbackFunc)(IN voipapp_SendDigitsCallbackFunc func);

// Register OnLineStatus callback
typedef EXTVOIP_RC(*voipapp_RegisterOnLineStatusCallbackFunc)(IN voipapp_LineStatusCallbackFunc func);


/*
 * Contacts sync
 */

// Retrieve contacts size
typedef int (*voipapp_GetContactsCountFunc)();

// Retrieve contact entry
typedef EXTVOIP_RC(*voipapp_GetContactEntryFunc)(IN int itemid, OUT char* firstName, OUT char* lastName, OUT char* number, OUT int* isOnline);


#if defined( __cplusplus )
}
#endif

#endif   // CMBS_VOIP_API_H
//*/
