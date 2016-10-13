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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "camera_util.h"
CAMERA_CLIP_FILE_S *clipfile = NULL;

void add_clip_to_camera(DAEMON_CAMERA_INFO_S *camera, CAMERA_CLIP_FILE_S *clipfile)
{
    CAMERA_CLIP_FILE_S *tmp = NULL;
    char *p = NULL;
    time_t t;
    struct tm tmp_time;
    clipfile->next = NULL;
    clipfile->load = 0;


    //struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
    p = strptime(clipfile->triggertime, "%Y-%m-%d %H:%M:%S", &tmp_time);
    if (p == NULL)
    {
        t = time(NULL);
    }
    else
    {
        t = mktime(&tmp_time);
    }
    //printf("%ld\n",t);
    camera->recordtick = t;
    //free(tmp_time);


    if (camera->clip == NULL)
    {
        camera->clip = clipfile;
    }
    else
    {
        if (strlen(camera->clip->filename) > strlen(clipfile->filename))
        {
            DEBUG_ERROR("Discard new %s clip file\n", clipfile->filename);
            free(clipfile);
        }
        else
        {
            DEBUG_ERROR("Discard old %s clip file\n", camera->clip->filename);
            free(camera->clip);
            camera->clip = clipfile;
        }
    }
    memcpy(camera->device.destPath, camera->clip->filename, strlen(camera->clip->filename));
    /*
        tmp = camera->clip;
        while (tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = clipfile;
    */
    return;
}

static void *
printXML(xmlDocPtr doc, xmlNodePtr cur, char *content, int type)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        if (xmlStrcmp(cur->name, (const xmlChar *) "text") != 0)
        {
            //printf("cur->name = %s\n", cur->name);
            xmlChar * value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            //printf("cur->name value = %s\n", value);
            if (type == E_XML_ADDRESS)
            {
                if (strcmp((char *)cur->name, "Address") == 0)
                    memcpy(content, (char *)value, strlen((char *)value));
            } else if (type == E_XML_URI){
                if (strcmp((char *)cur->name, "Uri") == 0)
                    memcpy(content, (char *)value, strlen((char *)value));
            } else if (type == E_XML_CLIP)
            {
                DAEMON_CAMERA_INFO_S *camera = (DAEMON_CAMERA_INFO_S *)content;
                if (strcmp((char *)cur->name, "destPath") == 0)
                {
                    //camera->newclip++;
                    camera->newclip = 1;
                    clipfile = malloc(sizeof(CAMERA_CLIP_FILE_S));

                    memset(clipfile, 0, sizeof(CAMERA_CLIP_FILE_S));
                    memcpy(clipfile->filename, (char *)value, strlen((char *)value));
                    //memcpy(camera->device.destPath, (char *)value, strlen((char *)value));
                }
                if (strcmp((char *)cur->name, "triggerTime") == 0)
                {
                    memcpy(clipfile->triggertime, (char *)value, strlen((char *)value));
                    memcpy(camera->device.triggerTime, (char *)value, strlen((char *)value));
                    add_clip_to_camera(camera, clipfile);
                }

            }
            xmlFree(value);
        }

        printXML(doc, cur, content, type);
        cur = cur->next;
    }

    return NULL;
}

int parseXML(char *buf, char *content, int type)
{
    xmlDocPtr doc;
    //xmlNsPtr ns;
    xmlNodePtr cur;
    doc = xmlParseMemory(buf, strlen(buf));
    //doc = xmlParseFile(filename);
    if (doc == NULL)
    {
        DEBUG_ERROR("parse document error\n");
        return -1;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
        DEBUG_ERROR("empty document\n");
        xmlFreeDoc(doc);
        return -1;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        printXML(doc,  cur, content, type);
        if (strlen(content) > 1)
        {
            break;
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    if (strlen(content) < 2)
    {
        return -1;
    }
    else
    {
        if (strlen(content) < 100)
            DEBUG_INFO("content = %s\n", content);
        return 0;
    }
}

int parse_ONVIF_XML(char *buf, char *content, int type)
{
    int ret = -1;
    if (type == E_XML_CLIP && strlen(buf) < 200)
    {
        //DEBUG_ERROR("buf = %s\n", buf);
        return -1;
    }
    /* COMPAT: Do not genrate nodes for formatting spaces */
    LIBXML_TEST_VERSION
    xmlKeepBlanksDefault(0);
    //printf("XML file: %s\n", filename);
    ret = parseXML(buf, content, type);

    /* Clean up everything else before quitting. */
    //xmlCleanupParser();

    return ret;
}

static void *
printXML_file(xmlDocPtr doc, xmlNodePtr cur, char *content, int type)
{

    static int start = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        if (xmlStrcmp(cur->name, (const xmlChar *) "text") != 0)
        {
            //printf("cur->name = %s\n", cur->name);
            xmlChar * value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            //printf("cur->name value = %s\n", value);

            if (start == 0)
            {
                if (strcmp((char *)cur->name, "i2") == 0)
                {
                    start = 1;
                }
            }
            else if (start == 1)
            {
                if (strcmp((char *)cur->name, "name") == 0)
                {
                    start = 0;
                    if (value != NULL)
                    {
                        memcpy(content, (char *)value, strlen((char *)value));
                        xmlFree(value);
                        break;
                    }
                }

            }
            xmlFree(value);
        }

        printXML_file(doc, cur, content, type);
        cur = cur->next;
    }

    return NULL;
}

int parseXML_file(char *file, char *content, int type)
{
    xmlDocPtr doc;
    //xmlNsPtr ns;
    xmlNodePtr cur;
    //doc = xmlParseMemory(buf, strlen(buf));
    doc = xmlParseFile(file);
    //doc = xmlParseFile(filename);
    if (doc == NULL)
    {
        DEBUG_ERROR("parse document error\n");
        return -1;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
        DEBUG_ERROR("empty document\n");
        xmlFreeDoc(doc);
        return -1;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        printXML_file(doc,  cur, content, type);
        if (strlen(content) > 1)
        {
            break;
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    if (strlen(content) < 2)
    {
        return -1;
    }
    else
    {
        if (strlen(content) < 100)
            DEBUG_INFO("content = %s\n", content);
        return 0;
    }
}

int parse_config_XML_file(char *file, char *content, int type)
{
    int ret = -1;

    /* COMPAT: Do not genrate nodes for formatting spaces */
    LIBXML_TEST_VERSION
    xmlKeepBlanksDefault(0);
    //printf("XML file: %s\n", filename);
    ret = parseXML_file(file, content, type);

    /* Clean up everything else before quitting. */
    //xmlCleanupParser();

    return ret;
}

