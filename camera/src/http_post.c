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

#include "curl/curl.h"
#include <string.h>
#include <stdio.h>
#include "camera_util.h"
extern int posttimeout;

int OnDebug(CURL *k, curl_infotype itype, char * pData, int size, void *j)
{
    if (itype == CURLINFO_TEXT)
    {
        //printf("[TEXT]%s\n", pData);
    }
    else if (itype == CURLINFO_HEADER_IN)
    {
        printf("[HEADER_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_HEADER_OUT)
    {
        printf("[HEADER_OUT]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_IN)
    {
        printf("[DATA_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_OUT)
    {
        printf("[DATA_OUT]%s\n", pData);
    }
    return 0;
}
int offset = 0;
int OnWriteData(void* buffer, int size, int nmemb, void* respbuf)
{
    char *newbuf = NULL;
    if (NULL == respbuf || NULL == buffer)
    {
        return -1;
    }
    //printf("respbuf len = %d\n", size * nmemb);
    if (offset >= (POLLINGBUFLEN - 1024))
    {
        newbuf = malloc(2 * POLLINGBUFLEN);
        if (newbuf == NULL)
        {
            DEBUG_INFO("OnWriteData malloc failure\n");
            memset(pollingrecvbuf, 0, POLLINGBUFLEN);
            return -1;
        }
        else
        {
            DEBUG_INFO("update polling receive buffer length to %d\n", POLLINGBUFLEN * 2);
            memcpy(newbuf, respbuf, offset);
            if (pollingrecvbuf != NULL)
            {
                free(pollingrecvbuf);
            }
            pollingrecvbuf = newbuf;
            respbuf = pollingrecvbuf;
            POLLINGBUFLEN += POLLINGBUFLEN;
        }
    }
    memcpy(respbuf + offset, buffer, size * nmemb);
    offset += size * nmemb;
    //DEBUG_INFO("respbuf = %s\n", (char *)respbuf);
    return size * nmemb;
}


int AppendResData(void* buffer, int size, int nmemb, void* respbuf)
{
    char *p = NULL;
    if (NULL == respbuf || NULL == buffer)
    {
        return -1;
    }
	if((strlen((char*)respbuf) + size * nmemb) > 8192)
	{
		DEBUG_ERROR("Response buffer is too long\n");
		return -1;
	}

	p = strchr(respbuf, '\0');

    memcpy(p, buffer, size * nmemb);
    //DEBUG_INFO("respbuf = %s\n", (char *)respbuf);
    return size * nmemb;
}


int send_post(char *strUrl, char * strPost, char *Responsebuf, int *p_res_code)
{
	CURLcode res;
	CURL* curl = curl_easy_init();
	struct curl_slist *list = NULL;
	offset = 0;
	if (NULL == curl)
	{
		return CURLE_FAILED_INIT;
	}
	if (0)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
	}
	curl_easy_setopt(curl, CURLOPT_URL, strUrl);
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	list = curl_slist_append(list, "Content-Type: application/soap+xml; charset=utf-8");
	//list = curl_slist_append(list, "Expect:");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, Responsebuf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AppendResData);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, posttimeout);
	curl_easy_setopt(curl, CURLOPT_USERPWD, "root:root");
	//curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST | CURLAUTH_BASIC);
	res = curl_easy_perform(curl);

	if(CURLE_OK == res)
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, p_res_code);
	}
	curl_slist_free_all(list);

	curl_easy_cleanup(curl);
	return (int)res;
}


int send_get(char *strUrl, char *Responsebuf)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    //struct curl_slist *list = NULL;
    offset = 0;
    if (NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if (0)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    curl_easy_setopt(curl, CURLOPT_URL, strUrl);
    //curl_easy_setopt(curl, CURLOPT_POST, 0);
    //list = curl_slist_append(list, "Content-Type: application/soap+xml; charset=utf-8");
    //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, Responsebuf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AppendResData);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, posttimeout);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "root:root");
    //curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST | CURLAUTH_BASIC);
    res = curl_easy_perform(curl);
    //curl_slist_free_all(list);

    curl_easy_cleanup(curl);
    return (int)res;
}

int send_file_post(char *strUrl, char *path, char *Responsebuf)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr  = NULL;
    offset = 0;
    if (NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if (1)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "uploadBackup", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);
    //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_PTRCONTENTS, data, CURLFORM_CONTENTSLENGTH, size, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, path, CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_URL, strUrl);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, Responsebuf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AppendResData);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, posttimeout);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "root:root");
    //curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST | CURLAUTH_BASIC);
    res = curl_easy_perform(curl);
    curl_formfree(formpost);

    curl_easy_cleanup(curl);
    return (int)res;
}

