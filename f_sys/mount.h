#ifndef MOUNT_H
#define MOUNT_H

#include <f_sys/inode.h>

typedef struct mount {
    inode_t *root_inode;
} mount_t;

void vfs_mount(inode_t *root);
inode_t *vfs_get_root();

#endif
