/**
 * @file    exti.c
 * @brief   GD32VF10x 外部中断（EXTI）配置： PA1 上升沿触发
 *
 * 本文件实现将 GPIOA 的 PA0 和 PA1 配置为外部中断输入，用于按键检测。
 * - 触发条件：**上升沿**（适用于按键一端接地，另一端接 PAx，按下时从低→高）
 * - 中断线：EXTI0（PA0）、EXTI1（PA1）
 * - 中断优先级：抢占优先级 2，子优先级分别为 0（EXTI0）和 1（EXTI1）
 *
 * @note
 * - 默认假设按键硬件连接为： PA1 → 按键 → GND，内部下拉（IPD）使空闲时为低电平；
 * - 按键按下时引脚被拉高，产生上升沿，触发中断；
 * - 若硬件使用上拉电阻（按键接 VCC），应改为下降沿触发（EXTI_Trigger_Falling）；
 * - 当前中断服务函数仅清除标志位，用户需在其中添加实际处理逻辑。
 *
 * @warning
 * - PA1 必须映射到 EXTI0/1（GD32VF10x 中默认 AFIO 映射即为此）；
 * - 若使用其他 GPIO（如 PB0），需额外配置 AFIO_EXTICR 寄存器。
 */
#include "exti.h"
#include "gd32vf103_exti.h"

/*------------------ 外部中断初始化 ------------------*/

/**
 * @brief 初始化 PA1 为上升沿触发的外部中断（用于按键检测）
 * @details
 * 1. 配置  PA1 为 **内部下拉输入（IPD）**；
 * 2. 使能 EXTI0 和 EXTI1，上升沿触发；
 * 3. 配置 NVIC 中断优先级（EXTI0: sub=0, EXTI1: sub=1）；
 * @note
 * - 内部下拉确保无按键时引脚为低电平；
 * - 按键按下 → 引脚被拉高 → 上升沿 → 触发中断；
 * - 若使用外部上拉，请改用 GPIO_Mode_IPU 并切换为下降沿触发。
 */
void EXTI_KEY_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);

    /* enable and set key EXTI interrupt to the lowest priority */
    eclic_global_interrupt_enable();
    eclic_irq_enable(EXTI0_IRQn,1, 1);
    eclic_irq_enable(EXTI1_IRQn,1, 1);
    eclic_irq_enable(EXTI2_IRQn,1, 1);
    eclic_irq_enable(EXTI3_IRQn,1, 1);
    eclic_irq_enable(EXTI4_IRQn,1, 1);
    eclic_irq_enable(EXTI5_9_IRQn,1, 1);

    /* connect key EXTI line to key GPIO pin */
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0 | GPIO_PIN_SOURCE_1 | GPIO_PIN_SOURCE_2 | GPIO_PIN_SOURCE_3 | GPIO_PIN_SOURCE_4 | GPIO_PIN_SOURCE_5);

    /* configure key EXTI line */
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_0);
    exti_init(EXTI_1, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_1);
    exti_init(EXTI_2, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_2);
    exti_init(EXTI_3, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_3);
    exti_init(EXTI_4, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_4);
    exti_init(EXTI_5, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_5);
}

/*------------------ 中断服务函数 ------------------*/

/**
 * @brief EXTI1 中断服务函数（对应 PA1）
 * @details
 * - 检测 EXTI_Line1 中断标志；
 * - 清除挂起位；
 * @note GD32VF1x的中断需添加"__attribute__((interrupt))"标记，以防止某些编译BUG。
 */
__attribute__((interrupt))
void EXTI1_IRQHandler(void)
{
  exti_interrupt_flag_clear(EXTI_1);
}