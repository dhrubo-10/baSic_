/* baSic_ - kernel/signal.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * signal dispatch: pending signal bitmap per process
 * signal_dispatch() is called from the scheduler on each context switch
 */

#include "signal.h"
#include "process.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

typedef struct {
    u32 pid;
    u8  pending;    /* bitmask of pending signals */
    u8  used;
} sig_entry_t;

#define SIG_TABLE_SIZE  16
static sig_entry_t sig_table[SIG_TABLE_SIZE];

void signal_init(void)
{
    memset(sig_table, 0, sizeof(sig_table));
    kprintf("[OK] signal: handler table ready\n");
}

static sig_entry_t *find_or_alloc(u32 pid)
{
    for (int i = 0; i < SIG_TABLE_SIZE; i++)
        if (sig_table[i].used && sig_table[i].pid == pid)
            return &sig_table[i];
    for (int i = 0; i < SIG_TABLE_SIZE; i++) {
        if (!sig_table[i].used) {
            sig_table[i].pid     = pid;
            sig_table[i].pending = 0;
            sig_table[i].used    = 1;
            return &sig_table[i];
        }
    }
    return NULL;
}

void signal_send(u32 pid, int signo)
{
    if (signo < 1 || signo >= SIG_MAX) return;
    sig_entry_t *e = find_or_alloc(pid);
    if (e) {
        e->pending |= (u8)(1 << (signo - 1));
        kprintf("[signal] sent %d to pid %d\n", signo, pid);
    }
}

void signal_dispatch(u32 pid)
{
    sig_entry_t *e = NULL;
    for (int i = 0; i < SIG_TABLE_SIZE; i++)
        if (sig_table[i].used && sig_table[i].pid == pid)
            e = &sig_table[i];
    if (!e || !e->pending) return;

    for (int s = 1; s < SIG_MAX; s++) {
        if (!(e->pending & (1 << (s - 1)))) continue;
        e->pending &= ~(u8)(1 << (s - 1));

        process_t *proc = proc_current();
        switch (s) {
        case SIGKILL:
        case SIGTERM:
            kprintf("[signal] pid %d terminated by signal %d\n", pid, s);
            if (proc) proc->state = PROC_DEAD;
            break;
        case SIGSTOP:
            kprintf("[signal] pid %d stopped\n", pid);
            if (proc) proc->state = PROC_SLEEPING;
            break;
        case SIGCONT:
            kprintf("[signal] pid %d continued\n", pid);
            if (proc) proc->state = PROC_READY;
            break;
        default:
            kprintf("[signal] pid %d: unhandled signal %d\n", pid, s);
            break;
        }
    }
}