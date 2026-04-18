/* baSic_ - kernel/panic.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel panic and assert
 */

#ifndef PANIC_H
#define PANIC_H

#include "../include/types.h"

void kpanic(const char *file, u32 line, const char *msg) NORETURN;

#define PANIC(msg)          kpanic(__FILE__, __LINE__, msg)
#define ASSERT(cond)        do { if (!(cond)) PANIC("assert: " #cond); } while(0)
#define ASSERT_MSG(cond, m) do { if (!(cond)) PANIC(m); } while(0)

#endif