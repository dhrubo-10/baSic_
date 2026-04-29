/* baSic_ - kernel/ata.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ATA PIO mode driver :- primary bus (0x1F0), master drive (0xA0)
 * uses 28-bit LBA addressing, polling (no DMA, no IRQ)
 */

#include "ata.h"
#include "serial.h"
#include "../lib/kprintf.h"

#define ATA_DATA         0x1F0
#define ATA_ERR          0x1F1
#define ATA_SECCOUNT     0x1F2
#define ATA_LBA_LO       0x1F3
#define ATA_LBA_MID      0x1F4
#define ATA_LBA_HI       0x1F5
#define ATA_DRIVE        0x1F6
#define ATA_CMD          0x1F7
#define ATA_STATUS       0x1F7
#define ATA_ALT_STATUS   0x3F6

#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_CACHE_FLUSH   0xE7

#define SR_BSY  0x80
#define SR_DRDY 0x40
#define SR_DRQ  0x08
#define SR_ERR  0x01
#define SR_DF   0x20

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

static inline u16 inw(u16 port)
{
    u16 val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outw(u16 port, u16 val)
{
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static void ata_delay(void)
{
    inb(ATA_ALT_STATUS); inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS); inb(ATA_ALT_STATUS);
}

static int ata_wait_ready(void)
{
    int timeout = 100000;
    while (--timeout) {
        u8 s = inb(ATA_STATUS);
        if (!(s & SR_BSY)) {
            if (s & (SR_ERR | SR_DF)) return 0;
            return 1;
        }
    }
    return 0;
}

static int ata_wait_drq(void)
{
    int timeout = 100000;
    while (--timeout) {
        u8 s = inb(ATA_STATUS);
        if (s & SR_ERR) return 0;
        if (s & SR_DRQ) return 1;
    }
    return 0;
}

int ata_init(void)
{
    outb(ATA_ALT_STATUS, 0x04);
    ata_delay();
    outb(ATA_ALT_STATUS, 0x00);
    ata_delay();

    outb(ATA_DRIVE, 0xA0);
    ata_delay();

    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LO,   0);
    outb(ATA_LBA_MID,  0);
    outb(ATA_LBA_HI,   0);
    outb(ATA_CMD, ATA_CMD_IDENTIFY);
    ata_delay();

    u8 status = inb(ATA_STATUS);
    if (status == 0) {
        kprintf("[ATA] no drive detected\n");
        return 0;
    }

    if (!ata_wait_ready()) {
        kprintf("[ATA] drive not ready\n");
        return 0;
    }

    for (int i = 0; i < 256; i++)
        inw(ATA_DATA);

    kprintf("[OK] ATA: primary master detected\n");
    return 1;
}

static void ata_setup_lba(u32 lba, u8 count)
{
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_ERR,      0x00);
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LO,   (u8)(lba & 0xFF));
    outb(ATA_LBA_MID,  (u8)((lba >> 8)  & 0xFF));
    outb(ATA_LBA_HI,   (u8)((lba >> 16) & 0xFF));
}

int ata_read(u32 lba, u8 count, u8 *buf)
{
    ata_setup_lba(lba, count);
    outb(ATA_CMD, ATA_CMD_READ_SECTORS);
    ata_delay();

    for (int s = 0; s < count; s++) {
        if (!ata_wait_drq()) return 0;
        for (int i = 0; i < 256; i++) {
            u16 data = inw(ATA_DATA);
            buf[s * 512 + i * 2]     = (u8)(data & 0xFF);
            buf[s * 512 + i * 2 + 1] = (u8)(data >> 8);
        }
    }
    return 1;
}

int ata_write(u32 lba, u8 count, const u8 *buf)
{
    ata_setup_lba(lba, count);
    outb(ATA_CMD, ATA_CMD_WRITE_SECTORS);
    ata_delay();

    for (int s = 0; s < count; s++) {
        if (!ata_wait_drq()) return 0;
        for (int i = 0; i < 256; i++) {
            u16 data = (u16)buf[s * 512 + i * 2] |
                       ((u16)buf[s * 512 + i * 2 + 1] << 8);
            outw(ATA_DATA, data);
        }
    }
    outb(ATA_CMD, ATA_CMD_CACHE_FLUSH);
    return ata_wait_ready();
}