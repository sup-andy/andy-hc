/*!
* \file  cmbs_dbg.c
* \brief  This file contains debug functions for CMBS API
* \Author  andriig
*
* @(#) %filespec: cmbs_dbg.c~24.1.2 %
*
*******************************************************************************/

#include "cmbs_dbg.h"
#include "cmbs_util.h"
#include "cfr_ie.h"
#include "cfr_debug.h"
#include <string.h>
#include "ctrl_common_lib.h"

#ifdef WIN32
#include <windows.h>
#endif

#if defined( __linux__ )
#include <time.h>
#include <sys/time.h>
#endif

static pfn_cmbs_dbg_ParseIEFunc pfn_ParseIEFunction = NULL;

//  ==========  cmbs_dbg_GetEventName ===========
/*!
 \brief     return CMBS event string; For host side
 \param[in,out]   id   CMBSinternal command
 \return           <const char * >
*/
const char *  cmbs_dbg_GetEventName(E_CMBS_EVENT_ID id)
{
    switch (id)
    {
            caseretstr(CMBS_EV_UNDEF);
            caseretstr(CMBS_EV_DSR_HS_PAGE_START);              /*!< Performs paging handsets */
            caseretstr(CMBS_EV_DSR_HS_PAGE_START_RES);          /*!< Response to CMBS_EV_DSR_HS_PAGE_START */
            caseretstr(CMBS_EV_DSR_HS_PAGE_STOP);               /*!< Performs stop paging handsets */
            caseretstr(CMBS_EV_DSR_HS_PAGE_STOP_RES);           /*!< Response to CMBS_EV_DSR_HS_PAGE_STOP */
            caseretstr(CMBS_EV_DSR_HS_DELETE);                  /*!< Delete one or more handsets from the base's database */
            caseretstr(CMBS_EV_DSR_HS_DELETE_RES);              /*!< Response to CMBS_EV_DSR_HS_DELETE */
            caseretstr(CMBS_EV_DSR_HS_REGISTERED);              /*!< Unsolicited event generated on successful register/unregister operation of a handset */
            caseretstr(CMBS_EV_DSR_HS_IN_RANGE);                /*!< Generated when a handset in range */
            caseretstr(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET);     /*!< Get list of registered handsets */
            caseretstr(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_GET_RES); /*!< Get list of registered handsets */
            caseretstr(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_SET);     /*!< Set new name for registered handset */
            caseretstr(CMBS_EV_DSR_HS_SUBSCRIBED_LIST_SET_RES); /*!< Set new name for registered handset */
            caseretstr(CMBS_EV_DSR_LINE_SETTINGS_LIST_GET);     /*!< Get line settings */
            caseretstr(CMBS_EV_DSR_LINE_SETTINGS_LIST_GET_RES); /*!< Get line settings */
            caseretstr(CMBS_EV_DSR_LINE_SETTINGS_LIST_SET);     /*!< Set line settings */
            caseretstr(CMBS_EV_DSR_LINE_SETTINGS_LIST_SET_RES); /*!< Get line settings */
            caseretstr(CMBS_EV_DSR_CORD_OPENREG);               /*!< Starts registration mode on the base station */
            caseretstr(CMBS_EV_DSR_CORD_OPENREG_RES);           /*!< Response to CMBS_EV_DSR_CORD_OPENREG */
            caseretstr(CMBS_EV_DSR_CORD_CLOSEREG);              /*!< Stops registration mode on the base station */
            caseretstr(CMBS_EV_DSR_CORD_CLOSEREG_RES);          /*!< Response to CMBS_EV_DSR_CORD_CLOSEREG */
            caseretstr(CMBS_EV_DSR_PARAM_GET);                  /*!< Get a parameter value */
            caseretstr(CMBS_EV_DSR_PARAM_GET_RES);              /*!< Response to CMBS_EV_DSR_PARAM_GET */
            caseretstr(CMBS_EV_DSR_PARAM_SET);                  /*!< Sets / updates a parameter value */
            caseretstr(CMBS_EV_DSR_PARAM_SET_RES);              /*!< Response to CMBS_EV_DSR_PARAM_SET */
            caseretstr(CMBS_EV_DSR_FW_UPD_START);               /*!< Starts firmware update on the base station */
            caseretstr(CMBS_EV_DSR_FW_UPD_START_RES);           /*!< Response to CMBS_EV_DSR_FW_UPD_START */
            caseretstr(CMBS_EV_DSR_FW_UPD_PACKETNEXT);          /*!< Sends a chunk of firmware to the base station */
            caseretstr(CMBS_EV_DSR_FW_UPD_PACKETNEXT_RES);      /*!< Response to CMBS_EV_DSR_FW_UPD_PACKETNEXT */
            caseretstr(CMBS_EV_DSR_FW_UPD_END);                 /*!< Ending firmware update process with last chunk of data */
            caseretstr(CMBS_EV_DSR_FW_UPD_END_RES);             /*!< Response to CMBS_EV_DSR_FW_UPD_END */
            caseretstr(CMBS_EV_DSR_FW_VERSION_GET);             /*!< Gets the base's current firmware version of a particular module */
            caseretstr(CMBS_EV_DSR_FW_VERSION_GET_RES);         /*!< Response to CMBS_EV_DSR_FW_VERSION_GET */
            caseretstr(CMBS_EV_DSR_SYS_START);                  /*!< Starts the base station's CMBS after parameters were set */
            caseretstr(CMBS_EV_DSR_SYS_START_RES);              /*!< Response to CMBS_EV_DSR_SYS_START  */
            caseretstr(CMBS_EV_DSR_SYS_SEND_RAWMSG);            /*!< Event containing a raw message to the target */
            caseretstr(CMBS_EV_DSR_SYS_SEND_RAWMSG_RES);        /*!< Response to CMBS_EV_DSR_SYS_SEND_RAWMSG */
            caseretstr(CMBS_EV_DSR_SYS_STATUS);                 /*!< Announce current target status); e.g. up); down); removed */
            caseretstr(CMBS_EV_DSR_SYS_LOG);                    /*!< Event containing target system logs */
            caseretstr(CMBS_EV_DSR_SYS_RESET);                  /*!< Performs a base station reboot */
            caseretstr(CMBS_EV_DSR_SYS_POWER_OFF);              /*!< Performs a base station power off */
            caseretstr(CMBS_EV_DSR_RF_SUSPEND);                 /*!< RF Suspend on CMBS target */
            caseretstr(CMBS_EV_DSR_RF_RESUME);                  /*!< RF Resume on CMBS target */
            caseretstr(CMBS_EV_DSR_TURN_ON_NEMO);               /*!< Turn On NEMo mode for the CMBS base */
            caseretstr(CMBS_EV_DSR_TURN_OFF_NEMO);              /*!< Turn Off NEMo mode for the CMBS base */
            caseretstr(CMBS_EV_DEE_CALL_ESTABLISH);             /*!< Event generated on start of a new call( incoming or outgoing ) */
            caseretstr(CMBS_EV_DEE_CALL_PROGRESS);              /*!< Events for various call progress states */
            caseretstr(CMBS_EV_DEE_CALL_ANSWER);                /*!< Generated when a call is answered */
            caseretstr(CMBS_EV_DEE_CALL_RELEASE);               /*!< Generated when a call is released */
            caseretstr(CMBS_EV_DEE_CALL_RELEASECOMPLETE);       /*!< Generated when call instance deleted */
            caseretstr(CMBS_EV_DEE_CALL_INBANDINFO);            /*!< Events created for inband keys */
            caseretstr(CMBS_EV_DCM_CALL_STATE);                 /*!< Call state */
            caseretstr(CMBS_EV_DEE_CALL_MEDIA_OFFER);           /*!< Offer media */
            caseretstr(CMBS_EV_DEE_CALL_MEDIA_OFFER_RES);       /*!< Response to CMBS_EV_DEE_CALL_MEDIA_OFFER */
            caseretstr(CMBS_EV_DEE_CALL_MEDIA_UPDATE);          /*!< Received when cordless module updated the media */
            caseretstr(CMBS_EV_DEE_CALL_HOLD);                  /*!< Generated on call HOLD */
            caseretstr(CMBS_EV_DEE_CALL_RESUME);                /*!< Generated on call RESUME */
            caseretstr(CMBS_EV_DSR_HS_PAGE_PROGRESS);           /*!< Events for various handset locator progress states */
            caseretstr(CMBS_EV_DSR_HS_PAGE_ANSWER);             /*!< Generated when a handset locator is answered */
            caseretstr(CMBS_EV_DEE_CALL_HOLD_RES);              /*!< this is response for the call hold request. */
            caseretstr(CMBS_EV_DEE_CALL_RESUME_RES);            /*!< this is response for the call response request */
            caseretstr(CMBS_EV_DEM_CHANNEL_START);              /*!< Start sending (voice) data on a particular channel */
            caseretstr(CMBS_EV_DEM_CHANNEL_START_RES);          /*!< Response to CMBS_EV_DEM_CHANNEL_START */
            caseretstr(CMBS_EV_DEM_CHANNEL_INTERNAL_CONNECT);   /*!< Modify the IN/Out Connection for Media Channels */
            caseretstr(CMBS_EV_DEM_CHANNEL_INTERNAL_CONNECT_RES);/*!< Response to CMBS_EV_DEM_CHANNEL_INTERNAL_CONNECT */
            caseretstr(CMBS_EV_DEM_CHANNEL_STOP);               /*!< Stop sending data on a particular channel */
            caseretstr(CMBS_EV_DEM_CHANNEL_STOP_RES);           /*!< Response to CMBS_EV_DEM_CHANNEL_STOP */
            caseretstr(CMBS_EV_DEM_TONE_START);                 /*!< Start the tone generation on a particular media channel */
            caseretstr(CMBS_EV_DEM_TONE_START_RES);             /*!< Response to CMBS_EV_DEM_TONE_START */
            caseretstr(CMBS_EV_DEM_TONE_STOP);                  /*!< Stop tone generation on a particular media channel */
            caseretstr(CMBS_EV_DEM_TONE_STOP_RES);              /*!< Response to CMBS_EV_DEM_TONE_STOP */
            caseretstr(CMBS_EV_DSR_SYS_LOG_START);              /*!< Start system logging */
            caseretstr(CMBS_EV_DSR_SYS_LOG_STOP);               /*!< Stop system logging */
            caseretstr(CMBS_EV_DSR_SYS_LOG_REQ);                /*!< Request to get content of the log buffer */
            caseretstr(CMBS_EV_DSR_PARAM_UPDATED);
            caseretstr(CMBS_EV_DSR_PARAM_AREA_UPDATED);
            caseretstr(CMBS_EV_DSR_PARAM_AREA_GET);
            caseretstr(CMBS_EV_DSR_PARAM_AREA_GET_RES);
            caseretstr(CMBS_EV_DSR_PARAM_AREA_SET);
            caseretstr(CMBS_EV_DSR_PARAM_AREA_SET_RES);

            caseretstr(CMBS_EV_DSR_GEN_SEND_MWI);               /*!< Send Voice Message Waiting Indication to one or more handsets */
            caseretstr(CMBS_EV_DSR_GEN_SEND_MWI_RES);           /*!< Response to CMBS_EV_DSR_GEN_SEND_MWI */
            caseretstr(CMBS_EV_DSR_GEN_SEND_MISSED_CALLS);      /*!< Send Missed Calls Indication to one or more handsets */
            caseretstr(CMBS_EV_DSR_GEN_SEND_MISSED_CALLS_RES);  /*!< Response to CMBS_EV_DSR_GEN_SEND_MISSED_CALLS */
            caseretstr(CMBS_EV_DSR_GEN_SEND_LIST_CHANGED);      /*!< Send List Changed event to one or more handsets */
            caseretstr(CMBS_EV_DSR_GEN_SEND_LIST_CHANGED_RES);  /*!< Response to CMBS_EV_DSR_GEN_SEND_LIST_CHANGED */
            caseretstr(CMBS_EV_DSR_GEN_SEND_WEB_CONTENT);       /*!< Send Web Content event to one or more handsets */
            caseretstr(CMBS_EV_DSR_GEN_SEND_WEB_CONTENT_RES);   /*!< Response to CMBS_EV_DSR_GEN_SEND_WEB_CONTENT */
            caseretstr(CMBS_EV_DSR_GEN_SEND_PROP_EVENT);        /*!< Send Escape to Proprietary event to one or more handsets */
            caseretstr(CMBS_EV_DSR_GEN_SEND_PROP_EVENT_RES);    /*!< Response to CMBS_EV_DSR_GEN_SEND_PROP_EVENT */
            caseretstr(CMBS_EV_DSR_TIME_UPDATE);                /*!< Send Time-Date update event to one or more handsets */
            caseretstr(CMBS_EV_DSR_TIME_UPDATE_RES);            /*!< Response to CMBS_EV_DSR_TIME_UPDATE */
            caseretstr(CMBS_EV_DSR_TIME_INDICATION);            /*!< Event received when a handset has updated its Time-Date setting */
            caseretstr(CMBS_EV_DSR_HS_DATA_SESSION_OPEN);
            caseretstr(CMBS_EV_DSR_HS_DATA_SESSION_OPEN_RES);
            caseretstr(CMBS_EV_DSR_HS_DATA_SESSION_CLOSE);
            caseretstr(CMBS_EV_DSR_HS_DATA_SESSION_CLOSE_RES);
            caseretstr(CMBS_EV_DSR_HS_DATA_SEND);
            caseretstr(CMBS_EV_DSR_HS_DATA_SEND_RES);
            caseretstr(CMBS_EV_DSR_LA_SESSION_START);
            caseretstr(CMBS_EV_DSR_LA_SESSION_START_RES);
            caseretstr(CMBS_EV_DSR_LA_SESSION_END);
            caseretstr(CMBS_EV_DSR_LA_SESSION_END_RES);
            caseretstr(CMBS_EV_DSR_LA_QUERY_SUPP_ENTRY_FIELDS);
            caseretstr(CMBS_EV_DSR_LA_QUERY_SUPP_ENTRY_FIELDS_RES);
            caseretstr(CMBS_EV_DSR_LA_READ_ENTRIES);
            caseretstr(CMBS_EV_DSR_LA_READ_ENTRIES_RES);
            caseretstr(CMBS_EV_DSR_LA_SEARCH_ENTRIES);
            caseretstr(CMBS_EV_DSR_LA_SEARCH_ENTRIES_RES);
            caseretstr(CMBS_EV_DSR_LA_EDIT_ENTRY);
            caseretstr(CMBS_EV_DSR_LA_EDIT_ENTRY_RES);
            caseretstr(CMBS_EV_DSR_LA_SAVE_ENTRY);
            caseretstr(CMBS_EV_DSR_LA_SAVE_ENTRY_RES);
            caseretstr(CMBS_EV_DSR_LA_DELETE_ENTRY);
            caseretstr(CMBS_EV_DSR_LA_DELETE_ENTRY_RES);
            caseretstr(CMBS_EV_DSR_LA_DELETE_LIST);
            caseretstr(CMBS_EV_DSR_LA_DELETE_LIST_RES);
            caseretstr(CMBS_EV_DSR_LA_DATA_PACKET_RECEIVE);
            caseretstr(CMBS_EV_DSR_LA_DATA_PACKET_RECEIVE_RES);
            caseretstr(CMBS_EV_DSR_LA_DATA_PACKET_SEND);
            caseretstr(CMBS_EV_DSR_LA_DATA_PACKET_SEND_RES);
            caseretstr(CMBS_EV_DCM_CALL_TRANSFER);
            caseretstr(CMBS_EV_DCM_CALL_TRANSFER_RES);
            caseretstr(CMBS_EV_DCM_CALL_CONFERENCE);
            caseretstr(CMBS_EV_DCM_CALL_CONFERENCE_RES);
            caseretstr(CMBS_EV_DSR_TARGET_UP);
            caseretstr(CMBS_EV_DEE_HANDSET_LINK_RELEASE);

            caseretstr(CMBS_EV_DSR_GPIO_CONNECT);
            caseretstr(CMBS_EV_DSR_GPIO_CONNECT_RES);
            caseretstr(CMBS_EV_DSR_GPIO_DISCONNECT);
            caseretstr(CMBS_EV_DSR_GPIO_DISCONNECT_RES);
            caseretstr(CMBS_EV_DSR_ATE_TEST_START);
            caseretstr(CMBS_EV_DSR_ATE_TEST_START_RES);
            caseretstr(CMBS_EV_DSR_ATE_TEST_LEAVE);
            caseretstr(CMBS_EV_DSR_ATE_TEST_LEAVE_RES);

            caseretstr(CMBS_EV_DSR_SUOTA_HS_VERSION_RECEIVED);
            caseretstr(CMBS_EV_DSR_SUOTA_URL_RECEIVED);
            caseretstr(CMBS_EV_DSR_SUOTA_NACK_RECEIVED);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_SW_UPD_IND);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_SW_UPD_IND_RES);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_VERS_AVAIL);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_VERS_AVAIL_RES);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_URL);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_URL_RES);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_NACK);
            caseretstr(CMBS_EV_DSR_SUOTA_SEND_NACK_RES);

            caseretstr(CMBS_EV_DSR_TARGET_LIST_CHANGE_NOTIF);

            caseretstr(CMBS_EV_DSR_HW_VERSION_GET);
            caseretstr(CMBS_EV_DSR_HW_VERSION_GET_RES);

            caseretstr(CMBS_EV_DSR_DECT_SETTINGS_LIST_GET);
            caseretstr(CMBS_EV_DSR_DECT_SETTINGS_LIST_GET_RES);
            caseretstr(CMBS_EV_DSR_DECT_SETTINGS_LIST_SET);
            caseretstr(CMBS_EV_DSR_DECT_SETTINGS_LIST_SET_RES);

            caseretstr(CMBS_EV_RTP_SESSION_START);
            caseretstr(CMBS_EV_RTP_SESSION_START_RES);
            caseretstr(CMBS_EV_RTP_SESSION_STOP);
            caseretstr(CMBS_EV_RTP_SESSION_STOP_RES);
            caseretstr(CMBS_EV_RTP_SESSION_UPDATE);
            caseretstr(CMBS_EV_RTP_SESSION_UPDATE_RES);
            caseretstr(CMBS_EV_RTCP_SESSION_START);
            caseretstr(CMBS_EV_RTCP_SESSION_START_RES);
            caseretstr(CMBS_EV_RTCP_SESSION_STOP);
            caseretstr(CMBS_EV_RTCP_SESSION_STOP_RES);
            caseretstr(CMBS_EV_RTP_SEND_DTMF);
            caseretstr(CMBS_EV_RTP_SEND_DTMF_RES);
            caseretstr(CMBS_EV_RTP_DTMF_NOTIFICATION);
            caseretstr(CMBS_EV_RTP_FAX_TONE_NOTIFICATION);
            caseretstr(CMBS_EV_RTP_ENABLE_FAX_AUDIO_PROCESSING_MODE);
            caseretstr(CMBS_EV_RTP_ENABLE_FAX_AUDIO_PROCESSING_MODE_RES);
            caseretstr(CMBS_EV_RTP_DISABLE_FAX_AUDIO_PROCESSING_MODE);
            caseretstr(CMBS_EV_RTP_DISABLE_FAX_AUDIO_PROCESSING_MODE_RES);
            caseretstr(CMBS_EV_DCM_INTERNAL_TRANSFER);
            caseretstr(CMBS_EV_DSR_ADD_EXTENSION);
            caseretstr(CMBS_EV_DSR_ADD_EXTENSION_RES);
            caseretstr(CMBS_EV_DSR_RESERVED);
            caseretstr(CMBS_EV_DSR_RESERVED_RES);
            caseretstr(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS);
            caseretstr(CMBS_EV_DSR_LA_ADD_TO_SUPP_LISTS_RES);
            caseretstr(CMBS_EV_DSR_LA_PROP_CMD);
            caseretstr(CMBS_EV_DSR_LA_PROP_CMD_RES);
            caseretstr(CMBS_EV_DSR_ENCRYPT_DISABLE);
            caseretstr(CMBS_EV_DSR_ENCRYPT_ENABLE);
            caseretstr(CMBS_EV_DSR_SET_BASE_NAME);
            caseretstr(CMBS_EV_DSR_SET_BASE_NAME_RES);
            caseretstr(CMBS_EV_DSR_FIXED_CARRIER);
            caseretstr(CMBS_EV_DSR_FIXED_CARRIER_RES);
            caseretstr(CMBS_EV_DEE_HS_CODEC_CFM_FAILED);
            caseretstr(CMBS_EV_DSR_EEPROM_SIZE_GET);
            caseretstr(CMBS_EV_DSR_EEPROM_SIZE_GET_RES);
            caseretstr(CMBS_EV_DSR_RECONNECT_REQ);
            caseretstr(CMBS_EV_DSR_RECONNECT_RES);
            caseretstr(CMBS_EV_DSR_GET_BASE_NAME);
            caseretstr(CMBS_EV_DSR_GET_BASE_NAME_RES);
            caseretstr(CMBS_EV_DSR_EEPROM_VERSION_GET);
            caseretstr(CMBS_EV_DSR_EEPROM_VERSION_GET_RES);
            caseretstr(CMBS_EV_DSR_START_DECT_LOGGER);
            caseretstr(CMBS_EV_DSR_START_DECT_LOGGER_RES);
            caseretstr(CMBS_EV_DSR_STOP_AND_READ_DECT_LOGGER);
            caseretstr(CMBS_EV_DSR_STOP_AND_READ_DECT_LOGGER_RES);
            caseretstr(CMBS_EV_DSR_DECT_DATA_IND);
            caseretstr(CMBS_EV_DSR_DECT_DATA_IND_RES);
            caseretstr(CMBS_EV_DSR_DC_SESSION_START);
            caseretstr(CMBS_EV_DSR_DC_SESSION_START_RES);
            caseretstr(CMBS_EV_DSR_DC_SESSION_STOP);
            caseretstr(CMBS_EV_DSR_DC_SESSION_STOP_RES);
            caseretstr(CMBS_EV_DSR_DC_DATA_SEND);
            caseretstr(CMBS_EV_DSR_DC_DATA_SEND_RES);
            caseretstr(CMBS_EV_DSR_PING);
            caseretstr(CMBS_EV_DSR_PING_RES);

            caseretstr(CMBS_EV_DSR_SUOTA_SESSION_CREATE);
            caseretstr(CMBS_EV_DSR_SUOTA_SESSION_CREATE_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_OPEN_SESSION);
            caseretstr(CMBS_EV_DSR_SUOTA_OPEN_SESSION_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_DATA_SEND);
            caseretstr(CMBS_EV_DSR_SUOTA_DATA_SEND_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_REG_CPLANE_CB);
            caseretstr(CMBS_EV_DSR_SUOTA_REG_CPLANE_CB_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_REG_APP_CB);
            caseretstr(CMBS_EV_DSR_SUOTA_REG_APP_CB_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_DATA_RECV);
            caseretstr(CMBS_EV_DSR_SUOTA_DATA_RECV_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_HS_VER_IND_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_SESSION_CLOSE);
            caseretstr(CMBS_EV_DSR_SUOTA_SESSION_CLOSE_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_CONTROL_SET);
            caseretstr(CMBS_EV_DSR_SUOTA_CONTROL_SET_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_COTROL_RESET);
            caseretstr(CMBS_EV_DSR_SUOTA_COTROL_RESET_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_UPDATE_OPTIONAL_GRP);
            caseretstr(CMBS_EV_DSR_SUOTA_UPDATE_OPTIONAL_GRP_ACK);
            caseretstr(CMBS_EV_DSR_SUOTA_FACILITY_CB);
            caseretstr(CMBS_EV_DSR_SUOTA_PUSH_MODE);
            caseretstr(CMBS_EV_DSR_SUOTA_UPLANE_COMMANDS_END);
            caseretstr(CMBS_EV_DSR_HS_PROP_EVENT);
            caseretstr(CMBS_EV_DSR_FW_APP_INVALIDATE);
            caseretstr(CMBS_EV_DSR_FW_APP_INVALIDATE_RES);

            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_CONNECT);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_CONNECT_RES);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_ENABLE);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_ENABLE_RES);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_DISABLE);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_DISABLE_RES);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_GAIN);
            caseretstr(CMBS_EV_DSR_AFE_ENDPOINT_GAIN_RES);
            caseretstr(CMBS_EV_DSR_AFE_AUX_MEASUREMENT);
            caseretstr(CMBS_EV_DSR_AFE_AUX_MEASUREMENT_RES);
            caseretstr(CMBS_EV_DSR_AFE_AUX_MEASUREMENT_RESULT);
            caseretstr(CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE);
            caseretstr(CMBS_EV_DSR_AFE_CHANNEL_ALLOCATE_RES);
            caseretstr(CMBS_EV_DSR_AFE_CHANNEL_DEALLOCATE);
            caseretstr(CMBS_EV_DSR_AFE_CHANNEL_DEALLOCATE_RES);
            caseretstr(CMBS_EV_DSR_DHSG_SEND_BYTE);
            caseretstr(CMBS_EV_DSR_DHSG_SEND_BYTE_RES);
            caseretstr(CMBS_EV_DSR_DHSG_NEW_DATA_RCV);
            caseretstr(CMBS_EV_DSR_GPIO_ENABLE);
            caseretstr(CMBS_EV_DSR_GPIO_ENABLE_RES);
            caseretstr(CMBS_EV_DSR_GPIO_DISABLE);
            caseretstr(CMBS_EV_DSR_GPIO_DISABLE_RES);
            caseretstr(CMBS_EV_DSR_GPIO_CONFIG_SET);
            caseretstr(CMBS_EV_DSR_GPIO_CONFIG_SET_RES);
            caseretstr(CMBS_EV_DSR_GPIO_CONFIG_GET);
            caseretstr(CMBS_EV_DSR_GPIO_CONFIG_GET_RES);
            caseretstr(CMBS_EV_DSR_TURN_ON_NEMO_RES);
            caseretstr(CMBS_EV_DSR_TURN_OFF_NEMO_RES);

            caseretstr(CMBS_EV_DSR_EXT_INT_CONFIG);
            caseretstr(CMBS_EV_DSR_EXT_INT_CONFIG_RES);
            caseretstr(CMBS_EV_DSR_EXT_INT_ENABLE);
            caseretstr(CMBS_EV_DSR_EXT_INT_ENABLE_RES);
            caseretstr(CMBS_EV_DSR_EXT_INT_DISABLE);
            caseretstr(CMBS_EV_DSR_EXT_INT_DISABLE_RES);
            caseretstr(CMBS_EV_DSR_EXT_INT_INDICATION);
            caseretstr(CMBS_EV_DSR_LOCATE_SUGGEST_REQ);
            caseretstr(CMBS_EV_DSR_LOCATE_SUGGEST_RES);
            caseretstr(CMBS_EV_DSR_TERMINAL_CAPABILITIES_IND);
            caseretstr(CMBS_EV_DSR_HS_PROP_DATA_RCV_IND);
            caseretstr(CMBS_EV_CHECKSUM_FAILURE);

            caseretstr(CMBS_EV_DSR_USER_DEFINED_START);
            caseretstr(CMBS_EV_DSR_USER_DEFINED_END);

            caseretstr(CMBS_EV_MAX);

        case CMBS_EV_DSR_HAN_DEFINED_START:
        case CMBS_EV_DSR_HAN_DEFINED_END:
            break; // handled later
    }

    if (CMBS_EV_DSR_HAN_DEFINED_START <= id && id <= CMBS_EV_DSR_HAN_DEFINED_END)
    {
        switch ((E_CMBS_HAN_EVENT_ID)id)
        {
                caseretstr(CMBS_EV_DSR_HAN_MNGR_INIT);
                caseretstr(CMBS_EV_DSR_HAN_MNGR_INIT_RES);
                caseretstr(CMBS_EV_DSR_HAN_MNGR_START);
                caseretstr(CMBS_EV_DSR_HAN_MNGR_START_RES);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_READ_TABLE);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_READ_TABLE_RES);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_WRITE_TABLE_RES);
                caseretstr(CMBS_EV_DSR_HAN_BIND_READ_TABLE);
                caseretstr(CMBS_EV_DSR_HAN_BIND_READ_TABLE_RES);
                caseretstr(CMBS_EV_DSR_HAN_BIND_WRITE_TABLE);
                caseretstr(CMBS_EV_DSR_HAN_BIND_WRITE_TABLE_RES);
                caseretstr(CMBS_EV_DSR_HAN_GROUP_READ_TABLE);
                caseretstr(CMBS_EV_DSR_HAN_GROUP_READ_TABLE_RES);
                caseretstr(CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE);
                caseretstr(CMBS_EV_DSR_HAN_GROUP_WRITE_TABLE_RES);
                caseretstr(CMBS_EV_DSR_HAN_MSG_RECV_REGISTER);
                caseretstr(CMBS_EV_DSR_HAN_MSG_RECV_REGISTER_RES);
                caseretstr(CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER);
                caseretstr(CMBS_EV_DSR_HAN_MSG_RECV_UNREGISTER_RES);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_RES);
                caseretstr(CMBS_EV_DSR_HAN_MSG_RECV);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_TX_START_REQUEST_RES);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_TX_END_REQUEST_RES);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_TX_READY);
                caseretstr(CMBS_EV_DSR_HAN_MSG_SEND_TX_ENDED);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_1_NOTIFICATION);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_2_NOTIFICATION);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_REG_STAGE_3_NOTIFICATION);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_FORCEFUL_DELETE_RES);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_REG_DELETED);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_UNKNOWN_DEV_CONTACT);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS);
                caseretstr(CMBS_EV_DSR_HAN_DEVICE_GET_CONNECTION_STATUS_RES);
                caseretstr(CMBS_EV_DSR_HAN_MAX);
        }
    }

    if (CMBS_EV_DSR_USER_DEFINED_START <= id && id <= CMBS_EV_DSR_USER_DEFINED_END)
    {
        return "CMBS_EV_DSR_USER_DEFINED";
    }

    return "Unknown Event";
}

//  ==========  cmbs_dbg_GetCommandName ===========
/*!
 \brief    return command string
 \param[in,out]  id   CMBSinternal command
 \return    <const char * >
*/

const char *  cmbs_dbg_GetCommandName(CMBS_CMD id)
{
    switch (id)
    {
            caseretstr(CMBS_CMD_HELLO);
            caseretstr(CMBS_CMD_HELLO_RPLY);
            caseretstr(CMBS_CMD_FLOW_NOK);
            caseretstr(CMBS_CMD_FLOW_RESTART);
            caseretstr(CMBS_CMD_RESET);
            caseretstr(CMBS_CMD_FLASH_START_REQ);
            caseretstr(CMBS_CMD_FLASH_START_RES);
            caseretstr(CMBS_CMD_FLASH_STOP_REQ);
            caseretstr(CMBS_CMD_FLASH_STOP_RES);
            caseretstr(CMBS_CMD_CAPABILITIES);
            caseretstr(CMBS_CMD_CAPABILITIES_RPLY);

        default:
            break;
    }

    return "Unknown CMBS command";
}

//  ==========  cmbs_dbg_GetParamName ===========
/*!
 \brief            return cmbs parameter string
 \param[in]        parameter id
 \return        <const char * >
*/
const char *  cmbs_dbg_GetParamName(E_CMBS_PARAM e_ParamId)
{
    switch (e_ParamId)
    {
            caseretstr(CMBS_PARAM_RFPI);
            caseretstr(CMBS_PARAM_RVBG);
            caseretstr(CMBS_PARAM_RVREF);
            caseretstr(CMBS_PARAM_RXTUN);
            caseretstr(CMBS_PARAM_MASTER_PIN);
            caseretstr(CMBS_PARAM_AUTH_PIN);
            caseretstr(CMBS_PARAM_COUNTRY);
            caseretstr(CMBS_PARAM_SIGNALTONE_DEFAULT);
            caseretstr(CMBS_PARAM_TEST_MODE);
            caseretstr(CMBS_PARAM_AUTO_REGISTER);
            caseretstr(CMBS_PARAM_NTP);
            caseretstr(CMBS_PARAM_ECO_MODE);
            caseretstr(CMBS_PARAM_RESET_ALL);
            caseretstr(CMBS_PARAM_SUBS_DATA);
            caseretstr(CMBS_PARAM_AUXBGPROG);
            caseretstr(CMBS_PARAM_ADC_MEASUREMENT);
            caseretstr(CMBS_PARAM_PMU_MEASUREMENT);
            caseretstr(CMBS_PARAM_RSSI_VALUE);
            caseretstr(CMBS_PARAM_DECT_TYPE);
            caseretstr(CMBS_PARAM_MAX_NUM_ACT_CALLS_PT);
            caseretstr(CMBS_PARAM_ANT_SWITCH_MASK);
            caseretstr(CMBS_PARAM_PORBGCFG);
            caseretstr(CMBS_PARAM_AUXBGPROG_DIRECT);
            caseretstr(CMBS_PARAM_BERFER_VALUE);
            caseretstr(CMBS_PARAM_INBAND_COUNTRY);
            caseretstr(CMBS_PARAM_FP_CUSTOM_FEATURES);
            caseretstr(CMBS_PARAM_HAN_DECT_SUB_DB_START);
            caseretstr(CMBS_PARAM_HAN_DECT_SUB_DB_END);
            caseretstr(CMBS_PARAM_HAN_ULE_SUB_DB_START);
            caseretstr(CMBS_PARAM_HAN_ULE_SUB_DB_END);
            caseretstr(CMBS_PARAM_HAN_FUN_SUB_DB_START);
            caseretstr(CMBS_PARAM_HAN_FUN_SUB_DB_END);
            caseretstr(CMBS_PARAM_DHSG_ENABLE);
            caseretstr(CMBS_PARAM_AUXBGP_DCIN);
            caseretstr(CMBS_PARAM_AUXBGP_RESISTOR_FACTOR);
        default:
            break;
    }

    return "Unknown CMBS parameter";
}

//  ========== cmbs_dbg_GetIEName ===========
/*!
 \brief            returns Information Element enumerator definition string
 \param[in]        id    CMBSinternal command
 \return           <const char * >
*/
const char *  cmbs_dbg_GetIEName(E_CMBS_IE_TYPE e_IE)
{
    switch (e_IE)
    {
            caseretstr(CMBS_IE_UNDEF);
            caseretstr(CMBS_IE_CALLINSTANCE);
            caseretstr(CMBS_IE_CALLERPARTY);
            caseretstr(CMBS_IE_CALLERNAME);
            caseretstr(CMBS_IE_CALLEDPARTY);
            caseretstr(CMBS_IE_CALLPROGRESS);
            caseretstr(CMBS_IE_CALLINFO);
            caseretstr(CMBS_IE_DISPLAY_STRING);
            caseretstr(CMBS_IE_CALLRELEASE_REASON);
            caseretstr(CMBS_IE_CALLSTATE);
            caseretstr(CMBS_IE_MEDIACHANNEL);
            caseretstr(CMBS_IE_MEDIA_INTERNAL_CONNECT);
            caseretstr(CMBS_IE_MEDIADESCRIPTOR);
            caseretstr(CMBS_IE_TONE);
            caseretstr(CMBS_IE_TIMEOFDAY);
            caseretstr(CMBS_IE_HANDSETINFO);
            caseretstr(CMBS_IE_PARAMETER);
            caseretstr(CMBS_IE_SUBSCRIBED_HS_LIST);
            caseretstr(CMBS_IE_LINE_SETTINGS_LIST);
            caseretstr(CMBS_IE_RESERVED_1);
            caseretstr(CMBS_IE_FW_VERSION);
            caseretstr(CMBS_IE_SYS_LOG);
            caseretstr(CMBS_IE_RESPONSE);
            caseretstr(CMBS_IE_STATUS);
            caseretstr(CMBS_IE_INTEGER_VALUE);
            caseretstr(CMBS_IE_LINE_ID);
            caseretstr(CMBS_IE_PARAMETER_AREA);
            caseretstr(CMBS_IE_REQUEST_ID);
            caseretstr(CMBS_IE_HANDSETS);
            caseretstr(CMBS_IE_GEN_EVENT);
            caseretstr(CMBS_IE_PROP_EVENT);
            caseretstr(CMBS_IE_DATETIME);
            caseretstr(CMBS_IE_DATA);
            caseretstr(CMBS_IE_DATA_SESSION_ID);
            caseretstr(CMBS_IE_DATA_SESSION_TYPE);
            caseretstr(CMBS_IE_LA_SESSION_ID);
            caseretstr(CMBS_IE_LA_LIST_ID);
            caseretstr(CMBS_IE_LA_FIELDS);
            caseretstr(CMBS_IE_LA_SORT_FIELDS);
            caseretstr(CMBS_IE_LA_EDIT_FIELDS);
            caseretstr(CMBS_IE_LA_CONST_FIELDS);
            caseretstr(CMBS_IE_LA_SEARCH_CRITERIA);
            caseretstr(CMBS_IE_LA_ENTRY_ID);
            caseretstr(CMBS_IE_LA_ENTRY_INDEX);
            caseretstr(CMBS_IE_LA_ENTRY_COUNT);
            caseretstr(CMBS_IE_LA_IS_LAST);
            caseretstr(CMBS_IE_LA_REJECT_REASON);
            caseretstr(CMBS_IE_LA_NR_OF_ENTRIES);
            caseretstr(CMBS_IE_CALLTRANSFERREQ);
            caseretstr(CMBS_IE_HS_NUMBER);
            caseretstr(CMBS_IE_SHORT_VALUE);
            caseretstr(CMBS_IE_ATE_SETTINGS);
            caseretstr(CMBS_IE_LA_READ_DIRECTION);
            caseretstr(CMBS_IE_LA_MARK_REQUEST);
            caseretstr(CMBS_IE_LINE_SUBTYPE);
            caseretstr(CMBS_IE_AVAIL_VERSION_DETAILS);
            caseretstr(CMBS_IE_HS_VERSION_BUFFER);
            caseretstr(CMBS_IE_HS_VERSION_DETAILS);
            caseretstr(CMBS_IE_SU_SUBTYPE);
            caseretstr(CMBS_IE_URL);
            caseretstr(CMBS_IE_NUM_OF_URLS);
            caseretstr(CMBS_IE_REJECT_REASON);
            caseretstr(CMBS_IE_NB_CODEC_OTA);
            caseretstr(CMBS_IE_TARGET_LIST_CHANGE_NOTIF);
            caseretstr(CMBS_IE_HW_VERSION);
            caseretstr(CMBS_IE_DECT_SETTINGS_LIST);
            caseretstr(CMBS_IE_RTP_SESSION_INFORMATION);
            caseretstr(CMBS_IE_RTCP_INTERVAL);
            caseretstr(CMBS_IE_RTP_DTMF_EVENT);
            caseretstr(CMBS_IE_RTP_DTMF_EVENT_INFO);
            caseretstr(CMBS_IE_RTP_FAX_TONE_TYPE);
            caseretstr(CMBS_IE_INTERNAL_TRANSFER);
            caseretstr(CMBS_IE_LA_PROP_CMD);
            caseretstr(CMBS_IE_MELODY);
            caseretstr(CMBS_IE_BASE_NAME);
            caseretstr(CMBS_IE_REG_CLOSE_REASON);
            caseretstr(CMBS_IE_EEPROM_VERSION);
            caseretstr(CMBS_IE_DC_REJECT_REASON);
            caseretstr(CMBS_IE_SUOTA_APP_ID);
            caseretstr(CMBS_IE_SUOTA_SESSION_ID);
            caseretstr(CMBS_IE_HS_PROP_EVENT);
            caseretstr(CMBS_IE_SYPO_SPECIFICATION);
            caseretstr(CMBS_IE_AFE_ENDPOINT_CONNECT);
            caseretstr(CMBS_IE_AFE_ENDPOINT);
            caseretstr(CMBS_IE_AFE_ENDPOINT_GAIN);
            caseretstr(CMBS_IE_AFE_ENDPOINT_GAIN_DB);
            caseretstr(CMBS_IE_AFE_AUX_MEASUREMENT_SETTINGS);
            caseretstr(CMBS_IE_AFE_AUX_MEASUREMENT_RESULT);
            caseretstr(CMBS_IE_AFE_RESOURCE_TYPE);
            caseretstr(CMBS_IE_AFE_INSTANCE_NUM);
            caseretstr(CMBS_IE_DHSG_VALUE);
            caseretstr(CMBS_IE_GPIO_ID);
            caseretstr(CMBS_IE_GPIO_MODE);
            caseretstr(CMBS_IE_GPIO_VALUE);
            caseretstr(CMBS_IE_GPIO_PULL_TYPE);
            caseretstr(CMBS_IE_GPIO_PULL_ENA);
            caseretstr(CMBS_IE_GPIO_ENA);

            caseretstr(CMBS_IE_CALLEDNAME);

            caseretstr(CMBS_IE_EXT_INT_CONFIGURATION);
            caseretstr(CMBS_IE_EXT_INT_NUM);
            caseretstr(CMBS_IE_TERMINAL_CAPABILITIES);
            caseretstr(CMBS_IE_CALL_HOLD_REASON);

            caseretstr(CMBS_IE_MAX);

        case CMBS_IE_HAN_DEFINED_START:
        case CMBS_IE_HAN_DEFINED_END:
        default:
            break;
    }

    if (CMBS_IE_HAN_DEFINED_START <= e_IE && e_IE <= CMBS_IE_HAN_DEFINED_END)
    {
        switch ((E_CMBS_IE_HAN_TYPE)e_IE)
        {
                caseretstr(CMBS_IE_HAN_MSG);
                caseretstr(CMBS_IE_HAN_MSG_CONTROL);
                caseretstr(CMBS_IE_HAN_DEVICE_TABLE_BRIEF_ENTRIES);
                caseretstr(CMBS_IE_HAN_DEVICE_TABLE_EXTENDED_ENTRIES);
                caseretstr(CMBS_IE_HAN_BIND_TABLE_ENTRIES);
                caseretstr(CMBS_IE_HAN_GROUP_TABLE_ENTRIES);
                caseretstr(CMBS_IE_HAN_DEVICE);
                caseretstr(CMBS_IE_HAN_CFG);
                caseretstr(CMBS_IE_HAN_NUM_OF_ENTRIES);
                caseretstr(CMBS_IE_HAN_INDEX_1ST_ENTRY);
                caseretstr(CMBS_IE_HAN_MSG_REG_INFO);
                caseretstr(CMBS_IE_HAN_TABLE_UPDATE_INFO);
                caseretstr(CMBS_IE_HAN_MAX);
                caseretstr(CMBS_IE_HAN_TABLE_ENTRY_TYPE);
                caseretstr(CMBS_IE_HAN_GENERAL_STATUS);
                caseretstr(CMBS_IE_HAN_SEND_FAIL_REASON);
                caseretstr(CMBS_IE_HAN_TX_ENDED_REASON);
                caseretstr(CMBS_IE_HAN_DEVICE_FORCEFUL_DELETE_ERROR_STATUS_REASON);
                caseretstr(CMBS_IE_HAN_DEVICE_REG_ERROR_REASON);
                caseretstr(CMBS_IE_HAN_DEVICE_REG_STAGE1_OK_STATUS_PARAMS);
                caseretstr(CMBS_IE_HAN_DEVICE_REG_STAGE2_OK_STATUS_PARAMS);
                caseretstr(CMBS_IE_HAN_UNKNOWN_DEVICE_CONTACTED_PARAMS);
                caseretstr(CMBS_IE_HAN_BASE_INFO);
                caseretstr(CMBS_IE_HAN_DEVICE_CONNECTION_STATUS);

        }
    }

    return "Unknown Information Element";
}

//  ========== cmbs_dbg_GetHsTypeName ===========
/*!
 \brief           return string of enumeration
 \param[in]   e_Param  value of enumeration
 \return    <const char *>   return string of enumeration
*/
const char * cmbs_dbg_GetHsTypeName(E_CMBS_HS_TYPE e_HsType)
{
    switch (e_HsType)
    {
            caseretstr(CMBS_HS_TYPE_GAP);                /*!< GAP handset */
            caseretstr(CMBS_HS_TYPE_CATIQ_1);            /*!< CATiq 1.0 compliant handset */
            caseretstr(CMBS_HS_TYPE_CATIQ_2);            /*!< CATiq 2.0 compliant handset */
            caseretstr(CMBS_HS_TYPE_DSPG);               /*!< DSPG handset */

        default:
            return "Handset type not defined";
    }
}

//  ========== cmbs_dbg_GetToneName ===========
/*!
 \brief           return string of enumeration
 \param[in]   e_Param  value of enumeration
 \return    <const char *>   return string of enumeration
*/
const char * cmbs_dbg_GetToneName(E_CMBS_TONE e_Tone)
{
    switch (e_Tone)
    {
            caseretstr(CMBS_TONE_DIAL);                  /*!< Dial tone according country spec. */
            caseretstr(CMBS_TONE_MWI);                   /*!< Message waiting tone according country spec. */
            caseretstr(CMBS_TONE_RING_BACK);             /*!< Ring-back tone according country spec. */
            caseretstr(CMBS_TONE_BUSY);                  /*!< Busy tone according country spec. */
            caseretstr(CMBS_TONE_CALL_WAITING);          /*!< Call waiting tone according country spec. */
            caseretstr(CMBS_TONE_DIAL_CALL_FORWARD);     /*!< Dial tone French or Call Forwarding according country spec. */
            caseretstr(CMBS_TONE_MWI_OR_CONGESTION);     /*!< MWI tone French or Congestion Swiss accordiog country spec. */

            caseretstr(CMBS_TONE_RING_BACK_FT_FR);             /*!< Ring-back tone according country spec. */
            caseretstr(CMBS_TONE_BUSY_FT_FR);                  /*!< Busy tone according country spec. */
            caseretstr(CMBS_TONE_CALL_WAITING_FT_FR);          /*!< Call waiting tone according country spec. */

            caseretstr(CMBS_TONE_DIAL_FT_PL);                  /*!< Dial tone according country spec. */
            caseretstr(CMBS_TONE_MWI_FT_PL);                   /*!< Stutter dial tone according country spec. */
            caseretstr(CMBS_TONE_RING_BACK_FT_PL);             /*!< Ring-back tone according country spec. */
            caseretstr(CMBS_TONE_BUSY_FT_PL);                  /*!< Busy tone according country spec. */
            caseretstr(CMBS_TONE_CALL_WAITING_FT_PL);          /*!< Call waiting tone according country spec.  */

            caseretstr(CMBS_TONE_DIAL_OUTBAND);
            caseretstr(CMBS_TONE_RING_BACK_OUTBAND);
            caseretstr(CMBS_TONE_BUSY_OUTBAND);
            caseretstr(CMBS_TONE_CALL_WAITING_OUTBAND);
            caseretstr(CMBS_TONE_OFF_OUTBAND);

            caseretstr(CMBS_TONE_INTERCEPT);             /*!< Interception tone */
            caseretstr(CMBS_TONE_NWK_CONGESTION);        /*!< Network congestion tone */
            caseretstr(CMBS_TONE_CONFIRM);               /*!< Confirmation tone */
            caseretstr(CMBS_TONE_ANSWER);                /*!< Answering tone */
            caseretstr(CMBS_TONE_OFF_HOOK_WARN);         /*!< Off Hook warning tone */

            caseretstr(CMBS_TONE_HINT);                  /*!< Hint tone */
            caseretstr(CMBS_TONE_OK);                    /*!< OK tone */
            caseretstr(CMBS_TONE_NOK);                   /*!< Not OK tone */
            caseretstr(CMBS_TONE_DTMF_0);
            caseretstr(CMBS_TONE_DTMF_1);
            caseretstr(CMBS_TONE_DTMF_2);
            caseretstr(CMBS_TONE_DTMF_3);
            caseretstr(CMBS_TONE_DTMF_4);
            caseretstr(CMBS_TONE_DTMF_5);
            caseretstr(CMBS_TONE_DTMF_6);
            caseretstr(CMBS_TONE_DTMF_7);
            caseretstr(CMBS_TONE_DTMF_8);
            caseretstr(CMBS_TONE_DTMF_9);
            caseretstr(CMBS_TONE_DTMF_STAR);
            caseretstr(CMBS_TONE_DTMF_HASH);
            caseretstr(CMBS_TONE_DTMF_A);
            caseretstr(CMBS_TONE_DTMF_B);
            caseretstr(CMBS_TONE_DTMF_C);
            caseretstr(CMBS_TONE_DTMF_D);
            //CMBS_TONE_USER_DEF               /*!< User defined tone, not supported, yet. */
        default:
            break;
    }

    return "Tone not defined";
}

//  ========== cmbs_dbg_GetCallProgressName ===========
/*!
 \brief           return string of enumeration
 \param[in]   e_Param  value of enumeration
 \return    <const char *>   return string of enumeration
*/
const char * cmbs_dbg_GetCallProgressName(E_CMBS_CALL_PROGRESS e_Prog)
{
    switch (e_Prog)
    {
            caseretstr(CMBS_CALL_PROGR_UNDEF);
            caseretstr(CMBS_CALL_PROGR_SETUP_ACK);
            caseretstr(CMBS_CALL_PROGR_PROCEEDING);
            caseretstr(CMBS_CALL_PROGR_RINGING);
            caseretstr(CMBS_CALL_PROGR_BUSY);
            caseretstr(CMBS_CALL_PROGR_CALLWAITING);
            caseretstr(CMBS_CALL_PROGR_INBAND);
            caseretstr(CMBS_CALL_PROGR_DISCONNECTING);
            caseretstr(CMBS_CALL_PROGR_IDLE);
            caseretstr(CMBS_CALL_PROGR_HOLD);
            caseretstr(CMBS_CALL_PROGR_CONNECT);

        default:
            break;
    }

    return "Call Progress not defined";
}
//  ========== getstr_CMBS_CALL_TYPE ===========
/*!
 \brief           return string of enumeration
 \param[in]   e_CallType  value of enumeration
 \return    <const char *>   return string of enumeration
*/
const char * cmbs_dbg_GetCallTypeName(E_CMBS_CALL_STATE_TYPE    e_CallType)
{
    switch (e_CallType)
    {
            caseretstr(CMBS_CALL_STATE_TYPE_INTERNAL);
            caseretstr(CMBS_CALL_STATE_TYPE_EXT_INCOMING);
            caseretstr(CMBS_CALL_STATE_TYPE_EXT_OUTGOING);
            caseretstr(CMBS_CALL_STATE_TYPE_TRANSFER);
            caseretstr(CMBS_CALL_STATE_TYPE_CONFERENCE);
            caseretstr(CMBS_CALL_STATE_TYPE_SERVICE);
            caseretstr(CMBS_CALL_STATE_TYPE_HS_LOCATOR);
        default:
            break;
    }
    return "UNDEFINED";
}

//  ========== cmbs_dbg_GetCallStateName ===========
/*!
 \brief           return string of enumeration
 \param[in]   e_CallType  value of enumeration
 \return    <const char *>   return string of enumeration
*/
const char * cmbs_dbg_GetCallStateName(E_CMBS_CALL_STATE_STATUS  e_CallStatus)
{
    switch (e_CallStatus)
    {
            caseretstr(CMBS_CALL_STATE_STATUS_IDLE);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_SETUP);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_SETUP_ACK);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_PROCEEDING);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_ALERTING);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_CONNECTED);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_DISCONNECTING);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_HOLD);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_UNDER_TRANSFER);
            caseretstr(CMBS_CALL_STATE_STATUS_CONF_CONNECTED);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_INTERCEPTED);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_WAITING);
            caseretstr(CMBS_CALL_STATE_STATUS_CALL_REINJECTED);
            caseretstr(CMBS_CALL_STATE_STATUS_IDLE_PENDING);
            caseretstr(CMBS_CALL_STATE_STATUS_CONF_SECONDARY);
        default:
            break;
    }
    return "UNDEFINED";
}


//  ========== cmbs_dbg_String2E_CMBS_PARAM ===========
/*!
 \brief          return enumeration value of string
 \param[in]  psz_Value         pointer enumeration string
 \return   <E_CMBS_PARAM>    enumeration value
*/
E_CMBS_PARAM cmbs_dbg_String2E_CMBS_PARAM(const char * psz_Value)
{
    int i = 0;

    for (i = 0; i < CMBS_PARAM_MAX; i++)
        if (!strcmp(psz_Value, cmbs_dbg_GetParamName(i)))
            return i;

    return CMBS_PARAM_UNKNOWN;
}

//  ========== cmbs_dbg_String2E_CMBS_TONE ===========
/*!
 \brief          return enumeration value of string
 \param[in]  psz_Value        pointer enumeration string
 \return   <E_CMBS_TONE>    enumeration value
*/
E_CMBS_TONE cmbs_dbg_String2E_CMBS_TONE(const char * psz_Value)
{
    int i = 0;
    for (i = 0; i < CMBS_TONE_USER_DEF; i++)
        if (!strcmp(psz_Value, cmbs_dbg_GetToneName(i)))
            return i;
    return CMBS_TONE_MAX;
}

//  ========== cmbs_dbg_String2E_CMBS_CALL_PROGR ===========
/*!
 \brief          return enumeration value of string
 \param[in]  psz_Value    pointer enumeration string
 \return   <E_CMBS_CALL_PROGRESS> enumeration value
*/
E_CMBS_CALL_PROGRESS cmbs_dbg_String2E_CMBS_CALL_PROGR(const char * psz_Value)
{
    int i = 0;

    for (i = 0; i < CMBS_CALL_PROGR_MAX; i++)
        if (!strcmp(psz_Value, cmbs_dbg_GetCallProgressName(i)))
            return i;
    return CMBS_CALL_PROGR_UNDEF;
}

//  ========== cmbs_dbg_DumpIEList  ===========
/*!
 \brief     dumps IE list
 \param[in,out]   pu8_Buffer   pointer to message buffer
 \param[in,out]   u16_Size   size of message buffer
 \return     < none >
*/
#if !defined( CMBS_API_TARGET )
static char Dump_IE_List_buffer[CMBS_MAX_IE_PRINT_SIZE] = {0};
void    cmbs_dbg_DumpIEList(u8 * pu8_Buffer, u16 u16_Size)
{
    void*   pv_IE;
    u16    u16_IE;
    ST_CFR_IE_LIST  pv_List;

    if (!u16_Size || !pfn_ParseIEFunction)
        return;

    memset(&pv_List, 0, sizeof(ST_CFR_IE_LIST));
    pv_List.u16_CurSize = u16_Size;
    pv_List.pu8_Buffer = pu8_Buffer;

    cmbs_api_ie_GetFirst(&pv_List, &pv_IE, &u16_IE);
    while (pv_IE != NULL)
    {

        u16 length = 0;

        length = pfn_ParseIEFunction(Dump_IE_List_buffer, sizeof(Dump_IE_List_buffer), pv_IE, u16_IE);
        if (length >= CMBS_MAX_IE_PRINT_SIZE)
        {
            Dump_IE_List_buffer[CMBS_MAX_IE_PRINT_SIZE - 1] = 0;
            Dump_IE_List_buffer[CMBS_MAX_IE_PRINT_SIZE - 2] = '.';
            Dump_IE_List_buffer[CMBS_MAX_IE_PRINT_SIZE - 3] = '.';
            Dump_IE_List_buffer[CMBS_MAX_IE_PRINT_SIZE - 4] = '.';
        }

        CFR_DBG_OUT("%s", Dump_IE_List_buffer);
        ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "%s",Dump_IE_List_buffer);
        cmbs_api_ie_GetNext(&pv_List, &pv_IE, &u16_IE);
    }

    CFR_DBG_OUT("\n");
    ctrl_log_print_ex(LOG_INFO, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "\n");

}
#endif

//  ========== cmbs_dbg_CmdTrace  ===========
/*!
 \brief     trace CMBS internal command
 \param[in,out]   pu8_Buffer   pointer to message buffer
 \param[in,out]   u16_Size   size of message buffer
 \return    < none >
*/
void    cmbs_dbg_CmdTrace(const char* message, ST_CMBS_SER_MSGHDR *pst_Hdr, u8 * pu8_Buffer)
{
    char timestamp_str[200] = {0};
    char summary[1024];
    char parameter[512];
    // data validation
    if (!pst_Hdr)
        return;

    cmbs_dbg_getTimestampString(timestamp_str);

#if defined( CMBS_API_TARGET )
    // trace events only on host
    if ((pst_Hdr->u16_EventID & CMBS_CMD_MASK) != CMBS_CMD_MASK)
        return;
#endif

    // Debug output
    CFR_DBG_OUT("\n%s %s %s [0x%x, %d bytes, #%d]: ",
                timestamp_str,
                message,
#if !defined( CMBS_API_TARGET )
                // trace events only on host
                ((pst_Hdr->u16_EventID & CMBS_CMD_MASK) == CMBS_CMD_MASK) ? cmbs_dbg_GetCommandName(pst_Hdr->u16_EventID & ~CMBS_CMD_MASK) : cmbs_dbg_GetEventName(pst_Hdr->u16_EventID),
#else
                // trace only commands on target
                cmbs_dbg_GetCommandName(pst_Hdr->u16_EventID & ~CMBS_CMD_MASK),
#endif
                (pst_Hdr->u16_EventID & CMBS_CMD_MASK) ? (pst_Hdr->u16_EventID & ~CMBS_CMD_MASK) : pst_Hdr->u16_EventID,
                pst_Hdr->u16_TotalLength,
                pst_Hdr->u16_PacketNr);
    snprintf(summary, sizeof(summary), "\n%s %s %s [0x%x, %d bytes, #%d]: ",
                timestamp_str,
                message,
#if !defined( CMBS_API_TARGET )
                // trace events only on host
                ((pst_Hdr->u16_EventID & CMBS_CMD_MASK) == CMBS_CMD_MASK) ? cmbs_dbg_GetCommandName(pst_Hdr->u16_EventID & ~CMBS_CMD_MASK) : cmbs_dbg_GetEventName(pst_Hdr->u16_EventID),
#else
                // trace only commands on target
                cmbs_dbg_GetCommandName(pst_Hdr->u16_EventID & ~CMBS_CMD_MASK),
#endif
                (pst_Hdr->u16_EventID & CMBS_CMD_MASK) ? (pst_Hdr->u16_EventID & ~CMBS_CMD_MASK) : pst_Hdr->u16_EventID,
                pst_Hdr->u16_TotalLength,
                pst_Hdr->u16_PacketNr);

#if !defined( CMBS_API_TARGET )
    memset(parameter, 0, sizeof(parameter));
    if (pu8_Buffer && pst_Hdr->u16_ParamLength)
    {
        int i = 0;
        char tmp[16];
        for (i = 0; i < pst_Hdr->u16_ParamLength; i++)
        {
            CFR_DBG_OUT("%02x ", pu8_Buffer[i]);
            sprintf(tmp, "%02x ", pu8_Buffer[i]);
            strcat(parameter, tmp);
            if (i == 31)
            {
                CFR_DBG_OUT("more...");
                strcat(parameter, "more...");
                break;
            }
        }

        CFR_DBG_OUT("\n");
        strcat(parameter, "\n");

        if (((pst_Hdr->u16_EventID & CMBS_HAN_MASK) != CMBS_HAN_MASK) && (pst_Hdr->u16_EventID & CMBS_CMD_MASK) != CMBS_CMD_MASK && !cmbs_util_RawPayloadEvent(pst_Hdr->u16_EventID) && pu8_Buffer)
            cmbs_dbg_DumpIEList(pu8_Buffer, pst_Hdr->u16_ParamLength);
    }

#endif
    ctrl_log_print_ex(LOG_DEBUG, __FUNCTION__, __LINE__, DECT_CTRL_NAME_TYPE, "%s%s",summary, parameter);

}

void cmbs_dbg_getTimestampString(char *buffer)
{
#ifdef WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);

    sprintf(buffer, "%0.2d:%0.2d:%0.2d.%0.3d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#endif
#if defined( __linux__ )
    struct timeval tv;
    struct timezone tz;
    struct tm *tm;
    gettimeofday(&tv, &tz);
    tm = localtime(&tv.tv_sec);

    sprintf(buffer, "%.2d:%.2d:%.2d.%.3d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec);
#endif
}


void cmbs_dbg_SetParseIEFunc(pfn_cmbs_dbg_ParseIEFunc pfn_Func)
{
    pfn_ParseIEFunction = pfn_Func;
}
/****** [End Of File]***************************************************************************************/
