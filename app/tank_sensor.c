#include "tank_sensor.h"
int tank_level_from_echo(uint32_t pulse_us, uint16_t full_mm,
                         uint16_t empty_mm, uint16_t margin_mm, uint16_t *distance_mm)
{
    uint32_t mm;
    if(distance_mm)*distance_mm=0;
    if(full_mm<20u || empty_mm>4000u || empty_mm<=full_mm ||
       margin_mm>=full_mm || pulse_us==0u || pulse_us>25000u)return -1;
    /* 343 m/s at approximately 20 C. No claimed temperature compensation. */
    mm=(pulse_us*343u+1000u)/2000u;
    if(mm<20u || mm>4000u)return -1;
    if(distance_mm)*distance_mm=(uint16_t)mm;
    if(mm<(uint32_t)full_mm-margin_mm || mm>(uint32_t)empty_mm+margin_mm)return -1;
    if(mm<=full_mm)return 100;
    if(mm>=empty_mm)return 0;
    return (int)(((uint32_t)empty_mm-mm)*100u/(empty_mm-full_mm));
}
