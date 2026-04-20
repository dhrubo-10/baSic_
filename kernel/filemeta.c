/* baSic_ - kernel/filemeta.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * file metadata: per-path permission table
 */

#include "filemeta.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static filemeta_t meta_table[META_MAX];

void filemeta_init(void)
{
    memset(meta_table, 0, sizeof(meta_table));
    /* set sane defaults for system paths */
    filemeta_set("/etc/motd",  PERM_READ);
    filemeta_set("/etc/init",  PERM_READ | PERM_EXEC);
    kprintf("[OK] filemeta: initialized\n");
}

void filemeta_set(const char *path, u8 perms)
{
    for (int i = 0; i < META_MAX; i++) {
        if (meta_table[i].used && !strcmp(meta_table[i].path, path)) {
            meta_table[i].perms = perms;
            return;
        }
    }
    for (int i = 0; i < META_MAX; i++) {
        if (!meta_table[i].used) {
            strncpy(meta_table[i].path, path, 63);
            meta_table[i].path[63] = '\0';
            meta_table[i].perms    = perms;
            meta_table[i].used     = 1;
            return;
        }
    }
    kprintf("[WARN] filemeta: table full\n");
}

u8 filemeta_get(const char *path)
{
    for (int i = 0; i < META_MAX; i++)
        if (meta_table[i].used && !strcmp(meta_table[i].path, path))
            return meta_table[i].perms;
    return PERM_DEFAULT;
}

int filemeta_check(const char *path, u8 perm)
{
    return (filemeta_get(path) & perm) != 0;
}