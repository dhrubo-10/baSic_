// ik its dumb, but whatever..

#include <kernelb/stdio.h>
#include <kernelb/string.h>
#include <drivers/vga.h>

static void print_uint(uint32_t n)
{
    char buf[32];
    int i = 0;
    
    do {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    } while (n);
    
    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

static void print_hex(uint32_t n)
{
    char buf[32];
    int i = 0;
    
    do {
        int digit = n % 16;
        buf[i++] = digit < 10 ? '0' + digit : 'A' + digit - 10;
        n /= 16;
    } while (n);
    
    vga_putchar('0');
    vga_putchar('x');
    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

void printk(const char *fmt, ...)
{
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd') {
                // Would parse integer from args
            } else if (*fmt == 's') {
                // Would parse string from args
            } else if (*fmt == 'c') {
                // Would parse char from args
            }
        } else {
            vga_putchar(*fmt);
        }
        fmt++;
    }
}