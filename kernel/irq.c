/* baSic_ - kernel/irq.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * hardware IRQ dispatch .. routes IRQs to registered C handlers
 */

#include "irq.h"
#include "pic.h"
#include "idt.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

/* IRQ stubs declared in irq_stubs.asm */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* table of registered handler function pointers, one per IRQ line */
static irq_handler_t irq_handlers[16];

void irq_init(void)
{
    memset(irq_handlers, 0, sizeof(irq_handlers));

    /* register IRQ stubs in IDT at vectors 32–47 */
    idt_set_gate(32, (u32)irq0,  0x08, 0x8E);
    idt_set_gate(33, (u32)irq1,  0x08, 0x8E);
    idt_set_gate(34, (u32)irq2,  0x08, 0x8E);
    idt_set_gate(35, (u32)irq3,  0x08, 0x8E);
    idt_set_gate(36, (u32)irq4,  0x08, 0x8E);
    idt_set_gate(37, (u32)irq5,  0x08, 0x8E);
    idt_set_gate(38, (u32)irq6,  0x08, 0x8E);
    idt_set_gate(39, (u32)irq7,  0x08, 0x8E);
    idt_set_gate(40, (u32)irq8,  0x08, 0x8E);
    idt_set_gate(41, (u32)irq9,  0x08, 0x8E);
    idt_set_gate(42, (u32)irq10, 0x08, 0x8E);
    idt_set_gate(43, (u32)irq11, 0x08, 0x8E);
    idt_set_gate(44, (u32)irq12, 0x08, 0x8E);
    idt_set_gate(45, (u32)irq13, 0x08, 0x8E);
    idt_set_gate(46, (u32)irq14, 0x08, 0x8E);
    idt_set_gate(47, (u32)irq15, 0x08, 0x8E);
}

/* register a C function to handle a specific IRQ line */
void irq_register(u8 irq, irq_handler_t handler)
{
    if (irq < 16)
        irq_handlers[irq] = handler;
}

/* called from irq_common in irq_stubs.asm for every hardware IRQ */
void irq_handler(registers_t *regs)
{
    /* int_no holds the vector (32–47), map back to IRQ number 0–15 */
    u8 irq = (u8)(regs->int_no - 32);

    if (irq < 16 && irq_handlers[irq])
        irq_handlers[irq](regs);

    pic_send_eoi(irq);
}