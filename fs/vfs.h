/* baSic_ - fs/vfs.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * virtual filesystem switch: abstract interface all filesystems implement
 */

#ifndef VFS_H
#define VFS_H

#include "../include/types.h"

#define VFS_NAME_MAX    64
#define VFS_PATH_MAX    256
#define VFS_MAX_NODES   128

#define VFS_FILE        0x01
#define VFS_DIR         0x02

struct vfs_node;

typedef u32 (*vfs_read_fn) (struct vfs_node *node, u32 offset, u32 size, u8 *buf);
typedef u32 (*vfs_write_fn)(struct vfs_node *node, u32 offset, u32 size, u8 *buf);
typedef struct vfs_node *(*vfs_finddir_fn)(struct vfs_node *dir, const char *name);

typedef struct vfs_node {
    char         name[VFS_NAME_MAX];
    u32          flags;         /* VFS_FILE or VFS_DIR */
    u32          length;        /* file size in bytes  */
    u32          inode;         /* unique node id      */
    vfs_read_fn    read;
    vfs_write_fn   write;
    vfs_finddir_fn finddir;
    struct vfs_node *parent;
} vfs_node_t;

/* directory entry returned by readdir */
typedef struct {
    char name[VFS_NAME_MAX];
    u32  inode;
    u32  flags;
} vfs_dirent_t;

void        vfs_init(void);
void        vfs_set_root(vfs_node_t *node);
vfs_node_t *vfs_root(void);
vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name);
vfs_node_t *vfs_resolve(const char *path);
u32         vfs_read(vfs_node_t *node, u32 offset, u32 size, u8 *buf);
u32         vfs_write(vfs_node_t *node, u32 offset, u32 size, u8 *buf);

#endif