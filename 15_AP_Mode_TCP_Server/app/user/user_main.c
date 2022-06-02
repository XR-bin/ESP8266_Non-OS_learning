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
	os_printf("\r\n--------------- 成功发送数据 ---------------\r\n");
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
	os_printf("\r\nESP8266_Receive_Data = %s\r\n",pdata);		//串口打印接收到的数据

	os_printf("发送方的信息\r\n");
	os_printf("端口：%d\r\n", T_arg->proto.tcp->remote_port);
	os_printf("IP：%d.%d.%d.%d\r\n",
			T_arg->proto.tcp->remote_ip[0],
			T_arg->proto.tcp->remote_ip[1],
			T_arg->proto.tcp->remote_ip[2],
			T_arg->proto.tcp->remote_ip[3]);

	//向对方发送应答
	espconn_send(T_arg,"ESP8266_WIFI_Recv_OK",os_strlen("ESP8266_WIFI_Recv_OK"));
}

//成功断开回调函数
void ICACHE_FLASH_ATTR ESP8266_TCP_Disconnect(void *arg)
{
	os_printf("\r\n--------------- 成功正常断开 ---------------\r\n");
}

//连接成功回调函数
void ICACHE_FLASH_ATTR ESP8266_WIFI_Connectcb(void *arg)
{
	espconn_regist_sentcb((struct espconn *)arg, ESP8266_WIFI_Send);			// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb((struct espconn *)arg, ESP8266_WIFI_Recv);			// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb((struct espconn *)arg,ESP8266_TCP_Disconnect);	    // 注册成功断开TCP连接的回调函数

	os_printf("\r\n--------------- 成功连接 ---------------\r\n");
}

//异常断开回调函数
void ICACHE_FLASH_ATTR ESP8266_WIFI_Reconcb(void *arg,sint8 err)
{
	os_printf("\r\n--------------- 异常断开%d ---------------\r\n", err);
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

//初始化网络连接(TCP通信)
void ICACHE_FLASH_ATTR ESP8266_TCP_Init(void)
{
	// ESPCONN_UDP:0x20---UDP协议
	// ESPCONN_TCP:0x10---TCP协议
	ST_NetCon.type = ESPCONN_TCP;		// 通信协议：TCP
	ST_NetCon.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));	// TCP协议指针申请内存
	// 此处无需设置目标IP/端口(ESP8266作为Server，不需要预先知道Client的IP/端口)
	ST_NetCon.proto.tcp->local_port  = 8266;		// 设置本地端口

	//注册回调函数
	espconn_regist_connectcb(&ST_NetCon, ESP8266_WIFI_Connectcb);	// 注册TCP连接成功的回调函数
	espconn_regist_reconcb(&ST_NetCon, ESP8266_WIFI_Reconcb);	    // 注册TCP异常断开的回调函数

	// 创建TCP_server，建立侦听
	espconn_accept(&ST_NetCon);

	//正式初始化TCP通信
	//参数1---对应的网络连接的结构体
	//参数2---超时时间，单位: 秒，最大值: 7200秒
	//【注意】如果超时时间设置为 0，ESP8266_TCP_server将始终不会断开已经不与它通信的TCP_client，不建议这样使用。
	//参数3---0:对所有TCP连接生效      1:对某个TCP连接生效
	espconn_regist_time(&ST_NetCon, 300, 0);
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
	u8  ESP8266_IP[4];		        // 点分十进制形式保存IP

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

	// 获取ESP8266_AP模式下的IP地址
	//【AP模式下，如果开启DHCP(默认开启的)，并且未设置IP相关参数，ESP8266的IP地址=192.168.4.1】
	// SOFTAP_IF 0x01 --- 获取的是AP模式的IP信息
	wifi_get_ip_info(SOFTAP_IF,&ST_ESP8266_IP);	// 参数2：IP信息结构体指针

	if(ST_ESP8266_IP.ip.addr != 0)
	{
		// 将二进制IP地址改为十进制
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// 点分十进制IP的第一个数 <==> addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// 点分十进制IP的第二个数 <==> addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// 点分十进制IP的第三个数 <==> addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// 点分十进制IP的第四个数 <==> addr高八位

		// 打印IP地址
		os_printf("ESP8266_IP = %d.%d.%d.%d\r\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
		// 打印连接设备的数量
		os_printf("Number of devices connected to this WIFI = %d\r\n",wifi_softap_get_station_num());

		os_timer_disarm(&OS_Timer1);	//关闭定时器

		ESP8266_TCP_Init();   //初始化网络连接(TCP通信)

		os_printf("\r\n网络初始化完成\r\n");
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

//初始化ESP8266的AP模式
void ICACHE_FLASH_ATTR ESP8266_AP_Init(void)
{
	/*****************************************
	 * struct softap_config结构体成员
	 * 		      ssid:  WiFi的SSID(服务集标识)，通俗点就是WiFi名
	 *        password:  WiFi的密码
	 *        ssid_len:  WiFi的SSID长度
	 *         channel:  通信号1~13
	 *        authmode:  是否设置加密模式
	 *     ssid_hidden:  是否隐藏SSID
	 *  max_connection:  最大连接的设备数量，最大是4台
	 * beacon_interval:  信标间隔时槽100～60000 ms
	 ****************************************/
	struct softap_config AP_Config;     //定义一个AP参数结构体

	wifi_set_opmode(0x02);    //WiFi设置为AP模式，并保存到flash

	// 初始化AP_Config结构体(清零操作)
	os_memset(&AP_Config, 0, sizeof(struct softap_config));

	// 设置WiFi的AP模式参数
	os_strcpy(AP_Config.ssid, "My_ESP8266");   //WiFi名为My_ESP8266
	os_strcpy(AP_Config.password,"123456789"); //WiFi密码为123456789
	AP_Config.ssid_len = 9;                    //密码长度为9
	AP_Config.channel=1;                       // 通道号1
	AP_Config.authmode=AUTH_WPA2_PSK;          // 设置加密模式
	AP_Config.ssid_hidden=0;                   // 不隐藏SSID
	AP_Config.max_connection=4;                // 最大连接数4台设备
	AP_Config.beacon_interval=100;             // 信标间隔时槽100 ms

	// 这个函数并不会立即设置，它有点类似于任务，只有空闲时才进行设置
	wifi_softap_set_config(&AP_Config);				// 设置soft-AP，并保存到Flash
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

	//设置WiFi为AP模式，并设置其AP模式参数
	ESP8266_AP_Init();

	//软件定时器初始化  1s
	OS_Timer1_Init(1000, 1);
}

