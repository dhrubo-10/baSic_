#include <stdint.h>
#include <stddef.h>
#include "bios.h"

void kmain(void) {
    bios_clear_screen();
    bios_puts("\r\nTinyKernel v0.1\r\n");
    bios_puts("BIOS support initialized.\r\n");

    unsigned char boot_dl;
    asm volatile ("movb %%dl, %0" : "=r"(boot_dl) :: );
    bios_puts("Boot device: 0x");
    bios_puthex8(boot_dl);
    bios_puts("\r\n\r\nPress any key to halt...");
    bios_wait_key();

    asm volatile ("cli; hlt");
}

void _start(void) __attribute__((noreturn));
void _start(void) {
    kmain();
    for (;;) asm volatile("hlt");
}



/* BIOS teletype: AH=0x0E, AL=char, BH=page(0), BL=color thats optional WEIRD */
static inline void bios_putc(char c) {
    /* Put character c in AL and AH=0x0E then call int 0x10 */
    uint16_t ax = (uint8_t)c | (0x0E << 8);
    asm volatile ("int $0x10" : : "a"(ax) : "cc", "memory");
}

static void bios_puts(const char *s) {
    for (const char *p = s; *p; ++p) bios_putc(*p);
}

/* Wait for keypress using BIOS INT 0x16 (AH=0x00) */
static inline void bios_wait_key(void) {
    asm volatile ("xor %%ah, %%ah\n\tint $0x16" : : : "ax", "memory");
}

static void print_hex8(uint8_t v) {
    const char *hex = "0123456789ABCDEF";
    bios_putc(hex[(v >> 4) & 0xF]);
    bios_putc(hex[v & 0xF]);
}

void kmain(void) {
    /* Example status line */
    bios_puts("\r\nTinyKernel v0.1\r\n");
    bios_puts("Bootloader -> kernel handover OK.\r\n");

    /* If the bootloader left the boot device in DL, show it (common convention). */
    unsigned char boot_dl;
    asm volatile ("movb %%dl, %0" : "=r"(boot_dl) :: );
    bios_puts("Boot device (DL) = 0x");
    print_hex8(boot_dl);
    bios_puts("\r\n\r\nPress any key to halt...");

    bios_wait_key();

    asm volatile (
        "cli\n\t"
    "1:\n\t"
        "hlt\n\t"
        "jmp 1b\n\t"
    );
}

/* Provide a tiny symbol so a linker can use it as an entry if desired. */
void _start(void) __attribute__((noreturn));
void _start(void) {
    /* Some linkers jump to _start; just forward to kmain. */
    kmain();
    for (;;) asm volatile("hlt");
}
