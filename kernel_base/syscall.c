#include "syscall.h"

/*
 * Provided thin wrappers used by kernel code. The underlying names with a
 * leading underscore are expected to be provided by the
 * low-level runtime / filesystem / syscall layer.
 */

/* Weak syscall backends (defaults return error / do nothing). */
int __attribute__((weak)) _creat(const char *path) {
    (void)path;
    return -1; /* not implemented */
}
int __attribute__((weak)) _open(const char *path, uint8_t flags) {
    (void)path; (void)flags;
    return -1;
}
int __attribute__((weak)) _close(int fd) {
    (void)fd;
    return -1;
}
int __attribute__((weak)) _read(int fd, void *buf, uint16_t count) {
    (void)fd; (void)buf; (void)count;
    return -1;
}
int __attribute__((weak)) _write(int fd, const void *buf, uint16_t count) {
    (void)fd; (void)buf; (void)count;
    return -1;
}
long __attribute__((weak)) _lseek(int fd, uint32_t offset, uint8_t whence) {
    (void)fd; (void)offset; (void)whence;
    return -1;
}
void __attribute__((weak)) _exit(uint8_t code) {
    (void)code;
    /* Default: hang the CPU */
    for (;;) asm volatile("cli\n\thlt");
}

/* Public wrappers used by kernel code */
int sys_creat(const char *path) { return _creat(path); }
int sys_open(const char *path, uint8_t flags) { return _open(path, flags); }
int sys_close(int fd) { return _close(fd); }
int sys_read(int fd, void *buf, uint16_t count) { return _read(fd, buf, count); }
int sys_write(int fd, const void *buf, uint16_t count) { return _write(fd, buf, count); }
long sys_lseek(int fd, uint32_t offset, uint8_t whence) { return _lseek(fd, offset, whence); }
void sys_exit(uint8_t code) { _exit(code); }
