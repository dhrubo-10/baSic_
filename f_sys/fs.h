#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_NAME_LEN 32
#define MAX_CHILDREN 16
#define MAX_FILES 128

typedef enum {
    FILE_TYPE_FILE,
    FILE_TYPE_DIR
} file_type_t;

typedef struct file {
    char name[MAX_NAME_LEN];
    file_type_t type;
    struct file* parent;
    struct file* children[MAX_CHILDREN];
    int child_count;
    char content[512]; // small in-memory data
} file_t;

extern file_t* root;
extern file_t* current_dir;

void fs_init();
void fs_list(file_t* dir);
void fs_mkdir(const char* name);
void fs_rmdir(const char* name);
void fs_cd(const char* name);
void fs_touch(const char* name);
void fs_rm(const char* name);
void fs_write(const char* name, const char* data);
void fs_read(const char* name);

#endif
