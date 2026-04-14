/* baSic_ - fs/ramfs.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ramfs :simple in-memory filesystem
 * stores files as flat byte arrays in kernel heap
 */

#ifndef RAMFS_H
#define RAMFS_H

#include "vfs.h"
#include "../include/types.h"

#define RAMFS_MAX_NODES     64
#define RAMFS_MAX_FILESIZE  4096    /* 4 KB per file */

vfs_node_t *ramfs_init(void);
vfs_node_t *ramfs_mkdir(vfs_node_t *parent, const char *name);
vfs_node_t *ramfs_mkfile(vfs_node_t *parent, const char *name);

#endif