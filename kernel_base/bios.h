#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>

void bios_putc(char c);
void bios_puts(const char *s);
void bios_wait_key(void);
void bios_puthex8(uint8_t val);
void bios_clear_screen(void);

#endif
