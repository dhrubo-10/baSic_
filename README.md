# baSic_ 1.0

A minimal CLI-based experimental operating system, built entirely from scratch. Custom bootloader. Custom kernel. Custom everything.
The project explores kernel development, low-level device interaction, memory management, and console rendering from first principles... no libraries, no OS abstractions, no shortcuts.

## Features
 
**Kernel**
- Two-stage MBR bootloader with LBA disk read
- x86 GDT, IDT, full exception handling with register dumps
- 8259 PIC remapping, hardware IRQ dispatch table
- PIT timer at 1000 Hz, PS/2 keyboard driver, CMOS RTC
- COM1 serial port debug output
- Kernel panic with red full-screen display
**Memory**
- Bitmap physical memory manager (4 KB frames)
- x86 two-level paging, identity mapped 8 MB
- Free-list kernel heap with coalescing (kmalloc / kfree)
**Filesystem**
- Virtual filesystem switch (VFS)
- In-memory ramfs backed by kernel heap
- File descriptor table with per-fd seek offset
**Process and Syscall**
- ELF32 executable loader
- Process table with saved CPU context
- Round-robin scheduler (10 ms quantum, timer-driven)
- int 0x80 syscall interface (exit, write, read, open, close, getpid)
**Shell**
- Interactive shell with fixed header, scrolling output area, dynamic prompt
- Command history (16 deep), Ctrl-P / Ctrl-U shortcuts
- Environment variables (env / export / unset)
- Kernel ring log (dmesg)
- Pipe detection in command lines
**Built-in tools**
- `calc` — integer expression evaluator with full precedence and parentheses
- `edit` — full text editor with line numbers, cursor, save/load from VFS
- `find` / `grep` — recursive filesystem search and pattern matching
- `shoot` — VGA text-mode space shooter with levels and score
---
 
## Building
 
```bash
# Prerequisites
sudo apt install nasm qemu-system-x86 gcc-multilib
 
# Cross-compiler (recommended)
# https://github.com/lordmilko/i686-elf-tools
# Makefile falls back to host gcc -m32 automatically if not found
 
make              # build build/baSic_.img
make run          # launch in QEMU
make run-debug    # QEMU + GDB stub on :1234
make clean
```
 
---
 
## Running
 
```bash
make run
```
 
QEMU opens. Splash screen shows for 2 seconds, then the shell loads.
 
Type `help` for all commands.
 
**Debug with GDB:**
```bash
make run-debug
# in another terminal:
gdb
(gdb) target remote :1234
```
 
---
 
## Shell Commands
 
```
help              list commands
clear             clear shell
echo <text>       print text
calc <expr>       calculator: calc (2+3)*4 = 20
color <0-7>       change shell color
env               show environment variables
export KEY=val    set environment variable
uptime / time     time information
mem               memory usage
sysinfo           system info
dmesg             kernel ring log
ps                process list
history           command history
pwd / cd <dir>    navigation
ls / cat / mkdir  filesystem
write <f> <text>  write to file
find <name>       recursive search
grep <p> <path>   search file
edit <file>       text editor
shoot             shooter game
reboot / halt     power control
```
 
---
 
## Project Structure
 
```
baSic_/
├── boot/           bootloader (stage 1 MBR + stage 2 GDT/PM)
├── kernel/         kernel drivers, shell, editor, game
├── mm/             memory management (PMM, VMM, heap)
├── fs/             filesystem (VFS, ramfs, fd table)
├── lib/            string utilities, kprintf
├── include/        type definitions
├── scripts/        QEMU run script
├── docs/           technical documentation
├── linker.ld       linker script
└── Makefile        build system
```
 
Full technical documentation is in `Documentation/DOCS.md`.
 
