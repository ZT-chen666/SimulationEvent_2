#ifndef AGRI_CONTROL_H
#define AGRI_CONTROL_H
#include <stdint.h>

/* Pure C policy: no hardware/RTOS dependencies; values are engineering demo
 * settings, not universal agronomic prescriptions. All time values are ms. */
#define AG_AIR   1u
#define AG_SOIL  2u
#define AG_LIGHT 4u
#define AG_WATER 8u
#define AG_ALL   15u

enum { AG_AUTO, AG_MANUAL };
enum { AG_SEEDLING, AG_TRANSPLANT, AG_PROFILE_COUNT };
enum { AG_ADAPTIVE, AG_THRESHOLD };
enum { AG_IDLE, AG_WATERING, AG_SOAK, AG_PROTECT };
enum { AG_CMD_NONE, AG_CMD_MODE, AG_CMD_PROFILE, AG_CMD_PUMP, AG_CMD_ACK, AG_CMD_POLICY };
enum { AG_REASON_IDLE, AG_REASON_DRY, AG_REASON_VPD, AG_REASON_SOAK,
       AG_REASON_SENSOR, AG_REASON_WATER, AG_REASON_LIMIT, AG_REASON_MANUAL,
       AG_REASON_TARGET, AG_REASON_BUDGET, AG_REASON_TIMING };
enum { AG_FAULT_STALE=1u, AG_FAULT_SOIL=2u, AG_FAULT_AIR=4u,
       AG_FAULT_LIGHT=8u, AG_FAULT_WATER=16u, AG_FAULT_DRY_TANK=32u,
       AG_FAULT_RUN_LIMIT=64u, AG_FAULT_BUDGET=128u, AG_FAULT_TIMING=256u };

typedef struct {
    uint32_t at_ms;
    int16_t temperature10;
    uint16_t humidity10;
    uint8_t soil, light, water, valid;
    uint16_t distance_mm, soil_adc, light_adc;
} AgSample;

typedef struct {
    uint8_t soil_start, soil_target, water_min, light_on, light_off;
    int16_t fan_on10, fan_off10;
    uint32_t min_off_ms, max_run_ms, soak_ms, hourly_budget_ms;
} AgProfile;

typedef struct {
    AgSample sample;
    uint32_t faults, sticky_faults, state_since, last_step, last_stop;
    uint32_t budget_start, hour_pump_ms, pulse_ms, transitions;
    uint64_t total_pump_ms;
    uint32_t pump_starts, fan_ms, light_ms, stress_ms;
    uint16_t vpd100, demand100;
    uint8_t mode, profile, policy, state, reason;
    uint8_t pump, fan, lamp, manual_request;
    uint8_t ever_sample, filtered, tank_empty;
} AgControl;

extern const AgProfile ag_profiles[AG_PROFILE_COUNT];
void ag_init(AgControl *c, uint32_t now);
void ag_accept(AgControl *c, const AgSample *sample);
void ag_command(AgControl *c, uint8_t command, uint32_t now);
void ag_step(AgControl *c, uint32_t now);
uint16_t ag_vpd100(int16_t temperature10, uint16_t humidity10);
uint16_t ag_demand100(uint8_t soil, uint8_t light, uint16_t vpd100, const AgProfile *p);
int ag_normalize(uint16_t raw, uint16_t dry, uint16_t wet);
int ag_decode_dht(const uint8_t frame[5], int16_t *temperature10, uint16_t *humidity10);
const char *ag_state_name(uint8_t s);
const char *ag_reason_name(uint8_t r);
#endif
