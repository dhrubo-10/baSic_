/* baSic_ - kernel/disksync.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ramfs: FAT12 sync utilities
 */

#ifndef DISKSYNC_H
#define DISKSYNC_H

/* read /etc/init from disk and execute its commands at boot */
void disksync_run_init(void);

/* flush disk cache (called before halt/reboot) */
void disksync_flush(void);

#endif