/* baSic_ - kernel/rtc.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * CMOS real-time clock driver interface
 */

#ifndef RTC_H
#define RTC_H

#include "../include/types.h"

typedef struct {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u8 year;    /* two-digit year from CMOS, e.g. 26 for 2026 */
} rtc_time_t;

void rtc_init(void);
void rtc_read(rtc_time_t *t);

#endif