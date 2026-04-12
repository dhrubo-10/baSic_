/* baSic_ - kernel/rtc.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * CMOS RTC driver
 * reads real time from ports 0x70 (index) / 0x71 (data)
 * handles BCD → binary conversion
 */

#include "rtc.h"

#define CMOS_INDEX  0x70
#define CMOS_DATA   0x71

/* CMOS register indices */
#define RTC_REG_SECOND  0x00
#define RTC_REG_MINUTE  0x02
#define RTC_REG_HOUR    0x04
#define RTC_REG_DAY     0x07
#define RTC_REG_MONTH   0x08
#define RTC_REG_YEAR    0x09
#define RTC_REG_STATUS_B 0x0B   /* status register B: BCD vs binary mode */

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* read one CMOS register */
static u8 cmos_read(u8 reg)
{
    outb(CMOS_INDEX, reg);
    return inb(CMOS_DATA);
}

/* returns 1 if an RTC update is in progress — must not read while updating */
static int rtc_updating(void)
{
    outb(CMOS_INDEX, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

/* BCD to binary: 0x26 → 26 */
static inline u8 bcd2bin(u8 bcd)
{
    return (u8)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

void rtc_init(void)
{
    /* nothing to configure — BIOS sets up RTC for us */
    /* we just read it */
}

void rtc_read(rtc_time_t *t)
{
    u8 status_b;
    u8 sec, min, hr, day, mon, yr;

    /* wait until no update in progress */
    while (rtc_updating())
        ;

    /* read all registers in one go */
    sec = cmos_read(RTC_REG_SECOND);
    min = cmos_read(RTC_REG_MINUTE);
    hr  = cmos_read(RTC_REG_HOUR);
    day = cmos_read(RTC_REG_DAY);
    mon = cmos_read(RTC_REG_MONTH);
    yr  = cmos_read(RTC_REG_YEAR);

    /* read again to confirm no update happened mid-read */
    while (rtc_updating())
        ;

    u8 sec2 = cmos_read(RTC_REG_SECOND);
    if (sec2 != sec) {
        /* update happened — read a clean set */
        sec = cmos_read(RTC_REG_SECOND);
        min = cmos_read(RTC_REG_MINUTE);
        hr  = cmos_read(RTC_REG_HOUR);
        day = cmos_read(RTC_REG_DAY);
        mon = cmos_read(RTC_REG_MONTH);
        yr  = cmos_read(RTC_REG_YEAR);
    }

    /* status B bit 2: 1 = binary mode, 0 = BCD mode */
    status_b = cmos_read(RTC_REG_STATUS_B);
    if (!(status_b & 0x04)) {
        /* BCD mode — convert everything */
        sec = bcd2bin(sec);
        min = bcd2bin(min);
        hr  = bcd2bin(hr);
        day = bcd2bin(day);
        mon = bcd2bin(mon);
        yr  = bcd2bin(yr);
    }

    t->second = sec;
    t->minute = min;
    t->hour   = hr;
    t->day    = day;
    t->month  = mon;
    t->year   = yr;
}