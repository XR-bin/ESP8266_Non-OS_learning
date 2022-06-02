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

//���ô�������ļ�
#include "driver/uart.h"
#include "driver/uart_register.h"
//���Ź�����ļ�
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


u8 F_LED = 0;  //LED��״̬
// �Լ�д��Ӳ����ʱ���жϻص�����
void HW_Timer_INT(void)
{
	F_LED = !F_LED;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), F_LED);
	os_printf("\r\n-----Hello HW_Timer-----\r\n");
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
	//��ʼ������
	uart_init(9600, 9600);    //��һ�������Ǵ���0�Ĳ����ʣ� �ڶ����Ǵ���1�Ĳ�����

	os_delay_us(1000);   //��ʱ�ȴ������ȶ�

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //���ڴ�ӡSDK�汾
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //����0�����ַ���
	os_printf("\r\n-----------------------------------------------------\r\n");

	// GPIO�ܽŹ���ѡ��---����1:PIN_NAME�ܽ���    ����2:FUNC�ܽŹ���(��Щ�ܽſ��Է��ó��������ܣ�����SPI��IIC)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);   //��GPIO4����Ϊ��ͨIO
	// ����GPIO�ܽ�Ϊ���ģʽ���������ָ����ƽ---����1:gpio_noָ��GPIO��   ����2:�����ƽ
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);   //GPIO4���ģʽ������ߵ�ƽ

	// ע�⣺��PIN_NAME��/��FUNC��/��gpio_no����Ҫ����
	// PIN_NAME       �ܽ�����                PERIPHS_IO_MUX + �ܽ���
	// FUNC           �ܽŹ���               �������-1
	// gpio_no        IO�˿����         GPIO_ID_PIN(IO�˿ں�)

	//��ʼ��Ӳ����ʱ��
	//����1��ѡ���ж�Դ       ����2���Ƿ��ظ���ʱ
	//�ж�Դѡ��
	//    0 --- FRC1�ж�Դ   ��ʱ���޷��������ISR(�ж�)
	//    1 --- NMI�ж�Դ   ��ʱ��Ϊ������ȼ������Դ������ISR(�ж�)
	hw_timer_init(0, 1);
	//ע���жϻص�����
	hw_timer_set_func(HW_Timer_INT);
	//���ö�ʱʱ��
	hw_timer_arm(500000);    //��λ��us����������<=1 677 721us

	while(1)
	{
		system_soft_wdt_feed();  //ι������
	}
}
