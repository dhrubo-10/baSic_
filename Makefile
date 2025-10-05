ASM = nasm
CC = i686-elf-gcc
LD = i686-elf-ld

CFLAGS = -m32 -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie -Wall -Wextra
LDFLAGS = -Ttext 0x1000 --oformat binary

BUILD = build
BOOT = boot/boot.asm
KERNEL = kernel/kernel.c
BOOT_BIN = $(BUILD)/boot.bin
KERNEL_BIN = $(BUILD)/kernel.bin
OS_IMG = $(BUILD)/os.img

all: $(OS_IMG)

$(BUILD):
	mkdir -p $(BUILD)
$(BOOT_BIN): $(BOOT) | $(BUILD)
	$(ASM) -f bin $< -o $@

$(KERNEL_BIN): $(KERNEL) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $(BUILD)/kernel.o
	$(LD) $(LDFLAGS) $(BUILD)/kernel.o -o $@
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMG)
	@echo "[+] Bootable image created: $(OS_IMG)"

run: $(OS_IMG)
	qemu-system-i386 -drive format=raw,file=$(OS_IMG)

clean:
	rm -rf $(BUILD)

.PHONY: all clean run
