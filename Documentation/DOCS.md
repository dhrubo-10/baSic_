# baSic_ — Technical Documentation

**Version:** v1.0
**Author:** Shahriar Dhrubo
**License:** GPL v2
**Architecture:** x86 32-bit protected mode

---

## Overview

baSic_ is an original operating system written from scratch in C and x86 Assembly. It boots from bare metal via a custom MBR bootloader, transitions to 32-bit protected mode through a hand-built GDT, and runs a kernel with memory management, interrupt-driven devices, a virtual filesystem, process scheduler, syscall interface, and an interactive shell.

---

## Project Structure

```
baSic_/
├── boot/
│   ├── boot.asm            stage 1 MBR bootloader
│   └── stage2.asm          stage 2: GDT + protected mode
├── kernel/
│   ├── main.c              kernel entry point
│   ├── vga.c / .h          VGA text-mode driver
│   ├── idt.c / .h          interrupt descriptor table
│   ├── isr.c / .h          CPU exception handlers
│   ├── isr_stubs.asm       ISR stubs (vectors 0–31)
│   ├── irq.c / .h          hardware IRQ dispatch
│   ├── irq_stubs.asm       IRQ stubs (vectors 32–47)
│   ├── pic.c / .h          8259 PIC remapping
│   ├── timer.c / .h        PIT 8253 timer
│   ├── keyboard.c / .h     PS/2 keyboard driver
│   ├── rtc.c / .h          CMOS real-time clock
│   ├── serial.c / .h       COM1 serial port
│   ├── panic.c / .h        kernel panic + ASSERT
│   ├── klog.c / .h         kernel ring buffer log
│   ├── tss.c / .h          Task State Segment (reserved, not active)
│   ├── userspace.c / .h    ring 3 GDT entries (reserved, not active)
│   ├── elf.c / .h          ELF32 loader
│   ├── syscall.c / .h      int 0x80 syscall interface
│   ├── syscall_stub.asm    syscall entry stub
│   ├── process.c / .h      process table
│   ├── sched.c / .h        round-robin scheduler
│   ├── signal.c / .h       signal dispatch
│   ├── terminal.c / .h     VT100 input decoder
│   ├── profiler.c / .h     kernel tick counters
│   ├── watchdog.c / .h     hang detection timer
│   ├── env.c / .h          environment variables
│   ├── calc.c / .h         expression evaluator
│   ├── pipe.c / .h         pipe buffer
│   ├── ata.c / .h          ATA PIO disk driver
│   ├── disk.c / .h         sector cache
│   ├── fat12.c / .h        FAT12 filesystem
│   ├── filemeta.c / .h     file permission table
│   ├── disksync.c / .h     disk flush + INIT.CFG
│   ├── editor.c / .h       text editor
│   ├── shooter.c / .h      VGA shooter game
│   └── shell.c / .h        interactive shell
├── mm/
│   ├── pmm.c / .h          physical memory manager
│   ├── vmm.c / .h          virtual memory manager
│   └── heap.c / .h         kernel heap
├── fs/
│   ├── vfs.c / .h          virtual filesystem switch
│   ├── ramfs.c / .h        in-memory filesystem
│   └── fd.c / .h           file descriptor table
├── lib/
│   ├── string.c / .h       memory and string utilities
│   └── kprintf.c / .h      kernel printf
├── include/
│   └── types.h             u8, u16, u32, u64, PACKED, NORETURN
├── scripts/
│   └── run.sh              QEMU launch script
├── linker.ld               linker script
├── Makefile                build system
├── LICENSE                 GPL v2
└── docs/
    └── DOCS.md            
```

---

## Building

```bash
sudo apt install nasm qemu-system-x86 gcc-multilib

make              # builds build/baSic_.img
make run          # build and launch in QEMU
make run-debug    # QEMU + GDB stub on :1234
make clean
```

For the FAT12 data disk:
```bash
dd if=/dev/zero of=disk.img bs=512 count=2880
mkfs.fat -F 12 disk.img
sudo apt install mtools
echo "hello" > test.txt && mcopy -i disk.img test.txt ::TEST.TXT
```

`run.sh` automatically attaches `disk.img` if it exists.

---

## Boot Sequence

```
BIOS
 └─ MBR at 0x7C00 — INT 13h LBA reads 128 sectors at 0x8000
     └─ stage2.asm
         ├─ lgdt → null / code(0x08) / data(0x10)
         ├─ CR0 PE bit → 32-bit protected mode
         ├─ far jump to flush pipeline
         ├─ segment registers → 0x10, esp → 0x9F000
         └─ call kmain()
```

`kmain()` init order:
```
vga_init       clear screen, disable hardware cursor
idt_init       256-gate IDT, exceptions 0–31
pic_init       remap PIC: IRQ 0–7 → 32–39, IRQ 8–15 → 40–47
irq_init       IRQ stubs at vectors 32–47
timer_init     PIT 1000 Hz
keyboard_init  PS/2 → IRQ 1
rtc_init       CMOS RTC
serial_init    COM1 115200 baud
klog_init      ring log
pmm_init       bitmap PMM
vmm_init       paging, 8MB identity mapped
heap_init      free-list heap at 3MB
vfs_init       VFS layer
ramfs_init     ramfs at /, default tree
fd_init        16-slot fd table
syscall_init   int 0x80 (DPL=3)
proc_init      process table
sched_init     round-robin scheduler
signal_init    signal pending table
env_init       default env vars
prof_init      profiler counters
term_init      terminal decoder
filemeta_init  permission table
watchdog_init  30s watchdog
disk_init      ATA + sector cache
tss_init       TSS descriptor at GDT slot 3, ltr 0x18
userspace_init_gdt  ring 3 code/data at GDT slots 4/5
fat12_init     FAT12 (if disk present)
disksync_run_init  INIT.CFG (if present)
sti            interrupts on
shell_init → shell_run
```

---

## Memory Map

| Address | Contents |
|---|---|
| 0x0000–0x7BFF | BIOS, IVT |
| 0x7C00–0x7DFF | Stage 1 MBR |
| 0x8000–0x9EFFF | Stage 2 + kernel |
| 0x9F000 | Kernel stack top |
| 0x100000 | PMM bitmap |
| 0x200000 | Kernel reserved |
| 0x300000 | Kernel heap (1MB) |
| 0x400000+ | Free frames |
| 0xB8000 | VGA framebuffer |
| 0x500000 | User program area |
| 0x600000 | User stack    |
| 0x700000 | User heap (sbrk) |

---

## Subsystems

### VGA Driver

Direct writes to 0xB8000. Each cell is 2 bytes: ASCII + attribute (bg<<4|fg). Hardware cursor disabled in `vga_init()`. Include guard is `KERNEL_VGA_H` to avoid collision with the `VGA_H 25` constant. `VGA_W 80` and `VGA_H 25` defined in `vga.h`.
VGA attribute controller blink bit (bit 3 of register 0x10) cleared at init — on real hardware bg color ≥ 8 sets the hardware blink bit causing 
visible flicker. Cleared once in vga_init() to enable bright backgrounds.

### Interrupt System

IDT: vectors 0–31 = CPU exceptions, 32–47 = hardware IRQs, 0x80 = syscall. Every ISR stub pushes a dummy error code for exceptions that don't have one so the stack frame is always uniform. PIC remapped via ICW1–ICW4 with `io_wait()` between writes.

### Timer

PIT 8253 channel 0, mode 3. Divisor = 1193182/freq. At 1000Hz = 1ms per tick. `timer_sleep(ms)` halts between ticks. Scheduler called directly from `timer_irq_handler()` — tick count always increments first.

### Keyboard

PS/2 scancode set 1. Port 0x60 on IRQ 1. Tracks `shift_held` and `ctrl_held`. Ctrl+letter = ASCII − 96. Single volatile `last_char` consumed by `keyboard_getchar()`.

### Terminal Driver

Wraps keyboard with PS/2 extended scancode (0xE0 prefix) decoding. Maps to `term_key_type_t`: UP, DOWN, LEFT, RIGHT, HOME, END, DEL, PGUP, PGDN. Shell uses `term_poll()` for full arrow key navigation and mid-line cursor movement.

### RTC

CMOS via ports 0x70/0x71. Waits for update-in-progress flag before reading. Reads seconds twice to catch mid-read updates. Converts BCD→binary based on status register B bit 2. Time displayed in BDT (UTC+6).

### Physical Memory Manager

Bitmap at 0x100000, one bit per 4KB frame. Scans 32 bits at a time. Everything below 2MB reserved. `pmm_alloc()` returns physical address or 0 on OOM.

### Virtual Memory Manager

x86 two-level paging. `vmm_map(virt, phys, flags)` allocates page tables via `pmm_alloc()`, writes PTEs, flushes TLB with `invlpg`. Identity maps 0–8MB at boot.

### Kernel Heap

Free-list at 3MB, 1MB size. Block header: magic `0xB451C000`, size, free flag, next pointer. `kmalloc()` aligns to 8 bytes, splits when there is room. `kfree()` coalesces adjacent free blocks.

### VFS and ramfs

VFS is a pure dispatch layer — `vfs_read/write/finddir` call through function pointers on each node. `vfs_resolve("/a/b/c")` walks path components with `finddir`. ramfs stores files as 4KB `kmalloc`'d buffers. Directories hold 32 child node pointers. The `inode` field in `vfs_node_t` is repurposed as a pointer to ramfs internal data.

Default tree on boot:
```
/
├── etc/
│   └── motd
└── dev/
```

### File Descriptor Table

16 slots. Each entry: `vfs_node_t*` + per-fd seek offset. `fd_open()` finds free slot. `fd_read/write()` advance offset automatically.

### Process

16-slot process table. Each slot holds pid, state, saved cpu context, and a 4KB kernel stack. `proc_create()` finds a free slot, assigns a pid, and sets esp to the top of the stack buffer. `proc_fork()` copies the parent slot into a free child slot, duplicates the kernel stack, fixes up esp to point into the child's own stack buffer, and sets eax=0 in 
the child context so fork returns 0 to the child and the child pid to the parent. `proc_cleanup()` zeroes the slot and resets state to PROC_UNUSED so the slot is reclaimable. 
`sys_exit` calls proc_cleanup before returning no more halting forever on exit.
PROC_ZOMBIE added — process stores exit code and waits for parent to call wait() before slot is reclaimed. proc_wait() spins with sti/hlt yielding to the scheduler until the target child goes zombie, reads the exit code, then calls proc_cleanup(). parent_pid tracked on each process so wait() can verify parentage in future. sys_exit now sets PROC_ZOMBIE and 
stores the exit code instead of halting forever. sys_exec resolves an ELF from VFS, loads it to 0x400000, and queues a new READY process — userspace can spawn processes 
without going through the shell. proc_wait() now accepts -1 as child_pid to reap any child belonging to the current process — returns the pid of the reaped child. proc_getppid() returns parent_pid of the 
current process. sys_exit now sends SIGCHLD to the parent before going zombie so the parent can be notified without polling.

### Scheduler

Round-robin, 10ms quantum. `sched_tick()` called from timer IRQ. Saves current process registers from interrupt frame, finds next READY process, restores its registers into the frame. `iret` resumes new process.

### Signals

16-slot pending signal bitmask table. `signal_send(pid, signo)` sets a bit. `signal_dispatch(pid)` handles: SIGKILL/SIGTERM → PROC_DEAD, SIGSTOP → PROC_SLEEPING, SIGCONT → PROC_READY. Shell command: `kill <pid> <sig>`.

### Syscall Interface

`int 0x80`, DPL=3. `eax` = number, `ebx/ecx/edx` = args. Return in `eax`. Implemented: exit(0), write(1), read(2), open(3), close(4), getpid(5), getenv(6), sleep(7), yield(8), kill(9), uptime(10), fork(11), wait(12), exec(13), getppid(14), sbrk(15): grows userspace heap from base 0x700000 up to 0x800000, sigaction(16), sigreturn(17), pipe(18), dup2(19), sigmask(20).

### ELF Loader

Verifies `\x7FELF`, `ET_EXEC`, `EM_386`. Walks program headers, copies `PT_LOAD` segments to `p_vaddr`, zero-fills BSS. Returns `e_entry`.

### Kernel Panic

`PANIC("msg")` → `kpanic(__FILE__, __LINE__, msg)`. Clears screen to white-on-red, prints message + file + line, sends to COM1, halts. `ASSERT(cond)` calls PANIC if false.

### Kernel Log

64-line circular buffer, 80 chars per line. `klog_write(line)` appends. `klog_dump()` prints in order — used by `dmesg` command.

### Profiler

Five named tick counters: scheduler, irq, syscall, disk, vfs. `prof_inc(counter)` at hot paths. `prof_dump()` prints all — visible in `top` command.

### Watchdog

Countdown timer, default 30s. `watchdog_kick()` resets on every clock tick in `shell_run()`. `watchdog_fired()` returns 1 on timeout — shown in `top`.

### Environment Variables

32-entry key=value table. Keys ≤32 chars, values ≤64 chars. Defaults at boot: OS, VERSION, AUTHOR, ARCH, SHELL. Commands: `env`, `export KEY=val`, `unset KEY`.

### Expression Evaluator

Recursive descent. Grammar: `expr = term {(+|-) term}`, `term = factor {(*|/|%) factor}`, `factor = '(' expr ')' | [-] number`. Returns 1 on success, 0 on error or division by zero.

### ATA Driver

PIO mode, primary bus (0x1F0), master drive. Software reset on init, IDENTIFY to detect. 28-bit LBA. `ata_read/write()` transfer 512 bytes as 256 words via `inw/outw`. 400ns delay (4× alt-status reads) between drive-select and command.

### Disk Cache

16-slot sector cache on top of ATA. Cache hit = no disk access. On miss, evicts oldest dirty slot (write-back). `disk_cache_flush()` writes all dirty slots — called before halt/reboot.

### FAT12 Driver

Reads BPB from boot sector. Computes FAT/root/data LBAs. Caches FAT in memory (up to 2 sectors). `fat12_get/set()` unpacks 12-bit FAT entries with cluster+cluster/2 offset. `fat12_list()` walks root directory. `fat12_read()` follows cluster chain. `fat12_write()` allocates clusters, writes data, creates 8.3 directory entry. `fat12_delete()` marks entry 0xE5, frees cluster chain.

### File Metadata

64-entry permission table. `PERM_READ(0x01)`, `PERM_WRITE(0x02)`, `PERM_EXEC(0x04)`. `filemeta_set/get/check()` by path. System paths get restricted defaults at boot.

### Disk Sync

`disksync_run_init()` reads `INIT.CFG` from FAT12 disk at boot. Supported directives: `motd=<text>` sets `/etc/motd`, `env=KEY=VALUE` sets an env var. Lines beginning with `#` are comments. `disksync_flush()` calls `disk_cache_flush()`.

INIT.CFG format:
```
# baSic_ init
motd=welcome to baSic_
env=HOSTNAME=baSic
```

### Text Editor

128 lines × 80 chars. Screen: row 0 = title bar, rows 1–22 = text with line number gutter, row 23 = status bar, row 24 = message bar. Insert at cursor shifts right. Delete merges lines when at column 0. Ctrl-S saves to VFS, Ctrl-Q quits with unsaved prompt.

### Shooter Game

`shoot` command. Player `(^)` on row 23, moves with A/D. Enemies V/W/M in 3×8 formation march and drop at screen edges. SPACE fires bullets. Score = 10×level per kill. Speed increases per level. Q exits.

### TSS and Userspace

TSS descriptor installed at GDT slot 3 (selector 0x18) by `tss_init()`. Ring 3 code (0x23) and data (0x2B) segments installed at slots 4 and 5 by 
`userspace_init_gdt()`. Both called from `kmain()` after `idt_init()`. `userspace_enter(entry, user_stack)` drops the CPU to ring 3 via `iret`,
pushing ss/esp/eflags/cs/eip onto the kernel stack with IF set. On any interrupt or syscall from ring 3, the CPU uses the TSS esp0/ss0 fields to
switch back to the kernel stack at 0x9F000.

`sys_exit()` handles ring 3 process termination — marks the process DEAD, resets esp/ebp to 0x9F000, and restarts the shell cleanly. `ring3test` shell command 
copies a minimal x86 program blob to 0x500000 and enters ring 3. The program calls sys_write then sys_exit via int 0x80.

---

## Shell

Layout: rows 0–22 scroll terminal output, row 23 = prompt, row 24 = status bar.

### Commands

| Command | Description |
|---|---|
| help | list all commands |
| clear | clear terminal |
| echo \<text\> | print text |
| calc \<expr\> | integer arithmetic |
| env / export / unset | environment variables |
| uptime / time | time info (BDT) |
| mem | memory usage |
| sysinfo | system info |
| dmesg | kernel log |
| top | processes + profiler + watchdog |
| ps | process list |
| kill \<pid\> \<sig\> | send signal |
| spawn \<file\> | load and queue ELF |
| history | command history |
| pwd / cd | navigate |
| ls / cat / mkdir | ramfs |
| write \<f\> \<text\> | write to ramfs |
| find / grep | search |
| diskls | FAT12 root directory |
| diskcat \<file\> | read from disk |
| diskwrite \<f\> \<text\> | write to disk |
| diskdel \<file\> | delete from disk |
| disksync | flush disk cache |
| chmod \<path\> \<rwx\> | set permissions |
| edit \<file\> | text editor |
| shoot | shooter game |
| about | about baSic_ |
| alias | list active aliases from INIT.CFG |
| ring3test | run first ring 3 user process |
| reboot / halt | power |
| poweroff | ACPI shutdown |

---