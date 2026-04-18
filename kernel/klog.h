/* baSic_ - kernel/klog.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel ring buffer log, stores last N kernel messages
 */

#ifndef KLOG_H
#define KLOG_H

#define KLOG_LINES  64
#define KLOG_WIDTH  80

void klog_init(void);
void klog_write(const char *line);
void klog_dump(void);          /* print all stored lines to shell */

#endif