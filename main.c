/* Competition learning/research application derived from the official
 * LiteOS-M RISC-V tutorial. Upstream component notices are preserved. */
#include "app_runtime.h"
#include "board_port.h"
#include "los_task.h"
#include <stdio.h>
int main(void)
{
    UINT32 rc;board_safe_init();setvbuf(stdout,NULL,_IONBF,0);rc=LOS_KernelInit();
    if(rc!=LOS_OK){printf("BOOT_FAIL,%lx\n",(unsigned long)rc);while(1){}}
    app_start();rc=LOS_Start();
    printf("SCHED_EXIT,%lx\n",(unsigned long)rc);
    gpio_bit_reset(GPIOA,GPIO_PIN_6|GPIO_PIN_7);gpio_bit_reset(GPIOB,GPIO_PIN_0);
    while(1){}
}
