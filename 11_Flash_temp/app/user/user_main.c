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

//自己写的ms延时函数
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{
	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

//【注意】扇区编号 != 扇区地址          扇区地址 = 扇区编号 * 4096
u16 FLASH_SEC = 0x77;     //存储数据的扇区编号
u32 W_Data[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};   //要写入Flash的数据
u32 R_Data[16] = {0};   //用来接收Flash的数据

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
	u8 i;

	//初始化串口
	uart_init(9600, 9600);    //第一个参数是串口0的波特率， 第二个是串口1的波特率

	os_delay_us(1000);   //延时等待串口稳定

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //串口打印SDK版本
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //串口0发送字符串
	os_printf("\r\n-----------------------------------------------------\r\n");

	/*******************************************************************************
	 * ESP-12F模组的外部Flash，除了存储系统程序、系统参数外，还可以用来存储用户数据，复位/掉电也不会丢失用户数据
	 *
	 * ESP-12F模组的外部Flash = 32Mbit = 4MB
	 * 地址范围为 0x000 000 ~ 0x3ff fff
	 * 扇区编号为 0x000 000 ~ 0x3ff
	 * 扇区地址 = 扇区编号 *4096
	 * 一个扇区有4KB大小
	 *
	 * 注意：
	 * 	读写Flash的地址，不能与系统程序区发生冲突，可以放在0x70 000地址之后
	 * 	Flash读写，必须4字节对齐
	 * 	Flash写数据时，必须先擦除扇区数据
	 *****************************************************************************/

	// 擦除0x77 000 地址扇区
	// 参数---扇区编号      扇区地址 = 扇区编号 * 4096
	spi_flash_erase_sector(0x77);
	// 向0x77 000 地址扇区写入数据
	// 参数1-要写入的扇区地址     参数2-要写入的数据开始地址    参数3-要写入的数据个数
	spi_flash_write(FLASH_SEC*4096, (u32 *)W_Data, sizeof(W_Data));

	os_printf("\r\n------数据写入flash完成------\r\n");

	// 从 0x77 000 扇区地址中，读取16个数据
	// 参数1-要读取的扇区地址     参数2-要接收数据的缓存开始地址      参数3-读取数据的个数
	spi_flash_read(FLASH_SEC*4096, (u32 *)R_Data, sizeof(R_Data));

	for(i=0; i<16; i++)
	{
		os_printf("Read Data%d = %d\r\n", i, R_Data[i]);
		delay_ms(100);
	}
}

