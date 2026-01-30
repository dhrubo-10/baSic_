#ifndef KERNELB_PANIC_H
#define KERNELB_PANIC_H

#include <kernel_b/stdio.h>

#define panic(fmt, ...) do { \
    printk("KERNEL PANIC at %s:%d: ", __FILE__, __LINE__); \
    printk(fmt, ##__VA_ARGS__); \
    printk("\n"); \
    for(;;); \
} while(0)

#endif