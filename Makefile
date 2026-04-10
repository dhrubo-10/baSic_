# DhrubOS Makefile
# Toolchain: nasm, i686-elf-gcc (cross-compiler) or gcc -m32 as fallback

AS      := nasm
CC      := i686-elf-gcc
LD      := i686-elf-ld

# Fallback to host gcc with -m32 if cross-compiler isn't installed
ifeq (, $(shell which $(CC) 2>/dev/null))
    CC  := gcc
    LD  := ld
    $(warning Cross-compiler not found, using host gcc -m32. Install i686-elf-gcc for best results.)
endif

ASFLAGS  := -f elf32
CFLAGS   := -m32 \
             -ffreestanding \
             -fno-stack-protector \
             -fno-builtin \
             -fno-pic \
             -nostdlib \
             -nostdinc \
             -Wall \
             -Wextra \
             -O2 \
             -I include
LDFLAGS  := -T linker.ld \
             -melf_i386 \
             --oformat binary


BUILD_DIR := build
ISO_DIR   := iso

BOOT_SRCS  := boot/stage2.asm
KERNEL_SRCS := kernel/main.c \
               kernel/vga.c

BOOT_OBJS   := $(patsubst boot/%.asm,   $(BUILD_DIR)/boot/%.o,   $(BOOT_SRCS))
KERNEL_OBJS := $(patsubst kernel/%.c,   $(BUILD_DIR)/kernel/%.o, $(KERNEL_SRCS))

ALL_OBJS := $(BOOT_OBJS) $(KERNEL_OBJS)

MBR_BIN     := $(BUILD_DIR)/boot.bin
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
OS_IMAGE    := $(BUILD_DIR)/baSic_.img


.PHONY: all
all: $(OS_IMAGE)


$(OS_IMAGE): $(MBR_BIN) $(KERNEL_BIN)
	@echo "[IMG] $@"
	@cat $(MBR_BIN) $(KERNEL_BIN) > $@
	@# Pad to a round number of sectors (multiple of 512)
	+	@python3 -c "\
import os; \
target = 65 * 512; \
size = os.path.getsize('$@'); \
open('$@', 'ab').write(b'\\x00' * max(0, target - size))"
	@echo "[OK] Disk image: $@ ($(shell wc -c < $@) bytes)"


$(MBR_BIN): boot/boot.asm | $(BUILD_DIR)/boot
	@echo "[AS] $<"
	@$(AS) -f bin $< -o $@


$(KERNEL_BIN): $(ALL_OBJS) linker.ld
	@echo "[LD] $@"
	@$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)


$(BUILD_DIR)/boot/%.o: boot/%.asm | $(BUILD_DIR)/boot
	@echo "[AS] $<"
	@$(AS) $(ASFLAGS) $< -o $@


$(BUILD_DIR)/kernel/%.o: kernel/%.c | $(BUILD_DIR)/kernel
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/boot:
	@mkdir -p $@

$(BUILD_DIR)/kernel:
	@mkdir -p $@


.PHONY: run
run: $(OS_IMAGE)
	@./scripts/run.sh $(OS_IMAGE)


.PHONY: run-debug
run-debug: $(OS_IMAGE)
	@./scripts/run.sh $(OS_IMAGE) debug


.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "[OK] Cleaned."
