/* baSic_ - fs/fd.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * fd table: maps integer fds to vfs nodes
 */

#ifndef FD_H
#define FD_H

#include "vfs.h"
#include "../include/types.h"

#define FD_MAX  16

typedef struct {
    vfs_node_t *node;
    u32         offset;
    u8          open;
} fd_entry_t;

void fd_init(void);
int  fd_open(vfs_node_t *node);
void fd_close(int fd);
int  fd_read(int fd, u8 *buf, u32 size);
int  fd_write(int fd, u8 *buf, u32 size);

#endif