#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <kernelb/types.h>

#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_CREAT   0x0040
#define O_TRUNC   0x0200
#define O_APPEND  0x0400

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

#define FS_TYPE_NONE  0
#define FS_TYPE_FAT32 1
#define FS_TYPE_EXT2  2

struct file;
struct inode;
struct superblock;

struct file_operations {
    ssize_t (*read)(struct file *, void *, size_t);
    ssize_t (*write)(struct file *, const void *, size_t);
    int (*open)(struct inode *, struct file *);
    int (*close)(struct inode *, struct file *);
    off_t (*lseek)(struct file *, off_t, int);
};

struct inode_operations {
    int (*create)(struct inode *, const char *, mode_t);
    int (*mkdir)(struct inode *, const char *, mode_t);
    int (*rmdir)(struct inode *, const char *);
    int (*unlink)(struct inode *, const char *);
    int (*rename)(struct inode *, const char *, struct inode *, const char *);
};

struct file {
    struct inode *inode;
    off_t pos;
    uint32_t flags;
    struct file_operations *f_op;
};

struct inode {
    uint32_t i_ino;
    uint32_t i_mode;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_uid;
    uint32_t i_gid;
    struct superblock *i_sb;
    struct inode_operations *i_op;
    void *i_private;
};

struct superblock {
    uint32_t s_magic;
    uint32_t s_blocksize;
    uint32_t s_flags;
    struct file_operations *s_op;
    void *s_fs_info;
};

struct filesystem_type {
    const char *name;
    int fs_type;
    struct superblock *(*mount)(void *device);
    void (*umount)(struct superblock *);
};

int register_filesystem(struct filesystem_type *);
int unregister_filesystem(const char *);
struct superblock *mount_fs(const char *type, void *device);
int umount_fs(struct superblock *);
struct file *file_open(const char *path, int flags);
ssize_t file_read(struct file *, void *, size_t);
ssize_t file_write(struct file *, const void *, size_t);
int file_close(struct file *);
off_t file_seek(struct file *, off_t, int);
int file_create(const char *, mode_t);
int file_unlink(const char *);
int file_mkdir(const char *, mode_t);
int file_rmdir(const char *);
int file_rename(const char *, const char *);

#endif