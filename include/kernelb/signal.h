#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <kernelb/types.h>

#define SIGHUP     1
#define SIGINT     2
#define SIGQUIT    3
#define SIGILL     4
#define SIGTRAP    5
#define SIGABRT    6
#define SIGBUS     7
#define SIGFPE     8
#define SIGKILL    9
#define SIGUSR1    10
#define SIGSEGV    11
#define SIGUSR2    12
#define SIGPIPE    13
#define SIGALRM    14
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
    sigset_t sa_mask;
};

void sigemptyset(sigset_t *set) {
    *set = 0;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
#endif