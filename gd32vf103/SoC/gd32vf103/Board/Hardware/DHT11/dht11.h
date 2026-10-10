/**
 ******************************************************************************
 * @file    dht11.h
 * @brief   DHT11 温湿度传感器驱动头文件
 *
 * 声明 DHT11 单总线通信的初始化、数据读取及底层 IO 控制接口。
 * 默认使用 GD32VF103 的 PA0 作为 DQ 数据引脚，通过 GPIO 操作实现
 * 高精度时序控制。
 *
 * @note    - 所有延时依赖 delay.h 提供的 delay_us() / delay_ms()
 *          - 温湿度数据以整数形式返回（单位：0.1°C / 0.1%RH）
 *          - 本驱动适用于标准 DHT11 模块（不支持 DHT22/AM2302）
 ******************************************************************************
 */
#ifndef __DHT11_H
#define __DHT11_H 
#include "nuclei_sdk_soc.h"  

//IO方向设置
//为了精确起见，这里采用寄存器写法
#define DHT11_IO_IN()  gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_0)
#define DHT11_IO_OUT() gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0)
////IO操作函数											   
#define	DHT11_DQ_IN  gpio_input_bit_get(GPIOA,GPIO_PIN_0)  //数据端口	PA0 

u8 DHT11_Init(void);//初始化DHT11
u8 DHT11_Read_Data(int *temp,int *humi);//读取温湿度
u8 DHT11_Read_Byte(void);//读出一个字节
u8 DHT11_Read_Bit(void);//读出一个位
u8 DHT11_Check(void);//检测是否存在DHT11
void DHT11_Rst(void);//复位DHT11    
#endif
