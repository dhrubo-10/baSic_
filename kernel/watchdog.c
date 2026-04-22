/* baSic_ - kernel/watchdog.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * simple countdown watchdog
 * if watchdog_kick() is not called within timeout_ms milliseconds,
 * watchdog_fired() returns 1 and the kernel can take action
 */

#include "watchdog.h"
#include "timer.h"
#include "../lib/kprintf.h"

static u32 timeout_ticks = 0;
static u32 last_kick     = 0;
static int enabled       = 0;

void watchdog_init(u32 timeout_ms)
{
    timeout_ticks = timeout_ms;
    last_kick     = timer_ticks();
    enabled       = 1;
    kprintf("[OK] watchdog: %dms timeout\n", timeout_ms);
}

void watchdog_kick(void)
{
    last_kick = timer_ticks();
}

void watchdog_tick(void)
{
    (void)0;
}

int watchdog_fired(void)
{
    if (!enabled) return 0;
    return (timer_ticks() - last_kick) > timeout_ticks;
}