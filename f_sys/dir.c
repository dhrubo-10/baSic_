#include "fs.h"
#include <kernelb/stdio.h>
#include <kernelb/string.h>
#include <kernelb/stdlib.h>
#include <kernelb/errno.h>
#include <kernelb/time.h>

file_t *fs_root = NULL;
file_t *current_dir = NULL;
mount_t *mount_list = NULL;
static file_handle_t *open_files[MAX_OPEN_FILES] = {0};

static void path_split(const char *path, char *dir_part, char *file_part) {
    const char *last_slash = strrchr(path, '/');
    if (!last_slash) {
        strcpy(dir_part, ".");
        strcpy(file_part, path);
        return;
    }
    
    size_t dir_len = last_slash - path;
    if (dir_len == 0) dir_len = 1;
    strncpy(dir_part, path, dir_len);
    dir_part[dir_len] = '\0';
    strcpy(file_part, last_slash + 1);
}

static file_t* path_resolve(const char *path) {
    if (!path || !*path) return NULL;
    
    if (path[0] == '/') return fs_lookup_from(fs_root, path + 1);
    return fs_lookup_from(current_dir, path);
}

file_t* fs_find_in_dir(file_t* dir, const char* name) {
    if (!dir || dir->type != FILE_TYPE_DIRECTORY) return NULL;
    
    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return NULL;
}

file_t* fs_lookup(const char *path) {
    if (!path || !*path) return NULL;
    
    if (strcmp(path, "/") == 0) return fs_root;
    
    char component[MAX_NAME_LEN];
    file_t *current = (path[0] == '/') ? fs_root : current_dir;
    
    const char *start = path + (path[0] == '/');
    while (*start) {
        const char *end = strchr(start, '/');
        if (!end) end = start + strlen(start);
        
        size_t len = end - start;
        if (len >= MAX_NAME_LEN) return NULL;
        
        strncpy(component, start, len);
        component[len] = '\0';
        
        if (len == 0 || strcmp(component, ".") == 0) {
        } else if (strcmp(component, "..") == 0) {
            current = current->parent ? current->parent : current;
        } else {
            file_t *next = fs_find_in_dir(current, component);
            if (!next) return NULL;
            current = next;
        }
        
        start = (*end == '/') ? end + 1 : end;
    }
    
    return current;
}

int fs_stat(const char *path, struct stat *statbuf) {
    file_t *file = fs_lookup(path);
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    
    memset(statbuf, 0, sizeof(struct stat));
    statbuf->st_ino = (uint32_t)(uintptr_t)file;
    statbuf->st_mode = file->permissions;
    
    if (file->type == FILE_TYPE_DIRECTORY)
        statbuf->st_mode |= S_IFDIR;
    else if (file->type == FILE_TYPE_REGULAR)
        statbuf->st_mode |= S_IFREG;
    else if (file->type == FILE_TYPE_SYMLINK)
        statbuf->st_mode |= S_IFLNK;
    
    statbuf->st_nlink = 1;
    statbuf->st_uid = file->uid;
    statbuf->st_gid = file->gid;
    statbuf->st_size = file->size;
    statbuf->st_blksize = 512;
    statbuf->st_blocks = (file->size + 511) / 512;
    statbuf->st_atime = file->accessed;
    statbuf->st_mtime = file->modified;
    statbuf->st_ctime = file->created;
    
    return 0;
}

file_handle_t* fs_open(const char *path, int flags) {
    file_t *file = fs_lookup(path);
    int create = (flags & O_CREAT);
    
    if (!file) {
        if (!create) {
            errno = ENOENT;
            return NULL;
        }
        
        char dir_part[MAX_PATH_LEN];
        char file_part[MAX_NAME_LEN];
        path_split(path, dir_part, file_part);
        
        file_t *parent = fs_lookup(dir_part);
        if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
            errno = ENOTDIR;
            return NULL;
        }
        
        if (fs_find_in_dir(parent, file_part)) {
            errno = EEXIST;
            return NULL;
        }
        
        file = malloc(sizeof(file_t));
        memset(file, 0, sizeof(file_t));
        strncpy(file->name, file_part, MAX_NAME_LEN - 1);
        file->type = FILE_TYPE_REGULAR;
        file->parent = parent;
        file->created = time(NULL);
        file->modified = file->created;
        file->accessed = file->created;
        file->permissions = 0644;
        file->uid = 0;
        file->gid = 0;
        
        parent->children[parent->child_count++] = file;
    }
    
    if (file->type == FILE_TYPE_DIRECTORY && (flags & O_WRONLY)) {
        errno = EISDIR;
        return NULL;
    }
    
    file_handle_t *handle = malloc(sizeof(file_handle_t));
    memset(handle, 0, sizeof(file_handle_t));
    handle->file = file;
    handle->mode = flags;
    handle->ref_count = 1;
    file->open_count++;
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i]) {
            open_files[i] = handle;
            break;
        }
    }
    
    return handle;
}

ssize_t fs_read(file_handle_t *handle, void *buf, size_t count) {
    if (!handle || !handle->file) {
        errno = EBADF;
        return -1;
    }
    
    if (!(handle->mode & O_RDONLY)) {
        errno = EACCES;
        return -1;
    }
    
    file_t *file = handle->file;
    
    if (handle->offset >= file->size)
        return 0;
    
    size_t to_read = count;
    if (handle->offset + to_read > file->size)
        to_read = file->size - handle->offset;
    
    if (file->data) {
        memcpy(buf, (char*)file->data + handle->offset, to_read);
    } else {
        memset(buf, 0, to_read);
    }
    
    handle->offset += to_read;
    file->accessed = time(NULL);
    
    return to_read;
}

ssize_t fs_write(file_handle_t *handle, const void *buf, size_t count) {
    if (!handle || !handle->file) {
        errno = EBADF;
        return -1;
    }
    
    if (!(handle->mode & O_WRONLY)) {
        errno = EACCES;
        return -1;
    }
    
    file_t *file = handle->file;
    
    if (handle->mode & O_APPEND)
        handle->offset = file->size;
    
    size_t new_size = handle->offset + count;
    if (new_size > file->data_capacity) {
        uint32_t new_capacity = ((new_size + 4095) / 4096) * 4096;
        void *new_data = realloc(file->data, new_capacity);
        if (!new_data) {
            errno = ENOMEM;
            return -1;
        }
        
        if (!file->data)
            memset(new_data, 0, new_capacity);
        else if (new_capacity > file->data_capacity)
            memset((char*)new_data + file->data_capacity, 0, 
                   new_capacity - file->data_capacity);
        
        file->data = new_data;
        file->data_capacity = new_capacity;
    }
    
    memcpy((char*)file->data + handle->offset, buf, count);
    handle->offset += count;
    
    if (handle->offset > file->size) {
        file->size = handle->offset;
    }
    
    file->modified = time(NULL);
    file->accessed = file->modified;
    file->dirty = 1;
    
    return count;
}

off_t fs_seek(file_handle_t *handle, off_t offset, int whence) {
    if (!handle) {
        errno = EBADF;
        return -1;
    }
    
    file_t *file = handle->file;
    off_t new_offset;
    
    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = handle->offset + offset;
            break;
        case SEEK_END:
            new_offset = file->size + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    
    if (new_offset < 0) {
        errno = EINVAL;
        return -1;
    }
    
    handle->offset = new_offset;
    return new_offset;
}

int fs_close(file_handle_t *handle) {
    if (!handle) {
        errno = EBADF;
        return -1;
    }
    
    handle->ref_count--;
    if (handle->ref_count > 0)
        return 0;
    
    handle->file->open_count--;
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i] == handle) {
            open_files[i] = NULL;
            break;
        }
    }
    
    free(handle);
    return 0;
}

int fs_create(const char *path, int mode) {
    file_handle_t *handle = fs_open(path, O_CREAT | O_WRONLY | O_TRUNC);
    if (!handle) return -1;
    
    file_t *file = handle->file;
    file->permissions = mode & 0777;
    
    return fs_close(handle);
}

int fs_unlink(const char *path) {
    char dir_part[MAX_PATH_LEN];
    char file_part[MAX_NAME_LEN];
    path_split(path, dir_part, file_part);
    
    file_t *parent = fs_lookup(dir_part);
    if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
        errno = ENOTDIR;
        return -1;
    }
    
    for (int i = 0; i < parent->child_count; i++) {
        file_t *child = parent->children[i];
        if (strcmp(child->name, file_part) == 0) {
            if (child->type == FILE_TYPE_DIRECTORY) {
                errno = EISDIR;
                return -1;
            }
            
            if (child->open_count > 0) {
                errno = EBUSY;
                return -1;
            }
            
            free(child->data);
            free(child);
            
            for (int j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            
            parent->child_count--;
            return 0;
        }
    }
    
    errno = ENOENT;
    return -1;
}

int fs_rename(const char *oldpath, const char *newpath) {
    char old_dir[MAX_PATH_LEN];
    char old_name[MAX_NAME_LEN];
    char new_dir[MAX_PATH_LEN];
    char new_name[MAX_NAME_LEN];
    
    path_split(oldpath, old_dir, old_name);
    path_split(newpath, new_dir, new_name);
    
    file_t *old_parent = fs_lookup(old_dir);
    file_t *new_parent = fs_lookup(new_dir);
    
    if (!old_parent || !new_parent ||
        old_parent->type != FILE_TYPE_DIRECTORY ||
        new_parent->type != FILE_TYPE_DIRECTORY) {
        errno = ENOTDIR;
        return -1;
    }
    
    file_t *file = NULL;
    int old_index = -1;
    
    for (int i = 0; i < old_parent->child_count; i++) {
        if (strcmp(old_parent->children[i]->name, old_name) == 0) {
            file = old_parent->children[i];
            old_index = i;
            break;
        }
    }
    
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    
    if (fs_find_in_dir(new_parent, new_name)) {
        errno = EEXIST;
        return -1;
    }
    
    if (old_parent == new_parent) {
        strncpy(file->name, new_name, MAX_NAME_LEN - 1);
    } else {
        old_parent->children[old_index] = old_parent->children[--old_parent->child_count];
        
        file->parent = new_parent;
        strncpy(file->name, new_name, MAX_NAME_LEN - 1);
        new_parent->children[new_parent->child_count++] = file;
    }
    
    file->modified = time(NULL);
    return 0;
}

int fs_chmod(const char *path, uint32_t mode) {
    file_t *file = fs_lookup(path);
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    
    file->permissions = mode & 0777;
    file->modified = time(NULL);
    return 0;
}

int fs_chown(const char *path, uint32_t uid, uint32_t gid) {
    file_t *file = fs_lookup(path);
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    
    file->uid = uid;
    file->gid = gid;
    file->modified = time(NULL);
    return 0;
}

int fs_truncate(const char *path, uint32_t length) {
    file_t *file = fs_lookup(path);
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    
    if (file->type != FILE_TYPE_REGULAR) {
        errno = EINVAL;
        return -1;
    }
    
    if (length > file->data_capacity) {
        uint32_t new_capacity = ((length + 4095) / 4096) * 4096;
        void *new_data = realloc(file->data, new_capacity);
        if (!new_data) {
            errno = ENOMEM;
            return -1;
        }
        
        if (new_capacity > file->data_capacity)
            memset((char*)new_data + file->data_capacity, 0,
                   new_capacity - file->data_capacity);
        
        file->data = new_data;
        file->data_capacity = new_capacity;
    }
    
    if (length < file->size)
        memset((char*)file->data + length, 0, file->size - length);
    
    file->size = length;
    file->modified = time(NULL);
    file->dirty = 1;
    
    return 0;
}

int fs_mkdir(const char *path, uint32_t mode) {
    char dir_part[MAX_PATH_LEN];
    char dir_name[MAX_NAME_LEN];
    path_split(path, dir_part, dir_name);
    
    if (!dir_name[0]) {
        errno = EINVAL;
        return -1;
    }
    
    file_t *parent = fs_lookup(dir_part);
    if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
        errno = ENOTDIR;
        return -1;
    }
    
    if (fs_find_in_dir(parent, dir_name)) {
        errno = EEXIST;
        return -1;
    }
    
    if (parent->child_count >= MAX_CHILDREN) {
        errno = ENOSPC;
        return -1;
    }
    
    file_t *new_dir = malloc(sizeof(file_t));
    memset(new_dir, 0, sizeof(file_t));
    strncpy(new_dir->name, dir_name, MAX_NAME_LEN - 1);
    new_dir->type = FILE_TYPE_DIRECTORY;
    new_dir->parent = parent;
    new_dir->created = time(NULL);
    new_dir->modified = new_dir->created;
    new_dir->accessed = new_dir->created;
    new_dir->permissions = (mode & 0777) | S_IFDIR;
    new_dir->uid = 0;
    new_dir->gid = 0;
    
    parent->children[parent->child_count++] = new_dir;
    return 0;
}

int fs_rmdir(const char *path) {
    file_t *dir = fs_lookup(path);
    if (!dir) {
        errno = ENOENT;
        return -1;
    }
    
    if (dir->type != FILE_TYPE_DIRECTORY) {
        errno = ENOTDIR;
        return -1;
    }
    
    if (dir == fs_root) {
        errno = EBUSY;
        return -1;
    }
    
    if (dir->child_count > 0) {
        errno = ENOTEMPTY;
        return -1;
    }
    
    if (dir->open_count > 0) {
        errno = EBUSY;
        return -1;
    }
    
    file_t *parent = dir->parent;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == dir) {
            free(dir);
            for (int j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            return 0;
        }
    }
    
    errno = ENOENT;
    return -1;
}

int fs_symlink(const char *target, const char *linkpath) {
    char dir_part[MAX_PATH_LEN];
    char link_name[MAX_NAME_LEN];
    path_split(linkpath, dir_part, link_name);
    
    file_t *parent = fs_lookup(dir_part);
    if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
        errno = ENOTDIR;
        return -1;
    }
    
    if (fs_find_in_dir(parent, link_name)) {
        errno = EEXIST;
        return -1;
    }
    
    file_t *link = malloc(sizeof(file_t));
    memset(link, 0, sizeof(file_t));
    strncpy(link->name, link_name, MAX_NAME_LEN - 1);
    strncpy(link->symlink_target, target, MAX_PATH_LEN - 1);
    link->type = FILE_TYPE_SYMLINK;
    link->parent = parent;
    link->created = time(NULL);
    link->modified = link->created;
    link->accessed = link->created;
    link->permissions = 0777;
    link->uid = 0;
    link->gid = 0;
    link->size = strlen(target);
    
    parent->children[parent->child_count++] = link;
    return 0;
}

ssize_t fs_readlink(const char *path, char *buf, size_t bufsize) {
    file_t *link = fs_lookup(path);
    if (!link) {
        errno = ENOENT;
        return -1;
    }
    
    if (link->type != FILE_TYPE_SYMLINK) {
        errno = EINVAL;
        return -1;
    }
    
    size_t len = strlen(link->symlink_target);
    if (bufsize - 1 < len)
        len = bufsize - 1;
    
    strncpy(buf, link->symlink_target, len);
    buf[len] = '\0';
    
    return len;
}

int fs_mount(const char *source, const char *target, const char *fstype,
             unsigned long flags, const void *data) {
    file_t *mount_point = fs_lookup(target);
    if (!mount_point || mount_point->type != FILE_TYPE_DIRECTORY) {
        errno = ENOTDIR;
        return -1;
    }
    
    mount_t *mount = malloc(sizeof(mount_t));
    memset(mount, 0, sizeof(mount_t));
    strncpy(mount->mount_point, target, MAX_PATH_LEN - 1);
    
    mount->sb = malloc(sizeof(superblock_t));
    memset(mount->sb, 0, sizeof(superblock_t));
    
    if (strcmp(fstype, "ext2") == 0) mount->sb->fs_type = FS_EXT2;
    else if (strcmp(fstype, "fat32") == 0) mount->sb->fs_type = FS_FAT32;
    else if (strcmp(fstype, "tmpfs") == 0) mount->sb->fs_type = FS_TMPFS;
    else {
        free(mount->sb);
        free(mount);
        errno = ENODEV;
        return -1;
    }
    
    mount->root = mount_point;
    mount->next = mount_list;
    mount_list = mount;
    
    mount_point->mount = mount;
    
    return 0;
}

int fs_umount(const char *target) {
    mount_t *prev = NULL;
    mount_t *mount = mount_list;
    
    while (mount) {
        if (strcmp(mount->mount_point, target) == 0) {
            if (mount->root->open_count > 0 || mount->root->child_count > 0) {
                errno = EBUSY;
                return -1;
            }
            
            if (prev) prev->next = mount->next;
            else mount_list = mount->next;
            
            mount->root->mount = NULL;
            free(mount->sb);
            free(mount);
            return 0;
        }
        prev = mount;
        mount = mount->next;
    }
    
    errno = EINVAL;
    return -1;
}

int fs_sync(file_t *file) {
    if (!file) return 0;
    
    if (file->dirty) {
        if (file->mount && file->mount->sb) {
        }
        file->dirty = 0;
    }
    
    if (file->type == FILE_TYPE_DIRECTORY) {
        for (int i = 0; i < file->child_count; i++) {
            fs_sync(file->children[i]);
        }
    }
    
    return 0;
}

void fs_sync_all(void) {
    fs_sync(fs_root);
    
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i] && open_files[i]->file->dirty) {
            fs_sync(open_files[i]->file);
        }
    }
}

void fs_list(file_t* dir) {
    if (!dir || dir->type != FILE_TYPE_DIRECTORY) {
        printf("Not a directory\n");
        return;
    }
    
    printf("Contents of %s:\n", dir->name);
    printf("Permissions  Size  Created              Name\n");
    printf("-----------  ----  -------------------  ----\n");
    
    for (int i = 0; i < dir->child_count; i++) {
        file_t *child = dir->children[i];
        char perms[11] = "----------";
        
        if (child->type == FILE_TYPE_DIRECTORY) perms[0] = 'd';
        else if (child->type == FILE_TYPE_SYMLINK) perms[0] = 'l';
        
        if (child->permissions & S_IRUSR) perms[1] = 'r';
        if (child->permissions & S_IWUSR) perms[2] = 'w';
        if (child->permissions & S_IXUSR) perms[3] = 'x';
        if (child->permissions & S_IRGRP) perms[4] = 'r';
        if (child->permissions & S_IWGRP) perms[5] = 'w';
        if (child->permissions & S_IXGRP) perms[6] = 'x';
        if (child->permissions & S_IROTH) perms[7] = 'r';
        if (child->permissions & S_IWOTH) perms[8] = 'w';
        if (child->permissions & S_IXOTH) perms[9] = 'x';
        
        struct tm *tm_info = localtime(&child->created);
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        printf("%s  %6u  %s  %s",
               perms, child->size, time_buf, child->name);
        
        if (child->type == FILE_TYPE_SYMLINK)
            printf(" -> %s", child->symlink_target);
        
        printf("\n");
    }
}

void fs_cd(const char *name) {
    if (!name || !*name) return;
    
    if (strcmp(name, "/") == 0) {
        current_dir = fs_root;
        return;
    }
    
    if (strcmp(name, "..") == 0) {
        if (current_dir->parent)
            current_dir = current_dir->parent;
        return;
    }
    
    if (strcmp(name, ".") == 0) return;
    
    file_t *dir = fs_lookup_from(current_dir, name);
    if (!dir || dir->type != FILE_TYPE_DIRECTORY) {
        printf("No such directory: %s\n", name);
        return;
    }
    current_dir = dir;
}

void fs_init(void) {
    fs_root = malloc(sizeof(file_t));
    memset(fs_root, 0, sizeof(file_t));
    strcpy(fs_root->name, "/");
    fs_root->type = FILE_TYPE_DIRECTORY;
    fs_root->parent = NULL;
    fs_root->created = time(NULL);
    fs_root->modified = fs_root->created;
    fs_root->accessed = fs_root->created;
    fs_root->permissions = 0755;
    fs_root->uid = 0;
    fs_root->gid = 0;
    
    current_dir = fs_root;
    
    fs_mkdir("/bin", 0755);
    fs_mkdir("/etc", 0755);
    fs_mkdir("/home", 0755);
    fs_mkdir("/tmp", 0777);
    fs_mkdir("/var", 0755);
    
    printf("Filesystem initialized\n");
}