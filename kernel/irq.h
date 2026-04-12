/* baSic_ - kernel/irq.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * hardware IRQ handler registration interface
 */

#ifndef IRQ_H
#define IRQ_H

#include "idt.h"

/* function pointer type for IRQ handlers */
typedef void (*irq_handler_t)(registers_t *regs);

void irq_init(void);
void irq_register(u8 irq, irq_handler_t handler);
void irq_handler(registers_t *regs);

#endif