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
//JSON相关头文件
#include "user_json.h"
#include "c_types.h"


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

//函数声明
LOCAL int ICACHE_FLASH_ATTR JSON_AssignValue(struct jsontree_context *js_ctx);
int ICACHE_FLASH_ATTR JSON_Tree_Parse(struct jsontree_context *js_ctx, struct jsonparse_state *parser);

// 设置【创建JSON树】、【解析JSON树】的回调函数
//【如果觉得放在一个赋"值"函数中麻烦，可以定义多个JSON复制函数】
struct jsontree_callback JSON_Tree_Set = JSONTREE_CALLBACK(JSON_AssignValue, JSON_Tree_Parse);	// 结构体定义
char A_JSON_Tree[256] = {0};	// 存放JSON树

/************************************************************************************/
/*                                  JSON树创建相关代码                                                                                                       */
/************************************************************************************/
//【jsontree_path_name(...)：		获取JSON树对应深度的【"键"】的指针】
//【jsontree_write_string(...)：	赋值JSON树对应深度的【"键"】(以字符串形式赋值)】
//【jsontree_write_atom(...)：	赋值JSON树对应深度的【"键"】(以数值的形式赋值)】
/*创建JSON树的回调函数：JSON树赋值*/
LOCAL int ICACHE_FLASH_ATTR JSON_AssignValue(struct jsontree_context *js_ctx)
{
	//获取JSON树对应深度的【"键"】的指针，我们要获取当前层的键值对的键名
	const char *P_Key_current = jsontree_path_name(js_ctx,js_ctx->depth-1);	// 获取指向当前【"键"】的指针

	//判断目标键，并给我想要的目标键附上想要得键值
	if(os_strncmp(P_Key_current, "XXX", os_strlen("XXX")) == 0 )			// 判断当前【"键"】 ?= "result"
	{
		//赋值JSON树对应深度的【"键"】(以字符串形式赋值)
		jsontree_write_string(js_ctx,"Hello World!");	// 将【"值"="Shenzhen is too hot!"】写入对应位置
	}
	else
	{
		//获取JSON树对应深度的【"键"】的指针，我们要获取当前层的键值对的上一层键名
		const char *P_Key_upper = jsontree_path_name(js_ctx,js_ctx->depth-2);

		//判断当上一层的键名是否为"Shanghai"
		if(os_strncmp(P_Key_upper, "Shanghai", os_strlen("Shanghai")) == 0)
		{
			//判断当前层的键名是否是目标键名
			if(os_strncmp(P_Key_current, "temp", os_strlen("temp")) == 0)
			{
				//赋值JSON树对应深度的【"键"】(以字符串形式赋值)
				jsontree_write_string(js_ctx,"30℃");
			}
			if(os_strncmp(P_Key_current, "humid", os_strlen("humid")) == 0)
			{
				//赋值JSON树对应深度的【"键"】(以数值的形式赋值)
				jsontree_write_atom(js_ctx,"30");
			}

		}
		//判断当上一层的键名是否为"Shenzhen"
		else if(os_strncmp(P_Key_upper, "Shenzhen", os_strlen("Shenzhen")) == 0 )
		{
			if(os_strncmp(P_Key_current, "temp", os_strlen("temp")) == 0)
			{
				jsontree_write_string(js_ctx,"35℃");
			}

			if(os_strncmp(P_Key_current, "humid", os_strlen("humid")) == 0)
			{
				jsontree_write_atom(js_ctx,"50");
			}
		}
	}

	return 0;
}

/*******************************************
* 创建第三层一个对象名为V_Key_1，
* 第三层有两个键值对分别是
* 键名为temp，键值为JSON_Tree_Set
* 键名为humid，键值为JSON_Tree_Set
******************************************/
JSONTREE_OBJECT(V_Key_1,
				JSONTREE_PAIR("temp", &JSON_Tree_Set),
				JSONTREE_PAIR("humid", &JSON_Tree_Set));	// 设置【"键":"值"】
JSONTREE_OBJECT(V_Key_2,
				JSONTREE_PAIR("temp", &JSON_Tree_Set),
				JSONTREE_PAIR("humid", &JSON_Tree_Set));	// 设置【"键":"值"】

/*******************************************
* 创建第二层一个对象名为V_JSON，
* 第二层有三个键值对分别是
* 键名为Shanghai，键值为V_Key_1
* 键名为Shenzhen，键值为V_Key_2
* 键名为guangzhou，键值为V_Key_3
******************************************/
JSONTREE_OBJECT(V_JSON,
				JSONTREE_PAIR("Shanghai", &V_Key_1),
				JSONTREE_PAIR("Shenzhen", &V_Key_2),
				JSONTREE_PAIR("XXX", &JSON_Tree_Set));		// 设置【"键":"值"】


//JSONTREE_OBJECT( 参数1, 参数2, 参数3, ... )
//参数1：生成的JSON树对象的名称
//参数2~n：键值对
//【注意】：最外层的JSON对象名、"键"都不显示。只显示：{ &V_JSON }
/*创建最外层(第一层)一个对象名为Object_JOSN(不显示)，最外层(第一层)键名为V_JOSN(不显示)，最外层(第一层)键值为V_JSON*/
JSONTREE_OBJECT(Object_JOSN, JSONTREE_PAIR("K_JOSN", &V_JSON));		// 生成JOSN对象

/*创建JSON树*/
void ICACHE_FLASH_ATTR Setup_JSON_Tree(void)
{
	//创建JSON树
	//参数1---首JSON对象指针
	//参数2---首JSON对象的键
	//参数3---JSON树缓存指针
	json_ws_send((struct jsontree_value *)&Object_JOSN, "K_JOSN", A_JSON_Tree);

	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");

	os_printf("%s",A_JSON_Tree);	// 串口打印JSON树

	os_printf("\r\n-------------------- 创建JSON树 -------------------\r\n");
}
/************************************************************************************/



/************************************************************************************/
/*                                  JSON树解析相关代码                                                                                                       */
/************************************************************************************/
/*解析JSON树的回调函数*/
int ICACHE_FLASH_ATTR JSON_Tree_Parse(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
	int type;						// 字符类型
	char buffer[64] = {0};			// 缓存【值】的数组

	os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");

	//jsonparse_next()---作用：解析JSON格式下的一个元素
	//【注意】：每调用一次jsonparse_next(),parser都会改变
	type = jsonparse_next(parser);	// 【{】：JSON对象的框架首字符
	os_printf("%c\n", type);

	//【注意】解析到的是一个键名，则返回'N'
	type = jsonparse_next(parser);	// 【键N】：第一个顶级键【Shanghai】
	if(type != 0)
	{
		// 判断解析的是不是键值
		if(type == JSON_TYPE_PAIR_NAME)
		{
			//判断键名是不是"Shanghai"
			if(jsonparse_strcmp_value(parser, "Shanghai") == 0)
			{
				os_printf("\t Shanghai{2} \n");
				type = jsonparse_next(parser);		// 【:】
				type = jsonparse_next(parser);		// 【{】
				type = jsonparse_next(parser);  	// 【键N】

				if (jsonparse_strcmp_value(parser, "temp") == 0)	// 【temp】
				{
					type = jsonparse_next(parser);		// 【:】
					type = jsonparse_next(parser);  	// 【"】
					if (type == JSON_TYPE_STRING)		// 判断是否是【"】
					{
						jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
						os_printf("\t\t temp: %s\n", buffer);		// 【30℃】
					}

					type = jsonparse_next(parser);  	// 【,】
					type = jsonparse_next(parser);  	// 【键N】

					if (jsonparse_strcmp_value(parser, "humid") == 0)	// 【humid】
					{
						type = jsonparse_next(parser);  	// 【:】
						type = jsonparse_next(parser);  	// 【0】("值" = 数值)
						if (type == JSON_TYPE_NUMBER)		// 判断是否是【0】(数值 == ASSIC码形式)
						{
							jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
							os_printf("\t\t humid: %s\n", buffer);	// 【30】

							type = jsonparse_next(parser);  	// 【}】
							type = jsonparse_next(parser);  	// 【,】

							type = jsonparse_next(parser);  	//【键N】：第二个顶级键【Shenzhen】
							if(jsonparse_strcmp_value(parser, "Shenzhen") == 0)	// 【Shenzhen】
							{
								os_printf("\t Shenzhen{2} \n");

								jsonparse_next(parser);		// 【:】
								jsonparse_next(parser);		// 【{】

								jsonparse_next(parser);  	// 【键N】
								if(jsonparse_strcmp_value(parser, "temp") == 0)	// 【temp】
								{
									type = jsonparse_next(parser);		// 【:】
									type = jsonparse_next(parser);  	// 【"】
									if(type == JSON_TYPE_STRING)		// 判断是否是【"】
									{
										jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
										os_printf("\t\t temp: %s\n", buffer);	// 【35℃】

										type = jsonparse_next(parser);  	// 【,】
										type = jsonparse_next(parser);  	// 【键N】
										if(jsonparse_strcmp_value(parser, "humid") == 0)	// 【humid】
										{
											type = jsonparse_next(parser);  	// 【:】
											type = jsonparse_next(parser);  	// 【0】("值" = 数值)

											if (type == JSON_TYPE_NUMBER)		// 判断是否是【0】(数值 == ASSIC码形式)
											{
												jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
												os_printf("\t\t humid: %s\n", buffer);	// 【50%RH】

												type = jsonparse_next(parser);  	// 【}】
												type = jsonparse_next(parser);  	// 【,】

												type = jsonparse_next(parser);  	//【键N】：第三个顶级键【result】
												if(jsonparse_strcmp_value(parser, "XXX") == 0)
												{
													type = jsonparse_next(parser);  // 【:】
													type = jsonparse_next(parser);  // 【"】
													if (type == JSON_TYPE_STRING)	// 判断是否是【"】
													{
														jsonparse_copy_value(parser, buffer, sizeof(buffer));	// 获取【键】对应的值
														os_printf("\t XXX: %s\n", buffer);	//【Shenzhen is too hot!】

														type = jsonparse_next(parser);	// 【}】：JSON对象的框架尾字符
														os_printf("%c\n", type);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	os_printf("\r\n-------------------- 解析JSON树 -------------------\r\n");
	return 0;
}


/*解析JSON树*/
void ICACHE_FLASH_ATTR Parse_JSON_Tree(void)
{
	struct jsontree_context js;		// JSON树环境结构体

	// 和要解析的JSON树建立联系
	//【参数1：JSON树环境结构体指针		参数2：JSON树的第二个对象名指针		参数3：json_putchar函数】
	jsontree_setup(&js, (struct jsontree_value *)&V_JSON, json_putchar);

	// 解析JSON树
	//【参数1：JSON树环境结构体指针(解析的JSON树)		参数2：存放解析JSON树后的缓存指针
	json_parse(&js, A_JSON_Tree);	// 执行这条语句，将会调用对应的JSON树解析回调函数
}
/************************************************************************************/



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

	Setup_JSON_Tree();		// 创建JSON树
	Parse_JSON_Tree();		// 解析JSON树
}

