#ifndef BOARD_PORT_H
#define BOARD_PORT_H
#include "agri_control.h"
#include "gd32vf103.h"
/* Calibrate endpoints in the simulator. Percentages are normalized signals. */
#define AG_SOIL_DRY_ADC 4095u
#define AG_SOIL_WET_ADC 0u
#define AG_LIGHT_DARK_ADC 0u
#define AG_LIGHT_BRIGHT_ADC 4095u
/* Example regular tank geometry; calibrate actual probe-to-liquid distances. */
#define AG_TANK_FULL_MM 100u
#define AG_TANK_EMPTY_MM 600u
#define AG_TANK_MARGIN_MM 50u
#define AG_SAMPLE_PERIOD_MS 2000u
#define AG_CONTROL_PERIOD_MS 50u
#define AG_UI_PERIOD_MS 20u
#define AG_RENDER_PERIOD_MS 1000u
void board_safe_init(void);
void board_sensor_init(void);
void board_read_sample(AgSample *s, uint32_t now);
void board_apply(const AgControl *c);
void board_all_off(void);
uint8_t board_buttons(void);
void board_display_init(void);
void board_display(const AgControl *c);
#endif
