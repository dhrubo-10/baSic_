#ifndef FS_H
#define FS_H

#include <kernel_b/types.h>

#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 4096
#define MAX_CHILDREN 512
#define MAX_OPEN_FILES 256
#define PERM_READ   0x04
#define PERM_WRITE  0x02
#define PERM_EXEC   0x01

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

#endif