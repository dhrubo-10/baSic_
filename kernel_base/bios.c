#include <stdint.h>
#include <stddef.h>
#include "bios.h"

/* BIOS teletype output (INT 0x10, AH=0x0E) */
void bios_putc(char c) {
    uint16_t ax = (uint8_t)c | (0x0E << 8);
    asm volatile ("int $0x10" : : "a"(ax) : "cc", "memory");
}

/* Print string (CRLF friendly if the string contains \n) */
void bios_puts(const char *s) {
    while (*s) {
        char c = *s++;
        if (c == '\n') bios_putc('\r');
        bios_putc(c);
    }
}

/* Wait for keypress using BIOS INT 0x16 (AH=0x00) */
void bios_wait_key(void) {
    asm volatile ("xor %%ah, %%ah\n\tint $0x16" : : : "ax", "memory");
}

void bios_puthex8(uint8_t val) {
    static const char *hex = "0123456789ABCDEF";
    bios_putc(hex[(val >> 4) & 0xF]);
    bios_putc(hex[val & 0xF]);
}

void bios_clear_screen(void) {
    asm volatile (
        "mov $0x0600, %%ax\n\t"   /* AH=6 (scroll/window), AL=0 -> clear entire window */
        "mov $0x07, %%bh\n\t"     /* attribute */
        "xor %%cx, %%cx\n\t"      /* top-left (0,0) */
        "mov $0x184F, %%dx\n\t"   /* bottom-right (24,79) */
        "int $0x10"
        :
        :
        : "ax", "bx", "cx", "dx", "memory"
    );
}

