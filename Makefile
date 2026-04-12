# baSic_ Makefile

AS      := nasm
CC      := i686-elf-gcc
LD      := i686-elf-ld

ifeq (, $(shell which $(CC) 2>/dev/null))
    CC  := gcc
    LD  := ld
    $(warning Cross-compiler not found, using host gcc -m32.)
endif

ASFLAGS := -f elf32

CFLAGS  := -m32              \
            -ffreestanding   \
            -fno-stack-protector \
            -fno-builtin     \
            -fno-pic         \
            -nostdlib        \
            -nostdinc        \
            -Wall            \
            -Wextra          \
            -O2              \
            -I include

LDFLAGS := -T linker.ld      \
            -melf_i386       \
            --oformat binary

BUILD_DIR := build

BOOT_SRCS    := boot/stage2.asm

KERNEL_SRCS  := kernel/main.c  \
                kernel/vga.c   \
                kernel/idt.c   \
                kernel/isr.c   \
                kernel/irq.c   \
                kernel/pic.c

KERNEL_ASMS  := kernel/isr_stubs.asm  \
                kernel/irq_stubs.asm

LIB_SRCS     := lib/string.c  \
                lib/kprintf.c

BOOT_OBJS    := $(patsubst boot/%.asm,    $(BUILD_DIR)/boot/%.o,    $(BOOT_SRCS))
KERNEL_OBJS  := $(patsubst kernel/%.c,    $(BUILD_DIR)/kernel/%.o,  $(KERNEL_SRCS))
KERNEL_AOBJS := $(patsubst kernel/%.asm,  $(BUILD_DIR)/kernel/%.o,  $(KERNEL_ASMS))
LIB_OBJS     := $(patsubst lib/%.c,       $(BUILD_DIR)/lib/%.o,     $(LIB_SRCS))

ALL_OBJS := $(BOOT_OBJS) $(KERNEL_OBJS) $(KERNEL_AOBJS) $(LIB_OBJS)

MBR_BIN  := $(BUILD_DIR)/boot.bin
KERN_BIN := $(BUILD_DIR)/kernel.bin
OS_IMG   := $(BUILD_DIR)/baSic_.img

.PHONY: all
all: $(OS_IMG)

$(OS_IMG): $(MBR_BIN) $(KERN_BIN)
	@echo "[IMG] $@"
	@cat $(MBR_BIN) $(KERN_BIN) > $@
	@python3 -c "\
import os; \
target = 65 * 512; \
size = os.path.getsize('$@'); \
open('$@', 'ab').write(b'\\x00' * max(0, target - size))"
	@echo "[OK]  $@ ready"

$(MBR_BIN): boot/boot.asm | $(BUILD_DIR)/boot
	@echo "[AS]  $<"
	@$(AS) -f bin $< -o $@

$(KERN_BIN): $(ALL_OBJS) linker.ld
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -o $@ \
		$(BUILD_DIR)/boot/stage2.o \
		$(filter-out $(BUILD_DIR)/boot/stage2.o, $(ALL_OBJS))

$(BUILD_DIR)/boot/%.o: boot/%.asm | $(BUILD_DIR)/boot
	@echo "[AS]  $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel/%.o: kernel/%.c | $(BUILD_DIR)/kernel
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/%.o: kernel/%.asm | $(BUILD_DIR)/kernel
	@echo "[AS]  $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/lib/%.o: lib/%.c | $(BUILD_DIR)/lib
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot:
	@mkdir -p $@

$(BUILD_DIR)/kernel:
	@mkdir -p $@

$(BUILD_DIR)/lib:
	@mkdir -p $@

.PHONY: run
run: $(OS_IMG)
	@./scripts/run.sh $(OS_IMG)

.PHONY: run-debug
run-debug: $(OS_IMG)
	@./scripts/run.sh $(OS_IMG) debug

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "[OK]  cleaned"