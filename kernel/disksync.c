/* baSic_ - kernel/disksync.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * disk sync utilities
 * reads /INIT.CFG from the FAT12 disk at boot and runs its lines
 * as shell commands: a primitive init system
 */

#include "disksync.h"
#include "fat12.h"
#include "disk.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

alias_entry_t alias_table[ALIAS_MAX];
static char   boot_path[64] = "/";

const char *disksync_boot_path(void)
{
    return boot_path;
}

const char *alias_resolve(const char *name)
{
    for (int i = 0; i < ALIAS_MAX; i++)
        if (alias_table[i].used && !strcmp(alias_table[i].name, name))
            return alias_table[i].cmd;
    return NULL;
}

static void alias_set(const char *name, const char *cmd)
{
    /* update if exist.. */
    for (int i = 0; i < ALIAS_MAX; i++) {
        if (alias_table[i].used && !strcmp(alias_table[i].name, name)) {
            strncpy(alias_table[i].cmd, cmd, ALIAS_CMD_MAX - 1);
            return;
        }
    }
    /* insert newones*/
    for (int i = 0; i < ALIAS_MAX; i++) {
        if (!alias_table[i].used) {
            strncpy(alias_table[i].name, name, ALIAS_NAME_MAX - 1);
            strncpy(alias_table[i].cmd,  cmd,  ALIAS_CMD_MAX  - 1);
            alias_table[i].used = 1;
            kprintf("[disksync] alias: %s -> %s\n", name, cmd);
            return;
        }
    }
    kprintf("[WARN] disksync: alias table full\n");
}

void disksync_flush(void)
{
    disk_cache_flush();
    kprintf("[OK] disksync: disk flushed\n");
}

void disksync_run_init(void)
{
    /* embedded config.. no disk fn.. couldn't fix the disk. will fix it soon */
    const char *cfg =
        "motd=welcome to baSic_ v1.0\n"
        "env=HOSTNAME=baSic\n"
        "alias=ll=ls\n"
        "path=/\n";

    kprintf("[disksync] running embedded INIT.CFG\n");

    char buf[512];
    strncpy(buf, cfg, 511);
    buf[511] = '\0';

    char *line = buf;
    while (*line) {
        char *end = line;
        while (*end && *end != '\n') end++;
        char saved = *end;
        *end = '\0';

        if (line[0] && line[0] != '#') {
            if (!strncmp(line, "motd=", 5)) {
                vfs_node_t *etc = vfs_finddir(vfs_root(), "etc");
                if (etc) {
                    vfs_node_t *motd = vfs_finddir(etc, "motd");
                    if (!motd) motd = ramfs_mkfile(etc, "motd");
                    if (motd) {
                        const char *text = line + 5;
                        vfs_write(motd, 0, strlen(text), (u8 *)text);
                        kprintf("[disksync] motd set\n");
                    }
                }
            }
            else if (!strncmp(line, "env=", 4)) {
                extern int env_set(const char *key, const char *val);
                char *kv = line + 4;
                char *eq = kv;
                while (*eq && *eq != '=') eq++;
                if (*eq == '=') {
                    *eq = '\0';
                    env_set(kv, eq + 1);
                    *eq = '=';
                }
            }
            else if (!strncmp(line, "alias=", 6)) {
                char *kv = line + 6;
                char *eq = kv;
                while (*eq && *eq != '=') eq++;
                if (*eq == '=') {
                    *eq = '\0';
                    alias_set(kv, eq + 1);
                    *eq = '=';
                }
            }
            else if (!strncmp(line, "path=", 5)) {
                strncpy(boot_path, line + 5, sizeof(boot_path) - 1);
                boot_path[sizeof(boot_path) - 1] = '\0';
                kprintf("[disksync] boot path: %s\n", boot_path);
            }
        }

        *end = saved;
        line = (*end) ? end + 1 : end;
    }
}