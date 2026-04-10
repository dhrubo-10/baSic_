/* baSic_ - kernel/main.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 * 
 * kernel entry point, called from stage2 after protected mode
 */

#include "vga.h"
#include "../lib/kprintf.h"
#include "../include/types.h"

static const char *banner[] = {
    "  _               _      ",
    " | |__   __ _ ___(_) ___ ",
    " | '_ \\ / _` / __| |/ __|",
    " | |_) | (_| \\__ \\ | (__ ",
    " |_.__/ \\__,_|___/_|\\___|",
    0,
};

static void print_banner(void)
{
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    for (int i = 0; banner[i]; i++)
        kprintf("%s\n", banner[i]);
    kprintf("\n");
}

static void print_info(void)
{
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("baSic_ v0.01 - Linux 0.01-inspired kernel\n");
    kprintf("Inspired by Linus Torvalds, Dennis Ritchie, Ken Thompson\n\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("[OK] stage 1 bootloader    loaded\n");
    kprintf("[OK] stage 2 (GDT + PM)   done\n");
    kprintf("[OK] VGA driver            ready\n");
    kprintf("[OK] kprintf               ready\n");
    kprintf("[OK] kmain()               running\n\n");

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprintf("32-bit protected mode. next: IDT + PIC\n\n");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("kernel base:  %x\n", 0x8000);
    kprintf("stack top:    %x\n", 0x9F000);

    /* prove kprintf format specifiers work */
    kprintf("decimal test: %d  unsigned: %u  char: %c\n", -42, 255u, 'K');
}

void kmain(void)
{
    vga_init();
    print_banner();
    print_info();

    for (;;)
        __asm__ volatile ("hlt");
}