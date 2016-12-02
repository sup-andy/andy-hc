#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <capi.h>
#include <unistd.h>
#include <fcntl.h>
#include "ctrl_common_lib.h"
#include "sp_ble_serial.h"

#define GET_CABLE_PHONE_INFO_COMMAND    "get_connected_phone_info"
int keep_looping 		 = 1;
int g_notify_type        = LED_STATE_SOLID_ON;
int g_battery_level	     = 0xFF ;
int g_bootloader_enabled     = 0x00 ;  /* 0 --> TI BLE module bootloader disabled , 1 --> TI BLE module bootloader enabled */

DBusConnection* dbus_conn;


extern int read_app_event(CTRL_SIG_INFO_U info, DBusConnection * conn, DBusError * err);

static int sp_ble_ctrl_debug_call_back(DBusConnection* conn, DBusMessage* msg)
{
    char *result_msg = NULL;
    int ret = 0;
    char *errstring = "OK";
    int errline = 0;
    int len;
    int offset;

    int command = ctrl_parse_debug_command(msg);

    result_msg = calloc(MAX_CTRL_DEBUG_RESULT_MSG, 1);

    switch (command) {

        case CTRL_DEBUG_GET_STATUS:

            //result_msg = calloc(MAX_CTRL_DEBUG_RESULT_MSG, 1);

            if (!result_msg) {
                ret = ENOMEM;
                errline = __LINE__;
                errstring =  "calloc failed";
                JUMP_AND_RETURN(error_return);
            }

            /* process mediaserver status*/
            offset = 0;
            len = sprintf(result_msg,
                          "sp_ble_ctrl is alive.\n");

            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,
                           "len[%d] offset[%d]\n", len, offset);
            if (len <= 0) {
                ret = ENOENT;
                errline = __LINE__;
                errstring =  "sprintf failed";
                JUMP_AND_RETURN(error_return);
            }
            offset += len;

            break;
	case CTRL_DEBUG_USER1:

	   if (!result_msg) {
                ret = ENOMEM;
                errline = __LINE__;
                errstring =  "calloc failed";
                JUMP_AND_RETURN(error_return);
            }

            /* process mediaserver status*/
            offset = 0;
	    if (g_bootloader_enabled)  /*  TI BLE Bootlader backdoor On  */
	    {
            	len = sprintf(result_msg,
                          		"ON\n");
	    }
	    else  /* TI BLE Bootlader backdoor Off  */
	    {
		 len = sprintf(result_msg,
                                        "OFF\n");
	    }
            ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,
                           "len[%d] offset[%d]\n", len, offset);
            if (len <= 0) {
                ret = ENOENT;
                errline = __LINE__;
                errstring =  "sprintf failed";
                JUMP_AND_RETURN(error_return);
            }
            offset += len;
				    	
	    break;
            /*
             * Other command cases...TBD
             */

        default:
            ret = ERANGE;
            errline = __LINE__;
            errstring =  "command out of range";
            JUMP_AND_RETURN(error_return);
            break;

    } // switch


error_return:
    if (ret != 0) {
        ctrl_log_print(LOG_ERR, __FUNCTION__, errline,
                       "ERROR errstring[%s] ret[%d:%s] command[%d]\n", errstring, ret, strerror(ret), command);
    } else {
        ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__,
                       "OKAY command[%d] result_msg[%s]\n", command, result_msg);
        ctrl_reply_debug_command(conn, msg, (char *)result_msg);
    }

    if (NULL != result_msg)
        free(result_msg);

    return (ret);
}


//if (( g_notify_type != SC_NOTIFY_SP_RGB_STATE) && 
//				( g_notify_type != SC_NOTIFY_ANCS_RGB_SETTING))
//				{
//					sp_ios_battery_level_color_setting();
//				}

/*
typedef enum _ctrl_led_state_e {

    LED_STATE_OFF =0,
    LED_STATE_SOLID_ON,
    LED_STATE_FLASHING,
    LED_STATE_HEARTBEAT
} CTRL_LED_COLOR_STATE_E;
*/

void sp_ble_set_battery_level(int level)
{
	g_battery_level = level;	

	/* Once change the Battey level */
	if (( g_notify_type != SP_NOTIFY_SP_RGB_STATE) && 
           ( g_notify_type != SP_NOTIFY_ANCS_RGB_SETTING))
        {
            	sp_ble_battery_level_color_setting();
	}
}

/*
 * Send the LED State Changes to sp_led_controller 
 */
int sp_ble_send_led_state_message(int rgb_val, int notify_type, int led_time)
{
	CTRL_SC_LED_STATUS_S led_info;
	int led_st;

	if (notify_type == SP_NOTIFY_NONE)
	{
		g_notify_type = SP_NOTIFY_NONE;
		sp_ble_battery_level_color_setting();
	}
	else if (notify_type == SP_NOTIFY_ANCS_CLEARED_STATE)
	{
		g_notify_type = SP_NOTIFY_ANCS_CLEARED_STATE ;
		sp_ble_battery_level_color_setting();
	}
	else 
	{
		if (notify_type == SP_NOTIFY_SP_RGB_STATE)
		{
			g_notify_type     = SP_NOTIFY_SP_RGB_STATE;
			led_st		  = LED_STATE_SOLID_ON ;	
		}
		else if (notify_type == SP_NOTIFY_ANCS_RGB_SETTING) /* if Not RGB Mode then Sett the Color */
		{
			g_notify_type 	=  SP_NOTIFY_ANCS_RGB_SETTING;
			led_st		=  LED_STATE_HEARTBEAT ;	
		}
		led_info.color_val = rgb_val;
		led_info.led_state = led_st;
		led_info.led_time  = led_time;
		ctrl_sendsignal_with_struct(dbus_conn, CTRL_COMMON_INTERFACE,
							   CTRL_SIG_SET_LED_STATE,&led_info, sizeof(led_info));
	}
	return 0;
}

/*
 * Setting LED Color as per current Battery level setting 
 */
int sp_ble_battery_level_color_setting(void)
{
        int rgb_val=0;

        if (g_battery_level != 0xFF)
        {
                /* Change the color here as per the battery level */
                if ( g_battery_level  > IDEVICE_BATTERY_LEVEL_60_PRECENTAGE )
                        rgb_val = GREEN_COLOR_RGB;
                else if (( g_battery_level  >= IDEVICE_BATTERY_LEVEL_30_PRECENTAGE) &&
                            (g_battery_level <= IDEVICE_BATTERY_LEVEL_60_PRECENTAGE))
                        rgb_val = YELLOW_COLOR_RGB;
                else if ( g_battery_level  < IDEVICE_BATTERY_LEVEL_30_PRECENTAGE)
                        rgb_val = ORANGE_COLOR_RGB;

                sp_ble_send_led_state_message(rgb_val, LED_STATE_SOLID_ON, 0x00);
        }
        else
        {
                ctrl_log_print(LOG_INFO, __FUNCTION__,__LINE__,"Battey level not yet Update\n");
        }
        return 0;
}


int main(int argc, char *argv[])
{
    int ret = CTRL_ERROR;
    //for dbus
    //DBusConnection* dbus_conn;
    DBusMessage* dbus_msg = NULL;
    DBusError dbus_err;
    //for dbus end

    //for pthread
    struct sigaction newAction;
    pthread_t pid_serial;
    //for pthread end

    fd_set rfds;

    /* Register ble Controller */
    ret = ctrl_init(SP_BLE_CTRL_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &dbus_conn , &dbus_err);
    if (ret)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "ERROR sys_lib_init() ret[%3d:%s]\n", ret, strerror(ret));
        return ret;
    }

    ret = ctrl_add_signal_into_filter(dbus_conn, CTRL_SIG_SC_IOS_SEND_USB_CONNECTED_INFO, &dbus_err);
    if (ret)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
                       "ERROR add_signal_into_filter(), signal:  CTRL_SIG_SC_IOS_SEND_USB_CONNECTED_INFO \n");
        return ret;
    }

    /* Wait for sync events */
    ret = ctrl_wait_for_service_up(dbus_conn, &dbus_err, CTRL_SERVICE_SYNC_BOOTUP_CFG_DONE);
    if (ret)
    {
        /* If failure, jump to error_reurn lable */
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "ERROR ctrl_wait_for_service_up(), sync: CTRL_SERVICE_SYNC_BOOTUP_CFG_DONE\n");
        goto INIT_EXIT;
    }

    /* Wait for sync events */
    ret = ctrl_wait_for_service_up(dbus_conn, &dbus_err, CTRL_SERVICE_SYNC_BOOTUP_START_CFG); /* config mgr ready */
    if (ret)
    {
        /* If failure, jump to error_reurn lable */
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "ERROR ctrl_wait_for_service_up(), sync: CTRL_SERVICE_SYNC_BOOTUP_START_CFG\n");
        goto INIT_EXIT;
    }

    ret = ctrl_wait_for_service_up(dbus_conn, &dbus_err, CTRL_SERVICE_SYNC_DBUS_READY);
    if (ret)
    {
        /* If failure, jump to error_reurn lable */
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "ERROR ctrl_wait_for_service_up(), sync: CTRL_SERVICE_SYNC_DBUS_READY\n");
        goto INIT_EXIT;
    }

    /* BLE Module Firmware Upgrade */
    ret = ctrl_add_signal_into_filter(dbus_conn, CTRL_SIG_BLE_FIRMWARE_UPGRADE, &dbus_err);
    if (ret)
    {
                /* if failure, jump to error_reurn lable */
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
                "ERROR add_signal_into_filter(), signal:  CTRL_SIG_BLE_FIRMWARE_UPGRADE\n");
                goto INIT_EXIT;
    }
    /* BLE Module firmware upgrade done  */
    ret = ctrl_add_signal_into_filter(dbus_conn, CTRL_SIG_BLE_FIRMWARE_UPGRADE_DONE, &dbus_err);
    if (ret)
    {
                /* if failure, jump to error_reurn lable */
                ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
                "ERROR add_signal_into_filter(), signal:  CTRL_SIG_BLE_FIRMWARE_UPGRADE_DONE\n");
                goto INIT_EXIT;
    }

    /* Initial capi library */
    ret = capi_init();
    if (ret != CAPI_SUCCESS)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__,
                       "ERROR capi_init()\n");
        goto INIT_EXIT;
    }

    // initialize serial, ttyS1
    ret = serial_init();
    if (ret == -1)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Initialize serial(ble) failed, ret = %d\n", ret);
        goto INIT_EXIT;
    }


    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "BLE Controller Start\n");


    /* Ignore broken pipes */
    signal(SIGPIPE, SIG_IGN);

    ret = reset_ble_module(5);
    if (ret != 0)
    {
        ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "Failed to reset ble_module, ret = %d\n", ret);
        goto INIT_EXIT;
    }

    //ask ios_ctrl to get usb cable connected info
    ctrl_sendsignal_with_intvalue(dbus_conn, CTRL_COMMON_INTERFACE, CTRL_SIG_SC_BLE_GET_USB_CONNECTED_INFO, 0);


    /* Enter the while-loop */
    CTRL_SIG_INFO_U sig_info;
    memset(&sig_info, 0, sizeof(sig_info));

    ctrl_log_print(LOG_INFO, __FUNCTION__, __LINE__, "SP_BLE Controller Enter main while loop\n");
    while (1)
    {
        memset(&sig_info, 0, sizeof(sig_info));
        FD_ZERO(&rfds);
        FD_SET(serial_fd, &rfds);

        dbus_msg = dbus_connection_pop_message(dbus_conn);
        if (!dbus_msg) {
            ctrl_process_user_fd_event(serial_fd + 1, &rfds, dbus_conn, &dbus_err, read_app_event);
            dbus_connection_read_write(dbus_conn, 100);
            continue;
        }


        // check this is a method call for the right interface & method
        if (dbus_message_is_method_call(dbus_msg, CTRL_COMMON_INTERFACE, CTRL_METHOD_DEBUG)) 
	{
            sp_ble_ctrl_debug_call_back(dbus_conn, dbus_msg);
            ctrl_log_print(LOG_ERR, __FUNCTION__, __LINE__, "sp_ble_ctrl start!! + bb_ctrl_debug_call_back\n");
        }
        else if (dbus_message_is_signal(dbus_msg, CTRL_COMMON_INTERFACE, CTRL_SIG_TIMER_EXPIRED)) 
	{
            ;

        }
	else if (dbus_message_is_signal(dbus_msg, CTRL_COMMON_INTERFACE, CTRL_SIG_BLE_FIRMWARE_UPGRADE)) 
        {
        	/* TI BLE firmware upgrade start event */
		ctrl_log_print(LOG_INFO,__FUNCTION__,__LINE__,"TI BLE: Firmware upgrade event!!!!\n");

		/* Close serial port */
		serial_uninit();
		 
		/* Enable TI BLE Bootloader backdoor */
		ti_ble_enable_sbl_backdoor(5);

		/* Set global flag */
		g_bootloader_enabled = 1; 
        } 
	else if (dbus_message_is_signal(dbus_msg, CTRL_COMMON_INTERFACE, CTRL_SIG_BLE_FIRMWARE_UPGRADE_DONE)) 
        {
	    //CTRL_SIG_FW_UPGRADE_DONE_INFO_S* fw_info =  (CTRL_SIG_FW_UPGRADE_DONE_INFO_S *) &sig_info;
	
            /* Firmware upgrade type TI BLE module then */
	    //if (fw_info->upgrade_type == FW_BLE_PACKAGE)	
	    {
             	/* TI BLE firmware upgrade done event */
             	ctrl_log_print(LOG_INFO,__FUNCTION__,__LINE__,"TI BLE: Firmware upgrade done!\n");
            
             	/* Disable TI BLE Bootloader backdoor */
             	ti_ble_disable_sbl_backdoor(5);

             	/* Set global flag */
             	g_bootloader_enabled = 0;

	     	/* Reset the TI BLE module */
	     	reset_ble_module(5);

	     	/* Open Serial port */
	     	serial_init();	
	    }
        } 
 	else 
	{
            ctrl_log_print(LOG_DEBUG, __FUNCTION__, __LINE__, "sp_ble_ctrl loop if else!!!\n");
        }


        if (NULL != dbus_msg) {
            dbus_message_unref(dbus_msg);
            dbus_msg = NULL;
        }

    } /* end of while(1) */


INIT_EXIT:
    serial_uninit();
    capi_uninit();
    if (NULL != dbus_msg)
        dbus_message_unref(dbus_msg);

    if (NULL != dbus_conn)
        dbus_connection_unref(dbus_conn);

}


