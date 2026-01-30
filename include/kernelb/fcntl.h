#ifndef _FCNTL_H
#define _FCNTL_H

#include "types.h"

#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_NONBLOCK  0x0800

#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4

int open(const char *pathname, int flags, mode_t mode);
int creat(const char *pathname, mode_t mode);
int fcntl(int fd, int cmd, ...);

#endif