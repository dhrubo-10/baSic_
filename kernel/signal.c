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
    u8  mask;
    u8  used;
    sighandler_t handlers[SIG_MAX];
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

void signal_set_handler(u32 pid, int signo, sighandler_t handler)
{
    if (signo < 1 || signo >= SIG_MAX) return;
    sig_entry_t *e = find_or_alloc(pid);
    if (e) e->handlers[signo - 1] = handler;
}

sighandler_t signal_get_handler(u32 pid, int signo)
{
    if (signo < 1 || signo >= SIG_MAX) return NULL;
    for (int i = 0; i < SIG_TABLE_SIZE; i++)
        if (sig_table[i].used && sig_table[i].pid == pid)
            return sig_table[i].handlers[signo - 1];
    return NULL;
}

void signal_set_mask(u32 pid, i32 how, u8 mask)
{
    sig_entry_t *e = find_or_alloc(pid);
    if (!e) return;
    if (how == 0) e->mask |= mask;       /* 0 */
    else if (how == 1) e->mask &= ~mask; /* 1 */
    else if (how == 2) e->mask  = mask;  /* 2 */ /* extern syscall mental model "in case u dont get it" */
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
        if (e->mask & (1 << (s - 1))) continue;  
        e->pending &= ~(u8)(1 << (s - 1));

        if (e->handlers[s - 1] &&
            (usize)e->handlers[s - 1] > 1) {
            e->handlers[s - 1](s);
            continue;
        }

        process_t *proc = proc_current();
        switch (s) {
        case SIGKILL:
        case SIGTERM:
            kprintf("[signal] pid %d terminated by signal %d\n", pid, s);
            if (proc) { proc->exit_code = -s; proc->state = PROC_ZOMBIE; }
            break;
        case SIGSTOP:
            kprintf("[signal] pid %d stopped\n", pid);
            if (proc) proc->state = PROC_SLEEPING;
            break;
        case SIGCONT:
            kprintf("[signal] pid %d continued\n", pid);
            if (proc) proc->state = PROC_READY;
            break;
        case SIGCHLD:
            kprintf("[signal] pid %d got SIGCHLD\n", pid);
            break;
        default:
            kprintf("[signal] pid %d: unhandled signal %d\n", pid, s);
            break;
        }
    }
}