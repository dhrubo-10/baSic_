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