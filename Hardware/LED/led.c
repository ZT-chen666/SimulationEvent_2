/**
 * @file led.c
 * @brief LED 控制模块实现（PA6）
 *
 * 本模块提供对板载 LED 的初始化接口，使用 PA6 控制输出。
 */

#include "led.h"

/**
 * @brief 初始化 PA6 用于控制 LED
 *
 * 将 PA6 配置为推挽输出，速度 50MHz，并将输出设为高电平（默认灭或亮取决于电路设计）。
 */
void LED_Init(void)
{
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
	gpio_bit_set(GPIOA, GPIO_PIN_6);
}
 
