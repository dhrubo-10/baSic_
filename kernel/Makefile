CC = gcc
CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra -nostdlib -fno-stack-protector \
         -fno-builtin -fno-PIC -I../include

OBJS = main.o printk.o

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

.PHONY: all clean