#include "fs.h"
#include <kernel_b/stdio.h>
#include <kernel_b/string.h>
#include <kernel_b/stdlib.h>
#include <asm/system.h>  

extern file_t* fs_find_in_dir(file_t* dir, const char* name);

void fs_touch(const char* name) {
    if (fs_find_in_dir(current_dir, name)) {
        printf("File already exists: %s\n", name);
        return;
    }

    if (current_dir->child_count >= MAX_CHILDREN) {
        printf("Directory full, cannot create more files.\n");
        return;
    }

    file_t* new_file = (file_t*)malloc(sizeof(file_t));
    if (!new_file) {
        printf("Memory allocation failed.\n");
        return;
    }

    memset(new_file, 0, sizeof(file_t));
    strncpy(new_file->name, name, MAX_NAME_LEN - 1);
    new_file->type = FILE_TYPE_FILE;
    new_file->parent = current_dir;

    current_dir->children[current_dir->child_count++] = new_file;
    printf("Created file: %s\n", name);
}

void fs_rm(const char* name) {
    for (int i = 0; i < current_dir->child_count; i++) {
        file_t* child = current_dir->children[i];

        if (child->type == FILE_TYPE_FILE && strcmp(child->name, name) == 0) {
            free(child);

            // Shift array left
            for (int j = i; j < current_dir->child_count - 1; j++) {
                current_dir->children[j] = current_dir->children[j + 1];
            }
            current_dir->children[--current_dir->child_count] = NULL;

            printf("Removed file: %s\n", name);
            return;
        }
    }

    printf("No such file: %s\n", name);
}

void fs_write(const char* name, const char* data) {
    file_t* f = fs_find_in_dir(current_dir, name);
    if (!f || f->type != FILE_TYPE_FILE) {
        printf("No such file: %s\n", name);
        return;
    }

    strncpy(f->content, data, sizeof(f->content) - 1);
    f->content[sizeof(f->content) - 1] = '\0';  // ensure null-termination

    printf("Wrote to %s\n", name);
}

void fs_read(const char* name) {
    file_t* f = fs_find_in_dir(current_dir, name);
    if (!f || f->type != FILE_TYPE_FILE) {
        printf("No such file: %s\n", name);
        return;
    }

    if (f->content[0] == '\0')
        printf("File is empty: %s\n", name);
    else
        printf("Contents of %s:\n%s\n", name, f->content);
}
