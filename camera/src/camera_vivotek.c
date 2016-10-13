/*******************************************************************************
 *   Copyright 2015 WondaLink CO., LTD.
 *   All Rights Reserved. This material can not be duplicated for any
 *   profit-driven enterprise. No portions of this material can be reproduced
 *   in any form without the prior written permission of WondaLink CO., LTD.
 *   Forwarding, transmitting or communicating its contents of this document is
 *   also prohibited.
 *
 *   All titles, proprietaries, trade secrets and copyrights in and related to
 *   information contained in this document are owned by WondaLink CO., LTD.
 *
 *   WondaLink CO., LTD.
 *   23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *   HSIN-CHU, TAIWAN R.O.C.
 *
 ******************************************************************************/
/******************************************************************************
 *    Department:
 *    Project :
 *    Block   :
 *    Creator :
 *    File    :
 *    Abstract:
 *    Date    :
 *    $Id:$
 *
 *    Modification History:
 *    By           Date       Ver.   Modification Description
 *    -----------  ---------- -----  -----------------------------
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "capi.h"
#include "error.h"
#include "hcapi.h"
#include "hc_msg.h"
#include "hc_common.h"
#include "cJSON.h"
#include "camera_util.h"
#include "libxml/xmlmemory.h"
#include "libxml/parser.h"
#include "public.h"

extern char g_camera_version[];
extern int posttimeout;

int vivo_set_camera_name(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    snprintf(strUrl2,
             sizeof(strUrl2),
             "server_i4_name=%s", g_camera_version);
#else
    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i2_name=%s", g_camera_version);
#endif

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (strstr(Responsebuf, "server_i4_name=") != NULL)
    {
#else
    if (strstr(Responsebuf, "event_i2_name=") != NULL)
    {
#endif
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_wlan(char *ip)
{
    char strUrl[1024] = {0};
    char Responsebuf[8192] = {0};
    int length = 0;

    CAPI_RESULT_E result;
    WLAN_BASIC_CFG_S wlan_basic_u_cfg;
    WLAN_SEC_CFG_S wlan_security_u_cfg;

    memset(&wlan_basic_u_cfg, 0, sizeof(wlan_basic_u_cfg));
    memset(&wlan_security_u_cfg, 0, sizeof(wlan_security_u_cfg));

    if ((result = capi_get_wlan_basic(&wlan_basic_u_cfg)) == CAPI_FAILURE)
    {
        DEBUG_ERROR("ERROR: capi_get_wlan_basic_upstream, result: %d\n", result);
        return -1;
    }

    if ((result = capi_get_wlan_security(&wlan_security_u_cfg)) == CAPI_FAILURE)
    {
        DEBUG_ERROR("ERROR: capi_get_wlan_security_upstream, result: %d\n", result);
        return -1;
    }

    if (WIFI_AUTH_WPA2_PERSONAL == wlan_security_u_cfg.wlan_auth_type
            || WIFI_AUTH_WPA_PERSONAL == wlan_security_u_cfg.wlan_auth_type
            || WIFI_AUTH_DISABLED == wlan_security_u_cfg.wlan_auth_type
            || WIFI_AUTH_WEP == wlan_security_u_cfg.wlan_auth_type)
    {
    }
    else
    {
        DEBUG_ERROR("ERROR: Camera unsupport the wlan_auth_type: %d\n", result);
        return -1;
    }


    if (WIFI_AUTH_WPA2_PERSONAL == wlan_security_u_cfg.wlan_auth_type)
    {
        snprintf(strUrl, sizeof(strUrl),
            "http://%s/cgi-bin/admin/setparam.cgi?"
            //"network_restart=1&"
            //"network_ipaddress=192.168.33.33&"
            //"network_http_port=80&"
            "wireless_ssid=%s&"
            //"wireless_wlmode=Infra&"
            "wireless_encrypt=%d&"
            //"wireless_authmode=OPEN&"
            //"wireless_keylength=64&"
            //"wireless_keyformat=ASCII&"
            //"wireless_keyselect=1&"
            //"wireless_key1=&"
            //"wireless_key2=&"
            //"wireless_key3=&"
            //"wireless_key4=&"
            "wireless_algorithm=%s&"
            "wireless_presharedkey=%s",
            ip,
            wlan_basic_u_cfg.wlan_essid,
            3,
            (wlan_security_u_cfg.wlan_encry_type == WPA_TKIP) ? "TKIP" : "AES",
            wlan_security_u_cfg.wlan_passphrase);

    }
    else if (WIFI_AUTH_WPA_PERSONAL == wlan_security_u_cfg.wlan_auth_type)
    {
        snprintf(strUrl, sizeof(strUrl),
            "http://%s/cgi-bin/admin/setparam.cgi?"
            "wireless_ssid=%s&"
            "wireless_encrypt=%d&"
            "wireless_algorithm=%s&"
            "wireless_presharedkey=%s"
            ,
            ip,
            wlan_basic_u_cfg.wlan_essid,
            2,
            (wlan_security_u_cfg.wlan_encry_type == WPA_TKIP) ? "TKIP" : "AES",
            wlan_security_u_cfg.wlan_passphrase);

    }
    else if (WIFI_AUTH_DISABLED == wlan_security_u_cfg.wlan_auth_type)
    {
        snprintf(strUrl, sizeof(strUrl),
            "http://%s/cgi-bin/admin/setparam.cgi?"
            "wireless_ssid=%s&"
            "wireless_encrypt=%d&"
            , 
            ip,
            wlan_basic_u_cfg.wlan_essid,
            0);

    }
    else if (WIFI_AUTH_WEP == wlan_security_u_cfg.wlan_auth_type)
    {
        snprintf(strUrl, sizeof(strUrl),
            "http://%s/cgi-bin/admin/setparam.cgi?"
            "wireless_ssid=%s&"
            "wireless_encrypt=%d&"
            "wireless_keyformat=%s&"
            "wireless_keyselect=%d&"
            "wireless_key1=%s&"
            "wireless_key2=%s&"
            "wireless_key3=%s&"
            "wireless_key4=%s&"
            , 
            ip,
            wlan_basic_u_cfg.wlan_essid,
            1,
            "ASCII",
            wlan_security_u_cfg.wlan_key_used,
            wlan_security_u_cfg.wlan_key[0],
            wlan_security_u_cfg.wlan_key[1],
            wlan_security_u_cfg.wlan_key[2],
            wlan_security_u_cfg.wlan_key[3]);

    }

    length = strlen(strUrl);
    if (length > 0 && strUrl[length] == '&')
        strUrl[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    int res = send_get(strUrl, Responsebuf);
    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf,  "wireless_ssid=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }

    return 0;
}

int vivo_set_media_video(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);
    /* timeshift_enable=0&timeshift_c0_s0_allow=0&timeshift_c0_s1_allow=1&media_i0_videoclip_source=0&media_i1_videoclip_source=0&media_i2_videoclip_source=0&media_i3_videoclip_source=0&media_i4_videoclip_source=0&timeshift_Stream_num=1&videoin_c0_s0_codectype=h264&stream1_h264_resolution=640x400&videoin_c0_s0_h264_maxframe=3&videoin_c0_s0_h264_intraperiod=1000&videoin_c0_s0_h264_bitrate=512000&videoin_c0_s0_h264_ratecontrolmode=vbr&videoin_c0_s0_h264_qvalue=26&videoin_c0_s0_h264_quant=3&stream1_mjpeg_resolution=640x400&videoin_c0_s0_mjpeg_maxframe=30&videoin_c0_s0_mjpeg_qvalue=50&videoin_c0_s0_mjpeg_quant=3&videoin_c0_s1_codectype=h264&stream2_h264_resolution=640x400&videoin_c0_s1_h264_maxframe=10&videoin_c0_s1_h264_intraperiod=1000&videoin_c0_s1_h264_bitrate=128000&videoin_c0_s1_h264_ratecontrolmode=vbr&videoin_c0_s1_h264_qvalue=26&videoin_c0_s1_h264_quant=3&stream2_mjpeg_resolution=176x144&videoin_c0_s1_mjpeg_maxframe=30&videoin_c0_s1_mjpeg_qvalue=50&videoin_c0_s1_mjpeg_quant=3&videoin_c0_s0_resolution=640x400&videoin_c0_s1_resolution=640x400 */

    snprintf(strUrl2, sizeof(strUrl2),
             "videoin_c0_s0_codectype=h264&"
             "stream1_h264_resolution=640x400&"
             "videoin_c0_s0_h264_maxframe=3&"
             "videoin_c0_s0_h264_intraperiod=1000&"
             "videoin_c0_s0_h264_bitrate=512000&"
             "videoin_c0_s0_h264_ratecontrolmode=vbr&"
             "videoin_c0_s0_h264_qvalue=26&"
             "videoin_c0_s0_h264_quant=3&"
             "videoin_c0_s1_codectype=h264&"
             "stream2_h264_resolution=640x400&"
             "videoin_c0_s1_h264_maxframe=10&"
             "videoin_c0_s1_h264_intraperiod=1000&"
             "videoin_c0_s1_h264_bitrate=128000&"
             "videoin_c0_s1_h264_ratecontrolmode=vbr&"
             "videoin_c0_s1_h264_qvalue=26&"
             "videoin_c0_s1_h264_quant=3&"
             "videoin_c0_s0_resolution=640x400&"
             "videoin_c0_s1_resolution=640x400"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "videoin_c0_s0_codectype=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_network_stream(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* network_preprocess=0&network_http_authmode=digest&network_http_port=80&network_http_alternateport=8092&network_http_s0_accessname=video.mjpg&network_http_s1_accessname=video2.mjpg&network_rtsp_authmode=digest&network_rtsp_s0_accessname=live.3gpp&network_rtsp_s1_accessname=live2.3gpp&network_rtsp_port=554&network_rtp_videoport=5556&network_rtp_audioport=5558&network_rtsp_s0_multicast_alwaysmulticast=0&network_rtsp_s0_multicast_ipaddress=239.128.1.99&network_rtsp_s0_multicast_videoport=5560&network_rtsp_s0_multicast_audioport=5562&network_rtsp_s0_multicast_ttl=15&network_rtsp_s1_multicast_alwaysmulticast=0&network_rtsp_s1_multicast_ipaddress=239.128.1.100&network_rtsp_s1_multicast_videoport=5564&network_rtsp_s1_multicast_audioport=5566&network_rtsp_s1_multicast_ttl=15 */
    snprintf(strUrl2,
             sizeof(strUrl2),
             "network_sip_port=5072&"
             "network_rtsp_authmode=digest&"
             "network_rtsp_s0_accessname=live.3gpp&"
             "network_rtsp_s1_accessname=live2.3gpp&"
             "network_rtsp_port=554&"
             "network_rtp_videoport=5556&"
             "network_rtp_audioport=5558&"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "network_rtsp_authmode=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_sd_card(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* disk_i0_cyclic_enabled=1&disk_i0_autocleanup_enabled=1&disk_i0_autocleanup_maxage=15 */
    snprintf(strUrl2, sizeof(strUrl2),
             "disk_i0_cyclic_enabled=1&disk_i0_autocleanup_enabled=1&disk_i0_autocleanup_maxage=15");

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "disk_i0_cyclic_enabled=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_root_password(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* disk_i0_cyclic_enabled=1&disk_i0_autocleanup_enabled=1&disk_i0_autocleanup_maxage=15 */
    snprintf(strUrl2, sizeof(strUrl2),
             "security_user_i0_pass=&confirm=");

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "security_user_i0_pass=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
#ifdef SUPPORT_UPLOADCLIP_AUTO

int vivo_set_event_clips_long(char *ip, char *serialnumber)
#else
int vivo_set_event_clips_long(char *ip)
#endif
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* media_i1_name=clips_long&media_i1_snapshot_source=0&media_i1_snapshot_preevent=1&media_i1_snapshot_postevent=1&media_i1_snapshot_prefix=&media_i1_snapshot_datesuffix=0&media_i1_type=videoclip&media_i1_videoclip_source=1&media_i1_videoclip_preevent=5&media_i1_videoclip_maxduration=15&media_i1_videoclip_maxsize=3072&media_i1_videoclip_prefix= */
 #ifdef SUPPORT_UPLOADCLIP_AUTO
    snprintf(strUrl2,
             sizeof(strUrl2),
             "media_i0_name=clips_long&"
             "media_i0_snapshot_source=0&"
             "media_i0_snapshot_preevent=1&"
             "media_i0_snapshot_postevent=1&"
             "media_i0_snapshot_prefix=&"
             "media_i0_snapshot_datesuffix=0&"
             "media_i0_type=videoclip&"
             "media_i0_videoclip_source=1&"
             "media_i0_videoclip_preevent=5&"
             "media_i0_videoclip_maxduration=15&"
             "media_i0_videoclip_maxsize=3072&"
             "media_i0_videoclip_prefix=%s-",
             serialnumber
             );
#else
   snprintf(strUrl2,
             sizeof(strUrl2),
             "media_i0_name=clips_long&"
             "media_i0_snapshot_source=0&"
             "media_i0_snapshot_preevent=1&"
             "media_i0_snapshot_postevent=1&"
             "media_i0_snapshot_prefix=&"
             "media_i0_snapshot_datesuffix=0&"
             "media_i0_type=videoclip&"
             "media_i0_videoclip_source=1&"
             "media_i0_videoclip_preevent=5&"
             "media_i0_videoclip_maxduration=15&"
             "media_i0_videoclip_maxsize=3072&"
             "media_i0_videoclip_prefix="
             );
#endif
    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "media_i0_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
#ifdef SUPPORT_UPLOADCLIP_AUTO
int vivo_set_event_clips_short(char *ip, char *serialnumber)
#else
int vivo_set_event_clips_short(char *ip)
#endif
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* media_i0_name=clips_short&media_i0_snapshot_source=0&media_i0_snapshot_preevent=1&media_i0_snapshot_postevent=1&media_i0_snapshot_prefix=&media_i0_snapshot_datesuffix=0&media_i0_type=videoclip&media_i0_videoclip_source=1&media_i0_videoclip_preevent=2&media_i0_videoclip_maxduration=10&media_i0_videoclip_maxsize=3072&media_i0_videoclip_prefix= */
 #ifdef SUPPORT_UPLOADCLIP_AUTO

    snprintf(strUrl2,
             sizeof(strUrl2),
             "media_i1_name=clips_short&"
             "media_i1_type=videoclip&"
             "media_i1_snapshot_source=0&"
             "media_i1_snapshot_prefix=&"
             "media_i1_snapshot_datesuffix=0&"
             "media_i1_snapshot_preevent=1&"
             "media_i1_snapshot_postevent=1&"
             "media_i1_videoclip_source=1&"
             "media_i1_videoclip_prefix=%s-&"
             "media_i1_videoclip_preevent=2&"
             "media_i1_videoclip_maxduration=10&"
             "media_i1_videoclip_maxsize=3072",
             serialnumber
             );
 #else
     snprintf(strUrl2,
             sizeof(strUrl2),
             "media_i1_name=clips_short&"
             "media_i1_snapshot_source=0&"
             "media_i1_snapshot_preevent=1&"
             "media_i1_snapshot_postevent=1&"
             "media_i1_snapshot_prefix=&"
             "media_i1_snapshot_datesuffix=0&"
             "media_i1_type=videoclip&"
             "media_i1_videoclip_source=1&"
             "media_i1_videoclip_preevent=2&"
             "media_i1_videoclip_maxduration=10&"
             "media_i1_videoclip_maxsize=3072&"
             "media_i1_videoclip_prefix=");

 #endif
    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "media_i1_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_server_i0_ftp(char *ip, char *serIP)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* event_i0_name=motion_detect&event_i0_enable=1&=on&event_i0_priority=1&event_i0_delay=30&event_i0_weekday=127&day=on&day=on&day=on&day=on&day=on&day=on&day=on&method=on&event_i0_begintime=00%3A00&event_i0_endtime=24%3A00&event_i0_trigger=motion&event_i0_mdwin=1&win=on&event_i0_mdwin0=1&win_p=on&event_i0_inter=1&event_i0_di=1&event_i0_triggerstatus=trigger&event_i0_exttriggerstatus=&triggerstatus=on&event_i0_vi=0&event_i0_action_cf_backup=0&event_i0_action_goto_enable=0&event_i0_action_cf_enable=1&=on&event_i0_action_cf_media=2&event_i0_action_server_i0_enable=0&event_i0_action_server_i0_media=&event_i0_action_server_i0_datefolder=0&event_i0_action_server_i1_enable=0&event_i0_action_server_i1_media=&event_i0_action_server_i1_datefolder=0&event_i0_action_server_i2_enable=0&event_i0_action_server_i2_media=&event_i0_action_server_i2_datefolder=0&event_i0_action_server_i3_enable=0&event_i0_action_server_i3_media=&event_i0_action_server_i3_datefolder=0&event_i0_action_server_i4_enable=0&event_i0_action_server_i4_media=&event_i0_action_server_i4_datefolder=0 */
    snprintf(strUrl2,
             sizeof(strUrl2),
             "server_i0_name=VSFTPD&"
             "server_i0_type=ftp&"
             "server_i0_ftp_address=%s&"
             "server_i0_ftp_username=camera&"
             "server_i0_ftp_passwd=camera2016&"
             "server_i0_ftp_port=21800",
             serIP
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';
    
    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);
    
    if (strstr(Responsebuf, "server_i0_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_event_clips_pir(char *ip, char *serialnumber)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* media_i1_name=clips_long&media_i1_snapshot_source=0&media_i1_snapshot_preevent=1&media_i1_snapshot_postevent=1&media_i1_snapshot_prefix=&media_i1_snapshot_datesuffix=0&media_i1_type=videoclip&media_i1_videoclip_source=1&media_i1_videoclip_preevent=5&media_i1_videoclip_maxduration=15&media_i1_videoclip_maxsize=3072&media_i1_videoclip_prefix= */
    snprintf(strUrl2,
             sizeof(strUrl2),
             "media_i2_name=clips_pir&"
             "media_i2_type=videoclip&"
             "media_i2_snapshot_source=0&"
             "media_i2_snapshot_prefix=&"
             "media_i2_snapshot_datesuffix=0&"
             "media_i2_snapshot_preevent=1&"
             "media_i2_snapshot_postevent=1&"
             "media_i2_videoclip_source=1&"
             "media_i2_videoclip_prefix=%s-PR-&"
             "media_i2_videoclip_preevent=5&"
             "media_i2_videoclip_maxduration=15&"
             "media_i2_videoclip_maxsize=3072",
             serialnumber
             );
    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "media_i2_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_event_motion_detect(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* event_i0_name=motion_detect&event_i0_enable=1&=on&event_i0_priority=1&event_i0_delay=30&event_i0_weekday=127&day=on&day=on&day=on&day=on&day=on&day=on&day=on&method=on&event_i0_begintime=00%3A00&event_i0_endtime=24%3A00&event_i0_trigger=motion&event_i0_mdwin=1&win=on&event_i0_mdwin0=1&win_p=on&event_i0_inter=1&event_i0_di=1&event_i0_triggerstatus=trigger&event_i0_exttriggerstatus=&triggerstatus=on&event_i0_vi=0&event_i0_action_cf_backup=0&event_i0_action_goto_enable=0&event_i0_action_cf_enable=1&=on&event_i0_action_cf_media=2&event_i0_action_server_i0_enable=0&event_i0_action_server_i0_media=&event_i0_action_server_i0_datefolder=0&event_i0_action_server_i1_enable=0&event_i0_action_server_i1_media=&event_i0_action_server_i1_datefolder=0&event_i0_action_server_i2_enable=0&event_i0_action_server_i2_media=&event_i0_action_server_i2_datefolder=0&event_i0_action_server_i3_enable=0&event_i0_action_server_i3_media=&event_i0_action_server_i3_datefolder=0&event_i0_action_server_i4_enable=0&event_i0_action_server_i4_media=&event_i0_action_server_i4_datefolder=0 */
    #ifdef SUPPORT_UPLOADCLIP_AUTO
    snprintf(strUrl2,
             sizeof(strUrl2),
             "motion_c0_profile_i0_policy=schedule&"
             "motion_c0_profile_i0_begintime=00:00&"
             "motion_c0_profile_i0_endtime=24:00&"
             "event_i0_name=motion_detect&"
             "event_i0_priority=1&"
             "event_i0_delay=30&"
             "event_i0_trigger=motion&"
             "event_i0_triggerstatus=trigger&"
             "event_i0_exttriggerstatus=&"
             "event_i0_di=1&"
             "event_i0_mdwin=1&"
             "event_i0_vi=0&"
             "event_i0_mdwin0=1&"
             "event_i0_inter=1&"
             "event_i0_weekday=127&"
             "event_i0_begintime=00:00&"
             "event_i0_endtime=24:00&"
             "event_i0_lowlightcondition=1&"
             "event_i0_action_goto_enable=0&"
             "event_i0_action_goto_name=&"
             "event_i0_action_cf_enable=0&"
             "event_i0_action_cf_folder=&"
             "event_i0_action_cf_media=1&"
             "event_i0_action_cf_datefolder=1&"
             "event_i0_action_cf_backup=0&"
             "event_i0_action_server_i0_enable=1&"
             "event_i0_action_server_i0_media=0"
             );
    #else
    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i0_name=motion_detect&"
             "event_i0_priority=1&"
             "event_i0_delay=30&"
             "event_i0_weekday=127&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "method=on&"
             "event_i0_trigger=motion&"
             "event_i0_mdwin=1&"
             "win=on&"
             "event_i0_mdwin0=1&"
             "win_p=on&"
             "event_i0_inter=1&"
             "event_i0_di=1&"
             "event_i0_triggerstatus=trigger&"
             "event_i0_exttriggerstatus=&"
             "triggerstatus=on&"
             "event_i0_vi=0&"
             "event_i0_action_cf_backup=0&"
             "event_i0_action_goto_enable=0&"
             "event_i0_action_cf_enable=1&"
             "event_i0_action_cf_media=1&"
             );
    #endif

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "event_i0_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_event_virtual_input(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* event_i1_name=virtual_input&event_i1_enable=1&=on&event_i1_priority=2&event_i1_delay=10&event_i1_weekday=127&day=on&day=on&day=on&day=on&day=on&day=on&day=on&method=on&event_i1_begintime=00%3A00&event_i1_endtime=24%3A00&event_i1_mdwin=0&event_i1_mdwin0=0&event_i1_inter=1&event_i1_di=1&event_i1_triggerstatus=trigger&event_i1_exttriggerstatus=&triggerstatus=on&event_i1_trigger=vi&event_i1_vi=1&vin=on&event_i1_action_cf_backup=0&event_i1_action_goto_enable=0&event_i1_action_cf_enable=1&=on&event_i1_action_cf_media=0&event_i1_action_server_i0_enable=0&event_i1_action_server_i0_media=&event_i1_action_server_i0_datefolder=0&event_i1_action_server_i1_enable=0&event_i1_action_server_i1_media=&event_i1_action_server_i1_datefolder=0&event_i1_action_server_i2_enable=0&event_i1_action_server_i2_media=&event_i1_action_server_i2_datefolder=0&event_i1_action_server_i3_enable=0&event_i1_action_server_i3_media=&event_i1_action_server_i3_datefolder=0&event_i1_action_server_i4_enable=0&event_i1_action_server_i4_media=&event_i1_action_server_i4_datefolder=0 */
  #ifdef SUPPORT_UPLOADCLIP_AUTO
    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i1_name=virtual_input&"
             "event_i1_priority=2&"
             "event_i1_delay=10&"
             "event_i1_weekday=127&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "method=on&"
             "event_i1_mdwin=0&"
             "event_i1_mdwin0=0&"
             "event_i1_inter=1&"
             "event_i1_di=1&"
             "event_i1_triggerstatus=trigger&"
             "event_i1_exttriggerstatus=&"
             "triggerstatus=on&"
             "event_i1_trigger=vi&"
             "event_i1_vi=1&"
             "vin=on&"
             "event_i1_action_cf_backup=0&"
             "event_i1_action_goto_enable=0&"
             "event_i1_action_cf_enable=0&"
             "event_i1_action_cf_media=0&"
             "event_i1_action_server_i0_enable=1&"
             "event_i1_action_server_i0_media=0&"
             );
#else
    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i1_name=virtual_input&"
             "event_i1_priority=2&"
             "event_i1_delay=10&"
             "event_i1_weekday=127&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "day=on&"
             "method=on&"
             "event_i1_mdwin=0&"
             "event_i1_mdwin0=0&"
             "event_i1_inter=1&"
             "event_i1_di=1&"
             "event_i1_triggerstatus=trigger&"
             "event_i1_exttriggerstatus=&"
             "triggerstatus=on&"
             "event_i1_trigger=vi&"
             "event_i1_vi=1&"
             "vin=on&"
             "event_i1_action_cf_backup=0&"
             "event_i1_action_goto_enable=0&"
             "event_i1_action_cf_enable=1&"
             "event_i1_action_cf_media=0&"
             //"event_i1_action_server_i0_enable=0&"
             //"event_i1_action_server_i0_media=&"
             //"event_i1_action_server_i0_datefolder=0&"
             //"event_i1_action_server_i1_enable=0&"
             //"event_i1_action_server_i1_media=&"
             //"event_i1_action_server_i1_datefolder=0&"
             //"event_i1_action_server_i2_enable=0&"
             //"event_i1_action_server_i2_media=&"
             //"event_i1_action_server_i2_datefolder=0&"
             //"event_i1_action_server_i3_enable=0&"
             //"event_i1_action_server_i3_media=&"
             //"event_i1_action_server_i3_datefolder=0&"
             //"event_i1_action_server_i4_enable=0&"
             //"event_i1_action_server_i4_media=&"
             //"event_i1_action_server_i4_datefolder=0"
             );
#endif
    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "event_i1_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_enable_pir(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    snprintf(strUrl2,
             sizeof(strUrl2),
             "pir_enable=1"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "pir_enable=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_event_pri_trigger(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i2_name=pri_trigger&"
             "event_i2_priority=1&"
             "event_i2_delay=10&"
             "event_i2_trigger=pir&"
             "event_i2_triggerstatus=trigger&"
             "event_i2_exttriggerstatus=&"
             "event_i2_exttriggerstatus1=&"
             "event_i2_di=0&"
             "event_i2_mdwin=0&"
             "event_i2_vadp=0&"
             "event_i2_vi=0&"
             "event_i2_mdwin0=0&"
             "event_i2_valevel=0&"
             "event_i2_valevel0=0&"
             "event_i2_inter=1&"
             "event_i2_weekday=127&"
             "event_i2_begintime=00:00&"
             "event_i2_endtime=24:00&"
             "event_i2_action_goto_enable=0&"
             "event_i2_action_cf_enable=0&"
             "event_i2_action_cf_media=2&"
             "event_i2_action_cf_backup=0&"
             "event_i2_action_server_i0_enable=1&"
             "event_i2_action_server_i0_media=2&"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "event_i2_name=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_enable_event(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    snprintf(strUrl2,
             sizeof(strUrl2),
             "event_i0_enable=1&"
             "event_i1_enable=1&"
             "event_i2_enable=1"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "event_i0_enable=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_app_motion_detect(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* motion_c0_win_i0_enable=1&motion_c0_win_i0_name=MD&motion_c0_win_i0_left=24&motion_c0_win_i0_top=27&motion_c0_win_i0_width=260&motion_c0_win_i0_height=189&motion_c0_win_i0_objsize=0&motion_c0_win_i0_sensitivity=90&motion_c0_win_i1_enable=0&motion_c0_win_i1_name=&motion_c0_win_i1_left=0&motion_c0_win_i1_top=0&motion_c0_win_i1_width=0&motion_c0_win_i1_height=0&motion_c0_win_i1_objsize=0&motion_c0_win_i1_sensitivity=0&motion_c0_win_i2_enable=0&motion_c0_win_i2_name=&motion_c0_win_i2_left=0&motion_c0_win_i2_top=0&motion_c0_win_i2_width=0&motion_c0_win_i2_height=0&motion_c0_win_i2_objsize=0&motion_c0_win_i2_sensitivity=0 */
    snprintf(strUrl2,
             sizeof(strUrl2),
             "motion_c0_enable=1&"
             "motion_c0_win_i0_enable=1&"
             "motion_c0_win_i0_name=MD&"
             "motion_c0_win_i0_left=24&"
             "motion_c0_win_i0_top=27&"
             "motion_c0_win_i0_width=260&"
             "motion_c0_win_i0_height=189&"
             "motion_c0_win_i0_objsize=0&"
             "motion_c0_win_i0_sensitivity=90&"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "motion_c0_win_i0_enable=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_app_motion_profile(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* motion_c0_profile_i0_win_i0_enable=1&motion_c0_profile_i0_win_i0_name=MD-Night&motion_c0_profile_i0_win_i0_left=29&motion_c0_profile_i0_win_i0_top=33&motion_c0_profile_i0_win_i0_width=269&motion_c0_profile_i0_win_i0_height=186&motion_c0_profile_i0_win_i0_objsize=5&motion_c0_profile_i0_win_i0_sensitivity=80&motion_c0_profile_i0_win_i1_enable=0&motion_c0_profile_i0_win_i1_name=&motion_c0_profile_i0_win_i1_left=0&motion_c0_profile_i0_win_i1_top=0&motion_c0_profile_i0_win_i1_width=0&motion_c0_profile_i0_win_i1_height=0&motion_c0_profile_i0_win_i1_objsize=0&motion_c0_profile_i0_win_i1_sensitivity=0&motion_c0_profile_i0_win_i2_enable=0&motion_c0_profile_i0_win_i2_name=&motion_c0_profile_i0_win_i2_left=0&motion_c0_profile_i0_win_i2_top=0&motion_c0_profile_i0_win_i2_width=0&motion_c0_profile_i0_win_i2_height=0&motion_c0_profile_i0_win_i2_objsize=0&motion_c0_profile_i0_win_i2_sensitivity=0 */
    snprintf(strUrl2, sizeof(strUrl2),
             "motion_c0_profile_i0_win_i0_enable=1&"
             "motion_c0_profile_i0_win_i0_name=MD-Night&"
             "motion_c0_profile_i0_win_i0_left=29&"
             "motion_c0_profile_i0_win_i0_top=33&"
             "motion_c0_profile_i0_win_i0_width=269&"
             "motion_c0_profile_i0_win_i0_height=186&"
             "motion_c0_profile_i0_win_i0_objsize=5&"
             "motion_c0_profile_i0_win_i0_sensitivity=80&"
             );

    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "motion_c0_profile_i0_win_i0_enable=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vivo_set_app_motion_profile_general(char *ip)
{
    char strUrl[256] = {0};
    char strUrl2[8192] = {0};
    char Responsebuf[8192] = {0};
    FILE *fp = NULL;
    int res = -1;
    int length = 0;
	int res_code = 0;

    memset(Responsebuf, 0, sizeof(Responsebuf));
    memset(strUrl, 0, sizeof(strUrl));
    memset(strUrl2, 0, sizeof(strUrl2));

    snprintf(strUrl,
             sizeof(strUrl),
             "http://%s/cgi-bin/admin/setparam.cgi",
             ip);

    /* motion_c0_profile_i0_enable=1&enableprofile=on&motion_c0_profile_i0_policy=night&motion_c0_profile_i0_begintime=18%3A00&motion_c0_profile_i0_endtime=06%3A00 */
    snprintf(strUrl2, sizeof(strUrl2),
             "motion_c0_profile_i0_enable=1&"
             "enableprofile=on&"
             "motion_c0_profile_i0_policy=night");


    length = strlen(strUrl2);
    if (--length > 0 && strUrl2[length] == '&')
        strUrl2[length] = '\0';

    posttimeout = 10;
    DEBUG_INFO("strUrl: %s\n", strUrl);
    DEBUG_INFO("strUrl2: %s\n", strUrl2);
    res = send_post(strUrl, strUrl2, Responsebuf, &res_code);
	if(res != 0 || res_code != 200)
		DEBUG_ERROR("send_post res = %d, camera response code = %d\n", res, res_code);

    DEBUG_INFO("Response: [%s]\n", Responsebuf);

    if (strstr(Responsebuf, "motion_c0_profile_i0_enable=") != NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
int vivo_IP8131W_init(char *ip, char *serIP, char *serialnumber)
#else
int vivo_IP8131W_init(char *ip)
#endif
{
    if (vivo_set_root_password(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_root_password failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_wlan(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_wlan failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_media_video(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_media_video failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_network_stream(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_network_stream failed.\n");
        return -1;
    }
    sleep(1);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_server_i0_ftp(ip,serIP) != 0)
    {
        DEBUG_ERROR("vivo_set_server_i0_ftp failed.\n");
        return -1;
    }
#else
    if (vivo_set_sd_card(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_sd_card failed.\n");
        return -1;
    }
#endif
    sleep(1);
    // App -> motion detection
    if (vivo_set_app_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_detect failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile_general(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile_general failed.\n");
        return -1;
    }
    sleep(1);

    // event

#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_long(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_long(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_long failed.\n");
        return -1;
    }
    sleep(1);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_short(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_short(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_short failed.\n");
        return -1;
    }
    sleep(1);
    
    if (vivo_set_event_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_motion_detect failed.\n");
        return -1;
    }
    sleep(1);
    
    if (vivo_set_event_virtual_input(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_virtual_input failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_enable_event(ip) !=0)
    {
        DEBUG_ERROR("vivo_set_enable_event failed,\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_camera_name(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_camera_name failed.\n");
        return -1;
    }

    DEBUG_INFO("Initialize camera config success.\n");

    return 0;
}

#ifdef SUPPORT_UPLOADCLIP_AUTO
int vivo_IB8168_init(char *ip ,char *serIP, char *serialnumber)
#else
int vivo_IB8168_init(char *ip)
#endif
{
    if (vivo_set_root_password(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_root_password failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_media_video(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_media_video failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_network_stream(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_network_stream failed.\n");
        return -1;
    }
    sleep(1);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_server_i0_ftp(ip,serIP) != 0)
    {
        DEBUG_ERROR("vivo_set_server_i0_ftp failed.\n");
        return -1;
    }
#else
    if (vivo_set_sd_card(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_sd_card failed.\n");
        return -1;
    }
#endif
    sleep(1);
    // App -> motion detection
    if (vivo_set_app_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_detect failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile_general(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile_general failed.\n");
        return -1;
    }
    sleep(1);

    // event
  #ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_long(ip, serialnumber) != 0)
  #else
    if (vivo_set_event_clips_long(ip) != 0)
  #endif
    {
        DEBUG_ERROR("vivo_set_event_clips_long failed.\n");
        return -1;
    }
    sleep(1);

#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_short(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_short(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_short failed.\n");
        return -1;
    }

    sleep(1);
    
    if (vivo_set_event_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_motion_detect failed.\n");
        return -1;
    }
    sleep(1);
    
    if (vivo_set_event_virtual_input(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_virtual_input failed.\n");
        return -1;
    }
    sleep(1);
    
    if (vivo_set_camera_name(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_camera_name failed.\n");
        return -1;
    }

    DEBUG_INFO("Initialize camera config success.\n");

    return 0;
}

//#define TIME_8137 1 //PIR fail
//#define TIME_8137 5 //PIR okay
#define TIME_8137 3 //PIR okay
int vivo_WD8137W_init(char *ip ,char *serIP, char *serialnumber)
{
    if (vivo_set_root_password(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_root_password failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_media_video(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_media_video failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_network_stream(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_network_stream failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_server_i0_ftp(ip, serIP) != 0)
    {
        DEBUG_ERROR("vivo_set_server_i0_ftp failed.\n");
        return -1;
    }
    sleep(1);

    // App -> motion detection
    if (vivo_set_app_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_detect failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile_general(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile_general failed.\n");
        return -1;
    }
    sleep(1);

    // event
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_long(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_long(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_long failed.\n");
        return -1;
    }
    sleep(1);

#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_short(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_short(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_short failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_event_clips_pir(ip, serialnumber) != 0)
    {
        DEBUG_ERROR("vivo_set_event_clips_pir failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_event_motion_detect(ip) !=0)
    {
        DEBUG_ERROR("vivo_set_event_motion_detect failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_event_virtual_input(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_virtual_input failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_enable_pir(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_enable_pir failed.\n");
        return -1;
    }
    sleep(TIME_8137);

    if (vivo_set_event_pri_trigger(ip) !=0)
    {
        DEBUG_ERROR("vivo_set_event_pri_trigger failed,\n");
        return -1;
    }
    sleep(TIME_8137);

    if (vivo_set_enable_event(ip) !=0)
    {
        DEBUG_ERROR("vivo_set_enable_event failed,\n");
        return -1;
    }
    sleep(1);
    
    if (vivo_set_camera_name(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_camera_name failed.\n");
        return -1;
    }

    DEBUG_INFO("Initialize camera config success.\n");

    return 0;
}


#ifdef SUPPORT_UPLOADCLIP_AUTO
int vivo_default_init(char *ip ,char *serIP, char *serialnumber)
#else
int vivo_default_init(char *ip)
#endif
{
    if (vivo_set_root_password(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_root_password failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_media_video(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_media_video failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_network_stream(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_network_stream failed.\n");
        return -1;
    }
    sleep(1);
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_server_i0_ftp(ip,serIP) != 0)
    {
        DEBUG_ERROR("vivo_set_server_i0_ftp failed.\n");
        return -1;
    }
#else
    if (vivo_set_sd_card(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_sd_card failed.\n");
        return -1;
    }
#endif

    sleep(1);
    // App -> motion detection
    if (vivo_set_app_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_detect failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile failed.\n");
        return -1;
    }
    sleep(1);
    if (vivo_set_app_motion_profile_general(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_app_motion_profile_general failed.\n");
        return -1;
    }
    sleep(1);

    // event
#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_long(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_long(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_long failed.\n");
        return -1;
    }
    sleep(1);

#ifdef SUPPORT_UPLOADCLIP_AUTO
    if (vivo_set_event_clips_short(ip, serialnumber) != 0)
#else
    if (vivo_set_event_clips_short(ip) != 0)
#endif
    {
        DEBUG_ERROR("vivo_set_event_clips_short failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_event_motion_detect(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_motion_detect failed.\n");
        return -1;
    }
    sleep(1);

    if (vivo_set_event_virtual_input(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_event_virtual_input failed.\n");
        return -1;
    }
    sleep(1);
    
    if (vivo_set_camera_name(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_camera_name failed.\n");
        return -1;
    }
    /*
     * Compatible with some cameras without wlan, so do not
     * care the wlan settings result, just log error info.
     */
    sleep(1);
    if (vivo_set_wlan(ip) != 0)
    {
        DEBUG_ERROR("vivo_set_wlan failed.\n");
        //return -1;
    }

    DEBUG_INFO("Initialize camera config success.\n");

    return 0;
}

