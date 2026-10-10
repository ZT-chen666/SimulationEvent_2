/**
 * @file    dma.c
 * @brief   GD32VF10x DMA0 通用通道配置函数
 *
 * 本文件提供一个通用的 DMA 通道初始化函数，适用于大多数外设（如 USART、SPI、ADC 等）
 * 的 **存储器到外设（Memory-to-Peripheral）** 数据传输场景。
 *
 * @note
 * - 当前实现固定为以下配置：
 *   - 传输方向：存储器 → 外设（DMA_DIR_PeripheralDST）
 *   - 数据宽度：8 位（Byte）
 *   - 存储器地址自增，外设地址固定
 *   - 模式：Normal（非循环）
 *   - 优先级：High
 * - 若需其他模式（如外设→存储器、16位、循环模式等），请扩展此函数或创建专用接口。
 *
 * @warning
 * - 本函数仅适用于 DMA0（GD32VF10x 中 DMA1 用于高级外设如 ADC3、SPI3 等）；
 * - 调用前需确保目标外设时钟已使能；
 * - 不支持内存到内存（M2M）传输（当前强制禁用）。
 */

#include "dma.h"


/*------------------ 通用 DMA 通道配置函数 ------------------*/

/**
 * @brief 配置 DMA0 指定通道的基本参数（存储器→外设，8位，Normal 模式）
 * @param DMA_CHx       DMA 通道，如 DMA0_Channel4（对应 USART1_TX）
 * @param cpar          外设寄存器地址（如 (u32)&USART1->DR）
 * @param cmar          存储器起始地址（如 (u32)tx_buffer）
 * @param cndtr         待传输的数据项数量（注意：不是字节数，而是“项数”）
 * @details
 * 此函数适用于典型的发送场景，例如：
 *   - USART 发送：cpar = &USARTx->DR, cmar = buffer, cndtr = len
 *   - SPI 发送：cpar = &SPIx->DR, cmar = data, cndtr = count
 *
 * @note
 * - 数据宽度固定为 8 位（Byte），若外设要求 16 位（HalfWord），需修改配置；
 * - 传输完成后通道自动停止（Normal 模式）；
 * - 函数内部使能 DMA0 时钟，无需外部调用；
 * - 建议在启用外设 DMA 请求前调用此函数。
 *
 */
void DMA_Config(dma_channel_enum DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	dma_parameter_struct dma_init_struct;

    /* 使能 DMA0 时钟（AHB 总线外设） */
    rcu_periph_clock_enable(RCU_DMA0);

    /* deinitialize DMA channel3(USART0 tx) */
    dma_deinit(DMA0, DMA_CHx);

    /* 配置 DMA 通道参数 */
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.periph_addr = cpar;  // 外设数据地址
    dma_init_struct.memory_addr = cmar;       // 源数据地址
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL; // 内存到外设
    dma_init_struct.number    = cndtr;    // 传输字节数
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE; // 外设地址不变
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;      // 内存地址递增
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    /* 应用配置到 DMA0 Channel3（USART0_TX 固定映射） */
    dma_init(DMA0, DMA_CHx, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CHx);     // 单次传输
    dma_memory_to_memory_disable(DMA0, DMA_CHx);// 禁止内存到内存
} 
