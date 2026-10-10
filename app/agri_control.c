#include "agri_control.h"
#include <string.h>

/* Adjustable demonstration profiles; verify crop/substrate values separately. */
const AgProfile ag_profiles[AG_PROFILE_COUNT] = {
    {45,65,15,25,40,320,290,2000,6000,4000,120000},
    {40,60,15,25,40,330,300,2000,6000,5000,120000}
};

static uint32_t elapsed(uint32_t now, uint32_t then) { return now - then; }
static uint16_t clamp100(int v) { return (uint16_t)(v<0?0:v>100?100:v); }

int ag_decode_dht(const uint8_t f[5],int16_t *t,uint16_t *h)
{
    int temperature; uint16_t humidity;
    if(!f || !t || !h || (uint8_t)(f[0]+f[1]+f[2]+f[3])!=f[4])return -1;
    temperature=f[2]*10+(f[3]&0x7fu);
    if(f[3]&0x80u)temperature=-temperature;
    humidity=(uint16_t)(f[0]*10u+f[1]);
    if(temperature<0 || temperature>500 || humidity>1000)return -1;
    *t=(int16_t)temperature;*h=humidity;return 0;
}

int ag_normalize(uint16_t raw, uint16_t dry, uint16_t wet)
{
    int denominator=(int)wet-(int)dry;
    if (!denominator) return -1;
    return clamp100(((int)raw-(int)dry)*100/denominator);
}

/* Saturation vapour pressure table 0..50 C in 5 C steps, units 0.01 kPa.
 * Generated with 0.6108*exp(17.27*T/(T+237.3)), interpolated using integers.
 * Air temperature approximates leaf temperature; not a transpiration model. */
uint16_t ag_vpd100(int16_t temperature10, uint16_t humidity10)
{
    static const uint16_t es[]={61,87,123,171,234,317,424,562,738,959,1234};
    int t=temperature10, i, f; uint32_t saturation;
    if (t<0) t=0;
    if (t>500) t=500;
    if (humidity10>1000) humidity10=1000;
    i=t/50; f=t%50;
    saturation=es[i];
    if (i<10) saturation+=(uint32_t)(es[i+1]-es[i])*(uint32_t)f/50u;
    return (uint16_t)(saturation*(1000u-humidity10)/1000u);
}

/* Small zero-order fuzzy model: wet/dry membership blends low/high VPD
 * rules, modulated by light. Root-zone dryness is the dominant factor. */
uint16_t ag_demand100(uint8_t soil, uint8_t light, uint16_t vpd, const AgProfile *p)
{
    uint16_t dry=clamp100(((int)p->soil_target-soil)*100/((int)p->soil_target-p->soil_start));
    uint16_t atmospheric=clamp100(((int)vpd-80)*100/160);
    uint32_t dry_low=(uint32_t)dry*(100u-atmospheric);
    uint32_t dry_high=(uint32_t)dry*atmospheric;
    uint32_t wet_low=(uint32_t)(100u-dry)*(100u-atmospheric);
    uint32_t wet_high=(uint32_t)(100u-dry)*atmospheric;
    uint32_t result=(dry_low*65u+dry_high*95u+wet_low*0u+wet_high*15u)/10000u;
    result=result*(80u+light/5u)/100u;
    return (uint16_t)result;
}

void ag_init(AgControl *c, uint32_t now)
{
    memset(c,0,sizeof(*c));
    c->last_step=c->last_stop=c->state_since=c->budget_start=now;
    c->state=AG_PROTECT;c->reason=AG_REASON_SENSOR;
}

static uint8_t avg(uint8_t previous,uint8_t next) { return (uint8_t)((previous+3u*next+2u)/4u); }
void ag_accept(AgControl *c, const AgSample *sample)
{
    AgSample s=*sample;
    uint8_t previous_valid=c->sample.valid;
    if (s.temperature10<0 || s.temperature10>500 || s.humidity10>1000) s.valid&=(uint8_t)~AG_AIR;
    if (s.soil>100) s.valid&=(uint8_t)~AG_SOIL;
    if (s.light>100) s.valid&=(uint8_t)~AG_LIGHT;
    if (s.water>100) s.valid&=(uint8_t)~AG_WATER;
    if (c->ever_sample && (int32_t)(s.at_ms-c->sample.at_ms)<0) return;
    if (c->filtered) {
        if ((s.valid&previous_valid&AG_SOIL)!=0) s.soil=avg(c->sample.soil,s.soil);
        if ((s.valid&previous_valid&AG_LIGHT)!=0) s.light=avg(c->sample.light,s.light);
        /* Never smooth low water upwards: safety uses latest unfiltered water. */
    }
    c->sample=s;c->ever_sample=1;c->filtered=1;
}

static void set_state(AgControl *c,uint8_t state,uint8_t reason,uint32_t now)
{
    if (c->state!=state) { c->state=state;c->state_since=now;c->transitions++; }
    c->reason=reason;
}

static void stop_pump(AgControl *c,uint32_t now)
{
    if (c->pump) c->last_stop=now;
    c->pump=0;
}

void ag_command(AgControl *c,uint8_t command,uint32_t now)
{
    if (command==AG_CMD_MODE || command==AG_CMD_PROFILE || command==AG_CMD_POLICY) {
        stop_pump(c,now);c->manual_request=0;
        if (command==AG_CMD_MODE)c->mode=(uint8_t)!c->mode;
        if (command==AG_CMD_PROFILE)c->profile=(uint8_t)((c->profile+1u)%AG_PROFILE_COUNT);
        if (command==AG_CMD_POLICY)c->policy=(uint8_t)!c->policy;
        set_state(c,AG_IDLE,AG_REASON_IDLE,now);
    } else if (command==AG_CMD_PUMP && c->mode==AG_MANUAL) {
        c->manual_request=(uint8_t)!c->manual_request;
        if (!c->manual_request) { stop_pump(c,now);set_state(c,AG_IDLE,AG_REASON_MANUAL,now); }
    } else if (command==AG_CMD_ACK) {
        /* Conditions are re-evaluated by ag_step; ACK cannot bypass live faults. */
        c->sticky_faults=0;c->manual_request=0;
        stop_pump(c,now);c->last_stop=now;
        set_state(c,AG_IDLE,AG_REASON_IDLE,now);
    }
}

void ag_step(AgControl *c,uint32_t now)
{
    const AgProfile *p=&ag_profiles[c->profile];
    uint32_t dt=elapsed(now,c->last_step),fresh;
    uint32_t critical;
    c->last_step=now;
    if (elapsed(now,c->budget_start)>=3600000u) { c->budget_start=now;c->hour_pump_ms=0; }
    if (c->pump) { c->total_pump_ms+=dt;c->hour_pump_ms+=dt; }
    if (c->fan)c->fan_ms+=dt;
    if (c->lamp)c->light_ms+=dt;
    if (dt>250u) c->sticky_faults|=AG_FAULT_TIMING;
    fresh=c->ever_sample && elapsed(now,c->sample.at_ms)<=5000u;
    c->faults=c->sticky_faults;
    if (!fresh)c->faults|=AG_FAULT_STALE;
    if (!fresh || !(c->sample.valid&AG_SOIL))c->faults|=AG_FAULT_SOIL;
    if (!fresh || !(c->sample.valid&AG_AIR))c->faults|=AG_FAULT_AIR;
    if (!fresh || !(c->sample.valid&AG_LIGHT))c->faults|=AG_FAULT_LIGHT;
    if (!fresh || !(c->sample.valid&AG_WATER))c->faults|=AG_FAULT_WATER;
    if (fresh && (c->sample.valid&AG_WATER)) {
        if (c->sample.water<=p->water_min)c->tank_empty=1;
        if (c->sample.water>=p->water_min+5)c->tank_empty=0;
    }
    if (c->tank_empty)c->faults|=AG_FAULT_DRY_TANK;
    if (c->hour_pump_ms>=p->hourly_budget_ms)c->faults|=AG_FAULT_BUDGET;
    if (c->pump && elapsed(now,c->state_since)>=p->max_run_ms) {
        c->sticky_faults|=AG_FAULT_RUN_LIMIT;c->faults|=AG_FAULT_RUN_LIMIT;
    }
    if (c->faults&AG_FAULT_AIR) { c->fan=1;c->vpd100=0; }
    else {
        c->vpd100=ag_vpd100(c->sample.temperature10,c->sample.humidity10);
        if (c->sample.temperature10>=p->fan_on10)c->fan=1;
        if (c->sample.temperature10<=p->fan_off10)c->fan=0;
    }
    if (c->faults&AG_FAULT_LIGHT)c->lamp=0;
    else {
        if (c->sample.light<=p->light_on)c->lamp=1;
        if (c->sample.light>=p->light_off)c->lamp=0;
    }
    c->demand100=0;
    if (!(c->faults&AG_FAULT_SOIL)) {
        c->demand100=ag_demand100(c->sample.soil,
            (c->faults&AG_FAULT_LIGHT)?0:c->sample.light,c->vpd100,p);
        if(c->sample.soil<p->soil_start || c->sample.soil>p->soil_target)c->stress_ms+=dt;
    }
    critical=c->faults&(AG_FAULT_STALE|AG_FAULT_SOIL|AG_FAULT_WATER|AG_FAULT_DRY_TANK|
                         AG_FAULT_RUN_LIMIT|AG_FAULT_BUDGET|AG_FAULT_TIMING);
    if (critical) {
        stop_pump(c,now);c->manual_request=0;
        set_state(c,AG_PROTECT,(critical&AG_FAULT_TIMING)?AG_REASON_TIMING:
            (critical&AG_FAULT_DRY_TANK)?AG_REASON_WATER:
            (critical&AG_FAULT_RUN_LIMIT)?AG_REASON_LIMIT:
            (critical&AG_FAULT_BUDGET)?AG_REASON_BUDGET:AG_REASON_SENSOR,now);
        return;
    }
    if(c->state==AG_PROTECT) {set_state(c,AG_IDLE,AG_REASON_IDLE,now);c->last_stop=now;}
    if(c->pump) {
        if(c->mode==AG_AUTO && c->sample.soil>=p->soil_target) {
            stop_pump(c,now);set_state(c,AG_SOAK,AG_REASON_TARGET,now);
        } else if(c->mode==AG_AUTO && elapsed(now,c->state_since)>=
                  (c->policy==AG_ADAPTIVE?c->pulse_ms:p->max_run_ms-500u)) {
            stop_pump(c,now);set_state(c,AG_SOAK,AG_REASON_SOAK,now);
        }
        return;
    }
    if(c->state==AG_SOAK && elapsed(now,c->state_since)<p->soak_ms) {c->reason=AG_REASON_SOAK;return;}
    if(c->state==AG_SOAK)set_state(c,AG_IDLE,AG_REASON_IDLE,now);
    if(elapsed(now,c->last_stop)<p->min_off_ms)return;
    if((c->mode==AG_MANUAL && c->manual_request) ||
       (c->mode==AG_AUTO && (c->sample.soil<=p->soil_start ||
         (c->policy==AG_ADAPTIVE && c->demand100>=60 && c->sample.soil<p->soil_target-5)))) {
        c->pulse_ms=500u+(uint32_t)c->demand100*15u;
        c->pump=1;c->pump_starts++;
        set_state(c,AG_WATERING,c->mode==AG_MANUAL?AG_REASON_MANUAL:
                  c->sample.soil<=p->soil_start?AG_REASON_DRY:AG_REASON_VPD,now);
    } else {set_state(c,AG_IDLE,AG_REASON_IDLE,now);}
}

const char *ag_state_name(uint8_t s)
{ static const char *n[]={"IDLE","WATER","SOAK","PROTECT"};return s<4?n[s]:"?"; }
const char *ag_reason_name(uint8_t r)
{ static const char *n[]={"READY","SOIL_DRY","HIGH_VPD","SOAK_WAIT","SENSOR_FAIL","TANK_LOW","RUN_LIMIT","MANUAL","TARGET","BUDGET","TASK_LATE"};return r<11?n[r]:"?"; }
