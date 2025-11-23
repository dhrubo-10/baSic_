#include <f_sys/mount.h>

static mount_t main_mount;

void vfs_mount(inode_t *root) {
    main_mount.root_inode = root;
}

inode_t *vfs_get_root() {
    return main_mount.root_inode;
}
