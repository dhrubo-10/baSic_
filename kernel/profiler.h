/* baSic_ - kernel/profiler.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * simple kernel profiler: tick counters per subsystem
 */

#ifndef PROFILER_H
#define PROFILER_H

#include "../include/types.h"

typedef enum {
    PROF_SCHED    = 0,
    PROF_IRQ,
    PROF_SYSCALL,
    PROF_DISK,
    PROF_VFS,
    PROF_COUNT
} prof_counter_t;

void prof_init(void);
void prof_inc(prof_counter_t c);
void prof_dump(void);      
u32  prof_get(prof_counter_t c);

#endif