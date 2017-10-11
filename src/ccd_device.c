
#include "platform_os.h"
#include "device.h"
#include "ccd_device.h"


static uint8 ccd_uart_makecrc(uint8 *data, uint8 length)
{
	uint8 i = 0;
	int crc32 = 0;
	uint8 crc = 0;

	for (i = 0; i < length; i++) {
		crc32 += data[i];
	}
	crc = crc32 & 0x000000FF;
	crc = ~crc;
	return crc + 1;
}

//读取一帧数据的头部并校验
EG_RETURN com_uart_head_check(uint8 *p_head,int len)
{
		EG_RETURN ret = EG_FAIL;
		if ((len == 1) && (p_head[1] == 0xAA)){
			ret = EG_OK;	
		}
		return ret;
}


//校验一帧的长度部分做校验并返回帧长
int com_uart_long_check(uint8 *p_long,int len)
{
	int dataLen = 0;
	if (len == 4)
	{
		dataLen = (p_long[2] | p_long[3] << 8);
		if(dataLen < CONFIG_UART_RECIVE_ARRAY_MAX)
		{
			return dataLen-len;
		}
	}
	return -EG_FAIL;
}



//20 process


//AA (head)   02 01 0F 00(long)    05(msg type) 0A 05 02 ,00 05 00 07 00 00 (data),CC(crc)
//crc  校验的是从第二个字节到倒数第二个字节，注意没有包含头哦
EG_RETURN com_uart_mess_process(uint8 *p_mess,int len)
{
	EG_RETURN ret = EG_FAIL;
	//1 check crc
	uint8 crc =  ccd_uart_makecrc(&p_mess[1],len-2);
	if(crc !=p_mess[len-1])
	{
		return EG_ERROR;	
	}

	//2 process msg
	CCD_UART_MSG_TYPE msg_type=p_mess[5];
	UartPacketProcessCB cb = eg_get_uart_cmd_func_cb(msg_type);
	if(cb!=NULL)
	{
		cb(p_mess,len);
		ret = EG_OK;
	}
	else
	{
		eg_log_debug("can't find uart msg type = %02X \r\n",msg_type);
		eg_uart_mess_upload(p_mess,len);
		ret = EG_OK;	
	}
	return ret;
}




//05 process
static uint8 Process_ReadDeviceQosResponseCB(uint8 *f_uart ,int len)
{ 
	return EG_OK;
}

//07 process
static uint8 Process_readDeviceUuidResponseCB(uint8 *f_uart,int len )
{ 
	//store uuid 
	return EG_OK;
}

#if 0
uint8 Process_GetDeviceInfoResponseCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_SettingUartBaudRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_CheckNetworkStatusRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_DeviceSyncSystemTimeRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_queryDeviceVersionResponseCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_judgeDeviceOTAResponseCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_ActiveDeviceOtaResponseCB(uint8 *f_uart,int len){return EG_OK;}
uint8 ProcessWifiPowerRequest(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_RenameWifiSSIDRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_DeviceEasyLinkRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_DeviceRebootRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_DeviceResetToFactoryRequestCB(uint8 *f_uart,int len){return EG_OK;}
uint8 Process_DeviceGetWifiModulePropertyRequestCB(uint8 *f_uart,int len){return EG_OK;}
#endif
void com_regis_uart_cmd_func_cb()
{
  eg_clear_uart_cmd_func_cb();

  
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_READ_QOS_RESPONSE, Process_ReadDeviceQosResponseCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_READ_DEVICESN_RESPONSE, Process_readDeviceUuidResponseCB);
#if 0  
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_READ_DEVICEINFO_RESPONSE, Process_GetDeviceInfoResponseCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_SET_UART_BAUD_RESPONSE, Process_SettingUartBaudRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_CHECK_NETWORK_STATE_REQUEST, Process_CheckNetworkStatusRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_SYNC_SYSTEMTIME_RESPONSE, Process_DeviceSyncSystemTimeRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_QUERY_DEVICE_VERSION_RESPONSE, Process_queryDeviceVersionResponseCB);//0x61 uart?ìó|′|àí
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_QUERY_DEVICE_UPDATE_RESPONSE, Process_judgeDeviceOTAResponseCB);//0x62 uart?ìó|′|àí
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_ACTIVE_DEVICE_UPDATE_RESPONSE, Process_ActiveDeviceOtaResponseCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_WIFI_POWER_REQUEST, ProcessWifiPowerRequest);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_RENAME_WIFISSID_REQUEST, Process_RenameWifiSSIDRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_EASY_LINK_REQUEST, Process_DeviceEasyLinkRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_REBOOT_RESPONSE, Process_DeviceRebootRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_RESET2FACTORY_RESPONSE, Process_DeviceResetToFactoryRequestCB);
  eg_register_uart_cmd_func_cb(UART2WIFICOMMANDS_READ_WIFIMODULE_PROPERTY_REQUEST, Process_DeviceGetWifiModulePropertyRequestCB);
#endif
}



