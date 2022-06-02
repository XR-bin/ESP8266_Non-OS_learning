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
//申请内存malloc的相关文件
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

// 自己写的延时ms函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{
	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

//定义消息队列深度宏
#define   MESSAGE_QUEUE_LEN   2   //消息队列深度(对于同一个任务，系统最多接受的叠加任务数)
//【注意】ESP8266无操作系统Non-OS最多只能有3个任务

//定义任务指针结构体
os_event_t *Pointer_Task1;

//任务执行函数(【注意】形参类型必须为os_event_t *)
void Func_Task1(os_event_t *Task_message)
{
	// os_event_t结构体只有两个参数，sig=消息类型   par=消息参数
	os_printf("\r\n消息类型=%d，消息参数=%c\r\n", Task_message->sig, Task_message->par);
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
	u8 C_Task = 0;           //用于计数任务中消息队列数量
	u8 Message_Type = 1;     //消息类型
	u8 Message_Para = 'A';   //消息参数

	//初始化串口
	uart_init(9600, 9600);    //第一个参数是串口0的波特率， 第二个是串口1的波特率

	os_delay_us(1000);   //延时等待串口稳定

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //串口打印SDK版本
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //串口0发送字符串
	os_printf("\r\n-----------------------------------------------------\r\n");

	// 给任务1申请内存空间(任务1空间 = 1个队列空间*队列数)
	Pointer_Task1 = (os_event_t *)os_malloc((sizeof(os_event_t) * MESSAGE_QUEUE_LEN));
	// 创建任务
	// 参数1-任务执行函数      参数2-任务优先等级    参数3-任务空间指针   参数4-消息队列深度
	// 【注意】任务优先等级  2>1>0
	system_os_task(Func_Task1, USER_TASK_PRIO_0, Pointer_Task1, MESSAGE_QUEUE_LEN);

	for(; C_Task<4; C_Task++)
	{
		system_soft_wdt_feed();  //喂狗操作
		delay_ms(1000);
		os_printf("\r\n安排任务：Task==%d\r\n", C_Task);

		// 给系统安排任务
		// 参数1-任务优先等级(可以来区分任务)  参数2-消息类型     参数3-消息参数
		// 无操作系统Non-os最多就3个任务
		system_os_post(USER_TASK_PRIO_0, Message_Type++, Message_Para++);
	}

	os_printf("\r\n------安排好任务了------\r\n");
}

