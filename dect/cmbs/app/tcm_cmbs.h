/*!
    \brief main programm to run XML test cases.

*/
#ifndef TCM_CMBS_H
#define  TCM_CMBS_H
#include "cmbs_api.h"
#include "cmbs_han.h"
#include "dect_device.h"

typedef enum {
    TCM_DEVICE_REGISTER_SUCCESS = 0,
    TCM_DEVICE_REGISTER_FAILURE,    
    TCM_DEVICE_REGISTER_UNKNOW,
    TCM_DEVICE_REGISTER_MAX
} TCM_DEVICE_REGISTER_STUTAS_E;

int tcm_cmbs_init(void);
int tcm_han_mgr_start(void);
int tcm_GetDeviceList(DECT_HAN_DEVICES *pDevice_info);
int tcm_dect_device_on_off(int on, int dstid);
int tcm_dect_device_slide(int value, int dstid);
int tcm_createDeviceReportThread(void);
int tcm_createDeviceKeepAliveThread(void);
void tcm_HanRegisterInterface(void);
void tcm_HanRegisterUnterface(void);
int tcm_add_device(unsigned int timeout);
int tcm_delete_device(unsigned int  id);
int tcm_close_register(void);
int tcm_get_cmbs_info(DECT_CMBS_INFO *pInfo);
int tcm_device_image_upgrade(char *fw_file_path);
void *tcm_DeviceKeepAliveThread(void *args);
void* tcm_DeviceReportThread(void *args);
const char*  tcm_GetDeviceName(int type);
void dect_Handle_voip_Message(char *pMsg);
int tcm_handset_list(DECT_HAN_DEVICES *pDevice_info);
int tcm_handset_delete(char *hs);
int tcm_handset_page(unsigned int id);
int tcm_handset_stop_page(void);
int tcm_dect_type_set(int type);
int tcm_dect_type_get(int *type);
int tcm_dect_system_reboot();


#endif   // TCM_CMBS_H
