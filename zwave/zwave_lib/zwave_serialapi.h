
#ifndef _ZWAVE_SERIALAPI_H_
#define _ZWAVE_SERIALAPI_H_

#include "zwave_appl.h"
//#pragma pack(1)

extern ZW_STATUS exe_status;
extern ZW_STATUS response_status;
extern ZW_STATUS ack_status;


extern int add_remove_node_flag;

extern ZW_NODES_INFO *zw_nodes;
extern BYTE gResponseBuffer[ZW_BUF_SIZE];
extern BYTE gRequestBuffer[ZW_BUF_SIZE];
extern BYTE gResponseBufferLength;
extern BYTE gRequestBufferLength;
extern ZW_TIME timer_array[TIMER_MAX];
extern WORD gGettingStatusNodeID;
extern ZW_STATUS gGettingNodeStatus;
extern BYTE gNodeRequest;

extern void (*cbFuncReportHandle)(ZW_DEVICE_INFO *);
extern void (*cbFuncEventHandler)(ZW_EVENT_REPORT *);
extern void (*cbFuncLearnModeHandler)(LEARN_INFO *);
extern void (*cbFuncAddRemoveNodeSuccess)(ZW_DEVICE_INFO *);
extern void (*cbFuncAddRemoveNodeFail)(ZW_STATUS);
extern void (*cbFuncSendData)(BYTE);
extern void (*cbFuncRemoveFailedNodeHandler)(BYTE);
extern void (*cbFuncSetDefaultHandler)(void);
extern void (*cbFuncMemoryPutBuffer)(void);
extern void (*cbFuncAssignReturnRouteHandler)(BYTE);
extern void (*cbFuncDeleteReturnRouteHandler)(BYTE);



extern BYTE CalculateChecksum(BYTE *pData, int len);
extern BYTE OwnCommandClass(WORD nodeID, BYTE cmdClass);
extern ZW_STATUS WriteCom(BYTE *data, BYTE len);
extern ZW_STATUS  Request(BYTE func, BYTE *pRequest, BYTE reqLen);
extern ZW_STATUS SendRequestAndWaitForResponse(BYTE func, BYTE *pRequest, BYTE reqLen, BYTE *pResponse, BYTE *pResLen);
extern void GetResponseBuffer(BYTE *pResponse, BYTE *pLen);
extern BYTE DoAck(void);
extern void MsecSleep(DWORD Msec);
extern ZW_STATUS WaitStatusReady(ZW_STATUS *pWatchStatus, ZW_STATUS expect_status, DWORD *pMsec);
extern ZW_STATUS CheckDataValid(BYTE *pData, BYTE len);
extern ZW_STATUS ZW_AddNodeToNetwork(BYTE bMode, VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO *));
extern void AddNodeLearnHandler(LEARN_INFO *learn_info);
extern ZW_STATUS ZW_RemoveNodeFromNetwork(BYTE bMode, VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO *));
extern void RemoveNodeLearnHandler(LEARN_INFO *learn_info);
extern ZW_STATUS ZW_GetRandomWord(BYTE *randomWord, BOOL bResetRadio);
extern ZW_STATUS ZW_SetRFReceiveMode(BYTE mode);
extern void SetDefaultHandler(void);
extern ZW_STATUS ZW_SetDefault(VOID_CALLBACKFUNC(completedFunc)(void));
extern ZW_STATUS ZW_SoftReset(void);
extern ZW_STATUS ZW_Version(BYTE *lib_version, BYTE *lib_type);
extern ZW_STATUS ZW_MemoryGetID(BYTE *pHomeID, BYTE *pNodeID);
extern ZW_STATUS MemoryGetBuffer(WORD offset, BYTE *pBuffer, BYTE length);
extern void CB_MemoryPutBuffer(void);
extern ZW_STATUS MemoryPutBuffer(WORD offset, BYTE *pBuffer, WORD length, VOID_CALLBACKFUNC(func)(void));
extern ZW_STATUS ZW_GetInitData(BYTE *version, BYTE *capabilities, BYTE *len, BYTE *nodes, BYTE *chip_type, BYTE *chip_version);
extern ZW_STATUS ZW_GetNodeProtocolInfo(BYTE bNodeID, NODEINFO *nodeInfo);
extern ZW_STATUS ZW_RequestNodeInfo(BYTE nodeID, void (*completedFunc)(BYTE txStatus));
extern ZW_STATUS ZW_isFailedNode(WORD nodeID);
extern void CB_RemoveFailedNodeID(BYTE txStatus);
extern ZW_STATUS ZW_RemoveFailedNodeID(WORD nodeID, VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus));
extern void CB_SendDataComplete(BYTE txStatus);
extern ZW_STATUS ZW_SendData(BYTE nodeID, BYTE *pData, BYTE dataLen, BYTE options, VOID_CALLBACKFUNC(completedFunc)(BYTE));
extern void ZW_SendDataAbout(void);
extern void CB_AssignReturnRoute(BYTE txStatus);
extern ZW_STATUS ZW_AssignReturnRoute(BYTE bSrcNodeID, BYTE bDstNodeID, VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus));
extern void CB_DeleteReturnRoute(BYTE txStatus);
extern ZW_STATUS ZW_DeleteReturnRoute(BYTE nodeID, VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus));


#endif

