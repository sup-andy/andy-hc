#ifndef _ZWAVE_UART_H_
#define _ZWAVE_UART_H_

extern int g_zw_uart_fd;

extern int ZW_UART_open(char *dev);
extern int ZW_UART_tx(BYTE *data, int len);
extern void *ZW_UART_rx(void *args);
#endif
