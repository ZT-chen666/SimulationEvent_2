/* Bounded GD32VF103 DHT11 driver derived from the supplied teaching driver.
 * PA0. Start pulse sleeps; bounded response/data burst masks interrupts.
 * Interrupt mask duration is bounded by timeouts (~10 ms worst case).
 * Validate simulator timing; never disable interrupts over the 20 ms pulse. */
#include "dht11.h"
#include "systick.h"
#include "los_task.h"
#include "los_interrupt.h"
#include "agri_control.h"
static int wait_level(uint8_t level)
{
    unsigned n;for(n=0;n<120;n++) {
        if((uint8_t)DHT11_DQ_IN==level)return 0;
        delay_us(1);
    }return -1;
}
void DHT11_Rst(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);DHT11_IO_OUT();
    gpio_bit_reset(GPIOA,GPIO_PIN_0);LOS_Msleep(20);
    gpio_bit_set(GPIOA,GPIO_PIN_0);delay_us(20);DHT11_IO_IN();
}
u8 DHT11_Check(void)
{return (wait_level(0)||wait_level(1)||wait_level(0))?1:0;}
static int read_bit(uint8_t *value)
{
    if(wait_level(1))return -1;
    delay_us(40);
    *value=(uint8_t)DHT11_DQ_IN;return wait_level(0);
}
u8 DHT11_Read_Bit(void) {uint8_t v=0;(void)read_bit(&v);return v;}
u8 DHT11_Read_Byte(void)
{unsigned i;uint8_t v=0,b;for(i=0;i<8;i++){if(read_bit(&b))return 0;v=(uint8_t)((v<<1)|b);}return v;}
u8 DHT11_Init(void)
{
    UINT32 saved;u8 rc;DHT11_Rst();saved=LOS_IntLock();
    rc=DHT11_Check();LOS_IntRestore(saved);return rc;
}
u8 DHT11_Read_Data(int *temp,int *humi)
{
    uint8_t bytes[5]={0};unsigned i,j;int fail=0;
    int16_t t;uint16_t h;UINT32 saved;
    if(!temp || !humi)return 1;
    DHT11_Rst();saved=LOS_IntLock();
    if(DHT11_Check())fail=1;
    for(i=0;i<5 && !fail;i++)for(j=0;j<8;j++) {
        uint8_t b;if(read_bit(&b)){fail=1;break;}
        bytes[i]=(uint8_t)((bytes[i]<<1)|b);
    }
    LOS_IntRestore(saved);
    if(fail || ag_decode_dht(bytes,&t,&h))return 1;
    *temp=t;*humi=h;return 0;
}
