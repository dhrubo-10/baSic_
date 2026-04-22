/* baSic_ - kernel/tss.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * Task State Segment: required by x86 for ring 3 -> ring 0 transitions
 */

#ifndef TSS_H
#define TSS_H

#include "../include/types.h"

typedef struct {
    u32 prev_tss;
    u32 esp0;       /* kernel stack pointer: loaded on ring 3 -> ring 0 */
    u32 ss0;        /* kernel stack segment selector (0x10)              */
    u32 esp1, ss1;
    u32 esp2, ss2;
    u32 cr3, eip, eflags;
    u32 eax, ecx, edx, ebx;
    u32 esp, ebp, esi, edi;
    u32 es, cs, ss, ds, fs, gs;
    u32 ldt;
    u16 trap, iomap_base;
} PACKED tss_t;

void tss_init(void);
void tss_set_kernel_stack(u32 esp0);

#endif