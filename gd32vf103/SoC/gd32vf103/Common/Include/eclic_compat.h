/*!
    \file    eclic_compat.h
    \brief   compatibility wrappers for lowercase eclic_* APIs used in board drivers

    The NMSIS core provides uppercase ECLIC_xxx / __enable_irq APIs.
    Some board drivers use the older lowercase eclic_* names, so this
    header maps them to the current API.
*/

#ifndef __ECLIC_COMPAT_H
#define __ECLIC_COMPAT_H

#include "gd32vf103.h"

#ifndef ECLIC_GROUP_LEVEL3_PRIO1
#define ECLIC_GROUP_LEVEL3_PRIO1    3U
#endif

#define eclic_global_interrupt_enable()     __enable_irq()
#define eclic_set_nlbits(nlbits)            ECLIC_SetCfgNlbits(nlbits)

static inline void eclic_irq_enable(IRQn_Type IRQn, uint8_t level, uint8_t priority)
{
    ECLIC_SetLevelIRQ(IRQn, level);
    ECLIC_SetPriorityIRQ(IRQn, priority);
    ECLIC_EnableIRQ(IRQn);
}

#endif /* __ECLIC_COMPAT_H */
