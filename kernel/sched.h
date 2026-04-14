/* baSic_ - kernel/sched.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * round-robin scheduler
 */

#ifndef SCHED_H
#define SCHED_H

#include "process.h"
#include "idt.h"

void sched_init(void);
void sched_tick(registers_t *regs);  /* called from timer IRQ */
void sched_yield(void);

#endif
void sched_start(void);