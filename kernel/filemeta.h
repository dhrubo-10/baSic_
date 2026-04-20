/* baSic_ - kernel/filemeta.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * lightweight file metadata layer
 * tracks permissions and type for files in ramfs
 */

#ifndef FILEMETA_H
#define FILEMETA_H

#include "../include/types.h"

#define META_MAX        64

#define PERM_READ       0x01
#define PERM_WRITE      0x02
#define PERM_EXEC       0x04
#define PERM_DEFAULT    (PERM_READ | PERM_WRITE)

typedef struct {
    char path[64];
    u8   perms;
    u8   used;
} filemeta_t;

void filemeta_init(void);
void filemeta_set(const char *path, u8 perms);
u8   filemeta_get(const char *path);
int  filemeta_check(const char *path, u8 perm);

#endif