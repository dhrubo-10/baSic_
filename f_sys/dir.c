#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern file_t* fs_find_in_dir(file_t* dir, const char* name);

void fs_list(file_t* dir) {
    printf("Listing %s:\n", dir->name);
    for (int i = 0; i < dir->child_count; i++) {
        printf("  [%s] %s\n",
               dir->children[i]->type == FILE_TYPE_DIR ? "DIR" : "FILE",
               dir->children[i]->name);
    }
}

void fs_mkdir(const char* name) {
    if (fs_find_in_dir(current_dir, name)) {
        printf("Directory already exists: %s\n", name);
        return;
    }
    file_t* new_dir = malloc(sizeof(file_t));
    memset(new_dir, 0, sizeof(file_t));
    strncpy(new_dir->name, name, MAX_NAME_LEN);
    new_dir->type = FILE_TYPE_DIR;
    new_dir->parent = current_dir;
    current_dir->children[current_dir->child_count++] = new_dir;
    printf("Created dir: %s\n", name);
}

void fs_cd(const char* name) {
    if (strcmp(name, "..") == 0) {
        if (current_dir->parent)
            current_dir = current_dir->parent;
        return;
    }

    file_t* dir = fs_find_in_dir(current_dir, name);
    if (!dir || dir->type != FILE_TYPE_DIR) {
        printf("No such directory: %s\n", name);
        return;
    }
    current_dir = dir;
}

void fs_rmdir(const char* name) {
    for (int i = 0; i < current_dir->child_count; i++) {
        file_t* child = current_dir->children[i];
        if (child->type == FILE_TYPE_DIR && strcmp(child->name, name) == 0) {
            free(child);
            for (int j = i; j < current_dir->child_count - 1; j++)
                current_dir->children[j] = current_dir->children[j + 1];
            current_dir->child_count--;
            printf("Removed dir: %s\n", name);
            return;
        }
    }
    printf("No such directory: %s\n", name);
}
