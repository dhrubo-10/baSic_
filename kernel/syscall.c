/* baSic_ - kernel/syscall.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * syscall dispatch — int 0x80
 * eax = syscall number
 * ebx = arg1, ecx = arg2, edx = arg3
 * return value written back into eax
 */

#include "syscall.h"
#include "idt.h"
#include "vga.h"
#include "../fs/fd.h"
#include "../lib/kprintf.h"

/* int 0x80 stub defined in syscall_stub.asm */
extern void syscall_stub(void);

static i32 sys_exit(i32 code)
{
    kprintf("[proc] exit(%d)\n", code);
    /* for now just halt — proper process cleanup comes with scheduler */
    for (;;) __asm__ volatile ("hlt");
    return 0;
}

static i32 sys_write(i32 fd, const u8 *buf, u32 count)
{
    if (fd == 1 || fd == 2) {
        /* stdout / stderr — write directly to VGA via kprintf */
        for (u32 i = 0; i < count; i++)
            vga_putchar((char)buf[i]);
        return (i32)count;
    }
    return fd_write(fd, (u8 *)buf, count);
}

static i32 sys_read(i32 fd, u8 *buf, u32 count)
{
    return fd_read(fd, buf, count);
}

static i32 sys_open(const char *path, i32 flags)
{
    (void)flags;
    extern vfs_node_t *vfs_resolve(const char *path);
    vfs_node_t *node = vfs_resolve(path);
    if (!node) return -1;
    return fd_open(node);
}

static i32 sys_close(i32 fd)
{
    fd_close(fd);
    return 0;
}

static i32 sys_getpid(void)
{
    return 1;   /* single process for now */
}

void syscall_handler(registers_t *regs)
{
    i32 ret = -1;

    switch (regs->eax) {
    case SYS_EXIT:   ret = sys_exit((i32)regs->ebx);                               break;
    case SYS_WRITE:  ret = sys_write((i32)regs->ebx, (u8 *)regs->ecx, regs->edx); break;
    case SYS_READ:   ret = sys_read((i32)regs->ebx, (u8 *)regs->ecx, regs->edx);  break;
    case SYS_OPEN:   ret = sys_open((const char *)regs->ebx, (i32)regs->ecx);      break;
    case SYS_CLOSE:  ret = sys_close((i32)regs->ebx);                              break;
    case SYS_GETPID: ret = sys_getpid();                                            break;
    default:
        kprintf("[WARN] unknown syscall %d\n", regs->eax);
        ret = -1;
    }

    regs->eax = (u32)ret;
}

void syscall_init(void)
{
    /* register int 0x80 DPL=3 so user space can trigger it */
    idt_set_gate(0x80, (u32)syscall_stub, 0x08, 0xEE);
    kprintf("[OK] syscall: int 0x80 ready\n");
}