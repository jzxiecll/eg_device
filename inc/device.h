#ifndef __DEVICE_H__
#define __DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "device_hal.h"

#define CONFIG_UART_RECIVE_ARRAY_MAX  100
#define CONFIG_UART_SEND_ARRAY_MAX  100

#define CONFIG_MAX_UART_COMMANDS_COUNT 20
#define CONFIG_UART_BRIGE_LENTH        10


#define CONFIG_UART_FRAME_HEAD_LENTH  1
#define CONFIG_UART_FRAME_LONG_LENTH  4


typedef  unsigned char  uint8;

typedef enum {
	EG_OK,
	EG_FAIL,
	EG_ERROR
}EG_RETURN;

typedef struct {
	char mac[6];
	char ip[4];
	char rountStatus;
	char cloudStatus;
    char deviceStatus;
}iot_wifi;

/****************************************************/
#define MAX_XXXX_LEN	(20)
typedef struct{
	char sn[MAX_XXXX_LEN];
    char name[MAX_XXXX_LEN];
    char brand[MAX_XXXX_LEN];
    char type[MAX_XXXX_LEN];
    char category[MAX_XXXX_LEN];
    char manufacturer[MAX_XXXX_LEN];
    char version[MAX_XXXX_LEN];
}iot_device_linkinfo;

typedef struct {
	int length;
	int r_cursor;
	int w_cursor;
	void *data;
}eg_queue;

typedef struct {
	eg_queue* s_queue;
	eg_queue* r_queue;	
}iot_brige;

typedef struct{
	iot_brige  *i_brige;
	iot_device_linkinfo i_device_linkinfo;
}iot_device;

typedef struct {
	iot_wifi i_wifi;
	iot_device i_device;
}iot_product;

/************************************************************************************/
typedef struct {
	char* frame;
	char  f_len;
}uart_frame;


typedef enum {
	EG_UART_MSG_HEAD = 1,
	EG_UART_MSG_LENGTH,
	EG_UART_MSG_CONTENT,
}EG_UART_MSG_RECEIVE_STATUS;

typedef unsigned char(*UartPacketProcessCB)(unsigned char *,int);

typedef struct {

  unsigned char commandID;
  unsigned char classID;
  unsigned char length;
  UartPacketProcessCB func_cb;

}UartCommandProcessCB;


#define EG_ASSERT(expr)  { if (!(expr))  return 1; }

void ccd_uart_mess_hex(uint8 *f_uart,int len);
uint8 eg_uart_mess_upload(uint8 *f_uart,int len);
uint8 eg_uart_mess_downctrl(uint8 *f_uart,int len);
uint8 eg_register_uart_cmd_func_cb(uint8 commandID, UartPacketProcessCB func);
UartPacketProcessCB eg_get_uart_cmd_func_cb(uint8 commandID);

int eg_uart_brige_init();
EG_RETURN eg_uart_brige_start();
EG_RETURN eg_uart_brige_stop();



#ifdef __cplusplus
}
#endif

#endif 

