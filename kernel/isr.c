/* baSic_ - kernel/isr.c
 * Copyright (C) 2026 Shahriar Dhrubo 
 * GPL v2 : see LICENSE
 * C handler for CPU exceptions 0–31
 * called by isr_common in isr.asm
 */

#include "isr.h"
#include "../lib/kprintf.h"

/* 32 CPU exceptions */
static const char *exception_names[] = {
    "Division By Zero",          /* 0  */
    "Debug",                     /* 1  */
    "Non-Maskable Interrupt",    /* 2  */
    "Breakpoint",                /* 3  */
    "Overflow",                  /* 4  */
    "Bound Range Exceeded",      /* 5  */
    "Invalid Opcode",            /* 6  */
    "Device Not Available",      /* 7  */
    "Double Fault",              /* 8  */
    "Coprocessor Segment",       /* 9  */
    "Invalid TSS",               /* 10 */
    "Segment Not Present",       /* 11 */
    "Stack-Segment Fault",       /* 12 */
    "General Protection Fault",  /* 13 */
    "Page Fault",                /* 14 */
    "Reserved",                  /* 15 */
    "x87 FPU Fault",             /* 16 */
    "Alignment Check",           /* 17 */
    "Machine Check",             /* 18 */
    "SIMD FP Exception",         /* 19 */
    "Virtualization Exception",  /* 20 */
    "Reserved",                  /* 21 */
    "Reserved",                  /* 22 */
    "Reserved",                  /* 23 */
    "Reserved",                  /* 24 */
    "Reserved",                  /* 25 */
    "Reserved",                  /* 26 */
    "Reserved",                  /* 27 */
    "Reserved",                  /* 28 */
    "Reserved",                  /* 29 */
    "Security Exception",        /* 30 */
    "Reserved",                  /* 31 */
};

void isr_handler(registers_t *regs)
{
    /* set red for the panic dump */
    extern void vga_set_color(int fg, int bg);
    vga_set_color(12, 0);   /* VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK */

    kprintf("\n*** baSic_ EXCEPTION ***\n");
    kprintf("  #%d — %s\n",
        regs->int_no,
        (regs->int_no < 32) ? exception_names[regs->int_no] : "Unknown");
    kprintf("  err code: %x\n", regs->err_code);
    kprintf("  eip: %x  cs: %x  eflags: %x\n",
        regs->eip, regs->cs, regs->eflags);
    kprintf("  eax: %x  ebx: %x  ecx: %x  edx: %x\n",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    kprintf("  esp: %x  ebp: %x  esi: %x  edi: %x\n",
        regs->esp_dummy, regs->ebp, regs->esi, regs->edi);

    /* halt ... unrecoverable */
    for (;;)
        __asm__ volatile ("hlt");
}