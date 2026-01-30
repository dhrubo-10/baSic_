#include "fs.h"
#include <kernel_b/stdio.h>
#include <kernel_b/string.h>
#include <kernel_b/stdlib.h>
#include <kernel_b/errno.h>
#include <kernel_b/time.h>
#include <kernel_b/unistd.h>

int fs_touch(const char *name) {
    if (!name || !*name) {
        printf("Error: Invalid filename\n");
        return -1;
    }
    
    file_t *existing = fs_find_in_dir(current_dir, name);
    if (existing) {
        if (existing->type == FILE_TYPE_REGULAR) {
            existing->modified = time(NULL);
            existing->accessed = existing->modified;
            printf("Updated timestamp: %s\n", name);
            return 0;
        } else {
            printf("Error: '%s' is not a regular file\n", name);
            return -1;
        }
    }
    
    if (current_dir->child_count >= MAX_CHILDREN) {
        printf("Error: Directory full\n");
        errno = ENOSPC;
        return -1;
    }
    
    file_t *new_file = malloc(sizeof(file_t));
    if (!new_file) {
        printf("Error: Memory allocation failed\n");
        errno = ENOMEM;
        return -1;
    }
    
    memset(new_file, 0, sizeof(file_t));
    strncpy(new_file->name, name, MAX_NAME_LEN - 1);
    new_file->type = FILE_TYPE_REGULAR;
    new_file->parent = current_dir;
    new_file->created = time(NULL);
    new_file->modified = new_file->created;
    new_file->accessed = new_file->created;
    new_file->permissions = DEFAULT_FILE_PERM;
    new_file->uid = getuid();
    new_file->gid = getgid();
    new_file->size = 0;
    new_file->ref_count = 1;
    
    new_file->data_capacity = 4096;
    new_file->data = malloc(new_file->data_capacity);
    if (!new_file->data) {
        free(new_file);
        printf("Error: Memory allocation failed\n");
        errno = ENOMEM;
        return -1;
    }
    memset(new_file->data, 0, new_file->data_capacity);
    
    current_dir->children[current_dir->child_count++] = new_file;
    printf("Created file: %s\n", name);
    return 0;
}

int fs_rm(const char *name) {
    if (!name || !*name) {
        printf("Error: Invalid filename\n");
        return -1;
    }
    
    for (int i = 0; i < current_dir->child_count; i++) {
        file_t *child = current_dir->children[i];
        
        if (child->type == FILE_TYPE_REGULAR && strcmp(child->name, name) == 0) {
            if (child->open_count > 0) {
                printf("Error: File '%s' is currently open\n", name);
                errno = EBUSY;
                return -1;
            }
            
            if (child->data) free(child->data);
            free(child);
            
            for (int j = i; j < current_dir->child_count - 1; j++) {
                current_dir->children[j] = current_dir->children[j + 1];
            }
            current_dir->children[--current_dir->child_count] = NULL;
            
            printf("Removed file: %s\n", name);
            return 0;
        }
    }
    
    printf("Error: No such file: %s\n", name);
    errno = ENOENT;
    return -1;
}

int fs_write(const char *name, const char *data, size_t size) {
    if (!name || !*name) {
        printf("Error: Invalid filename\n");
        return -1;
    }
    
    if (!data && size > 0) return -1;
    
    file_t *file = fs_find_in_dir(current_dir, name);
    if (!file) {
        printf("Error: No such file: %s\n", name);
        errno = ENOENT;
        return -1;
    }
    
    if (file->type != FILE_TYPE_REGULAR) {
        printf("Error: '%s' is not a regular file\n", name);
        errno = EINVAL;
        return -1;
    }
    
    if (!(file->permissions & PERM_WRITE)) {
        printf("Error: Permission denied\n");
        errno = EACCES;
        return -1;
    }
    
    if (size > file->data_capacity) {
        uint32_t new_capacity = ((size + 4095) / 4096) * 4096;
        void *new_data = realloc(file->data, new_capacity);
        if (!new_data) {
            printf("Error: Memory allocation failed\n");
            errno = ENOMEM;
            return -1;
        }
        
        if (new_capacity > file->data_capacity) {
            memset((char*)new_data + file->data_capacity, 0, new_capacity - file->data_capacity);
        }
        
        file->data = new_data;
        file->data_capacity = new_capacity;
    }
    
    if (size > 0) memcpy(file->data, data, size);
    
    file->size = size;
    file->modified = time(NULL);
    file->accessed = file->modified;
    file->dirty = 1;
    
    printf("Wrote %zu bytes to %s\n", size, name);
    return 0;
}

int fs_append(const char *name, const char *data, size_t size) {
    if (!name || !*name) return -1;
    if (!data && size > 0) return -1;
    
    file_t *file = fs_find_in_dir(current_dir, name);
    if (!file) {
        printf("Error: No such file: %s\n", name);
        errno = ENOENT;
        return -1;
    }
    
    if (file->type != FILE_TYPE_REGULAR) {
        printf("Error: '%s' is not a regular file\n", name);
        errno = EINVAL;
        return -1;
    }
    
    if (!(file->permissions & PERM_WRITE)) {
        printf("Error: Permission denied\n");
        errno = EACCES;
        return -1;
    }
    
    size_t new_size = file->size + size;
    
    if (new_size > file->data_capacity) {
        uint32_t new_capacity = ((new_size + 4095) / 4096) * 4096;
        void *new_data = realloc(file->data, new_capacity);
        if (!new_data) {
            printf("Error: Memory allocation failed\n");
            errno = ENOMEM;
            return -1;
        }
        
        if (new_capacity > file->data_capacity) {
            memset((char*)new_data + file->data_capacity, 0, new_capacity - file->data_capacity);
        }
        
        file->data = new_data;
        file->data_capacity = new_capacity;
    }
    
    if (size > 0) memcpy((char*)file->data + file->size, data, size);
    
    file->size = new_size;
    file->modified = time(NULL);
    file->accessed = file->modified;
    file->dirty = 1;
    
    printf("Appended %zu bytes to %s (new size: %zu)\n", size, name, new_size);
    return 0;
}

char* fs_read(const char *name) {
    if (!name || !*name) {
        printf("Error: Invalid filename\n");
        return NULL;
    }
    
    file_t *file = fs_find_in_dir(current_dir, name);
    if (!file) {
        printf("Error: No such file: %s\n", name);
        errno = ENOENT;
        return NULL;
    }
    
    if (file->type != FILE_TYPE_REGULAR) {
        printf("Error: '%s' is not a regular file\n", name);
        errno = EINVAL;
        return NULL;
    }
    
    if (!(file->permissions & PERM_READ)) {
        printf("Error: Permission denied\n");
        errno = EACCES;
        return NULL;
    }
    
    char *content = malloc(file->size + 1);
    if (!content) {
        printf("Error: Memory allocation failed\n");
        errno = ENOMEM;
        return NULL;
    }
    
    if (file->size > 0) memcpy(content, file->data, file->size);
    content[file->size] = '\0';
    
    file->accessed = time(NULL);
    
    return content;
}

void fs_cat(const char *name) {
    char *content = fs_read(name);
    if (content) {
        printf("%s\n", content);
        free(content);
    }
}

void fs_ls(const char *path) {
    file_t *dir = current_dir;
    
    if (path && *path) {
        dir = fs_lookup(path);
        if (!dir) {
            printf("Error: No such directory: %s\n", path);
            return;
        }
        
        if (dir->type != FILE_TYPE_DIRECTORY) {
            printf("Error: '%s' is not a directory\n", path);
            return;
        }
    }
    
    printf("Contents of %s:\n", dir == current_dir ? "." : path);
    printf("Type  Size      Modified            Permissions  Name\n");
    printf("----  --------  ------------------  -----------  ----\n");
    
    for (int i = 0; i < dir->child_count; i++) {
        file_t *child = dir->children[i];
        
        char type_char;
        switch (child->type) {
            case FILE_TYPE_REGULAR:   type_char = '-'; break;
            case FILE_TYPE_DIRECTORY: type_char = 'd'; break;
            case FILE_TYPE_SYMLINK:   type_char = 'l'; break;
            case FILE_TYPE_DEVICE:    type_char = 'c'; break;
            case FILE_TYPE_PIPE:      type_char = 'p'; break;
            case FILE_TYPE_SOCKET:    type_char = 's'; break;
            default:                  type_char = '?'; break;
        }
        
        struct tm *tm_info = localtime(&child->modified);
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        
        char perms[11] = "----------";
        perms[0] = type_char;
        
        if (child->permissions & S_IRUSR) perms[1] = 'r';
        if (child->permissions & S_IWUSR) perms[2] = 'w';
        if (child->permissions & S_IXUSR) perms[3] = 'x';
        if (child->permissions & S_IRGRP) perms[4] = 'r';
        if (child->permissions & S_IWGRP) perms[5] = 'w';
        if (child->permissions & S_IXGRP) perms[6] = 'x';
        if (child->permissions & S_IROTH) perms[7] = 'r';
        if (child->permissions & S_IWOTH) perms[8] = 'w';
        if (child->permissions & S_IXOTH) perms[9] = 'x';
        
        if (child->type == FILE_TYPE_DIRECTORY) {
            printf("%c     %-8u  %s  %s  %s/\n", type_char, child->size, time_buf, perms, child->name);
        } else {
            printf("%c     %-8u  %s  %s  %s\n", type_char, child->size, time_buf, perms, child->name);
        }
    }
    
    if (dir->child_count == 0) printf("(empty)\n");
}

void fs_tree_recursive(file_t *dir, int depth, int max_depth) {
    if (max_depth >= 0 && depth > max_depth) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("|-- %s\n", dir->name);
    
    for (int i = 0; i < dir->child_count; i++) {
        file_t *child = dir->children[i];
        
        if (child->type == FILE_TYPE_DIRECTORY) {
            fs_tree_recursive(child, depth + 1, max_depth);
        } else {
            for (int j = 0; j < depth + 1; j++) printf("  ");
            printf("|-- %s\n", child->name);
        }
    }
}

void fs_tree(const char *path, int depth) {
    file_t *dir = current_dir;
    
    if (path && *path) {
        dir = fs_lookup(path);
        if (!dir) {
            printf("Error: No such directory: %s\n", path);
            return;
        }
        
        if (dir->type != FILE_TYPE_DIRECTORY) {
            printf("Error: '%s' is not a directory\n", path);
            return;
        }
    }
    
    printf("%s\n", dir == fs_root ? "/" : dir->name);
    fs_tree_recursive(dir, 0, depth);
}

void fs_pwd(void) {
    char path[MAX_PATH_LEN] = "";
    file_t *dir = current_dir;
    
    while (dir && dir != fs_root) {
        char temp[MAX_PATH_LEN];
        snprintf(temp, sizeof(temp), "/%s%s", dir->name, path);
        strcpy(path, temp);
        dir = dir->parent;
    }
    
    printf("%s\n", path[0] ? path : "/");
}

void fs_cd(const char *path) {
    if (!path || !*path) {
        printf("Error: Invalid path\n");
        return;
    }
    
    if (strcmp(path, "/") == 0) {
        current_dir = fs_root;
        return;
    }
    
    if (strcmp(path, ".") == 0) return;
    
    if (strcmp(path, "..") == 0) {
        if (current_dir->parent) current_dir = current_dir->parent;
        return;
    }
    
    file_t *new_dir = fs_lookup(path);
    if (!new_dir) {
        printf("Error: No such directory: %s\n", path);
        return;
    }
    
    if (new_dir->type != FILE_TYPE_DIRECTORY) {
        printf("Error: '%s' is not a directory\n", path);
        return;
    }
    
    current_dir = new_dir;
}

void fs_mkdir_simple(const char *name) {
    int result = fs_mkdir(name, DEFAULT_DIR_PERM);
    if (result == 0) printf("Created directory: %s\n", name);
}

void fs_rmdir_simple(const char *name) {
    int result = fs_rmdir(name);
    if (result == 0) printf("Removed directory: %s\n", name);
}

void fs_stat_simple(const char *name) {
    struct stat statbuf;
    int result = fs_stat(name, &statbuf);
    if (result != 0) return;
    
    printf("File: %s\n", name);
    printf("  Size: %llu bytes\n", statbuf.st_size);
    printf("  Inode: %u\n", statbuf.st_ino);
    printf("  Links: %u\n", statbuf.st_nlink);
    
    char type_str[20];
    if (S_ISREG(statbuf.st_mode)) strcpy(type_str, "Regular file");
    else if (S_ISDIR(statbuf.st_mode)) strcpy(type_str, "Directory");
    else if (S_ISLNK(statbuf.st_mode)) strcpy(type_str, "Symbolic link");
    else if (S_ISCHR(statbuf.st_mode)) strcpy(type_str, "Character device");
    else if (S_ISBLK(statbuf.st_mode)) strcpy(type_str, "Block device");
    else strcpy(type_str, "Unknown");
    
    printf("  Type: %s\n", type_str);
    
    char perms[11] = "----------";
    if (S_ISDIR(statbuf.st_mode)) perms[0] = 'd';
    else if (S_ISLNK(statbuf.st_mode)) perms[0] = 'l';
    else if (S_ISCHR(statbuf.st_mode)) perms[0] = 'c';
    else if (S_ISBLK(statbuf.st_mode)) perms[0] = 'b';
    
    if (statbuf.st_mode & S_IRUSR) perms[1] = 'r';
    if (statbuf.st_mode & S_IWUSR) perms[2] = 'w';
    if (statbuf.st_mode & S_IXUSR) perms[3] = 'x';
    if (statbuf.st_mode & S_IRGRP) perms[4] = 'r';
    if (statbuf.st_mode & S_IWGRP) perms[5] = 'w';
    if (statbuf.st_mode & S_IXGRP) perms[6] = 'x';
    if (statbuf.st_mode & S_IROTH) perms[7] = 'r';
    if (statbuf.st_mode & S_IWOTH) perms[8] = 'w';
    if (statbuf.st_mode & S_IXOTH) perms[9] = 'x';
    
    printf("  Permissions: %s\n", perms);
    
    struct tm *atime = localtime(&statbuf.st_atime);
    struct tm *mtime = localtime(&statbuf.st_mtime);
    struct tm *ctime = localtime(&statbuf.st_ctime);
    
    char atime_str[64], mtime_str[64], ctime_str[64];
    strftime(atime_str, sizeof(atime_str), "%Y-%m-%d %H:%M:%S", atime);
    strftime(mtime_str, sizeof(mtime_str), "%Y-%m-%d %H:%M:%S", mtime);
    strftime(ctime_str, sizeof(ctime_str), "%Y-%m-%d %H:%M:%S", ctime);
    
    printf("  Accessed: %s\n", atime_str);
    printf("  Modified: %s\n", mtime_str);
    printf("  Created:  %s\n", ctime_str);
}

uint32_t fs_size(const char *name) {
    file_t *file = fs_find_in_dir(current_dir, name);
    if (!file) {
        printf("Error: No such file: %s\n", name);
        return 0;
    }
    return file->size;
}

time_t fs_mtime(const char *name) {
    file_t *file = fs_find_in_dir(current_dir, name);
    if (!file) {
        printf("Error: No such file: %s\n", name);
        return 0;
    }
    return file->modified;
}

int fs_copy(const char *src, const char *dst) {
    if (!src || !dst) return -1;
    
    char *content = fs_read(src);
    if (!content) return -1;
    
    file_t *dst_file = fs_find_in_dir(current_dir, dst);
    if (dst_file) {
        printf("Error: Destination file already exists\n");
        free(content);
        return -1;
    }
    
    int result = fs_write(dst, content, strlen(content));
    free(content);
    
    if (result == 0) printf("Copied %s to %s\n", src, dst);
    return result;
}

int fs_move(const char *src, const char *dst) {
    if (!src || !dst) return -1;
    
    if (fs_copy(src, dst) != 0) return -1;
    
    return fs_rm(src);
}

int fs_echo(const char *name, const char *text) {
    if (!name || !text) return -1;
    
    int result = fs_write(name, text, strlen(text));
    if (result == 0) printf("Echoed to %s\n", name);
    return result;
}

int fs_clear(const char *name) {
    if (!name) return -1;
    
    int result = fs_write(name, "", 0);
    if (result == 0) printf("Cleared %s\n", name);
    return result;
}

void fs_test(void) {
    printf("=== Filesystem Test ===\n");
    
    printf("1. Creating test files...\n");
    fs_touch("test1.txt");
    fs_touch("test2.txt");
    fs_mkdir_simple("test_dir");
    
    printf("2. Writing data...\n");
    fs_write("test1.txt", "Hello World!", 12);
    fs_append("test1.txt", " Appended!", 10);
    
    printf("3. Reading data...\n");
    char *content = fs_read("test1.txt");
    if (content) {
        printf("Content: %s\n", content);
        free(content);
    }
    
    printf("4. Directory listing...\n");
    fs_ls(NULL);
    
    printf("5. File information...\n");
    fs_stat_simple("test1.txt");
    
    printf("6. Copy test...\n");
    fs_copy("test1.txt", "test_copy.txt");
    
    printf("7. Cleanup...\n");
    fs_rm("test1.txt");
    fs_rm("test2.txt");
    fs_rm("test_copy.txt");
    fs_rmdir_simple("test_dir");
    
    printf("=== Test Complete ===\n");
}

void fs_benchmark(void) {
    printf("=== Filesystem Benchmark ===\n");
    
    time_t start = time(NULL);
    
    const int iterations = 100;
    const char *test_data = "This is test data for benchmarking filesystem operations.";
    size_t data_len = strlen(test_data);
    
    printf("Creating %d test files...\n", iterations);
    for (int i = 0; i < iterations; i++) {
        char name[32];
        snprintf(name, sizeof(name), "bench_%03d.txt", i);
        fs_touch(name);
        fs_write(name, test_data, data_len);
    }
    
    printf("Reading %d test files...\n", iterations);
    for (int i = 0; i < iterations; i++) {
        char name[32];
        snprintf(name, sizeof(name), "bench_%03d.txt", i);
        char *content = fs_read(name);
        if (content) free(content);
    }
    
    printf("Deleting %d test files...\n", iterations);
    for (int i = 0; i < iterations; i++) {
        char name[32];
        snprintf(name, sizeof(name), "bench_%03d.txt", i);
        fs_rm(name);
    }
    
    time_t end = time(NULL);
    time_t elapsed = end - start;
    
    printf("Time elapsed: %ld seconds\n", elapsed);
    printf("Operations per second: %.2f\n", (float)(iterations * 3) / elapsed);
    printf("=== Benchmark Complete ===\n");
}

void fs_dump_inode(uint32_t inode) {
    printf("Inode dump not implemented\n");
}
