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
#include "base64.h"

// SerComm BASE64 table
char keyStr[] = "ACEGIKMOQSUWYBDFHJLNPRTVXZacegikmoqsuwybdfhjlnprtvxz0246813579=+/";

char userKeyStr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/*--------------------------------------------------------
//  Description:   Encrypt the input data with the base64
//  Input:
//                 char i_buf[]- input buffer
//                 char i_key_str[]- key str
//  Output:
//                 char o_buf[]- output buffer
//  Return:
//                 encrypted string length
//--------------------------------------------------------*/
int encode64(char i_buf[], char o_buf[], char i_key_str[])
{
    char chr1 = (char)0;
    char chr2 = (char)0;
    char chr3 = (char)0;

    //These are the 3 bytes to be encoded
    int enc1 = 0;
    int enc2 = 0;
    int enc3 = 0;
    int enc4 = 0; //These are the 4 encoded bytes
    int i = 0, j = 0; //Position counter

    do   //Set up the loop here
    {
        chr1 = i_buf[i++]; //Grab the first byte

        if (i < strlen(i_buf))
            chr2 = i_buf[i++]; //Grab the second byte

        if (i < strlen(i_buf))
            chr3 = i_buf[i++]; //Grab the third byte

        //Here is the actual base64 encode part.
        //There really is only one way to do it.

        enc1 = chr1 >> 2;
        enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
        enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
        enc4 = chr3 & 63;

        if (chr2 == (char)0)
        {
            enc3 = enc4 = 64;
        }
        else if (chr3 == (char)0)
        {
            enc4 = 64;
        }

        o_buf[j++] = i_key_str[enc1];
        o_buf[j++] = i_key_str[enc2];
        o_buf[j++] = i_key_str[enc3];
        o_buf[j++] = i_key_str[enc4];

        // OK, now clean out the variables used.
        chr1 = chr2 = chr3 = (char)0;
        enc1 = enc2 = enc3 = enc4 = (char)0;
    }
    while (i < strlen(i_buf)); //And finish off the loop

    //Now return the encoded values.
    return j;
}


/*--------------------------------------------------------
//  Description:   decrypt the input data with the base64
//  Input:
//                 char i_buf[]- input buffer
//                 char i_key_str[]- key str
//  Output:
//                 char o_buf[]- output buffer
//  Return:
//                 decrypted string length
//--------------------------------------------------------*/
int decode64(char i_buf[], char o_buf[], char i_key_str[])
{
    //These are the 3 bytes to be encoded
    char chr1 = (char)0;
    char chr2 = (char)0;
    char chr3 = (char)0;

    //These are the 4 encoded bytes
    int enc1 = 0;
    int enc2 = 0;
    int enc3 = 0;
    int enc4 = 0;

    int i = 0, j = 0; //Position counter

    do   //Here¡¯s the decode loop.
    {
        //Grab 4 bytes of encoded content.
        enc1 = (int)(strchr(i_key_str, i_buf[i++]) - i_key_str);
        if (i < strlen(i_buf))
            enc2 = (int)(strchr(i_key_str, i_buf[i++]) - i_key_str);
        if (i < strlen(i_buf))
            enc3 = (int)(strchr(i_key_str, i_buf[i++]) - i_key_str);
        if (i < strlen(i_buf))
            enc4 = (int)(strchr(i_key_str, i_buf[i++]) - i_key_str);

        //Heres the decode part. There¡¯s really only one way to do it.
        chr1 = (enc1 << 2) | (enc2 >> 4);
        chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
        chr3 = ((enc3 & 3) << 6) | enc4;

        o_buf[j++] = (char)chr1;

        if (enc3 != 64)
        {
            o_buf[j++] = (char)chr2;
        }

        if (enc4 != 64)
        {
            o_buf[j++] = (char)chr3;
        }
        //now clean out the variables used
        chr1 = (char)0;
        chr2 = (char)0;
        chr3 = (char)0;

        enc1 = 0;
        enc2 = 0;
        enc3 = 0;
        enc4 = 0;
    }
    while (i < strlen(i_buf)); //finish off the loop

    //Now return the decoded values.
    return j;
}

