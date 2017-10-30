#include "platform_os.h"
#include "device_hal.h"
#include "device.h"

static UartCommandProcessCB g_uartCommands[CONFIG_MAX_UART_COMMANDS_COUNT];
static unsigned char g_uart_recvData[CONFIG_UART_RECIVE_ARRAY_MAX];
static iot_product g_iot_product;

static eg_thread_t UARTReceiveThread_thread;
static eg_thread_stack_define(UARTReceiveThread_stack, 2048);
static eg_thread_t UARTSendThread_thread;
static eg_thread_stack_define(UARTSendThread_stack, 2048);

void ccd_uart_mess_hex(uint8 *f_uart,int len)
{
		int i = 0;
		EG_P("uart frame :");
		for(i=0;i<len;i++)
		{
			EG_P("%02X ", f_uart[i]);
		}
		EG_P("\r\n");
}


void eg_clear_uart_cmd_func_cb()
{
	uint8 i = 0;
	for (i = 0; i < CONFIG_MAX_UART_COMMANDS_COUNT; i++)
	{
		g_uartCommands[i].commandID = 0x00;
		g_uartCommands[i].func_cb = NULL;
	}
}

UartPacketProcessCB eg_get_uart_cmd_func_cb(uint8 commandID)
{
	uint8 i = 0;
	for (i = 0; i < CONFIG_MAX_UART_COMMANDS_COUNT; i++)
	{
		if (g_uartCommands[i].commandID == commandID)
		{
			return g_uartCommands[i].func_cb;
		}
	}
	return NULL;
}


uint8 eg_register_uart_cmd_func_cb(uint8 commandID, UartPacketProcessCB func)
{
	uint8 i = 0;
	uint8 ret = EG_FAIL;

	for (i = 0; i < CONFIG_MAX_UART_COMMANDS_COUNT; i++)
	{
		if (g_uartCommands[i].commandID == 0x00)
		{
			break;
		}
	}

	if (i < (CONFIG_MAX_UART_COMMANDS_COUNT - 1))
	{
		g_uartCommands[i].commandID = commandID;
		g_uartCommands[i].func_cb = func;
		ret = EG_OK;
	}

	return ret;
}

unsigned char  eg_queue_push(eg_queue *queue, void *data)
{
	EG_ASSERT(queue);
	int w_cursor = 0;
	unsigned char  ret = EG_FAIL;
	w_cursor = (queue->w_cursor + 1) % queue->length;
	if (w_cursor != queue->r_cursor)
	{
		((void **)queue->data)[queue->w_cursor] = data;
		queue->w_cursor = w_cursor;
		ret = EG_OK;
	}
	return ret;

}

unsigned char eg_queue_pop(eg_queue *queue, void **data)
{
	EG_ASSERT(queue);
	unsigned char ret = EG_FAIL;
	if (queue->r_cursor != queue->w_cursor)
	{
		*data = ((void **)queue->data)[queue->r_cursor];
		queue->r_cursor = (queue->r_cursor + 1) % queue->length;
		ret = EG_OK;
	}
	return ret;
}

unsigned char eg_brige_recycle(iot_brige* r_brige)
{
		eg_mem_free(r_brige->s_queue);
		eg_mem_free(r_brige->r_queue);
		eg_mem_free(r_brige);
		r_brige = NULL;
		return 0;
}

unsigned  char eg_set_brige_length(iot_brige* i_brige, int length)
{
	unsigned  char s_ret,r_ret = EG_FAIL;
	i_brige->s_queue->data = eg_mem_alloc(sizeof(void *)* length);
	i_brige->r_queue->data = eg_mem_alloc(sizeof(void *)* length);

	if(i_brige->s_queue->data==NULL || i_brige->r_queue->data ==NULL)
	{
		eg_mem_free(i_brige->s_queue->data);
		eg_mem_free(i_brige->r_queue->data);
		
	}else
	{
		i_brige->s_queue->r_cursor = 0;
		i_brige->s_queue->w_cursor = 0;
		i_brige->s_queue->length = length;
		s_ret = EG_OK;

		i_brige->r_queue->r_cursor = 0;
		i_brige->r_queue->w_cursor = 0;
		i_brige->r_queue->length = length;
		r_ret = EG_OK;
	}
	
	return s_ret+r_ret;
}

iot_brige *eg_brige_create(unsigned char c_len)
{
	iot_brige  *i_brige=NULL;
	i_brige = (iot_brige*)eg_mem_alloc(sizeof(iot_brige));

	if(!i_brige)
	{
		return i_brige;
	}else
	{
		i_brige->s_queue = (eg_queue*)eg_mem_alloc(sizeof(eg_queue));
		i_brige->r_queue = (eg_queue*)eg_mem_alloc(sizeof(eg_queue));
		if(i_brige->s_queue==NULL ||i_brige->r_queue == NULL)
		{
			eg_brige_recycle(i_brige);
			i_brige = NULL;

		}else
		{
			if(eg_set_brige_length(i_brige,c_len)!=0)
			{
				eg_brige_recycle(i_brige);
				i_brige = NULL;
			}
		}

	}
	return i_brige;
}



uart_frame *create_uart_frame(char *buffer,int c_len)
{
	uart_frame *r_frame=(uart_frame*)eg_mem_alloc(sizeof(uart_frame));
	if(!r_frame)
	{
		return r_frame;
	}
	r_frame->frame=(char*)eg_mem_alloc(c_len);
	if(!r_frame->frame)
	{
		eg_mem_free(r_frame);
		r_frame = NULL;
	}else{
		r_frame->f_len = c_len;
		eg_memcpy(r_frame->frame,buffer,c_len);
	}

	return r_frame;
}

int relase_uart_frame(char *buffer,uart_frame *u_frame)
{

	int r_len=0;
	eg_memcpy(buffer,u_frame->frame,u_frame->f_len);
	r_len = u_frame->f_len;
	return r_len;
}

void free_uart_frame(uart_frame *u_frame)
{
	if(u_frame)
	{
		if(u_frame->frame)
			eg_mem_free(u_frame->frame);
		eg_mem_free(u_frame);
		u_frame = NULL;
	}
}

static int common_uart_read_msg(int u_headLenth,int u_lenglenth)
{
	EG_UART_MSG_RECEIVE_STATUS		receiveStatus = EG_UART_MSG_HEAD;
	eg_memset(g_uart_recvData,0,sizeof(g_uart_recvData));
	int len = 0;
	int alreadyReadCnt = 0;
	unsigned char *headArray = (unsigned char *)g_uart_recvData;
	unsigned char *lengArray = (unsigned char *)(g_uart_recvData+u_headLenth);
	unsigned char *messArray = (unsigned char *)(g_uart_recvData+u_headLenth+u_lenglenth);
	unsigned char messageContentLength = 0x00;
	int dataLen = 0;
	unsigned char crc = 0;

	unsigned char byte_recv = 0;
	while(1)
	{

#if 1	
			switch(receiveStatus)
			{

					case EG_UART_MSG_HEAD:
					{
						alreadyReadCnt = eg_device_read((uint8*)headArray + len, u_headLenth- len);
						len += alreadyReadCnt;
						if (len == u_headLenth)
						{
							len = 0;
							if(com_uart_head_check(headArray,u_headLenth)==EG_OK)
							{
								receiveStatus = EG_UART_MSG_LENGTH;
							}
							else{
								eg_memset(g_uart_recvData,0,sizeof(g_uart_recvData));
							}
					
						}
						break;
					}

					case EG_UART_MSG_LENGTH:
					{
						alreadyReadCnt = eg_device_read(lengArray + len, u_lenglenth - len);
						len += alreadyReadCnt;
						if (len == u_lenglenth) {

							messageContentLength = com_uart_long_check(lengArray,u_lenglenth);
							if(messageContentLength < 0 )
							{
								receiveStatus = EG_UART_MSG_HEAD;
								eg_memset(g_uart_recvData,0,sizeof(g_uart_recvData));
							}else{	
								receiveStatus = EG_UART_MSG_CONTENT;
							}
							len = 0;	
							
						}
						break;

					}
					case EG_UART_MSG_CONTENT:
					{
						alreadyReadCnt = eg_device_read( messArray + len, messageContentLength - len);			
						len += alreadyReadCnt;
						if (len == messageContentLength) 
						{
							com_uart_mess_process(g_uart_recvData,u_headLenth+u_lenglenth+messageContentLength);		
							eg_memset(g_uart_recvData,0,sizeof(g_uart_recvData));
							receiveStatus = EG_UART_MSG_HEAD;	
							len = 0;
						}
						break;
					}
			}
#else
			
		   	if(eg_device_read((unsigned char *)&byte_recv, 1)==1)
				EG_P("%02x:  ",byte_recv);
					
#endif
			eg_thread_sleep(eg_msec_to_ticks(10));
		}
}


//收集数据
static void eg_uart_recv_thread()
{
	common_uart_read_msg(CONFIG_UART_FRAME_HEAD_LENTH,CONFIG_UART_FRAME_LONG_LENTH);
}


//分发数据
static void eg_uart_send_thread()
{

	char buffer[CONFIG_UART_SEND_ARRAY_MAX];
	int len;
	bool ret = false;
	uart_frame *uart_frame=NULL;
	while(1)
	{
		uart_frame=NULL;
		len = 0 ;
		eg_memset(buffer,0,sizeof(buffer));
		if(eg_queue_pop(g_iot_product.i_device.i_brige->s_queue,(void**)&uart_frame)==EG_OK)
		{
			
			len = relase_uart_frame(buffer,uart_frame);	
			ret = eg_device_write(buffer,len);
			if(ret==true)
				eg_log_debug("send a uart frame success!");
			else
				eg_log_debug("send a uart frame failed!");
		}
		free_uart_frame(uart_frame);
		eg_thread_sleep(eg_msec_to_ticks(1000));
	}

}


int eg_uart_brige_init()
{
	//I init hardware uart
	if(eg_device_open()!=0)
	{
		return -1;
	}

	//II create brige
	g_iot_product.i_device.i_brige = eg_brige_create(CONFIG_UART_BRIGE_LENTH);
	
	// III register callback	
	com_regis_uart_cmd_func_cb();
	return 0;
}
EG_RETURN eg_uart_brige_start()
{
	int ret1,ret2 = 0;
	ret1 = eg_thread_create(&UARTReceiveThread_thread,
		"eg_uart_recv_thread",
		(void *)eg_uart_recv_thread, 0,
		&UARTReceiveThread_stack, 1);

	if (ret1!=0){
		eg_log_error("unable to create UARTReceiveThread.\r\n");
	}
	else{
		printf(" create UARTReceiveThread success1 \r\n");
	}

	ret2 = eg_thread_create(&UARTSendThread_thread,
		"eg_uart_send_thread",
		(void *)eg_uart_send_thread, 0,
		&UARTSendThread_stack, 1);

	if (ret2!=0){		
		eg_log_error(" unable to create UARTSendThread.\r\n");
	}else{
		printf(" create UARTSendThread success2 \r\n");
	}

	return ret1|ret2;
}

EG_RETURN eg_uart_brige_stop()
{
	if(eg_device_close()!=0)
	{
		return -1;
	}
}


//需要透传到云端的数据
uint8 eg_uart_mess_upload(uint8 *f_uart,int len)
{
	ccd_uart_mess_hex(f_uart,len);
	//放到发送list里面去
	uart_frame *u_frame= create_uart_frame(f_uart, len);
	if(u_frame)
	{
		if(eg_queue_push(g_iot_product.i_device.i_brige->r_queue,u_frame)!=EG_OK)
		{
			eg_log_debug("r_queue push failed");
			free_uart_frame(u_frame);
		}
	}
	return EG_OK;
}



//从云端接收的数据，需要通过串口发送到家电
uint8 eg_uart_mess_downctrl(uint8 *f_uart,int len)
{
	ccd_uart_mess_hex(f_uart,len);
	//放到发送list里面去
	uart_frame *u_frame= create_uart_frame(f_uart, len);
	if(u_frame)
	{
		if(eg_queue_push(g_iot_product.i_device.i_brige->s_queue,u_frame)!=EG_OK)
		{
			eg_log_debug("s_queue push failed");
			free_uart_frame(u_frame);
			return EG_FAIL;
		}
	}else
	{
		return EG_FAIL;
	}
	return EG_OK;
}

