/* baSic_ - kernel/timer.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * PIT 8253/8254 timer driver interface
 */

#ifndef TIMER_H
#define TIMER_H

#include "../include/types.h"

/* initialise PIT freq is ticks per second (Hz) */
void timer_init(u32 freq);

/* returns total ticks since timer_init() */
u32 timer_ticks(void);

/* busy/wait for approximately ms milliseconds */
void timer_sleep(u32 ms);
void timer_tick_increment(void);

#endif