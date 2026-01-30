#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "types.h"

#define SIGINT     2
#define SIGILL     4
#define SIGABRT    6
#define SIGFPE     8
#define SIGSEGV    11
#define SIGTERM    15
#define SIGCHLD    17
#define SIGCONT    18
#define SIGSTOP    19
#define SIGTSTP    20
#define SIGTTIN    21
#define SIGTTOU    22

#define SA_NOCLDSTOP   0x00000001
#define SA_NOCLDWAIT   0x00000002
#define SA_SIGINFO     0x00000004
#define SA_ONSTACK     0x00000008
#define SA_RESTART     0x00000010
#define SA_NODEFER     0x00000020
#define SA_RESETHAND   0x00000040
#define SA_RESTORER    0x00000080

typedef void (*sighandler_t)(int);

struct sigaction {
    sighandler_t sa_handler;
    unsigned long sa_flags;
    void (*sa_restorer)(void);
};

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int kill(pid_t pid, int sig);
int raise(int sig);

#endif