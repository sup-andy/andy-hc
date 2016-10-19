/* == IDENTIFICATION ==================================================
*
* Copyright (C) 2010, DSP Group
*
* System           : Vega Family
* Component        : FT
* Module           : MLP
* Unit             : FTMLP
* File             : bhldbg.c
*
*  @(#)   %filespec: chl18msg.c~2 %
*
*/

/* == HISTORY ========================================================= */
/*
* Name     Date        Ver     Action
* --------------------------------------------------------------------
* andriig  04-May-2011 4      Implemented List Access parsing
* bhkori   07-Apr-2011 3      Put Andriis New changes
* tcmc_asa 22-Sep-2010 2      bring to Octopus
* andriig  16-Sep-2010 1      First version
*
*/

//#include "cg0type.h"
//#include "cap03def.h"


#include <stdio.h>
#include <string.h>
#include "cmbs_chl18msg.h"
#include "cmbs_api.h"


#define DECT_DBG_LIST_VALUE(list, index) (sizeof(list)/sizeof((list)[0]) <= index ? "Unknown" : (list)[index])

#define DECT_DBG_MAX_LA_SESSIONS 0x10
#define DECT_DBG_MAX_LA_MAX_DATA_LENGTH 0xFF

typedef struct
{
    u8 session_id;
    u8 list_id;
    u8 handsetNumber;
    u8 data[DECT_DBG_MAX_LA_MAX_DATA_LENGTH];
    u32 data_length;
} LA_Session;



LA_Session u8_hl18_LA_Sessions[DECT_DBG_MAX_LA_SESSIONS];

void p_hl18_LA_Session_Init()
{
    u8 i = 0;
    for (i = 0; i < DECT_DBG_MAX_LA_SESSIONS; i++)
    {
        u8_hl18_LA_Sessions[i].session_id = 0;
        u8_hl18_LA_Sessions[i].list_id = 0;
        u8_hl18_LA_Sessions[i].data_length = 0;
    }
}

void p_hl18_LA_Session_Add(u8 session_id, u8 list_id, u8 handsetNumber)
{
    u8 i = 0;
    for (i = 0; i < DECT_DBG_MAX_LA_SESSIONS; i++)
    {
        if (u8_hl18_LA_Sessions[i].session_id == 0)
        {
            u8_hl18_LA_Sessions[i].session_id = session_id;
            u8_hl18_LA_Sessions[i].list_id = list_id;
            u8_hl18_LA_Sessions[i].handsetNumber = handsetNumber;
            u8_hl18_LA_Sessions[i].data_length = 0;
            return;
        }
    }
    printf("DECT-DBG WARNING: Can't add session to the list\n");
}

u8 p_hl18_LA_Session_GetListID(u8 session_id)
{
    u8 i = 0;
    for (i = 0; i < DECT_DBG_MAX_LA_SESSIONS; i++)
    {
        if (u8_hl18_LA_Sessions[i].session_id == session_id)
            return u8_hl18_LA_Sessions[i].list_id;
    }
    //printf("DECT-DBG WARNING: Unknown session ID: 0x%x\n", session_id);
    return 0;
}

void p_hl18_LA_Session_Delete(u8 session_id)
{
    u8 i = 0;
    for (i = 0; i < DECT_DBG_MAX_LA_SESSIONS; i++)
    {
        if (u8_hl18_LA_Sessions[i].session_id == session_id)
        {
            u8_hl18_LA_Sessions[i].session_id = 0;
            u8_hl18_LA_Sessions[i].data_length = 0;
            u8_hl18_LA_Sessions[i].handsetNumber = 0;
            break;
        }
    }
}

void p_hl18_LA_Session_DeleteByHandset(u8 handsetNumber)
{
    u8 i = 0;
    for (i = 0; i < DECT_DBG_MAX_LA_SESSIONS; i++)
    {
        if (u8_hl18_LA_Sessions[i].handsetNumber == handsetNumber)
        {
            u8_hl18_LA_Sessions[i].session_id = 0;
            u8_hl18_LA_Sessions[i].data_length = 0;
            u8_hl18_LA_Sessions[i].handsetNumber = 0;
        }
    }
}

LA_Session* p_hl18_LA_Session_GetSession(u8 session_id)
{
    u8 i = 0;
    for (i = 0; i < DECT_DBG_MAX_LA_SESSIONS; i++)
    {
        if (u8_hl18_LA_Sessions[i].session_id == session_id)
        {
            return &u8_hl18_LA_Sessions[i];
        }
    }
    return NULL;
}

void p_hl18_LA_Session_Data(u8 session_id, u8* data, u8 length)
{
    LA_Session* obj = p_hl18_LA_Session_GetSession(session_id);
    if (!obj)
    {
        printf("DECT-DBG WARNING: (u8_hl18_LA_Session_Data) Unknown session ID: 0x%x\n", session_id);
        return;
    }

    if (obj->data_length + length >= DECT_DBG_MAX_LA_MAX_DATA_LENGTH)
    {
        printf("DECT-DBG WARNING: (u8_hl18_LA_Session_Data) Too much data, buffer is not enough. Session ID: 0x%x\n", session_id);
        // mark buffer as corrupted
        obj->data_length = DECT_DBG_MAX_LA_MAX_DATA_LENGTH;
        return;
    }

    memcpy(obj->data + obj->data_length, data, length);
    obj->data_length += length;
}

typedef enum
{
    DECT_DBG_LA_LIST_SUPPORTED_LISTS = 0x0,
    DECT_DBG_LA_LIST_MISSED_CALLS = 0x1,
    DECT_DBG_LA_LIST_OUTGOING_CALLS = 0x2,
    DECT_DBG_LA_LIST_INCOMING_CALLS = 0x3,
    DECT_DBG_LA_LIST_ALL_CALLS = 0x4,
    DECT_DBG_LA_LIST_CONTACT_LIST = 0x5,
    DECT_DBG_LA_LIST_INTERNAL_NAMES = 0x6,
    DECT_DBG_LA_LIST_DECT_SYSTEM_SETTINGS = 0x7,
    DECT_DBG_LA_LIST_LINE_SETTINGS_LIST = 0x8,
    DECT_DBG_LA_LIST_ALL_INCOMING_CALLS = 0x9
} DECT_DBG_LA_LIST;

typedef enum
{
    DECT_DBG_FIELD_TYPE_UNKNOWN,
    DECT_DBG_FIELD_TYPE_NUMBER,
    DECT_DBG_FIELD_TYPE_NAME,
    DECT_DBG_FIELD_TYPE_INTERCEPTION,
    DECT_DBG_FIELD_TYPE_MELODY,
    DECT_DBG_FIELD_TYPE_LINE_ID,
    DECT_DBG_FIELD_TYPE_LINE_NAME,
    DECT_DBG_FIELD_TYPE_FIRST_NAME,
    DECT_DBG_FIELD_TYPE_DATETIME,
    DECT_DBG_FIELD_TYPE_NUMBER_OF_CALLS,
    DECT_DBG_FIELD_TYPE_CALL_TYPE,
    DECT_DBG_FIELD_TYPE_NEW,
    DECT_DBG_FIELD_TYPE_CONTACT_NUMBER,
    DECT_DBG_FIELD_TYPE_PIN_CODE,
    DECT_DBG_FIELD_TYPE_CLOCK_MASTER,
    DECT_DBG_FIELD_TYPE_BASE_RESET,
    DECT_DBG_FIELD_TYPE_IP_TYPE,
    DECT_DBG_FIELD_TYPE_IP_VALUE,
    DECT_DBG_FIELD_TYPE_IP_SUBNET_MASK,
    DECT_DBG_FIELD_TYPE_IP_GATEWAY,
    DECT_DBG_FIELD_TYPE_IP_DNS_SERVER,
    DECT_DBG_FIELD_TYPE_FW_VERSION,
    DECT_DBG_FIELD_TYPE_EEPROM_VERSION,
    DECT_DBG_FIELD_TYPE_HW_VERSION,
    DECT_DBG_FIELD_TYPE_ATTACHED_HANDSETS,
    DECT_DBG_FIELD_TYPE_DIAL_PREFIX,
    DECT_DBG_FIELD_TYPE_FP_MELODY,
    DECT_DBG_FIELD_TYPE_FP_VOLUME,
    DECT_DBG_FIELD_TYPE_BLOCKED_NUMBER,
    DECT_DBG_FIELD_TYPE_MULTICALL_MODE,
    DECT_DBG_FIELD_TYPE_INTRUSION_CALL,
    DECT_DBG_FIELD_TYPE_PERMANENT_CLIR,
    DECT_DBG_FIELD_TYPE_CFUN,
    DECT_DBG_FIELD_TYPE_CFNA,
    DECT_DBG_FIELD_TYPE_CFBS,
    DECT_DBG_FIELD_TYPE_NEMO_MODE,
    DECT_DBG_FIELD_TYPE_NEW_PIN_CODE,
    DECT_DBG_FIELD_TYPE_MAX
} DECT_DBG_FIELD_TYPES;

DECT_DBG_FIELD_TYPES p_hl18_LA_GetFieldType(u8 session_id, u8 fieldID)
{
    u8 listId = p_hl18_LA_Session_GetListID(session_id);

    switch (listId)
    {
        case DECT_DBG_LA_LIST_SUPPORTED_LISTS:
            break;
        case DECT_DBG_LA_LIST_MISSED_CALLS:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_NUMBER;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_DATETIME;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_NEW;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_LINE_NAME;
                case 0x6:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
                case 0x7:
                    return DECT_DBG_FIELD_TYPE_NUMBER_OF_CALLS;
            }
            break;
        case DECT_DBG_LA_LIST_OUTGOING_CALLS:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_NUMBER;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_DATETIME;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_LINE_NAME;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
            }
            break;
        case DECT_DBG_LA_LIST_INCOMING_CALLS:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_NUMBER;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_DATETIME;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_LINE_NAME;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
            }
            break;
        case DECT_DBG_LA_LIST_ALL_CALLS:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_CALL_TYPE;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_NUMBER;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_DATETIME;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_LINE_NAME;
                case 0x6:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
            }
            break;
        case DECT_DBG_LA_LIST_CONTACT_LIST:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_FIRST_NAME;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_CONTACT_NUMBER;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_MELODY;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
            }
            break;
        case DECT_DBG_LA_LIST_INTERNAL_NAMES:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_NUMBER;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_INTERCEPTION;
            }
            break;
        case DECT_DBG_LA_LIST_DECT_SYSTEM_SETTINGS:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_PIN_CODE;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_CLOCK_MASTER;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_BASE_RESET;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_IP_TYPE;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_IP_VALUE;
                case 0x6:
                    return DECT_DBG_FIELD_TYPE_IP_SUBNET_MASK;
                case 0x7:
                    return DECT_DBG_FIELD_TYPE_IP_GATEWAY;
                case 0x8:
                    return DECT_DBG_FIELD_TYPE_IP_DNS_SERVER;
                case 0x9:
                    return DECT_DBG_FIELD_TYPE_FW_VERSION;
                case 0xA:
                    return DECT_DBG_FIELD_TYPE_EEPROM_VERSION;
                case 0xB:
                    return DECT_DBG_FIELD_TYPE_HW_VERSION;
                case 0xC:
                    return DECT_DBG_FIELD_TYPE_NEMO_MODE;
                case 0xD:
                    return DECT_DBG_FIELD_TYPE_NEW_PIN_CODE;
            }
            break;
        case DECT_DBG_LA_LIST_LINE_SETTINGS_LIST:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_LINE_NAME;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_ATTACHED_HANDSETS;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_DIAL_PREFIX;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_FP_MELODY;
                case 0x6:
                    return DECT_DBG_FIELD_TYPE_FP_VOLUME;
                case 0x7:
                    return DECT_DBG_FIELD_TYPE_BLOCKED_NUMBER;
                case 0x8:
                    return DECT_DBG_FIELD_TYPE_MULTICALL_MODE;
                case 0x9:
                    return DECT_DBG_FIELD_TYPE_INTRUSION_CALL;
                case 0xA:
                    return DECT_DBG_FIELD_TYPE_PERMANENT_CLIR;
                case 0xB:
                    return DECT_DBG_FIELD_TYPE_CFUN;
                case 0xC:
                    return DECT_DBG_FIELD_TYPE_CFNA;
                case 0xD:
                    return DECT_DBG_FIELD_TYPE_CFBS;
            }
            break;
        case DECT_DBG_LA_LIST_ALL_INCOMING_CALLS:
            switch (fieldID)
            {
                case 0x1:
                    return DECT_DBG_FIELD_TYPE_NUMBER;
                case 0x2:
                    return DECT_DBG_FIELD_TYPE_NAME;
                case 0x3:
                    return DECT_DBG_FIELD_TYPE_DATETIME;
                case 0x4:
                    return DECT_DBG_FIELD_TYPE_NEW;
                case 0x5:
                    return DECT_DBG_FIELD_TYPE_LINE_NAME;
                case 0x6:
                    return DECT_DBG_FIELD_TYPE_LINE_ID;
                case 0x7:
                    return DECT_DBG_FIELD_TYPE_NUMBER_OF_CALLS;
            }
            break;
    }

    return DECT_DBG_FIELD_TYPE_UNKNOWN;
}

const char* u8_hl18_LAListIdentifier[] =
{
    "List of supported lists",
    "Missed calls list",
    "Outgoing calls list",
    "Incoming accepted calls list",
    "All calls list",
    "Contact list",
    "Internal names list",
    "DECT system settings list",
    "Line settings list",
    "All incoming calls list"
};

const char* u8_hl18_LAFieldsSupportedLists[] =
{
    "List identifiers"
};

const char* u8_hl18_LAFieldsContact[] =
{
    "Name",
    "First name",
    "Contact number",
    "Associated melody",
    "Line id"
};

const char* u8_hl18_LAFieldsInternalNames[] =
{
    "Number",
    "Name",
    "Call interception"
};

const char* u8_hl18_LAFieldsOutgoing[] =
{
    "Number",
    "Name",
    "Date and Time",
    "Line name",
    "Line id"
};

const char* u8_hl18_LAFieldsMissing[] =
{
    "Number",
    "Name",
    "Date and Time",
    "Read status",
    "Line name",
    "Line id",
    "Number of calls"
};

const char* u8_hl18_LAFieldsIncomingAccepted[] =
{
    "Number",
    "Name",
    "Date and Time",
    "Line name",
    "Line id"
};

const char* u8_hl18_LAFieldsAllCallList[] =
{
    "Call type",
    "Number",
    "Name",
    "Date and Time",
    "Line name",
    "Line id"
};

const char* u8_hl18_LAFieldsAllIncomingCallList[] =
{
    "Number",
    "Name",
    "Date and Time",
    "New",
    "Line name",
    "Line id",
    "Number of calls"
};

const char* u8_hl18_LAFieldsLineSettingsList[] =
{
    "Line name",
    "Line ID",
    "Attached Handsets",
    "Dial prefix",
    "FP melody",
    "FP volume",
    "Blocked number",
    "Multicall mode",
    "Intrusion call",
    "Permanent CLIR",
    "Call FWD Uncond",
    "Call FWD NA",
    "Call FWD Busy"
};

const char* u8_hl18_LAFieldsDectSettingsList[] =
{
    "PIN code",
    "Clock master",
    "Base reset",
    "IP type",
    "IP value",
    "IP subnet",
    "IP address",
    "IP DNS",
    "FW version",
    "EEPROM version",
    "HW version",
    "NEMO mode",
    "New PIN code"
};

const char* u8_hl18_CapToneCapabilities[] =
{
    "Not applicable",
    "No display",
    "Numeric",
    "Numeric plus",
    "Alphanumeric",
    "Full display"
};

const char* u8_hl18_CapDisplayCapabilities[] =
{
    "Not applicable",
    "No tone capability",
    "Dial tone only",
    "ITU-T Reccomendation tones supported",
    "Complete DECT tones supported"
};

void p_hl18_PrintMsgType(u8 code)
{
    switch (code)
    {
        case 0 :
            printf("Reserved");
            break;
        case 1 :
            printf("{CC-ALERTING}");
            break;
        case 2 :
            printf("{CC-CALL-PROC}");
            break;
        case 5 :
            printf("{CC-SETUP}");
            break;
        case 7 :
            printf("{CC-CONNECT}");
            break;
        case 8 :
            printf("{COMS-NOTIFY}");
            break;
        case 13 :
            printf("{CC-SETUP-ACK}");
            break;
        case 15 :
            printf("{CC-CONNECT-ACK}");
            break;
        case 32 :
            printf("{CC-SERVICE-CHANGE}");
            break;
        case 33 :
            printf("{CC-SERVICE-ACCEPT}");
            break;
        case 35 :
            printf("{CC-SERVICE-REJECT}");
            break;
        case 36 :
            printf("{HOLD}");
            break;
        case 40 :
            printf("{HOLD-ACK}");
            break;
        case 48 :
            printf("{HOLD-REJECT}");
            break;
        case 49 :
            printf("{RETRIEVE}");
            break;
        case 51 :
            printf("{RETRIEVE-ACK}");
            break;
        case 55 :
            printf("{RETRIEVE-REJECT}");
            break;
        case 64 :
            printf("{AUTHENTICATION-REQUEST}");
            break;
        case 65 :
            printf("{AUTHENTICATION-REPLY}");
            break;
        case 66 :
            printf("{KEY-ALLOCATE}");
            break;
        case 67 :
            printf("{AUTHENTICATION-REJECT}");
            break;
        case 68 :
            printf("{ACCESS-RIGHTS-REQUEST}");
            break;
        case 69 :
            printf("{ACCESS-RIGHTS-ACCEPT}");
            break;
        case 71 :
            printf("{ACCESS-RIGHTS-REJECT}");
            break;
        case 72 :
            printf("{ACCESS-RIGHTS-TERMINATE-REQUEST}");
            break;
        case 73 :
            printf("{ACCESS-RIGHTS-TERMINATE-ACCEPT}");
            break;
        case 75 :
            printf("{ACCESS-RIGHTS-TERMINATE-REJECT}");
            break;
        case 76 :
            printf("{CIPHER-REQUEST}");
            break;
        case 77 :
            printf("{CC-RELEASE}");
            break;
        case 78 :
            printf("{CIPHER-SUGGEST}");
            break;
        case 79 :
            printf("{CIPHER-REJECT}");
            break;
        case 80 :
            printf("{MM-INFO-REQUEST}");
            break;
        case 81 :
            printf("{MM-INFO-ACCEPT}");
            break;
        case 82 :
            printf("{MM-INFO-SUGGEST}");
            break;
        case 83 :
            printf("{MM-INFO-REJECT}");
            break;
        case 84 :
            printf("{LOCATE-REQUEST}");
            break;
        case 85 :
            printf("{LOCATE-ACCEPT}");
            break;
        case 86 :
            printf("{DETACH}");
            break;
        case 87 :
            printf("{LOCATE-REJECT}");
            break;
        case 88 :
            printf("{IDENTITY-REQUEST}");
            break;
        case 89 :
            printf("{IDENTITY-REPLY}");
            break;
        case 90 :
            printf("{CC-RELEASE-COM}");
            break;
        case 91 :
            printf("{MM-IWU}");
            break;
        case 92 :
            printf("{TEMPORARY-IDENTITY-ASSIGN}");
            break;
        case 93 :
            printf("{TEMPORARY-IDENTITY-ASSIGN-ACK}");
            break;
        case 95 :
            printf("{TEMPORARY-IDENTITY-ASSIGN-REJ}");
            break;
        case 96 :
            printf("{IWU-INFO}");
            break;
        case 98 :
            printf("{FACILITY}");
            break;
        case 100 :
            printf("{CISS-REGISTER}");
            break;
        case 110 :
            printf("{CC-NOTIFY}");
            break;
        case 113 :
            printf("{LCE-PAGE-RESPONSE}");
            break;
        case 114 :
            printf("{LCE-PAGE-REJECT}");
            break;
        case 120 :
            printf("{COMS-ACK}");
            break;
        case 123 :
            printf("{CC-INFO}");
            break;

        default:
            printf("{UNKNOWN} ");
            return;
            return;
    }

}

void p_hl18_DbgPrintInfoMsgType(u8 code)
{
    switch (code)
    {
        case 1 :
            printf("<<Info Type>>");
            break;
        case 2 :
            printf("<<Identity type>>");
            break;
        case 5 :
            printf("<<Portable identity>>");
            break;
        case 6 :
            printf("<<Fixed identity>>");
            break;
        case 7 :
            printf("<<Location area>>");
            break;
        case 9 :
            printf("<<NWK assigned identity>>");
            break;
        case 10 :
            printf("<<AUTH-TYPE>>");
            break;
        case 11 :
            printf("<<Allocation type>>");
            break;
        case 12 :
            printf("<<RAND>>");
            break;
        case 13 :
            printf("<<RES>>");
            break;
        case 14 :
            printf("<<RS>>");
            break;
        case 18 :
            printf("<<IWU attributes>>");
            break;
        case 19 :
            printf("<<Call attributes>>");
            break;
        case 22 :
            printf("<<Service change info>>");
            break;
        case 23 :
            printf("<<Connection attributes>>");
            break;
        case 25 :
            printf("<<Cipher info>>");
            break;
        case 26 :
            printf("<<Call identity>>");
            break;
        case 27 :
            printf("<<Connection identity>>");
            break;
        case 28 :
            printf("<<Facility>>");
            break;
        case 30 :
            printf("<<Progress indicator>>");
            break;
        case 32 :
            printf("<<MMS Generic Header>>");
            break;
        case 33 :
            printf("<<MMS Object Header>>");
            break;
        case 34 :
            printf("<<MMS Extended Header>>");
            break;
        case 35 :
            printf("<<Time-Date>>");
            break;
        case 40 :
            printf("<<Multi-Display>>");
            break;
        case 44 :
            printf("<<Multi-Keypad>>");
            break;
        case 56 :
            printf("<<Feature Activate>>");
            break;
        case 57 :
            printf("<<Feature Indicate>>");
            break;
        case 65 :
            printf("<<Network parameter>>");
            break;
        case 66 :
            printf("<<Ext h/o indicator>>");
            break;
        case 82 :
            printf("<<ZAP field>>");
            break;
        case 84 :
            printf("<<Service class>>");
            break;
        case 86 :
            printf("<<Key>>");
            break;
        case 96 :
            printf("<<Reject Reason>>");
            break;
        case 98 :
            printf("<<Setup capability>>");
            break;
        case 99 :
            printf("<<Terminal capability>>");
            break;
        case 100 :
            printf("<<End-to-End compatibility>>");
            break;
        case 101 :
            printf("<<Rate parameters>>");
            break;
        case 102 :
            printf("<<Transit Delay>>");
            break;
        case 103 :
            printf("<<Window size>>");
            break;
        case 108 :
            printf("<<Calling Party Number>>");
            break;
        case 0x6D :
            printf("<<Calling Party Name>>");
            break;
        case 112 :
            printf("<<Called Party Number>>");
            break;
        case 113 :
            printf("<<Called Party Subaddr>>");
            break;
        case 114 :
            printf("<<Duration>>");
            break;
        case 117 :
            printf("<<Segmented info>>");
            break;
        case 118 :
            printf("<<Alphanumeric>>");
            break;
        case 119 :
            printf("<<IWU-to-IWU>>");
            break;
        case 120 :
            printf("<<Model identifier>>");
            break;
        case 122 :
            printf("<<IWU-PACKET>>");
            break;
        case 123 :
            printf("<<Escape to proprietary>>");
            break;
        case 124 :
            printf("<<Codec List>>");
            break;
        case 125 :
            printf("<<Events notification>>");
            break;
        case 126 :
            printf("<<Call information>>");
            break;
        case 127 :
            printf("<<Escape for extension>>");
            break;
        case 0xE0:
            printf("<<BASIC-SERVICE>>");
            break;
        case 0xE4:
            printf("<<SIGNAL>>");
            break;
        case 0xE2:
            printf("<<RELEASE-REASON>>");
            break;

        default:
            printf("<< UNKNOWN >>");
            break;
    }
    printf("\t (0x%x)\n", code);
}

u8 p_hl18_OneByte(u8 value)
{
    return value & 0x7F;
}

u8 p_hl18_ExtendedMultiplicityValue(u8 value)
{
    return (!(value & 0x80));
}

int p_hl18_GetMultiplicityValue(u8 * buff)
{
    int value = buff[0] & 0x7F;

    if (!(buff[0] & 0x80)) // two-byte value
        value = value << 7 | (buff[1] & 0x7F);
    return value;
}

u8 p_hl18_LAParseListIdentifier(u8 * buff)
{
    if (buff[0] < sizeof(u8_hl18_LAListIdentifier) / sizeof(u8_hl18_LAListIdentifier[0]))
    {
        printf("\t%s", u8_hl18_LAListIdentifier[buff[0]]);
    }
    else
    {
        printf("\tWrong List identifier!");
    }
    return 1;
}

u8 p_hl18_LAParseSessionID(u8 * buff)
{
    int session_id = p_hl18_OneByte(buff[0]);
    int listID = p_hl18_LA_Session_GetListID(session_id);
    printf("\tSessionID: %d (%s)\n", session_id, u8_hl18_LAListIdentifier[listID]);
    return 1;
}

u8 u8_hl18_LAParseAvailableEntries(u8 * buff)
{
    printf("\tAvailable entries: %d\n", p_hl18_OneByte(buff[0]));
    return 1;
}

u8 p_hl18_LAParseStartIndex(u8 * buff)
{
    if (p_hl18_OneByte(buff[0]) == 0)
    {
        printf("\tStart index: Last entry\n");
    }
    else
    {
        printf("\tStart index: Entry #%d\n", p_hl18_OneByte(buff[0]));
    }
    return 1;
}

u8 p_hl18_LAParseDirectionCounter(u8 * buff)
{
    printf("\t%s, Counter: %d\n", (buff[0] & 0x80) ? "Backward" : "Forward", p_hl18_OneByte(buff[0]));
    return 1;
}

u8 p_hl18_LAParsePartialCounter(u8 * buff)
{
    printf("\t%s, Counter: %d\n", (buff[0] & 0x80) ? "Partial delivery" : "Full delivery", p_hl18_OneByte(buff[0]));
    return 1;
}

u8 p_hl18_LAParseMarkEntriesRequest(u8 * buff)
{
    switch (buff[0])
    {
        case 0:
            break;
        case 0x7F:
            printf("All read entries 'Read status' field shall be reset\n");
            break;
        case 0xFF:
            printf("All read entries 'Read status' field shall be set\n");
            break;
    }
    return 1;
}

void p_hl18_LAParseRequestedFields(u8 session_id, u8 * buff, u8 len)
{
    u8 i = 0;
    u8 listId = p_hl18_LA_Session_GetListID(session_id);

    printf("\t\t");
    for (i = 0; i < len; i++)
    {
        u8 val = buff[i];
        switch (listId)
        {
            case DECT_DBG_LA_LIST_SUPPORTED_LISTS: //"List of supported lists",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsSupportedLists, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_MISSED_CALLS: //"Missed calls list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsMissing, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_OUTGOING_CALLS: //"Outgoing calls list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsOutgoing, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_INCOMING_CALLS: //"Incoming accepted calls list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsIncomingAccepted, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_ALL_CALLS: //"All calls list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsAllCallList, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_CONTACT_LIST: //"Contact list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsContact, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_INTERNAL_NAMES: //"Internal names list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsInternalNames, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_ALL_INCOMING_CALLS: //"All incoming calls list"
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsAllIncomingCallList, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_DECT_SYSTEM_SETTINGS: //"DECT system settings list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsDectSettingsList, (u8)(val - 1)), val);
                break;
            case DECT_DBG_LA_LIST_LINE_SETTINGS_LIST: //"Line settings list",
                printf("%s (0x%x), ", DECT_DBG_LIST_VALUE(u8_hl18_LAFieldsLineSettingsList, (u8)(val - 1)), val);
                break;
            default:
                printf("%x, ", val);
        }
    }
    printf("\n");
}

void p_hl18_ParseLaStartSession(u8 * buff, u8 len)
{
    u8 pos = 0;
    UNUSED_PARAMETER(len);
    pos += p_hl18_LAParseListIdentifier(buff + pos);
    pos += p_hl18_LAParseSessionID(buff + pos);
}

void p_hl18_ParseLaStartSessionConfirm(u8 * buff, u8 len, u8 u8_HandsetNumber)
{
    u8 pos = 0;
    u8 listID = p_hl18_OneByte(buff[pos]);
    u8 sessionID = p_hl18_OneByte(buff[pos + 1]);
    UNUSED_PARAMETER(len);
    p_hl18_LA_Session_Add(sessionID, listID, u8_HandsetNumber);

    pos += p_hl18_LAParseListIdentifier(buff + pos);
    pos += p_hl18_LAParseSessionID(buff + pos);
    pos += u8_hl18_LAParseAvailableEntries(buff + pos);
}

void p_hl18_ParseLaSearchEntries(u8 * buff, u8 len)
{
    u8 pos = 0;
    u8 sessionID = p_hl18_OneByte(buff[0]);
    char str[255] = {0}; // ugly hardcoded



    pos += p_hl18_LAParseSessionID(buff + pos);

    printf("\tMatching options: 0x%x\n", buff[pos++]);
    strncpy(str, buff + pos + 1, buff[pos]);
    printf("\tSearched value: \"%s\"\n", str);
    pos += buff[pos] + 1;
    pos += p_hl18_LAParseDirectionCounter(buff + pos);
    pos += p_hl18_LAParseMarkEntriesRequest(buff + pos);
    p_hl18_LAParseRequestedFields(sessionID, buff + pos, len - pos);
}

void p_hl18_ParseLaSearchEntriesCfm(u8 * buff, u8 len)
{
    u8 pos = 0;
    UNUSED_PARAMETER(len);

    pos += p_hl18_LAParseSessionID(buff + pos);
    pos += p_hl18_LAParseStartIndex(buff + pos);
    pos += p_hl18_LAParsePartialCounter(buff + pos);
}

void p_hl18_ParseLaReadEntries(u8 * buff, u8 len)
{
    u8 pos = 0;
    u8 sessionID = p_hl18_OneByte(buff[0]);

    pos += p_hl18_LAParseSessionID(buff + pos);
    pos += p_hl18_LAParseStartIndex(buff + pos);
    pos += p_hl18_LAParseDirectionCounter(buff + pos);
    pos += p_hl18_LAParseMarkEntriesRequest(buff + pos);
    p_hl18_LAParseRequestedFields(sessionID, buff + pos, len - pos);
}

void p_hl18_ParseLaReadEntriesCfm(u8 * buff, u8 len)
{
    u8 pos = 0;
    UNUSED_PARAMETER(len);
    pos += p_hl18_LAParseSessionID(buff + pos);
    pos += p_hl18_LAParseStartIndex(buff + pos);
    pos += p_hl18_LAParseDirectionCounter(buff + pos);
}

void p_hl18_ParseLaEndSession(u8 * buff, u8 len)
{
    u8 pos = 0;
    UNUSED_PARAMETER(len);

    p_hl18_LA_Session_Delete(p_hl18_OneByte(buff[pos]));

    pos += p_hl18_LAParseSessionID(buff + pos);
}

void p_hl18_ParseLaEndSessionCfm(u8 * buff, u8 len)
{
    u8 pos = 0;
    UNUSED_PARAMETER(len);

    pos += p_hl18_LAParseSessionID(buff + pos);
}

void p_hl18_ParseLaQuerySupportedEntryFields(u8 * buff, u8 len)
{
    u8 pos = 0;
    UNUSED_PARAMETER(len);

    pos += p_hl18_LAParseSessionID(buff + pos);
}

void p_hl18_ParseLaQuerySupportedEntryFieldsCfm(u8 * buff, u8 len)
{
    u8 pos = 0;
    u8 sessionID = p_hl18_OneByte(buff[pos]);
    UNUSED_PARAMETER(len);
    pos += p_hl18_LAParseSessionID(buff + pos);

    printf("\tNumber of editable entry fields: %d\n", buff[pos++]);
    p_hl18_LAParseRequestedFields(sessionID, buff + pos, buff[pos - 1]);

    pos += buff[pos - 1];

    printf("\tNumber of non-editable entry fields: %d\n", buff[pos++]);
    p_hl18_LAParseRequestedFields(sessionID, buff + pos, buff[pos - 1]);
}

void p_hl18_ParseLaDataPacket(u8 * buff, u8 len)
{
    u8 pos = 0;
    u8 sessionID = p_hl18_OneByte(buff[pos]);

    pos += p_hl18_LAParseSessionID(buff + pos);
    p_hl18_LA_Session_Data(sessionID, buff + pos, len - pos);
}

void p_hl18_ParseLaFieldData(DECT_DBG_FIELD_TYPES type, u8* data, u8 length)
{
    switch (type)
    {
        case DECT_DBG_FIELD_TYPE_NAME:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("Name: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_NUMBER:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("Number: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_DIAL_PREFIX:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("Dial prefix: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_BLOCKED_NUMBER:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("Blocked number: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_CONTACT_NUMBER:
        {
            char str[255] = {0}; // ugly hardcoded

            strncpy(str, data + 1, length - 1);
            printf("Number: %s  ", str);
            if (data[0] & 0x2)
                printf("Work ");
            if (data[0] & 0x4)
                printf("Mobile ");
            if (data[0] & 0x8)
                printf("Fixed ");
            if (data[0] & 0x10)
                printf("Own ");
            if (data[0] & 0x40)
                printf("Editable ");
            break;
        }
        case DECT_DBG_FIELD_TYPE_NUMBER_OF_CALLS:
        {
            printf("Number of calls: %d", data[1]);
            break;
        }
        case DECT_DBG_FIELD_TYPE_INTERCEPTION:
        {
            printf("Interception: %c", data[1]);
            break;
        }
        case DECT_DBG_FIELD_TYPE_MELODY:
        case DECT_DBG_FIELD_TYPE_FP_MELODY:
        {
            printf("Melody: %d", data[1]);
            break;
        }
        case DECT_DBG_FIELD_TYPE_LINE_ID:
        {
            if (data[1] == 0x4)
            {
                printf("Line ID: All lines\n");
            }
            else
            {
                printf("Line ID: %d", p_hl18_OneByte(data[2]));
            }
            break;
        }
        case DECT_DBG_FIELD_TYPE_FIRST_NAME:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("First name: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_LINE_NAME:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("Line name: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_DATETIME:
        {
            printf("DateTime: %x.%x.%x %x:%x:%x", data[4], data[3], data[2], data[5], data[6], data[7]);
            break;
        }
        case DECT_DBG_FIELD_TYPE_NEW:
        {
            printf("New: %s", data[0] & 0x20 ? "Yes" : "No");
            break;
        }
        case DECT_DBG_FIELD_TYPE_CALL_TYPE:
        {
            printf("Call type: ");
            if (data[0] & 0x8)
                printf("Outgoing call ");
            if (data[0] & 0x10)
                printf("Accepted call ");
            if (data[0] & 0x20)
                printf("Missed call ");
            if (data[0] & 0x40)
                printf("Editable ");
            break;
        }
        case DECT_DBG_FIELD_TYPE_ATTACHED_HANDSETS:
        {
            printf("Attached handsets: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("No of HS: %d ", p_hl18_OneByte(data[1]));
            printf("Bitmask: 0x%x ", p_hl18_GetMultiplicityValue(data + 2));
            break;
        }
        case DECT_DBG_FIELD_TYPE_FP_VOLUME:
        {
            printf("FP volume: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("Value: %d ", data[1]);
            break;
        }
        case DECT_DBG_FIELD_TYPE_MULTICALL_MODE:
        {
            printf("Multicall mode: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("%s ", data[1] == 0x31 ? "Multicall mode" : "Single call mode");
            break;
        }
        case DECT_DBG_FIELD_TYPE_INTRUSION_CALL:
        {
            printf("Intrusion call: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("%s ", data[1] == 0x31 ? "Active" : "Not active");
            break;
        }
        case DECT_DBG_FIELD_TYPE_PERMANENT_CLIR:
        {
            u8 actLen = data[2];
            char str[255] = {0}; // ugly, I know
            printf("Permanent CLIR: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");

            printf("%s ", data[1] == 0x31 ? "Active" : "Not active");

            strncpy(str, data + 3, actLen);
            printf("Activation code: %s  ", str);

            strncpy(str, data + 3 + actLen + 1, actLen);
            printf("Deactivation code: %s  ", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_CFUN:
        case DECT_DBG_FIELD_TYPE_CFNA:
        case DECT_DBG_FIELD_TYPE_CFBS:
        {
            u8 actLen = data[2];
            u8 deactLen = data[2 + actLen + 1];
            u8 numberLen = data[2 + actLen + deactLen + 2];
            char str[255] = {0}; // ugly, I know


            if (type == DECT_DBG_FIELD_TYPE_CFUN)
                printf("Call FWD UNC: ");
            if (type == DECT_DBG_FIELD_TYPE_CFNA)
                printf("Call FWD n/a: ");
            if (type == DECT_DBG_FIELD_TYPE_CFBS)
                printf("Call FWD busy: ");

            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("%s ", data[1] == 0x31 ? "Active" : "Not active");

            strncpy(str, data + 3, actLen);
            printf("Activation code: %s  ", str);

            strncpy(str, data + 3 + actLen + 1, actLen);
            printf("Deactivation code: %s  ", str);

            strncpy(str, data + 3 + actLen + deactLen + 2, numberLen); // totally ugly code
            printf("FWD number: %s  ", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_PIN_CODE:
        case DECT_DBG_FIELD_TYPE_NEW_PIN_CODE:
        {
            if (type == DECT_DBG_FIELD_TYPE_PIN_CODE)
            {
                printf("PIN code: ");
            }
            else
            {
                printf("New PIN code: ");
            }

            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("%x%x%x%x%x%x%x%x",

                   (data[1] & 0xF0) >> 4,
                   data[1] & 0xF,

                   (data[2] & 0xF0) >> 4,
                   data[2] & 0xF,

                   (data[3] & 0xF0) >> 4,
                   data[3] & 0xF,

                   (data[4] & 0xF0) >> 4,
                   data[4] & 0xF
                  );
            break;
        }
        case DECT_DBG_FIELD_TYPE_BASE_RESET:
        {
            printf("Base reset: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("%s ", data[1] == 0x31 ? "YES" : "NO");
            break;
        }
        case DECT_DBG_FIELD_TYPE_FW_VERSION:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("FW version: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_HW_VERSION:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("HW version: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_EEPROM_VERSION:
        {
            char str[255] = {0}; // ugly hardcoded
            strncpy(str, data + 1, length - 1);
            printf("EEPROM version: %s", str);
            break;
        }
        case DECT_DBG_FIELD_TYPE_IP_TYPE:
        {
            printf("IP type: ");
            if (data[0] & 0x10)
                printf("Static");
            if (data[0] & 0x20)
                printf("DHCP");
            break;
        }
        case DECT_DBG_FIELD_TYPE_IP_VALUE:
        case DECT_DBG_FIELD_TYPE_IP_SUBNET_MASK:
        case DECT_DBG_FIELD_TYPE_IP_GATEWAY:
        case DECT_DBG_FIELD_TYPE_IP_DNS_SERVER:
        {
            u8 isV6 = data[0] & 0x20;
            if (type == DECT_DBG_FIELD_TYPE_IP_VALUE)
                printf("IP address :");
            if (type == DECT_DBG_FIELD_TYPE_IP_SUBNET_MASK)
                printf("IP subnet :");
            if (type == DECT_DBG_FIELD_TYPE_IP_GATEWAY)
                printf("IP gateway :");
            if (type == DECT_DBG_FIELD_TYPE_IP_DNS_SERVER)
                printf("IP DNS :");

            if (!isV6)
            {
                printf("%d.%d.%d.%d", data[1], data[2], data[3], data[4]);
            }
            else
            {
                printf("%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx",
                       data[1], data[2], data[3], data[4],
                       data[5], data[6], data[7], data[8]);
            }
            break;
        }
        case DECT_DBG_FIELD_TYPE_NEMO_MODE:
        {
            printf("NEMO mode: %d", data[0] & 0x1);
            break;
        }
        case DECT_DBG_FIELD_TYPE_CLOCK_MASTER:
        {
            printf("Clock master call: ");
            if (data[0] & 0x1)
                printf("(PIN protected) ");
            if (data[0] & 0x40)
                printf("(Editable) ");
            printf("%s ", data[1] == 0x31 ? "PP" : "FP");
            break;
        }
        default:
            printf("Unknown field, %d bytes of data", length);
    }
}

void p_hl18_ParseLaDataPacketLast(u8 * buff, u8 len)
{
    u8 pos = 0;
    u8 sessionID = p_hl18_OneByte(buff[pos]);
    LA_Session* obj = p_hl18_LA_Session_GetSession(sessionID);

    pos += p_hl18_LAParseSessionID(buff + pos);
    p_hl18_LA_Session_Data(sessionID, buff + pos, len - pos);

    // data collected
    if (obj)
    {
        u32 i = 0;
        while (i < obj->data_length)
        {
            u8 entryID = 0;
            u8 entryLength = 0;
            u8* entryData;
            u32 j = 0;

            entryID = p_hl18_GetMultiplicityValue(obj->data + i);
            if (p_hl18_ExtendedMultiplicityValue(obj->data[i++]))
                i++;

            entryLength = p_hl18_GetMultiplicityValue(obj->data + i);
            if (p_hl18_ExtendedMultiplicityValue(obj->data[i++]))
                i++;

            entryData = obj->data + i;
            j = 0;
            printf("\tEntry #0x%x:\n", entryID);
            while (j < entryLength)
            {
                u8 fieldID = p_hl18_OneByte(entryData[j++]);
                u8 fieldLength = p_hl18_OneByte(entryData[j++]);

                printf("\t * ");
                p_hl18_ParseLaFieldData(p_hl18_LA_GetFieldType(sessionID, fieldID), entryData + j, fieldLength);
                printf("\n");
                j += fieldLength;
            }
            i += entryLength;
        }
        obj->data_length = 0;
    }

}

void p_hl18_ParseIwuInfo(u8 * buff, u8 len, u8 u8_HandsetNumber)
{
    u8 i = 0;
    printf("\t%s, %s", ((buff[i] & 0x40) == 0x40) ? "Transmission" : "Rejection", ((buff[i] & 0x3F) == 3) ? "List Access" : "Other");
    if ((buff[i] & 0x3F) != 0x3)
        return;
    i++;

    printf(", ");
    switch (buff[i])
    {
        case 0x0:
            printf("Start session\n");
            p_hl18_ParseLaStartSession(buff + 2, len - i - 1);
            break;
        case 0x1:
            printf("Start session confirm\n");
            p_hl18_ParseLaStartSessionConfirm(buff + 2, len - i - 1, u8_HandsetNumber);
            break;
        case 0x2:
            printf("End session\n");
            p_hl18_ParseLaEndSession(buff + 2, len - i - 1);
            break;
        case 0x3:
            printf("End session confirm\n");
            p_hl18_ParseLaEndSessionCfm(buff + 2, len - i - 1);
            break;
        case 0x4:
            printf("Query supported entry fields\n");
            p_hl18_ParseLaQuerySupportedEntryFields(buff + 2, len - i - 1);

            break;
        case 0x5:
            printf("Query supported entry fields confirm\n");
            p_hl18_ParseLaQuerySupportedEntryFieldsCfm(buff + 2, len - i - 1);
            break;
        case 0x6:
            printf("Read entries\n");
            p_hl18_ParseLaReadEntries(buff + 2, len - i - 1);
            break;
        case 0x7:
            printf("Read entries confirm\n");
            p_hl18_ParseLaReadEntriesCfm(buff + 2, len - i - 1);
            break;
        case 0x8:
            printf("Edit entry\n");
            break;
        case 0x9:
            printf("Edit entry confirm\n");
            break;
        case 0xA:
            printf("Save entry\n");
            break;
        case 0xB:
            printf("Save entry confirm\n");
            break;
        case 0xC:
            printf("Delete entry\n");
            break;
        case 0xD:
            printf("Delete entry confirm\n");
            break;
        case 0xE:
            printf("Delete list\n");
            break;
        case 0xF:
            printf("Delete list confirm\n");
            break;
        case 0x10:
            printf("Search entries\n");
            p_hl18_ParseLaSearchEntries(buff + 2, len - i - 1);
            break;
        case 0x11:
            printf("Search entries confirm\n");
            p_hl18_ParseLaSearchEntriesCfm(buff + 2, len - i - 1);
            break;
        case 0x12:
            printf("Negative acknowledgement\n");
            break;
        case 0x13:
            printf("Data packet\n");
            p_hl18_ParseLaDataPacket(buff + 2, len - i - 1);
            break;
        case 0x14:
            printf("Data packet last\n");
            p_hl18_ParseLaDataPacketLast(buff + 2, len - i - 1);
            break;
        default:
            printf("Unknown command\n");
            return;
    }
}

void p_hl18_ParseCodecList(u8 * buff, u8 u8_Len)
{

    u8 pos = 0;

    printf("\t%s\n", (buff[pos] & 0x70) ? "Codec negotiation" : "Negotiation not possible");
    pos++;
    while (pos < u8_Len)
    {
        printf("\tCodec: ");
        switch (buff[pos])
        {
            case 1:
                printf("user specific 32 kbit/s");
                break;
            case 2:
                printf("G.726");
                break;
            case 3:
                printf("G.722");
                break;
            case 4:
                printf("G.711 A law PCM");
                break;
            case 5:
                printf("G.711 µ law PCM");
                break;
            case 6:
                printf("G.729.1");
                break;
            case 7:
                printf("MPEG-4 ER AAC-LD 32 kbit/s");
                break;
            case 8:
                printf("MPEG-4 ER AAC-LD 64 kbit/s");
                break;
            case 9:
                printf("user specific 64 kbit/s");
                break;
            default:
                printf("Unknown");
                break;
        }

        printf(", ");

        //printf("Slot: ");
        switch (buff[pos + 2] & 0xF)
        {
            case 0:
                printf("Half slot; j = 0");
                break;
            case 1:
                printf("Long slot; j = 640");
                break;
            case 2:
                printf("Long slot; j = 672");
                break;
            case 4:
                printf("Full slot");
                break;
            case 5:
                printf("Double slot");
                break;
            default:
                printf("Unknown slot");
                break;
        }
        printf("\n");

        pos += 3;
    }
    printf("\n");
}

void p_hl18_ParseMultiKeypad(u8* buff, u8 u8_Len)
{
    u8 pos = 0;
    printf("\t");

    switch (buff[pos])
    {
        case 0x17:
            printf("Outgoing parallel call initiation (internal): ");
            pos++;
            break;
        case 0x1C:
            switch (buff[pos + 1])
            {
                case 0x15:
                    printf("Outgoing parallel call initiation (external) ");
                    pos += 2;
                    break;
                case 0x31:
                    printf("Call toggle request (external or internal) ");
                    pos += 2;
                    break;
                case 0x32:
                    printf("3 party conference call request (external or internal) ");
                    pos += 2;
                    break;
                case 0x33:
                    printf("Call release (of the indicated call) ");
                    pos += 2;
                    break;
                case 0x34:
                    printf("Call transfer request (external or internal) ");
                    pos += 2;
                    break;
                case 0x35:
                    printf("Call waiting acceptation ");
                    pos += 2;
                    break;
                case 0x36:
                    printf("Call waiting rejection ");
                    pos += 2;
                    break;
                case 0x38:
                    printf("Active call release with replacement (from PP to FP) ");
                    pos += 2;
                    break;
                case 0x40:
                    printf("Explicit call intrusion ( in {CC SETUP} ?)");
                    pos += 2;
                    break;
                case 0x41:
                    printf("Putting a call on hold ");
                    pos += 2;
                    break;
                case 0x42:
                    printf("Resuming a call put on hold ");
                    pos += 2;
                    break;
                case 0x50:
                    printf("Call interception request from HPP (or PP) ( in {CC SETUP} ?)");
                    pos += 2;
                    break;
            }
            break;
        default:
            break;
    }
    for (; pos < u8_Len; pos++)
    {
        if (buff[pos] >= '0' && buff[pos] <= '9')
        {
            printf("0x%x (\"%c\") ", buff[pos], buff[pos]);
        }
        else
        {
            printf("0x%x ", buff[pos]);
        }
    }
    printf("\n");
}

void p_hl18_ParseCallInfo(u8* buff, u8 u8_Len)
{
    u8 pos = 0;
    u8 value = 0;
    while (pos < u8_Len)
    {
        printf("\t");
        //printf("Identifier type: ");
        switch (buff[pos])
        {
            case 0:
                printf("Line ID: ");
                break;
            case 1:
                printf("Call ID: ");
                break;
            case 2:
                printf("Call status: ");
                break;
            default:
                printf("UNKNOWN: ");
                break;
        }

        value = buff[pos + 2] & 0x7F;

        switch (buff[pos])
        {
            case 2: // call status/reason
            {
                if (buff[pos + 1] == 1)
                    switch (value)
                    {
                        case 0:
                            printf("CS idle");
                            break;
                        case 1:
                            printf("CS call setup");
                            break;
                        case 2:
                            printf("CS call setup ack ");
                            break;
                        case 3:
                            printf("CS call proc");
                            break;
                        case 4:
                            printf("CS call alerting");
                            break;
                        case 5:
                            printf("CS call connect");
                            break;
                        case 6:
                            printf("CS call disconnecting");
                            break;
                        case 9:
                            printf("CS call hold");
                            break;
                        case 0xA:
                            printf("CS call intercepted");
                            break;
                        case 0xB:
                            printf("CS conference connect");
                            break;
                        case 0xC:
                            printf("CS call under transfer");
                            break;
                        default:
                            printf("UNKNOWN");
                            break;

                    }

                if (buff[pos + 1] == 2)
                    switch (value)
                    {
                        case 0:
                            printf("system busy ");
                            break;
                        case 1:
                            printf("line in use");
                            break;
                        case 2:
                            printf("control code not supported");
                            break;
                        case 3:
                            printf("control code failed");
                            break;
                        case 16:
                            printf("user busy");
                            break;
                        case 17:
                            printf("number not available");
                            break;
                        default:
                            printf("UNKNOWN");
                            break;
                    }

            }
            default:
                //printf("\n");
                break;
        }


        printf(" 0x%x ", value);

        if (buff[pos] == 0)
        {

            //printf("Identifier subtype: ");
            switch (buff[pos + 1])
            {
                case 0:
                    printf(" (LineID for external call) ");
                    break;
                case 1:
                    printf(" (Source line identifier for internal call) ");
                    break;
                case 2:
                    printf(" (Target line identifier for internal call) ");
                    break;
                case 3:
                    printf(" (\"Relating-to\" line identifier) ");
                    break;
                case 4:
                    printf(" (\"All lines\") ");
                    break;
                case 5:
                    printf(" (Line type information) ");
                    break;
                default:
                    printf(" (UNKNOWN) ");
                    break;
            }


        }

        printf("\n");

        pos += 3;
    }
    printf("\n");
}

void p_hl18_ParseBasicService(u8 * buff)
{
    u8 pos = 0;
    u8 callClass = (buff[1] & 0xF0);
    u8 basicService = (buff[1] & 0x0F);
    UNUSED_PARAMETER(pos);
    printf("\tCall class: ");
    switch (callClass)
    {
        case 0x20:
            printf("LiA service setup");
            break;
        case 0x40:
            printf("Message call setup");
            break;
        case 0x70:
            printf("DECT/ISDN IIP");
            break;
        case 0x80:
            printf("Normal call setup");
            break;
        case 0x90:
            printf("Internal call setup");
            break;
        case 0xA0:
            printf("Emergency call setup");
            break;
        case 0xB0:
            printf("Service call setup");
            break;
        case 0xC0:
            printf("External handover call setup");
            break;
        case 0xD0:
            printf("Supplementary service call setup");
            break;
        case 0xE0:
            printf("OA&M call setup");
            break;
    }

    printf("\n");

    printf("\tBasic service: ");
    switch (basicService)
    {
        case 0x0:
            printf("Basic speech default setup attributes");
            break;
        case 0x4:
            printf("DECT GSM IWP profile");
            break;
        case 0x6:
            printf("DECT UMTS IWP/GSM IWP SMS");
            break;
        case 0x5:
            printf("LRMS (E-profile) service");
            break;
        case 0x8:
            printf("Wideband speech default setup attributes");
            break;
        case 0x9:
            printf("Light data services: SUOTA, Class 4 DPRS management");
            break;
        case 0xA:
            printf("Light data services: SUOTA, Class 3 DPRS management");
            break;
        case 0xF:
            printf("Other");
            break;
    }
    printf("\n");
}

void p_hl18_ParseSignal(u8 * buff)
{
    printf("\t");
    switch (buff[0])
    {
        case 0:
            printf("Dial tone on");
            break;
        case 1:
            printf("Ring-back tone on");
            break;
        case 2:
            printf("Intercept tone on");
            break;
        case 3:
            printf("Network congestion tone on");
            break;
        case 4:
            printf("Busy tone on");
            break;
        case 5:
            printf("Confirm tone on");
            break;
        case 6:
            printf("Answer tone on");
            break;
        case 7:
            printf("Call waiting tone on");
            break;
        case 8:
            printf("Off-hook warning tone on");
            break;
        case 9:
            printf("Negative acknowledgment tone");
            break;
        case 0x3F:
            printf("Tones off");
            break;
        case 0x40:
            printf("Alerting on - pattern 0");
            break;
        case 0x41:
            printf("Alerting on - pattern 1");
            break;
        case 0x42:
            printf("Alerting on - pattern 2");
            break;
        case 0x43:
            printf("Alerting on - pattern 3");
            break;
        case 0x44:
            printf("Alerting on - pattern 4");
            break;
        case 0x45:
            printf("Alerting on - pattern 5");
            break;
        case 0x46:
            printf("Alerting on - pattern 6");
            break;
        case 0x47:
            printf("Alerting on - pattern 7");
            break;
        case 0x48:
            printf("Alerting on - continuous");
            break;
        case 0x49:
            printf("Alerting off");
            break;
    }
    printf("\n");
}

void p_hl18_ParseCallingPartyNumber(u8* buff, u8 u8_Len)
{
    u8 pos = 0;
    u8 has_octet3a = (buff[0] & 0x80) != 0x80;
    u8 numberType = (buff[0] & 0x70);
    u8 numberingPlan = (buff[0] & 0xF);

    printf("\tNumber type: ");
    switch (numberType)
    {
        case 0:
            printf("Unknown");
            break;
        case 1:
            printf("International number");
            break;
        case 2:
            printf("National number");
            break;
        case 3:
            printf("Network specific number");
            break;
        case 4:
            printf("Subscriber number");
            break;
        case 6:
            printf("Abbreviated number");
            break;
        case 7:
            printf("Reserved for extension");
            break;
    }

    printf("\n");

    printf("\tNumbering plan: ");
    switch (numberingPlan)
    {
        case 0:
            printf("Unknown");
            break;
        case 1:
            printf("ISDN/telephony plan");
            break;
        case 3:
            printf("Data plan");
            break;
        case 7:
            printf("TCP/IP address");
            break;
        case 8:
            printf("National standard");
            break;
        case 9:
            printf("Private plan");
            break;
        case 0xA:
            printf("SIP addressing scheme");
            break;
        case 0xB:
            printf("Internet character format address ");
            break;
        case 0xC:
            printf("LAN MAC address ");
            break;
        case 0xD:
            printf("ITU-T Recommendation X.400");
            break;
        case 0xE:
            printf("Profile service specific alphanumeric identifier ");
            break;
        case 0xF:
            printf("Reserved for extension");
            break;
    }

    printf("\n");

    if (has_octet3a)
    {
        u8 presentIndicator = buff[pos] & 0x60;

        pos++;

        printf("\tPresentation indicator: ");
        switch (presentIndicator)
        {
            case 0:
                printf("Presentation allowed");
                break;
            case 1:
                printf("Presentation restricted");
                break;
            case 2:
                printf("Number not available");
                break;
            case 3:
                printf("Reserved");
                break;
        }

        printf(", ");

        printf("Screening indicator: ");
        switch (presentIndicator)
        {
            case 0:
                printf("User-provided, not screened");
                break;
            case 1:
                printf("User-provided, verified and passed");
                break;
            case 2:
                printf("User-provided, verified and failed");
                break;
            case 3:
                printf("Network provided");
                break;
        }

        printf("\n");
    }

    pos++;

    printf("\tNumber: ");
    while (pos < u8_Len)
    {
        if (buff[pos] >= '0' && buff[pos] <= '9')
        {
            printf("0x%x (\"%c\") ", buff[pos], buff[pos]);
        }
        else
        {
            printf("0x%x ", buff[pos]);
        }
        pos++;
    }

    printf("\n");
}

void p_hl18_ParseProgressIndicator(u8* buff, u8 u8_Len)
{
    u8 codingStandard = (buff[0] & 0x60) >> 5;
    u8 progressDescription = buff[1] & 0x7F;
    UNUSED_PARAMETER(u8_Len);
    printf("\tCoding standard: ");
    switch (codingStandard)
    {
        case 0x0:
            printf("ITU T standardized coding");
            break;
        case 0x1:
            printf("reserved for other international standards");
            break;
        case 0x2:
            printf("national standard");
            break;
        case 0x3:
            printf("standard specific to identified location");
            break;
    }

    printf("\n\tLocation: ");
    switch (codingStandard)
    {
        case 0x0:
            printf("user");
            break;
        case 0x1:
            printf("private network serving the local user");
            break;
        case 0x2:
            printf("public network serving the local user");
            break;
        case 0x4:
            printf("public network serving the remote user");
            break;
        case 0x5:
            printf("private network serving the remote user");
            break;
        case 0x7:
            printf("international network");
            break;
        case 0xA:
            printf("network beyond interworking point");
            break;
        case 0xF:
            printf("not applicable");
            break;
        default:
            printf("Unknown");
            break;
    }

    printf("\n\tProgress description: ");
    switch (progressDescription)
    {
        case 0x1:
            printf("Call is not end-to-end ISDN, further call progress info may be available in band");
            break;
        case 0x2:
            printf("Destination address is non-ISDN");
            break;
        case 0x3:
            printf("Origination address is non-ISDN");
            break;
        case 0x4:
            printf("Call has returned to the ISDN");
            break;
        case 0x5:
            printf("Service change has occurred");
            break;
        case 0x8:
            printf("In-band information or appropriate pattern now available");
            break;
        case 0x9:
            printf("In-band information not available");
            break;
        case 0x20:
            printf("Call is end-to-end PLMN/ISDN");
            break;
        default:
            printf("Unknown");
            break;
    }

    printf("\n");

}

void p_hl18_ParseEventsNotification(u8* buff, u8 u8_Len)
{
    //1 81 8F
    u8 pos = 0;
    u8 type = buff[pos];
    int value = p_hl18_GetMultiplicityValue(buff + pos + 2);

    printf("\t");
    switch (type)
    {
        case 0:
            printf("Message waiting, ");
            switch (p_hl18_OneByte(buff[pos + 1]))
            {
                case 0:
                    printf("Unknown");
                    break;
                case 1:
                    printf("Voice message");
                    break;
                case 2:
                    printf("SMS message");
                    break;
                case 3:
                    printf("E-mail message");
                    break;
                default:
                    printf("Unknown");
                    break;
            }
            printf("\n\tValue: %d\n", value);
            break;
        case 1:
            printf("Missed call, ");
            switch (p_hl18_OneByte(buff[pos]))
            {
                case 1:
                    printf("A new external missed voice call just arrived");
                    break;
                case 2:
                    printf("No new missed call arrived, but the number of ‘unread’ external missed voice calls has -or may have- changed");
                    break;
                default:
                    printf("Unknown");
                    break;
            }
            printf("\n\tValue: %d\n", value);
            break;
        case 2:

            printf("Web content");
            printf("\n\tValue: %d\n", value);
            break;
        case 3:
            printf("List change indication, ");
            switch (buff[pos + 1])
            {
                case 0:
                    printf("Message waiting ");
                    break;
                case 1:
                    printf("Missed call ");
                    break;
                default:
                    printf("? ");
                    break;
            }
            printf("\n\tTotal number of elements in the list: %d\n", value);
            break;
        case 4:
            printf("Software upgrade indication");
            break;
        default:
            printf("Unknown");
            break;
    }

    if (p_hl18_ExtendedMultiplicityValue(buff[pos + 2]))
        pos++;
    pos += 3;

    if (pos < u8_Len && type != 3)
        p_hl18_ParseEventsNotification(buff + pos, u8_Len - pos);
}

void p_hl18_ParseDateTime(u8* buff, u8 u8_Len)
{
    // 23[DT] 8[L] C0 9 10 3 11 55 1 8
    u8 coding = (buff[0] & 0xC0) >> 6;
    u8 interpretation = buff[0] & 0x3F;
    UNUSED_PARAMETER(u8_Len);
    printf("\tCoding: ");
    switch (coding)
    {
        case 1:
            printf("Time");
            break;
        case 2:
            printf("Date");
            break;
        case 3:
            printf("Time and Date");
            break;
    }

    printf(", Interpretation: ");
    switch (interpretation)
    {
        case 0:
            printf("The current time/date");
            break;
        case 1:
            printf("Time duration (in Years, Months, Days and/or Hours, Minutes, Seconds, Time Zone = 0)");
            break;
        case 0x20:
            printf("The time/date at which to start forwarding/delivering the MMS message");
            break;
        case 0x21:
            printf("The time/date the MMS user data was created");
            break;
        case 0x22:
            printf("The time/date the MMS user data was last modified");
            break;
        case 0x23:
            printf("The time/date the message was received by the MCE");
            break;
        case 0x24:
            printf("The time/date the message was delivered/accessed by the End Entity");
            break;
        case 0x28:
            printf("The time/date stamp for use as an identifier");
            break;
    }
    printf("\n");
    printf("\t%x.%x.%x %x:%x:%x TIMEZONE: %x\n", buff[3], buff[2], buff[1], buff[4], buff[5], buff[6], buff[7]);
}

void p_hl18_ParseServiceChangeInfo(u8* buff, u8 u8_Len)
{
    // 16 1 8D
    u8 master = (buff[0] & 0x10) >> 4;
    u8 changeMode = buff[0] & 0xF;
    UNUSED_PARAMETER(u8_Len);
    printf("\t%s, ", master == 0 ? "Initiating side is master" : "Receiving side is master");

    switch (changeMode)
    {
        case 0x0:
            printf("None");
            break;
        case 0x1:
            printf("Connection Reversal");
            break;
        case 0x2:
            printf("Bandwidth change");
            break;
        case 0x3:
            printf("Modulation scheme change");
            break;
        case 0x4:
            printf("Rerouting (of U-plane links)");
            break;
        case 0x5:
            printf("Bandwidth plus modulation scheme change");
            break;
        case 0x6:
            printf("Rerouting plus bandwidth change");
            break;
        case 0x7:
            printf("Bandwidth or modulation scheme change");
            break;
        case 0x8:
            printf("Suspend");
            break;
        case 0x9:
            printf("Resume");
            break;
        case 0xA:
            printf("Voice/data change to data");
            break;
        case 0xB:
            printf("Voice/data change to voice");
            break;
        case 0xC:
            printf("IWU attribute change");
            break;
        case 0xD:
            printf("Audio Codec change");
            break;
        case 0xE:
            printf("Profile/Basic service and IWU attributes change");
            break;
        case 0xF:
        default:
            printf("Reserved for extension");
            break;
    }
    printf("\n");
}

void p_hl18_ParseCallingPartyName(u8* buff, u8 u8_Len)
{
    u8 pIndicator = (buff[0] & 0x60) >> 5;
    u8 alphabet = (buff[0] & 0x1C) >> 2;
    u8 sIndicator = buff[0] & 0x03;
    u8 i = 0;

    printf("\t\"");
    for (i = 1; i < u8_Len; i++)
        printf("%c", buff[i]);
    printf("\"\t");

    switch (pIndicator)
    {
        case 0x0:
            printf("Presentation allowed");
            break;
        case 0x1:
            printf("Presentation restricted");
            break;
        case 0x2:
            printf("Name not available");
            break;
        case 0x3:
            printf("Handset locator");
            break;
        default:
            break;
    }

    printf(", ");

    switch (alphabet)
    {
        case 0x0:
            printf("DECT standard");
            break;
        case 0x1:
            printf("UTF-8");
            break;
        case 0x7:
            printf("Network specific");
            break;
        default:
            break;
    }

    printf(", ");

    switch (sIndicator)
    {
        case 0x0:
            printf("User-provided, not screened");
            break;
        case 0x1:
            printf("User-provided, verified and passed");
            break;
        case 0x2:
            printf("User-provided, verified and failed");
            break;
        case 0x3:
        default:
            printf("Network provided");
            break;
    }

    printf("\n");

}

void p_hl18_ParseReleaseReason(u8* buff)
{
    printf("\t");
    switch (buff[0])
    {
        case 0x00:
            printf("Normal");
            break;
        case 0x01:
            printf("Unexpected Message");
            break;
        case 0x02:
            printf("Unknown Transaction Identifier");
            break;
        case 0x03:
            printf("Mandatory information element missing");
            break;
        case 0x04:
            printf("Invalid information element contents");
            break;
        case 0x05:
            printf("Incompatible service");
            break;
        case 0x06:
            printf("Service not implemented");
            break;
        case 0x07:
            printf("Negotiation not supported");
            break;
        case 0x08:
            printf("Invalid identity");
            break;
        case 0x09:
            printf("Authentication failed");
            break;
        case 0x0A:
            printf("Unknown identity");
            break;
        case 0x0B:
            printf("Negotiation failed");
            break;
        case 0x0C:
            printf("Reserved");
            break;
        case 0x0D:
            printf("Timer expiry");
            break;
        case 0x0E:
            printf("Partial release");
            break;
        case 0x0F:
            printf("Unknown");
            break;
        case 0x10:
            printf("User detached");
            break;
        case 0x11:
            printf("User not in range");
            break;
        case 0x12:
            printf("User unknown");
            break;
        case 0x13:
            printf("User already active");
            break;
        case 0x14:
            printf("User busy");
            break;
        case 0x15:
            printf("User rejection");
            break;
        case 0x16:
            printf("User call modify");
            break;
        case 0x20:
            printf("Reserved");
            break;
        case 0x21:
            printf("External Handover not supported");
            break;
        case 0x22:
            printf("Network Parameters missing");
            break;
        case 0x23:
            printf("External Handover release");
            break;
        case 0x30:
            printf("Reserved");
            break;
        case 0x31:
            printf("Overload");
            break;
        case 0x32:
            printf("Insufficient resources");
            break;
        case 0x33:
            printf("Insufficient bearers available");
            break;
        case 0x34:
            printf("IWU congestion");
            break;
        case 0x40:
            printf("Security attack assumed");
            break;
        case 0x41:
            printf("Encryption activation failed");
            break;
        case 0x42:
            printf("Re-Keying failed");
            break;
        default:
            printf("Unknown");
            break;

    }
    printf("\n");
}

void p_hl18_ParseRejectReason(u8* buff, u8 u8_Len)
{
    UNUSED_PARAMETER(u8_Len);
    switch (buff[0])
    {
        case 0x01:
            printf("\tTPUI unknown\n");
            break;
        case 0x02:
            printf("\tIPUI unknown\n");
            break;
        case 0x03:
            printf("\tnetwork assigned identity unknown\n");
            break;
        case 0x05:
            printf("\tIPEI not accepted\n");
            break;
        case 0x06:
            printf("\tIPUI not accepted\n");
            break;
        case 0x10:
            printf("\tauthentication failed\n");
            break;
        case 0x11:
            printf("\tno authentication algorithm\n");
            break;
        case 0x12:
            printf("\tauthentication algorithm not supported\n");
            break;
        case 0x13:
            printf("\tauthentication key not supported\n");
            break;
        case 0x14:
            printf("\tUPI not entered\n");
            break;
        case 0x17:
            printf("\tno cipher algorithm\n");
            break;
        case 0x18:
            printf("\tcipher algorithm not supported\n");
            break;
        case 0x19:
            printf("\tcipher key not supported\n");
            break;
        case 0x20:
            printf("\tincompatible service\n");
            break;
        case 0x21:
            printf("\tfalse LCE reply (no corresponding service)\n");
            break;
        case 0x22:
            printf("\tlate LCE reply (service already taken)\n");
            break;
        case 0x23:
            printf("\tinvalid TPUI\n");
            break;
        case 0x24:
            printf("\tTPUI assignment limits unacceptable\n");
            break;
        case 0x2F:
            printf("\tinsufficient memory\n");
            break;
        case 0x30:
            printf("\toverload (see note)\n");
            break;
        case 0x40:
            printf("\ttest call back: normal, en-bloc\n");
            break;
        case 0x41:
            printf("\ttest call back: normal, piecewise\n");
            break;
        case 0x42:
            printf("\ttest call back: emergency, en-bloc\n");
            break;
        case 0x43:
            printf("\ttest call back: emergency, piecewise\n");
            break;
        case 0x5F:
            printf("\tinvalid message\n");
            break;
        case 0x60:
            printf("\tinformation element error\n");
            break;
        case 0x64:
            printf("\tinvalid information element contents\n");
            break;
        case 0x70:
            printf("\ttimer expiry\n");
            break;
        case 0x76:
            printf("\tPLMN not allowed\n");
            break;
        case 0x80:
            printf("\tLocation area not allowed\n");
            break;
        case 0x81:
            printf("\tNational roaming not allowed in this location area\n");
            break;
        default:
            printf("Unknown reason\n");
    }
}

void p_hl18_ParseTerminalCapabilities(u8* buff, u8 u8_Len)
{
    u8 pos = 0;
    u8 toneCapabilities;
    u8 displayCapabilities;
    u8 slottype;
    u8 displayMS = 0, displayLS = 0, displayLines = 0, displayCharactersLine = 0;
    u8 p7 = 0;
    UNUSED_PARAMETER(u8_Len);
    do {
        // octet 3
        toneCapabilities = (buff[pos] >> 4) & 0x7;
        displayCapabilities = buff[pos] & 0xF;

        printf("\t");
        printf("Display %s, %s\n", DECT_DBG_LIST_VALUE(u8_hl18_CapToneCapabilities, toneCapabilities), DECT_DBG_LIST_VALUE(u8_hl18_CapDisplayCapabilities, displayCapabilities));
        if (buff[pos++] & 0x80)
            break;

        /* not interesting */

        if (buff[pos++] & 0x80)
            break;

        // octet 3a
        slottype = buff[pos] & 0x7F;
        if (slottype & 0x1)
            printf("\tHalf slot; j=80\n");
        if (slottype & 0x2)
            printf("\tLong slot; j=640\n");
        if (slottype & 0x4)
            printf("\tLong slot; j=672\n");
        if (slottype & 0x8)
            printf("\tFull slot\n");
        if (slottype & 0x10)
            printf("\tDouble slot\n");

        if (buff[pos++] & 0x80)
            break;

        displayMS = buff[pos] & 0x7F;
        if (buff[pos++] & 0x80)
            break;

        displayLS = buff[pos] & 0x7F;
        if (buff[pos++] & 0x80)
            break;

        displayLines = buff[pos] & 0x7F;
        if (buff[pos++] & 0x80)
            break;

        displayCharactersLine = buff[pos] & 0x7F;
        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;
    } while (0);

    printf("\tDisplay: %d/%d characters, lines: %d, characters per line: %d\n", displayMS, displayLS, displayLines, displayCharactersLine);

    do {
        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

        p7 = buff[pos] & 0x7F;
        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

        if (buff[pos++] & 0x80)
            break;

    } while (0);

    printf("\tProfile indicators: \n");
    if (p7 & 0x1)
        printf("\t\t64-level modulation scheme supported (B+Z field)\n");
    if (p7 & 0x2)
        printf("\t\tNG-DECT Part 1: Wideband voice supported\n");
    if (p7 & 0x6)
        printf("\t\tSupport of NG-DECT Part 3\n");
    if (p7 & 0x10)
        printf("\t\tSupport of the \"Headset management\" feature\n");
    if (p7 & 0x20)
        printf("\t\tSupport of \"Re-keying\" and \"default cipher key mechanism early encryption\"\n");
}

void cmbs_int_ParseDectMsg(u8 * buff, u8 u8_ILen, u8 u8_HandsetNumber)
{
    u32 u32_Cnt = 0;
    u32 u32_Len = 0;
    u8 value = 0;

    p_hl18_PrintMsgType(buff[u32_Cnt]);
    printf("\n");

    // remove all LA sessions for this Handset
    if (buff[u32_Cnt] == 0x4D)
        p_hl18_LA_Session_DeleteByHandset(u8_HandsetNumber);

    u32_Cnt++;

    while (u32_Cnt < u8_ILen)
    {
        printf("  ");
        p_hl18_DbgPrintInfoMsgType(buff[u32_Cnt]);

        value = buff[u32_Cnt];
        u32_Len = buff[u32_Cnt + 1];
        u32_Cnt += 2;
        //printf("Length: %x\n", u8_Len);

        switch (value)
        {
            case 0x77:
                p_hl18_ParseIwuInfo(buff + u32_Cnt, u32_Len, u8_HandsetNumber);
                break;
            case 0x2C:
                p_hl18_ParseMultiKeypad(buff + u32_Cnt, u32_Len);
                break;
            case 0x7C:
                p_hl18_ParseCodecList(buff + u32_Cnt, u32_Len);
                break;
            case 0x7E:
                p_hl18_ParseCallInfo(buff + u32_Cnt, u32_Len);
                break;
            case 0xE0:
                p_hl18_ParseBasicService(buff + u32_Cnt - 2);
                u32_Len = 0;
                break;
            case 0xE4:
                p_hl18_ParseSignal(buff + u32_Cnt - 1);
                u32_Len = 0;
                break;
            case 0x6C:
                p_hl18_ParseCallingPartyNumber(buff + u32_Cnt, u32_Len);
                break;
            case 0x6D:
                p_hl18_ParseCallingPartyName(buff + u32_Cnt, u32_Len);
                break;
            case 0xE2:
                p_hl18_ParseReleaseReason(buff + u32_Cnt - 1);
                u32_Len = 0;
                break;
            case 0x16:
                p_hl18_ParseServiceChangeInfo(buff + u32_Cnt, u32_Len);
                break;
            case 0x1E:
                p_hl18_ParseProgressIndicator(buff + u32_Cnt, u32_Len);
                break;
            case 0x23:
                p_hl18_ParseDateTime(buff + u32_Cnt, u32_Len);
                break;
            case 0x7D:
                p_hl18_ParseEventsNotification(buff + u32_Cnt, u32_Len);
                break;
            case 0x63:
                p_hl18_ParseTerminalCapabilities(buff + u32_Cnt, u32_Len);
                break;
            case 0x60:
                p_hl18_ParseRejectReason(buff + u32_Cnt, u32_Len);
                break;
        }

        u32_Cnt += u32_Len;

    }
    printf("\n");
}
