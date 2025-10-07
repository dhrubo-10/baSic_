#include <stdint.h>
#include "bios.h"
#include "syscall.h"

/* Kernel main */
void kmain(void) {
    bios_clear_screen();
    bios_puts("\r\nbaSic_ kernel booted\r\n");
    bios_puts("TinyKernel-style handoff OK.\r\n");

    /* Show boot device (DL) if the bootloader preserved it (common convention). */
    unsigned char boot_dl = 0xFF;
    asm volatile ("movb %%dl, %0" : "=r"(boot_dl) :: );
    bios_puts("Boot device (DL) = 0x");
    bios_puthex8(boot_dl);
    bios_puts("\r\n");

    /* Try a filesystem syscall as a demo. If not implemented, the weak stub
       returns -1 and we print a helpful message. */
    bios_puts("Attempting sys_creat(\"TEST.TXT\")...\r\n");
    int fd = sys_creat("TEST.TXT");
    if (fd >= 0) {
        /* If a real FS exists, write sample text */
        const char msg[] = "Hello from baSic_ kernel!\r\n";
        sys_write(fd, msg, (uint16_t) (sizeof(msg) - 1));
        sys_close(fd);
        bios_puts("File written successfully.\r\n");
    } else {
        bios_puts("No filesystem/syscall backend available yet (sys_creat returned -1).\r\n");
    }

    bios_puts("\r\nPress any key to halt.\r\n");
    bios_wait_key();

    /* Halt forever */
    asm volatile (
        "cli\n\t"
    "1:\n\t"
        "hlt\n\t"
        "jmp 1b\n\t"
    );
}

/* Provide _start symbol so bootloaders that jump to _start will work. */
void _start(void) __attribute__((noreturn));
void _start(void) {
    /* Forward to the C kernel main */
    kmain();

    /* If kmain returns for some reason, halt. */
    for (;;) asm volatile("cli\n\thlt");
}
