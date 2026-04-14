/* baSic_ - fs/vfs.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * virtual filesystem switch
 * thin dispatch layer — calls through function pointers on each node
 */

#include "vfs.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static vfs_node_t *fs_root = NULL;

void vfs_init(void)
{
    fs_root = NULL;
    kprintf("[OK] VFS: initialized\n");
}

vfs_node_t *vfs_root(void)
{
    return fs_root;
}

void vfs_set_root(vfs_node_t *node)
{
    fs_root = node;
}

u32 vfs_read(vfs_node_t *node, u32 offset, u32 size, u8 *buf)
{
    if (node && node->read)
        return node->read(node, offset, size, buf);
    return 0;
}

u32 vfs_write(vfs_node_t *node, u32 offset, u32 size, u8 *buf)
{
    if (node && node->write)
        return node->write(node, offset, size, buf);
    return 0;
}

vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name)
{
    if (dir && dir->finddir)
        return dir->finddir(dir, name);
    return NULL;
}

/* resolve an absolute path like "/etc/motd" to a vfs_node */
vfs_node_t *vfs_resolve(const char *path)
{
    if (!path || path[0] != '/')
        return NULL;

    vfs_node_t *cur = fs_root;
    if (!cur) return NULL;

    /* skip leading slash */
    path++;
    if (*path == '\0')
        return fs_root;

    char part[VFS_NAME_MAX];
    while (*path && cur) {
        int i = 0;
        while (*path && *path != '/' && i < VFS_NAME_MAX - 1)
            part[i++] = *path++;
        part[i] = '\0';
        if (*path == '/') path++;
        cur = vfs_finddir(cur, part);
    }
    return cur;
}