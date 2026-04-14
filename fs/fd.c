/* baSic_ - fs/fd.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * file descriptor table
 * kernel-side fd vfs_node mapping with per-fd seek offset
 */

#include "fd.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static fd_entry_t fd_table[FD_MAX];

void fd_init(void)
{
    memset(fd_table, 0, sizeof(fd_table));
    kprintf("[OK] fd: descriptor table ready (%d slots)\n", FD_MAX);
}

int fd_open(vfs_node_t *node)
{
    for (int i = 0; i < FD_MAX; i++) {
        if (!fd_table[i].open) {
            fd_table[i].node   = node;
            fd_table[i].offset = 0;
            fd_table[i].open   = 1;
            return i;
        }
    }
    kprintf("[ERR] fd: no free descriptors\n");
    return -1;
}

void fd_close(int fd)
{
    if (fd < 0 || fd >= FD_MAX) return;
    fd_table[fd].open   = 0;
    fd_table[fd].node   = NULL;
    fd_table[fd].offset = 0;
}

int fd_read(int fd, u8 *buf, u32 size)
{
    if (fd < 0 || fd >= FD_MAX || !fd_table[fd].open) return -1;
    fd_entry_t *e = &fd_table[fd];
    u32 n = vfs_read(e->node, e->offset, size, buf);
    e->offset += n;
    return (int)n;
}

int fd_write(int fd, u8 *buf, u32 size)
{
    if (fd < 0 || fd >= FD_MAX || !fd_table[fd].open) return -1;
    fd_entry_t *e = &fd_table[fd];
    u32 n = vfs_write(e->node, e->offset, size, buf);
    e->offset += n;
    return (int)n;
}