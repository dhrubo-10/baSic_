#ifndef IDT_H
#define IDT_H

#include "../include/types.h"

/* one IDT gate descriptor : 8 bytes, must be packed */
typedef struct {
    u16  offset_low;    /* bits 0–15  of handler address */
    u16  selector;      /* code segment selector (0x08)  */
    u8   zero;          /* always 0                      */
    u8   type_attr;     /* gate type + DPL + present bit */
    u16  offset_high;   /* bits 16–31 of handler address */
} PACKED idt_entry_t;

/* loaded into IDTR with lidt */
typedef struct {
    u16  limit;
    u32  base;
} PACKED idt_ptr_t;

/* register state pushed by our ISR stubs before calling the C handler */
typedef struct {
    /* pushed by pusha */
    u32 edi, esi, ebp, esp_dummy;
    u32 ebx, edx, ecx, eax;
    /* pushed by our stub */
    u32 int_no;
    u32 err_code;
    /* pushed automatically by the CPU on interrupt */
    u32 eip, cs, eflags;
} PACKED registers_t;

void idt_init(void);
void idt_set_gate(u8 num, u32 handler, u16 selector, u8 type_attr);

#endif