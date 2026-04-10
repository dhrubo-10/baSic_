#include "vga.h"
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
    for (int i = 0; banner[i]; i++) {
        vga_print(banner[i]);
        vga_putchar('\n');
    }
    vga_putchar('\n');
}

static void print_info(void)
{
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("baSic_ v0.01 - Linux 0.01-inspired kernel\n");
    vga_print("Inspired by Linus Torvalds, Dennis Ritchie, Ken Thompson\n");
    vga_putchar('\n');

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("[OK] Bootloader stage 1   loaded\n");
    vga_print("[OK] Bootloader stage 2   loaded (GDT + protected mode)\n");
    vga_print("[OK] VGA driver           initialized\n");
    vga_print("[OK] kmain()              entered\n");
    vga_putchar('\n');

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("System is running in 32-bit protected mode.\n");
    vga_print("Next: IDT, ISR, PIC, keyboard driver...\n");
    vga_putchar('\n');

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("Kernel base address:  ");
    vga_print_hex(0x8000);
    vga_putchar('\n');
    vga_print("Stack pointer:        ");
    vga_print_hex(0x9F000);
    vga_putchar('\n');
}

/*
 * kmain - Kernel entry point.
 * Called from stage2.asm after protected mode is established.
 * Never returns.
 */
void kmain(void)
{
    vga_init();
    print_banner();
    print_info();

    /* Kernel idle loop — will host scheduler later */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}