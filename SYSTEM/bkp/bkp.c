/**
 * @file    bkp.c
 * @brief   GD32VF10x 后备寄存器与侵入检测（Tamper）功能驱动
 *
 * 本文件实现基于 RTC 侵入检测引脚（TAMPER，通常为 PC13）的安全监控功能。
 * 当检测到有效电平跳变（默认低电平有效）时，触发侵入事件，自动：
 *   - 清除所有后备寄存器（BKP_DRx）内容
 *   - 清除 RTC 寄存器（若 RTC 已配置）
 *   - 可选：产生中断通知 CPU
 *
 * @note
 * - 侵入检测用于防拆机、防篡改等安全场景
 * - 一旦触发，后备区数据立即清零（不可恢复）
 * - TAMPER 引脚默认为 PC13（具体请参考芯片数据手册）
 * - 本例中通过 USART 输出调试信息（需确保 usart.h 已初始化）
 *
 * @warning
 * - 侵入事件发生后，必须调用 BKP_ClearFlag() 才能重新使能检测
 * - 中断处理中应尽快清除标志，避免重复触发
 */
#include "bkp.h"
#include "usart.h"
#include <stdio.h>

/*------------------ 侵入检测中断配置 ------------------*/

/**
 * @brief 配置 TAMPER 中断优先级并使能侵入中断
 * @details
 * - 中断通道：TAMPER_IRQn
 * - 抢占优先级：0，子优先级：0
 * - 同时使能 BKP 侵入中断
 * @note 若仅需事件检测而无需中断响应，可不调用此函数
 */
void TAMPER_ITConfig(void)
{
	/* ECLIC configuration */
    eclic_global_interrupt_enable();
    eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO1);
    eclic_irq_enable(TAMPER_IRQn, 1, 0);
}

/*------------------ 侵入检测初始化 ------------------*/

/**
 * @brief 初始化侵入检测功能（Tamper Detection）
 * @details
 * 1. 使能 PWR 和 BKP 时钟
 * 2. 允许访问后备寄存器区域
 * 3. 配置 TAMPER 引脚为低电平有效
 * 4. 清除历史侵入标志
 * 5. 使能中断（可选）和 TAMPER 引脚检测
 * @note
 * - 调用前确保系统时钟已配置
 * - 默认检测电平为低有效（可修改 BKP_TamperPinLevelConfig）
 */
void TAMPER_Init(void)
{
    TAMPER_ITConfig();

    /* PMU lock enable */
    rcu_periph_clock_enable(RCU_PMU);
    /* BKP clock enable */
    rcu_periph_clock_enable(RCU_BKPI);
    
	/* enable write access to the registers in backup domain */
    pmu_backup_write_enable();
    /* confirm RCU_BDCTL_BKPRST bit is reset */
    if(RESET != (RCU_BDCTL & RCU_BDCTL_BKPRST)){
        rcu_bkp_reset_disable();
    }
    /* tamper pin active level set */
    bkp_tamper_active_level_set(TAMPER_PIN_ACTIVE_HIGH);
    /* tamper detection disable  */
    bkp_tamper_detection_disable();
    /* disable the tamper interrupt */
    bkp_interrupt_disable();
    /* clear the bit flag of tamper event */
    bkp_flag_clear();
    
    /* configure the tamper pin active on low level, and enable the tamper pin */
    bkp_interrupt_enable();
    /* tamper detection enable  */
    bkp_tamper_detection_enable();
}

/*------------------ 侵入中断服务函数 ------------------*/

/**
 * @brief TAMPER 侵入检测中断服务函数
 * @details
 * - 检测到侵入事件后，通过 USART 打印提示信息
 * - 必须清除中断挂起位和事件标志
 * @note
 * - 侵入事件已导致后备寄存器清零，应用层需重新初始化关键数据
 * - printf 仅用于调试，发布版本建议移除或替换为轻量日志
 */
void TAMPER_IRQHandler(void)
{
    if(RESET != bkp_interrupt_flag_get())
    {
        printf("触发侵入中断\r\n");
        /* clear the interrupt bit flag of tamper interrupt */
        bkp_interrupt_flag_clear();
	    /* clear the bit flag of tamper event */
        bkp_flag_clear();
    }
}

