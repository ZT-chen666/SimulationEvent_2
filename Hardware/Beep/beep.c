/**
 * @file beep.c
 * @brief 有源蜂鸣器 控制模块实现（PA1）
 *
 * 本模块提供对板载 有源蜂鸣器 的初始化接口，使用 PA1 控制输出。
 */

#include "beep.h"

/**
 * @brief 初始化 PA1 用于控制 蜂鸣器
 *
 * 将 PA1 配置为推挽输出，速度 50MHz。
 */
void Beep_Init(void)
{
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
}