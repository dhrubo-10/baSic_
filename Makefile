ASM = nasm
CC  = gcc
LD  = ld
CFLAGS = -m16 -ffreestanding -fno-stack-protector -fno-pic -nostdlib -nostdinc -Wall
LDFLAGS = -Ttext 0x100

OBJS = bios.o syscall.o kernel.o add.o

all: kernel.bin

add.o: add.s
	$(ASM) -f elf add.s -o add.o

bios.o: bios.c bios.h
	$(CC) $(CFLAGS) -c bios.c -o bios.o

syscall.o: syscall.c syscall.h
	$(CC) $(CFLAGS) -c syscall.c -o syscall.o

kernel.o: kernel.c bios.h syscall.h
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

kernel.bin: $(OBJS)
	$(LD) -m elf_i386 $(LDFLAGS) -o kernel.bin $(OBJS)

clean:
	rm -f *.o *.bin


.PHONY: all clean run

