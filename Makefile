ASM      = nasm
CC       = gcc
LD       = ld
CFLAGS   = -m16 -ffreestanding -fno-stack-protector -fno-pic -nostdlib -nostdinc -Wall
LDFLAGS  = -Ttext 0x100

BOOT_DIR = boot
KERNEL_DIR = kernel
FS_DIR = f_sys

BOOT_OBJS   = $(BOOT_DIR)/boot.o $(BOOT_DIR)/hr.o $(BOOT_DIR)/iR_1.o $(BOOT_DIR)/log.o $(BOOT_DIR)/push_c.o $(BOOT_DIR)/sector.o
KERNEL_OBJS = $(KERNEL_DIR)/bios.o $(KERNEL_DIR)/syscall.o $(KERNEL_DIR)/kernel.o
FS_OBJS     = $(FS_DIR)/io.o $(FS_DIR)/file.o $(FS_DIR)/dir.o $(FS_DIR)/vfs.o

OBJS = $(BOOT_OBJS) $(KERNEL_OBJS) $(FS_OBJS)

TARGET = kernel.bin

all: $(TARGET)

$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.s
	$(ASM) -f elf $< -o $@

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c $(KERNEL_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(FS_DIR)/%.o: $(FS_DIR)/%.c $(FS_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(LD) -m elf_i386 $(LDFLAGS) -o $(TARGET) $(OBJS)


clean:
	rm -f $(BOOT_DIR)/*.o $(KERNEL_DIR)/*.o $(FS_DIR)/*.o $(TARGET)

run: $(TARGET)
	qemu-system-x86_64 -drive format=raw,file=$(TARGET)

.PHONY: all clean run
