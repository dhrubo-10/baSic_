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
#include "signal.h"
#include "env.h"
#include "profiler.h"
#include "watchdog.h"
#include "terminal.h"
#include "disk.h"
#include "fat12.h"
#include "filemeta.h"
#include "disksync.h"
#include "shell.h"
#include "tss.h"
#include "userspace.h"
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
    tss_init();
    userspace_init_gdt();
    proc_init();
    sched_init();
    signal_init();
    env_init();
    prof_init();
    term_init();
    filemeta_init();
    watchdog_init(30000);

    disk_init();
    if (fat12_init())
        disksync_run_init();

    __asm__ volatile ("sti");
    sched_start();

    shell_init();
    shell_run();
}