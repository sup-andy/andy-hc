/**********************************************************************
 *    Copyright 2015 WondaLink CO., LTD.
 *    All Rights Reserved. This material can not be duplicated for any
 *    profit-driven enterprise. No portions of this material can be reproduced
 *    in any form without the prior written permission of WondaLink CO., LTD.
 *    Forwarding, transmitting or communicating its contents of this document is
 *    also prohibited.
 *
 *    All titles, proprietaries, trade secrets and copyrights in and related to
 *    information contained in this document are owned by WondaLink CO., LTD.
 *
 *    WondaLink CO., LTD.
 *    23, R&D ROAD2, SCIENCE-BASED INDUSTRIAL PARK,
 *    HSIN-CHU, TAIWAN R.O.C.
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

#ifndef __IPCAM_HTTPS_H__
#define __IPCAM_HTTPS_H__


#define USE_SSL

#ifdef USE_SSL
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <openssl/ssl2.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#define SSL_DEPTH 5

#endif

/* socket related headers */
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/nameser.h>
#include <resolv.h>



#include "base64.h"
#include "capi_struct.h"


#define HTTP_UPLOAD_SEND_LENGTH (4*1024*1024)  //for not crash when sending big packets, 4MB MAX
#define HTTP_UPLOAD_RECV_LENGTH (10*1024)
#define HTTP_PACKET_HEAD_MAX_SIZE  (32*1024)  // 32k size max

#define CONNECT_TIME_OUT_SECOND 10    /* socket connect timeout */
#define CONNECT_TIME_OUT_USECOND 0    /* socket connect timeout */

/*
 * Define http header value
 */
#define HTTP_VERSION    "HTTP/1.1"
#define USER_AGENT      "DLC"
#define CONNECTION      "Keep-Alive"

#define BOUNDARY  "-----------------------------7dd923456ccef"

#define PROTOCOL_UNSUPPORT_KEY "501 Method Not Implemented"


#define HTTP_MAX_TRY_TIMES 1


/*
 * Define global constants ......
 */
#define NC_VALUE_LEN       8

#define HTTP_HEAD_LINE_LEN     512
#define MAX_COOKIE_NUM 20

#define COOKIE_NAME_LEN    64
#define COOKIE_VALUE_LEN   128
#define COOKIE_DOMAIN_LEN  64
#define COOKIE_PATH_LEN    32
#define COOKIE_PORT_LEN    32

#define DIGEST_BIG_LENGTH 256
#define DIGEST_MIDDLE_LENGTH 128
#define DIGEST_SMALL_LENGTH 16

#define IP_LENGTH 64
#define HOST_LENGTH 128
#define HTTP_PATH_LENGTH 256
#define PACKET_HEAD_LENGTH 256
#define PACKET_TAIL_LENGTH 512
#define FILE_NAME_LEGNTH 64
#define TIME_STR_LENGTH 32
#define SEND_TIME_OUT 10 /* socket send timeout */ /* 10 seconds timeout*/
#define RECV_TIME_OUT 10 /* socket recv timeout */ /* 10 seconds timeout*/
#define SSL_TIME_OUT 10 /* SSL connect, read, write timeout */ /* 10 seconds timeout*/
#define MAX_SEND_TIME_OUT  600 /* socket sent timeout in total */

#define MAX_USERNAME_LENGTH 255
#define MAX_PASSWORD_LENGTH 255

#define SOCKET_BUFF_LEN 8192

typedef enum
{
    SOAP_HEAD,
    DOWNLOAD_HEAD,
    UPLOAD_HEAD
} HeadType;

typedef enum
{
    tGetMethod = 0,
    tPostMethod,
    tPutMethod,
    tMaxMethod
} HttpMethod;

typedef enum
{
    STATUS_OK = 0,
    STATUS_NEED_AUTH,
    STATUS_NEED_REDIRECTD,
    STATUS_NO_CONTENT,
    CONTENT_NOT_SUPPORT,
    PROTOCOL_NOT_SUPPORT,
    UNKNOW_STATUS_CODE,
    FORMAT_ERROR,
    PARSER_ERROR
} HttpStatus;

typedef enum
{
    NO_AUTH = 0,
    BASIC_AUTH,
    DIGEST_AUTH
} AuthStatus;

typedef struct _DigestAuth
{
    char username[DIGEST_BIG_LENGTH + 1];
    char realm[DIGEST_MIDDLE_LENGTH + 1];
    char password[DIGEST_MIDDLE_LENGTH + 1];
    char uri[DIGEST_MIDDLE_LENGTH + 1];
    char nonce[DIGEST_MIDDLE_LENGTH + 1];
    char nonceCount[DIGEST_SMALL_LENGTH + 1];
    char cNonce[DIGEST_MIDDLE_LENGTH + 1];
    char qop[DIGEST_MIDDLE_LENGTH + 1];
    char alg[DIGEST_MIDDLE_LENGTH + 1];
    char method[DIGEST_SMALL_LENGTH + 1];
    char opaque[DIGEST_MIDDLE_LENGTH + 1];
    int nc;
} DigestAuth;

typedef struct
{
    char name[COOKIE_NAME_LEN];
    char value[COOKIE_VALUE_LEN];
    char path[COOKIE_PATH_LEN];
    char domain[COOKIE_DOMAIN_LEN];
    char port[COOKIE_PORT_LEN];
    int discard_flag;
} SessionCookie;

int init_session_socket(int *sockFdPtr, void  **sslP, void **ctxP,
                        char *serverName, char *serverIP,  char *hostinfo,
                        int *serverPort, char *path, int *sslFlag, char * url,
                        int *p_addr_type, CAPI_IPPROTO_VERSION_E ipproto_version);
int  destroy_session_socket(int *sockFdPtr, void ** sslP, void ** ctxP, int sslFlag);
int  send_raw(int *sockFdPtr, void **sslP, char * sendBuf, int sendLength, int sslFlag);
int recv_raw(int *sockFdPtr, void **sslP, char *recvBuf, int length, int sslFlag, int *recvLenPtr);


#endif // __IPCAM_HTTPS_H__

