#ifndef TANK_SENSOR_H
#define TANK_SENSOR_H
#include <stdint.h>
/* Echo round-trip time -> relative level. -1 rejects missing/out-of-tank data. */
int tank_level_from_echo(uint32_t pulse_us, uint16_t full_mm,
                         uint16_t empty_mm, uint16_t margin_mm, uint16_t *distance_mm);
#endif
