/* baSic_ - kernel/ata.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ATA PIO driver: primary bus, master drive
 */

#ifndef ATA_H
#define ATA_H

#include "../include/types.h"

#define ATA_SECTOR_SIZE  512

int ata_init(void);
int ata_read(u32 lba, u8 count, u8 *buf);
int ata_write(u32 lba, u8 count, const u8 *buf);

#endif