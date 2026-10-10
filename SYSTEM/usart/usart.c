/**
 * @file    usart.c
 * @brief   GD32VF10x 串口驱动：USART0（主调试口）与 USART1（辅助通信口）
 *
 * 本文件实现：
 * - **USART0**：用于调试输出（重定向 printf）和中断接收；
 * - **USART1**：通用串口通信（仅初始化，中断服务需用户扩展）；
 * - 支持标准库 `printf` 通过 USART0 输出（禁用半主机模式）。
 *
 * @note
 * - USART0 引脚：TX=PA9, RX=PA10；
 * - USART1 引脚：TX=PA2, RX=PA3；
 * - 接收使用全局缓冲区 `USART_RX_BUF` 和状态标记 `USART_RX_STA`（仅 USART0）；
 * - 当前 USART0 中断仅清除标志，**未处理数据接收逻辑**（需用户补充）。
 *
 * @warning
 * - 若使用 `printf`，确保主频 ≥ 8 MHz，否则高速波特率可能失真；
 * - 接收缓冲区无溢出保护，请确保上位机发送长度 ≤ `USART_REC_LEN - 1`；
 * - USART1 初始化未配置 NVIC，若需中断，需额外调用 NVIC 配置。
 */
#include "usart.h"

/*------------------ 全局接收缓冲区（仅 USART0） ------------------*/

/** 接收缓冲区，最大 USART_REC_LEN 字节 */
u8 USART_RX_BUF[USART_REC_LEN];

/**
 * @brief 接收状态标记（位域定义）
 * - bit15: 接收完成标志（1=完成）
 * - bit14: 接收到 0x0D（回车）
 * - bit13～0: 已接收有效字节数
 */
u16 USART_RX_STA = 0; //接收状态标记

/*------------------ USART0 初始化 ------------------*/

/**
 * @brief 初始化 USART0（PA9-TX, PA10-RX）
 * @param bound 波特率（如 115200）
 * @details
 * - 配置 GPIO 为复用推挽（TX）和浮空输入（RX）；
 * - 使能接收中断（RXNE）；
 * - NVIC 配置：抢占优先级 3，子优先级 3；
 * - 支持 printf 重定向（通过 fputc）。
 * @note
 * - 接收中断服务函数需用户完善数据处理逻辑；
 * - 当前 ISR 仅清除中断标志，防止死循环。
 */
void uart_init(u32 bound)
{
	/* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART0);

    /* connect port to USARTx_Tx */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    /* connect port to USARTx_Rx */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* USART configure */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, bound);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    /* USART interrupt configuration */
    eclic_global_interrupt_enable();
    eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO1);
    eclic_irq_enable(USART0_IRQn, 1, 0);
    usart_interrupt_enable(USART0, USART_INT_RBNE);
}

/*------------------ USART1 初始化 ------------------*/

/**
 * @brief 初始化 USART1（PA2-TX, PA3-RX）
 * @param BaudRatePrescaler 波特率（如 9600）
 * @details
 * - 仅配置 GPIO 和 USART 寄存器；
 * - **未配置 NVIC 中断**（若需中断，需用户自行添加）；
 * - 默认无流控、8N1 格式。
 * @note
 * - USART1 时钟来自 APB1，确保系统配置正确；
 * - 如需接收中断，请在调用后配置 NVIC 并实现 USART1_IRQHandler。
 */
void Usart_Int2(uint32_t BaudRatePrescaler)
{
	/* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART1);

    /* connect port to USARTx_Tx */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

    /* connect port to USARTx_Rx */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);

    /* USART configure */
    usart_deinit(USART1);
    usart_baudrate_set(USART1, BaudRatePrescaler);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);
}

/*------------------ USART0 中断服务函数 ------------------*/

/**
 * @brief USART0 全局中断服务函数
 * @details
 * - 仅处理 RXNE（接收非空中断）；
 * - **当前仅清除中断标志，未读取 DR 或更新缓冲区**；
 * - 用户应在此处添加接收逻辑（如存入 USART_RX_BUF）。
 * @warning
 * - 必须读取 DR 或清除标志，否则会重复进入中断；
 * - 建议在 ISR 中读取 `USART_ReceiveData(USART0)` 避免数据丢失。
 */
void USART0_IRQHandler(void)
{
    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)){
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
    }
}
