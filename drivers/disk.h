#ifndef DRV_DISK_H
#define DRV_DISK_H

#include "../inc/types.h"

/* BIOS based LBA read: drive 0x80, lba, count sectors, buffer */
int bios_disk_read_lba(u8 drive, u64 lba, u16 count, void *buf);

int ata_pio_read_lba(u8 drive, u64 lba, u16 count, void *buf);

#endif
