#ifndef DRV_TIMER_H
#define DRV_TIMER_H

#include "../inc/types.h"

void timer_init(u32 freq_hz);
u64 timer_get_ticks(void);
void timer_sleep(u32 ms);

#endif
