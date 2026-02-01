AS = nasm
ASFLAGS = -f bin

TARGETS = boot.bin

all: $(TARGETS)

boot.bin: boot.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean