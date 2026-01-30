#ifndef KERNELB_INODE_H
#define KERNELB_INODE_H

#include <kernelb/types.h>
#include <kernelb/list.h>
#include <asm/atomic.h>
#include <asm/spinlock.h>
#include <asm/page.h>

#define INODE_TYPE_FILE     0x01
#define INODE_TYPE_DIR      0x02
#define INODE_TYPE_SYMLINK  0x04
#define INODE_TYPE_DEVICE   0x08
#define INODE_TYPE_PIPE     0x10
#define INODE_TYPE_SOCKET   0x20

#define INODE_FLAG_DIRTY    0x0001
#define INODE_FLAG_LOCKED   0x0002
#define INODE_FLAG_WRITE    0x0004
#define INODE_FLAG_READ     0x0008
#define INODE_FLAG_EXEC     0x0010
#define INODE_FLAG_DELETED  0x0020

struct superblock;

struct inode_ops {
    int (*create)(struct inode*, const char*, uint32_t);
    int (*lookup)(struct inode*, const char*, struct inode**);
    int (*link)(struct inode*, struct inode*, const char*);
    int (*unlink)(struct inode*, const char*);
    int (*mkdir)(struct inode*, const char*, uint32_t);
    int (*rmdir)(struct inode*, const char*);
    int (*rename)(struct inode*, const char*, struct inode*, const char*);
    int (*read)(struct inode*, void*, uint32_t, uint32_t);
    int (*write)(struct inode*, const void*, uint32_t, uint32_t);
    int (*truncate)(struct inode*, uint32_t);
    int (*permission)(struct inode*, uint32_t);
    int (*sync)(struct inode*);
};

struct inode {
    uint32_t i_ino;
    uint32_t i_mode;
    uint32_t i_nlink;
    uint32_t i_uid;
    uint32_t i_gid;
    uint64_t i_size;
    uint32_t i_blocks;
    uint32_t i_flags;
    
    struct timespec i_atime;
    struct timespec i_mtime;
    struct timespec i_ctime;
    
    struct superblock *i_sb;
    void *i_private;
    
    const struct inode_ops *i_op;
    
    void *i_data;
    uint32_t i_data_size;
    uint32_t i_data_capacity;
    
    atomic_t i_count;
    spinlock_t i_lock;
    
    struct list_head i_list;
    struct list_head i_hash;
    struct list_head i_dirty;
    
    union {
        struct {
            struct list_head i_dentry;
        } dir;
        struct {
            uint32_t device;
        } dev;
        struct {
            char *target;
        } symlink;
    } u;
};

struct inode_cache {
    struct list_head hash_table[256];
    spinlock_t lock;
    atomic_t count;
    uint32_t next_ino;
};

extern struct inode_cache inode_cache_global;

void inode_cache_init(void);
struct inode* inode_alloc(struct superblock *sb);
void inode_free(struct inode *inode);
struct inode* inode_get(uint32_t ino);
void inode_put(struct inode *inode);
struct inode* inode_create(struct superblock *sb, uint32_t mode);
int inode_link(struct inode *inode, struct inode *dir, const char *name);
int inode_unlink(struct inode *dir, const char *name);
int inode_rename(struct inode *old_dir, const char *old_name,
                 struct inode *new_dir, const char *new_name);
int inode_read(struct inode *inode, void *buf, uint32_t size, uint32_t offset);
int inode_write(struct inode *inode, const void *buf, uint32_t size, uint32_t offset);
int inode_truncate(struct inode *inode, uint32_t size);
int inode_sync(struct inode *inode);
int inode_permission(struct inode *inode, uint32_t mask);
void inode_setattr(struct inode *inode, uint32_t mode, uint32_t uid, uint32_t gid);
void inode_mark_dirty(struct inode *inode);
void inode_clear_dirty(struct inode *inode);
uint32_t inode_hash(uint32_t ino);
struct inode* inode_lookup(struct inode *dir, const char *name);
int inode_mkdir(struct inode *dir, const char *name, uint32_t mode);
int inode_rmdir(struct inode *dir, const char *name);
int inode_symlink(struct inode *dir, const char *name, const char *target);
ssize_t inode_readlink(struct inode *inode, char *buf, size_t bufsize);
void inode_stat(struct inode *inode, struct stat *statbuf);

#endif