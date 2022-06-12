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
//SNTP头文件
#include "sntp.h"


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

// 注：OS_Time1必须定义全局变量，因为ESP8266的内核还要使用
os_timer_t OS_Timer1;  //定义软件定时器(os_timer_t类型结构体)

// 软件定时器回调函数(软件定时器不是真的定时器，它是运行在任务中的，所以不是用中断，而是回调函数)
// 用于SNTP定时获取时间用的
void ICACHE_FLASH_ATTR OS_Timer_SNTP_cb(void *arg)
{
	// 字符串整理 相关变量
	u8 C_Str = 0;				// 字符串字节计数
	char A_Str_Data[20] = {0};	// 【"日期"】字符串数组
	char *T_A_Str_Data = A_Str_Data;	// 缓存数组指针
	char A_Str_Clock[10] = {0};	// 【"时间"】字符串数组
	char * Str_Head_Week;		// 【"星期"】字符串首地址
	char * Str_Head_Month;		// 【"月份"】字符串首地址
	char * Str_Head_Day;		// 【"日数"】字符串首地址
	char * Str_Head_Clock;		// 【"时钟"】字符串首地址
	char * Str_Head_Year;		// 【"年份"】字符串首地址
	uint32	TimeStamp;		// 时间戳
	char * Str_RealTime;	// 实际时间的字符串

	// 查询当前距离基准时间(1970.01.01 00:00:00 GMT+8)的时间戳(单位:秒)
	TimeStamp = sntp_get_current_timestamp();

	if(TimeStamp)		// 判断是否获取到偏移时间
	{
		// 查询实际时间(GMT+8):东八区(北京时间)
		Str_RealTime = sntp_get_real_time(TimeStamp);

		os_printf("\r\n---------------------------------------------\r\n");
		os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		    // 时间戳
		os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// 实际时间
		os_printf("\r\n---------------------------------------------\r\n");
	}
}

// 软件定时器初始化(ms)   参数1：设置定时器时间     参数2：设置是否重复触发
// 用于SNTP定时获取时间用的
void ICACHE_FLASH_ATTR OS_Timer_SNTP_Init(u32 time_ms, u8 time_repetitive)
{
	// 关闭定时器
	os_timer_disarm(&OS_Timer1);   //关闭软件定时器

	// 设置软件定时器
	// 参数1：要设置定时器   参数2：定时器的回调函数   参数3：传给回调函数的参数
	os_timer_setfn(&OS_Timer1, (os_timer_func_t *)OS_Timer_SNTP_cb, NULL);

	// 使能定时器
	// 参数1：要使能设置的定时器     参数2：定时时间ms   参数3：1表重复触发  0表只触发一次
	os_timer_arm(&OS_Timer1, time_ms, time_repetitive);

	// 注意：
	//    system_timer_reinit 软件定时器重新初始化
	//    如果未调用system_timer_reinit, 可支持范围5ms ~ 6 870 947ms
	//    如果有调用system_timer_reinit, 可支持范围100ms ~ 428 496ms
}

//SNTP服务器初始化
void ICACHE_FLASH_ATTR ESP8266_SNTP_Init(void)
{
	//申请一个用于IP地址的空间
	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

	//通过域名设置SNTP服务器
	//us.pool.ntp.org和ntp.sjtu.edu.cn，还有IP地址的210.72.145.44都是网上找的别人SNTP服务器
	//我们连上去使用就行
	//ESP8266最多支持3个SNTP服务器，0号为主服务器，1,2号为备选，但0号不能用时会按顺序使用备选服务器
	//参数1：服务器优先0~3
	//参数2：SNTP域名
	sntp_setservername(0, "us.pool.ntp.org");	// 服务器0号
	sntp_setservername(1, "ntp.sjtu.edu.cn");	// 服务器1号

	//将字符串的IP地址转换为u32为的IP地址
	ipaddr_aton("210.72.145.44", addr);			// 点分十进制 => 32位二进制
	//通过IP地址设置SNTP服务器
	//参数1：服务器优先0~3
	//参数2：IP地址
	sntp_setserver(2, addr);					// 服务器2号

	os_free(addr);								// 释放addr内存空间

	sntp_init();	// SNTP初始化API

	//重新设置软件定时器
	OS_Timer_SNTP_Init(1000,1);
}

// 软件定时器回调函数(软件定时器不是真的定时器，它是运行在任务中的，所以不是用中断，而是回调函数)
// 用于连接WiFi
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

		os_timer_disarm(&OS_Timer1);	// 关闭定时器

		ESP8266_SNTP_Init(); //初始化SNTP
	}
}

// 软件定时器初始化(ms)   参数1：设置定时器时间     参数2：设置是否重复触发
// 用于连接WiFi用的
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

	//软件定时器初始化  3s
	OS_Timer1_Init(3000, 1);
}

