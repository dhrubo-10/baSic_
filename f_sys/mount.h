#ifndef KERNELB_MOUNT_H
#define KERNELB_MOUNT_H

#include "inode.h"
#include <kernelb/list.h>
#include <kernelb/spinlock.h>

struct superblock;
struct filesystem_type;

struct vfsmount {
    struct superblock *mnt_sb;
    struct inode *mnt_root;
    struct vfsmount *mnt_parent;
    struct list_head mnt_mounts;
    struct list_head mnt_child;
    struct list_head mnt_list;
    atomic_t mnt_count;
    uint32_t mnt_flags;
    char mnt_devname[256];
};

struct superblock_ops {
    int (*alloc_inode)(struct superblock *);
    void (*destroy_inode)(struct inode *);
    int (*write_inode)(struct inode *);
    int (*sync_fs)(struct superblock *, int);
    int (*statfs)(struct superblock *, struct statfs *);
    int (*remount_fs)(struct superblock *, int *, char *);
    void (*clear_inode)(struct inode *);
    void (*umount_begin)(struct superblock *);
};

struct superblock {
    struct list_head s_list;
    struct list_head s_inodes;
    struct list_head s_dirty;
    struct superblock_ops *s_op;
    struct filesystem_type *s_type;
    struct vfsmount *s_mount;
    uint32_t s_flags;
    uint32_t s_magic;
    uint32_t s_blocksize;
    uint64_t s_maxbytes;
    atomic_t s_active;
    spinlock_t s_lock;
    void *s_fs_info;
};

struct filesystem_type {
    char name[32];
    int fs_flags;
    struct superblock *(*mount)(struct filesystem_type *, int, const char *, void *);
    void (*kill_sb)(struct superblock *);
    struct module *owner;
    struct list_head fs_supers;
    struct list_head list;
};

extern struct vfsmount *root_mount;
extern struct list_head super_blocks;
extern struct list_head file_systems;

void vfs_init(void);
struct vfsmount *vfs_mount(struct filesystem_type *, const char *, uint32_t, const char *, void *);
int vfs_umount(struct vfsmount *, int);
struct vfsmount *vfs_lookup_mount(struct inode *);
struct vfsmount *vfs_path_lookup(const char *, struct inode **);
int vfs_mount_root(void);
void vfs_add_filesystem(struct filesystem_type *);
void vfs_remove_filesystem(struct filesystem_type *);
struct filesystem_type *vfs_find_filesystem(const char *);
struct superblock *alloc_super(struct filesystem_type *);
void destroy_super(struct superblock *);
int vfs_statfs(struct vfsmount *, struct statfs *);
int vfs_remount(struct vfsmount *, int, char *);
void vfs_umount_all(void);
int vfs_mounted(struct vfsmount *);
struct vfsmount *vfs_clone_mount(struct vfsmount *);
void vfs_put_mount(struct vfsmount *);
struct vfsmount *vfs_get_mount(struct vfsmount *);

#endif