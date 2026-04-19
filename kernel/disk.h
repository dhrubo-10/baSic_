/* baSic_ - kernel/disk.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * disk abstraction layer: sector cache on top of ATA
 */

#ifndef DISK_H
#define DISK_H

#include "../include/types.h"

#define DISK_CACHE_SLOTS  16
#define DISK_SECTOR_SIZE  512

void disk_init(void);
int  disk_read_sector(u32 lba, u8 *buf);
int  disk_write_sector(u32 lba, const u8 *buf);
void disk_cache_flush(void);

#endif