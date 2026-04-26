/* baSic_ - kernel/process.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * process structure and process table
 */

#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define PROC_MAX        16
#define PROC_STACK_SIZE 4096

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_DEAD,
    PROC_ZOMBIE,   
} proc_state_t;

/* saved CPU register state for context switch */
typedef struct {
    u32 eax, ecx, edx, ebx;
    u32 esp, ebp, esi, edi;
    u32 eip;
    u32 eflags;
} cpu_context_t;

typedef struct {
    u32           pid;
    u32           parent_pid;   /* who forked me */
    i32           exit_code;    /* stored for wait() to read */
    proc_state_t  state;
    cpu_context_t ctx;
    u32           stack_top;
    u8            stack[PROC_STACK_SIZE];
    char          name[32];
} process_t;

void       proc_init(void);
void       proc_cleanup(process_t *p);
void       proc_set_current(u32 pid);
process_t *proc_create(const char *name, u32 entry);
process_t *proc_current(void);
process_t *proc_fork(u32 parent_esp);
process_t *proc_table_get(int i);
u32        proc_getpid(void);
i32 proc_wait(u32 child_pid, i32 *exit_code_out);
u32 proc_getppid(void);

#endif