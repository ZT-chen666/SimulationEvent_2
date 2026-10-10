#include "board_port.h"
#include "adc.h"
#include "dht11.h"
#include "lcd_driver.h"
#include "lcd_calculate.h"
#include <stdio.h>
#include <string.h>
static int adc_ready;

void board_safe_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);rcu_periph_clock_enable(RCU_GPIOB);
    gpio_bit_reset(GPIOA,GPIO_PIN_6|GPIO_PIN_7);gpio_bit_reset(GPIOB,GPIO_PIN_0);
    gpio_init(GPIOA,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_6|GPIO_PIN_7);
    gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_0);
    gpio_init(GPIOB,GPIO_MODE_IPU,GPIO_OSPEED_50MHZ,GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14);
}
void board_sensor_init(void) { adc_ready=(ADC0_DMA_Config()==0); }
void board_read_sample(AgSample *s,uint32_t now)
{
    int t=0,h=0,value;uint16_t raw[3];
    memset(s,0,sizeof(*s));s->at_ms=now;
    raw[0]=raw[1]=raw[2]=0xffffu;
    if(adc_ready)(void)ADC0_Read_Frame(raw);
    if(raw[0]<=4095u && (value=ag_normalize(raw[0],AG_SOIL_DRY_ADC,AG_SOIL_WET_ADC))>=0) {
        s->soil=(uint8_t)value;s->valid|=AG_SOIL;
    }
    if(raw[1]<=4095u && (value=ag_normalize(raw[1],AG_LIGHT_DARK_ADC,AG_LIGHT_BRIGHT_ADC))>=0) {
        s->light=(uint8_t)value;s->valid|=AG_LIGHT;
    }
    if(raw[2]<=4095u && (value=ag_normalize(raw[2],AG_WATER_EMPTY_ADC,AG_WATER_FULL_ADC))>=0) {
        s->water=(uint8_t)value;s->valid|=AG_WATER;
    }
    if(DHT11_Read_Data(&t,&h)==0) {
        s->temperature10=(int16_t)t;s->humidity10=(uint16_t)h;s->valid|=AG_AIR;
    }
}
static void pin(uint32_t port,uint32_t number,uint8_t on)
{if(on)gpio_bit_set(port,number);else gpio_bit_reset(port,number);}
void board_apply(const AgControl *c)
{pin(GPIOA,GPIO_PIN_6,c->fan);pin(GPIOA,GPIO_PIN_7,c->lamp);pin(GPIOB,GPIO_PIN_0,c->pump);}
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
    char padded[18];unsigned i;
    memset(padded,' ',17);padded[17]=0;strncpy(padded,text,17);
    for(i=0;i<17;i++)if(padded[i]==0)padded[i]=' ';
    static char previous[9][18];
    static uint16_t colors[9];
    static unsigned cursor=0;
    unsigned index=y/18u;
    if(index==cursor && (strcmp(previous[index],padded)!=0 || colors[index]!=color)) {
        extern const unsigned char asc16[];
        unsigned py,px,k;
        /* One address window per row, 136*16 pixels; no per-pixel commands. */
        Lcd_SetRegion(0,y,135,y+15);
        for(py=0;py<16;py++)for(k=0;k<17;k++) {
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
    snprintf(b,sizeof(b),"S:%u L:%u W:%u",c->sample.soil,c->sample.light,c->sample.water);row(54,b,WHITE);
    snprintf(b,sizeof(b),"VPD:%u.%02u D:%u",c->vpd100/100,c->vpd100%100,c->demand100);row(72,b,YELLOW);
    snprintf(b,sizeof(b),"%s %s",ag_state_name(c->state),c->policy?"BASE":"FUZZY");row(90,b,YELLOW);
    snprintf(b,sizeof(b),"P:%u F:%u B:%u",c->pump,c->fan,c->lamp);row(108,b,GREEN);
    row(126,ag_reason_name(c->reason),c->faults?RED:WHITE);
    snprintf(b,sizeof(b),"ERR:%03lX n:%lu",(unsigned long)c->faults,(unsigned long)c->pump_starts);row(144,b,c->faults?RED:WHITE);
}

void board_display(const AgControl *c) { display_rows(c); }
