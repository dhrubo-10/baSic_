/* baSic_ - kernel/panic.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel panic prints a full red screen and halts
 */

#include "panic.h"
#include "vga.h"
#include "serial.h"
#include "../lib/kprintf.h"

static const char *panic_banner[] = {
    " +-------------------------------------------------+",
    " |              baSic_  KERNEL  PANIC              |",
    " +-------------------------------------------------+",
    0,
};

void kpanic(const char *file, u32 line, const char *msg)
{
    __asm__ volatile ("cli");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clear();

    for (int i = 0; panic_banner[i]; i++) {
        vga_print(panic_banner[i]);
        vga_putchar('\n');
    }

    vga_putchar('\n');
    vga_print(" message : "); vga_print(msg);     vga_putchar('\n');
    vga_print(" file    : "); vga_print(file);    vga_putchar('\n');
    vga_print(" line    : "); vga_print_dec(line); vga_putchar('\n');
    vga_putchar('\n');
    vga_print(" system halted. reset to continue.");

    serial_print("\n[PANIC] ");
    serial_print(msg);
    serial_print("\n  file: ");
    serial_print(file);
    serial_print("\n");

    for (;;)
        __asm__ volatile ("hlt");
}