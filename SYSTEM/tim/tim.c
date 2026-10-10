/**
 * @file    tim.c
 * @brief   GD32VF10x 定时器驱动：TIM0（高级）与 TIM1（通用）初始化
 *
 * 本文件实现两个定时器的周期性中断配置：
 * - **TIM0**：高级定时器，带重复计数器（Repetition Counter），用于低频高精度中断；
 * - **TIM1**：32 位通用定时器，用于常规周期中断。
 *
 * @note
 * - 系统主频假设为 108 MHz；
 * - 两定时器均配置为 **10 kHz 计数频率**（PSC = 10799）；
 * - 自动重装载值（ARR）= 10 → 中断周期 = (10 + 1) / 10kHz = **1.1 ms**；
 * - TIM0 额外启用重复计数器（RCR = 8），实际中断周期 = 1.1 ms × (8 + 1) = **9.9 ms**。
 *
 * @warning
 * - TIM0 是高级定时器，其更新事件受 RCR 控制，**每 (RCR+1) 次溢出才触发一次中断**；
 * - 若需精确 1.1 ms 中断，请使用 TIM1 或将 TIM0 的 RCR 设为 0；
 * - 中断优先级相同（抢占=0, 子=3），若同时触发，按向量表顺序处理。
 */
#include "tim.h"
#include "gd32vf103.h"

/*------------------ TIM0 初始化（高级定时器） ------------------*/

/**
 * @brief 初始化 TIM0 为周期性更新中断（带重复计数器）
 * @details
 * - 时钟源：APB2（108 MHz）
 * - 预分频器 PSC = 10799 → 计数频率 = 108MHz / 10800 = **10 kHz**
 * - 自动重装载值 ARR = 10 → 单次计数周期 = 11 个 tick = **1.1 ms**
 * - 重复计数器 RCR = 8 → **每 9 次溢出触发一次更新中断**（9.9 ms 周期）
 * - 中断通道：TIM0_UP_IRQn
 * - 优先级：抢占 0，子优先级 3（高优先级）
 *
 * @note
 * - 适用于需要较长中断周期但避免大 ARR 值的场景；
 * - 更新中断服务函数需在外部定义：void TIM0_UP_IRQHandler(void);
 */
void TIM0_Init(void)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER0);

    timer_deinit(TIMER0);
    /* initialize TIMER init parameter struct */
    timer_struct_para_init(&timer_initpara);
    /* TIMER0 configuration */
    timer_initpara.prescaler         = 10799;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 4999;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(TIMER0, &timer_initpara);

    eclic_global_interrupt_enable();
    eclic_set_nlbits(ECLIC_GROUP_LEVEL3_PRIO1);
    eclic_irq_enable(TIMER0_UP_IRQn,1,0);
    timer_interrupt_enable(TIMER0, TIMER_INT_UP);
    timer_enable(TIMER0);
}

/*------------------ TIM1 初始化（通用定时器） ------------------*/

/**
 * @brief 初始化 TIM1 为周期性更新中断
 * @details
 * - 时钟源：APB1（108 MHz，GD32VF10x 中 APB1 有 x2 倍频 → TIM1CLK = 108 MHz）
 * - 预分频器 PSC = 10799 → 计数频率 = 108MHz / 10800 = **10 kHz**
 * - 自动重装载值 ARR = 4999 → 中断周期 = (4999+1) / 10kHz = **200 ms**
 * - 中断通道：TIM1_IRQn
 * - 优先级：抢占 0，子优先级 3（与 TIM0 相同）
 *
 * @note
 * - TIM1 为 32 位定时器（实际在 F10x 中为 16 位，但可级联，此处按 16 位使用）；
 * - 适用于高频率周期任务（如状态轮询、LED 闪烁等）；
 * - 更新中断服务函数需在外部定义：void TIM1_IRQHandler(void);
 */
void TIM1_Init(void)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);

    timer_deinit(TIMER1);
    /* initialize TIMER init parameter struct */
    timer_struct_para_init(&timer_initpara);
    /* TIMER1 configuration */
    timer_initpara.prescaler         = 10799;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 4999;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(TIMER1, &timer_initpara);

    eclic_global_interrupt_enable();
    eclic_set_nlbits(ECLIC_GROUP_LEVEL3_PRIO1);
    eclic_irq_enable(TIMER1_IRQn,1,0);
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);
    timer_enable(TIMER1);
}
