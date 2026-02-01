// ik its dumb, but whatever..

#include <kernelb/stdio.h>

static void putchar(char c)
{
    volatile char *video = (volatile char*)0xB8000;
    static int pos = 0;
    
    if (c == '\n') {
        pos = (pos + 160) - (pos % 160);
    } else {
        video[pos++] = c;
        video[pos++] = 0x0F;
    }
    
    // Scroll if needed
    if (pos >= 80*25*2) {
        for (int i = 160; i < 80*25*2; i++) {
            video[i-160] = video[i];
        }
        pos -= 160;
        for (int i = pos; i < 80*25*2; i++) {
            video[i] = 0;
        }
    }
}

void printk(const char *str)
{
    while (*str) {
        putchar(*str++);
    }
}