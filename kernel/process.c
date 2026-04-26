/* baSic_ - kernel/process.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * process table — create and track kernel tasks
 */

#include "process.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static process_t proc_table[PROC_MAX];
static u32       next_pid   = 1;
static u32       current_pid = 0;

void proc_init(void)
{
    memset(proc_table, 0, sizeof(proc_table));
    kprintf("[OK] proc: table ready (%d slots)\n", PROC_MAX);
}

process_t *proc_create(const char *name, u32 entry)
{
    for (int i = 0; i < PROC_MAX; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            process_t *p = &proc_table[i];
            memset(p, 0, sizeof(process_t));

            p->pid   = next_pid++;
            p->state = PROC_READY;
            strncpy(p->name, name, 31);

            /* stack grows downward — set esp to top of stack buffer */
            p->stack_top    = (u32)(p->stack + PROC_STACK_SIZE);
            p->ctx.esp      = p->stack_top;
            p->ctx.eip      = entry;
            p->ctx.eflags   = 0x202;    /* IF=1, reserved bit 1 */

            kprintf("[OK] proc: created '%s' pid=%d entry=%x\n",
                    name, p->pid, entry);
            return p;
        }
    }
    kprintf("[ERR] proc: table full\n");
    return NULL;
}

void proc_cleanup(process_t *p)
{
    if (!p) return;
    kprintf("[OK] proc: cleanup pid=%d '%s'\n", p->pid, p->name);
    memset(p, 0, sizeof(process_t));
    /* state is now PROC_UNUSED (0) */
}

process_t *proc_fork(u32 parent_esp)
{
    process_t *parent = proc_current();
    if (!parent) return NULL;

    process_t *child = NULL;
    for (int i = 0; i < PROC_MAX; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            child = &proc_table[i];
            break;
        }
    }
    if (!child) {
        kprintf("[ERR] proc: fork failed — table full\n");
        return NULL;
    }

    /* copy entire parent struct into child */
    memcpy(child, parent, sizeof(process_t));

    child->pid   = next_pid++;
    child->parent_pid = parent->pid;
    child->state = PROC_READY;

    /* child has its own stack array — fix up esp relative to parent stack */
    u32 parent_stack_base = (u32)parent->stack;
    u32 offset            = parent_esp - parent_stack_base;
    memcpy(child->stack, parent->stack, PROC_STACK_SIZE);
    child->stack_top = (u32)(child->stack + PROC_STACK_SIZE);
    child->ctx.esp   = (u32)(child->stack) + offset;

    /* child returns 0 from fork, parent gets child pid (set in sys_fork) */
    child->ctx.eax = 0;

    kprintf("[OK] proc: fork pid=%d -> child pid=%d\n",
            parent->pid, child->pid);
    return child;
}

i32 proc_wait(u32 child_pid, i32 *exit_code_out)
{
    /* spin — sti/hlt lets scheduler run between checks */
    for (;;) {
        for (int i = 0; i < PROC_MAX; i++) {
            process_t *p = proc_table_get(i);
            if (!p || p->state != PROC_ZOMBIE) continue;
            if (child_pid == (u32)-1) {
                process_t *cur = proc_current();
                if (!cur || p->parent_pid != cur->pid) continue;
            } else {
                if (p->pid != (u32)child_pid) continue;
            }

            if (exit_code_out) *exit_code_out = p->exit_code;
            i32 pid = (i32)p->pid;
            proc_cleanup(p);
            return pid;   /* return pid of reaped child */
        }
        /* let the scheduler run so the child actually gets cpu time */
        __asm__ volatile ("sti; hlt");
    }
}

u32 proc_getppid(void)
{
    process_t *p = proc_current();
    return p ? p->parent_pid : 0; /* yk */
}

process_t *proc_current(void)
{
    for (int i = 0; i < PROC_MAX; i++)
        if (proc_table[i].pid == current_pid)
            return &proc_table[i];
    return NULL;
}

u32 proc_getpid(void) { return current_pid; }

/* used by scheduler to set current */
void proc_set_current(u32 pid) { current_pid = pid; }

process_t *proc_table_get(int i)
{
    if (i < 0 || i >= PROC_MAX) return NULL;
    return &proc_table[i];
}