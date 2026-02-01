#include <kernelb/stdio.h>

void panic(const char *msg)
{
    printk("KERNEL PANIC: %s\n", msg);
    asm volatile("cli");
    // Halt
    asm volatile("hlt");
    while(1);
}