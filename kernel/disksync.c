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

void disksync_flush(void)
{
    disk_cache_flush();
    kprintf("[OK] disksync: disk flushed\n");
}

void disksync_run_init(void)
{
    u8 buf[512];
    int n = fat12_read("INIT.CFG", buf, sizeof(buf) - 1);
    if (n <= 0) {
        kprintf("[disksync] no INIT.CFG on disk — skipping\n");
        return;
    }
    buf[n] = '\0';
    kprintf("[disksync] running INIT.CFG (%d bytes)\n", n);

    /* parse line by line — each line is a simple key=value or comment */
    char *line = (char *)buf;
    while (*line) {
        char *end = line;
        while (*end && *end != '\n') end++;
        char saved = *end;
        *end = '\0';

        /* skip comments and blank lines */
        if (line[0] && line[0] != '#') {
            /* handle: motd=<text> — write to /etc/motd in ramfs */
            if (!strncmp(line, "motd=", 5)) {
                vfs_node_t *etc = vfs_finddir(vfs_root(), "etc");
                if (etc) {
                    vfs_node_t *motd = vfs_finddir(etc, "motd");
                    if (!motd) motd = ramfs_mkfile(etc, "motd");
                    if (motd) {
                        const char *text = line + 5;
                        vfs_write(motd, 0, strlen(text), (u8 *)text);
                        kprintf("[disksync] motd set from INIT.CFG\n");
                    }
                }
            }
            /* handle: env=KEY=VALUE: set environment variable */
            else if (!strncmp(line, "env=", 4)) {
                extern int env_set(const char *key, const char *val);
                char *kv  = line + 4;
                char *eq  = kv;
                while (*eq && *eq != '=') eq++;
                if (*eq == '=') {
                    *eq = '\0';
                    env_set(kv, eq + 1);
                    *eq = '=';
                }
            }
        }

        *end = saved;
        line = (*end) ? end + 1 : end;
    }
}