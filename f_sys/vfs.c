#include "fs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

file_t* root = NULL;
file_t* current_dir = NULL;

static file_t* create_file(const char* name, file_type_t type, file_t* parent) {
    file_t* f = malloc(sizeof(file_t));
    memset(f, 0, sizeof(file_t));
    strncpy(f->name, name, MAX_NAME_LEN);
    f->type = type;
    f->parent = parent;
    return f;
}

void fs_init() {
    root = create_file("/", FILE_TYPE_DIR, NULL);
    current_dir = root;
}

file_t* fs_find_in_dir(file_t* dir, const char* name) {
    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return NULL;
}
