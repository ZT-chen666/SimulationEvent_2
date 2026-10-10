/* Bounded adapter for the supplied GD32VF103 ADC/DMA HAL.
 * Vendor HAL and its original notices remain unchanged in Common/Source. */
#include "adc.h"
#include "los_task.h"
static volatile uint16_t frame[2];
static int wait_calibration(uint32_t mask)
{
    unsigned count;
    for(count=0;count<100;count++) {
        if(!(ADC_CTL1(ADC0)&mask))return 0;
        LOS_Msleep(1);
    }
    return -1;
}
int ADC0_DMA_Config(void)
{
    dma_parameter_struct p;
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8);
    gpio_init(GPIOA,GPIO_MODE_AIN,GPIO_OSPEED_50MHZ,GPIO_PIN_1|GPIO_PIN_2);
    dma_deinit(DMA0,DMA_CH0);dma_struct_para_init(&p);
    p.periph_addr=(uint32_t)&ADC_RDATA(ADC0);p.memory_addr=(uint32_t)frame;
    p.direction=DMA_PERIPHERAL_TO_MEMORY;p.number=2;
    p.periph_inc=DMA_PERIPH_INCREASE_DISABLE;p.memory_inc=DMA_MEMORY_INCREASE_ENABLE;
    p.periph_width=DMA_PERIPHERAL_WIDTH_16BIT;p.memory_width=DMA_MEMORY_WIDTH_16BIT;
    p.priority=DMA_PRIORITY_HIGH;dma_init(DMA0,DMA_CH0,&p);
    dma_circulation_disable(DMA0,DMA_CH0);dma_memory_to_memory_disable(DMA0,DMA_CH0);
    adc_deinit(ADC0);adc_mode_config(ADC0,ADC_MODE_FREE);
    adc_special_function_config(ADC0,ADC_SCAN_MODE,ENABLE);
    adc_special_function_config(ADC0,ADC_CONTINUOUS_MODE,DISABLE);
    adc_data_alignment_config(ADC0,ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0,ADC_REGULAR_CHANNEL,2);
    adc_regular_channel_config(ADC0,0,ADC_CHANNEL_1,ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0,1,ADC_CHANNEL_2,ADC_SAMPLETIME_55POINT5);
    adc_external_trigger_source_config(ADC0,ADC_REGULAR_CHANNEL,ADC0_1_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0,ADC_REGULAR_CHANNEL,ENABLE);
    adc_enable(ADC0);LOS_Msleep(1);
    ADC_CTL1(ADC0)|=ADC_CTL1_RSTCLB;
    if(wait_calibration(ADC_CTL1_RSTCLB))goto error;
    ADC_CTL1(ADC0)|=ADC_CTL1_CLB;
    if(wait_calibration(ADC_CTL1_CLB))goto error;
    adc_dma_mode_enable(ADC0);return 0;
error:
    adc_disable(ADC0);return -1;
}
int ADC0_Read_Frame(uint16_t raw[2])
{
    unsigned n;raw[0]=raw[1]=0xffffu;
    dma_channel_disable(DMA0,DMA_CH0);dma_flag_clear(DMA0,DMA_CH0,DMA_FLAG_G);
    frame[0]=frame[1]=0xffffu;
    dma_transfer_number_config(DMA0,DMA_CH0,2);dma_channel_enable(DMA0,DMA_CH0);
    adc_software_trigger_enable(ADC0,ADC_REGULAR_CHANNEL);
    for(n=0;n<5;n++) {
        if(dma_flag_get(DMA0,DMA_CH0,DMA_FLAG_ERR))break;
        if(dma_flag_get(DMA0,DMA_CH0,DMA_FLAG_FTF)) {
            dma_channel_disable(DMA0,DMA_CH0);
            raw[0]=frame[0];raw[1]=frame[1];return 0;
        }
        LOS_Msleep(1);
    }
    dma_channel_disable(DMA0,DMA_CH0);return -1;
}
