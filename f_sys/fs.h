#ifndef FS_H
#define FS_H

#include <kernel_b/types.h>
#include <asm/atomic.h>  
#include <asm/spinlock.h> 

#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 4096
#define MAX_CHILDREN 512
#define MAX_OPEN_FILES 256
#define PERM_READ   0x04
#define PERM_WRITE  0x02
#define PERM_EXEC   0x01
#define MAX_FILE_CONTENT 65536
#define DEFAULT_FILE_PERM 0644
#define DEFAULT_DIR_PERM  0755
#define FS_ROOT_INODE   1
#define FS_BLOCK_SIZE   4096
#define FS_MAX_BLOCKS   1024
#define FS_NAME_MAX     255
#define FS_PATH_MAX     4096
#define FS_INODES_MAX   1024

typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_DEVICE,
    FILE_TYPE_PIPE,
    FILE_TYPE_SOCKET
} file_type_t;

typedef enum {
    FS_EXT2,
    FS_FAT32,
    FS_NTFS,
    FS_ISO9660,
    FS_TMPFS
} fs_type_t;
typedef struct file_info {
    char name[MAX_NAME_LEN];
    file_type_t type;
    uint32_t size;
    time_t mtime;
    uint32_t perms;
    uint32_t uid;
    uint32_t gid;
} file_info_t;

typedef struct dir_list {
    file_info_t *entries;
    int count;
    int capacity;
} dir_list_t;

typedef struct file file_t;
typedef struct mount mount_t;
typedef struct superblock superblock_t;
typedef struct inode inode_t;

struct inode {
    uint32_t i_number;
    uint32_t i_mode;
    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_size;
    uint32_t i_blocks;
    time_t i_atime;
    time_t i_mtime;
    time_t i_ctime;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_version;
    uint32_t i_extra[4];
};

struct superblock {
    fs_type_t fs_type;
    uint32_t block_size;
    uint32_t blocks_count;
    uint32_t free_blocks;
    uint32_t inodes_count;
    uint32_t free_inodes;
    uint32_t magic;
    uint32_t state;
    uint32_t errors;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint32_t first_data_block;
    uint32_t log_block_size;
    void *fs_info;
};

struct mount {
    char mount_point[MAX_PATH_LEN];
    superblock_t *sb;
    file_t *root;
    mount_t *next;
};

struct file {
    char name[MAX_NAME_LEN];
    file_type_t type;
    file_t *parent;
    file_t *children[MAX_CHILDREN];
    int child_count;
    
    inode_t *inode;
    uint32_t size;
    time_t created;
    time_t modified;
    time_t accessed;
    uint32_t permissions;
    uint32_t uid;
    uint32_t gid;
    uint32_t device;
    
    void *data;
    uint32_t data_size;
    uint32_t data_capacity;
    
    mount_t *mount;
    file_t *link_target;
    char symlink_target[MAX_PATH_LEN];
    
    uint32_t ref_count;
    uint32_t open_count;
    uint8_t dirty;
};

struct file_handle {
    file_t *file;
    uint32_t offset;
    uint32_t mode;
    uint32_t ref_count;
    struct file_handle *next;
};

struct stat {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_rdev;
    uint64_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    uint32_t st_blksize;
    uint64_t st_blocks;
};
struct buffer_head {
    uint8_t *data;
    uint32_t block;
    uint32_t size;
    uint8_t dirty;
    struct buffer_head *next;
    struct buffer_head *prev;
};

struct block_device {
    uint32_t block_size;
    uint32_t blocks_count;
    uint32_t (*read)(struct block_device *dev, uint8_t *buf, uint32_t block, uint32_t count);
    uint32_t (*write)(struct block_device *dev, const uint8_t *buf, uint32_t block, uint32_t count);
    void *private;
};


struct filesystem {
    struct block_device *dev;
    struct superblock sb;
    struct mount *mounts;
    struct buffer_head *cache;
    spinlock_t lock;
    atomic_t ref_count;
};

struct file_operations {
    ssize_t (*read)(struct file *file, void *buf, size_t count, off_t *offset);
    ssize_t (*write)(struct file *file, const void *buf, size_t count, off_t *offset);
    int (*open)(struct inode *inode, struct file *file);
    int (*release)(struct inode *inode, struct file *file);
    int (*readdir)(struct file *file, void *dirent, filldir_t filldir);
    int (*ioctl)(struct file *file, unsigned int cmd, unsigned long arg);
    int (*mmap)(struct file *file, struct vm_area_struct *vma);
};

struct inode_operations {
    int (*create)(struct inode *dir, struct dentry *dentry, int mode);
    struct dentry *(*lookup)(struct inode *dir, struct dentry *dentry);
    int (*link)(struct dentry *old_dentry, struct inode *dir, struct dentry *new_dentry);
    int (*unlink)(struct inode *dir, struct dentry *dentry);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, int mode);
    int (*rmdir)(struct inode *dir, struct dentry *dentry);
    int (*rename)(struct inode *old_dir, struct dentry *old_dentry,
                  struct inode *new_dir, struct dentry *new_dentry);
    int (*permission)(struct inode *inode, int mask);
};


struct dentry {
    char name[FS_NAME_MAX];
    struct inode *inode;
    struct dentry *parent;
    struct list_head children;
    struct list_head sibling;
    atomic_t ref_count;
    uint32_t flags;
};

struct file_system_type {
    const char *name;
    int fs_flags;
    struct dentry *(*mount)(struct file_system_type *fs_type,
                            int flags, const char *dev_name, void *data);
    void (*kill_sb)(struct super_block *sb);
    struct module *owner;
    struct file_system_type *next;
    struct list_head fs_supers;
};

#define S_IFMT   0170000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFBLK  0060000
#define S_IFIFO  0010000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000

#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_IRWXU  0700
#define S_IRUSR  0400
#define S_IWUSR  0200
#define S_IXUSR  0100
#define S_IRWXG  070
#define S_IRGRP  040
#define S_IWGRP  020
#define S_IXGRP  010
#define S_IRWXO  07
#define S_IROTH  04
#define S_IWOTH  02
#define S_IXOTH  01

extern file_t *fs_root;
extern file_t *current_dir;
extern mount_t *mount_list;
extern struct filesystem *fs_rootfs;
extern spinlock_t fs_lock;
extern struct list_head filesystems;


file_t* fs_find_in_dir(file_t* dir, const char* name);
file_t* fs_lookup(const char *path);
int fs_stat(const char *path, struct stat *statbuf);
file_handle_t* fs_open(const char *path, int flags);
ssize_t fs_read(file_handle_t *handle, void *buf, size_t count);
ssize_t fs_write(file_handle_t *handle, const void *buf, size_t count);
off_t fs_seek(file_handle_t *handle, off_t offset, int whence);
int fs_close(file_handle_t *handle);
int fs_create(const char *path, int mode);
int fs_unlink(const char *path);
int fs_rename(const char *oldpath, const char *newpath);
int fs_chmod(const char *path, uint32_t mode);
int fs_chown(const char *path, uint32_t uid, uint32_t gid);
int fs_truncate(const char *path, uint32_t length);
int fs_mkdir(const char *path, uint32_t mode);
int fs_rmdir(const char *path);
int fs_symlink(const char *target, const char *linkpath);
ssize_t fs_readlink(const char *path, char *buf, size_t bufsize);
int fs_mount(const char *source, const char *target, const char *fstype, unsigned long flags, const void *data);
int fs_umount(const char *target);
int fs_sync(file_t *file);
void fs_sync_all(void);
int fs_touch(const char *name);
int fs_rm(const char *name);
int fs_write(const char *name, const char *data, size_t size);
int fs_append(const char *name, const char *data, size_t size);
char* fs_read(const char *name);
void fs_cat(const char *name);
void fs_ls(const char *path);
void fs_tree(const char *path, int depth);
void fs_pwd(void);
void fs_cd(const char *path);
void fs_mkdir_simple(const char *name);
void fs_rmdir_simple(const char *name);
void fs_stat_simple(const char *name);
uint32_t fs_size(const char *name);
time_t fs_mtime(const char *name);
int fs_copy(const char *src, const char *dst);
int fs_move(const char *src, const char *dst);
int fs_echo(const char *name, const char *text);
int fs_clear(const char *name);
void fs_test(void);
void fs_benchmark(void);
void fs_dump_inode(uint32_t inode);
struct filesystem* fs_alloc(void);
void fs_free(struct filesystem *fs);
int fs_register(const char *name, struct file_system_type *type);
int fs_unregister(const char *name);
struct superblock* fs_read_super(struct block_device *dev);
int fs_write_super(struct superblock *sb);
struct inode* fs_alloc_inode(struct superblock *sb);
void fs_destroy_inode(struct inode *inode);
struct dentry* fs_lookup(struct inode *dir, struct dentry *target);
int fs_create(struct inode *dir, struct dentry *dentry, int mode);
int fs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev);
int fs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *new_dentry);
int fs_unlink(struct inode *dir, struct dentry *dentry);
int fs_symlink(struct inode *dir, struct dentry *dentry, const char *symname);
int fs_mkdir(struct inode *dir, struct dentry *dentry, int mode);
int fs_rmdir(struct inode *dir, struct dentry *dentry);
int fs_rename(struct inode *old_dir, struct dentry *old_dentry,
              struct inode *new_dir, struct dentry *new_dentry);
int fs_permission(struct inode *inode, int mask);
void fs_setattr(struct inode *inode, struct iattr *attr);
void fs_notify_change(struct dentry *dentry, struct iattr *attr);
struct buffer_head* fs_bread(struct block_device *dev, uint32_t block);
int fs_bwrite(struct buffer_head *bh);
void fs_brelse(struct buffer_head *bh);
int fs_sync_inode(struct inode *inode);
void fs_sync_super(struct superblock *sb);
void fs_statfs(struct superblock *sb, struct kstatfs *buf);
int fs_show_options(struct seq_file *seq, struct dentry *root);
void fs_put_super(struct superblock *sb);
int fs_remount_fs(struct superblock *sb, int *flags, char *data);
void fs_umount_begin(struct superblock *sb);
int fs_bmap(struct inode *inode, int block);
int fs_readpage(struct file *file, struct page *page);
int fs_writepage(struct page *page, struct writeback_control *wbc);
int fs_write_begin(struct file *file, struct address_space *mapping,
                   loff_t pos, unsigned len, unsigned flags,
                   struct page **pagep, void **fsdata);
int fs_write_end(struct file *file, struct address_space *mapping,
                 loff_t pos, unsigned len, unsigned copied,
                 struct page *page, void *fsdata);
sector_t fs_bmap(struct address_space *mapping, sector_t block);
int fs_direct_IO(int rw, struct kiocb *iocb, const struct iovec *iov,
                 loff_t offset, unsigned long nr_segs);
int fs_migrate_page(struct address_space *mapping, struct page *newpage,
                    struct page *page, enum migrate_mode mode);
int fs_launder_page(struct page *page);
int fs_is_partially_uptodate(struct page *page, read_descriptor_t *desc,
                             unsigned long from);
int fs_error_remove_page(struct address_space *mapping, struct page *page);
void fs_init_early(void);
void fs_init(void);
void fs_late_init(void);


#endif