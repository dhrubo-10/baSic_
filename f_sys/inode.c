#include "inode.h"
#include <kernelb/string.h>
#include <kernelb/stdlib.h>
#include <kernelb/stdio.h>
#include <kernelb/errno.h>
#include <kernelb/time.h>

struct inode_cache inode_cache_global;

static void inode_init(struct inode *inode, struct superblock *sb, uint32_t mode)
{
    memset(inode, 0, sizeof(struct inode));
    
    inode->i_sb = sb;
    inode->i_mode = mode;
    inode->i_nlink = 1;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_size = 0;
    inode->i_blocks = 0;
    inode->i_flags = 0;
    
    time_t now = time(NULL);
    inode->i_atime.tv_sec = now;
    inode->i_mtime.tv_sec = now;
    inode->i_ctime.tv_sec = now;
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_nsec = 0;
    inode->i_ctime.tv_nsec = 0;
    
    atomic_set(&inode->i_count, 1);
    spin_lock_init(&inode->i_lock);
    
    INIT_LIST_HEAD(&inode->i_list);
    INIT_LIST_HEAD(&inode->i_hash);
    INIT_LIST_HEAD(&inode->i_dirty);
    
    if (S_ISDIR(mode)) {
        INIT_LIST_HEAD(&inode->u.dir.i_dentry);
    }
}

void inode_cache_init(void)
{
    for (int i = 0; i < 256; i++) {
        INIT_LIST_HEAD(&inode_cache_global.hash_table[i]);
    }
    spin_lock_init(&inode_cache_global.lock);
    atomic_set(&inode_cache_global.count, 0);
    inode_cache_global.next_ino = 1;
}

struct inode* inode_alloc(struct superblock *sb)
{
    struct inode *inode = malloc(sizeof(struct inode));
    if (!inode) return NULL;
    
    inode_init(inode, sb, 0);
    
    spin_lock(&inode_cache_global.lock);
    inode->i_ino = inode_cache_global.next_ino++;
    list_add(&inode->i_list, &inode_cache_global.hash_table[inode_hash(inode->i_ino)]);
    atomic_inc(&inode_cache_global.count);
    spin_unlock(&inode_cache_global.lock);
    
    return inode;
}

void inode_free(struct inode *inode)
{
    if (!inode) return;
    
    spin_lock(&inode->i_lock);
    
    if (inode->i_data) {
        free(inode->i_data);
        inode->i_data = NULL;
    }
    
    if (S_ISLNK(inode->i_mode) && inode->u.symlink.target) {
        free(inode->u.symlink.target);
        inode->u.symlink.target = NULL;
    }
    
    spin_lock(&inode_cache_global.lock);
    list_del(&inode->i_list);
    list_del(&inode->i_hash);
    if (inode->i_flags & INODE_FLAG_DIRTY) {
        list_del(&inode->i_dirty);
    }
    atomic_dec(&inode_cache_global.count);
    spin_unlock(&inode_cache_global.lock);
    
    spin_unlock(&inode->i_lock);
    
    if (inode->i_private) {
        free(inode->i_private);
    }
    
    free(inode);
}

struct inode* inode_get(uint32_t ino)
{
    struct inode *inode = NULL;
    struct list_head *head = &inode_cache_global.hash_table[inode_hash(ino)];
    
    spin_lock(&inode_cache_global.lock);
    
    list_for_each_entry(inode, head, i_list) {
        if (inode->i_ino == ino) {
            atomic_inc(&inode->i_count);
            spin_unlock(&inode_cache_global.lock);
            return inode;
        }
    }
    
    spin_unlock(&inode_cache_global.lock);
    return NULL;
}

void inode_put(struct inode *inode)
{
    if (!inode) return;
    
    if (atomic_dec_and_test(&inode->i_count)) {
        inode_free(inode);
    }
}

struct inode* inode_create(struct superblock *sb, uint32_t mode)
{
    struct inode *inode = inode_alloc(sb);
    if (!inode) return NULL;
    
    inode->i_mode = mode;
    
    if (S_ISREG(mode)) {
        inode->i_data_capacity = 4096;
        inode->i_data = malloc(inode->i_data_capacity);
        if (!inode->i_data) {
            inode_free(inode);
            return NULL;
        }
        memset(inode->i_data, 0, inode->i_data_capacity);
    } else if (S_ISDIR(mode)) {
        inode->i_mode |= 0755;
    }
    
    return inode;
}

int inode_link(struct inode *inode, struct inode *dir, const char *name)
{
    if (!inode || !dir || !name) return -EINVAL;
    if (!S_ISDIR(dir->i_mode)) return -ENOTDIR;
    
    spin_lock(&dir->i_lock);
    
    if (dir->u.dir.i_dentry.next == &dir->u.dir.i_dentry) {
        INIT_LIST_HEAD(&dir->u.dir.i_dentry);
    }
    
    struct dentry *dentry = malloc(sizeof(struct dentry));
    if (!dentry) {
        spin_unlock(&dir->i_lock);
        return -ENOMEM;
    }
    
    memset(dentry, 0, sizeof(struct dentry));
    strncpy(dentry->name, name, 255);
    dentry->inode = inode;
    
    list_add(&dentry->list, &dir->u.dir.i_dentry);
    
    atomic_inc(&inode->i_count);
    inode->i_nlink++;
    
    dir->i_mtime.tv_sec = time(NULL);
    dir->i_ctime.tv_sec = dir->i_mtime.tv_sec;
    
    spin_unlock(&dir->i_lock);
    
    return 0;
}

int inode_unlink(struct inode *dir, const char *name)
{
    if (!dir || !name) return -EINVAL;
    if (!S_ISDIR(dir->i_mode)) return -ENOTDIR;
    
    spin_lock(&dir->i_lock);
    
    struct dentry *dentry, *tmp;
    list_for_each_entry_safe(dentry, tmp, &dir->u.dir.i_dentry, list) {
        if (strcmp(dentry->name, name) == 0) {
            struct inode *inode = dentry->inode;
            
            list_del(&dentry->list);
            free(dentry);
            
            if (inode) {
                inode->i_nlink--;
                inode_put(inode);
            }
            
            dir->i_mtime.tv_sec = time(NULL);
            dir->i_ctime.tv_sec = dir->i_mtime.tv_sec;
            
            spin_unlock(&dir->i_lock);
            return 0;
        }
    }
    
    spin_unlock(&dir->i_lock);
    return -ENOENT;
}

int inode_read(struct inode *inode, void *buf, uint32_t size, uint32_t offset)
{
    if (!inode || !buf) return -EINVAL;
    if (!S_ISREG(inode->i_mode)) return -EINVAL;
    
    spin_lock(&inode->i_lock);
    
    if (offset >= inode->i_size) {
        spin_unlock(&inode->i_lock);
        return 0;
    }
    
    if (offset + size > inode->i_size) {
        size = inode->i_size - offset;
    }
    
    if (inode->i_data && size > 0) {
        memcpy(buf, (char*)inode->i_data + offset, size);
    }
    
    inode->i_atime.tv_sec = time(NULL);
    
    spin_unlock(&inode->i_lock);
    
    return size;
}

int inode_write(struct inode *inode, const void *buf, uint32_t size, uint32_t offset)
{
    if (!inode || !buf) return -EINVAL;
    if (!S_ISREG(inode->i_mode)) return -EINVAL;
    
    spin_lock(&inode->i_lock);
    
    uint32_t new_size = offset + size;
    
    if (new_size > inode->i_data_capacity) {
        uint32_t new_capacity = ((new_size + 4095) / 4096) * 4096;
        void *new_data = realloc(inode->i_data, new_capacity);
        if (!new_data) {
            spin_unlock(&inode->i_lock);
            return -ENOMEM;
        }
        
        if (new_capacity > inode->i_data_capacity) {
            memset((char*)new_data + inode->i_data_capacity, 0,
                   new_capacity - inode->i_data_capacity);
        }
        
        inode->i_data = new_data;
        inode->i_data_capacity = new_capacity;
    }
    
    if (size > 0) {
        memcpy((char*)inode->i_data + offset, buf, size);
    }
    
    if (new_size > inode->i_size) {
        inode->i_size = new_size;
    }
    
    inode->i_mtime.tv_sec = time(NULL);
    inode->i_ctime.tv_sec = inode->i_mtime.tv_sec;
    inode->i_flags |= INODE_FLAG_DIRTY;
    
    spin_unlock(&inode->i_lock);
    
    return size;
}

int inode_truncate(struct inode *inode, uint32_t size)
{
    if (!inode) return -EINVAL;
    if (!S_ISREG(inode->i_mode)) return -EINVAL;
    
    spin_lock(&inode->i_lock);
    
    if (size < inode->i_size && inode->i_data) {
        memset((char*)inode->i_data + size, 0, inode->i_size - size);
    }
    
    inode->i_size = size;
    inode->i_mtime.tv_sec = time(NULL);
    inode->i_ctime.tv_sec = inode->i_mtime.tv_sec;
    inode->i_flags |= INODE_FLAG_DIRTY;
    
    spin_unlock(&inode->i_lock);
    
    return 0;
}

uint32_t inode_hash(uint32_t ino)
{
    return (ino * 2654435761U) % 256;
}

struct inode* inode_lookup(struct inode *dir, const char *name)
{
    if (!dir || !name) return NULL;
    if (!S_ISDIR(dir->i_mode)) return NULL;
    
    spin_lock(&dir->i_lock);
    
    struct dentry *dentry;
    list_for_each_entry(dentry, &dir->u.dir.i_dentry, list) {
        if (strcmp(dentry->name, name) == 0) {
            struct inode *inode = dentry->inode;
            if (inode) {
                atomic_inc(&inode->i_count);
            }
            spin_unlock(&dir->i_lock);
            return inode;
        }
    }
    
    spin_unlock(&dir->i_lock);
    return NULL;
}

int inode_mkdir(struct inode *dir, const char *name, uint32_t mode)
{
    if (!dir || !name) return -EINVAL;
    if (!S_ISDIR(dir->i_mode)) return -ENOTDIR;
    
    struct inode *child = inode_create(dir->i_sb, S_IFDIR | (mode & 0777));
    if (!child) return -ENOMEM;
    
    int ret = inode_link(child, dir, name);
    if (ret < 0) {
        inode_free(child);
        return ret;
    }
    
    return 0;
}

int inode_rmdir(struct inode *dir, const char *name)
{
    struct inode *child = inode_lookup(dir, name);
    if (!child) return -ENOENT;
    
    if (!S_ISDIR(child->i_mode)) {
        inode_put(child);
        return -ENOTDIR;
    }
    
    spin_lock(&child->i_lock);
    
    int empty = list_empty(&child->u.dir.i_dentry);
    
    spin_unlock(&child->i_lock);
    
    if (!empty) {
        inode_put(child);
        return -ENOTEMPTY;
    }
    
    int ret = inode_unlink(dir, name);
    inode_put(child);
    
    return ret;
}

void inode_stat(struct inode *inode, struct stat *statbuf)
{
    if (!inode || !statbuf) return;
    
    memset(statbuf, 0, sizeof(struct stat));
    
    statbuf->st_ino = inode->i_ino;
    statbuf->st_mode = inode->i_mode;
    statbuf->st_nlink = inode->i_nlink;
    statbuf->st_uid = inode->i_uid;
    statbuf->st_gid = inode->i_gid;
    statbuf->st_size = inode->i_size;
    statbuf->st_blocks = inode->i_blocks;
    statbuf->st_atime = inode->i_atime.tv_sec;
    statbuf->st_mtime = inode->i_mtime.tv_sec;
    statbuf->st_ctime = inode->i_ctime.tv_sec;
}