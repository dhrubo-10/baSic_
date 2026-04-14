/* baSic_ - kernel/syscall.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * syscall interface triggered by user programs via int 0x80
 * syscall number in eax, args in ebx ecx edx
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "../include/types.h"
#include "idt.h"

/* syscall numbers */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_GETPID  5

void syscall_init(void);

/* called from the int 0x80 ISR stub */
void syscall_handler(registers_t *regs);

#endif