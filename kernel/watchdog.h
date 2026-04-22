/* baSic_ - kernel/watchdog.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel watchdog: detects hangs via heartbeat timer
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "../include/types.h"

void watchdog_init(u32 timeout_ms);
void watchdog_kick(void);       /* reset the watchdog counter */
void watchdog_tick(void);       /* called from timer IRQ */
int  watchdog_fired(void);      /* returns 1 if timeout expired */

#endif