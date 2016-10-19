/*************************************************************************************************************
*** ListsApp
*** An implementation of lists API over SQL database
**
**************************************************************************************************************/

/*******************************************
Includes
********************************************/
#include <stdio.h>
#include <assert.h>
#include "ListsApp.h"
#include "cmbs_platf.h"
#include "appcall.h"
#include "LASessionMgr.h"


/*******************************************
Defines
********************************************/
#define DATABASE g_pDatabaseHandle

#ifdef WIN32
#define snprintf _snprintf
#endif

/*******************************************
Types
********************************************/


/*******************************************
Globals
********************************************/
/* Database handle */
static void* g_pDatabaseHandle = NULL;

/*******************************************
SQL Queries
********************************************/
/* auxiliary */
static const char* g_ps_TypeStrings[FIELD_TYPE_MAX]     = { "CHAR", "INTEGER", "TEXT"};
static const char* g_ps_TableNameStrings[LIST_TYPE_MAX] = { "ContactList", "LineSettingsList", "CallsList", "CallsList", "CallsList", "CallsList", "CallsList"};

static const char* sAutoincrement                       = "AUTOINCREMENT";
static const char* sPrimaryKey                          = "PRIMARY KEY";
static const char* sNotNull                             = "NOT NULL";

/* Create table. Arg1 = table name, Arg2 = list of fields */
static const char* g_sCreateTableQuery = "CREATE TABLE IF NOT EXISTS %s ( %s );";

/* Get Number of entries. Arg1 = table name */
static const char* g_sSelectCountQuery = "SELECT COUNT(*) FROM %s";

/* Insert new entry. Arg1 = table name,  Arg2 = List of column names, Arg3 = list of values to be inserted */
static const char* g_sInsertQuery = "INSERT INTO %s ( %s ) VALUES ( %s );";

/* Delete all entries. Arg1 = table name */
static const char* g_sDeleteAllQuery = "DELETE FROM %s";

/* Reset Auto-increment to 1. Arg1 = table name */
static const char* g_sResetAutoIncQuery = "DELETE FROM SQLITE_SEQUENCE WHERE NAME='%s';";

/* Delete Entry by Id. Arg1 = table name,  Arg2 = entry id */
static const char* g_sDeleteEntryQuery = "DELETE FROM %s WHERE ENTRY_ID=%d;";

/* Update an existing entry. Arg1 = table name, Arg2 = list of pattern column=value, Arg3 = Entry Id  */
static const char* g_sUpdateEntryQuery = "UPDATE %s SET %s WHERE ENTRY_ID=%d;";

/* Read Entries. Arg1 = List of column names, Arg2 = table name */
static const char* g_sReadQuery = "SELECT %s FROM %s";

/* Selectors for call lists */
static const char* g_sCallsListSelector1   = " WHERE CALL_TYPE=%d";
static const char* g_sCallsListSelector2   = " WHERE CALL_TYPE=%d OR CALL_TYPE=%d";

/* Select by Entry Id */
static const char* g_sEntryIdSelector       = " WHERE ENTRY_ID=%d";

/*******************************************
Field Names
********************************************/
static const char* g_psFieldNames[] =
{
    "ENTRY_ID",
    "LAST_NAME",
    "FIRST_NAME",
    "CONTACT_NUM_1",
    "CONTACT_NUM_1_TYPE",
    "CONTACT_NUM_1_OWN",
    "CONTACT_NUM_1_DEFAULT",
    "CONTACT_NUM_2",
    "CONTACT_NUM_2_TYPE",
    "CONTACT_NUM_2_OWN",
    "CONTACT_NUM_2_DEFAULT",
    "ASSOCIATED_MELODY",
    "LINE_ID",
    "NUMBER",
    "DATE_AND_TIME",
    "READ_STATUS",
    "LINE_NAME",
    "NUM_OF_CALLS",
    "CALL_TYPE",
    "ATTACHED_HANDSETS",
    "DIALING_PREFIX",
    "FP_MELODY",
    "FP_VOLUME",
    "BLOCKED_NUMBER",
    "MULTIPLE_CALLS_MODE",
    "INTRUSION_CALL",
    "PERMANENT_CLIR",
    "PERMANENT_CLIR_ACTIVATION_CODE",
    "PERMANENT_CLIR_DEACTIVATION_CODE",
    "CALL_FWD_UNCOND",
    "CALL_FWD_UNCOND_ACTIVATION_CODE",
    "CALL_FWD_UNCOND_DEACTIVATION_CODE",
    "CALL_FWD_UNCOND_TARGET_NUMBER",
    "CALL_FWD_NO_ANSWER",
    "CALL_FWD_NO_ANSWER_ACTIVATION_CODE",
    "CALL_FWD_NO_ANSWER_DEACTIVATION_CODE",
    "CALL_FWD_NO_ANSWER_TARGET_NUMBER",
    "CALL_FWD_NO_ANSWER_TIMEOUT",
    "CALL_FWD_BUSY",
    "CALL_FWD_BUSY_ACTIVATION_CODE",
    "CALL_FWD_BUSY_DEACTIVATION_CODE",
    "CALL_FWD_BUSY_TARGET_NUMBER"
};

/*******************************************
List Definition
********************************************/
/* Contact List */
static const stListField g_pst_ContactListFields[] =
{
    /*  Name      Types    AutoInc  Primary-Key  Mandatory Editable Field-Id      */
    /*  -----      -----    -------  -----------  --------- -------- --------      */
    {   "ENTRY_ID",                 FIELD_TYPE_INT,     1,          1,              1,          0,          FIELD_ID_ENTRY_ID},
    {   "LAST_NAME",                FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_LAST_NAME},
    {   "FIRST_NAME",               FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_FIRST_NAME},
    {   "CONTACT_NUM_1",            FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_1},
    {   "CONTACT_NUM_1_TYPE",       FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_1_TYPE},
    {   "CONTACT_NUM_1_DEFAULT",    FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_1_DEFAULT},
    {   "CONTACT_NUM_1_OWN",        FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_1_OWN},
    {   "CONTACT_NUM_2",            FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_2},
    {   "CONTACT_NUM_2_TYPE",       FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_2_TYPE},
    {   "CONTACT_NUM_2_DEFAULT",    FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_2_DEFAULT},
    {   "CONTACT_NUM_2_OWN",        FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CONTACT_NUM_2_OWN},
    {   "ASSOCIATED_MELODY",        FIELD_TYPE_INT,     0,          0,              0,          1,          FIELD_ID_ASSOCIATED_MELODY},
    {   "LINE_ID",                  FIELD_TYPE_INT,     0,          0,              0,          1,          FIELD_ID_LINE_ID}
};
#define CONTACT_LIST_FIELDS_NUM LENGTHOF(g_pst_ContactListFields)

/* Calls List */
static const stListField g_pst_CallsListFields[] =
{
    /*  Name      Types    AutoInc  Primary-Key  Mandatory Editable Field-Id      */
    /*  -----      -----    -------  -----------  --------- -------- --------      */
    {   "ENTRY_ID",                 FIELD_TYPE_INT,     1,          1,              1,          0,          FIELD_ID_ENTRY_ID},
    {   "NUMBER",                   FIELD_TYPE_TEXT,    0,          0,              1,          0,          FIELD_ID_NUMBER},
    {   "DATE_AND_TIME",            FIELD_TYPE_INT,     0,          0,              1,          0,          FIELD_ID_DATE_AND_TIME},
    {   "LINE_ID",                  FIELD_TYPE_INT,     0,          0,              1,          0,          FIELD_ID_LINE_ID},
    {   "NUM_OF_CALLS",             FIELD_TYPE_INT,     0,          0,              0,          1,          FIELD_ID_NUM_OF_CALLS},
    {   "READ_STATUS",              FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_READ_STATUS},
    {   "CALL_TYPE",                FIELD_TYPE_CHAR,    0,          0,              1,          0,          FIELD_ID_CALL_TYPE}
};
#define CALLS_LIST_FIELDS_NUM   LENGTHOF(g_pst_CallsListFields)

/* Line Settings List */
static const stListField g_pst_LineSettingsListFields[] =
{
    /*  Name                  Types    AutoInc  Primary-Key  Mandatory Editable Field-Id      */
    /*  -----                  -----    -------  -----------  --------- -------- --------      */
    {   "ENTRY_ID",                             FIELD_TYPE_INT,     1,          1,              1,          0,          FIELD_ID_ENTRY_ID},
    {   "LINE_NAME",                            FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_LINE_NAME},
    {   "LINE_ID",                              FIELD_TYPE_INT,     0,          0,              1,          0,          FIELD_ID_LINE_ID},
    {   "ATTACHED_HANDSETS",                    FIELD_TYPE_INT,     0,          0,              1,          1,          FIELD_ID_ATTACHED_HANDSETS},
    {   "DIALING_PREFIX",                       FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_DIALING_PREFIX},
    {   "FP_MELODY",                            FIELD_TYPE_INT,     0,          0,              0,          1,          FIELD_ID_FP_MELODY},
    {   "FP_VOLUME",                            FIELD_TYPE_INT,     0,          0,              0,          1,          FIELD_ID_FP_VOLUME},
    {   "BLOCKED_NUMBER",                       FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_BLOCKED_NUMBER},
    {   "MULTIPLE_CALLS_MODE",                  FIELD_TYPE_CHAR,    0,          0,              1,          1,          FIELD_ID_MULTIPLE_CALLS_MODE},
    {   "INTRUSION_CALL",                       FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_INTRUSION_CALL},
    {   "PERMANENT_CLIR",                       FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_PERMANENT_CLIR},
    {   "PERMANENT_CLIR_ACTIVATION_CODE",       FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_PERMANENT_CLIR_ACTIVATION_CODE},
    {   "PERMANENT_CLIR_DEACTIVATION_CODE",     FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_PERMANENT_CLIR_DEACTIVATION_CODE},
    {   "CALL_FWD_UNCOND",                      FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_UNCOND},
    {   "CALL_FWD_UNCOND_ACTIVATION_CODE",      FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_UNCOND_ACTIVATION_CODE},
    {   "CALL_FWD_UNCOND_DEACTIVATION_CODE",    FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_UNCOND_DEACTIVATION_CODE},
    {   "CALL_FWD_UNCOND_TARGET_NUMBER",        FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_UNCOND_TARGET_NUMBER},
    {   "CALL_FWD_NO_ANSWER",                   FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_NO_ANSWER},
    {   "CALL_FWD_NO_ANSWER_ACTIVATION_CODE",   FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_NO_ANSWER_ACTIVATION_CODE},
    {   "CALL_FWD_NO_ANSWER_DEACTIVATION_CODE", FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_NO_ANSWER_DEACTIVATION_CODE},
    {   "CALL_FWD_NO_ANSWER_TARGET_NUMBER",     FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_NO_ANSWER_TARGET_NUMBER},
    {   "CALL_FWD_NO_ANSWER_TIMEOUT",           FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_NO_ANSWER_TIMEOUT},
    {   "CALL_FWD_BUSY",                        FIELD_TYPE_CHAR,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_BUSY},
    {   "CALL_FWD_BUSY_ACTIVATION_CODE",        FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_BUSY_ACTIVATION_CODE},
    {   "CALL_FWD_BUSY_DEACTIVATION_CODE",      FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_BUSY_DEACTIVATION_CODE},
    {   "CALL_FWD_BUSY_TARGET_NUMBER",          FIELD_TYPE_TEXT,    0,          0,              0,          1,          FIELD_ID_CALL_FWD_BUSY_TARGET_NUMBER}
};

#define LINE_SETTINGS_LIST_FIELDS_NUM   LENGTHOF(g_pst_LineSettingsListFields)

/*******************************************
** Auxiliary
********************************************/
static bool CallList(IN LIST_TYPE eListType)
{
    switch (eListType)
    {
        case LIST_TYPE_MISSED_CALLS:
        case LIST_TYPE_OUTGOING_CALLS:
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
        case LIST_TYPE_ALL_CALLS:
        case LIST_TYPE_ALL_INCOMING_CALLS:
            return TRUE;

        default:
            return FALSE;
    }
}


/*******************************************
Lists API
********************************************/
LIST_RC List_Init(void)
{
    SQL_RC sql_rc;

    /* init database */
    sql_rc = SQL_InitDatabase();
    if (sql_rc != SQL_RC_OK)
    {
        printf("List_Init: failed init database\n");
        return LIST_RC_FAIL;
    }

    /* get database handle */
    sql_rc = SQL_GetDatabaseHandle(&DATABASE);
    if (sql_rc != SQL_RC_OK)
    {
        printf("List_Init: failed getting database handle\n");
        return LIST_RC_FAIL;
    }

    return LIST_RC_OK;
}

LIST_RC List_CreateList(IN LIST_TYPE ListType)
{
    char sQuery[SQL_QUERY_MAX_LEN];
    char sFields[SQL_QUERY_MAX_LEN];
    const stListField* pst_Fields;
    u32 u32_total, u32_index, u32_FieldsSize;
    SQL_RC sql_rc;

    assert(DATABASE);

    switch (ListType)
    {
        case LIST_TYPE_MISSED_CALLS:
        case LIST_TYPE_OUTGOING_CALLS:
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
        case LIST_TYPE_ALL_CALLS:
        case LIST_TYPE_ALL_INCOMING_CALLS:
            pst_Fields = g_pst_CallsListFields;
            u32_FieldsSize = CALLS_LIST_FIELDS_NUM;
            break;

        case LIST_TYPE_CONTACT_LIST:
            pst_Fields = g_pst_ContactListFields;
            u32_FieldsSize = CONTACT_LIST_FIELDS_NUM;
            break;

        case LIST_TYPE_LINE_SETTINGS_LIST:
            pst_Fields = g_pst_LineSettingsListFields;
            u32_FieldsSize = LINE_SETTINGS_LIST_FIELDS_NUM;
            break;

        default:
            printf("List_CreateList: Unsupported list %d\n", ListType);
            return LIST_RC_UNSUPPORTED_LIST;
    }

    /* build fields string */
    sFields[0] = 0;
    u32_total = 0;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index, ++pst_Fields)
    {
        if (u32_index > 0)
        {
            sFields[u32_total++]  = ',';
        }

        /* Field name */
        u32_total += snprintf(sFields + u32_total, sizeof(sFields) - u32_total, "%s ", pst_Fields->s_Name);

        /* Field type */
        assert(pst_Fields->e_Type < FIELD_TYPE_MAX);
        u32_total += snprintf(sFields + u32_total, sizeof(sFields) - u32_total, "%s", g_ps_TypeStrings[pst_Fields->e_Type]);

        /* Primary Key */
        if (pst_Fields->b_PrimaryKey)
        {
            u32_total += snprintf(sFields + u32_total, sizeof(sFields) - u32_total, " %s", sPrimaryKey);
        }

        /* Auto-increment */
        if (pst_Fields->b_AutoInc)
        {
            u32_total += snprintf(sFields + u32_total, sizeof(sFields) - u32_total, " %s", sAutoincrement);
        }

        /* Mandatory */
        if (pst_Fields->b_Mandatory)
        {
            u32_total += snprintf(sFields + u32_total, sizeof(sFields) - u32_total, " %s", sNotNull);
        }
    }

    /* prepare 'create table' query */
    snprintf(sQuery, sizeof(sQuery), g_sCreateTableQuery, g_ps_TableNameStrings[ListType], sFields);
    sQuery[sizeof(sQuery) - 1] = 0;

    /* execute in database */
    sql_rc = SQL_Execute(DATABASE, sQuery);
    if (sql_rc != SQL_RC_OK)
    {
        printf("List_CreateList: failed creating table %d\n", ListType);
        return LIST_RC_FAIL;
    }

    return LIST_RC_OK;
}

LIST_RC List_GetCount(IN LIST_TYPE ListType, OUT u32* pu32_Count)
{
    char sQuery[SQL_QUERY_MAX_LEN];
    SQL_RC sql_rc;
    void* pv_SelectHandle;
    bool bStub;
    u32 u32_QueryTotal = 0;
    LIST_RC list_rc = LIST_RC_FAIL;

    assert(DATABASE);

    /* prepare query */
    u32_QueryTotal += snprintf(sQuery + u32_QueryTotal, sizeof(sQuery) - u32_QueryTotal, g_sSelectCountQuery, g_ps_TableNameStrings[ListType]);
    if (CallList(ListType))
    {
        switch (ListType)
        {
            case LIST_TYPE_MISSED_CALLS:
                u32_QueryTotal += snprintf(sQuery + u32_QueryTotal, sizeof(sQuery) - u32_QueryTotal, g_sCallsListSelector1, CALL_TYPE_MISSED);
                break;
            case LIST_TYPE_OUTGOING_CALLS:
                u32_QueryTotal += snprintf(sQuery + u32_QueryTotal, sizeof(sQuery) - u32_QueryTotal, g_sCallsListSelector1, CALL_TYPE_OUTGOING);
                break;
            case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
                u32_QueryTotal += snprintf(sQuery + u32_QueryTotal, sizeof(sQuery) - u32_QueryTotal, g_sCallsListSelector1, CALL_TYPE_INCOMING);
                break;
            case LIST_TYPE_ALL_INCOMING_CALLS:
                u32_QueryTotal += snprintf(sQuery + u32_QueryTotal, sizeof(sQuery) - u32_QueryTotal, g_sCallsListSelector2, CALL_TYPE_MISSED, CALL_TYPE_INCOMING);
                break;
            default:
                break;
        }
    }

    u32_QueryTotal += snprintf(sQuery + u32_QueryTotal, sizeof(sQuery) - u32_QueryTotal, ";");
    sQuery[sizeof(sQuery) - 1] = 0;

    /* perform select */
    sql_rc = SQL_Select(DATABASE, sQuery, FALSE, NULL, &pv_SelectHandle);
    if (sql_rc == SQL_RC_OK)
    {
        sql_rc = SQL_FetchRow(pv_SelectHandle, &bStub);
        if (sql_rc == SQL_RC_OK)
        {
            sql_rc = SQL_GetUValByIdx(pv_SelectHandle, 0, pu32_Count);
            if (sql_rc == SQL_RC_OK)
            {
                list_rc = LIST_RC_OK;
            }
        }
    }

    /* free memory */
    SQL_ReleaseSelectHandle(pv_SelectHandle);

    if (list_rc == LIST_RC_FAIL)
    {
        printf("List_GetCount: failed fetching count\n");
    }

    return list_rc;
}

LIST_RC List_GetSupportedFields(IN LIST_TYPE ListType,
                                OUT u32 pu32_EditableFieldsIds[],     INOUT u32* pu32_EditableSize,
                                OUT u32 pu32_NonEditableFieldsIds[],  INOUT u32* pu32_NonEditableSize)
{
    const stListField* pst_Fields;
    u32 u32_FieldsSize, u32_index;

    /* sample input array sizes */
    const u32 u32_EditableSize    = *pu32_EditableSize;
    const u32 u32_NonEditableSize = *pu32_NonEditableSize;

    switch (ListType)
    {

        case LIST_TYPE_MISSED_CALLS:
        case LIST_TYPE_OUTGOING_CALLS:
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
        case LIST_TYPE_ALL_CALLS:
        case LIST_TYPE_ALL_INCOMING_CALLS:
            pst_Fields = g_pst_CallsListFields;
            u32_FieldsSize = CALLS_LIST_FIELDS_NUM;
            break;

        case LIST_TYPE_CONTACT_LIST:
            pst_Fields = g_pst_ContactListFields;
            u32_FieldsSize = CONTACT_LIST_FIELDS_NUM;
            break;

        case LIST_TYPE_LINE_SETTINGS_LIST:
            pst_Fields = g_pst_LineSettingsListFields;
            u32_FieldsSize = LINE_SETTINGS_LIST_FIELDS_NUM;
            break;

        default:
            printf("List_GetSupportedFields: Unsupported list %d\n", ListType);
            return LIST_RC_UNSUPPORTED_LIST;
    }

    /* prepare output arrays */
    *pu32_EditableSize = *pu32_NonEditableSize = 0;
    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        /* tweaking for calls list... */
        if (
            (
                /* From missed calls, incoming calls we do not need the call type field */
                (ListType == LIST_TYPE_MISSED_CALLS || ListType == LIST_TYPE_ALL_INCOMING_CALLS) &&
                pst_Fields[u32_index].e_FieldId == FIELD_ID_CALL_TYPE
            )
            ||
            (
                /* From outgoing and incoming accepted calls we do not need the fields: call type, read status, num of calls */
                (ListType == LIST_TYPE_OUTGOING_CALLS || ListType == LIST_TYPE_INCOMING_ACCEPTED_CALLS) &&
                (pst_Fields[u32_index].e_FieldId == FIELD_ID_CALL_TYPE || pst_Fields[u32_index].e_FieldId == FIELD_ID_READ_STATUS || pst_Fields[u32_index].e_FieldId == FIELD_ID_NUM_OF_CALLS)
            )
            ||
            (
                /* From All calls list we do not need read status, num of calls */
                ListType == LIST_TYPE_ALL_CALLS &&
                (pst_Fields[u32_index].e_FieldId == FIELD_ID_READ_STATUS || pst_Fields[u32_index].e_FieldId == FIELD_ID_NUM_OF_CALLS)
            )
        )
        {
            continue;
        }

        if (pst_Fields[u32_index].b_Editable && (*pu32_EditableSize < u32_EditableSize))
        {
            pu32_EditableFieldsIds[(*pu32_EditableSize)++] = pst_Fields[u32_index].e_FieldId;
        }
        else if ((pst_Fields[u32_index].b_Editable == FALSE) && (*pu32_NonEditableSize < u32_NonEditableSize))
        {
            pu32_NonEditableFieldsIds[(*pu32_NonEditableSize)++] = pst_Fields[u32_index].e_FieldId;
        }
        else
        {
            printf("List_GetSupportedFields: Not enough memory\n");
            return LIST_RC_ARRAY_TOO_SMALL;
        }
    }

    /* For calls list we also support First name, LAst name and Line name, although not in the list in database */
    if (CallList(ListType))
    {
        pu32_NonEditableFieldsIds[(*pu32_NonEditableSize)++] = FIELD_ID_LAST_NAME;
        pu32_NonEditableFieldsIds[(*pu32_NonEditableSize)++] = FIELD_ID_FIRST_NAME;
        pu32_NonEditableFieldsIds[(*pu32_NonEditableSize)++] = FIELD_ID_LINE_NAME;
    }

    return LIST_RC_OK;
}

LIST_RC List_UpdateEntry(IN LIST_TYPE ListType, IN void* pv_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize, IN u32 u32_EntryId)
{
    char sQuery[SQL_QUERY_MAX_LEN];
    char sFields[SQL_QUERY_MAX_LEN];
    u32 u32_sFieldsSize = sizeof(sFields);
    LIST_RC rc;

    /* Prepare lists of fields and values */
    switch (ListType)
    {
        case LIST_TYPE_MISSED_CALLS:
        case LIST_TYPE_OUTGOING_CALLS:
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
        case LIST_TYPE_ALL_CALLS:
        case LIST_TYPE_ALL_INCOMING_CALLS:
            rc = List_PrepareCallsListEntryForUpdate(pv_Entry, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsSize);
            break;

        case LIST_TYPE_CONTACT_LIST:
            rc = List_PrepareContactListEntryForUpdate(pv_Entry, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsSize);
            break;

        case LIST_TYPE_LINE_SETTINGS_LIST:
            rc = List_PrepareLineSettingsListEntryForUpdate(pv_Entry, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsSize);
            break;

        default:
            printf("List_UpdateEntry: Unsupported list %d\n", ListType);
            rc = LIST_RC_UNSUPPORTED_LIST;
            break;
    }

    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    /* Prepare Update Query */
    snprintf(sQuery, sizeof(sQuery), g_sUpdateEntryQuery, g_ps_TableNameStrings[ListType], sFields, u32_EntryId);
    sQuery[sizeof(sQuery) - 1] = 0;

    /* Execute in database */
    assert(DATABASE);
    if (SQL_Execute(DATABASE, sQuery) != SQL_RC_OK)
    {
        return LIST_RC_FAIL;
    }

    return LIST_RC_OK;
}


LIST_RC List_InsertEntry(IN LIST_TYPE ListType, IN void* pv_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize, OUT u32* pu32_EntryId)
{
    char sQuery[SQL_QUERY_MAX_LEN];
    char sFields[SQL_QUERY_MAX_LEN];
    char sValues[SQL_QUERY_MAX_LEN];
    u32 u32_sFieldsSize = sizeof(sFields);
    u32 u32_sValuesSize = sizeof(sValues);
    LIST_RC rc;

    /* Prepare lists of fields and values */
    switch (ListType)
    {
        case LIST_TYPE_MISSED_CALLS:
        case LIST_TYPE_OUTGOING_CALLS:
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
        case LIST_TYPE_ALL_CALLS:
        case LIST_TYPE_ALL_INCOMING_CALLS:
            rc = List_PrepareCallsListEntryForInsert(ListType, pv_Entry, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsSize, sValues, &u32_sValuesSize);
            break;

        case LIST_TYPE_CONTACT_LIST:
            rc = List_PrepareContactListEntryForInsert(pv_Entry, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsSize, sValues, &u32_sValuesSize);
            break;

        case LIST_TYPE_LINE_SETTINGS_LIST:
            rc = List_PrepareLineSettingsListEntryForInsert(pv_Entry, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsSize, sValues, &u32_sValuesSize);
            break;

        default:
            printf("List_InsertEntry: Unsupported list %d\n", ListType);
            rc = LIST_RC_UNSUPPORTED_LIST;
            break;
    }

    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    /* Prepare Insert Query */
    snprintf(sQuery, sizeof(sQuery), g_sInsertQuery, g_ps_TableNameStrings[ListType], sFields, sValues);
    sQuery[sizeof(sQuery) - 1] = 0;

    /* Execute in database */
    assert(DATABASE);
    if (SQL_Execute(DATABASE, sQuery) != SQL_RC_OK)
    {
        return LIST_RC_FAIL;
    }

    /* Get Entry Id */
    if (SQL_GetIdOfLastInsert(DATABASE, pu32_EntryId) != SQL_RC_OK)
    {
        return LIST_RC_FAIL;
    }

    return LIST_RC_OK;
}

LIST_RC List_PrepareContactListEntryForInsert(IN stContactListEntry* pst_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
        OUT char sFields[], INOUT u32* pu32_FieldsSize,
        OUT char sValues[], INOUT u32* pu32_ValuesSize)
{
    u32 u32_FieldsTotal = 0, u32_ValuesTotal = 0, u32_index = 0;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        if (u32_index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
            u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",");
        }

        /* Field Name */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s", g_psFieldNames[pu32Fields[u32_index]]);

        /* Field Value */
        switch (pu32Fields[u32_index])
        {
            case FIELD_ID_LAST_NAME:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sLastName);
                break;

            case FIELD_ID_FIRST_NAME:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sFirstName);
                break;

            case FIELD_ID_CONTACT_NUM_1:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sNumber1);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CONTACT_NUM_1_TYPE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CONTACT_NUM_1_DEFAULT]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CONTACT_NUM_1_OWN]);

                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", pst_Entry->cNumber1Type);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", pst_Entry->bNumber1Default);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", pst_Entry->bNumber1Own);
                break;

            case FIELD_ID_CONTACT_NUM_2:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sNumber2);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CONTACT_NUM_2_TYPE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CONTACT_NUM_2_DEFAULT]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CONTACT_NUM_2_OWN]);

                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", pst_Entry->cNumber2Type);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", pst_Entry->bNumber2Default);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", pst_Entry->bNumber2Own);
                break;

            case FIELD_ID_ASSOCIATED_MELODY:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_AssociatedMelody);
                break;

            case FIELD_ID_LINE_ID:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_LineId);
                break;

            default:
                printf("List_PrepareContactListEntryForInsert: Unknown field id %d\n", pu32Fields[u32_index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;
    sValues[u32_ValuesTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;
    *pu32_ValuesSize = u32_ValuesTotal;

    return LIST_RC_OK;
}

LIST_RC List_PrepareCallsListEntryForInsert(IN LIST_TYPE ListType, IN stCallsListEntry* pst_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
        OUT char sFields[], INOUT u32* pu32_FieldsSize,
        OUT char sValues[], INOUT u32* pu32_ValuesSize)
{
    u32 u32_FieldsTotal = 0, u32_ValuesTotal = 0, u32_index = 0;
    bool bReadStatusSet = FALSE;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        if (u32_index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
            u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",");
        }

        /* Field Name */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s", g_psFieldNames[pu32Fields[u32_index]]);

        /* Field Value */
        switch (pu32Fields[u32_index])
        {
            case FIELD_ID_NUMBER:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sNumber);
                break;

            case FIELD_ID_DATE_AND_TIME:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%ld", pst_Entry->t_DateAndTime);
                break;

            case FIELD_ID_READ_STATUS:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bRead);
                bReadStatusSet = TRUE;
                break;

            case FIELD_ID_LINE_ID:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_LineId);
                break;

            case FIELD_ID_NUM_OF_CALLS:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_NumOfCalls);
                break;

            case FIELD_ID_CALL_TYPE:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->cCallType);
                break;

            default:
                printf("List_PrepareCallsListEntryForInsert: Unknown field id %d\n", pu32Fields[u32_index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    /* add call type */
    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_TYPE]);
    switch (ListType)
    {
        case LIST_TYPE_MISSED_CALLS:
            u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", CALL_TYPE_MISSED);
            break;
        case LIST_TYPE_OUTGOING_CALLS:
            u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", CALL_TYPE_OUTGOING);
            break;
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
            u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", CALL_TYPE_INCOMING);
            break;
        default:
            break;
    }

    if (bReadStatusSet == FALSE)
    {
        /* set defualt value = read */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_READ_STATUS]);
        u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%u", 1);
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;
    sValues[u32_ValuesTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;
    *pu32_ValuesSize = u32_ValuesTotal;

    return LIST_RC_OK;
}

LIST_RC List_PrepareLineSettingsListEntryForInsert(IN stLineSettingsListEntry* pst_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
        OUT char sFields[], INOUT u32* pu32_FieldsSize,
        OUT char sValues[], INOUT u32* pu32_ValuesSize)
{
    u32 u32_FieldsTotal = 0, u32_ValuesTotal = 0, u32_index = 0;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        if (u32_index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
            u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",");
        }

        /* Field Name */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s", g_psFieldNames[pu32Fields[u32_index]]);

        /* Field Value */
        switch (pu32Fields[u32_index])
        {
            case FIELD_ID_LINE_NAME:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sLineName);
                break;

            case FIELD_ID_LINE_ID:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_LineId);
                break;

            case FIELD_ID_ATTACHED_HANDSETS:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_AttachedHsMask);
                break;

            case FIELD_ID_DIALING_PREFIX:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sDialPrefix);
                break;

            case FIELD_ID_FP_MELODY:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_FPMelody);
                break;

            case FIELD_ID_FP_VOLUME:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->u32_FPVolume);
                break;

            case FIELD_ID_BLOCKED_NUMBER:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "\'%s\'", pst_Entry->sBlockedNumber);
                break;

            case FIELD_ID_MULTIPLE_CALLS_MODE:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bMultiCalls);
                break;

            case FIELD_ID_INTRUSION_CALL:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bIntrusionCall);
                break;

            case FIELD_ID_PERMANENT_CLIR:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bPermanentCLIR);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_PERMANENT_CLIR_ACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_PERMANENT_CLIR_DEACTIVATION_CODE]);

                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sPermanentCLIRActCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sPermanentCLIRDeactCode);
                break;

            case FIELD_ID_CALL_FWD_UNCOND:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bCallFwdUncond);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_ACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_DEACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_TARGET_NUMBER]);

                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdUncondActCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdUncondDeactCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdUncondNum);
                break;

            case FIELD_ID_CALL_FWD_NO_ANSWER:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bCallFwdNoAns);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_ACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_DEACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TARGET_NUMBER]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TIMEOUT]);

                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdNoAnsActCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdNoAnsDeactCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdNoAnsNum);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",%d", pst_Entry->u32_CallFwdNoAnsTimeout);
                break;

            case FIELD_ID_CALL_FWD_BUSY:
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, "%u", pst_Entry->bCallFwdBusy);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_ACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_DEACTIVATION_CODE]);
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s", g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_TARGET_NUMBER]);

                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdBusyActCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdBusyDeactCode);
                u32_ValuesTotal += snprintf(sValues + u32_ValuesTotal, *pu32_ValuesSize - u32_ValuesTotal, ",\'%s\'", pst_Entry->sCallFwdBusyNum);
                break;

            default:
                printf("List_PrepareLineSettingsListEntryForInsert: Unknown field id %d\n", pu32Fields[u32_index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;
    sValues[u32_ValuesTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;
    *pu32_ValuesSize = u32_ValuesTotal;

    return LIST_RC_OK;
}

LIST_RC List_DeleteEntry(IN LIST_TYPE ListType, IN u32 u32_EntryId)
{
    char sQuery[SQL_QUERY_MAX_LEN];

    if (ListType >= LIST_TYPE_MAX)
    {
        printf("List_DeleteEntry: Unknown List %d\n", ListType);
        return LIST_RC_UNSUPPORTED_LIST;
    }

    snprintf(sQuery, sizeof(sQuery), g_sDeleteEntryQuery, g_ps_TableNameStrings[ListType], u32_EntryId);
    sQuery[sizeof(sQuery) - 1] = 0;

    assert(DATABASE);
    if (SQL_Execute(DATABASE, sQuery) != SQL_RC_OK)
    {
        return LIST_RC_FAIL;
    }

    return LIST_RC_OK;
}

LIST_RC List_DeleteAllEntries(IN LIST_TYPE ListType)
{
    char sQuery[SQL_QUERY_MAX_LEN];
    u16 u16_QueryLen = 0;

    if (ListType >= LIST_TYPE_MAX)
    {
        printf("List_DeleteAllEntries: Unknown List %d\n", ListType);
        return LIST_RC_UNSUPPORTED_LIST;
    }

    /* Delete all entries */
    u16_QueryLen = snprintf(sQuery, sizeof(sQuery), g_sDeleteAllQuery, g_ps_TableNameStrings[ListType]);

    if (CallList(ListType) && (ListType != LIST_TYPE_ALL_CALLS))
    {
        u8 u8_CallType;
        switch (ListType)
        {
            case LIST_TYPE_MISSED_CALLS:
                u8_CallType = CALL_TYPE_MISSED;
                break;

            case LIST_TYPE_OUTGOING_CALLS:
                u8_CallType = CALL_TYPE_OUTGOING;
                break;

            default:
                u8_CallType = CALL_TYPE_INCOMING;
                break;
        }
        u16_QueryLen += snprintf(sQuery + u16_QueryLen, sizeof(sQuery) - u16_QueryLen, g_sCallsListSelector1, u8_CallType);
    }

    sQuery[u16_QueryLen++] = ';';
    sQuery[u16_QueryLen] = 0;

    assert(DATABASE);
    if (SQL_Execute(DATABASE, sQuery) != SQL_RC_OK)
    {
        return LIST_RC_FAIL;
    }

    /* reset auto-increment fields to 1 */
    if (!CallList(ListType) || (ListType == LIST_TYPE_ALL_CALLS))
    {
        snprintf(sQuery, sizeof(sQuery), g_sResetAutoIncQuery, g_ps_TableNameStrings[ListType]);
        sQuery[sizeof(sQuery) - 1] = 0;

        assert(DATABASE);
        if (SQL_Execute(DATABASE, sQuery) != SQL_RC_OK)
        {
            return LIST_RC_FAIL;
        }
    }

    if (ListType == LIST_TYPE_ALL_INCOMING_CALLS)
    {
        /* need to delete also the missed call list */
        return List_DeleteAllEntries(LIST_TYPE_MISSED_CALLS);
    }

    return LIST_RC_OK;
}

LIST_RC List_PrepareContactListEntryForUpdate(IN stContactListEntry* pst_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
        OUT char sFields[], INOUT u32* pu32_FieldsSize)
{
    u32 u32_FieldsTotal = 0, u32_index = 0;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        if (u32_index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
        }

        /* Field Name */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s=", g_psFieldNames[pu32Fields[u32_index]]);

        /* Field Value */
        switch (pu32Fields[u32_index])
        {
            case FIELD_ID_LAST_NAME:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sLastName);
                break;

            case FIELD_ID_FIRST_NAME:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sFirstName);
                break;

            case FIELD_ID_CONTACT_NUM_1:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sNumber1);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%u",
                                            g_psFieldNames[FIELD_ID_CONTACT_NUM_1_TYPE], pst_Entry->cNumber1Type);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%u",
                                            g_psFieldNames[FIELD_ID_CONTACT_NUM_1_DEFAULT], pst_Entry->bNumber1Default);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%u",
                                            g_psFieldNames[FIELD_ID_CONTACT_NUM_1_OWN], pst_Entry->bNumber1Own);
                break;

            case FIELD_ID_CONTACT_NUM_2:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sNumber2);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%u",
                                            g_psFieldNames[FIELD_ID_CONTACT_NUM_2_TYPE], pst_Entry->cNumber2Type);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%u",
                                            g_psFieldNames[FIELD_ID_CONTACT_NUM_2_DEFAULT], pst_Entry->bNumber2Default);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%u",
                                            g_psFieldNames[FIELD_ID_CONTACT_NUM_2_OWN], pst_Entry->bNumber2Own);
                break;

            case FIELD_ID_ASSOCIATED_MELODY:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_AssociatedMelody);
                break;

            case FIELD_ID_LINE_ID:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_LineId);
                break;

            default:
                printf("List_PrepareContactListEntryForUpdate: Unknown field id %d\n", pu32Fields[u32_index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;

    return LIST_RC_OK;
}

LIST_RC List_PrepareCallsListEntryForUpdate(IN stCallsListEntry* pst_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
        OUT char sFields[], INOUT u32* pu32_FieldsSize)
{
    u32 u32_FieldsTotal = 0, u32_index = 0;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        if (u32_index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
        }

        /* Field Name */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s=", g_psFieldNames[pu32Fields[u32_index]]);

        /* Field Value */
        switch (pu32Fields[u32_index])
        {
            case FIELD_ID_NUMBER:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sNumber);
                break;

            case FIELD_ID_DATE_AND_TIME:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%ld", pst_Entry->t_DateAndTime);
                break;

            case FIELD_ID_READ_STATUS:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bRead);
                break;

            case FIELD_ID_LINE_ID:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_LineId);
                break;

            case FIELD_ID_NUM_OF_CALLS:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_NumOfCalls);
                break;

            case FIELD_ID_CALL_TYPE:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->cCallType);
                break;

            default:
                printf("List_PrepareCallsListEntryForUpdate: Unknown field id %d\n", pu32Fields[u32_index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;

    return LIST_RC_OK;
}

LIST_RC List_PrepareLineSettingsListEntryForUpdate(IN stLineSettingsListEntry* pst_Entry, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
        OUT char sFields[], INOUT u32* pu32_FieldsSize)
{
    u32 u32_FieldsTotal = 0, u32_index = 0;

    for (u32_index = 0; u32_index < u32_FieldsSize; ++u32_index)
    {
        if (u32_index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
        }

        /* Field Name */
        u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s=", g_psFieldNames[pu32Fields[u32_index]]);

        /* Field Value */
        switch (pu32Fields[u32_index])
        {
            case FIELD_ID_LINE_NAME:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sLineName);
                break;

            case FIELD_ID_LINE_ID:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_LineId);
                break;

            case FIELD_ID_ATTACHED_HANDSETS:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_AttachedHsMask);
                break;

            case FIELD_ID_DIALING_PREFIX:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sDialPrefix);
                break;

            case FIELD_ID_FP_MELODY:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_FPMelody);
                break;

            case FIELD_ID_FP_VOLUME:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->u32_FPVolume);
                break;

            case FIELD_ID_BLOCKED_NUMBER:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "\'%s\'", pst_Entry->sBlockedNumber);
                break;

            case FIELD_ID_MULTIPLE_CALLS_MODE:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bMultiCalls);
                break;

            case FIELD_ID_INTRUSION_CALL:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bIntrusionCall);
                break;

            case FIELD_ID_PERMANENT_CLIR:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bPermanentCLIR);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_PERMANENT_CLIR_ACTIVATION_CODE], pst_Entry->sPermanentCLIRActCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_PERMANENT_CLIR_DEACTIVATION_CODE], pst_Entry->sPermanentCLIRDeactCode);
                break;

            case FIELD_ID_CALL_FWD_UNCOND:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bCallFwdUncond);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_ACTIVATION_CODE], pst_Entry->sCallFwdUncondActCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_DEACTIVATION_CODE], pst_Entry->sCallFwdUncondDeactCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_TARGET_NUMBER], pst_Entry->sCallFwdUncondNum);
                break;

            case FIELD_ID_CALL_FWD_NO_ANSWER:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bCallFwdNoAns);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_ACTIVATION_CODE], pst_Entry->sCallFwdNoAnsActCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_DEACTIVATION_CODE], pst_Entry->sCallFwdNoAnsDeactCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TARGET_NUMBER], pst_Entry->sCallFwdNoAnsNum);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=%d",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TIMEOUT], pst_Entry->u32_CallFwdNoAnsTimeout);
                break;

            case FIELD_ID_CALL_FWD_BUSY:
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%u", pst_Entry->bCallFwdBusy);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_ACTIVATION_CODE], pst_Entry->sCallFwdBusyActCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_DEACTIVATION_CODE], pst_Entry->sCallFwdBusyDeactCode);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s=\'%s\'",
                                            g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_TARGET_NUMBER], pst_Entry->sCallFwdBusyNum);
                break;

            default:
                printf("List_PrepareLineSettingsListEntryForUpdate: Unknown field id %d\n", pu32Fields[u32_index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;

    return LIST_RC_OK;
}

LIST_RC List_PrepareFieldsNamesForRead(IN LIST_TYPE ListType, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
                                       OUT char sFields[], INOUT u32* pu32_FieldsSize,
                                       OUT bool* pb_JoinContactList, OUT bool* pb_JoinLineSettingsList)
{
    u32 u32_Index = 0, u32_FieldsTotal = 0;

    *pb_JoinContactList = *pb_JoinLineSettingsList = FALSE;

    if (ListType >= LIST_TYPE_MAX)
    {
        printf("List_PrepareFieldsNamesForRead: Unknown list %d\n", ListType);
        return LIST_RC_UNSUPPORTED_LIST;
    }

    for (u32_Index = 0; u32_Index < u32_FieldsSize; ++u32_Index)
    {
        if (u32_Index > 0)
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",");
        }

        if (pu32Fields[u32_Index] >= FIELD_ID_MAX)
        {
            printf("List_PrepareFieldsNamesForRead: Unknown Field %d\n", pu32Fields[u32_Index]);
            return LIST_RC_UNKNOWN_FIELD;
        }

        if (CallList(ListType))
        {
            if ((pu32Fields[u32_Index] == FIELD_ID_LAST_NAME) || (pu32Fields[u32_Index] == FIELD_ID_FIRST_NAME))
            {
                /* in case of calls list, we take the name from the contact list */
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s.%s",
                                            g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST], g_psFieldNames[pu32Fields[u32_Index]]);

                *pb_JoinContactList = TRUE;
            }
            else if (pu32Fields[u32_Index] == FIELD_ID_LINE_NAME)
            {
                /* in case of calls list, we take the Line name from the Line Settings list */
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s.%s",
                                            g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST], g_psFieldNames[pu32Fields[u32_Index]]);

                *pb_JoinLineSettingsList = TRUE;
            }
            else
            {
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[pu32Fields[u32_Index]]);
            }
        }
        else
        {
            u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, "%s.%s",
                                        g_ps_TableNameStrings[ListType], g_psFieldNames[pu32Fields[u32_Index]]);

            if ((ListType == LIST_TYPE_CONTACT_LIST) && (pu32Fields[u32_Index] == FIELD_ID_CONTACT_NUM_1))
            {
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CONTACT_NUM_1_TYPE]);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CONTACT_NUM_1_OWN]);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CONTACT_NUM_1_DEFAULT]);
            }
            else if ((ListType == LIST_TYPE_CONTACT_LIST) && (pu32Fields[u32_Index] == FIELD_ID_CONTACT_NUM_2))
            {
                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CONTACT_NUM_2_TYPE]);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CONTACT_NUM_2_OWN]);

                u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                            g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CONTACT_NUM_2_DEFAULT]);
            }
            else if (ListType == LIST_TYPE_LINE_SETTINGS_LIST)
            {
                if (pu32Fields[u32_Index] == FIELD_ID_CALL_FWD_UNCOND)
                {
                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_ACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_DEACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_TARGET_NUMBER]);
                }
                else if (pu32Fields[u32_Index] == FIELD_ID_CALL_FWD_NO_ANSWER)
                {
                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_ACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_DEACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TARGET_NUMBER]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TIMEOUT]);
                }
                else if (pu32Fields[u32_Index] == FIELD_ID_CALL_FWD_BUSY)
                {
                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_ACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_DEACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_TARGET_NUMBER]);
                }
                else if (pu32Fields[u32_Index] == FIELD_ID_PERMANENT_CLIR)
                {
                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_PERMANENT_CLIR_ACTIVATION_CODE]);

                    u32_FieldsTotal += snprintf(sFields + u32_FieldsTotal, *pu32_FieldsSize - u32_FieldsTotal, ",%s.%s",
                                                g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_PERMANENT_CLIR_DEACTIVATION_CODE]);
                }
            }
        }
    }

    /* Null-character */
    sFields[u32_FieldsTotal] = 0;

    *pu32_FieldsSize = u32_FieldsTotal;

    return LIST_RC_OK;
}

LIST_RC List_ReadEntries(IN LIST_TYPE ListType, INOUT u32* pu32_StartIndex, IN bool bForward, IN eMarkRequest eMark, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
                         IN u32 u32_SortField1, IN u32 u32_SortField2, OUT void* pv_Entries, INOUT u32* pu32_EntriesSize)
{
    char sQuery[SQL_QUERY_MAX_LEN * 2];
    char sFields[SQL_QUERY_MAX_LEN];
    u32 u32_sFieldsLen = sizeof(sFields);
    u32 u32_sQueryTotal = 0, u32_sQuerySize = sizeof(sQuery);
    bool bJoinContactList = FALSE, bJoinLineSettingsList = FALSE, bMoreEntries = FALSE;
    LIST_RC rc;
    SQL_RC sql_rc;
    ST_COLUMN_MAP stColumnMap; /* used to access the select result in random order */
    void* pv_SelectHandle;

    /* Prepare fields names */
    rc = List_PrepareFieldsNamesForRead(ListType, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsLen, &bJoinContactList, &bJoinLineSettingsList);
    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    /* Prepare query */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, g_sReadQuery, sFields, g_ps_TableNameStrings[ListType]);

    /* JOIN for First/Last Name - in case of calls list, and name was requested - we get it from contact list */
    if (bJoinContactList)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LEFT JOIN %s ON %s.%s=%s.%s",
                                    g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST],
                                    g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_NUMBER],
                                    g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST], g_psFieldNames[FIELD_ID_CONTACT_NUM_1]);
    }
    /* JOIN for Line NAme - in case of calls list, and Line name was requested - we get it from Line Settings list */
    if (bJoinLineSettingsList)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LEFT JOIN %s ON %s.%s=%s.%s",
                                    g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST],
                                    g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_LINE_ID],
                                    g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST], g_psFieldNames[FIELD_ID_LINE_ID]);
    }

    /* add filtering for call lists */
    if (CallList(ListType))
    {
        switch (ListType)
        {
            case LIST_TYPE_MISSED_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, g_sCallsListSelector1, CALL_TYPE_MISSED);
                break;
            case LIST_TYPE_OUTGOING_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, g_sCallsListSelector1, CALL_TYPE_OUTGOING);
                break;
            case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, g_sCallsListSelector1, CALL_TYPE_INCOMING);
                break;
            case LIST_TYPE_ALL_INCOMING_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, g_sCallsListSelector2, CALL_TYPE_MISSED, CALL_TYPE_INCOMING);
                break;
            default:
                break;
        }
    }

    /* Sort */
    if ((s32)u32_SortField1 != FIELD_ID_INVALID)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " ORDER BY %s", g_psFieldNames[u32_SortField1]);
        if (u32_SortField1 == FIELD_ID_FIRST_NAME || u32_SortField1 == FIELD_ID_LAST_NAME)
        {
            /* Use case insensitive sort when sorting by name */
            u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " COLLATE NOCASE");
        }

        if ((s32)u32_SortField2 != FIELD_ID_INVALID)
        {
            u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, ",%s", g_psFieldNames[u32_SortField2]);

            if (u32_SortField1 == FIELD_ID_FIRST_NAME || u32_SortField1 == FIELD_ID_LAST_NAME)
            {
                /* Use case insensitive sort when sorting by name */
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " COLLATE NOCASE");
            }
        }

        if (u32_SortField1 == FIELD_ID_DATE_AND_TIME)
        {
            /* when using date and time sorting, newest entry should be first */
            u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " %s", "DESC");
        }
    }

    /* Limit */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LIMIT %d", *pu32_EntriesSize);

    /* Offset */
    if (*pu32_StartIndex > 1)
    {
        if ((bForward == FALSE) && ((s32)*pu32_StartIndex - (s32)*pu32_EntriesSize < 0))
        {
            return LIST_RC_INVALID_START_INDEX;
        }
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " OFFSET %d",
                                    bForward ? *pu32_StartIndex - 1 : *pu32_StartIndex - *pu32_EntriesSize);

        if (bForward == FALSE)
        {
            /* returned start index should be the minimum of returned entries' indices */
            *pu32_StartIndex = *pu32_StartIndex - *pu32_EntriesSize + 1;
        }
    }

    sQuery[u32_sQueryTotal++] = ';';
    sQuery[u32_sQueryTotal] = 0;

    /* Now select */
    assert(DATABASE);
    sql_rc = SQL_Select(DATABASE, sQuery, TRUE, &stColumnMap, &pv_SelectHandle);
    if (sql_rc == SQL_RC_OK)
    {
        /* num of returned entries */
        *pu32_EntriesSize = 0;

        while (TRUE)
        {
            sql_rc = SQL_FetchRow(pv_SelectHandle, &bMoreEntries);
            if (sql_rc != SQL_RC_OK)
            {
                SQL_ReleaseSelectHandle(pv_SelectHandle);
                return LIST_RC_FAIL;
            }

            if (bMoreEntries)
            {
                List_FillEntry(ListType, pu32Fields, u32_FieldsSize, pv_Entries, *pu32_EntriesSize, pv_SelectHandle, &stColumnMap);

                /* Mark as read */
                if (eMark != MARK_LEAVE_UNCHANGED && CallList(ListType))
                {
                    stCallsListEntry* pCallEntry = (stCallsListEntry*)((stCallsListEntry*)pv_Entries + *pu32_EntriesSize);
                    stCallsListEntry DummyEntry;

                    u32 u32_FieldId = FIELD_ID_CALL_TYPE;

                    List_ReadEntryById(ListType, pCallEntry->u32_EntryId, &u32_FieldId, 1, &DummyEntry);

                    if (DummyEntry.cCallType == CALL_TYPE_MISSED)
                    {
                        u32_FieldId = FIELD_ID_READ_STATUS;
                        DummyEntry.bRead = (eMark == MARK_CLEAR) ? TRUE : FALSE;
                        List_UpdateEntry(ListType, &DummyEntry, &u32_FieldId, 1, pCallEntry->u32_EntryId);
                    }
                }

                /* Incement num of returned entries */
                ++(*pu32_EntriesSize);
            }
            else
            {
                /* We're done! */
                SQL_ReleaseSelectHandle(pv_SelectHandle);
                return LIST_RC_OK;
            }
        }
    }

    return LIST_RC_FAIL;
}

LIST_RC List_ReadEntryById(IN LIST_TYPE ListType, IN u32 u32_EntryId, IN u32 pu32Fields[], IN u32 u32_FieldsSize, OUT void* pv_Entry)
{
    char sQuery[SQL_QUERY_MAX_LEN * 2];
    char sFields[SQL_QUERY_MAX_LEN];
    u32 u32_sFieldsLen = sizeof(sFields);
    u32 u32_sQueryTotal = 0, u32_sQuerySize = sizeof(sQuery);
    bool bJoinContactList = FALSE, bJoinLineSettingsList = FALSE, bEntryAvailable = FALSE;

    LIST_RC rc;
    SQL_RC sql_rc;
    ST_COLUMN_MAP stColumnMap; /* used to access the select result in random order */
    void* pv_SelectHandle;

    /* Prepare fields names */
    rc = List_PrepareFieldsNamesForRead(ListType, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsLen, &bJoinContactList, &bJoinLineSettingsList);
    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    /* Prepare query */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, g_sReadQuery, sFields, g_ps_TableNameStrings[ListType]);

    /* Specify Entry Id */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, g_sEntryIdSelector, u32_EntryId);

    /* JOIN for First/Last Name - in case of calls list, and name was requested - we get it from contact list */
    if (bJoinContactList)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LEFT JOIN %s ON %s.%s=%s.%s",
                                    g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST],
                                    g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_NUMBER],
                                    g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST], g_psFieldNames[FIELD_ID_CONTACT_NUM_1]);
    }
    /* JOIN for Line NAme - in case of calls list, and Line name was requested - we get it from Line Settings list */
    if (bJoinLineSettingsList)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LEFT JOIN %s ON %s.%s=%s.%s",
                                    g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST],
                                    g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_LINE_ID],
                                    g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST], g_psFieldNames[FIELD_ID_LINE_ID]);
    }

    sQuery[u32_sQueryTotal++] = ';';
    sQuery[u32_sQueryTotal] = 0;

    /* Now select */
    assert(DATABASE);
    sql_rc = SQL_Select(DATABASE, sQuery, TRUE, &stColumnMap, &pv_SelectHandle);
    if (sql_rc == SQL_RC_OK)
    {
        sql_rc = SQL_FetchRow(pv_SelectHandle, &bEntryAvailable);
        if (sql_rc != SQL_RC_OK)
        {
            SQL_ReleaseSelectHandle(pv_SelectHandle);
            return LIST_RC_FAIL;
        }

        if (bEntryAvailable)
        {
            List_FillEntry(ListType, pu32Fields, u32_FieldsSize, pv_Entry, 0, pv_SelectHandle, &stColumnMap);
        }

        SQL_ReleaseSelectHandle(pv_SelectHandle);

        if (bEntryAvailable == FALSE)
        {
            return LIST_RC_ENTRY_NOT_AVAILABLE;
        }
        return LIST_RC_OK;
    }

    return LIST_RC_FAIL;
}

LIST_RC List_SearchEntries(IN LIST_TYPE ListType, IN eMatchOption eMatching, IN bool bCaseSensitive, IN const char* sSearchedVal,
                           IN bool bForward, IN eMarkRequest eMark, IN u32 pu32Fields[], IN u32 u32_FieldsSize,
                           IN u32 u32_SortField1, IN u32 u32_SortField2, OUT void* pv_Entries, INOUT u32* pu32_EntriesSize, OUT u32* pu32_StartIndex)
{
    /* Search strategy: First find the index of the Entry searched, then call ReadEntries with this Index */

    char sQuery[SQL_QUERY_MAX_LEN * 2];
    char sFields[SQL_QUERY_MAX_LEN];
    u32 u32_sFieldsLen = sizeof(sFields);
    u32 u32_sQueryTotal = 0, u32_sQuerySize = sizeof(sQuery);
    bool bJoinContactList = FALSE, bJoinLineSettingsList = FALSE, bMoreEntries = FALSE;
    LIST_RC rc;
    SQL_RC sql_rc;
    ST_COLUMN_MAP stColumnMap; /* used to access the select result in random order */
    void* pv_SelectHandle;
    char* sComparison;

    *pu32_StartIndex = 0;

    if ((s32)u32_SortField1 == FIELD_ID_INVALID)
    {
        printf("List_SearchEntries: no sorting defined!\n");
        return LIST_RC_NO_SORT;
    }

    /* Prepare compare operator */
    switch (eMatching)
    {
        case MATCH_EXACT:
            sComparison = "=";
            break;

        case MATCH_RETURN_NEXT_ON_FAIL:
            sComparison = ">=";
            break;

        case MATCH_RETURN_PREV_ON_FAIL:
            sComparison = "<=";
            break;

        default:
            printf("List_SearchEntries: unknown match option - %d\n", eMatching);
            return LIST_RC_UNSUPPORTED_MATCH_OPTION;
    }

    /* Prepare fields names */
    rc = List_PrepareFieldsNamesForRead(ListType, pu32Fields, u32_FieldsSize, sFields, &u32_sFieldsLen, &bJoinContactList, &bJoinLineSettingsList);
    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    /* Prepare query */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, g_sReadQuery, sFields, g_ps_TableNameStrings[ListType]);

    /* Add searched value */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal,
                                " WHERE %s%s'%s'", g_psFieldNames[u32_SortField1], sComparison, sSearchedVal);

    /* Add filtering for calls list */
    if (CallList(ListType))
    {
        switch (ListType)
        {
            case LIST_TYPE_MISSED_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, ",%s=%d", g_psFieldNames[FIELD_ID_CALL_TYPE], CALL_TYPE_MISSED);
                break;
            case LIST_TYPE_OUTGOING_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, ",%s=%d", g_psFieldNames[FIELD_ID_CALL_TYPE], CALL_TYPE_OUTGOING);
                break;
            case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, ",%s=%d", g_psFieldNames[FIELD_ID_CALL_TYPE], CALL_TYPE_INCOMING);
                break;
            case LIST_TYPE_ALL_INCOMING_CALLS:
                u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, sizeof(sQuery) - u32_sQueryTotal, ",(%s=%d OR %s=%d)",
                                            g_psFieldNames[FIELD_ID_CALL_TYPE], CALL_TYPE_MISSED, g_psFieldNames[FIELD_ID_CALL_TYPE], CALL_TYPE_INCOMING);
                break;
            default:
                break;
        }
    }

    if (bCaseSensitive == FALSE)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " COLLATE NOCASE");
    }

    /* JOIN for First/Last Name - in case of calls list, and name was requested - we get it from contact list */
    if (bJoinContactList)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LEFT JOIN %s ON %s.%s=%s.%s",
                                    g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST],
                                    g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_NUMBER],
                                    g_ps_TableNameStrings[LIST_TYPE_CONTACT_LIST], g_psFieldNames[FIELD_ID_CONTACT_NUM_1]);
    }
    /* JOIN for Line NAme - in case of calls list, and Line name was requested - we get it from Line Settings list */
    if (bJoinLineSettingsList)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " LEFT JOIN %s ON %s.%s=%s.%s",
                                    g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST],
                                    g_ps_TableNameStrings[ListType], g_psFieldNames[FIELD_ID_LINE_ID],
                                    g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST], g_psFieldNames[FIELD_ID_LINE_ID]);
    }

    /* Sort */
    if ((s32)u32_SortField1 != FIELD_ID_INVALID)
    {
        u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " ORDER BY %s COLLATE NOCASE", g_psFieldNames[u32_SortField1]);

        if ((s32)u32_SortField2 != FIELD_ID_INVALID)
        {
            u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, ",%s COLLATE NOCASE", g_psFieldNames[u32_SortField2]);
        }

        if (eMatching == MATCH_RETURN_PREV_ON_FAIL)
        {
            u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, " %s", "DESC");
        }
    }

    sQuery[u32_sQueryTotal++] = ';';
    sQuery[u32_sQueryTotal] = 0;

    /* Now select */
    assert(DATABASE);
    sql_rc = SQL_Select(DATABASE, sQuery, TRUE, &stColumnMap, &pv_SelectHandle);
    if (sql_rc == SQL_RC_OK)
    {
        sql_rc = SQL_FetchRow(pv_SelectHandle, &bMoreEntries);
        if (sql_rc != SQL_RC_OK)
        {
            SQL_ReleaseSelectHandle(pv_SelectHandle);
            return LIST_RC_FAIL;
        }

        if (bMoreEntries)
        {
            stLineSettingsListEntry* pEntry = pv_Entries;

            List_FillEntry(ListType, pu32Fields, u32_FieldsSize, pv_Entries, 0, pv_SelectHandle, &stColumnMap);

            SQL_ReleaseSelectHandle(pv_SelectHandle);

            /* Entry Id is the first member of every entry */
            List_GetEntryIdex(ListType, u32_SortField1, u32_SortField2, pEntry->u32_EntryId, pu32_StartIndex);

            /* now use Read to return all requested entries */
            return List_ReadEntries(ListType, pu32_StartIndex, bForward, eMark, pu32Fields, u32_FieldsSize, u32_SortField1, u32_SortField2, pv_Entries, pu32_EntriesSize);

        }
        else
        {
            /* No entries found */
            *pu32_EntriesSize = 0;
            SQL_ReleaseSelectHandle(pv_SelectHandle);
            return LIST_RC_OK;
        }
    }

    return LIST_RC_FAIL;
}

LIST_RC List_FillEntry(IN LIST_TYPE ListType, IN u32 pu32Fields[], IN u32 u32_FieldsSize, OUT void* pv_Entries, IN u32 u32_EntriesOffset,
                       IN void* pv_SelectHandle, IN PST_COLUMN_MAP pst_Map)
{
    LIST_RC rc;

    switch (ListType)
    {
        case LIST_TYPE_MISSED_CALLS:
        case LIST_TYPE_OUTGOING_CALLS:
        case LIST_TYPE_INCOMING_ACCEPTED_CALLS:
        case LIST_TYPE_ALL_CALLS:
        case LIST_TYPE_ALL_INCOMING_CALLS:
            rc = List_FillCallsListEntry(pu32Fields, u32_FieldsSize, pv_Entries, u32_EntriesOffset, pv_SelectHandle, pst_Map);
            break;

        case LIST_TYPE_CONTACT_LIST:
            rc = List_FillContactListEntry(pu32Fields, u32_FieldsSize, pv_Entries, u32_EntriesOffset, pv_SelectHandle, pst_Map);
            break;

        case LIST_TYPE_LINE_SETTINGS_LIST:
            rc = List_FillLineSettingsEntry(pu32Fields, u32_FieldsSize, pv_Entries, u32_EntriesOffset, pv_SelectHandle, pst_Map);
            break;

        default:
            printf("List_FillEntry: Unsupported list %d\n", ListType);
            rc = LIST_RC_UNSUPPORTED_LIST;
            break;
    }

    return rc;
}

LIST_RC List_FillContactListEntry(IN u32 pu32Fields[], IN u32 u32_FieldsSize, OUT void* pv_Entries, IN u32 u32_EntriesOffset,
                                  IN void* pv_SelectHandle, IN PST_COLUMN_MAP pst_Map)
{
    stContactListEntry* pst_Entry = (stContactListEntry*)pv_Entries + u32_EntriesOffset;
    u32 u32_Index;
    u32 u32_Temp;
    const unsigned char* sTemp;

    for (u32_Index = 0; u32_Index < u32_FieldsSize; ++u32_Index)
    {
        switch (pu32Fields[u32_Index])
        {
            case FIELD_ID_ENTRY_ID:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_EntryId);
                break;

            case FIELD_ID_LAST_NAME:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sLastName, (const char*)sTemp, sizeof(pst_Entry->sLastName));
                pst_Entry->sLastName[sizeof(pst_Entry->sLastName) - 1] = 0;
                break;

            case FIELD_ID_FIRST_NAME:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sFirstName, (const char*)sTemp, sizeof(pst_Entry->sFirstName));
                pst_Entry->sFirstName[sizeof(pst_Entry->sFirstName) - 1] = 0;
                break;

            case FIELD_ID_CONTACT_NUM_1:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sNumber1, (const char*)sTemp, sizeof(pst_Entry->sNumber1));
                pst_Entry->sNumber1[sizeof(pst_Entry->sNumber1) - 1] = 0;

                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CONTACT_NUM_1_DEFAULT], pst_Map, &u32_Temp);
                pst_Entry->bNumber1Default = (bool)u32_Temp;
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CONTACT_NUM_1_OWN], pst_Map, &u32_Temp);
                pst_Entry->bNumber1Own = (bool)u32_Temp;
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CONTACT_NUM_1_TYPE], pst_Map, &u32_Temp);
                pst_Entry->cNumber1Type = (char)u32_Temp;
                break;

            case FIELD_ID_CONTACT_NUM_2:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sNumber2, (const char*)sTemp, sizeof(pst_Entry->sNumber2));
                pst_Entry->sNumber2[sizeof(pst_Entry->sNumber2) - 1] = 0;

                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CONTACT_NUM_2_DEFAULT], pst_Map, &u32_Temp);
                pst_Entry->bNumber2Default = (bool)u32_Temp;
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CONTACT_NUM_2_OWN], pst_Map, &u32_Temp);
                pst_Entry->bNumber2Own = (bool)u32_Temp;
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CONTACT_NUM_2_TYPE], pst_Map, &u32_Temp);
                pst_Entry->cNumber2Type = (char)u32_Temp;
                break;

            case FIELD_ID_ASSOCIATED_MELODY:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_AssociatedMelody);
                break;

            case FIELD_ID_LINE_ID:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_LineId);
                break;

            default:
                printf("List_FillContactListEntry: Unknown Field Id %d\n", pu32Fields[u32_Index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_FillCallsListEntry(IN u32 pu32Fields[], IN u32 u32_FieldsSize, OUT void* pv_Entries, IN u32 u32_EntriesOffset,
                                IN void* pv_SelectHandle, IN PST_COLUMN_MAP pst_Map)
{
    stCallsListEntry* pst_Entry = (stCallsListEntry*)pv_Entries + u32_EntriesOffset;
    u32 u32_Index;
    u32 u32_Temp;
    const unsigned char* sTemp;

    for (u32_Index = 0; u32_Index < u32_FieldsSize; ++u32_Index)
    {
        switch (pu32Fields[u32_Index])
        {
            case FIELD_ID_ENTRY_ID:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_EntryId);
                break;

            case FIELD_ID_NUMBER:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sNumber, (const char*)sTemp, sizeof(pst_Entry->sNumber));
                pst_Entry->sNumber[sizeof(pst_Entry->sNumber) - 1] = 0;
                break;

            case FIELD_ID_DATE_AND_TIME:
                if (sizeof(time_t) == sizeof(u64))
                {
                    SQL_GetUVal64ByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, (u64 *)&pst_Entry->t_DateAndTime);
                }
                else
                {
                    SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, (u32 *)&pst_Entry->t_DateAndTime);
                }
                break;

            case FIELD_ID_READ_STATUS:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bRead = (bool)u32_Temp;
                break;

            case FIELD_ID_LINE_NAME:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sLineName, (const char*)sTemp, sizeof(pst_Entry->sLineName));
                pst_Entry->sLineName[sizeof(pst_Entry->sLineName) - 1] = 0;
                break;

            case FIELD_ID_LINE_ID:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_LineId);
                break;

            case FIELD_ID_NUM_OF_CALLS:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_NumOfCalls);
                break;

            case FIELD_ID_CALL_TYPE:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->cCallType = (char)u32_Temp;
                break;

            case FIELD_ID_LAST_NAME:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sLastName, (const char*)sTemp, sizeof(pst_Entry->sLastName));
                pst_Entry->sLastName[sizeof(pst_Entry->sLastName) - 1] = 0;
                break;

            case FIELD_ID_FIRST_NAME:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sFirstName, (const char*)sTemp, sizeof(pst_Entry->sFirstName));
                pst_Entry->sFirstName[sizeof(pst_Entry->sFirstName) - 1] = 0;
                break;

            default:
                printf("List_FillCallsListEntry: Unknown Field Id %d\n", pu32Fields[u32_Index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_FillLineSettingsEntry(IN u32 pu32Fields[], IN u32 u32_FieldsSize, OUT void* pv_Entries, IN u32 u32_EntriesOffset,
                                   IN void* pv_SelectHandle, IN PST_COLUMN_MAP pst_Map)
{
    stLineSettingsListEntry* pst_Entry = (stLineSettingsListEntry*)pv_Entries + u32_EntriesOffset;
    u32 u32_Index;
    u32 u32_Temp;
    const unsigned char* sTemp;

    for (u32_Index = 0; u32_Index < u32_FieldsSize; ++u32_Index)
    {
        switch (pu32Fields[u32_Index])
        {
            case FIELD_ID_ENTRY_ID:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_EntryId);
                break;

            case FIELD_ID_LINE_NAME:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sLineName, (const char*)sTemp, sizeof(pst_Entry->sLineName));
                pst_Entry->sLineName[sizeof(pst_Entry->sLineName) - 1] = 0;
                break;

            case FIELD_ID_LINE_ID:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_LineId);
                break;

            case FIELD_ID_ATTACHED_HANDSETS:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_AttachedHsMask);
                break;

            case FIELD_ID_DIALING_PREFIX:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sDialPrefix, (const char*)sTemp, sizeof(pst_Entry->sDialPrefix));
                pst_Entry->sDialPrefix[sizeof(pst_Entry->sDialPrefix) - 1] = 0;
                break;

            case FIELD_ID_FP_MELODY:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_FPMelody);
                break;

            case FIELD_ID_FP_VOLUME:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &pst_Entry->u32_FPVolume);
                break;

            case FIELD_ID_BLOCKED_NUMBER:
                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &sTemp);
                strncpy(pst_Entry->sBlockedNumber, (const char*)sTemp, sizeof(pst_Entry->sBlockedNumber));
                pst_Entry->sBlockedNumber[sizeof(pst_Entry->sBlockedNumber) - 1] = 0;
                break;

            case FIELD_ID_MULTIPLE_CALLS_MODE:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bMultiCalls = (bool)u32_Temp;
                break;

            case FIELD_ID_INTRUSION_CALL:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bIntrusionCall = (bool)u32_Temp;
                break;

            case FIELD_ID_PERMANENT_CLIR:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bPermanentCLIR = (bool)u32_Temp;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_PERMANENT_CLIR_ACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sPermanentCLIRActCode, (const char*)sTemp, sizeof(pst_Entry->sPermanentCLIRActCode));
                pst_Entry->sPermanentCLIRActCode[sizeof(pst_Entry->sPermanentCLIRActCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_PERMANENT_CLIR_DEACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sPermanentCLIRDeactCode, (const char*)sTemp, sizeof(pst_Entry->sPermanentCLIRDeactCode));
                pst_Entry->sPermanentCLIRDeactCode[sizeof(pst_Entry->sPermanentCLIRDeactCode) - 1] = 0;
                break;

            case FIELD_ID_CALL_FWD_UNCOND:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bCallFwdUncond = (bool)u32_Temp;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_ACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdUncondActCode, (const char*)sTemp, sizeof(pst_Entry->sCallFwdUncondActCode));
                pst_Entry->sCallFwdUncondActCode[sizeof(pst_Entry->sCallFwdUncondActCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_DEACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdUncondDeactCode, (const char*)sTemp, sizeof(pst_Entry->sCallFwdUncondDeactCode));
                pst_Entry->sCallFwdUncondDeactCode[sizeof(pst_Entry->sCallFwdUncondDeactCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_UNCOND_TARGET_NUMBER], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdUncondNum, (const char*)sTemp, sizeof(pst_Entry->sCallFwdUncondNum));
                pst_Entry->sCallFwdUncondNum[sizeof(pst_Entry->sCallFwdUncondNum) - 1] = 0;
                break;

            case FIELD_ID_CALL_FWD_NO_ANSWER:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bCallFwdNoAns = (bool)u32_Temp;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_ACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdNoAnsActCode, (const char*)sTemp, sizeof(pst_Entry->sCallFwdNoAnsActCode));
                pst_Entry->sCallFwdNoAnsActCode[sizeof(pst_Entry->sCallFwdNoAnsActCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_DEACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdNoAnsDeactCode, (const char*)sTemp, sizeof(pst_Entry->sCallFwdNoAnsDeactCode));
                pst_Entry->sCallFwdNoAnsDeactCode[sizeof(pst_Entry->sCallFwdNoAnsDeactCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TARGET_NUMBER], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdNoAnsNum, (const char*)sTemp, sizeof(pst_Entry->sCallFwdNoAnsNum));
                pst_Entry->sCallFwdNoAnsNum[sizeof(pst_Entry->sCallFwdNoAnsNum) - 1] = 0;

                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_NO_ANSWER_TIMEOUT], pst_Map, &pst_Entry->u32_CallFwdNoAnsTimeout);
                break;

            case FIELD_ID_CALL_FWD_BUSY:
                SQL_GetUValByColumnName(pv_SelectHandle, g_psFieldNames[pu32Fields[u32_Index]], pst_Map, &u32_Temp);
                pst_Entry->bCallFwdBusy = (bool)u32_Temp;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_ACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdBusyActCode, (const char*)sTemp, sizeof(pst_Entry->sCallFwdBusyActCode));
                pst_Entry->sCallFwdBusyActCode[sizeof(pst_Entry->sCallFwdBusyActCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_DEACTIVATION_CODE], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdBusyDeactCode, (const char*)sTemp, sizeof(pst_Entry->sCallFwdBusyDeactCode));
                pst_Entry->sCallFwdBusyDeactCode[sizeof(pst_Entry->sCallFwdBusyDeactCode) - 1] = 0;

                SQL_GetStrByColumnName(pv_SelectHandle, g_psFieldNames[FIELD_ID_CALL_FWD_BUSY_TARGET_NUMBER], pst_Map, &sTemp);
                strncpy(pst_Entry->sCallFwdBusyNum, (const char*)sTemp, sizeof(pst_Entry->sCallFwdBusyNum));
                pst_Entry->sCallFwdBusyNum[sizeof(pst_Entry->sCallFwdBusyNum) - 1] = 0;
                break;

            default:
                printf("List_FillLineSettingsEntry: Unknown Field Id %d\n", pu32Fields[u32_Index]);
                return LIST_RC_UNKNOWN_FIELD;
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_GetEntryIdex(IN LIST_TYPE ListType, IN u32 u32_SortField1, IN u32 u32_SortField2, IN u32 u32_EntryId, OUT u32* pu32_Index)
{
    u32 u32_FieldId = FIELD_ID_ENTRY_ID;
    u32 u32_NumOfEntries = 1, u32_EntryCount;

    struct stDummy
    {
        u32 u32_EntryId;
    } st_Dummy;

    *pu32_Index = 1;

    List_GetCount(ListType, &u32_EntryCount);

    while (TRUE)
    {
        List_ReadEntries(ListType, pu32_Index, TRUE, MARK_LEAVE_UNCHANGED,
                         &u32_FieldId, 1, u32_SortField1, u32_SortField2, &st_Dummy, &u32_NumOfEntries);

        if (st_Dummy.u32_EntryId == u32_EntryId)
        {
            return LIST_RC_OK;
        }

        ++(*pu32_Index);

        if (*pu32_Index > u32_EntryCount)
        {
            return LIST_RC_ENTRY_NOT_AVAILABLE;
        }
    }
}

LIST_RC List_SetAttachedHs(IN u32 u32_LineId, IN u32 u32_AttachedHsMask)
{
    stLineSettingsListEntry st_Entry;
    u32 u32_NumOfEntries = 1, pu32_Fields[2];
    LIST_RC rc;
    u32 u32_EntryId; // of Line ID

    u32_EntryId = LineSettingsGetEntryIdByLineId(u32_LineId);

    if (!u32_EntryId)
    {
        return LIST_RC_FAIL;
    }

    pu32_Fields[0] = FIELD_ID_ATTACHED_HANDSETS;
    pu32_Fields[1] = FIELD_ID_ENTRY_ID;

    /* Read current Attached HS mask */
    rc = List_ReadEntries(LIST_TYPE_LINE_SETTINGS_LIST, &u32_EntryId, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, LENGTHOF(pu32_Fields),
                          FIELD_ID_LINE_ID, FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    if (u32_NumOfEntries == 0)
    {
        return LIST_RC_ENTRY_NOT_AVAILABLE;
    }

    /* Set Mask */
    st_Entry.u32_AttachedHsMask = u32_AttachedHsMask;

    /* Update in Database */
    rc = List_UpdateEntry(LIST_TYPE_LINE_SETTINGS_LIST, &st_Entry, pu32_Fields, 1, st_Entry.u32_EntryId);

    return rc;
}


LIST_RC List_GetAttachedHs(IN u32 u32_LineId, OUT u32* pu32_AttachedHsMask)
{
    char sQuery[SQL_QUERY_MAX_LEN];
    u32 u32_sQueryTotal = 0, u32_sQuerySize = sizeof(sQuery);
    SQL_RC sql_rc;
    ST_COLUMN_MAP stColumnMap; /* used to access the select result in random order */
    void* pv_SelectHandle;
    bool bEntryFound;

    /* Prepare query */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal, g_sReadQuery,
                                g_psFieldNames[FIELD_ID_ATTACHED_HANDSETS], g_ps_TableNameStrings[LIST_TYPE_LINE_SETTINGS_LIST]);

    /* Add Line ID */
    u32_sQueryTotal += snprintf(sQuery + u32_sQueryTotal, u32_sQuerySize - u32_sQueryTotal,
                                " WHERE %s=%d", g_psFieldNames[FIELD_ID_LINE_ID], u32_LineId);

    sQuery[u32_sQueryTotal++] = ';';
    sQuery[u32_sQueryTotal] = 0;

    /* Now select */
    assert(DATABASE);
    sql_rc = SQL_Select(DATABASE, sQuery, TRUE, &stColumnMap, &pv_SelectHandle);
    if (sql_rc == SQL_RC_OK)
    {
        sql_rc = SQL_FetchRow(pv_SelectHandle, &bEntryFound);
        if (sql_rc != SQL_RC_OK)
        {
            SQL_ReleaseSelectHandle(pv_SelectHandle);
            return LIST_RC_FAIL;
        }

        if (bEntryFound)
        {
            u32 u32_AttachedHsFieldId = FIELD_ID_ATTACHED_HANDSETS;
            stLineSettingsListEntry st_Entry;

            List_FillEntry(LIST_TYPE_LINE_SETTINGS_LIST, &u32_AttachedHsFieldId, 1, &st_Entry, 0, pv_SelectHandle, &stColumnMap);

            *pu32_AttachedHsMask = st_Entry.u32_AttachedHsMask;

            SQL_ReleaseSelectHandle(pv_SelectHandle);

            return LIST_RC_OK;

        }
        else
        {
            /* No entries found */
            SQL_ReleaseSelectHandle(pv_SelectHandle);
        }
    }

    return LIST_RC_FAIL;
}

LIST_RC List_GetMissedCallsNumOfEntries(IN u32 u32_LineId, OUT u32* pu32_NumOfUnread, OUT u32* pu32_NumOfRead)
{
    u32 u32_Count, u32_Index, u32_NumOfEntries = 1, pu32_Fields[2];
    stCallsListEntry st_Entry;
    LIST_RC rc;

    *pu32_NumOfUnread = *pu32_NumOfRead = 0;

    pu32_Fields[0] = FIELD_ID_LINE_ID;
    pu32_Fields[1] = FIELD_ID_READ_STATUS;

    /* Get Count */
    rc = List_GetCount(LIST_TYPE_MISSED_CALLS, &u32_Count);
    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    for (u32_Index = 1; u32_Index <= u32_Count; ++u32_Index)
    {
        rc = List_ReadEntries(LIST_TYPE_MISSED_CALLS, &u32_Index, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, 2, FIELD_ID_DATE_AND_TIME,
                              FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

        if (rc != LIST_RC_OK)
        {
            return rc;
        }

        if (st_Entry.u32_LineId != u32_LineId)
        {
            continue;
        }

        if (st_Entry.bRead)
        {
            ++(*pu32_NumOfUnread);
        }
        else
        {
            ++(*pu32_NumOfRead);
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_AddDefaultLine()
{
    u32 pu32_Fields[6], u32_FieldsNum = 6;
    stLineSettingsListEntry  st_LineSettingsListEntry;
    u32 u32_EntryId = 0;
    memset(&st_LineSettingsListEntry,   0, sizeof(st_LineSettingsListEntry));

    /* No Line entries - add default line */
    List_CreateList(LIST_TYPE_LINE_SETTINGS_LIST);
    strcpy(st_LineSettingsListEntry.sLineName, "Line-1");
    st_LineSettingsListEntry.u32_LineId = 0;
    st_LineSettingsListEntry.u32_AttachedHsMask = CMBS_U16_SUBSCRIBED_HS_MASK;
    st_LineSettingsListEntry.sDialPrefix[0] = 0;
    st_LineSettingsListEntry.bMultiCalls = 1;
    st_LineSettingsListEntry.bIntrusionCall = 1;

    pu32_Fields[0]  = FIELD_ID_LINE_NAME;
    pu32_Fields[1]  = FIELD_ID_LINE_ID;
    pu32_Fields[2]  = FIELD_ID_ATTACHED_HANDSETS;
    pu32_Fields[3]  = FIELD_ID_DIALING_PREFIX;
    pu32_Fields[4]  = FIELD_ID_MULTIPLE_CALLS_MODE;
    pu32_Fields[5]  = FIELD_ID_INTRUSION_CALL;

    List_InsertEntry(LIST_TYPE_LINE_SETTINGS_LIST, &st_LineSettingsListEntry, pu32_Fields, u32_FieldsNum, &u32_EntryId);

    return LIST_RC_OK;
}

LIST_RC List_AddHsToFirstLine(IN u8 u8_HsNum)
{
    stLineSettingsListEntry st_Entry;
    u32 u32_StartIdx = 1, u32_NumOfEntries = 1, pu32_Fields[2];
    LIST_RC rc;

    pu32_Fields[0] = FIELD_ID_ATTACHED_HANDSETS;
    pu32_Fields[1] = FIELD_ID_ENTRY_ID;

    /* Read current Attached HS mask */
    rc = List_ReadEntries(LIST_TYPE_LINE_SETTINGS_LIST, &u32_StartIdx, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, LENGTHOF(pu32_Fields),
                          FIELD_ID_LINE_ID, FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

    if (rc != LIST_RC_OK)
    {
        return rc;
    }

    if (u32_NumOfEntries == 0)
    {
        return List_AddDefaultLine();
    }

    /* Set relevant bit */
    st_Entry.u32_AttachedHsMask |= 1 << (u8_HsNum - 1);

    /* Update in Database */
    rc = List_UpdateEntry(LIST_TYPE_LINE_SETTINGS_LIST, &st_Entry, pu32_Fields, 1, st_Entry.u32_EntryId);

    return rc;
}

LIST_RC List_RemoveHsFromAllLines(IN u8 u8_HsNum)
{
    stLineSettingsListEntry st_Entry;
    u32 u32_StartIdx, u32_TotalNumOfEntries, pu32_Fields[2], u32_i, u32_NumOfEntries = 1, u32_BitMask;
    LIST_RC rc;

    pu32_Fields[0] = FIELD_ID_ATTACHED_HANDSETS;
    pu32_Fields[1] = FIELD_ID_ENTRY_ID;

    List_GetCount(LIST_TYPE_LINE_SETTINGS_LIST, &u32_TotalNumOfEntries);

    for (u32_i = 0; u32_i < u32_TotalNumOfEntries; ++u32_i)
    {
        /* Read current Attached HS mask */
        u32_StartIdx = u32_i + 1;
        rc = List_ReadEntries(LIST_TYPE_LINE_SETTINGS_LIST, &u32_StartIdx, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, LENGTHOF(pu32_Fields),
                              FIELD_ID_LINE_ID, FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

        if (rc != LIST_RC_OK)
        {
            return rc;
        }

        /* Reset relevant bit */
        u32_BitMask = 1 << (u8_HsNum - 1);
        st_Entry.u32_AttachedHsMask &= ~u32_BitMask;

        /* Update in Database */
        rc = List_UpdateEntry(LIST_TYPE_LINE_SETTINGS_LIST, &st_Entry, pu32_Fields, 1, st_Entry.u32_EntryId);

        if (rc != LIST_RC_OK)
        {
            return rc;
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_GetDefaultLine(IN u8 u8_HsNum, OUT u32* pu32_LineId)
{
    stLineSettingsListEntry st_Entry;
    u32 u32_StartIdx, u32_TotalNumOfEntries, pu32_Fields[3], u32_i, u32_NumOfEntries = 1, u32_BitMask;
    LIST_RC rc;

    *pu32_LineId = APPCALL_NO_LINE;

    pu32_Fields[0] = FIELD_ID_ATTACHED_HANDSETS;
    pu32_Fields[1] = FIELD_ID_LINE_ID;
    pu32_Fields[2] = FIELD_ID_MULTIPLE_CALLS_MODE;

    List_GetCount(LIST_TYPE_LINE_SETTINGS_LIST, &u32_TotalNumOfEntries);

    for (u32_i = 0; u32_i < u32_TotalNumOfEntries; ++u32_i)
    {
        /* Read current Attached HS mask */
        u32_StartIdx = u32_i + 1;
        rc = List_ReadEntries(LIST_TYPE_LINE_SETTINGS_LIST, &u32_StartIdx, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, LENGTHOF(pu32_Fields),
                              FIELD_ID_LINE_ID, FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

        if (rc != LIST_RC_OK)
        {
            return rc;
        }

        //if ( u32_i == 0 )
        //{
        /* As default use first line ID */
        //    *pu32_LineId = st_Entry.u32_LineId;
        //}

        /* check if the HS is attached to this line */
        u32_BitMask = 1 << (u8_HsNum - 1);
        if (u32_BitMask & st_Entry.u32_AttachedHsMask)
        {
            if ((st_Entry.bMultiCalls == TRUE) || (FALSE == appcall_IsLineInUse(st_Entry.u32_LineId)) || (TRUE == appcall_IsHsInCallWithLine(u8_HsNum, (u8)st_Entry.u32_LineId)))
            {
                *pu32_LineId = st_Entry.u32_LineId;
                return LIST_RC_OK;
            }
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_GetLinesOfHS(IN u8 u8_HsNum, OUT u32 pu32_LineId[], INOUT u32* pu32_LineIdLen)
{
    stLineSettingsListEntry st_Entry;
    u32 u32_StartIdx, u32_TotalNumOfEntries, pu32_Fields[2], u32_i, u32_NumOfEntries = 1, u32_BitMask;
    LIST_RC rc;

    pu32_Fields[0] = FIELD_ID_ATTACHED_HANDSETS;
    pu32_Fields[1] = FIELD_ID_LINE_ID;

    List_GetCount(LIST_TYPE_LINE_SETTINGS_LIST, &u32_TotalNumOfEntries);

    if (u32_TotalNumOfEntries > *pu32_LineIdLen)
    {
        printf("List_GetLinesOfHS - input array length (%d) too small (needed %d)!\n", *pu32_LineIdLen, u32_TotalNumOfEntries);
        return LIST_RC_ARRAY_TOO_SMALL;
    }

    *pu32_LineIdLen = 0;

    for (u32_i = 0; u32_i < u32_TotalNumOfEntries; ++u32_i)
    {
        /* Read current Attached HS mask */
        u32_StartIdx = u32_i + 1;
        rc = List_ReadEntries(LIST_TYPE_LINE_SETTINGS_LIST, &u32_StartIdx, TRUE, MARK_LEAVE_UNCHANGED, pu32_Fields, LENGTHOF(pu32_Fields),
                              FIELD_ID_LINE_ID, FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

        if (rc != LIST_RC_OK)
        {
            return rc;
        }

        /* check if the HS is attached to this line */
        u32_BitMask = 1 << (u8_HsNum - 1);
        if (u32_BitMask & st_Entry.u32_AttachedHsMask)
        {
            pu32_LineId[(*pu32_LineIdLen)++] = st_Entry.u32_LineId;
        }
    }

    return LIST_RC_OK;
}

LIST_RC List_GetCallForwardMode(IN u32 u32_LineID, OUT u8* pb_CFUncondEn, OUT u8* pb_CFNoAnsEn, OUT u8* pb_CFBusyEn)
{
    u32 u32_NumOfLines, u32_i, u32_StartIdx, pu32_FieldIDs[4], u32_NumOfEntries = 1;
    stLineSettingsListEntry st_Entry;

    pu32_FieldIDs[0] = FIELD_ID_LINE_ID;
    pu32_FieldIDs[1] = FIELD_ID_CALL_FWD_UNCOND;
    pu32_FieldIDs[2] = FIELD_ID_CALL_FWD_NO_ANSWER;
    pu32_FieldIDs[3] = FIELD_ID_CALL_FWD_BUSY;

    /* Set defaults for out params */
    *pb_CFUncondEn = *pb_CFNoAnsEn = *pb_CFBusyEn = FALSE;

    List_GetCount(LIST_TYPE_LINE_SETTINGS_LIST, &u32_NumOfLines);

    for (u32_i = 0, u32_StartIdx = 1; u32_i < u32_NumOfLines; ++u32_i, ++u32_StartIdx)
    {
        List_ReadEntries(LIST_TYPE_LINE_SETTINGS_LIST, &u32_StartIdx, TRUE, MARK_LEAVE_UNCHANGED,
                         pu32_FieldIDs, LENGTHOF(pu32_FieldIDs), FIELD_ID_LINE_ID, FIELD_ID_INVALID, &st_Entry, &u32_NumOfEntries);

        if (u32_LineID == st_Entry.u32_LineId)
        {
            *pb_CFUncondEn  = st_Entry.bCallFwdUncond;
            *pb_CFNoAnsEn   = st_Entry.bCallFwdNoAns;
            *pb_CFBusyEn    = st_Entry.bCallFwdBusy;

            return LIST_RC_OK;
        }
    }

    return LIST_RC_ENTRY_NOT_AVAILABLE;
}

/* End Of File *****************************************************************************************************************************/
