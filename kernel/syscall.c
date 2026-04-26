/* baSic_ - kernel/syscall.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * syscall dispatch — int 0x80
 * eax = number, ebx = arg1, ecx = arg2, edx = arg3
 */

#include "syscall.h"
#include "idt.h"
#include "vga.h"
#include "timer.h"
#include "env.h"
#include "signal.h"
#include "process.h"
#include "profiler.h"
#include "../fs/fd.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

extern void syscall_stub(void);

static i32 sys_exit(i32 code)
{
    kprintf("[proc] exit(%d)\n", code);
    process_t *p = proc_current();
    if (p) {
        p->exit_code = code;
        p->state     = PROC_ZOMBIE;  /* parent needs to wait() first */
    }
    proc_set_current(1);
    return 0;
}

static i32 sys_fork(void)
{
    u32 cur_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r"(cur_esp));
    process_t *child = proc_fork(cur_esp);
    if (!child) return -1;
    return (i32)child->pid;
}

static i32 sys_write(i32 fd, const u8 *buf, u32 count)
{
    prof_inc(PROF_SYSCALL);
    if (fd == 1 || fd == 2) {
        for (u32 i = 0; i < count; i++)
            vga_putchar((char)buf[i]);
        return (i32)count;
    }
    return fd_write(fd, (u8 *)buf, count);
}

static i32 sys_read(i32 fd, u8 *buf, u32 count)
{
    prof_inc(PROF_SYSCALL);
    return fd_read(fd, buf, count);
}

static i32 sys_open(const char *path, i32 flags)
{
    (void)flags;
    prof_inc(PROF_VFS);
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
    process_t *p = proc_current();
    return p ? (i32)p->pid : 1;
}

static i32 sys_getenv(const char *key, char *buf, u32 buflen)
{
    const char *val = env_get(key);
    if (!val) return -1;
    strncpy(buf, val, buflen - 1);
    buf[buflen - 1] = '\0';
    return (i32)strlen(val);
}

static i32 sys_sleep(u32 ms)
{
    timer_sleep(ms);
    return 0;
}

static i32 sys_yield(void)
{
    __asm__ volatile ("int $0x80" : : "a"(0xFF));
    return 0;
}

static i32 sys_kill(u32 pid, i32 signo)
{
    signal_send(pid, signo);
    return 0;
}

static i32 sys_uptime(void)
{
    return (i32)(timer_ticks() / 1000);
}

static i32 sys_wait(u32 child_pid, i32 *exit_code_out)
{
    return proc_wait(child_pid, exit_code_out);
}

static i32 sys_exec(const char *path)
{
    extern vfs_node_t *vfs_resolve(const char *path);
    extern u32 elf_load(u8 *buf, u32 size);

    vfs_node_t *node = vfs_resolve(path);
    if (!node || !(node->flags & VFS_FILE)) return -1;

    /* reuse the spawn buffer — dont overlap with kernel */
    u8 *elf_buf = (u8 *)0x400000;
    u32 n = vfs_read(node, 0, 0x100000, elf_buf);
    if (!n) return -1;

    u32 entry = elf_load(elf_buf, n);
    if (!entry) return -1;

    process_t *proc = proc_create(path, entry);
    if (!proc) return -1;

    proc->parent_pid = proc_getpid();
    proc->state      = PROC_READY;
    return (i32)proc->pid;
}

void syscall_handler(registers_t *regs)
{
    i32 ret = -1;
    switch (regs->eax) {
    case SYS_EXIT:    ret = sys_exit((i32)regs->ebx);                                       break;
    case SYS_WRITE:   ret = sys_write((i32)regs->ebx,(u8*)regs->ecx,regs->edx);             break;
    case SYS_READ:    ret = sys_read((i32)regs->ebx,(u8*)regs->ecx,regs->edx);              break;
    case SYS_OPEN:    ret = sys_open((const char*)regs->ebx,(i32)regs->ecx);                break;
    case SYS_CLOSE:   ret = sys_close((i32)regs->ebx);                                      break;
    case SYS_GETPID:  ret = sys_getpid();                                                   break;
    case SYS_GETENV:  ret = sys_getenv((const char*)regs->ebx,(char*)regs->ecx,regs->edx);  break;
    case SYS_SLEEP:   ret = sys_sleep(regs->ebx);                                           break;
    case SYS_YIELD:   ret = sys_yield();                                                    break;
    case SYS_KILL:    ret = sys_kill(regs->ebx,(i32)regs->ecx);                             break;
    case SYS_UPTIME:  ret = sys_uptime();                                                   break;
    case SYS_FORK:    ret = sys_fork();                                                     break;
    case SYS_WAIT:    ret = sys_wait(regs->ebx, (i32 *)regs->ecx);                          break;
    case SYS_EXEC:    ret = sys_exec((const char *)regs->ebx);                              break;
    default:
        kprintf("[WARN] unknown syscall %d\n", regs->eax);
        ret = -1;
    }
    regs->eax = (u32)ret;
}

void syscall_init(void)
{
    idt_set_gate(0x80, (u32)syscall_stub, 0x08, 0xEE);
    kprintf("[OK] syscall: int 0x80 ready\n");
}