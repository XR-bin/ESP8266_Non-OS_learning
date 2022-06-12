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

char A_JSON_Tree[256] = {0};	// 存放JSON树

/************************************************************************************/
/*                                  JSON树创建相关代码                                                                                                       */
/************************************************************************************/
//要创建的JSON树
#define		JSON_Tree_Format	" { \n "								\
    								" \"Shanghai\": \n "				\
									" { \n "							\
        								" \"temp\": \"%s\", \n "		\
										" \"humid\": \"%s\" \n "		\
    								" }, \n "							\
																		\
									" \"Shenzhen\": \n "				\
									" { \n "							\
    									" \"temp\": \"%s\", \n "		\
										" \"humid\": %s \n "			\
    								" }, \n "							\
																		\
									" \"XXX\": \"%s\" \n "			\
								" } \n "

/*创建JSON树*/
void ICACHE_FLASH_ATTR Setup_JSON_Tree(void)
{
	os_sprintf(A_JSON_Tree, JSON_Tree_Format, "30℃","30","35℃","50","Hello World!");

	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");
	os_printf("%s",A_JSON_Tree);	// 串口打印JSON树
	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");
}
/******************************************************************************/


/************************************************************************************/
/*                                  JSON树解析相关代码                                                                                                       */
/************************************************************************************/
int ICACHE_FLASH_ATTR Parse_JSON_Tree(void)
{
	char A_JSONTree_Value[64] = {0};	// JSON数据缓存数组
	char * T_Pointer_Head = NULL;		// 临时指针
	char * T_Pointer_end = NULL;		// 临时指针
	u8 T_Value_Len = 0;					// 【"值"】的长度

	os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");

	/*Shanghai*/
	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Shanghai\"");		// 【"Shanghai"】
	os_printf("Shanghai:\n");
	/*Shanghai-----temp*/
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t temp:%s\n",A_JSONTree_Value);
	/*Shanghai-----humid*/
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"humid\"");		// 【"humid"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t humid:%s\n",A_JSONTree_Value);


	/*Shenzhen*/
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"Shenzhen\"");		// 【"Shenzhen"】
	os_printf("Shenzhen:\n");
	/*Shenzhen-----temp*/
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t temp:%s\n",A_JSONTree_Value);

	/*Shenzhen-----humid*/
	//【注："humid"键所对应的值是数字。数字同样是由ASSIC码表示，但是没有""】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"humid\"");		// 【"humid"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	// 获取数字的首指针【数字为十进制形式，并且没有""】
	while(*T_Pointer_Head < '0' || *T_Pointer_Head > '9')	// 排除不在【0～9】范围内的字符
		T_Pointer_Head ++ ;
	T_Pointer_end = T_Pointer_Head;	// 设置数字尾指针初值
	// 获取数字的尾指针+1【数字为十进制形式，并且没有""】
	while(*T_Pointer_end >= '0' && *T_Pointer_end <= '9')	// 计算在【0～9】范围内的字符
		T_Pointer_end ++ ;
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值(数字)】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值(数字)】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t humid:%s\n",A_JSONTree_Value);


	/*XXX*/
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"XXX\"");		// 【"result"】
	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
	T_Pointer_Head = os_strstr(T_Pointer_Head, "\"") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(T_Pointer_Head, "\"");				// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value, T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("result:%s\n",A_JSONTree_Value);
	T_Pointer_Head = os_strstr(T_Pointer_Head, "}");			// 【}】

	os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");
	return 0 ;
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

	os_printf("JSON_Tree_Format:\n%s", JSON_Tree_Format);	// 打印JSON树格式

	Setup_JSON_Tree();		// 创建JSON树
	Parse_JSON_Tree();		// 解析JSON树
}

