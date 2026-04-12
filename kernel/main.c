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
#include "../include/types.h"

void kmain(void)
{
    vga_init();
    idt_init();
    pic_init();
    irq_init();
    timer_init(1000);
    keyboard_init();
    rtc_init();

    __asm__ volatile ("sti");

    shell_init();
    shell_run();  
}