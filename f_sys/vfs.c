#include <kernel_b/types.h>
#include <kernel_b/string.h>
#include <kernel_b/stdlib.h>
#include <asm/atomic.h>
#include <asm/spinlock.h>
#include <asm/page.h>
#include <kernel_b/printk.h>
#include <kernel_b/panic.h>
#include <kernel_b/list.h>
#include <kernel_b/slab.h>
#include "fs.h"


struct filesystem *fs_rootfs = NULL;
spinlock_t fs_lock = SPIN_LOCK_UNLOCKED;
struct list_head filesystems = LIST_HEAD_INIT(filesystems);
static struct kmem_cache *inode_cache = NULL;
static struct kmem_cache *dentry_cache = NULL;

#define BUFFER_CACHE_SIZE 256
static struct buffer_head buffer_cache[BUFFER_CACHE_SIZE];
static LIST_HEAD(buffer_cache_lru);

static struct buffer_head* find_buffer(struct block_device *dev, uint32_t block) {
    struct buffer_head *bh;
    
    list_for_each_entry(bh, &buffer_cache_lru, lru) {
        if (bh->dev == dev && bh->block == block) {
            list_move(&bh->lru, &buffer_cache_lru);
            return bh;
        }
    }
    return NULL;
}

static struct buffer_head* alloc_buffer(void) {
    struct buffer_head *bh;
    
    if (list_empty(&buffer_cache_lru)) {
        bh = list_last_entry(&buffer_cache_lru, struct buffer_head, lru);
        if (bh->dirty) {
            bh->dev->write(bh->dev, bh->data, bh->block, 1);
        }
    } else {
        bh = kmalloc(sizeof(struct buffer_head), GFP_KERNEL);
        if (!bh) return NULL;
        bh->data = kmalloc(FS_BLOCK_SIZE, GFP_KERNEL);
        if (!bh->data) {
            kfree(bh);
            return NULL;
        }
    }
    
    bh->ref_count = 1;
    list_move(&bh->lru, &buffer_cache_lru);
    return bh;
}

struct buffer_head* fs_bread(struct block_device *dev, uint32_t block) {
    struct buffer_head *bh;
    unsigned long flags;
    
    spin_lock_irqsave(&fs_lock, flags);
    bh = find_buffer(dev, block);
    if (!bh) {
        bh = alloc_buffer();
        if (!bh) {
            spin_unlock_irqrestore(&fs_lock, flags);
            return NULL;
        }
        bh->dev = dev;
        bh->block = block;
        dev->read(dev, bh->data, block, 1);
    }
    atomic_inc(&bh->ref_count);
    spin_unlock_irqrestore(&fs_lock, flags);
    
    return bh;
}

int fs_bwrite(struct buffer_head *bh) {
    if (!bh || !bh->dev) return -EINVAL;
    
    bh->dirty = 1;
    return 0;
}

void fs_brelse(struct buffer_head *bh) {
    if (!bh) return;
    
    if (atomic_dec_and_test(&bh->ref_count)) {
        if (bh->dirty) {
            bh->dev->write(bh->dev, bh->data, bh->block, 1);
            bh->dirty = 0;
        }
    }
}


static struct inode* alloc_inode(struct superblock *sb) {
    struct inode *inode;
    
    inode = kmem_cache_alloc(inode_cache, GFP_KERNEL);
    if (!inode) return NULL;
    
    memset(inode, 0, sizeof(struct inode));
    atomic_set(&inode->i_count, 1);
    inode->i_sb = sb;
    inode->i_state = 0;
    inode->i_ino = 0;
    
    INIT_LIST_HEAD(&inode->i_list);
    INIT_LIST_HEAD(&inode->i_sb_list);
    INIT_LIST_HEAD(&inode->i_dentry);
    
    return inode;
}

static void destroy_inode(struct inode *inode) {
    if (!inode) return;
    
    if (inode->i_data) {
        kfree(inode->i_data);
    }
    
    kmem_cache_free(inode_cache, inode);
}

struct inode* fs_alloc_inode(struct superblock *sb) {
    return alloc_inode(sb);
}

void fs_destroy_inode(struct inode *inode) {
    destroy_inode(inode);
}

static struct dentry* alloc_dentry(void) {
    struct dentry *dentry;
    
    dentry = kmem_cache_alloc(dentry_cache, GFP_KERNEL);
    if (!dentry) return NULL;
    
    memset(dentry, 0, sizeof(struct dentry));
    atomic_set(&dentry->d_count, 1);
    INIT_LIST_HEAD(&dentry->d_child);
    INIT_LIST_HEAD(&dentry->d_subdirs);
    
    return dentry;
}

static void free_dentry(struct dentry *dentry) {
    if (!dentry) return;
    kmem_cache_free(dentry_cache, dentry);
}


struct filesystem* fs_alloc(void) {
    struct filesystem *fs;
    
    fs = kmalloc(sizeof(struct filesystem), GFP_KERNEL);
    if (!fs) return NULL;
    
    memset(fs, 0, sizeof(struct filesystem));
    spin_lock_init(&fs->lock);
    atomic_set(&fs->ref_count, 1);
    
    return fs;
}

void fs_free(struct filesystem *fs) {
    if (!fs) return;
    
    if (atomic_dec_and_test(&fs->ref_count)) {
        if (fs->cache) {
            struct buffer_head *bh, *tmp;
            list_for_each_entry_safe(bh, tmp, &fs->cache->lru, lru) {
                kfree(bh->data);
                kfree(bh);
            }
        }
        kfree(fs);
    }
}


struct superblock* fs_read_super(struct block_device *dev) {
    struct superblock *sb;
    struct buffer_head *bh;
    
    sb = kmalloc(sizeof(struct superblock), GFP_KERNEL);
    if (!sb) return NULL;
    
    bh = fs_bread(dev, 0);
    if (!bh) {
        kfree(sb);
        return NULL;
    }
    
    memcpy(sb, bh->data, sizeof(struct superblock));
    sb->s_dev = dev;
    sb->s_blocksize = FS_BLOCK_SIZE;
    sb->s_magic = sb->magic;
    
    fs_brelse(bh);
    return sb;
}

int fs_write_super(struct superblock *sb) {
    struct buffer_head *bh;
    
    if (!sb || !sb->s_dev) return -EINVAL;
    
    bh = fs_bread(sb->s_dev, 0);
    if (!bh) return -EIO;
    
    memcpy(bh->data, sb, sizeof(struct superblock));
    fs_bwrite(bh);
    fs_brelse(bh);
    
    return 0;
}


static struct dentry* path_lookup(struct dentry *parent, const char *name) {
    struct dentry *dentry;
    
    list_for_each_entry(dentry, &parent->d_subdirs, d_child) {
        if (strcmp(dentry->name, name) == 0) {
            atomic_inc(&dentry->d_count);
            return dentry;
        }
    }
    return NULL;
}

struct dentry* fs_lookup(struct inode *dir, struct dentry *target) {
    struct dentry *dentry;
    struct inode *inode = NULL;
    
    if (!dir || !S_ISDIR(dir->i_mode)) return ERR_PTR(-ENOTDIR);
    
    dentry = path_lookup((struct dentry *)dir->i_private, target->name);
    if (dentry) {
        inode = dentry->inode;
        if (inode) atomic_inc(&inode->i_count);
    }
    
    target->inode = inode;
    return dentry ? dentry : ERR_PTR(-ENOENT);
}


int fs_create(struct inode *dir, struct dentry *dentry, int mode) {
    struct inode *inode;
    struct dentry *new_dentry;
    
    if (!dir || !S_ISDIR(dir->i_mode)) return -ENOTDIR;
    
    if (fs_lookup(dir, dentry) != ERR_PTR(-ENOENT)) return -EEXIST;
    
    inode = fs_alloc_inode(dir->i_sb);
    if (!inode) return -ENOMEM;
    
    inode->i_mode = mode | S_IFREG;
    inode->i_uid = current->fsuid;
    inode->i_gid = current->fsgid;
    inode->i_size = 0;
    inode->i_blocks = 0;
    inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
    inode->i_private = NULL;
    
    new_dentry = alloc_dentry();
    if (!new_dentry) {
        fs_destroy_inode(inode);
        return -ENOMEM;
    }
    
    strncpy(new_dentry->name, dentry->name, FS_NAME_MAX - 1);
    new_dentry->inode = inode;
    new_dentry->parent = (struct dentry *)dir->i_private;
    list_add(&new_dentry->d_child, &new_dentry->parent->d_subdirs);
    
    atomic_inc(&dir->i_count);
    dir->i_mtime = dir->i_ctime = CURRENT_TIME;
    
    return 0;
}


int fs_mkdir(struct inode *dir, struct dentry *dentry, int mode) {
    struct inode *inode;
    struct dentry *new_dentry;
    
    if (!dir || !S_ISDIR(dir->i_mode)) return -ENOTDIR;
    
    if (fs_lookup(dir, dentry) != ERR_PTR(-ENOENT)) return -EEXIST;
    
    inode = fs_alloc_inode(dir->i_sb);
    if (!inode) return -ENOMEM;
    
    inode->i_mode = mode | S_IFDIR;
    inode->i_uid = current->fsuid;
    inode->i_gid = current->fsgid;
    inode->i_size = 0;
    inode->i_blocks = 0;
    inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
    
    inode->i_private = kmalloc(sizeof(struct dentry), GFP_KERNEL);
    if (!inode->i_private) {
        fs_destroy_inode(inode);
        return -ENOMEM;
    }
    
    memset(inode->i_private, 0, sizeof(struct dentry));
    INIT_LIST_HEAD(&((struct dentry *)inode->i_private)->d_subdirs);
    
    new_dentry = alloc_dentry();
    if (!new_dentry) {
        kfree(inode->i_private);
        fs_destroy_inode(inode);
        return -ENOMEM;
    }
    
    strncpy(new_dentry->name, dentry->name, FS_NAME_MAX - 1);
    new_dentry->inode = inode;
    new_dentry->parent = (struct dentry *)dir->i_private;
    list_add(&new_dentry->d_child, &new_dentry->parent->d_subdirs);
    
    atomic_inc(&dir->i_count);
    dir->i_mtime = dir->i_ctime = CURRENT_TIME;
    
    return 0;
}


static file_t* create_file(const char* name, file_type_t type, file_t* parent) {
    file_t* f = kmalloc(sizeof(file_t), GFP_KERNEL);
    if (!f)
        return NULL;

    memset(f, 0, sizeof(file_t));
    strncpy(f->name, name, MAX_NAME_LEN - 1);
    f->type = type;
    f->parent = parent;
    f->child_count = 0;
    f->size = 0;
    f->permissions = (type == FILE_TYPE_DIRECTORY) ? DEFAULT_DIR_PERM : DEFAULT_FILE_PERM;
    f->uid = 0;
    f->gid = 0;
    f->created = f->modified = f->accessed = CURRENT_TIME;
    f->ref_count = 1;
    
    if (type == FILE_TYPE_REGULAR) {
        f->data_capacity = 4096;
        f->data = kmalloc(f->data_capacity, GFP_KERNEL);
        if (!f->data) {
            kfree(f);
            return NULL;
        }
        memset(f->data, 0, f->data_capacity);
    }
    
    return f;
}

void fs_init(void) {
    struct dentry *root_dentry;
    
    printk(KERN_INFO "Initializing filesystem...\n");
    

    inode_cache = kmem_cache_create("inode_cache", sizeof(struct inode),
                                    0, SLAB_HWCACHE_ALIGN, NULL);
    if (!inode_cache) panic("Failed to create inode cache");
    
    dentry_cache = kmem_cache_create("dentry_cache", sizeof(struct dentry),
                                     0, SLAB_HWCACHE_ALIGN, NULL);
    if (!dentry_cache) panic("Failed to create dentry cache");
    

    INIT_LIST_HEAD(&buffer_cache_lru);
    

    fs_rootfs = fs_alloc();
    if (!fs_rootfs) panic("Failed to allocate root filesystem");
    

    root = create_file("/", FILE_TYPE_DIRECTORY, NULL);
    if (!root) panic("fs_init: failed to allocate root directory");
    
    root_dentry = alloc_dentry();
    if (!root_dentry) panic("Failed to allocate root dentry");
    
    strcpy(root_dentry->name, "/");
    root_dentry->parent = NULL;
    INIT_LIST_HEAD(&root_dentry->d_subdirs);
    
    root->inode = fs_alloc_inode(NULL);
    if (!root->inode) panic("Failed to allocate root inode");
    
    root->inode->i_private = root_dentry;
    root->inode->i_mode = S_IFDIR | 0755;
    
    current_dir = root;
    
    fs_mkdir_simple("/bin");
    fs_mkdir_simple("/etc");
    fs_mkdir_simple("/home");
    fs_mkdir_simple("/tmp");
    fs_mkdir_simple("/var");
    fs_mkdir_simple("/dev");
    fs_mkdir_simple("/proc");
    fs_mkdir_simple("/sys");
    
    printk(KERN_INFO "Filesystem initialized successfully\n");
}

void fs_exit(void) {
    struct buffer_head *bh, *tmp;
    
    printk(KERN_INFO "Cleaning up filesystem...\n");
    
    list_for_each_entry_safe(bh, tmp, &buffer_cache_lru, lru) {
        if (bh->dirty) {
            bh->dev->write(bh->dev, bh->data, bh->block, 1);
        }
        kfree(bh->data);
        kfree(bh);
    }
    
    if (inode_cache) kmem_cache_destroy(inode_cache);
    if (dentry_cache) kmem_cache_destroy(dentry_cache);
    
    fs_free(fs_rootfs);
    
    printk(KERN_INFO "Filesystem cleanup complete\n");
}

file_t* fs_find_in_dir(file_t* dir, const char* name) {
    if (!dir || dir->type != FILE_TYPE_DIRECTORY)
        return NULL;

    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return NULL;
}