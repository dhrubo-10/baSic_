#ifndef _UNISTD_H
#define _UNISTD_H

#include "types.h"

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
gid_t getgid(void);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);
void _exit(int status);
char *getprogname(void);
ssize_t write(int fd, const void *buf, size_t count);

#endif