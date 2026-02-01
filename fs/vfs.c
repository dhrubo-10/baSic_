#include <kernelb/fs.h>
#include <kernelb/stdio.h>
#include <kernelb/string.h>

#define MAX_FILESYSTEMS 8
#define MAX_MOUNTS 16

static struct filesystem_type filesystems[MAX_FILESYSTEMS];
static int fs_count = 0;

static struct superblock mounts[MAX_MOUNTS];
static int mount_count = 0;

int register_filesystem(struct filesystem_type *fs)
{
    if (fs_count >= MAX_FILESYSTEMS) return -1;
    
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(filesystems[i].name, fs->name) == 0) {
            return -2;  // Already registered
        }
    }
    
    filesystems[fs_count++] = *fs;
    printk("Registered filesystem: %s\n", fs->name);
    return 0;
}

struct superblock *mount_fs(const char *type, void *device)
{
    for (int i = 0; i < fs_count; i++) {
        if (strcmp(filesystems[i].name, type) == 0) {
            if (mount_count >= MAX_MOUNTS) return NULL;
            
            struct superblock *sb = filesystems[i].mount(device);
            if (sb) {
                mounts[mount_count++] = *sb;
                printk("Mounted %s\n", type);
                return sb;
            }
        }
    }
    return NULL;
}

struct file *file_open(const char *path, int flags)
{
    // Path resolution logic
    printk("Opening: %s\n", path);
    return NULL;  // Placeholder
}

int fs_init(void)
{
    printk("Initializing VFS...\n");
    fs_count = 0;
    mount_count = 0;
    return 0;
}