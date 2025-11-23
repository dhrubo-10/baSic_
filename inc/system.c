// inc/system.c
#include "system.h"
#include "terminal.h"
#include <stdarg.h>

#ifdef HOSTED
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif


void system_init(void) {
    term_init();
    term_puts("System init...\n");
 
}


void system_reboot(void) {
    term_puts("Rebooting...\n");
#ifdef HOSTED
    fflush(stdout);
    system("reboot >/dev/null 2>&1 || echo 'Reboot not permitted in this test environment'\n");
#else

    asm volatile ("cli\n\tjmp 0xFFFF:0x0000");
#endif
}

/* Halt/power off */
void system_halt(void) {
    term_puts("Halting system...\n");
#ifdef HOSTED
    _exit(0);
#else
    for (;;) asm volatile ("hlt");
#endif
}


void panic(const char *msg) {
    term_puts("[PANIC] ");
    term_puts(msg);
    term_puts("\n");
    for (;;) asm volatile("cli\n\thlt");
}

long syscall(int num, ...) {
#ifdef HOSTED
    va_list ap;
    va_start(ap, num);
    long ret = -1;
    switch (num) {
        case SYS_WRITE: {
            const char *buf = va_arg(ap, const char *);
            size_t len = va_arg(ap, size_t);
            ret = fwrite(buf, 1, len, stdout);
            fflush(stdout);
            break;
        }
        case SYS_TIME: {
            ret = (long)time(NULL);
            break;
        }
        default:
            ret = -1;
            break;
    }
    va_end(ap);
    return ret;
#else
    (void)num;
    return -1;
#endif
}
