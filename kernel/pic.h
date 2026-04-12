/* baSic_ - kernel/pic.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * 8259 PIC definitions and interface
 */

#ifndef PIC_H
#define PIC_H

#include "../include/types.h"

/* after remapping:
 *   master PIC  -> IRQ 0–7  maps to vectors 32–39
 *   slave  PIC  -> IRQ 8–15 maps to vectors 40–47
 */
#define PIC_MASTER_OFFSET   32
#define PIC_SLAVE_OFFSET    40

void pic_init(void);
void pic_send_eoi(u8 irq);
void pic_mask_irq(u8 irq);
void pic_unmask_irq(u8 irq);

#endif