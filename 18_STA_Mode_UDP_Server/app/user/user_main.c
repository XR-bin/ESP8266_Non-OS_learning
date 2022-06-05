/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

//引用串口相关文件
#include "driver/uart.h"
#include "driver/uart_register.h"
//看门狗相关文件
#include "os_type.h"
#include "osapi.h"
//网络相关头文件
#include "espconn.h"
#include "mem.h"


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
            rf_cal_sec = 512 - 5;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}



void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}

// 成功发送网络数据的回调函数
// 参数1 arg ----- 网络传输结构体espconn指针
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send(void *arg)
{
	os_printf("\r\nESP8266_WIFI_Send_OK\r\n");
}

// 成功接收网络数据的回调函数
// 参数1 arg ----- 网络传输结构体espconn指针
// 参数2 pdata ----- 网络传输数据指针
// 参数3 len ----- 数据长度
void ICACHE_FLASH_ATTR ESP8266_WIFI_Recv(void * arg, char * pdata, unsigned short len)
{
	struct espconn * T_arg = arg;		// 缓存网络连接结构体指针
	/******************************
	 *  remot_info成员
	 *  	      state : 连接状态
	 *  	remote_port : 端口号
	 *  	  remote_ip : IP地址
	 ******************************/
	remot_info * P_port_info = NULL;	//远端连接信息结构体指针

	os_printf("\r\nESP8266_Receive_Data = %s\r\n",pdata);		//串口打印接收到的数据

	// 获取远端信息【UDP通信是无连接的，向远端主机回应时需获取对方的IP/端口信息】
	if(espconn_get_connection_info(T_arg, &P_port_info, 0)==ESPCONN_OK)	//获取远端信息
	{
		T_arg->proto.udp->remote_port  = P_port_info->remote_port;		//获取对方端口号
		os_memcpy(T_arg->proto.udp->remote_ip,P_port_info->remote_ip, 4);	//获取对方IP地址
	}

	os_printf("发送方的信息\r\n");
	os_printf("端口：%d\r\n", P_port_info->remote_port);
	os_printf("IP：%d.%d.%d.%d\r\n",
			P_port_info->remote_ip[0],
			P_port_info->remote_ip[1],
			P_port_info->remote_ip[2],
			P_port_info->remote_ip[3]);

	//向对方发送应答
	espconn_send(T_arg,"ESP8266_WIFI_Recv_OK",os_strlen("ESP8266_WIFI_Recv_OK"));
}

//定义espconn型结构体(网络连接结构体)
/***********************************
 * struct espconn结构体成员
 * 		type         : 协议类型(TCP/UDP)
 * 		state        : 当前连接状态
 * 		proto        : 协议空间指针
 * 		recv_callback: 接收回调函数
 * 		sent_callback: 发送回调函数
 * 		link_cnt     : 连接数量
 * 		reverse      :
 ***********************************/
struct espconn ST_NetCon;	// 注：必须定义为全局变量，内核将会使用此变量(内存)

//初始化网络连接(UDP通信)
void ICACHE_FLASH_ATTR ESP8266_UDP_Init(void)
{
	// ESPCONN_UDP:0x20---UDP协议
	// ESPCONN_TCP:0x10---TCP协议
	ST_NetCon.type = ESPCONN_UDP;		// 通信协议：UDP
	ST_NetCon.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));	// UDP协议指针申请内存
	// 此处无需设置目标IP/端口(ESP8266作为Server，不需要预先知道Client的IP/端口)
	ST_NetCon.proto.udp->local_port  = 8266;		// 设置本地端口

	//注册回调函数
	espconn_regist_sentcb(&ST_NetCon, ESP8266_WIFI_Send);	// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb(&ST_NetCon, ESP8266_WIFI_Recv);	// 注册网络数据接收成功的回调函数

	//正式初始化UDP通信
	espconn_create(&ST_NetCon);	// 初始化UDP通信
}

// 注：OS_Time1必须定义全局变量，因为ESP8266的内核还要使用
os_timer_t OS_Timer1;  //定义软件定时器(os_timer_t类型结构体)

// 软件定时器回调函数(软件定时器不是真的定时器，它是运行在任务中的，所以不是用中断，而是回调函数)
void ICACHE_FLASH_ATTR OS_Timer1_cb(void)
{
	/*************************
	* struct ip_info结构体成员
	* 		ip: IP地址
	* netmask: 网络掩码
	* 	    gw: 网关
	*************************/
	struct ip_info ST_ESP8266_IP;	// IP信息结构体
	u8 ESP8266_IP[4];		        // 点分十进制形式保存IP
	u8 S_WIFI_STA_Connect;			// WIFI接入状态标志

	// 查询ESP8266的工作模式
	switch(wifi_get_opmode())
	{
		//STA模式
		case 0x01:	os_printf("\r\nESP8266_Mode = STA模式\r\n");		break;
		//AP模式
		case 0x02:	os_printf("\r\nESP8266_Mode = AP模式\r\n");		break;
		//STA和AP模式
		case 0x03:	os_printf("\r\nESP8266_Mode = STA和AP模式\r\n");	break;
	}

	//查询STA接入WIFI状态
	/*******************************************************
	*  Station连接状态表
	*        0 == STATION_IDLE -------------- STATION闲置
	*        1 == STATION_CONNECTING -------- 正在连接WIFI
	*        2 == STATION_WRONG_PASSWORD ---- WIFI密码错误
	*        3 == STATION_NO_AP_FOUND ------- 未发现指定WIFI
	*        4 == STATION_CONNECT_FAIL ------ 连接失败
	*        5 == STATION_GOT_IP ------------ 获得IP，连接成功
	*******************************************************/
	S_WIFI_STA_Connect = wifi_station_get_connect_status();

	switch(S_WIFI_STA_Connect)
	{
		case 0 : 	os_printf("\r\nSTATION_IDLE\r\n");				break;
		case 1 : 	os_printf("\r\nSTATION_CONNECTING\r\n");		break;
		case 2 : 	os_printf("\r\nSTATION_WRONG_PASSWORD\r\n");	break;
		case 3 : 	os_printf("\r\nSTATION_NO_AP_FOUND\r\n");		break;
		case 4 : 	os_printf("\r\nSTATION_CONNECT_FAIL\r\n");		break;
		case 5 : 	os_printf("\r\nSTATION_GOT_IP\r\n");			break;
	}

	// 连接成功
	if(S_WIFI_STA_Connect == STATION_GOT_IP)
	{
		// 获取ESP8266_STA模式下的IP地址
		// STATION_IF 0x00 --- 获取的是STA模式的IP信息
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 参数2：IP信息结构体指针

		// 将二进制IP地址改为十进制
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// 点分十进制IP的第一个数 <==> addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// 点分十进制IP的第二个数 <==> addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// 点分十进制IP的第三个数 <==> addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// 点分十进制IP的第四个数 <==> addr高八位

		// 打印IP地址
		os_printf("ESP8266_IP = %d.%d.%d.%d\r\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);

		ESP8266_UDP_Init();   //初始化网络连接(UDP通信)

		os_timer_disarm(&OS_Timer1);	// 关闭定时器
	}
}

// 软件定时器初始化(ms)   参数1：设置定时器时间     参数2：设置是否重复触发
void ICACHE_FLASH_ATTR OS_Timer1_Init(u32 time_ms, u8 time_repetitive)
{
	// 关闭定时器
	os_timer_disarm(&OS_Timer1);   //关闭软件定时器

	// 设置软件定时器
	// 参数1：要设置定时器   参数2：定时器的回调函数   参数3：传给回调函数的参数
	os_timer_setfn(&OS_Timer1, (os_timer_func_t *)OS_Timer1_cb, NULL);

	// 使能定时器
	// 参数1：要使能设置的定时器     参数2：定时时间ms   参数3：1表重复触发  0表只触发一次
	os_timer_arm(&OS_Timer1, time_ms, time_repetitive);

	// 注意：
	//    system_timer_reinit 软件定时器重新初始化
	//    如果未调用system_timer_reinit, 可支持范围5ms ~ 6 870 947ms
	//    如果有调用system_timer_reinit, 可支持范围100ms ~ 428 496ms
}

//初始化ESP8266的STA模式
void ICACHE_FLASH_ATTR ESP8266_STA_Init(void)
{
	/*****************************************
	 * struct station_config结构体成员
	 * 		      ssid:  要接入的WiFi的SSID(服务集标识)，通俗点就是WiFi名
	 *        password:  要接入的WiFi的密码
	 *       bssid_set:  当一个局域网中有多个同名WiFi时使用，即我要连接的WiFi的名字在附近有几个WiFi都是一样的时候
	 *                   0：不校验MAC地址     1：连接WiFi时要校验MAC地址
	 *           bssid:  要连接的WiFi的MAC地址
	 *       threshold:  结构体内容包含要连接的WiFi的加密方式和信号强度结构体
	 ****************************************/
	struct station_config STA_Config;    //定义一个STA参数结构体
	/*************************
	 * struct ip_info结构体成员
	 * 		ip: IP地址
	 * netmask: 网络掩码
	 * 	    gw: 网关
	 *************************/
//	struct ip_info ST_ESP8266_IP;		 //STA信息结构体

	wifi_set_opmode(0x01);    //WiFi设置为STA模式，并保存到flash

	//自定义ESP8266在路由器中的IP
	//如果没有下面的操作则会通过DHCP自动分配IP,一般情况下都是不自己配IP的
	/*
	wifi_station_dhcpc_stop();						// 关闭 DHCP Client
	IP4_ADDR(&ST_ESP8266_IP.ip,192,168,8,88);		// 配置IP地址
	IP4_ADDR(&ST_ESP8266_IP.netmask,255,255,255,0);	// 配置子网掩码
	IP4_ADDR(&ST_ESP8266_IP.gw,192,168,8,1);		// 配置网关地址
	wifi_set_ip_info(STATION_IF,&ST_ESP8266_IP);	// 设置STA模式下的IP地址
	*/

	// 初始化STA_Config结构体(清零操作)
	os_memset(&STA_Config, 0, sizeof(struct station_config));
	// 设置WiFi的STA模式参数
	/*******************************************
	* 【注意】【注意】【注意】【重要的事情说三遍】
	* ssid如果是中文名称，则中文必须是URL编码
	* ssid如果是中文名称，则中文必须是URL编码
	* ssid如果是中文名称，则中文必须是URL编码
	*
	* 下面例子是WiFi名为小米(URL编码为0xe5b08f 0xe7b1b3)
	* \d或者直接\是8进制     \x是16进制
	* *****************************************/
	//WiFi名叫小米
//	os_strcpy(STA_Config.ssid, "\xe5\xb0\x8f\xe7\xb1\xb3");        //将要接入的WiFi名
	//WiFi名叫我家WiFi
	os_strcpy(STA_Config.ssid, "\xe6\x88\x91\xe5\xae\xb6WiFi");        //将要接入的WiFi名
	os_strcpy(STA_Config.password,"13612746918"); //将要接入的WiFi密码

	// 这个函数并不会立即设置，它有点类似于任务，只有空闲时才进行设置
	wifi_station_set_config(&STA_Config);  // 设置STA模式参数，并保存到Flash

	// wifi_station_connect()此API不能在user_init初始化中调用
	// 如果user_init中调用wifi_station_set_config()的话，内核会自动将ESP8266接入WIFI
	// wifi_station_connect();		// ESP8266连接WIFI,这里不需要使用
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
	//初始化串口
	uart_init(9600, 9600);    //第一个参数是串口0的波特率， 第二个是串口1的波特率

	os_delay_us(1000);   //延时等待串口稳定

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //串口打印SDK版本
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //串口0发送字符串
	os_printf("\r\n-----------------------------------------------------\r\n");
	//设置WiFi为STA模式，并设置其STA模式参数
	ESP8266_STA_Init();

	//软件定时器初始化  5s
	OS_Timer1_Init(5000, 1);
}

