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

u8 F_LED = 0; //LED状态
// 注：OS_Time1必须定义全局变量，因为ESP8266的内核还要使用
os_timer_t OS_Timer1;  //定义软件定时器(os_timer_t类型结构体)

// 软件定时器回调函数(软件定时器不是真的定时器，它是运行在任务中的，所以不是用中断，而是回调函数)
void ICACHE_FLASH_ATTR OS_Timer1_cb(void)
{
	F_LED = !F_LED;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), F_LED);
	os_printf("\r\n-----Hello OS_Timer1-----\r\n");
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

	// GPIO管脚功能选择---参数1:PIN_NAME管脚名    参数2:FUNC管脚功能(有些管脚可以服用成其他功能，例如SPI、IIC)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);   //将GPIO4设置为普通IO
	// 设置GPIO管脚为输出模式，并且输出指定电平---参数1:gpio_no指定GPIO口   参数2:输出电平
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);   //GPIO4输出模式，输出高电平

	// 注意：【PIN_NAME】/【FUNC】/【gpio_no】不要混淆
	// PIN_NAME       管脚名称                PERIPHS_IO_MUX + 管脚名
	// FUNC           管脚功能               功能序号-1
	// gpio_no        IO端口序号         GPIO_ID_PIN(IO端口号)

	//软件定时器初始化
	OS_Timer1_Init(500, 1);

	// while(1) system_soft_wdt_feed();  //喂狗操作;

}

