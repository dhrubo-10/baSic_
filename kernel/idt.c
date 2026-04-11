/* baSic_ - kernel/idt.c
 * Copyright (C) 2026 Shahriar Dhrubo 
 * GPL V2 . see LICENSE
 * IDT setup — 256 gates, exceptions 0–31 wired to ISR stubs
 */

#include "idt.h"
#include "../lib/string.h"

/* ISR stubs declared in isr.asm */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr;

/* IDT gate type attributes */
#define IDT_INTERRUPT_GATE  0x8E   /* P=1 DPL=0 type=1110 (32-bit interrupt gate) */
#define IDT_TRAP_GATE       0x8F   /* P=1 DPL=0 type=1111 (32-bit trap gate)      */

void idt_set_gate(u8 num, u32 handler, u16 selector, u8 type_attr)
{
    idt[num].offset_low  = (u16)(handler & 0xFFFF);
    idt[num].selector    = selector;
    idt[num].zero        = 0;
    idt[num].type_attr   = type_attr;
    idt[num].offset_high = (u16)((handler >> 16) & 0xFFFF);
}

void idt_init(void)
{
    memset(idt, 0, sizeof(idt));

    /* CPU exceptions 0–31 */
    idt_set_gate(0,  (u32)isr0,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(1,  (u32)isr1,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(2,  (u32)isr2,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(3,  (u32)isr3,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(4,  (u32)isr4,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(5,  (u32)isr5,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(6,  (u32)isr6,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(7,  (u32)isr7,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(8,  (u32)isr8,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(9,  (u32)isr9,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(10, (u32)isr10, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(11, (u32)isr11, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(12, (u32)isr12, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(13, (u32)isr13, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(14, (u32)isr14, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(15, (u32)isr15, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(16, (u32)isr16, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(17, (u32)isr17, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(18, (u32)isr18, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(19, (u32)isr19, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(20, (u32)isr20, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(21, (u32)isr21, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(22, (u32)isr22, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(23, (u32)isr23, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(24, (u32)isr24, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(25, (u32)isr25, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(26, (u32)isr26, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(27, (u32)isr27, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(28, (u32)isr28, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(29, (u32)isr29, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(30, (u32)isr30, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(31, (u32)isr31, 0x08, IDT_INTERRUPT_GATE);

    idt_ptr.limit = (u16)(sizeof(idt) - 1);
    idt_ptr.base  = (u32)&idt;

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}