/* baSic_ - kernel/syscall.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * syscall numbers and interface
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "../include/types.h"
#include "idt.h"

#define SYS_EXIT     0
#define SYS_WRITE    1
#define SYS_READ     2
#define SYS_OPEN     3
#define SYS_CLOSE    4
#define SYS_GETPID   5
#define SYS_GETENV   6
#define SYS_SLEEP    7
#define SYS_YIELD    8
#define SYS_KILL     9
#define SYS_UPTIME   10
#define SYS_FORK     11	/* just copy everything, fix it later */
#define SYS_WAIT     12
#define SYS_EXEC     13

void syscall_init(void);
void syscall_handler(registers_t *regs);

#endif