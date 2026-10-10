#include "agri_control.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned assertions,scenarios;
#define CHECK(x) do {assert(x);assertions++;}while(0)
static AgSample sample(uint32_t t,unsigned soil,unsigned water)
{AgSample s={t,250,600,(uint8_t)soil,60,(uint8_t)water,AG_ALL};return s;}
static void tick(AgControl *c,uint32_t t,unsigned soil,unsigned water)
{AgSample s=sample(t,soil,water);ag_accept(c,&s);ag_step(c,t);}
static void ready(AgControl *c,unsigned soil)
{uint32_t t;ag_init(c,0);for(t=0;t<=2500;t+=50)tick(c,t,soil,90);}

static void test_startup(void)
{
    AgControl c;ag_init(&c,0);ag_step(&c,0);
    CHECK(c.pump==0 && c.state==AG_PROTECT);CHECK(c.faults&AG_FAULT_STALE);
    ready(&c,70);CHECK(!c.pump && c.state==AG_IDLE);scenarios++;
}
static void test_soil_primary(void)
{
    AgControl c;AgSample s;uint32_t t;ready(&c,80);
    for(t=2550;t<4000;t+=50) {s=sample(t,80,90);s.temperature10=450;s.humidity10=50;ag_accept(&c,&s);ag_step(&c,t);}
    CHECK(!c.pump);CHECK(c.fan);CHECK(c.vpd100>700);scenarios++;
}
static void test_pulse_soak(void)
{
    AgControl c;uint32_t t;ready(&c,30);CHECK(c.pump);
    for(t=2550;t<=4500;t+=50)tick(&c,t,30,90);
    CHECK(!c.pump && c.state==AG_SOAK);CHECK(c.pump_starts==1);
    for(t=4550;t<=10000;t+=50)tick(&c,t,30,90);
    CHECK(c.pump_starts>=2);scenarios++;
}
static void test_water_priority(void)
{
    AgControl c;ready(&c,30);CHECK(c.pump);
    tick(&c,2550,30,0);CHECK(!c.pump && c.faults&AG_FAULT_DRY_TANK);
    ag_command(&c,AG_CMD_ACK,2550);ag_step(&c,2550);CHECK(!c.pump);
    tick(&c,2600,30,18);CHECK(c.faults&AG_FAULT_DRY_TANK);
    tick(&c,2650,30,25);CHECK(!(c.faults&AG_FAULT_DRY_TANK));scenarios++;
}
static void test_stale(void)
{
    AgControl c;uint32_t t;ready(&c,30);
    for(t=2550;t<=8000;t+=50)ag_step(&c,t);
    CHECK(!c.pump && c.faults&AG_FAULT_STALE);
    tick(&c,8050,30,90);CHECK(!c.pump);scenarios++;
}
static void test_manual_limit(void)
{
    AgControl c;uint32_t t;ready(&c,70);ag_command(&c,AG_CMD_MODE,2500);ag_command(&c,AG_CMD_PUMP,2500);
    for(t=2550;t<=4500;t+=50)tick(&c,t,70,90);
    CHECK(c.pump && c.mode==AG_MANUAL);
    for(t=4550;t<=11000;t+=50)tick(&c,t,70,90);
    CHECK(!c.pump && c.sticky_faults&AG_FAULT_RUN_LIMIT);
    ag_command(&c,AG_CMD_PUMP,11000);ag_step(&c,11000);CHECK(!c.pump);
    ag_command(&c,AG_CMD_ACK,11000);ag_step(&c,11000);CHECK(!c.pump && !c.sticky_faults);scenarios++;
}
static void test_sensor_degrade(void)
{
    AgControl c;AgSample s;ready(&c,30);s=sample(2550,30,90);s.valid&=(uint8_t)~AG_AIR;
    ag_accept(&c,&s);ag_step(&c,2550);CHECK(c.faults&AG_FAULT_AIR);CHECK(c.fan);
    s=sample(2600,30,90);s.valid&=(uint8_t)~AG_SOIL;ag_accept(&c,&s);ag_step(&c,2600);
    CHECK(!c.pump && c.faults&AG_FAULT_SOIL);scenarios++;
}
static void test_range(void)
{
    AgControl c;AgSample s;ready(&c,30);s=sample(2550,250,90);s.temperature10=800;s.light=255;
    ag_accept(&c,&s);ag_step(&c,2550);CHECK(!c.pump);CHECK(!c.lamp);CHECK(c.faults&AG_FAULT_AIR);scenarios++;
}
static void test_hysteresis(void)
{
    AgControl c;AgSample s;uint32_t t;ready(&c,70);
    for(t=2550;t<4000;t+=50){s=sample(t,70,90);s.temperature10=325;s.light=10;ag_accept(&c,&s);ag_step(&c,t);}
    CHECK(c.fan && c.lamp);
    for(t=4000;t<6000;t+=50){s=sample(t,70,90);s.temperature10=310;s.light=32;ag_accept(&c,&s);ag_step(&c,t);}
    CHECK(c.fan && c.lamp);
    for(t=6000;t<8000;t+=50){s=sample(t,70,90);s.temperature10=285;s.light=65;ag_accept(&c,&s);ag_step(&c,t);}
    CHECK(!c.fan && !c.lamp);scenarios++;
}
static void test_stage_mode(void)
{
    AgControl c;ready(&c,30);CHECK(c.pump);ag_command(&c,AG_CMD_PROFILE,2500);
    ag_step(&c,2500);CHECK(!c.pump && c.profile==AG_TRANSPLANT);
    ag_command(&c,AG_CMD_MODE,2500);ag_step(&c,2500);CHECK(!c.pump && !c.manual_request);scenarios++;
}
static void test_late_task(void)
{
    AgControl c;ready(&c,30);tick(&c,4000,30,90);
    CHECK(!c.pump && c.sticky_faults&AG_FAULT_TIMING);scenarios++;
}
static void test_rollover(void)
{
    AgControl c;uint32_t origin=0xfffff800u,i;ag_init(&c,origin);
    for(i=0;i<4000;i+=50)tick(&c,origin+i,30,90);
    CHECK(c.pump_starts>=1);CHECK(!(c.faults&AG_FAULT_STALE));CHECK(!(c.faults&AG_FAULT_TIMING));scenarios++;
}
static void test_budget(void)
{
    AgControl c;ready(&c,30);c.hour_pump_ms=ag_profiles[c.profile].hourly_budget_ms;
    tick(&c,2550,30,90);CHECK(!c.pump && c.faults&AG_FAULT_BUDGET);
    ag_command(&c,AG_CMD_ACK,2550);ag_step(&c,2550);CHECK(!c.pump);scenarios++;
}
static void test_decode(void)
{
    uint8_t good[5]={60,0,25,0,85},bad[5]={60,0,25,0,84};int16_t t=999;uint16_t h=999;
    CHECK(ag_decode_dht(good,&t,&h)==0 && t==250 && h==600);
    t=111;h=222;CHECK(ag_decode_dht(bad,&t,&h)!=0);CHECK(t==111 && h==222);
    CHECK(ag_normalize(4095,4095,0)==0);CHECK(ag_normalize(0,4095,0)==100);
    CHECK(ag_normalize(2048,0,4095)==50);CHECK(ag_normalize(100,100,100)<0);
    CHECK(ag_vpd100(250,600)>=126 && ag_vpd100(250,600)<=127);
    CHECK(ag_vpd100(250,1000)==0);
    CHECK(ag_demand100(40,90,240,&ag_profiles[0])>ag_demand100(40,20,60,&ag_profiles[0]));scenarios++;
}
static uint32_t seed=314159u;
static uint32_t random32(void){seed=seed*1664525u+1013904223u;return seed;}
static void test_random(void)
{
    AgControl c;uint32_t i,t=0;ag_init(&c,0);
    for(i=0;i<200000;i++) {
        AgSample s;uint32_t critical;
        t+=50;s=sample(t,random32()%101u,random32()%101u);
        s.temperature10=(int16_t)(random32()%501u);s.humidity10=(uint16_t)(random32()%1001u);
        s.light=(uint8_t)(random32()%101u);
        if((random32()%20u)==0)s.valid=(uint8_t)(random32()&15u);
        if(i%7==0)ag_accept(&c,&s);
        if(i%23==0)ag_command(&c,(uint8_t)(1u+random32()%6u),t);
        ag_step(&c,t);
        critical=c.faults&(AG_FAULT_STALE|AG_FAULT_SOIL|AG_FAULT_WATER|AG_FAULT_DRY_TANK|
                AG_FAULT_RUN_LIMIT|AG_FAULT_BUDGET|AG_FAULT_TIMING);
        CHECK(!critical || !c.pump);CHECK(c.pump<=1 && c.fan<=1 && c.lamp<=1);
        CHECK(c.state<=AG_PROTECT && c.demand100<=100);
        CHECK(!c.pump || c.state==AG_WATERING);
    }
    scenarios++;
}
int main(void)
{
    test_startup();test_soil_primary();test_pulse_soak();test_water_priority();test_stale();
    test_manual_limit();test_sensor_degrade();test_range();test_hysteresis();test_stage_mode();
    test_late_task();test_rollover();test_budget();test_decode();test_random();
    printf("PASS scenarios=%u assertions=%u random_steps=200000\n",scenarios,assertions);return 0;
}
