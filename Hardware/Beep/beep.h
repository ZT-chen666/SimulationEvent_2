/**
 * @file beep.h
 * @brief 蜂鸣器 控制模块头文件
 */

#ifndef __BEEP_H
#define __BEEP_H	 
#include "gd32vf103.h"

/** @} */

/** @defgroup Beep_Functions Beep 函数
 *  @{ */

/**
 * @brief 初始化 PA1 用于控制 Beep
 *
 * 初始化 PA1 为推挽输出。
 */
void Beep_Init(void);

		 				    
#endif