#include "syscall.h"

extern int _creat(const char *path);
extern int _open(const char *path, uint8_t flags);
extern int _close(int fd);
extern int _read(int fd, void *buf, uint16_t count);
extern int _write(int fd, const void *buf, uint16_t count);
extern long _lseek(int fd, uint32_t offset, uint8_t whence);
extern void _exit(uint8_t code);

int sys_creat(const char *path) { return _creat(path); }
int sys_open(const char *path, uint8_t flags) { return _open(path, flags); }
int sys_close(int fd) { return _close(fd); }
int sys_read(int fd, void *buf, uint16_t count) { return _read(fd, buf, count); }
int sys_write(int fd, const void *buf, uint16_t count) { return _write(fd, buf, count); }
long sys_lseek(int fd, uint32_t offset, uint8_t whence) { return _lseek(fd, offset, whence); }
void sys_exit(uint8_t code) { _exit(code); }
