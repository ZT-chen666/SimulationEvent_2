/**
 * @file    pwm.c
 * @brief   GD32VF10x 定时器3（TIMER2）四通道 PWM 输出驱动
 *
 * 本文件配置 TIMER2 生成 4 路 PWM 信号，输出到以下引脚：
 * - CH1 → PA6
 * - CH2 → PA7
 * - CH3 → PB0
 * - CH4 → PB1
 *
 * @note
 * - 使用 **PWM 模式 2**（TIM_OCMode_PWM2）：  
 *   当计数器 < CCR 时输出 **低电平**，否则输出 **高电平**（即占空比 = (ARR - CCR) / ARR）；
 *   若需常规 PWM（CCR 越大占空比越高），请改用 TIM_OCMode_PWM1；
 * - 定时器配置：
 *   - 预分频器（PSC）= 108 → 计数频率 = 108MHz / (108-1) ≈ 1000 kHz
 *   - 自动重装载值（ARR）= 256 → PWM 周期 ≈ 256 / 1000kHz ≈ **256 μs**（约 3.90 kHz）
 * - 所有通道默认占空比为 0%（因 CCR 初始为 0）；
 * - 用户可通过 `TIM_SetCompareX(TIMER2, value)` 动态调整占空比。
 *
 * @warning
 * - PA6/PA7/PB0/PB1 必须配置为复用推挽输出（AF_PP）；
 * - AFIO 时钟必须使能（用于重映射，但本例使用默认映射）；
 * - 当前未启用 TIMER2 更新中断（如需周期回调，请取消注释相关代码）。
 */
#include "pwm.h"


/*------------------ PWM 初始化 ------------------*/

/**
 * @brief 初始化 TIMER2 为单通道 PWM 输出（PB0）
 * @details
 * - 配置 GPIO 为复用推挽输出（50 MHz）
 * - 设置 TIMER2 时基：PSC=108, ARR=256 → ～3.85 kHz PWM
 * - 使用 PWM 模式 2（高电平有效逻辑需注意）
 * - 使能自动重装载预装载（ARR 和 CCR 寄存器缓冲）
 * @note
 * - 调用后所有通道输出低电平（CCR=0）；
 * - 占空比通过 TIM_SetCompare1～4() 动态设置；
 * - 示例：TIM_SetCompare1(TIMER2, 128); // CH1 占空比 ≈ 50%（模式2下）
 */
void PWM_Init(void)
{
    timer_oc_parameter_struct timer_ocinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER2);

    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);

    timer_deinit(TIMER2);
    /* initialize TIMER init parameter struct */
    timer_struct_para_init(&timer_initpara);
    /* TIMER2 configuration */
    timer_initpara.prescaler         = 107;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 255;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(TIMER2, &timer_initpara);

    /* initialize TIMER channel output parameter struct */
    timer_channel_output_struct_para_init(&timer_ocinitpara);
    /* CH2 configuration in PWM mode */
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER2,TIMER_CH_0,&timer_ocinitpara);
    timer_channel_output_config(TIMER2,TIMER_CH_1,&timer_ocinitpara);
    timer_channel_output_config(TIMER2,TIMER_CH_2,&timer_ocinitpara);

    /* CH1 configuration in PWM mode2*/
    timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_0, 255);
    timer_channel_output_mode_config(TIMER2,TIMER_CH_0,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2,TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);

    /* CH2 configuration in PWM mode2*/
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, 5);
    timer_channel_output_mode_config(TIMER2,TIMER_CH_1,TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER2,TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);

    /* CH3 configuration in PWM mode2*/
    timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_2, 5);
    timer_channel_output_mode_config(TIMER2,TIMER_CH_2,TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER2,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER2);

    timer_enable(TIMER2);
}


