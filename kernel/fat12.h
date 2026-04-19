/* baSic_ - kernel/fat12.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * FAT12 filesystem driver: read and write
 */

#ifndef FAT12_H
#define FAT12_H

#include "../include/types.h"

#define FAT12_NAME_MAX   13

typedef struct {
    char name[FAT12_NAME_MAX];
    u8   is_dir;
    u32  size;
    u16  first_cluster;
} fat12_dirent_t;

int  fat12_init(void);
int  fat12_list(const char *path, fat12_dirent_t *out, int max);
int  fat12_read(const char *path, u8 *buf, u32 buf_size);
int  fat12_write(const char *path, const u8 *buf, u32 size);
int  fat12_delete(const char *path);
int  fat12_exists(const char *path);

#endif