#include "ultrasonic.h"
#include "gd32vf103.h"
#include "los_task.h"
#include "los_interrupt.h"
static int ready;
static uint32_t timer_clock(void)
{
    uint32_t apb=rcu_clock_freq_get(CK_APB1);
    return apb==rcu_clock_freq_get(CK_AHB)?apb:apb*2u;
}
static void trigger_delay(uint32_t us)
{
    uint64_t start=SysTimer_GetLoadValue();
    uint64_t ticks=((uint64_t)(SystemCoreClock/4u)*us+999999u)/1000000u;
    while(SysTimer_GetLoadValue()-start<ticks){}
}
int ultrasonic_init(void)
{
    timer_parameter_struct t;
    timer_ic_parameter_struct ic;
    uint32_t hz=timer_clock();
    ready=0;
    rcu_periph_clock_enable(RCU_GPIOA);rcu_periph_clock_enable(RCU_TIMER1);
    gpio_bit_reset(GPIOA,GPIO_PIN_4);
    gpio_init(GPIOA,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_4);
    gpio_init(GPIOA,GPIO_MODE_IPD,GPIO_OSPEED_50MHZ,GPIO_PIN_3);
    if(hz<1000000u || hz%1000000u || hz/1000000u>65536u)return -1;
    timer_deinit(TIMER1);timer_struct_para_init(&t);
    t.prescaler=(uint16_t)(hz/1000000u-1u);t.period=65535u;
    timer_init(TIMER1,&t);
    /* PA3 is TIMER1 CI3. Capture both edges independently, even when task sleeps.
     * CH3 direct rising, paired CH2 indirect falling share this one input. */
    ic.icprescaler=TIMER_IC_PSC_DIV1;ic.icfilter=0;
    ic.icselection=TIMER_IC_SELECTION_DIRECTTI;ic.icpolarity=TIMER_IC_POLARITY_RISING;
    timer_input_capture_config(TIMER1,TIMER_CH_3,&ic);
    ic.icselection=TIMER_IC_SELECTION_INDIRECTTI;ic.icpolarity=TIMER_IC_POLARITY_FALLING;
    timer_input_capture_config(TIMER1,TIMER_CH_2,&ic);
    timer_disable(TIMER1);ready=1;return 0;
}
int ultrasonic_read(uint32_t *pulse_us)
{
    unsigned poll;
    uint64_t started;
    uint32_t rc=-1u;
    *pulse_us=0;
    if(!ready || gpio_input_bit_get(GPIOA,GPIO_PIN_3)!=RESET)return -1;
    timer_disable(TIMER1);timer_counter_value_config(TIMER1,0);
    timer_flag_clear(TIMER1,TIMER_FLAG_CH2|TIMER_FLAG_CH3|TIMER_FLAG_CH2O|TIMER_FLAG_CH3O|TIMER_FLAG_UP);
    timer_enable(TIMER1);started=SysTimer_GetLoadValue();
    {
        UINT32 saved=LOS_IntLock();
        gpio_bit_set(GPIOA,GPIO_PIN_4);trigger_delay(12);
        gpio_bit_reset(GPIOA,GPIO_PIN_4);LOS_IntRestore(saved);
    }
    for(poll=0;poll<35u;poll++) {
        if(SysTimer_GetLoadValue()-started>=(uint64_t)(SystemCoreClock/4000u)*35u)break;
        if(timer_flag_get(TIMER1,TIMER_FLAG_UP) ||
           timer_flag_get(TIMER1,TIMER_FLAG_CH2O) || timer_flag_get(TIMER1,TIMER_FLAG_CH3O))break;
        if(timer_flag_get(TIMER1,TIMER_FLAG_CH2) && timer_flag_get(TIMER1,TIMER_FLAG_CH3)) {
            uint32_t rise=timer_channel_capture_value_register_read(TIMER1,TIMER_CH_3);
            uint32_t fall=timer_channel_capture_value_register_read(TIMER1,TIMER_CH_2);
            if(fall>rise && fall-rise<=25000u && gpio_input_bit_get(GPIOA,GPIO_PIN_3)==RESET) {
                *pulse_us=fall-rise;rc=0;
            }
            break;
        }
        LOS_Msleep(1);
    }
    timer_disable(TIMER1);return rc==0?0:-1;
}
