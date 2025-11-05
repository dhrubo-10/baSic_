#ifndef ASM_CPU_H
#define ASM_CPU_H

#include "kernel_b/types.h"

struct cpu_state {
    u32 eax, ebx, ecx, edx;
    u32 esi, edi, ebp;
    u32 eip, cs, eflags;
};

#endif
