#include "board_port.h"
#include "adc.h"
#include "dht11.h"
#include "lcd_driver.h"
#include "lcd_calculate.h"
#include "ultrasonic.h"
#include "tank_sensor.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
static int adc_ready;
static int fan_pwm_ready;
static void fan_pwm_init(void)
{
    timer_parameter_struct t;timer_oc_parameter_struct oc;
    uint32_t apb=rcu_clock_freq_get(CK_APB1);
    uint32_t hz=apb==rcu_clock_freq_get(CK_AHB)?apb:apb*2u;
    if(hz<1000000u || hz%1000000u)return;
    rcu_periph_clock_enable(RCU_TIMER2);
    timer_deinit(TIMER2);timer_struct_para_init(&t);
    t.prescaler=(uint16_t)(hz/1000000u-1u);t.period=999u;timer_init(TIMER2,&t);
    timer_channel_output_struct_para_init(&oc);oc.outputstate=TIMER_CCX_ENABLE;
    oc.ocpolarity=TIMER_OC_POLARITY_HIGH;
    timer_channel_output_config(TIMER2,TIMER_CH_0,&oc);
    timer_channel_output_mode_config(TIMER2,TIMER_CH_0,TIMER_OC_MODE_PWM0);
    timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_0,0);
    timer_channel_output_shadow_config(TIMER2,TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);
    gpio_init(GPIOA,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_6);
    timer_enable(TIMER2);fan_pwm_ready=1;
}

void board_safe_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);rcu_periph_clock_enable(RCU_GPIOB);
    gpio_bit_reset(GPIOA,GPIO_PIN_6|GPIO_PIN_7);gpio_bit_reset(GPIOB,GPIO_PIN_0);
    gpio_init(GPIOA,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_6|GPIO_PIN_7);
    gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_0);
    gpio_init(GPIOB,GPIO_MODE_IPU,GPIO_OSPEED_50MHZ,GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14);
    uart_init(115200);
    fan_pwm_init();
}
void board_all_off(void)
{
    if(fan_pwm_ready)timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_0,0);
    gpio_bit_reset(GPIOA,GPIO_PIN_6|GPIO_PIN_7);gpio_bit_reset(GPIOB,GPIO_PIN_0);
}
void board_sensor_init(void) { adc_ready=(ADC0_DMA_Config()==0);(void)ultrasonic_init(); }
void board_read_sample(AgSample *s,uint32_t now)
{
    int t=0,h=0,value;uint16_t raw[2];uint32_t pulse_us=0;
    memset(s,0,sizeof(*s));s->at_ms=now;
    raw[0]=raw[1]=0xffffu;
    if(adc_ready)(void)ADC0_Read_Frame(raw);
    s->soil_adc=raw[0];s->light_adc=raw[1];
    if(raw[0]<=4095u && (value=ag_normalize(raw[0],AG_SOIL_DRY_ADC,AG_SOIL_WET_ADC))>=0) {
        s->soil=(uint8_t)value;s->valid|=AG_SOIL;
    }
    if(raw[1]<=4095u && (value=ag_normalize(raw[1],AG_LIGHT_DARK_ADC,AG_LIGHT_BRIGHT_ADC))>=0) {
        s->light=(uint8_t)value;s->valid|=AG_LIGHT;
    }
    if(ultrasonic_read(&pulse_us)==0) {
        value=tank_level_from_echo(pulse_us,AG_TANK_FULL_MM,AG_TANK_EMPTY_MM,AG_TANK_MARGIN_MM,&s->distance_mm);
        if(value>=0){s->water=(uint8_t)value;s->valid|=AG_WATER;}
    }
    if(DHT11_Read_Data(&t,&h)==0) {
        s->temperature10=(int16_t)t;s->humidity10=(uint16_t)h;s->valid|=AG_AIR;
    }
}
static void pin(uint32_t port,uint32_t number,uint8_t on)
{if(on)gpio_bit_set(port,number);else gpio_bit_reset(port,number);}
void board_apply(const AgControl *c)
{
    if(fan_pwm_ready)timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_0,c->fan?800u:0u);
    pin(GPIOA,GPIO_PIN_7,c->lamp);pin(GPIOB,GPIO_PIN_0,c->pump);
}
uint8_t board_buttons(void)
{
    return (gpio_input_bit_get(GPIOB,GPIO_PIN_12)==RESET?1u:0u)|
           (gpio_input_bit_get(GPIOB,GPIO_PIN_13)==RESET?2u:0u)|
           (gpio_input_bit_get(GPIOB,GPIO_PIN_14)==RESET?4u:0u);
}
void board_display_init(void)
{
    rcu_periph_clock_enable(RCU_AF);
    /* TFT uses PA15/PB4, otherwise reserved by the debug port after reset. */
    gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP,ENABLE);
    Lcd_Init();Lcd_Clear(BLACK);
}
static void row(uint16_t y,const char *text,uint16_t color)
{
    char padded[17];unsigned i;
    memset(padded,' ',16);padded[16]=0;strncpy(padded,text,16);
    for(i=0;i<16;i++)if(padded[i]==0)padded[i]=' ';
    static char previous[9][17];
    static uint16_t colors[9];
    static unsigned cursor=0;
    unsigned index=y/18u;
    if(index==cursor && (strcmp(previous[index],padded)!=0 || colors[index]!=color)) {
        extern const unsigned char asc16[];
        unsigned py,px,k;
        /* One address window per row, 128*16 pixels; no per-pixel commands. */
        Lcd_SetRegion(0,y,127,y+15);
        for(py=0;py<16;py++)for(k=0;k<16;k++) {
            unsigned ch=(unsigned char)padded[k];
            unsigned bits=asc16[(ch>=32 && ch<128?ch-32:0)*16+py];
            for(px=0;px<8;px++)LCD_WriteData_16Bit((bits&(0x80u>>px))?color:BLACK);
        }
        memcpy(previous[index],padded,sizeof(padded));colors[index]=color;
    }
    if(index==8)cursor=(cursor+1u)%9u;
}
static void display_rows(const AgControl *c)
{
    char b[48];row(0,"PEPPER GUARD",GREEN);
    snprintf(b,sizeof(b),"%s %s",c->mode?"MAN":"AUTO",c->profile?"TRANS":"SEED");row(18,b,WHITE);
    snprintf(b,sizeof(b),"T:%d.%d H:%u",c->sample.temperature10/10,c->sample.temperature10%10,c->sample.humidity10/10);row(36,b,WHITE);
    snprintf(b,sizeof(b),"S%u L%u W%u",c->sample.soil,c->sample.light,c->sample.water);row(54,b,WHITE);
    snprintf(b,sizeof(b),"VPD:%u.%02u D:%u",c->vpd100/100,c->vpd100%100,c->demand100);row(72,b,YELLOW);
    snprintf(b,sizeof(b),"%s %s",ag_state_name(c->state),c->policy?"BASE":"FUZZY");row(90,b,YELLOW);
    snprintf(b,sizeof(b),"P:%u F:%u B:%u",c->pump,c->fan,c->lamp);row(108,b,GREEN);
    row(126,ag_reason_name(c->reason),c->faults?RED:WHITE);
    snprintf(b,sizeof(b),"ERR:%03lX n:%lu",(unsigned long)c->faults,(unsigned long)c->pump_starts);row(144,b,c->faults?RED:WHITE);
}

void board_display(const AgControl *c) { display_rows(c); }
