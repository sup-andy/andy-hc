#ifndef _SP_BLE_UART_
#define _SP_BLE_UART_
#include <stdint.h>

#pragma pack(1)

//#define DEVICE_ID    0x49     //(iOS device id)
//#define DEVICE_ID    0x41 //(Android device id)

///#define BLE_ADDR_LEN   18
#define BLE_NAME_LEN   12

#define MAXIMUM_IOS_MTU_SIZE        100
#define IOS_MAX_DATA_TIME_SIZE          18
#define DISPLAY_NAME_LEN                    32
#define BLE_SERVICE_128BIT_UUID_LEN   16


#define IDEVICE_BATTERY_LEVEL_60_PRECENTAGE              60
#define IDEVICE_BATTERY_LEVEL_30_PRECENTAGE              30

#define BLACK_COLOR_RGB         0x000000
#define BLACK_COLOR_RGB         0x000000        // total 3 byte values 
#define WHITE_COLOR_RGB         0xFFFFFF        // 
#define RED_COLOR_RGB           0xFF0000        //
#define BLUE_COLOR_RGB          0x0000FF
#define YELLOW_COLOR_RGB        0xFFFF00
#define CYAN_COLOR_RGB          0x00FFFF
#define MAGENTA_COLOR_RGB       0xFF00FF
#define SILVER_COLOR_RGB        0xC0C0C0
#define GRAY_COLOR_RGB          0x808080
#define MAROON_COLOR_RGB        0x800000
#define OLIVE_COLOR_RGB         0x808000
#define GREEN_COLOR_RGB         0x008000
#define PURPLE_COLOR_RGB        0x800080
#define TEAL_COLOR_RGB          0x008080
#define NAVY_COLOR_RGB          0x000080
#define ORANGE_COLOR_RGB        0xFFA500
#define DARK_ORANGE_COLOR_RGB   0xFF8C00
#define HOT_PINK_COLOR_RGB      0xFF69B4
#define DEEP_PINK_COLOR_RGB     0xFF1493


typedef enum _sp_led_notify_type_e
{
       SP_NOTIFY_NONE,
       SP_NOTIFY_SP_RGB_STATE,
       SP_NOTIFY_ANCS_RGB_SETTING,                      /* ANCS RGB Color Setting */
       SP_NOTIFY_ANCS_CLEARED_STATE,                    /* ANCS Notification Cleared from iOS */
       SP_NOTIFY_MAX
} SP_LED_NOTIFY_TYPE_E ;


typedef enum __sp_ble_uart_protocol_e
{
    SC_LE_REQUEST_DEVICE_INFO,             /* Smart Charger Request to BLE Module */
    LE_SC_REV_BLE_STATE_INFO,    /* Received from BLE Module about current State */
    LE_SC_REV_DEVICE_INFO,                   /* Received from BLE Module  */
    LE_SC_REV_BATTERY_NOTIFY_INFO,    /* Received from BLE Module */
    LE_SC_REV_TIME_NOTIFY_INFO,          /* Received from BLE Module */
    LE_SC_REV_ANCS_NOTIFY_INFO,          /*  Received from BLE Module  */
    LE_SC_REV_ANDROID_NOTIFY_INFO,  /* Received from BLE Module  */
    LE_SC_REV_SIMPLE_NOTIFY_INFO,      /* Received from BLE Module */
    SC_LE_SEND_SIMPLE_NOTIFY_INFO,   /* Smart Charger send to BLE Module */
    SC_LE_MAX_NOTIFY_INFO
} SP_BLE_UART_PROTOCOL_E;


typedef enum __ble_module_status_e
{
    BLE_MODULE_ADVERTISING,
    BLE_MODULE_START_PARING,
    BLE_MODULE_CONNECTED,
    BLE_MODULE_DISCONNECTED,
    BLE_MODULE_OTHERS,
    BLE_MODULE_STATE_MAX
} BLE_MODULE_STATUS_E;

typedef struct _ble_uart_ancs_notify_s
{
    uint8_t EventID;
    uint32_t notify_uid;
    char AppIdentifier[MAXIMUM_IOS_MTU_SIZE];
    char Title[MAXIMUM_IOS_MTU_SIZE];
    char Subtitle[MAXIMUM_IOS_MTU_SIZE];
    char Message[MAXIMUM_IOS_MTU_SIZE];
    char Date[IOS_MAX_DATA_TIME_SIZE];
    char Display_name[DISPLAY_NAME_LEN];
} __attribute__((packed)) BLE_UART_ANCS_NOTIFY_S;

typedef struct __ble_device_info_s
{
    unsigned char ble_status;
    unsigned char ble_bdaddr[BLE_ADDR_LEN];
    char ble_name[BLE_NAME_LEN];
    unsigned char ble_srv_uuid[BLE_SERVICE_128BIT_UUID_LEN];
} BLE_DEVICE_INFO_S;

typedef struct __sc_ble_msg_info_s
{
    unsigned char dev_id;
    unsigned short  data_length;
    unsigned char ble_cmd;
    void *data;
    unsigned int crc_val;
} SC_BLE_MSG_INFO_S;


typedef struct __sp_led_status_s
{
        int val;             /*  RGB  Values  if LED state Enabled otherwise 0*/
        int notify_type;     /* ENUM_IOS_NOTIFICATION_TYPE_E  */
        int led_state;       /* 1->Solid ON, 2 -> Heartbeat  0--> Turn OFF LED */
        int led_time;        /* If any timing for LED heartbeat */
} SP_LED_STATUS_S;


int sp_ble_send_led_state_message(int rgb_val, int notify_type, int led_time) ;
void sp_ble_set_battery_level(int level);
int sp_ble_battery_level_color_setting(void);



#endif
