/* baSic_ - kernel/main.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel entry point, called from stage2 after protected mode
 */

#include "vga.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "rtc.h"
#include "shell.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"
#include "../include/types.h"

#define MEM_KB  32768   /* tell PMM we have 32 MB — adjust for real hardware */

void kmain(void)
{
    vga_init();
    idt_init();
    pic_init();
    irq_init();
    timer_init(1000);
    keyboard_init();
    rtc_init();
    pmm_init(MEM_KB);
    vmm_init();
    heap_init();

    __asm__ volatile ("sti");

    shell_init();
    shell_run();
}