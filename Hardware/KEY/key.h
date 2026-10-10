/**
 ******************************************************************************
 * @file    key.h
 * @brief   按键输入驱动头文件
 *
 * 定义按键硬件接口、电平逻辑及扫描函数。
 * 默认使用 PA2 作为独立按键输入引脚，配置为 **下拉输入**：
 * - 按键未按下：PA2 = 低电平（0）
 * - 按键按下：PA2 = 高电平（1）
 *
 * @note    - 按键电路需将 PA2 通过上拉（或直接）连接至 VCC，另一端接地
 *          - Key_Scan() 函数支持任意 GPIO 引脚，便于扩展多按键
 *          - 电平定义通过 KEY_ON / KEY_OFF 统一管理，便于移植
 ******************************************************************************
 */
#ifndef __KEY_H
#define __KEY_H	 

#include "gd32vf103.h"

/** @} */ // end of KEY_GPIO_Macros

/**
 * @brief  初始化默认按键引脚（PA2）为下拉输入模式
 *
 * 配置 GPIOA.2 为下拉输入（GPIO_Mode_IPD），确保无按键时为低电平。
 *
 * @note   若使用其他引脚或上拉模式，需修改此函数实现。
 */
void KEY_Init(void);

/**
 * @brief  扫描指定按键是否发生一次有效按下事件（带释放等待消抖）
 * @param  GPIOx: GPIO 端口（如 GPIOA）
 * @param  GPIO_Pin: 引脚位（如 GPIO_Pin_2）
 * @retval KEY_ON (0): 检测到完整按键动作（按下并已释放）
 * @retval KEY_OFF (1): 无按键事件
 *
 * @note   - 函数内部会阻塞等待按键释放，防止重复触发
 *         - 适用于短按检测，不支持长按或连发
 *         - 调用示例：\code if(Key_Scan(GPIOA, GPIO_Pin_1) == KEY_ON) { ... } \endcode
 *
 * @warning 按键硬件逻辑必须与 \c KEY_ON 定义一致（当前为高电平有效）。
 */
uint8_t Key_Scan(uint32_t GPIOx,uint16_t GPIO_Pin);	

/**
 * @defgroup KEY_State_Defines 按键状态定义
 * @{
 */
#define KEY_ON   0  ///< 按键按下时的逻辑电平
#define KEY_OFF  1  ///< 按键释放时的逻辑电平
/** @} */ // end of KEY_State_Defines

#endif  /* __KEY_H */
