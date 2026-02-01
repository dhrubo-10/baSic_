#ifndef KERNEL_STDIO_H
#define KERNEL_STDIO_H

#include <kernelb/types.h>

void printk(const char *fmt, ...);
void putchar(char c);
void cls(void);

#endif