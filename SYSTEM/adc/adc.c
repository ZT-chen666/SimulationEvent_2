/**
 * @file    adc.c
 * @brief   GD32VF10x ADC 驱动实现（支持单通道查询 + 多通道 DMA 循环采集）
 *
 * 本文件提供两种 ADC 使用模式：
 * 1. **单通道软件触发模式**（Adc_Init + Get_Adc）  
 *    - 适用于低频、按需采样场景（如电池电压检测）
 *    - 默认使用 PA6（ADC_Channel_1）
 *
 * 2. **5 通道 DMA 循环采集模式**（ADC0_DMA_Config）  
 *    - 适用于多路传感器同步采集（如温度、光敏、电位器等）
 *    - 使用 PA0～PA4（ADC_Channel_0～4），DMA 自动填充缓冲区
 *
 * @note
 * - ADC 时钟 ≤ 14 MHz（PCLK2=72MHz 时，分频 ≥ 6）
 * - 单通道模式采样时间建议 ≥ 7.5 cycles（当前为 1.5 cycles，仅适用于低阻抗源）
 * - DMA 模式使用循环缓冲（Circular Mode），数据持续更新
 *
 * @warning 若同时使用两种模式，请确保只初始化一种，避免资源冲突。
 */
 #include "adc.h"
#include "gd32vf103_adc.h"
 #include "systick.h"
	   
/*------------------ 中断配置 ------------------*/

/**
 * @brief 配置 ADC0/1 中断优先级
 * @details 用于 ADC 转换完成中断（EOC），当前未启用中断处理
 */
void Adc_IRQ_Config(void)
{
    eclic_global_interrupt_enable();
    eclic_set_nlbits(ECLIC_GROUP_LEVEL3_PRIO1);
    eclic_irq_enable(ADC0_1_IRQn,1,0);
    adc_interrupt_enable(ADC0, ADC_INT_EOC);
}

/*------------------ 单通道 ADC 初始化（软件触发） ------------------*/

/**
 * @brief 初始化 ADC0 为单通道连续转换模式（默认通道：PA1 / ADC_Channel_1）
 * @details
 * - 工作模式：独立模式、单通道、连续转换、软件启动
 * - 数据右对齐，无外部触发
 * - 自动执行校准
 * @note 此模式不启用 DMA 或中断，需调用 Get_Adc() 主动读取
 */																   
void  Adc_Init(void)
{ 	
	/* enable GPIOA clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* enable ADC0 clock */
    rcu_periph_clock_enable(RCU_ADC0);
    /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8);

	//PA1 作为模拟通道输入引脚                       
	/* config the GPIO as analog mode */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_1); 

	/* reset ADC */
    adc_deinit(ADC0);

	/* ADC mode config */
    adc_mode_config(ADC_MODE_FREE);
	/* ADC scan function enable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    /* ADC continous function enable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_NONE);
	/* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	/* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
	/* ADC temperature sensor channel config */
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_1, ADC_SAMPLETIME_239POINT5);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
    /* enable ADC interface */
    adc_enable(ADC0);
	delay_ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);
}				  

/**
 * @brief 获取指定 ADC 通道的单次转换值
 * @param ch ADC 通道号（如 ADC_Channel_1）
 * @return 12 位 ADC 转换结果（0～4095）
 * @note
 * - 每次调用会重新配置通道并启动一次转换
 * - 采样时间设为 1.5 cycles（仅适用于低输出阻抗信号源）
 * - 若信号源阻抗高，建议增加采样时间（如 ADC_SampleTime_55Cycles5）
 */
u16 Get_Adc(u8 ch)   
{
    //设置指定ADC的规则组通道，一个序列，采样时间
	adc_regular_channel_config(ADC0, 0, ch, ADC_SAMPLETIME_239POINT5);
  
	/* ADC software trigger enable */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
	 
	while(!adc_flag_get(ADC0, ADC_FLAG_EOC ));//等待转换结束

	return adc_regular_data_read(ADC0);	//返回最近一次ADC0规则组的转换结果
}

/**
 * @brief 对指定通道进行多次采样并返回平均值
 * @param ch ADC 通道号
 * @param times 采样次数
 * @return 平均 ADC 值
 * @note 每次采样间隔 1ms，适用于消除随机噪声
 */
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(1);
	}
	return temp_val/times;
}

/** 
 * @brief 存放 ADC 6 通道 DMA 采集结果（对应 PA0～PA5 → ADC_Channel_0～5）
 * @details 在 ADC0_DMA_Config() 启动后，该数组由 DMA 自动更新
 */
volatile unsigned short int ADC_ConvertedValue[6] ;

void ADC0_DMA_Config(void)
{
	/* enable GPIOA clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC0);
    /* enable DMA0 clock */
    rcu_periph_clock_enable(RCU_DMA0);
    /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8);
	
	/* * * * * * ADC 引脚配置 * * * * * */
	

	// PA0/1/2/3/4 作为模拟通道输入引脚                         
	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5); 

	/* * * * * * DMA 模式配置 * * * * * */
	/* ADC_DMA_channel configuration */
    dma_parameter_struct dma_data_parameter;

	/* ADC DMA_channel configuration */
    dma_deinit(DMA0, DMA_CH0);
	
	// 源数据地址
	dma_data_parameter.periph_addr  = (uint32_t)(&ADC_RDATA(ADC0));
	// 目标地址
	dma_data_parameter.memory_addr  = (uint32_t)(ADC_ConvertedValue);
	// 方向：外设到存储器
	dma_data_parameter.direction    = DMA_PERIPHERAL_TO_MEMORY;
	// 传输大小
	dma_data_parameter.number       = 6;
	// 外设地址递增
	dma_data_parameter.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
	// 内存地址递增
	dma_data_parameter.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
	// 外设数据单位
	dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
	// 内存数据单位
	dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT; 
	// 优先级
	dma_data_parameter.priority     = DMA_PRIORITY_HIGH;
	// 初始化DMA
	dma_init(DMA0, DMA_CH0, &dma_data_parameter);

	// DMA模式，循环
	dma_circulation_enable(DMA0, DMA_CH0);
	// 关闭内存到内存的传输
	dma_memory_to_memory_disable(DMA0, DMA_CH0);
	
	// 使能DMA
	dma_channel_enable(DMA0, DMA_CH0);
	
	/* * * * * * ADC 模式配置 * * * * * */
	/* reset ADC */
    adc_deinit(ADC0);

	// 只使用一个ADC，属于独立模式
	adc_mode_config(ADC_MODE_FREE);
	// 扫描模式
	adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
	// 连续转换模式
	adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
	// ADC数据右对齐
	adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	// 顺序进行规则转换的ADC通道的数目
	adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 6);
	
	adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_0, ADC_SAMPLETIME_55POINT5);
	adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_1, ADC_SAMPLETIME_55POINT5);
	adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_2, ADC_SAMPLETIME_55POINT5);
	adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_3, ADC_SAMPLETIME_55POINT5);
	adc_regular_channel_config(ADC0, 4, ADC_CHANNEL_4, ADC_SAMPLETIME_55POINT5);
	adc_regular_channel_config(ADC0, 5, ADC_CHANNEL_5, ADC_SAMPLETIME_55POINT5);
	
	// 转换由软件开启
	adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

	/* enable ADC interface */
    adc_enable(ADC0);
    delay_ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* ADC DMA function enable */
    adc_dma_mode_enable(ADC0);
    /* ADC software trigger enable */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);	
}

/*------------------ ADC 中断服务函数 ------------------*/

/**
 * @brief ADC0/1 共用中断服务函数
 * @details 当前仅清除 EOC 标志，无实际处理逻辑
 * @note 若需在转换完成时执行操作，请在此添加代码
 */
void ADC0_1_IRQHandler(void)
{
    if (adc_interrupt_flag_get(ADC0, ADC_INT_FLAG_EOC) == SET)
    {
        adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
    }
}