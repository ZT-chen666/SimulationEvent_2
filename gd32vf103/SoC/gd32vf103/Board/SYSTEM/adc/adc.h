/* PA1 soil, PA2 light, PA3 water. One triggered DMA frame.
 * 0 success, -1 error. An error leaves all output values invalid. */
#ifndef AGRI_ADC_H
#define AGRI_ADC_H
#include "gd32vf103.h"
int ADC0_DMA_Config(void);
int ADC0_Read_Frame(uint16_t raw[3]);
#endif
