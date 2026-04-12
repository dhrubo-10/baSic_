/* baSic_ - kernel/timer.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * PIT 8253/8254 timer driver
 * hooks into IRQ 0, provides tick counter and ksleep
 */

#include "timer.h"
#include "irq.h"
#include "idt.h"
#include "../lib/kprintf.h"

/* PIT I/O ports */
#define PIT_CHANNEL0    0x40    /* channel 0 data port (IRQ 0) */
#define PIT_CMD         0x43    /* mode/command register       */

/* command byte: channel 0, lobyte/hibyte, mode 3 (square wave) */
#define PIT_CMD_BYTE    0x36

/* PIT base frequency fixed by hardware */
#define PIT_BASE_HZ     1193182

static volatile u32 tick_count = 0;
static u32 ticks_per_ms = 0;

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* IRQ 0 handler fires at the configured frequency */
static void timer_irq_handler(registers_t *regs)
{
    (void)regs;
    tick_count++;
}

void timer_init(u32 freq)
{
    ticks_per_ms = freq / 1000;
    if (ticks_per_ms == 0)
        ticks_per_ms = 1;

    /* calculate the divisor that gives us the desired frequency */
    u32 divisor = PIT_BASE_HZ / freq;

    /* send command byte then divisor low/high */
    outb(PIT_CMD,      PIT_CMD_BYTE);
    outb(PIT_CHANNEL0, (u8)(divisor & 0xFF));         /* low byte  */
    outb(PIT_CHANNEL0, (u8)((divisor >> 8) & 0xFF));  /* high byte */

    irq_register(0, timer_irq_handler);

    kprintf("[OK] PIT timer             %d Hz\n", freq);
}

u32 timer_ticks(void)
{
    return tick_count;
}

void timer_sleep(u32 ms)
{
    u32 target = tick_count + (ms * ticks_per_ms);
    while (tick_count < target)
        __asm__ volatile ("hlt");   /* yield CPU until next IRQ wakes us */
}