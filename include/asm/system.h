#ifndef ASM_SYSTEM_H
#define ASM_SYSTEM_H

#include "kernel_b/types.h"

static inline void halt(void) {
    __asm__ volatile ("hlt");
}

#endif
