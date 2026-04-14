/* baSic_ - kernel/sched.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * round-robin scheduler
 * called on every timer tick — saves current context, picks next READY process,
 * restores its context
 */

#include "sched.h"
#include "process.h"
#include "timer.h"
#include "../lib/kprintf.h"

#define SCHED_TICKS  10     /* context switch every 10ms at 1000Hz */

static int sched_enabled  = 0;
static int tick_counter   = 0;

extern void proc_set_current(u32 pid);
extern process_t *proc_table_get(int i);

static process_t *pick_next(process_t *cur)
{
    int start = 0;
    if (cur) {
        for (int i = 0; i < PROC_MAX; i++) {
            if (proc_table_get(i) == cur) {
                start = (i + 1) % PROC_MAX;
                break;
            }
        }
    }

    for (int i = 0; i < PROC_MAX; i++) {
        int idx = (start + i) % PROC_MAX;
        process_t *p = proc_table_get(idx);
        if (p && p->state == PROC_READY)
            return p;
    }
    return cur;    /* no other ready process — keep running current */
}

void sched_tick(registers_t *regs)
{
    if (!sched_enabled) return;

    tick_counter++;
    if (tick_counter < SCHED_TICKS) return;
    tick_counter = 0;

    process_t *cur  = proc_current();
    process_t *next = pick_next(cur);

    if (!next || next == cur) return;

    /* save current context from interrupt frame */
    if (cur && cur->state == PROC_RUNNING) {
        cur->ctx.eax    = regs->eax;
        cur->ctx.ecx    = regs->ecx;
        cur->ctx.edx    = regs->edx;
        cur->ctx.ebx    = regs->ebx;
        cur->ctx.esp    = regs->esp_dummy;
        cur->ctx.ebp    = regs->ebp;
        cur->ctx.esi    = regs->esi;
        cur->ctx.edi    = regs->edi;
        cur->ctx.eip    = regs->eip;
        cur->ctx.eflags = regs->eflags;
        cur->state      = PROC_READY;
    }

    /* restore next context into interrupt frame */
    next->state     = PROC_RUNNING;
    proc_set_current(next->pid);

    regs->eax     = next->ctx.eax;
    regs->ecx     = next->ctx.ecx;
    regs->edx     = next->ctx.edx;
    regs->ebx     = next->ctx.ebx;
    regs->esp_dummy = next->ctx.esp;
    regs->ebp     = next->ctx.ebp;
    regs->esi     = next->ctx.esi;
    regs->edi     = next->ctx.edi;
    regs->eip     = next->ctx.eip;
    regs->eflags  = next->ctx.eflags;
}

void sched_init(void)
{
    sched_enabled = 0;
    tick_counter  = 0;

    kprintf("[OK] sched: round-robin ready (%dms quantum)\n", SCHED_TICKS);
}

void sched_yield(void)
{
    __asm__ volatile ("int $0x80" : : "a"(0xFF));
}

void sched_start(void)
{
    sched_enabled = 1;
}