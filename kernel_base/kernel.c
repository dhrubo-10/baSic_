#include <stdint.h>
#include "bios.h"
#include "syscall.h"
#include "../f_sys/fs.h"   

static int kstrcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return (int)((unsigned char)*a - (unsigned char)*b);
        a++; b++;
    }
    return (int)((unsigned char)*a - (unsigned char)*b);
}

static int kstrncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; ++i) {
        if (!a[i] && !b[i]) return 0;
        if (a[i] != b[i]) return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        if (!a[i] || !b[i]) return 0;
    }
    return 0;
}

static void kstrcpy(char *dst, const char *src) {
    while ((*dst++ = *src++)) {}
}

static int kstrlen(const char *s) {
    int n = 0; while (s && *s++) n++; return n;
}

static char bios_getch(void) {
    uint16_t ax;
    asm volatile ("xor %%ah, %%ah\n\tint $0x16" : "=a"(ax) : : "memory");
    return (char)(ax & 0xFF);
}


static void bios_readline(char *buf, int bufsz) {
    int pos = 0;
    for (;;) {
        char c = bios_getch();
        if (c == '\r' || c == '\n') {
            bios_putc('\r');
            bios_putc('\n');
            break;
        }
        if ((c == 0x08 || c == 0x7f) && pos > 0) { /* backspace */
            pos--;
            bios_putc(0x08);
            bios_putc(' ');
            bios_putc(0x08);
            continue;
        }
        /* ignore control characters except printable */
        if ((unsigned char)c < 0x20) continue;
        if (pos < bufsz - 1) {
            buf[pos++] = c;
            bios_putc(c);
        }
    }
    buf[pos] = '\0';
}

static void parse_tokens(char *line, char *cmd, char *argrest) {
    /* skip leading spaces */
    char *p = line;
    while (*p == ' ') p++;
    /* copy cmd */
    int i = 0;
    while (*p && *p != ' ' && i < 31) cmd[i++] = *p++;
    cmd[i] = '\0';
    while (*p == ' ') p++;
    /* rest of line */
    kstrcpy(argrest, p);
}

static void print_prompt(void) {
    bios_puts("\r\nbaSic_:");
    if (current_dir && current_dir->name[0]) {
        bios_puts(current_dir->name);
    } else {
        bios_puts("/");
    }
    bios_puts("$ ");
}

void kmain(void) {
    bios_clear_screen();
    bios_puts("\r\nbaSic_ kernel booted\r\n");
    bios_puts("TinyKernel-style handoff OK.\r\n");

   
    unsigned char boot_dl = 0xFF;
    asm volatile ("movb %%dl, %0" : "=r"(boot_dl) :: );
    bios_puts("Boot device (DL) = 0x");
    bios_puthex8(boot_dl);
    bios_puts("\r\n");

    bios_puts("Initializing in-memory filesystem...\r\n");
    fs_init();

   
    bios_puts("Attempting sys_creat(\"TEST.TXT\")...\r\n");
    int fd = sys_creat("TEST.TXT");
    if (fd >= 0) {
        const char msg[] = "Hello from baSic_ kernel!\r\n";
        sys_write(fd, msg, (uint16_t)(sizeof(msg) - 1));
        sys_close(fd);
        bios_puts("File written successfully via syscall backend.\r\n");
    } else {
        bios_puts("No filesystem/syscall backend available yet (sys_creat returned -1).\r\n");
    }

    char line[256];
    char cmd[32];
    char args[220];

    bios_puts("\r\nType 'help' for commands. Press Enter to start.\r\n");
    for (;;) {
        print_prompt();
        bios_readline(line, sizeof(line));
        parse_tokens(line, cmd, args);

        if (kstrcmp(cmd, "") == 0) {
            continue;
        } else if (kstrcmp(cmd, "help") == 0) {
            bios_puts("Commands: ls, cd <dir>, mkdir <name>, rmdir <name>, touch <name>, rm <name>, write <file> <text>, cat <file>, exit, halt\r\n");
        } else if (kstrcmp(cmd, "ls") == 0) {
            /* list current dir */
            fs_list(current_dir);
        } else if (kstrcmp(cmd, "cd") == 0) {
            if (args[0] == '\0') {
                bios_puts("Usage: cd <dirname>\r\n");
            } else {
                fs_cd(args);
            }
        } else if (kstrcmp(cmd, "mkdir") == 0) {
            if (args[0] == '\0') {
                bios_puts("Usage: mkdir <name>\r\n");
            } else {
                fs_mkdir(args);
            }
        } else if (kstrcmp(cmd, "rmdir") == 0) {
            if (args[0] == '\0') {
                bios_puts("Usage: rmdir <name>\r\n");
            } else {
                fs_rmdir(args);
            }
        } else if (kstrcmp(cmd, "touch") == 0) {
            if (args[0] == '\0') {
                bios_puts("Usage: touch <name>\r\n");
            } else {
                fs_touch(args);
            }
        } else if (kstrcmp(cmd, "rm") == 0) {
            if (args[0] == '\0') {
                bios_puts("Usage: rm <name>\r\n");
            } else {
                fs_rm(args);
            }
        } else if (kstrcmp(cmd, "write") == 0) {
            char filename[64];
            char *p = args;
            /* extract filename */
            int i = 0;
            while (*p && *p != ' ' && i < (int)(sizeof(filename)-1)) filename[i++] = *p++;
            filename[i] = '\0';
            while (*p == ' ') p++;
            if (filename[0] == '\0') {
                bios_puts("Usage: write <file> <text>\r\n");
            } else {
                fs_write(filename, p);
            }
        } else if (kstrcmp(cmd, "cat") == 0) {
            if (args[0] == '\0') {
                bios_puts("Usage: cat <file>\r\n");
            } else {
                fs_read(args);
            }
        } else if (kstrcmp(cmd, "exit") == 0 || kstrcmp(cmd, "halt") == 0) {
            bios_puts("Halting kernel... bye.\r\n");
            break;
        } else {
            bios_puts("Unknown command: ");
            bios_puts(cmd);
            bios_puts("\r\n");
        }
    }

    /* Halt forever */
    bios_puts("System halted. Press any key to keep CPU sleeping.\r\n");
    bios_wait_key();
    asm volatile (
        "cli\n\t"
    "1:\n\t"
        "hlt\n\t"
        "jmp 1b\n\t"
    );
}

void _start(void) __attribute__((noreturn));
void _start(void) {
    kmain();
    for (;;) asm volatile("cli\n\thlt");
}
