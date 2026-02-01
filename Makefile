# Main Makefile
CC = gcc
AS = nasm
LD = ld

CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra -nostdlib \
         -fno-stack-protector -fno-builtin -fno-PIC -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL_OBJS = \
	kernelb/core/main.o \
	kernelb/core/printk.o \
	kernelb/core/panic.o \
	kernelb/arch/x86/io.o \
	kernelb/arch/x86/gdt.o \
	kernelb/drivers/vga.o \
	kernelb/drivers/keyboard.o \
	fs/vfs.o \
	fs/fat.o \
	fs/file.o \
	fs/inode.o \
	mm/heap.o \
	mm/paging.o

BOOT_OBJS = boot/entry.o
KERNEL_BIN = kernel.bin
BOOT_BIN = boot/boot.bin
OS_IMG = os.img

all: $(OS_IMG)

$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMG)
	truncate -s 1440k $(OS_IMG)

$(KERNEL_BIN): $(KERNEL_OBJS) $(BOOT_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BOOT_BIN):
	$(MAKE) -C boot

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

run: $(OS_IMG)
	qemu-system-i386 -fda $(OS_IMG) -serial stdio

clean:
	$(MAKE) -C boot clean
	rm -f $(KERNEL_OBJS) $(BOOT_OBJS) $(KERNEL_BIN) $(OS_IMG)

.PHONY: all clean run