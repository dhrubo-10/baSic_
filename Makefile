CC = gcc
AS = nasm
LD = ld

CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra -nostdlib -fno-stack-protector \
         -fno-builtin -fno-PIC -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL_OBJS = kernel/main.o kernel/printk.o boot/entry.o
KERNEL_BIN = kernel.bin
BOOT_BIN = boot/boot.bin
OS_IMG = os.img

all: $(OS_IMG)

$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMG)
	truncate -s 1440k $(OS_IMG)

$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BOOT_BIN):
	$(MAKE) -C boot boot.bin

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

boot/entry.o: boot/entry.asm
	$(AS) $(ASFLAGS) $< -o $@

run: $(OS_IMG)
	qemu-system-i386 -fda $(OS_IMG)

debug: $(OS_IMG)
	qemu-system-i386 -fda $(OS_IMG) -s -S

clean:
	$(MAKE) -C boot clean
	rm -f $(KERNEL_OBJS) $(KERNEL_BIN) $(OS_IMG)

.PHONY: all clean run debug