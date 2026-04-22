/* baSic_ - kernel/userspace.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * user space entry: jump to ring 3 ELF entry point
 */

#ifndef USERSPACE_H
#define USERSPACE_H

#include "../include/types.h"

#define USER_CODE_SEL   0x23    /* GDT entry 4: 0x20 | RPL=3 */
#define USER_DATA_SEL   0x2B    /* GDT entry 5: 0x28 | RPL=3 */
#define USER_STACK_TOP  0xBFF00000

void userspace_init_gdt(void);
void userspace_enter(u32 entry, u32 user_stack);

#endif