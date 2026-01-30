#include "mount.h"
#include <kernelb/string.h>
#include <kernelb/stdlib.h>
#include <kernelb/errno.h>
#include <kernelb/stdio.h>

struct vfsmount *root_mount = NULL;
struct list_head super_blocks = LIST_HEAD_INIT(super_blocks);
struct list_head file_systems = LIST_HEAD_INIT(file_systems);
static spinlock_t mount_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t fs_lock = SPIN_LOCK_UNLOCKED;

static struct vfsmount *alloc_vfsmount(void)
{
    struct vfsmount *mnt = malloc(sizeof(struct vfsmount));
    if (!mnt) return NULL;
    
    memset(mnt, 0, sizeof(struct vfsmount));
    atomic_set(&mnt->mnt_count, 1);
    INIT_LIST_HEAD(&mnt->mnt_mounts);
    INIT_LIST_HEAD(&mnt->mnt_child);
    INIT_LIST_HEAD(&mnt->mnt_list);
    
    return mnt;
}

static void free_vfsmount(struct vfsmount *mnt)
{
    if (!mnt) return;
    
    if (mnt->mnt_sb) {
        destroy_super(mnt->mnt_sb);
    }
    
    free(mnt);
}

void vfs_init(void)
{
    INIT_LIST_HEAD(&super_blocks);
    INIT_LIST_HEAD(&file_systems);
}

struct vfsmount *vfs_mount(struct filesystem_type *type, const char *dev_name,
                           uint32_t flags, const char *dir_name, void *data)
{
    struct vfsmount *mnt;
    struct superblock *sb;
    struct inode *root_inode;
    
    if (!type || !type->mount) return NULL;
    
    mnt = alloc_vfsmount();
    if (!mnt) return NULL;
    
    sb = type->mount(type, flags, dev_name, data);
    if (!sb) {
        free(mnt);
        return NULL;
    }
    
    sb->s_type = type;
    sb->s_mount = mnt;
    
    root_inode = inode_get(sb->s_root->i_ino);
    if (!root_inode) {
        destroy_super(sb);
        free(mnt);
        return NULL;
    }
    
    mnt->mnt_sb = sb;
    mnt->mnt_root = root_inode;
    mnt->mnt_parent = mnt;
    strncpy(mnt->mnt_devname, dev_name ? dev_name : "none", 255);
    mnt->mnt_flags = flags;
    
    spin_lock(&mount_lock);
    if (!root_mount && strcmp(dir_name, "/") == 0) {
        root_mount = mnt;
    }
    list_add(&mnt->mnt_list, &super_blocks);
    spin_unlock(&mount_lock);
    
    return mnt;
}

int vfs_umount(struct vfsmount *mnt, int flags)
{
    if (!mnt) return -EINVAL;
    
    if (mnt == root_mount) return -EBUSY;
    
    if (atomic_read(&mnt->mnt_count) > 1) return -EBUSY;
    
    spin_lock(&mount_lock);
    
    if (!list_empty(&mnt->mnt_child)) {
        spin_unlock(&mount_lock);
        return -EBUSY;
    }
    
    list_del(&mnt->mnt_list);
    
    if (mnt->mnt_parent) {
        list_del(&mnt->mnt_child);
    }
    
    spin_unlock(&mount_lock);
    
    if (mnt->mnt_root) {
        inode_put(mnt->mnt_root);
    }
    
    if (mnt->mnt_sb && mnt->mnt_sb->s_op->umount_begin) {
        mnt->mnt_sb->s_op->umount_begin(mnt->mnt_sb);
    }
    
    free_vfsmount(mnt);
    
    return 0;
}

struct vfsmount *vfs_lookup_mount(struct inode *inode)
{
    struct vfsmount *mnt;
    
    spin_lock(&mount_lock);
    
    list_for_each_entry(mnt, &super_blocks, mnt_list) {
        if (mnt->mnt_root == inode) {
            atomic_inc(&mnt->mnt_count);
            spin_unlock(&mount_lock);
            return mnt;
        }
    }
    
    spin_unlock(&mount_lock);
    return NULL;
}

struct superblock *alloc_super(struct filesystem_type *type)
{
    struct superblock *sb = malloc(sizeof(struct superblock));
    if (!sb) return NULL;
    
    memset(sb, 0, sizeof(struct superblock));
    sb->s_type = type;
    atomic_set(&sb->s_active, 1);
    spin_lock_init(&sb->s_lock);
    INIT_LIST_HEAD(&sb->s_list);
    INIT_LIST_HEAD(&sb->s_inodes);
    INIT_LIST_HEAD(&sb->s_dirty);
    
    spin_lock(&mount_lock);
    list_add(&sb->s_list, &super_blocks);
    spin_unlock(&mount_lock);
    
    return sb;
}

void destroy_super(struct superblock *sb)
{
    if (!sb) return;
    
    spin_lock(&mount_lock);
    list_del(&sb->s_list);
    spin_unlock(&mount_lock);
    
    if (sb->s_fs_info) {
        free(sb->s_fs_info);
    }
    
    free(sb);
}

void vfs_add_filesystem(struct filesystem_type *type)
{
    spin_lock(&fs_lock);
    list_add(&type->list, &file_systems);
    spin_unlock(&fs_lock);
}

void vfs_remove_filesystem(struct filesystem_type *type)
{
    spin_lock(&fs_lock);
    list_del(&type->list);
    spin_unlock(&fs_lock);
}

struct filesystem_type *vfs_find_filesystem(const char *name)
{
    struct filesystem_type *type;
    
    spin_lock(&fs_lock);
    
    list_for_each_entry(type, &file_systems, list) {
        if (strcmp(type->name, name) == 0) {
            spin_unlock(&fs_lock);
            return type;
        }
    }
    
    spin_unlock(&fs_lock);
    return NULL;
}

int vfs_mount_root(void)
{
    struct filesystem_type *type = vfs_find_filesystem("rootfs");
    if (!type) return -ENODEV;
    
    root_mount = vfs_mount(type, NULL, 0, "/", NULL);
    if (!root_mount) return -ENOMEM;
    
    return 0;
}

struct vfsmount *vfs_get_mount(struct vfsmount *mnt)
{
    if (mnt) atomic_inc(&mnt->mnt_count);
    return mnt;
}

void vfs_put_mount(struct vfsmount *mnt)
{
    if (!mnt) return;
    
    if (atomic_dec_and_test(&mnt->mnt_count)) {
        free_vfsmount(mnt);
    }
}

void vfs_umount_all(void)
{
    struct vfsmount *mnt, *tmp;
    
    spin_lock(&mount_lock);
    
    list_for_each_entry_safe(mnt, tmp, &super_blocks, mnt_list) {
        if (mnt != root_mount) {
            spin_unlock(&mount_lock);
            vfs_umount(mnt, 0);
            spin_lock(&mount_lock);
        }
    }
    
    spin_unlock(&mount_lock);
}

int vfs_mounted(struct vfsmount *mnt)
{
    return mnt && mnt->mnt_root != NULL;
}