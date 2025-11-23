#include <kernel_b/types.h>
#include <kernel_b/string.h>
#include <kernel_b/stdlib.h>
#include "fs.h"

static file_t* root = NULL;
static file_t* current_dir = NULL;

static file_t* create_file(const char* name, file_type_t type, file_t* parent) {
    file_t* f = kmalloc(sizeof(file_t));
    if (!f)
        return NULL;

    memset(f, 0, sizeof(file_t));
    strncpy(f->name, name, MAX_NAME_LEN);
    f->type = type;
    f->parent = parent;
    f->child_count = 0;
    return f;
}

void fs_init(void) {
    root = create_file("/", FILE_TYPE_DIR, NULL);
    if (!root)
        panic("fs_init: failed to allocate root directory");
    current_dir = root;
}

file_t* fs_find_in_dir(file_t* dir, const char* name) {
    if (!dir || dir->type != FILE_TYPE_DIR)
        return NULL;

    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return NULL;
}
