/**
 * @file led.h
 * @brief LED 控制模块头文件（PA6）
 */

#ifndef __LED_H
#define __LED_H
#include "gd32vf103.h"

/** @} */

/** @defgroup LED_Functions LED 函数
 *  @{ */

/**
 * @brief 初始化 PA6 用于控制 LED
 *
 * 初始化 PA6 为推挽输出并设置为初始高电平。
 */
void LED_Init(void);

/** @} */

#endif
