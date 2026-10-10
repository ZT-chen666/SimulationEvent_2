/* Reproducible SELF-BUILT environment model. This is not Puliedu simulator
 * output or real crop data. No absolute water-saving claim is derived from it. */
#include "agri_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc,char **argv)
{
    AgControl c;uint32_t t;unsigned policy=argc>1?(unsigned)atoi(argv[1]):0;
    double soil=42,stored=0,tank=90,temp=27,rh=65,light=65,overshoot=0;
    double pump_gain=argc>3?atof(argv[3]):7.0;
    if(policy>1 || pump_gain<=0 || pump_gain>50) return 2;
    FILE *trace=argc>2?fopen(argv[2],"w"):NULL;
    ag_init(&c,0);c.policy=(uint8_t)policy;
    if(trace)fprintf(trace,"time_ms,soil,water,temp,rh,light,pump,fan,lamp,state,reason,vpd100,demand100,faults\n");
    for(t=0;t<720000u;t+=50) {
        /* 50 ms integration, delayed moisture infiltration, evap varies with climate. */
        double dt=0.05,infiltration=stored*dt/3.0;
        temp=27+7*sin((double)t/150000.0);
        rh=62-15*sin((double)t/150000.0);
        light=50+35*sin((double)t/100000.0);
        stored+=c.pump*pump_gain*dt-infiltration;
        soil+=infiltration-(0.22+0.06*(temp-27)/7)*dt;
        tank-=c.pump*0.04*dt;
        if(soil>100)soil=100;
        if(soil<0)soil=0;
        if(t%2000u==0) {
            AgSample s={t,(int16_t)(temp*10),(uint16_t)(rh*10),(uint8_t)(soil+0.5),
                (uint8_t)(light+0.5),(uint8_t)(tank+0.5),AG_ALL};ag_accept(&c,&s);
        }
        ag_step(&c,t);
        if(soil>ag_profiles[c.profile].soil_target)overshoot+=dt;
        if(trace && t%1000u==0)fprintf(trace,"%u,%.3f,%.3f,%.2f,%.2f,%.2f,%u,%u,%u,%u,%u,%u,%u,%u\n",
            t,soil,tank,temp,rh,light,c.pump,c.fan,c.lamp,c.state,c.reason,c.vpd100,c.demand100,c.faults);
    }
    if(trace)fclose(trace);
    printf("{\"policy\":\"%s\",\"model_seconds\":720,\"pump_gain\":%.1f,\"pump_ms\":%llu,\"pump_starts\":%u,\"sensor_stress_ms\":%u,\"model_above_target_seconds\":%.2f,\"final_soil\":%.2f,\"faults\":%u}\n",
        policy?"threshold":"adaptive",pump_gain,(unsigned long long)c.total_pump_ms,c.pump_starts,c.stress_ms,overshoot,soil,c.faults);
    return 0;
}
