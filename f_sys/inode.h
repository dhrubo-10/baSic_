#ifndef INODE_H
#define INODE_H

#include <kernel_b/types.h>

#define INODE_FILE 1
#define INODE_DIR  2

typedef struct inode {
    u32  id;
    u32  type;
    u32  size;

    struct inode *parent;

    struct inode *children[16];
    int child_count;

    char name[32];
    char data[512];
} inode_t;

inode_t *inode_create(inode_t *parent, const char *name, u32 type);
inode_t *inode_find(inode_t *dir, const char *name);

#endif
