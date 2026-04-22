/* baSic_ - kernel/tss.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * TSS setup: installs one TSS in the GDT so the CPU can switch stacks
 * on ring 3 ->  ring 0 transitions (interrupts, syscalls)
 */

#include "tss.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static tss_t tss;

/* GDT entry for the TSS — we patch the GDT that stage2 set up.
 * stage2 GDT: entry 0=null, 1=code(0x08), 2=data(0x10)
 * we add:     entry 3=TSS  (0x18)
 * The GDT pointer is at a known physical location we can read back. */

/* write one 8-byte GDT entry at the given GDT slot */
static void gdt_set_entry(int slot, u32 base, u32 limit,
                           u8 access, u8 granularity)
{
    /* GDT lives where the lgdt in stage2 pointed — read GDTR to find it */
    struct { u16 limit_val; u32 base_addr; } PACKED gdtr;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));

    u8 *entry = (u8 *)(gdtr.base_addr + slot * 8);

    entry[0] = (u8)(limit & 0xFF);
    entry[1] = (u8)((limit >> 8) & 0xFF);
    entry[2] = (u8)(base & 0xFF);
    entry[3] = (u8)((base >> 8) & 0xFF);
    entry[4] = (u8)((base >> 16) & 0xFF);
    entry[5] = access;
    entry[6] = (u8)(((limit >> 16) & 0x0F) | (granularity & 0xF0));
    entry[7] = (u8)((base >> 24) & 0xFF);
}

void tss_init(void)
{
    memset(&tss, 0, sizeof(tss));
    tss.ss0  = 0x10;        /* kernel data selector */
    tss.esp0 = 0x9F000;     /* initial kernel stack */
    tss.iomap_base = sizeof(tss_t);

    u32 base  = (u32)&tss;
    u32 limit = sizeof(tss_t) - 1;

    /* TSS descriptor: access=0x89 (present, ring0, 32-bit available TSS) */
    gdt_set_entry(3, base, limit, 0x89, 0x00);

    /* load TR with selector 0x18 (GDT entry 3) */
    __asm__ volatile ("mov $0x18, %%ax; ltr %%ax" : : : "ax");

    kprintf("[OK] TSS: installed at selector 0x18\n");
}

void tss_set_kernel_stack(u32 esp0)
{
    tss.esp0 = esp0;
}