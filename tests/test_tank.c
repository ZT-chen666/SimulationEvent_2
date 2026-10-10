#include "tank_sensor.h"
#include "agri_control.h"
#include <assert.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do {assert(x);checks++;}while(0)
static int level(unsigned mm)
{return tank_level_from_echo((mm*2000u+171u)/343u,100,600,50,0);}
int main(void)
{
    unsigned mm;int previous=100;uint16_t distance=99;
    AgControl c;AgSample s={0,250,600,30,60,90,AG_ALL,0,0,0};
    CHECK(level(100)==100);CHECK(level(350)==50);CHECK(level(600)==0);
    CHECK(level(50)==100);CHECK(level(650)==0);CHECK(level(49)==-1);CHECK(level(651)==-1);
    CHECK(tank_level_from_echo(0,100,600,50,&distance)==-1 && distance==0);
    CHECK(tank_level_from_echo(26000,100,600,50,0)==-1);
    CHECK(tank_level_from_echo(1000,600,100,50,0)==-1);
    CHECK(tank_level_from_echo(1000,100,100,50,0)==-1);
    CHECK(tank_level_from_echo(1000,100,600,100,0)==-1);
    CHECK(level(19)==-1);CHECK(level(4001)==-1);
    for(mm=100;mm<=600;mm++) {int n=level(mm);CHECK(n>=0 && n<=previous);previous=n;}
    ag_init(&c,0);
    for(mm=0;mm<=2500;mm+=50){s.at_ms=mm;s.water=(uint8_t)level(150);ag_accept(&c,&s);ag_step(&c,mm);}
    CHECK(c.pump);
    s.at_ms=2550;s.valid&=(uint8_t)~AG_WATER;ag_accept(&c,&s);ag_step(&c,2550);
    CHECK(!c.pump && (c.faults&AG_FAULT_WATER));
    s.at_ms=2600;s.valid=AG_ALL;s.water=(uint8_t)level(525);ag_accept(&c,&s);ag_step(&c,2600);
    CHECK(s.water==15 && !c.pump && (c.faults&AG_FAULT_DRY_TANK));
    s.at_ms=2650;s.water=(uint8_t)level(510);ag_accept(&c,&s);ag_step(&c,2650);
    CHECK(s.water==18 && (c.faults&AG_FAULT_DRY_TANK));
    s.at_ms=2700;s.water=(uint8_t)level(500);ag_accept(&c,&s);ag_step(&c,2700);
    CHECK(s.water==20 && !(c.faults&AG_FAULT_DRY_TANK));
    printf("TANK PASS: %u checks; missing echo stops pump; range and hysteresis verified\n",checks);
    return 0;
}
