CC = gcc
CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra -nostdlib -fno-stack-protector \
	-fno-builtin -fno-PIC -I../include -I.

OBJS = cored.o dir.o file.o inode.o mount.o vfs.o

all: $(OBJS) libfs.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


cored.o: cored.c fs.h ../include/kernelb/types.h ../include/kernelb/string.h \
	../include/kernelb/stdlib.h ../include/kernelb/errno.h \
	../include/kernelb/time.h ../include/kernelb/unistd.h

dir.o: dir.c fs.h ../include/kernelb/types.h ../include/kernelb/string.h \
	../include/kernelb/stdlib.h ../include/kernelb/stdio.h \
	../include/kernelb/errno.h ../include/kernelb/time.h

file.o: file.c fs.h ../include/kernelb/types.h ../include/kernelb/string.h \
	../include/kernelb/stdlib.h ../include/kernelb/stdio.h \
	../include/kernelb/errno.h ../include/kernelb/time.h \
	../include/kernelb/unistd.h

inode.o: inode.c inode.h ../include/kernelb/types.h ../include/kernelb/string.h \
	../include/kernelb/stdlib.h ../include/kernelb/stdio.h \
	../include/kernelb/errno.h ../include/kernelb/time.h \
	../include/kernelb/list.h ../include/kernelb/spinlock.h \
	../include/kernelb/atomic.h

mount.o: mount.c mount.h inode.h ../include/kernelb/types.h \
	../include/kernelb/string.h ../include/kernelb/stdlib.h \
	../include/kernelb/stdio.h ../include/kernelb/errno.h \
	../include/kernelb/list.h ../include/kernelb/spinlock.h \
	../include/kernelb/atomic.h

vfs.o: vfs.c fs.h inode.h mount.h ../include/kernelb/types.h \
	../include/kernelb/string.h ../include/kernelb/stdlib.h \
	../include/kernelb/stdio.h ../include/kernelb/errno.h \
	../include/kernelb/time.h ../include/kernelb/unistd.h \
	../include/kernelb/list.h ../include/kernelb/spinlock.h \
	../include/kernelb/atomic.h ../include/kernelb/panic.h

libfs.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libfs.a

.PHONY: all clean
