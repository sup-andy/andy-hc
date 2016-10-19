/*!
    \brief main programm to run XML test cases.

*/
#ifndef TCM_CALL_H
#define  TCM_CALL_H
#include "stdio.h"



//int tcm_HandleEndPointCallEvent(ST_APPCMBS_LINUX_CONTAINER *pContainer);

int tcm_call_establish(int line, char *pCalernum, char *pCallerName);
int tcm_call_release(int line, char * pType);
int tcm_call_answer(int line);
int tcm_call_progress(int line, char * pType);
int tcm_channel_start(int line);

extern void dect_call_request_offhook(int lineid);
extern void dect_call_request_onhook(int lineid);
extern void dect_call_request_dial(int digit, int lineid);
extern void dect_call_request_answer(int lineid);
extern void dect_call_request_flash(int lineid);


#endif   // TCM_CALL_H