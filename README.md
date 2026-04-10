# baSic_ 1.0

A minimal **CLI-based experimental operating system**, built from scratch for low-level exploration of **kernel development, device interaction, and console rendering**.


Beside the custom Linux kernel I am working on, this is completely built from scratch, OS which will have its own console, memory management, event system etc etc and will include process management, filesystem basics, and syscall handling.

```bash
# Prerequisites
sudo apt install nasm qemu-system-x86
# Cross-compiler (recommended):
# https://github.com/lordmilko/i686-elf-tools
 
make        # builds build/dhrubox.img
make run    # launches in QEMU
make run-debug   # QEMU + GDB stub on :1234
make clean
```