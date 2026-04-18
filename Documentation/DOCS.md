# baSic_ — Technical Documentation

**Version:** v1.0  
**Author:** Shahriar Dhrubo  
**License:** GPL v2  
**Architecture:** x86 32-bit protected mode

---

## Overview
It boots from bare metal via a custom MBR bootloader, transitions to 32-bit protected mode, and runs a complete kernel with memory management, an interrupt-driven device layer, a virtual filesystem, a process scheduler, a syscall interface, and an interactive shell.

---

## Project Structure

```
baSic_/
├── boot/
│   ├── boot.asm            stage 1 MBR bootloader
│   └── stage2.asm          stage 2: GDT + protected mode
├── kernel/
│   ├── main.c              kmain() — kernel entry point
│   ├── vga.c / .h          VGA text-mode driver
│   ├── idt.c / .h          interrupt descriptor table
│   ├── isr.c / .h          CPU exception handlers
│   ├── isr_stubs.asm       ISR assembly stubs (vectors 0-31)
│   ├── irq.c / .h          hardware IRQ dispatch
│   ├── irq_stubs.asm       IRQ assembly stubs (vectors 32-47)
│   ├── pic.c / .h          8259 PIC remapping
│   ├── timer.c / .h        PIT 8253 timer driver
│   ├── keyboard.c / .h     PS/2 keyboard driver
│   ├── rtc.c / .h          CMOS real-time clock
│   ├── serial.c / .h       COM1 serial port driver
│   ├── panic.c / .h        kernel panic + ASSERT
│   ├── klog.c / .h         kernel ring buffer log
│   ├── elf.c / .h          ELF32 executable loader
│   ├── syscall.c / .h      int 0x80 syscall interface
│   ├── syscall_stub.asm    syscall entry stub
│   ├── process.c / .h      process table
│   ├── sched.c / .h        round-robin scheduler
│   ├── env.c / .h          environment variables
│   ├── pipe.c / .h         pipe circular buffer
│   ├── calc.c / .h         integer expression evaluator
│   ├── editor.c / .h       text editor
│   ├── shooter.c / .h      VGA text-mode shooter game
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
├── NOTICE                  copyright notice
└── docs/
    └── DOCS.md             this file
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

GDB debugging:
```bash
make run-debug
# in another terminal:
gdb
(gdb) target remote :1234
```

---

## Boot Sequence

```
BIOS
 └─ loads boot.asm (MBR) at 0x7C00
     └─ INT 13h LBA read → loads image at 0x8000
         └─ stage2.asm
             ├─ lgdt → 3-entry flat GDT (null, code, data)
             ├─ set CR0 PE bit → protected mode
             ├─ far jump to flush pipeline
             ├─ set segment registers to 0x10
             ├─ esp = 0x9F000
             └─ call kmain()
```

kmain() initialization order:
```
vga_init        clear screen, disable hardware cursor
idt_init        256-gate IDT, exceptions 0-31 wired
pic_init        remap PIC: IRQ 0-7 → 32-39, IRQ 8-15 → 40-47
irq_init        IRQ stubs registered at vectors 32-47
timer_init      PIT at 1000 Hz
keyboard_init   PS/2 keyboard hooked to IRQ 1
rtc_init        CMOS RTC ready
serial_init     COM1 at 115200 baud
klog_init       ring log cleared
pmm_init        bitmap PMM initialized
vmm_init        paging enabled, 8MB identity mapped
heap_init       free-list heap at 3MB
vfs_init        VFS layer
ramfs_init      ramfs mounted at /, default tree created
fd_init         file descriptor table cleared
syscall_init    int 0x80 gate registered (DPL=3)
proc_init       process table cleared
sched_init      scheduler hooked to timer IRQ
env_init        default environment variables set
sti             interrupts enabled
sched_start     scheduler active
shell_init      splash → header → prompt
shell_run       main loop — never returns
```

---

## Memory Map

| Address | Contents |
|---|---|
| 0x0000 – 0x7BFF | BIOS data, IVT |
| 0x7C00 – 0x7DFF | Stage 1 MBR |
| 0x8000 – 0x9EFFF | Stage 2 + kernel image |
| 0x9F000 | Kernel stack top |
| 0x100000 | PMM bitmap |
| 0x200000 | Kernel reserved boundary |
| 0x300000 | Kernel heap (1 MB) |
| 0x400000+ | PMM-managed free frames |
| 0xB8000 | VGA text framebuffer |

---

## Subsystems

### VGA Driver

Writes directly to physical address 0xB8000. Each cell is 2 bytes: low byte = ASCII character, high byte = color attribute (bg << 4 | fg). Supports 16 foreground and background colors, automatic scrolling, tab stops, and backspace. Hardware blinking cursor is disabled in vga_init().

The include guard for vga.h is KERNEL_VGA_H rather than VGA_H to avoid a name collision with the VGA_H 25 height constant defined in the same file. Both VGA_W 80 and VGA_H 25 are defined in vga.h and shared across all files that need screen dimensions.

### Interrupt System

IDT layout:
- Vectors 0–31: CPU exceptions
- Vectors 32–47: Hardware IRQs (after PIC remap)
- Vector 0x80: syscall gate

Uniform interrupt stack frame:
```
[ESP]   edi esi ebp esp_dummy ebx edx ecx eax   (pusha)
[+32]   int_no
[+36]   err_code
[+40]   eip cs eflags                            (CPU)
```

Exceptions without a hardware error code push a dummy 0 so every handler receives the same frame shape.

PIC remapping sends ICW1–ICW4 to both master (0x20/0x21) and slave (0xA0/0xA1) PICs. io_wait() uses port 0x80 as a delay between writes.

### Timer

PIT 8253 channel 0, mode 3 (square wave). Divisor = 1193182 / freq. At 1000 Hz, one tick per millisecond. timer_sleep(ms) halts the CPU between ticks. The scheduler is called directly from timer_irq_handler() rather than registering a separate IRQ 0 handler, ensuring tick_count always increments first.

### Keyboard

PS/2 scancode set 1. Reads port 0x60 on IRQ 1. Tracks shift_held and ctrl_held via release scancodes (bit 7 set on key release). Ctrl+letter produces control characters (ASCII = letter - 96). Characters are stored in a volatile last_char byte consumed by keyboard_getchar().

### RTC

CMOS registers via index port 0x70 and data port 0x71. Waits for the update-in-progress flag (register 0x0A bit 7) before reading. Reads seconds twice to detect a mid-read update. Converts BCD to binary based on status register B bit 2.

### Physical Memory Manager

Bitmap at physical address 0x100000. One bit per 4 KB frame. Scans 32 bits at a time — full 32-bit words are skipped. Everything below 2 MB is reserved at initialization. pmm_alloc() returns a physical frame address or 0 on out-of-memory.

### Virtual Memory Manager

x86 two-level paging. A 4 KB-aligned page directory with 1024 entries, each pointing to a 1024-entry page table. vmm_map(virt, phys, flags) allocates new page tables via pmm_alloc() as needed, writes the PTE, and flushes the TLB with invlpg. Identity maps the first 8 MB so the kernel stays accessible after paging is enabled.

### Kernel Heap

Free-list allocator at physical address 3 MB, 1 MB total. Each block header contains a magic value 0xB451C000, the usable size, a free flag, and a next pointer. kmalloc() aligns sizes to 8 bytes and splits blocks when there is room for a new header plus at least 8 bytes. kfree() marks the block free and coalesces adjacent free blocks.

### VFS and ramfs

The VFS is a pure dispatch layer. Every vfs_node_t carries function pointers for read, write, and finddir. All kernel code calls through these pointers rather than calling ramfs functions directly. vfs_resolve("/a/b/c") walks path components by calling finddir at each level.

ramfs stores file data as kmalloc'd 4 KB buffers. Directories hold an array of 32 child node pointers. The inode field in vfs_node_t is reused as a raw pointer to the ramfs internal data structure.

Default tree on boot:
```
/
├── etc/
│   └── motd
└── dev/
```

### File Descriptor Table

16 slots. Each entry holds a pointer to a vfs_node_t and a per-fd seek offset. fd_open() finds the first free slot and returns its index. fd_read() and fd_write() advance the offset after each operation.

### Scheduler

Round-robin, 10 ms quantum (10 ticks at 1000 Hz). sched_tick() is called on every timer interrupt. It saves the current process's registers from the interrupt frame, finds the next READY process, and restores that process's registers into the same frame. The iret ending the interrupt resumes the new process. No separate context-switch trampoline is needed.

### Syscall Interface

int 0x80, gate registered with DPL=3 so user-space code can trigger it without a GPF. eax = syscall number, ebx/ecx/edx = arguments. Return value is written back to eax in the interrupt frame. Implemented: exit (0), write (1), read (2), open (3), close (4), getpid (5).

### ELF Loader

Verifies magic bytes \x7FELF, checks e_type == ET_EXEC and e_machine == EM_386. Iterates program headers, processes every PT_LOAD segment by copying p_filesz bytes to p_vaddr, then zero-fills p_memsz - p_filesz bytes for BSS. Returns e_entry.

### Kernel Panic

PANIC("message") expands to kpanic(__FILE__, __LINE__, msg). Disables interrupts, clears the screen to white-on-red, prints a banner with the message, source file, and line number, sends the same to COM1, then halts. ASSERT(condition) calls PANIC if the condition is false.

### Kernel Log

64-line circular ring buffer. Each entry up to 80 characters. New entries overwrite the oldest when full. klog_dump() prints all stored lines in order for the dmesg command.

### Environment Variables

32-entry linear key=value table. Keys up to 32 characters, values up to 64. Default values at boot: OS, VERSION, AUTHOR, ARCH, SHELL.

### Pipe Buffer

512-byte circular byte buffer. pipe_write() and pipe_read() operate independently on write_pos and read_pos. The shell splits command lines on | and initializes the pipe buffer for the pair.

### Expression Evaluator

Recursive descent parser. Grammar:
```
expr   = term   { ('+' | '-') term   }
term   = factor { ('*' | '/' | '%') factor }
factor = '(' expr ')' | ['-'] number
```
Returns 1 on success, 0 on parse error or division by zero.

### Text Editor

Fixed buffer of 128 lines × 80 characters. Screen layout:
```
row 0      title bar  (filename, modified flag, hints)
rows 1-22  text area  (line number gutter + content)
row 23     status bar (Ln: Col: Lines:)
row 24     message bar
```

Controls:

| Key | Action |
|---|---|
| type | insert character |
| Backspace | delete left / merge lines |
| Enter | split line at cursor |
| Ctrl-A | start of line |
| Ctrl-E | end of line |
| Ctrl-K | delete current line |
| Ctrl-S | save to VFS |
| Ctrl-Q | quit (prompts if unsaved) |

### Shooter Game

VGA text-mode space shooter launched by the shoot command. Player (^) moves along the bottom row. Enemies V W M march in formation and drop down at screen edges. SPACE fires bullets upward. Score increases by 10 × level per kill. Q exits and restores the shell.

---

## Shell

The header (rows 0–4) is fixed and never scrolls. Output scrolls between rows 6 and 22. The prompt is pinned to row 23.

The prompt shows the last component of the current working directory. At root it shows baSic_.

Keyboard shortcuts: Ctrl-P = previous history entry, Ctrl-U = clear line.

### Commands

| Command | Description |
|---|---|
| help | list all commands |
| clear | clear shell output |
| echo \<text\> | print text |
| calc \<expr\> | integer expression evaluator |
| color \<0-7\> | change shell background color |
| env | print environment variables |
| export KEY=val | set environment variable |
| unset KEY | remove environment variable |
| uptime | time since boot |
| time | current date/time from RTC |
| mem | memory usage |
| sysinfo | system information |
| dmesg | kernel ring log |
| ps | process list |
| history | command history (last 16) |
| pwd | print working directory |
| cd \<dir\> | change directory |
| ls | list current directory |
| cat \<path\> | print file contents |
| write \<file\> \<text\> | write text to file |
| mkdir \<name\> | create directory |
| find \<name\> | recursive search |
| grep \<pattern\> \<path\> | search file for pattern |
| edit \<file\> | open text editor |
| shoot | launch shooter game |
| about | about baSic_ |
| reboot | reboot via 8042 reset |
| halt | halt system |

---