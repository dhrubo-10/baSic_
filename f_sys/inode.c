#include <f_sys/inode.h>
#include <kernel_b/string.h>
#include <kernel_b/stdlib.h>
#include <kernel_b/stdio.h>

static u32 next_inode_id = 1;

inode_t *inode_create(inode_t *parent, const char *name, u32 type) {
    inode_t *node = malloc(sizeof(inode_t));
    if (!node) return NULL;

    memset(node, 0, sizeof(inode_t));

    node->id = next_inode_id++;
    node->type = type;
    node->parent = parent;

    strncpy(node->name, name, 31);

    if (parent && parent->child_count < 16) {
        parent->children[parent->child_count++] = node;
    }

    return node;
}

inode_t *inode_find(inode_t *dir, const char *name) {
    if (!dir || dir->type != INODE_DIR) return NULL;

    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return NULL;
}
