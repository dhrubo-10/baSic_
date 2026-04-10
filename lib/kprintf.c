/* baSic_ - lib/kprintf.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 * minimal kernel printf backed by vga_putchar
 * supports: %s %d %u %x %c %%
 */

#include "kprintf.h"
#include "../kernel/vga.h"
#include "../include/types.h"

/* va_list without stdarg.h — x86 cdecl: args pushed right-to-left on stack */
typedef u8 *va_list;
#define va_start(ap, last)  (ap = (va_list)&(last) + sizeof(last))
#define va_arg(ap, T)       (*(T *)((ap += sizeof(T)) - sizeof(T)))
#define va_end(ap)          ((void)0)

static void print_uint(u32 val, int base)
{
    const char digits[] = "0123456789abcdef";
    char  buf[32];
    int   pos = 31;

    buf[31] = '\0';

    if (val == 0) {
        vga_putchar('0');
        return;
    }

    while (val > 0) {
        buf[--pos] = digits[val % base];
        val /= base;
    }
    vga_print(&buf[pos]);
}

static void print_int(i32 val)
{
    if (val < 0) {
        vga_putchar('-');
        /* cast to u32 safely handles INT32_MIN */
        print_uint((u32)(-(val + 1)) + 1, 10);
    } else {
        print_uint((u32)val, 10);
    }
}

void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            vga_putchar(*fmt++);
            continue;
        }

        fmt++; /* skip '%' */

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            vga_print(s);
            break;
        }
        case 'd':
            print_int(va_arg(ap, i32));
            break;
        case 'u':
            print_uint(va_arg(ap, u32), 10);
            break;
        case 'x':
            vga_print("0x");
            print_uint(va_arg(ap, u32), 16);
            break;
        case 'c':
            vga_putchar((char)va_arg(ap, int));
            break;
        case '%':
            vga_putchar('%');
            break;
        default:
            /* unknown specifier — print literally */
            vga_putchar('%');
            vga_putchar(*fmt);
            break;
        }

        fmt++;
    }

    va_end(ap);
}