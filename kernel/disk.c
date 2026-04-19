/* baSic_ - kernel/disk.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * disk abstraction layer with a simple direct-mapped sector cache
 * reduces ATA reads when the same sector is accessed repeatedly
 */

#include "disk.h"
#include "ata.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

typedef struct {
    u32 lba;
    u8  data[DISK_SECTOR_SIZE];
    u8  valid;
    u8  dirty;
} cache_slot_t;

static cache_slot_t cache[DISK_CACHE_SLOTS];
static int disk_present = 0;

void disk_init(void)
{
    memset(cache, 0, sizeof(cache));
    disk_present = ata_init();
    if (disk_present)
        kprintf("[OK] disk: cache ready (%d slots)\n", DISK_CACHE_SLOTS);
}

static cache_slot_t *cache_find(u32 lba)
{
    for (int i = 0; i < DISK_CACHE_SLOTS; i++)
        if (cache[i].valid && cache[i].lba == lba)
            return &cache[i];
    return NULL;
}

static cache_slot_t *cache_evict(u32 lba)
{
    /* find invalid slot first */
    for (int i = 0; i < DISK_CACHE_SLOTS; i++) {
        if (!cache[i].valid) {
            cache[i].lba   = lba;
            cache[i].valid = 1;
            cache[i].dirty = 0;
            return &cache[i];
        }
    }
    /* evict slot 0 (simple FIFO — rotate later) */
    cache_slot_t *s = &cache[0];
    if (s->dirty && disk_present)
        ata_write(s->lba, 1, s->data);
    s->lba   = lba;
    s->valid = 1;
    s->dirty = 0;
    return s;
}

int disk_read_sector(u32 lba, u8 *buf)
{
    cache_slot_t *s = cache_find(lba);
    if (s) {
        memcpy(buf, s->data, DISK_SECTOR_SIZE);
        return 1;
    }
    if (!disk_present) return 0;
    s = cache_evict(lba);
    if (!ata_read(lba, 1, s->data)) {
        s->valid = 0;
        return 0;
    }
    memcpy(buf, s->data, DISK_SECTOR_SIZE);
    return 1;
}

int disk_write_sector(u32 lba, const u8 *buf)
{
    cache_slot_t *s = cache_find(lba);
    if (!s) s = cache_evict(lba);
    memcpy(s->data, buf, DISK_SECTOR_SIZE);
    s->dirty = 1;
    if (!disk_present) return 0;
    if (!ata_write(lba, 1, s->data)) return 0;
    s->dirty = 0;
    return 1;
}

void disk_cache_flush(void)
{
    if (!disk_present) return;
    for (int i = 0; i < DISK_CACHE_SLOTS; i++) {
        if (cache[i].valid && cache[i].dirty) {
            ata_write(cache[i].lba, 1, cache[i].data);
            cache[i].dirty = 0;
        }
    }
}