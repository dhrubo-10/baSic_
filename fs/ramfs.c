/* baSic_ - fs/ramfs.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ramfs: in-memory filesystem backed by kmalloc
 * each file has a fixed 4KB data buffer
 * each directory holds pointers to child nodes
 */

#include "ramfs.h"
#include "vfs.h"
#include "../mm/heap.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

#define MAX_CHILDREN    32

typedef struct ramfs_data {
    u8  *buf;               /* file data buffer         */
    u32  capacity;          /* allocated buffer size    */
    vfs_node_t *children[MAX_CHILDREN];
    u32  child_count;
} ramfs_data_t;

static u32 next_inode = 1;

static u32 ramfs_read(vfs_node_t *node, u32 offset, u32 size, u8 *buf)
{
    ramfs_data_t *d = (ramfs_data_t *)node->inode;
    if (!d || !d->buf) return 0;
    if (offset >= node->length) return 0;
    if (offset + size > node->length)
        size = node->length - offset;
    memcpy(buf, d->buf + offset, size);
    return size;
}

static u32 ramfs_write(vfs_node_t *node, u32 offset, u32 size, u8 *buf)
{
    ramfs_data_t *d = (ramfs_data_t *)node->inode;
    if (!d || !d->buf) return 0;
    if (offset + size > d->capacity)
        size = d->capacity - offset;
    memcpy(d->buf + offset, buf, size);
    if (offset + size > node->length)
        node->length = offset + size;
    return size;
}

static vfs_node_t *ramfs_finddir(vfs_node_t *dir, const char *name)
{
    ramfs_data_t *d = (ramfs_data_t *)dir->inode;
    if (!d) return NULL;
    for (u32 i = 0; i < d->child_count; i++) {
        if (!strcmp(d->children[i]->name, name))
            return d->children[i];
    }
    return NULL;
}

static vfs_node_t *make_node(const char *name, u32 flags)
{
    vfs_node_t   *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    ramfs_data_t *data = (ramfs_data_t *)kmalloc(sizeof(ramfs_data_t));
    if (!node || !data) return NULL;

    memset(node, 0, sizeof(vfs_node_t));
    memset(data, 0, sizeof(ramfs_data_t));

    strncpy(node->name, name, VFS_NAME_MAX - 1);
    node->flags   = flags;
    node->length  = 0;
    node->inode   = (u32)data;   /* abuse inode field as data pointer */
    node->read    = ramfs_read;
    node->write   = ramfs_write;
    node->finddir = ramfs_finddir;

    if (flags & VFS_FILE) {
        data->buf      = (u8 *)kmalloc(RAMFS_MAX_FILESIZE);
        data->capacity = data->buf ? RAMFS_MAX_FILESIZE : 0;
        if (data->buf)
            memset(data->buf, 0, RAMFS_MAX_FILESIZE);
    }

    (void)next_inode++;
    return node;
}

static void add_child(vfs_node_t *parent, vfs_node_t *child)
{
    ramfs_data_t *d = (ramfs_data_t *)parent->inode;
    if (!d || d->child_count >= MAX_CHILDREN) return;
    child->parent = parent;
    d->children[d->child_count++] = child;
}

vfs_node_t *ramfs_mkdir(vfs_node_t *parent, const char *name)
{
    vfs_node_t *node = make_node(name, VFS_DIR);
    if (node && parent)
        add_child(parent, node);
    return node;
}

vfs_node_t *ramfs_mkfile(vfs_node_t *parent, const char *name)
{
    vfs_node_t *node = make_node(name, VFS_FILE);
    if (node && parent)
        add_child(parent, node);
    return node;
}

vfs_node_t *ramfs_init(void)
{
    vfs_node_t *root = make_node("/", VFS_DIR);
    if (!root) return NULL;

    /* create a minimal default tree */
    vfs_node_t *etc = ramfs_mkdir(root, "etc");
    vfs_node_t *dev = ramfs_mkdir(root, "dev");
    (void)dev;

    /* /etc/motd */
    vfs_node_t *motd = ramfs_mkfile(etc, "motd");
    if (motd) {
        const char *msg = "welcome to baSic_ v1.0\nby Shahriar Dhrubo\n";
        vfs_write(motd, 0, strlen(msg), (u8 *)msg);
    }

    kprintf("[OK] ramfs: mounted at /\n");
    return root;
}