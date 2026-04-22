/* baSic_ - kernel/userspace.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * user space support — ring 3 GDT segments + iret jump to ring 3
 */

#include "userspace.h"
#include "tss.h"
#include "../lib/kprintf.h"

static void gdt_set_entry(int slot, u32 base, u32 limit,
                           u8 access, u8 granularity)
{
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

void userspace_init_gdt(void)
{
    gdt_set_entry(4, 0, 0xFFFFF, 0xFA, 0xCF);   /* ring 3 code */
    gdt_set_entry(5, 0, 0xFFFFF, 0xF2, 0xCF);   /* ring 3 data */
    kprintf("[OK] userspace: ring 3 GDT segments installed\n");
}

void userspace_enter(u32 entry, u32 user_stack)
{
    tss_set_kernel_stack(0x9F000);

    u32 user_ds = USER_DATA_SEL;
    u32 user_cs = USER_CODE_SEL;

    __asm__ volatile (
        "mov %0, %%eax          \n"
        "mov %%ax,  %%ds        \n"
        "mov %%ax,  %%es        \n"
        "mov %%ax,  %%fs        \n"
        "mov %%ax,  %%gs        \n"
        "pushl %0               \n"   /* ss  */
        "pushl %1               \n"   /* esp */
        "pushfl                 \n"   /* eflags */
        "popl  %%eax            \n"
        "orl   $0x200, %%eax    \n"   /* set IF */
        "pushl %%eax            \n"
        "pushl %2               \n"   /* cs  */
        "pushl %3               \n"   /* eip */
        "iret                   \n"
        :
        : "rm"(user_ds), "rm"(user_stack),
          "rm"(user_cs),  "rm"(entry)
        : "eax"
    );
}