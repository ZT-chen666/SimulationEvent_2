#ifndef ULTRASONIC_H
#define ULTRASONIC_H
#include <stdint.h>
int ultrasonic_init(void);
/* PA4 -> IN (trigger), PA3 <- OUT (echo). 0 success; otherwise invalid. */
int ultrasonic_read(uint32_t *pulse_us);
#endif
