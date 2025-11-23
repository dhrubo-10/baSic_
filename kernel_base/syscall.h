#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

int sys_creat(const char *path);
int sys_open(const char *path, uint8_t flags);
int sys_close(int fd);
int sys_read(int fd, void *buf, uint16_t count);
int sys_write(int fd, const void *buf, uint16_t count);
long sys_lseek(int fd, uint32_t offset, uint8_t whence);

/* Exit */
void sys_exit(uint8_t code);

#endif
