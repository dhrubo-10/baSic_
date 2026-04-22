/* baSic_ - kernel/profiler.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel profiler: lightweight tick counter per subsystem
 * increment with prof_inc() at hot paths, dump with prof_dump()
 */

#include "profiler.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

static u32 counters[PROF_COUNT];

static const char *names[] = {
    "scheduler",
    "irq",
    "syscall",
    "disk",
    "vfs",
};

void prof_init(void)
{
    memset(counters, 0, sizeof(counters));
    kprintf("[OK] profiler: ready\n");
}

void prof_inc(prof_counter_t c)
{
    if (c < PROF_COUNT) counters[c]++;
}

u32 prof_get(prof_counter_t c)
{
    return (c < PROF_COUNT) ? counters[c] : 0;
}

void prof_dump(void)
{
    kprintf("  kernel profiler counters:\n");
    for (int i = 0; i < PROF_COUNT; i++)
        kprintf("  %-12s %d\n", names[i], counters[i]);
}