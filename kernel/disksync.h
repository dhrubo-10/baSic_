/* baSic_ - kernel/disksync.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ramfs: FAT12 sync utilities
 */

#ifndef DISKSYNC_H
#define DISKSYNC_H

#define ALIAS_MAX      16
#define ALIAS_NAME_MAX 16
#define ALIAS_CMD_MAX  64

typedef struct {
    char name[ALIAS_NAME_MAX];
    char cmd[ALIAS_CMD_MAX];
    int  used;
} alias_entry_t;

extern alias_entry_t alias_table[ALIAS_MAX];
const char *alias_resolve(const char *name);
const char *disksync_boot_path(void);
/* read /etc/init from disk and execute its commands at boot */
void disksync_run_init(void);

/* flush disk cache (called before halt/reboot) */
void disksync_flush(void);

#endif