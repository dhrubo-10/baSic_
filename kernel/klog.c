/* baSic_ - kernel/klog.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * circular ring buffer that stores the last KLOG_LINES kernel log messages
 * klog_dump() lets the shell print them with the 'dmesg' command
 */

#include "klog.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static char log_buf[KLOG_LINES][KLOG_WIDTH];
static int  log_head  = 0;   /* next slot to write */
static int  log_count = 0;   /* total lines written (capped at KLOG_LINES) */

void klog_init(void)
{
    memset(log_buf, 0, sizeof(log_buf));
    log_head  = 0;
    log_count = 0;
}

void klog_write(const char *line)
{
    strncpy(log_buf[log_head], line, KLOG_WIDTH - 1);
    log_buf[log_head][KLOG_WIDTH - 1] = '\0';
    log_head = (log_head + 1) % KLOG_LINES;
    if (log_count < KLOG_LINES) log_count++;
}

void klog_dump(void)
{
    int start = (log_count < KLOG_LINES) ? 0 : log_head;
    for (int i = 0; i < log_count; i++) {
        int idx = (start + i) % KLOG_LINES;
        kprintf("%s\n", log_buf[idx]);
    }
}