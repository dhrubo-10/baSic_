/* baSic_ - kernel/fat12.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * FAT12 read/write driver
 * BPB parse, cluster chain, 8.3 names, directory entry management
 */

#include "fat12.h"
#include "disk.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static u16 bytes_per_sector;
static u8  sectors_per_cluster;
static u16 reserved_sectors;
static u8  num_fats;
static u16 root_entry_count;
static u16 sectors_per_fat;
static u32 total_sectors;
static u32 fat_start_lba;
static u32 root_start_lba;
static u32 data_start_lba;
static u32 total_clusters;
static int fat12_ready = 0;

static u8  fat_data[1024];
static int fat_dirty = 0;

static inline u16 read_u16(const u8 *p) { return (u16)(p[0] | ((u16)p[1] << 8)); }
static inline u32 read_u32(const u8 *p)
{
    return (u32)p[0]|((u32)p[1]<<8)|((u32)p[2]<<16)|((u32)p[3]<<24);
}

static void write_u16(u8 *p, u16 v) { p[0]=(u8)v; p[1]=(u8)(v>>8); }
static void write_u32(u8 *p, u32 v)
{
    p[0]=(u8)v; p[1]=(u8)(v>>8); p[2]=(u8)(v>>16); p[3]=(u8)(v>>24);
}

int fat12_init(void)
{
    u8 sector[512];
    if (!disk_read_sector(0, sector)) {
        kprintf("[FAT12] boot sector read failed\n");
        return 0;
    }
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        kprintf("[FAT12] no boot signature\n");
        return 0;
    }

    bytes_per_sector    = read_u16(sector + 11);
    sectors_per_cluster = sector[13];
    reserved_sectors    = read_u16(sector + 14);
    num_fats            = sector[16];
    root_entry_count    = read_u16(sector + 17);
    total_sectors       = read_u16(sector + 19);
    if (!total_sectors) total_sectors = read_u32(sector + 32);
    sectors_per_fat     = read_u16(sector + 22);

    fat_start_lba  = reserved_sectors;
    root_start_lba = fat_start_lba + (u32)num_fats * sectors_per_fat;
    data_start_lba = root_start_lba + (root_entry_count * 32 + 511) / 512;
    total_clusters = (total_sectors - data_start_lba) / sectors_per_cluster;

    disk_read_sector(fat_start_lba, fat_data);
    if (sectors_per_fat > 1)
        disk_read_sector(fat_start_lba + 1, fat_data + 512);

    fat_dirty  = 0;
    fat12_ready = 1;
    kprintf("[OK] FAT12: %d clusters  root@%d  data@%d\n",
            (int)total_clusters, (int)root_start_lba, (int)data_start_lba);
    return 1;
}

static u16 fat12_get(u16 cluster)
{
    u32 offset = cluster + cluster / 2;
    u16 val;
    if (cluster & 1)
        val = (u16)((fat_data[offset] >> 4) | (fat_data[offset+1] << 4));
    else
        val = (u16)(fat_data[offset] | ((fat_data[offset+1] & 0x0F) << 8));
    return val & 0x0FFF;
}

static void fat12_set(u16 cluster, u16 value)
{
    u32 offset = cluster + cluster / 2;
    if (cluster & 1) {
        fat_data[offset]   = (u8)((fat_data[offset] & 0x0F) | (value << 4));
        fat_data[offset+1] = (u8)(value >> 4);
    } else {
        fat_data[offset]   = (u8)(value & 0xFF);
        fat_data[offset+1] = (u8)((fat_data[offset+1] & 0xF0) | ((value >> 8) & 0x0F));
    }
    fat_dirty = 1;
}

static void fat_flush(void)
{
    if (!fat_dirty) return;
    disk_write_sector(fat_start_lba, fat_data);
    if (sectors_per_fat > 1)
        disk_write_sector(fat_start_lba + 1, fat_data + 512);
    /* write second copy of FAT */
    if (num_fats > 1) {
        disk_write_sector(fat_start_lba + sectors_per_fat, fat_data);
        if (sectors_per_fat > 1)
            disk_write_sector(fat_start_lba + sectors_per_fat + 1, fat_data + 512);
    }
    fat_dirty = 0;
}

static u16 fat12_alloc_cluster(void)
{
    for (u16 c = 2; c < (u16)(total_clusters + 2); c++)
        if (fat12_get(c) == 0x000) return c;
    return 0;
}

static void fat83_to_str(const u8 *raw, char *out)
{
    int i, j = 0;
    for (i = 0; i < 8 && raw[i] != ' '; i++)
        out[j++] = (char)raw[i];
    if (raw[8] != ' ') {
        out[j++] = '.';
        for (i = 8; i < 11 && raw[i] != ' '; i++)
            out[j++] = (char)raw[i];
    }
    out[j] = '\0';
}

/** uppercase converter */
static void str_to_fat83(const char *name, u8 *out)
{
    memset(out, ' ', 11);
    int dot = -1;
    for (int i = 0; name[i]; i++)
        if (name[i] == '.') { dot = i; break; }

    int base_len = (dot >= 0) ? dot : (int)strlen(name);
    if (base_len > 8) base_len = 8;

    for (int i = 0; i < base_len; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[i] = (u8)c;
    }

    if (dot >= 0) {
        int ext_len = (int)strlen(name) - dot - 1;
        if (ext_len > 3) ext_len = 3;
        for (int i = 0; i < ext_len; i++) {
            char c = name[dot + 1 + i];
            if (c >= 'a' && c <= 'z') c -= 32;
            out[8 + i] = (u8)c;
        }
    }
}

static int names_match(const char *fat_name, const char *query)
{
    usize nl = strlen(fat_name), ql = strlen(query);
    if (nl != ql) return 0;
    for (usize i = 0; i < nl; i++) {
        char a = fat_name[i], b = query[i];
        if (a >= 'a' && a <= 'z') a -= 32;
        if (b >= 'a' && b <= 'z') b -= 32;
        if (a != b) return 0;
    }
    return 1;
}

int fat12_list(const char *path, fat12_dirent_t *out, int max)
{
    if (!fat12_ready) return 0;
    (void)path;

    u8  sector[512];
    int found = 0;
    u32 lba   = root_start_lba;
    u32 left  = root_entry_count;

    while (left > 0 && found < max) {
        disk_read_sector(lba++, sector);
        u32 per = 512 / 32;
        if (per > left) per = left;
        left -= per;

        for (u32 i = 0; i < per && found < max; i++) {
            u8 *e = sector + i * 32;
            if (e[0] == 0x00) return found;
            if (e[0] == 0xE5 || (e[11] & 0x08)) continue;
            fat83_to_str(e, out[found].name);
            out[found].is_dir        = (e[11] & 0x10) ? 1 : 0;
            out[found].size          = read_u32(e + 28);
            out[found].first_cluster = read_u16(e + 26);
            found++;
        }
    }
    return found;
}

/* find a root directory entry by name; fill entry_buf[32] and return its LBA+offset */
static int find_entry(const char *name, u8 *entry_buf, u32 *out_lba, u32 *out_idx)
{
    u8  sector[512];
    u32 lba  = root_start_lba;
    u32 left = root_entry_count;

    while (left > 0) {
        disk_read_sector(lba, sector);
        u32 per = 512 / 32;
        if (per > left) per = left;
        left -= per;

        for (u32 i = 0; i < per; i++) {
            u8 *e = sector + i * 32;
            if (e[0] == 0x00) return 0;
            if (e[0] == 0xE5 || (e[11] & 0x08)) continue;
            char fat_name[FAT12_NAME_MAX];
            fat83_to_str(e, fat_name);
            if (names_match(fat_name, name)) {
                memcpy(entry_buf, e, 32);
                *out_lba = lba;
                *out_idx = i;
                return 1;
            }
        }
        lba++;
    }
    return 0;
}

int fat12_exists(const char *path)
{
    if (!fat12_ready) return 0;
    if (*path == '/') path++;
    u8 entry[32]; u32 lba, idx;
    return find_entry(path, entry, &lba, &idx);
}

int fat12_read(const char *path, u8 *buf, u32 buf_size)
{
    if (!fat12_ready || !path || !buf) return 0;
    if (*path == '/') path++;

    u8 entry[32]; u32 lba, idx;
    if (!find_entry(path, entry, &lba, &idx)) return 0;

    u32 size    = read_u32(entry + 28);
    u16 cluster = read_u16(entry + 26);
    u32 written = 0;

    while (cluster >= 0x002 && cluster <= 0xFEF && written < buf_size) {
        u32 clba = data_start_lba + (u32)(cluster - 2) * sectors_per_cluster;
        for (u8 s = 0; s < sectors_per_cluster && written < buf_size; s++) {
            u8 sec[512];
            disk_read_sector(clba + s, sec);
            u32 to_copy = 512;
            if (written + to_copy > buf_size) to_copy = buf_size - written;
            if (written + to_copy > size)     to_copy = size - written;
            memcpy(buf + written, sec, to_copy);
            written += to_copy;
        }
        cluster = fat12_get(cluster);
    }
    return (int)written;
}

/* free an existing file's cluster chain */
static void free_chain(u16 cluster)
{
    while (cluster >= 0x002 && cluster <= 0xFEF) {
        u16 next = fat12_get(cluster);
        fat12_set(cluster, 0x000);
        cluster = next;
    }
}

/* find a free root directory slot; return its LBA + index, or 0 on failure */
static int alloc_root_entry(u32 *out_lba, u32 *out_idx)
{
    u8  sector[512];
    u32 lba  = root_start_lba;
    u32 left = root_entry_count;

    while (left > 0) {
        disk_read_sector(lba, sector);
        u32 per = 512 / 32;
        if (per > left) per = left;
        left -= per;

        for (u32 i = 0; i < per; i++) {
            u8 first = sector[i * 32];
            if (first == 0x00 || first == 0xE5) {
                *out_lba = lba;
                *out_idx = i;
                return 1;
            }
        }
        lba++;
    }
    return 0;
}

int fat12_write(const char *path, const u8 *buf, u32 size)
{
    if (!fat12_ready || !path || !buf) return 0;
    if (*path == '/') path++;

    /* delete existing file if present */
    u8  old_entry[32]; u32 old_lba, old_idx;
    if (find_entry(path, old_entry, &old_lba, &old_idx)) {
        u16 old_cluster = read_u16(old_entry + 26);
        free_chain(old_cluster);
        /* mark directory entry as deleted */
        u8 sector[512];
        disk_read_sector(old_lba, sector);
        sector[old_idx * 32] = 0xE5;
        disk_write_sector(old_lba, sector);
    }

    /* allocate clusters for the new data */
    u32 clusters_needed = (size + 511) / 512 / sectors_per_cluster;
    if (!clusters_needed) clusters_needed = 1;

    u16 first_cluster = 0, prev = 0;
    for (u32 c = 0; c < clusters_needed; c++) {
        u16 nc = fat12_alloc_cluster();
        if (!nc) {
            kprintf("[FAT12] disk full\n");
            if (first_cluster) free_chain(first_cluster);
            return 0;
        }
        fat12_set(nc, 0xFFF);   /* end of chain for now */
        if (prev) fat12_set(prev, nc);
        else first_cluster = nc;
        prev = nc;
    }

    /* write data into the allocated clusters */
    u16 cluster = first_cluster;
    u32 written = 0;
    while (cluster >= 0x002 && cluster <= 0xFEF && written < size) {
        u32 clba = data_start_lba + (u32)(cluster - 2) * sectors_per_cluster;
        for (u8 s = 0; s < sectors_per_cluster && written < size; s++) {
            u8 sec[512];
            memset(sec, 0, 512);
            u32 to_write = 512;
            if (written + to_write > size) to_write = size - written;
            memcpy(sec, buf + written, to_write);
            disk_write_sector(clba + s, sec);
            written += to_write;
        }
        cluster = fat12_get(cluster);
    }

    /* create directory entry */
    u32 new_lba, new_idx;
    if (!alloc_root_entry(&new_lba, &new_idx)) {
        kprintf("[FAT12] root dir full\n");
        free_chain(first_cluster);
        return 0;
    }

    u8 sector[512];
    disk_read_sector(new_lba, sector);
    u8 *e = sector + new_idx * 32;
    memset(e, 0, 32);
    str_to_fat83(path, e);
    e[11] = 0x20;   /* archive attribute */
    write_u16(e + 26, first_cluster);
    write_u32(e + 28, size);
    disk_write_sector(new_lba, sector);

    fat_flush();
    disk_cache_flush();
    return (int)written;
}

int fat12_delete(const char *path)
{
    if (!fat12_ready || !path) return 0;
    if (*path == '/') path++;

    u8 entry[32]; u32 lba, idx;
    if (!find_entry(path, entry, &lba, &idx)) return 0;

    u16 cluster = read_u16(entry + 26);
    free_chain(cluster);

    u8 sector[512];
    disk_read_sector(lba, sector);
    sector[idx * 32] = 0xE5;
    disk_write_sector(lba, sector);

    fat_flush();
    disk_cache_flush();
    return 1;
}