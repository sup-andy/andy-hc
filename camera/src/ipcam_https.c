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

#include "ipcam_upgrade.h"
#include "ipcam_https.h"


char gMethodList[tMaxMethod][8] =
{
    "GET",
    "POST",
    "PUT"
};

unsigned short cookie_version;
SessionCookie session_cookie[MAX_COOKIE_NUM];

extern int g_ipcam_use_ssl;



int ipcam_parse_dns(char *name, int *addr_type, char *ip, int length, CAPI_IPPROTO_VERSION_E ipproto_version)
{
    int ret = IPCAM_FAILED, ret1;
    char *ptr;
    struct hostent hostinfo, *phost;
    char result_buf[1024];
    int i;

    if (NULL == name || NULL == addr_type || NULL == ip)
    {
        return ret;
    }


    if (IPPROTO_v4 == ipproto_version)
    {
        // clear cache before call gethostbyname
        res_init();
        ret = gethostbyname2_r(name, AF_INET , &hostinfo, result_buf, sizeof(result_buf), &phost, &ret1);
        if (!ret && !ret1 && phost)
        {
            if (phost->h_addrtype == AF_INET)
            {
                *addr_type = AF_INET;
                for (i = 0; ; i++)
                {
                    ptr = phost->h_addr_list[i];
                    if (NULL == ptr)
                    {
                        break;
                    }
                    if (NULL != inet_ntop(AF_INET, ptr, ip, length))
                    {
                        ret = IPCAM_SUCCESS;
                        break;
                    }
                }
            }
        }
    }

    return ret;
}


/***********************************************************************
Function: http_parse_url
Description:  parse url and get serverName, server(IP format),serverPort, path and ssl flag information


Return: int,
Input: IN char *url,http://www.acsserver.com:8080/acs/TR069
       OUT char *serverName,
       OUT char *server,
       OUT int *serverPort,
       OUT char * path,
       OUT int *sslFlagPtr,
************************************************************************/
int http_parse_url(IN char *acsurl, OUT char *serverName, OUT int *addr_type,
                   OUT char *server, OUT char *hostinfo, OUT int *serverPort, OUT char * path,
                   OUT int *sslFlagPtr, CAPI_IPPROTO_VERSION_E ipproto_version)
{
    char *ptr = NULL, *ptr1 = NULL;
    char url[256];
    struct in6_addr serverAddr6;

    if (acsurl == NULL)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "Uploading URL error in parse url\n");
        return IPCAM_FAILED;
    }
    strcpy(url, acsurl);

    if (strncasecmp(url, "http://", 7) == 0)
    {
        *sslFlagPtr = 0;
        //remove http://  header
        ptr = url + 7 ;
    }
    else if (strncasecmp(url, "https://", 8) == 0)
    {
        *sslFlagPtr = 1;
        //remove https:// header
        ptr = url + 8;
    }
    else
    {
        //sys_log_print(LOG_DEBUG, __FUNCTION__, __LINE__,
        //                   "Uploading URL no http:// or https:// header url[%s]\n", url);
        // use *sslFlagPtr orignal value
        ptr = url;
    }



    //get path
    ptr1 = strstr(ptr, "/");

    if (ptr1 == NULL)
    {
        strcpy(path, "/");
    }
    else
    {
        strcpy(path, ptr1);
        ptr1[0] = '\0';
    }

    strcpy(hostinfo, ptr);

    //get serverPort
    if (*ptr == '[')
    {
        ptr1 = strstr(ptr, "]:");

        if (ptr1 == NULL)
        {
            if ((*sslFlagPtr) == 1)
                *serverPort = 443;
            else
                *serverPort = 80;

            if (NULL == (ptr1 = strstr(ptr, "]")))
            {
                ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                                  "Uploading URL error [%s] in parse url\n", url);
                return IPCAM_FAILED;
            }
            else
            {
                *(ptr1) = '\0';
            }
        }
        else
        {
            *serverPort = atoi(ptr1 + 2);
            *(ptr1) = '\0';
        }
        *addr_type = AF_INET6;
        ptr++;
    }
    else
    {
        if (inet_pton(PF_INET6, ptr, &serverAddr6) > 0)
        {
            if ((*sslFlagPtr) == 1)
                *serverPort = 443;
            else
                *serverPort = 80;

            *addr_type = AF_INET6;
        }
        else
        {
            ptr1 = strstr(ptr, ":");
            if (ptr1 == NULL)
            {
                if ((*sslFlagPtr) == 1)
                    *serverPort = 443;
                else
                    *serverPort = 80;
            }
            else
            {
                *serverPort = atoi(ptr1 + 1);
                ptr1[0] = '\0';
            }
            *addr_type = AF_INET;
        }
    }

    if (IPPROTO_v4 == ipproto_version)
    {
        *addr_type = AF_INET;
    }
    else
    {
        *addr_type = AF_INET6;
    }

    //get serverName server
    if (ptr == NULL || strlen(ptr) == 0)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "No server name in parse url\n");
        return IPCAM_FAILED;
    }
    strcpy(serverName, ptr);


    if (ipcam_parse_dns(serverName, addr_type, server, IP_LENGTH, ipproto_version) < 0)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "Failed to gethostbyname [%s].\n", ptr);
        return IPCAM_FAILED;
    }

    return IPCAM_SUCCESS;
}

#ifdef USE_SSL

/***********************************************************************
Function: SSL_connect_ex
Description:  non-blocking ssl_connect.

Return: Result,
Input: IN int fd,  the http client socket
       OUT SSL **sslP,  ssl object binding with the socket
Output:
Others:
************************************************************************/
int SSL_connect_ex(int fd, SSL **sslP)
{
    int r = -1;
    unsigned long fc = 0;
    struct timeval timeout;
    timeout.tv_sec = SSL_TIME_OUT;
    timeout.tv_usec = 0;

    fc = 1;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        return r;
    }

    for (;;)
    {
        if ((r = SSL_connect(*sslP)) <= 0)
        {
            int err = SSL_get_error(*sslP, r);
            if (err != SSL_ERROR_WANT_CONNECT &&
                    err != SSL_ERROR_WANT_ACCEPT &&
                    err != SSL_ERROR_WANT_READ &&
                    err != SSL_ERROR_WANT_WRITE)
            {
                goto SSL_CONNECT_EX_EXIT;
            }
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            if (err == SSL_ERROR_WANT_READ)
                r = select((int)fd + 1, &fds, NULL, NULL, &timeout);
            else if (err == SSL_ERROR_WANT_WRITE)
                r = select((int)fd + 1, NULL, &fds, NULL, &timeout);
            else
                r = select((int)fd + 1, &fds, &fds, &fds, &timeout);

            if (r == 0)
            {
                goto SSL_CONNECT_EX_EXIT;
            }
            else if (r < 0)
            {
                if (errno == EAGAIN  ||  errno == EINTR)
                {
                    continue;
                }
                else
                {
                    goto SSL_CONNECT_EX_EXIT;
                }
            }
        }
        else
        {
            break;
        }
    }

    r = 1;

SSL_CONNECT_EX_EXIT:
    fc = 0;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        r =  -1;
    }

    return r;
}

/***********************************************************************
Function: SSL_read_ex
Description:  non-blocking ssl_read.

Return: Result,
Input: IN int fd,  the http client socket
       OUT SSL **sslP,  ssl object binding with the socket
Output:
Others:
************************************************************************/
int SSL_read_ex(int fd, SSL **sslP, OUT char *recvBuf, IN  int length)
{
    int r = -1;
    unsigned long fc = 0;
    struct timeval timeout;
    timeout.tv_sec = SSL_TIME_OUT;
    timeout.tv_usec = 0;

    fc = 1;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, set bio to non-blocking error\n", fd);
        return r;
    }

    for (;;)
    {
        r = SSL_read(*sslP, recvBuf, length);
        if (r <= 0)
        {
            int err = SSL_get_error(*sslP, r);
            if (err != SSL_ERROR_WANT_CONNECT &&
                    err != SSL_ERROR_WANT_ACCEPT &&
                    err != SSL_ERROR_WANT_READ &&
                    err != SSL_ERROR_WANT_WRITE)
            {
                goto SSL_READ_EX_EXIT;
            }

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            if (err == SSL_ERROR_WANT_READ)
                r = select((int)fd + 1, &fds, NULL, NULL, &timeout);
            else if (err == SSL_ERROR_WANT_WRITE)
                r = select((int)fd + 1, NULL, &fds, NULL, &timeout);
            else
                r = select((int)fd + 1, &fds, &fds, &fds, &timeout);

            if (r == 0)
            {
                goto SSL_READ_EX_EXIT;
            }
            else if (r < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    goto SSL_READ_EX_EXIT;
                }
            }
        }
        else
        {
            break;
        }
    }

SSL_READ_EX_EXIT:

    fc = 0;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, recover bio to blocking error\n", fd);
        return -1;
    }
    return r;
}

/***********************************************************************
Function: SSL_write_ex
Description:  non-blocking ssl_read.

Return: Result,
Input: IN int fd,  the http client socket
       OUT SSL **sslP,  ssl object binding with the socket
Output:
Others:
************************************************************************/
int SSL_write_ex(int fd, SSL **sslP, OUT char *recvBuf, IN  int length)
{
    int r = -1;
    unsigned long fc = 0;
    struct timeval timeout;
    fd_set fds;
    time_t time_start = time(NULL);

    fc = 1;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, set bio to non-blocking error\n", fd);
        return r;
    }

    for (;;)
    {
        /* Check total timer, not exceed MAX_SEND_TIME_OUT seconds*/
        if ((time(NULL) - time_start) >= MAX_SEND_TIME_OUT)
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, SSL_write timeout, failed to send data in [%d] seconds, length = [%d].\n", fd, MAX_SEND_TIME_OUT, length);

            r = -1;
            goto SSL_WRITE_EX_EXIT;
        }

        r = SSL_write(*sslP, recvBuf, length);
        if (r <= 0)
        {
            int err = SSL_get_error(*sslP, r);
            if (err != SSL_ERROR_WANT_CONNECT &&
                    err != SSL_ERROR_WANT_ACCEPT &&
                    err != SSL_ERROR_WANT_READ &&
                    err != SSL_ERROR_WANT_WRITE)
            {
                goto SSL_WRITE_EX_EXIT;
            }

            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            timeout.tv_sec = SSL_TIME_OUT;
            timeout.tv_usec = 0;

            if (err == SSL_ERROR_WANT_READ)
                r = select((int)fd + 1, &fds, NULL, NULL, &timeout);
            else if (err == SSL_ERROR_WANT_WRITE)
                r = select((int)fd + 1, NULL, &fds, NULL, &timeout);
            else
                r = select((int)fd + 1, &fds, &fds, &fds, &timeout);

            if (r == 0)
            {
                goto SSL_WRITE_EX_EXIT;
            }
            else if (r < 0)
            {
                if (errno == EAGAIN  ||  errno == EINTR)
                {
                    continue;
                }
                else
                {
                    goto SSL_WRITE_EX_EXIT;
                }
            }
        }
        else
        {
            break;
        }
    }

SSL_WRITE_EX_EXIT:

    fc = 0;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, recover bio to blocking error\n", fd);
        return -1;
    }
    return r;
}

static pthread_mutex_t *g_lock_cs = NULL;
static int g_ssl_init = 0;

void CRYPTO_THREADID_thread_id(CRYPTO_THREADID *id)
{
    CRYPTO_THREADID_set_numeric(id, (unsigned long)pthread_self());
}

unsigned long ssl_pthreads_thread_id(void)
{
    unsigned long ret;

    ret = (unsigned long)pthread_self();
    return (ret);
}

void ssl_pthreads_locking_callback(int mode, int type, char *file,
                                   int line)
{
    if (mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&(g_lock_cs[type]));
    }
    else
    {
        pthread_mutex_unlock(&(g_lock_cs[type]));
    }
}

int ssl_thread_setup(void)
{
    int i;

    g_lock_cs = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    if (g_lock_cs == NULL)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "OPENSSL_malloc failure.\n");

        return -1;
    }

    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        pthread_mutex_init(&(g_lock_cs[i]), NULL);
    }

    CRYPTO_THREADID_set_callback(CRYPTO_THREADID_thread_id);
    CRYPTO_set_locking_callback((void (*)(int, int, const char*, int))ssl_pthreads_locking_callback);

    return 0;
}

/***********************************************************************
Function: init_ssl_socket
Description:  bind socket fd to a ssl object.

Return: int,
Input: IN int *sockfdPtr,  the http client socket
       OUT SSL **sslP,  ssl object binding with the socket
       OUT SSL_CTX **ctxP, ssl enviroment
Output:
Others:
************************************************************************/
int init_ssl_socket(IN int *sockfdPtr, OUT SSL **sslP, OUT SSL_CTX **ctxP, char *url)
{
    int ret = IPCAM_SUCCESS;

    if (!g_ssl_init)
    {
        SSL_load_error_strings();
        if (SSL_library_init() != 1 || ssl_thread_setup() != 0)
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                              "load SSL lib failure.\n");
            return IPCAM_SSL_FAILED;
        }
        g_ssl_init = 1;
    }

    if ((*ctxP = SSL_CTX_new(SSLv23_method())) == NULL)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "create SSLv23 context failure.\n");
        ret = IPCAM_SSL_FAILED;
        goto end;
    }

    if ((*sslP = SSL_new(*ctxP)) == NULL)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "create SSL object failure.\n");
        ret = IPCAM_SSL_FAILED;
        goto end;
    }

    if (SSL_set_fd(*sslP, *sockfdPtr) != 1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "bind SSL socket failure.\n");
        ret = IPCAM_SSL_FAILED;
        goto end;
    }

    {
        int  i;
        char stringall[128];
        memset(stringall, 0, sizeof(stringall));
        srandom(time(NULL) + ssl_pthreads_thread_id());

        for (i = 0; i < 128; i++)
        {
            stringall[i] = (random() % (128));
        }

        RAND_seed(stringall, sizeof(stringall));
    }

    if (SSL_connect_ex(*sockfdPtr, sslP) != 1)
    {
        ERR_print_errors_fp(stderr);
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "SSL socket connect failure.\n");
        ret = IPCAM_SSL_FAILED;
    }

end:
    if (IPCAM_SUCCESS != ret)
    {
        if (*sslP != NULL)
        {
            SSL_free(*sslP);
            *sslP = NULL;
        }

        if (*sockfdPtr > 0)
        {
            close(*sockfdPtr);
            *sockfdPtr = -1;
        }

        if (*ctxP != NULL)
        {
            SSL_CTX_free(*ctxP);
            *ctxP = NULL;
        }
    }
    return ret;
}

#endif




/***********************************************************************
Function: init_session_socket
Description:  initialize a TCP connection through basic socket or SSL  with the given server ip and port.

Return: int,
Input: OUT int *sockFdPtr, the created socket.
       OUT SSL **sslP,  if needed, the created ssl binding with the socket.
       OUT SSL_CTX **ctxP, if needed, the ssl environment
       OUT char *serverName, http server name
       OUT char *serverIP, http server IP
       OUT int *serverPort, http server port
       OUT char *path, http requested path
       OUT int *sslFlag, ssl enabled or disabled.
       IN char * url, the target url ,acs url of ftp/http url.
       IN char *acsUserName, http authenticated username
       IN char *acsPassword,  http authenticated password
Output:
Others:
************************************************************************/
int init_session_socket(OUT int *sockFdPtr, OUT void  **sslP, OUT void **ctxP,
                        OUT char *serverName, OUT char *serverIP,  OUT char *hostinfo,
                        OUT int *serverPort, OUT char *path, OUT int *sslFlag, IN char * url,
                        OUT int *p_addr_type, CAPI_IPPROTO_VERSION_E ipproto_version)
{
    int ret = IPCAM_SUCCESS;
    struct timeval timeout;
    fd_set fds;
    int errlen, res;
    int error;
    unsigned long fc;
    struct sockaddr_in dest;
    struct sockaddr_in6 dest_v6;
    int addr_type, result;

    if (http_parse_url(url, serverName, &addr_type, serverIP, hostinfo, serverPort, path, sslFlag, ipproto_version) != IPCAM_SUCCESS)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "URL failed to parse, URL = [%s], IP version = [%s]\n", strlen(url) ? url : "NULL",
                          (ipproto_version == IPPROTO_v6) ? "IPv6" : "IPv4");

        ret = IPCAM_INIT_SOCKET_FAILED;
        goto INIT_SESSION_SOCKET_EXIT;
    }

    if (addr_type == AF_INET)
    {
        if ((*sockFdPtr = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                              "Create main session socket failed\n");

            ret = IPCAM_INIT_SOCKET_FAILED;
            goto INIT_SESSION_SOCKET_EXIT;
        }
    }
    else
    {
#ifdef ENABLE_IPV6_IN_USER_APP
        if ((*sockFdPtr = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                              "Create main session socket failed\n");

            ret = IPCAM_INIT_SOCKET_FAILED;
            goto INIT_SESSION_SOCKET_EXIT;
        }
#else
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "not support for ipv6\n");

        ret = IPCAM_INIT_SOCKET_FAILED;
        goto INIT_SESSION_SOCKET_EXIT;
#endif
    }

    bzero(&dest, sizeof(dest));
    bzero(&dest_v6, sizeof(dest_v6));

    if (AF_INET == addr_type)
    {
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr(serverIP);
        dest.sin_port = htons(*serverPort);
    }
    else
    {
        dest_v6.sin6_family = AF_INET6;
        dest_v6.sin6_flowinfo = 0;
        dest_v6.sin6_port = htons(*serverPort);
        inet_pton(PF_INET6, serverIP, &dest_v6.sin6_addr);
    }

    fc = 1;
    if (ioctl(*sockFdPtr, FIONBIO, &fc) == -1)
    {
        ret = IPCAM_INIT_SOCKET_FAILED;
        goto INIT_SESSION_SOCKET_EXIT;
    }

    if (AF_INET == addr_type)
    {
        result = connect(*sockFdPtr, (struct sockaddr *) & dest, sizeof(struct sockaddr));
    }
    else
    {
#ifdef ENABLE_IPV6_IN_USER_APP
        result = connect(*sockFdPtr, (struct sockaddr *) & dest_v6, sizeof(struct sockaddr_in6));
#else
        result = -1;
#endif
    }

    if (result == -1)
    {
        if (errno == EINPROGRESS)
        {
            errlen = sizeof(int);
            FD_ZERO(&fds);
            FD_SET(*sockFdPtr, &fds);
            timeout.tv_sec = CONNECT_TIME_OUT_SECOND;
            timeout.tv_usec = CONNECT_TIME_OUT_USECOND;

            if ((res = select(*sockFdPtr + 1, NULL, &fds, NULL, &timeout)) > 0)
            {
                getsockopt(*sockFdPtr, SOL_SOCKET, SO_ERROR, &error, (socklen_t *) &errlen);
                if (error != 0)
                {
                    ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                                      "Remote server connect SO_ERROR, value = [%d], reason = [%s]\n", error, strerror(error));

                    ret = IPCAM_TCP_FAILED;
                    goto INIT_SESSION_SOCKET_EXIT;
                }
                else
                {
                    errno = 0;
                }
            }
            else
            {
                ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                                  "Remote server connect Timeout\n");

                ret = IPCAM_TCP_FAILED;
                goto INIT_SESSION_SOCKET_EXIT;
            }
        }
        else
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                              "Remote server connect failed\n");

            ret = IPCAM_TCP_FAILED;
            goto INIT_SESSION_SOCKET_EXIT;
        }
    }

    fc = 0;
    if (ioctl(*sockFdPtr, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "IOCTL error\n");

        ret = IPCAM_INIT_SOCKET_FAILED;
        goto INIT_SESSION_SOCKET_EXIT;
    }

    if (g_ipcam_use_ssl)
    {
        if (*sslFlag == 1)
            ret = init_ssl_socket(sockFdPtr, (SSL **) sslP, (SSL_CTX **) ctxP, url);
    }
    else
    {
        if (*sslFlag == 1)
            ret = IPCAM_SSL_FAILED;
    }

    if (p_addr_type)
        *p_addr_type = addr_type;

INIT_SESSION_SOCKET_EXIT:
    if (ret != IPCAM_SUCCESS && (*sockFdPtr > 0))
    {
        close(*sockFdPtr);
        *sockFdPtr = -1;
    }

    return ret;
}

/***********************************************************************
Function: destroy_session_socket
Description:  destroy the socket and ssl resources.

Return: int ,
Input: int *sockFdPtr,
       void ** sslP,
       void ** ctxP,
       int sslFlag,
Output:
Others:
************************************************************************/
int  destroy_session_socket(int *sockFdPtr, void ** sslP, void ** ctxP, int sslFlag)
{
    int ret = IPCAM_SUCCESS;
    char buf[SOCKET_BUFF_LEN];
    int fc, rc;

    //reset scoket
#ifdef USE_SSL
    if (sslFlag == 1)
    {
        fc = 1;
        if (ioctl(*sockFdPtr, FIONBIO, &fc) > 0)
        {
            do
            {
                rc = SSL_read(* sslP, buf, sizeof(buf));
            }
            while (rc > 0);
        }

        if (* ((SSL **) sslP) != NULL)
        {
            SSL_shutdown(* ((SSL **) sslP));
            SSL_free(* ((SSL **) sslP));
            *sslP = NULL;
        }

        close(*sockFdPtr);

        if (* ((SSL_CTX **) ctxP) != NULL)
        {
            SSL_CTX_free(* ((SSL_CTX **) ctxP));
            *ctxP = NULL;
        }
    }
    else
#endif
    {
        fc = 1;
        if (ioctl(*sockFdPtr, FIONBIO, &fc) > 0)
        {
            do
            {
                rc = read(*sockFdPtr, buf, sizeof(buf));
            }
            while (rc > 0);
        }
        close(*sockFdPtr);
    }
    return ret;
}

char *ssl_error_string(int err_code)
{
    switch (err_code)
    {
        case SSL_ERROR_NONE:
            return "SSL_ERROR_NONE";
        case SSL_ERROR_SSL:
            return "SSL_ERROR_SSL";
        case SSL_ERROR_WANT_READ:
            return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:
            return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_X509_LOOKUP:
            return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:
            return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_ZERO_RETURN:
            return "SSL_ERROR_ZERO_RETURN";
        case SSL_ERROR_WANT_CONNECT:
            return "SSL_ERROR_WANT_CONNECT";
        case SSL_ERROR_WANT_ACCEPT:
            return "SSL_ERROR_WANT_ACCEPT";
    }

    return "SSL_ERROR_UNKNOWN ";
}


extern int errno;



static int ipcam_send(int fd, const char *pdata, int len)
{
    int i, n = 0;
    int fc;
    int nselect = 0;
    struct timeval timeo;
    fd_set fds;
    time_t time_start = time(NULL);

    if ((fd <= 0) || (NULL == pdata) || (len <= 0))
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "invalid para fd[%d] pdata[%x] len[%d]\n", fd, pdata, len);
        return -1;
    }

    fc = 1;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        return -1;
    }

    do
    {
        /* Check total timer, not exceed MAX_SEND_TIME_OUT seconds*/
        if ((time(NULL) - time_start) >= MAX_SEND_TIME_OUT)
        {
            ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, send timeout, failed to send data in [%d] seconds, length = [%d].\n", fd, MAX_SEND_TIME_OUT, len);

            break;
        }

        i = send(fd, pdata + n, (len - n), MSG_DONTWAIT);
        if (i > 0)
        {
            n += i;
            if (n >= len)
            {
                break;
            }
        }
        else
        {
            if (EINTR == errno || EAGAIN == errno)
            {
                FD_ZERO(&fds);
                FD_SET(fd, &fds);
                timeo.tv_sec = SEND_TIME_OUT;
                timeo.tv_usec = 0;

                nselect = select(fd + 1, NULL, &fds, NULL, &timeo);
                if (nselect > 0)
                {
                    continue;
                }
                break;
            }
            break;
        }
    }
    while (1);

    fc = 0;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, recover bio to blocking error\n", fd);
        return -1;
    }

    return n;
}

static int ipcam_safe_read(int fd, char *buf, int count)
{
    int i, n = 0;
    int fc;
    int nselect = 0;
    struct timeval timeo;
    fd_set fds;

    if ((fd <= 0) || (NULL == buf) || (count <= 0))
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE,
                          "invalid para fd[%d] pdata[%x] len[%d]\n", fd, buf, count);
        return -1;
    }

    fc = 1;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        return -1;
    }

    timeo.tv_sec = RECV_TIME_OUT;
    timeo.tv_usec = 0;

    do
    {
        i = recv(fd, buf + n, count - n, MSG_DONTWAIT);
        if (i < 0)
        {
            if (EINTR == errno || EAGAIN == errno)
            {
                FD_ZERO(&fds);
                FD_SET(fd, &fds);
                nselect = select(fd + 1, &fds, NULL, NULL, &timeo);
                if (nselect > 0)
                {
                    continue;
                }
                break;
            }
            break;
        }
        else if (i == 0)
        {
            break;
        }
        n += i;
        if (n >= count)
        {
            break;
        }
    }
    while (1);

    fc = 0;
    if (ioctl(fd, FIONBIO, &fc) == -1)
    {
        ctrl_log_print_ex(LOG_ERR, __FUNCTION__, __LINE__, IPCAM_CTRL_NAME_TYPE, "fd = %d, recover bio to blocking error\n", fd);
        return -1;
    }

    return n;
}


/***********************************************************************
Function: send_raw
Description:  send raw data through the given socket or ssl.

Return: int ,
Input: IN int *sockFdPtr, the given socket
       IN void **sslP,  the given ssl
       IN char * sendBuf,  ready to send data
       IN int sendLength, sending data length
       IN int sslFlag,  ssl enable or disable
Output:
Others:
************************************************************************/
int  send_raw(IN int *sockFdPtr, IN void **sslP, IN char * sendBuf, IN int sendLength, IN int sslFlag)
{
    int ret;
    int send_ret_value;
    fd_set sendfds;
    struct timeval timeout;

    FD_ZERO(&sendfds);
    FD_SET(*sockFdPtr, &sendfds);
    timeout.tv_sec = SEND_TIME_OUT;
    timeout.tv_usec = 0;

    if (sendBuf == NULL || sendLength <= 0)
        return IPCAM_SUCCESS;

    if (select(*sockFdPtr + 1, NULL, &sendfds, NULL, &timeout) > 0)
    {
        if (FD_ISSET(*sockFdPtr, &sendfds))
        {
            if (sslFlag == 1)
            {
                if (g_ipcam_use_ssl)
                {
                    ERR_clear_error();

                    send_ret_value = SSL_write_ex(*sockFdPtr, (SSL **)sslP, sendBuf, sendLength);
                    if (send_ret_value < sendLength)
                    {
                        if (errno == EAGAIN  ||  errno == EINTR)
                        {
                            ret = IPCAM_SEND_FAILED;
                        }
                        else
                        {
                            /* connection was reset by peer. */
                            if (errno == ECONNRESET)
                            {
                                SSL_set_shutdown(* ((SSL **) sslP), SSL_RECEIVED_SHUTDOWN);
                                SSL_set_shutdown(* ((SSL **) sslP), SSL_SENT_SHUTDOWN);
                            }
                            ret = IPCAM_SEND_FAILED;
                        }

                    }
                    else
                    {
                        ret = IPCAM_SUCCESS;
                    }
                }
                else
                {
                    ret = IPCAM_SEND_FAILED;
                }
            }
            else
            {
                send_ret_value = ipcam_send(*sockFdPtr, sendBuf, sendLength);
                if (send_ret_value < sendLength)
                {
                    ret = IPCAM_SEND_FAILED;
                }
                else
                {
                    ret = IPCAM_SUCCESS;
                }
            }
        }
        else
        {
            ret = IPCAM_SEND_FAILED;
        }
    }
    else
    {
        ret = IPCAM_SEND_FAILED;
    }

    return ret;
}

/***********************************************************************
Function: recv_raw
Description:  receive data through given socket or ssl

Return: int,
Input: IN int *sockFdPtr, the pointer point to the socket
       IN void **sslP,  the pointer point to  the pointer point to the ssl
       OUT char *recvBuf, the pointer point to the receiving data
       IN int length, max recv length length
       IN int sslFlag, ssl enable
       OUT int *recvLenPtr, real recv length
Output:
Others:
************************************************************************/
int recv_raw(IN int *sockFdPtr, IN void **sslP, OUT char *recvBuf, IN  int length, IN  int sslFlag, OUT int *recvLenPtr)
{
    int ret = IPCAM_SUCCESS;
    int recv_ret_value;

    *recvLenPtr = 0;
    if (sslFlag == 1)
    {
        if (g_ipcam_use_ssl)
        {
            ERR_clear_error();

            recv_ret_value = SSL_read_ex(*sockFdPtr, (SSL **)sslP, recvBuf, length);
            if (recv_ret_value < 0)
            {
                if (errno == EAGAIN  ||  errno == EINTR)
                {

                    ret = IPCAM_RECV_FAILED;
                }
                else
                {

                    /* connection was reset by peer. */
                    if (errno == ECONNRESET)
                    {
                        SSL_set_shutdown(* ((SSL **) sslP), SSL_RECEIVED_SHUTDOWN);
                        SSL_set_shutdown(* ((SSL **) sslP), SSL_SENT_SHUTDOWN);
                    }

                    ret = IPCAM_RECV_FAILED;
                }
            }
            else
            {
                if (recv_ret_value > length)
                {
                    recvBuf[0] = '\0';
                    ret = IPCAM_RECV_FAILED;
                }
                else
                {
                    *recvLenPtr = recv_ret_value;
                    recvBuf[recv_ret_value] = '\0';
                }
            }
        }
        else
        {

            ret = IPCAM_RECV_FAILED;
        }
    }
    else
    {
        recv_ret_value = ipcam_safe_read(*sockFdPtr, recvBuf, length);
        if (recv_ret_value < 0)
        {
            ret = IPCAM_RECV_FAILED;
        }
        else
        {
            if (recv_ret_value > length)
            {
                recvBuf[0] = '\0';
                ret = IPCAM_RECV_FAILED;
            }
            else
            {
                *recvLenPtr = recv_ret_value;
                recvBuf[recv_ret_value] = '\0';
            }
        }
    }
    return ret;
}


