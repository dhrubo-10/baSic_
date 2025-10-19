#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

void sys_init();
void sys_shutdown();
void sys_reboot();
void sys_info();

/* Kernel panic */
void panic(const char *msg);

/* Simple syscall definitions (mock) */
enum {
    SYS_WRITE = 1,
    SYS_READ,
    SYS_TIME,
    SYS_EXIT
};

long syscall(int num, ...);

#endif




