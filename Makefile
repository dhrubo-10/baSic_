CC = gcc
AS = nasm
LD = ld

CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra -nostdlib -fno-stack-protector \
         -fno-builtin -fno-PIC -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL_SOURCES = \
	boot/boot.asm \
	kernel_base/entry.asm \
	kernel_base/main.c \
	kernel_base/printk.c \
	kernel_base/panic.c \
	mm/mm.c \
	drivers/serial.c \
	drivers/vga.c \
	usr/init.c

FS_SOURCES = \
	f_sys/cored.c \
	f_sys/dir.c \
	f_sys/file.c \
	f_sys/inode.c \
	f_sys/mount.c \
	f_sys/vfs.c

KERNEL_OBJS = $(patsubst %.c,%.o,$(filter %.c,$(KERNEL_SOURCES))) \
              $(patsubst %.asm,%.o,$(filter %.asm,$(KERNEL_SOURCES)))

FS_OBJS = $(patsubst %.c,%.o,$(FS_SOURCES))

TARGET = kernel.bin
ISO = os.iso

all: $(ISO)

$(ISO): $(TARGET)
	mkdir -p iso/boot/grub
	cp $(TARGET) iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o $@ iso

$(TARGET): $(KERNEL_OBJS) $(FS_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -serial stdio

debug: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -serial stdio -s -S

clean:
	rm -f $(KERNEL_OBJS) $(FS_OBJS) $(TARGET) $(ISO)
	rm -rf iso

fsclean:
	rm -f $(FS_OBJS)

.PHONY: all clean fsclean run debug