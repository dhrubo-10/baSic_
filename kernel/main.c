/* baSic_ - kernel/main.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel entry point, called from stage2 after protected mode
 */

#include "vga.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "rtc.h"
#include "serial.h"
#include "panic.h"
#include "klog.h"
#include "syscall.h"
#include "process.h"
#include "sched.h"
#include "env.h"
#include "shell.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../fs/fd.h"
#include "../include/types.h"

#define MEM_KB  32768

void kmain(void)
{
    vga_init();
    idt_init();
    pic_init();
    irq_init();
    timer_init(1000);
    keyboard_init();
    rtc_init();
    serial_init();
    klog_init();
    pmm_init(MEM_KB);
    vmm_init();
    heap_init();

    vfs_init();
    vfs_node_t *root = ramfs_init();
    vfs_set_root(root);
    fd_init();

    syscall_init();
    proc_init();
    sched_init();
    env_init();

    __asm__ volatile ("sti");
    sched_start();

    shell_init();
    shell_run();
}