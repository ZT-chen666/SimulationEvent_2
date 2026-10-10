#include "app_runtime.h"
#include "board_port.h"
#include "los_task.h"
#include "los_queue.h"
#include "los_mux.h"
#include "los_tick.h"
#include "los_config.h"
#include "los_memory.h"
#include "gd32vf103.h"
#include <stdio.h>
#include <string.h>

typedef struct {uint32_t at;uint8_t command;} AppCommand;
typedef struct {
    uint32_t at,faults,starts,pump_ms;
    uint16_t vpd,demand;
    uint8_t state,reason,pump,fan,lamp,mode,policy,profile,soil,water;
} AppEvent;
static UINT32 sample_q,command_q,event_q,snapshot_mux;
static AgControl snapshot;
static volatile uint32_t sample_drops,command_drops,event_drops,expired_commands;
static UINT32 task_ids[3];
static uint32_t now_ms(void)
{return (uint32_t)(SysTimer_GetLoadValue()/(SystemCoreClock/4000u));}
static void fail(const char *where,UINT32 rc)
{
    board_all_off();
    printf("FATAL,%s,0x%lx\n",where,(unsigned long)rc);while(1){}
}
static void check(const char *where,UINT32 rc) {if(rc!=LOS_OK)fail(where,rc);}

static VOID *sensor_task(UINT32 arg)
{
    (void)arg;board_sensor_init();
    while(1) {
        AgSample s;board_read_sample(&s,now_ms());
        if(LOS_QueueWriteCopy(sample_q,&s,sizeof(s),LOS_NO_WAIT)!=LOS_OK) {
            AgSample old;UINT32 size=sizeof(old);
            (void)LOS_QueueReadCopy(sample_q,&old,&size,LOS_NO_WAIT);sample_drops++;
            if(LOS_QueueWriteCopy(sample_q,&s,sizeof(s),LOS_NO_WAIT)!=LOS_OK)sample_drops++;
        }
        LOS_Msleep(AG_SAMPLE_PERIOD_MS);
    }
    return NULL;
}
static int changed(const AgControl *a,const AgControl *b)
{
    return a->state!=b->state || a->faults!=b->faults || a->reason!=b->reason ||
           a->pump!=b->pump || a->fan!=b->fan || a->lamp!=b->lamp ||
           a->mode!=b->mode || a->profile!=b->profile || a->policy!=b->policy;
}
static VOID *control_task(UINT32 arg)
{
    AgControl c,previous;(void)arg;ag_init(&c,now_ms());previous=c;
    while(1) {
        AgSample s;AppCommand cmd;UINT32 size;uint32_t now;
        size=sizeof(s);
        while(LOS_QueueReadCopy(sample_q,&s,&size,LOS_NO_WAIT)==LOS_OK) {ag_accept(&c,&s);size=sizeof(s);}
        now=now_ms();ag_step(&c,now); /* also checks expiry when queue is empty */
        size=sizeof(cmd);
        while(LOS_QueueReadCopy(command_q,&cmd,&size,LOS_NO_WAIT)==LOS_OK) {
            if((uint32_t)(now-cmd.at)<=500u)ag_command(&c,cmd.command,now);else expired_commands++;
            size=sizeof(cmd);
        }
        ag_step(&c,now);board_apply(&c); /* re-evaluate safety after user commands */
        if(changed(&previous,&c)) {
            AppEvent e={now,c.faults,c.pump_starts,(uint32_t)c.total_pump_ms,c.vpd100,c.demand100,
                c.state,c.reason,c.pump,c.fan,c.lamp,c.mode,c.policy,c.profile,c.sample.soil,c.sample.water};
            if(LOS_QueueWriteCopy(event_q,&e,sizeof(e),LOS_NO_WAIT)!=LOS_OK)event_drops++;
            previous=c;
        }
        check("control_lock",LOS_MuxPend(snapshot_mux,LOS_WAIT_FOREVER));snapshot=c;
        check("control_unlock",LOS_MuxPost(snapshot_mux));LOS_Msleep(AG_CONTROL_PERIOD_MS);
    }
    return NULL;
}
static void enqueue(uint8_t command,uint32_t now)
{
    AppCommand c={now,command};
    if(LOS_QueueWriteCopy(command_q,&c,sizeof(c),LOS_NO_WAIT)!=LOS_OK)command_drops++;
}
static VOID *ui_task(UINT32 arg)
{
    uint8_t candidate=0,stable=0,long_sent[3]={0,0,0};
    uint32_t changed_at=0,pressed_at[3]={0,0,0},render_at=0,resource_at=0;
    (void)arg;board_display_init();
    printf("event_ms,state,reason,faults,pump,fan,lamp,soil,water,vpd100,demand,starts,pump_ms,mode,policy,stage\n");
    while(1) {
        static AgControl copy;
        uint32_t now=now_ms();uint8_t raw=board_buttons();unsigned i;
        AppEvent e;UINT32 size=sizeof(e);
        if(raw!=candidate){candidate=raw;changed_at=now;}
        if(now-changed_at>=40u && candidate!=stable) {
            uint8_t old=stable;stable=candidate;
            for(i=0;i<3;i++) {
                uint8_t bit=(uint8_t)(1u<<i);
                if((stable&bit) && !(old&bit)){pressed_at[i]=now;long_sent[i]=0;}
                if(!(stable&bit) && (old&bit) && !long_sent[i]) {
                    enqueue(i==0?AG_CMD_MODE:i==1?AG_CMD_PROFILE:AG_CMD_PUMP,now);
                }
            }
        }
        for(i=1;i<3;i++)if((stable&(1u<<i)) && !long_sent[i] && now-pressed_at[i]>=1500u) {
            enqueue(i==1?AG_CMD_POLICY:AG_CMD_ACK,now);long_sent[i]=1;
        }
        while(LOS_QueueReadCopy(event_q,&e,&size,LOS_NO_WAIT)==LOS_OK) {
            printf("%lu,%s,%s,%03lX,%u,%u,%u,%u,%u,%u,%u,%lu,%lu,%u,%u,%u\n",
                (unsigned long)e.at,ag_state_name(e.state),ag_reason_name(e.reason),(unsigned long)e.faults,
                e.pump,e.fan,e.lamp,e.soil,e.water,e.vpd,e.demand,(unsigned long)e.starts,
                (unsigned long)e.pump_ms,e.mode,e.policy,e.profile);size=sizeof(e);
        }
        if(now-render_at>=AG_RENDER_PERIOD_MS) {
            check("ui_lock",LOS_MuxPend(snapshot_mux,LOS_WAIT_FOREVER));copy=snapshot;
            check("ui_unlock",LOS_MuxPost(snapshot_mux));
            render_at=now;
            printf("STATUS,%lu,valid=%u,drop=%lu/%lu/%lu,expired_cmd=%lu,heap_used=%lu,distance_mm=%u,soil_adc=%u,light_adc=%u\n",(unsigned long)now,
                copy.sample.valid,(unsigned long)sample_drops,(unsigned long)command_drops,
                (unsigned long)event_drops,(unsigned long)expired_commands,
                (unsigned long)LOS_MemTotalUsedGet(m_aucSysMem0),copy.sample.distance_mm,
                copy.sample.soil_adc,copy.sample.light_adc);
        }
        board_display(&copy); /* At most one changed display row per UI pass. */
        if(now-resource_at>=5000u) {
            TSK_INFO_S info;resource_at=now;
            for(i=0;i<3;i++)if(LOS_TaskInfoGet(task_ids[i],&info)==LOS_OK)
                printf("STACK,%lu,task=%lu,peak=%lu,size=%lu,overflow=%u\n",
                    (unsigned long)now,(unsigned long)task_ids[i],
                    (unsigned long)info.uwPeakUsed,(unsigned long)info.uwStackSize,info.bOvf);
        }
        LOS_Msleep(AG_UI_PERIOD_MS);
    }
    return NULL;
}
static void task(unsigned slot,const char *name,TSK_ENTRY_FUNC entry,UINT16 priority,UINT32 stack)
{
    TSK_INIT_PARAM_S p;UINT32 id;memset(&p,0,sizeof(p));
    p.pcName=(CHAR*)name;p.pfnTaskEntry=entry;p.usTaskPrio=priority;p.uwStackSize=stack;
    check(name,LOS_TaskCreate(&id,&p));task_ids[slot]=id;
}
void app_start(void)
{
    check("sample_queue",LOS_QueueCreate("samples",4,&sample_q,0,sizeof(AgSample)));
    check("command_queue",LOS_QueueCreate("commands",8,&command_q,0,sizeof(AppCommand)));
    check("event_queue",LOS_QueueCreate("events",16,&event_q,0,sizeof(AppEvent)));
    check("snapshot_mutex",LOS_MuxCreate(&snapshot_mux));ag_init(&snapshot,now_ms());
    task(0,"Control",control_task,4,1536);task(1,"Sensors",sensor_task,6,1280);task(2,"Display",ui_task,10,2048);
}
