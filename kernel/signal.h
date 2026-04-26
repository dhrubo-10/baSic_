/* baSic_ - kernel/signal.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * basic signal system; SIGKILL, SIGTERM, SIGSTOP, SIGCONT
 */

#ifndef SIGNAL_H
#define SIGNAL_H

#include "../include/types.h"

#define SIGTERM   1
#define SIGKILL   2
#define SIGSTOP   3
#define SIGCONT   4
#define SIGSEGV   5
#define SIGFPE    6
#define SIG_MAX   7
#define SIGCHLD   8

typedef void (*sighandler_t)(int signo);

void signal_init(void);
void signal_send(u32 pid, int signo);
void signal_dispatch(u32 pid);

#endif