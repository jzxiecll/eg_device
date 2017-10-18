
#ifndef __DEVICE_HAL_H__
#define __DEVICE_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal.h"
#include "hal_uart.h"
#include "platform_os.h"
#include "eg_log.h"



typedef enum
{
  EG_UART0_ID = 0,	                                 /*!< UART0 port define */
  EG_UART1_ID = 1,                                      /*!< UART1 port define */
  EG_UART2_ID,                                      /*!< UART2 port define */
  EG_UART3_ID,                                      /*!< UART3 port define */
}EG_UART_ID_Type;


int eg_device_open();
int eg_device_close();
uint32_t eg_device_read(uint8_t *buf, uint32_t len);
bool eg_device_write(uint8_t *buf, uint32_t len);



#ifdef __cplusplus
		}
#endif
		
#endif 

