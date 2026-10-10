/**
 * @file led.h
 * @brief LED 控制模块头文件（PA0）
 */

#ifndef __LED_H
#define __LED_H
#include "gd32vf103.h"
#include "gd32vf103_gpio.h"

/** @} */

/** @defgroup LED_Functions LED 函数
 *  @{ */

/**
 * @brief 初始化 PA0 用于控制 LED
 *
 * 初始化 PA0 为推挽输出并设置为初始高电平。
 */
void LED_Init(void);

/** @} */

#endif
