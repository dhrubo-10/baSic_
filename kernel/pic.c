/* baSic_ - kernel/pic.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * 8259 PIC remapping: moves IRQ vectors above CPU exceptions
 */

#include "pic.h"

/* I/O port addresses */
#define PIC_MASTER_CMD   0x20
#define PIC_MASTER_DATA  0x21
#define PIC_SLAVE_CMD    0xA0
#define PIC_SLAVE_DATA   0xA1

/* initialization control words */
#define PIC_ICW1_INIT    0x11   /* start init sequence, expect ICW4 */
#define PIC_ICW4_8086    0x01   /* 8086 mode */

#define PIC_EOI          0x20   /* end-of-interrupt command */

/* write a byte to an I/O port */
static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* read a byte from an I/O port */
static inline u8 inb(u16 port)
{
    u8 val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* short I/O delay .. give old hardware time to respond */
static inline void io_wait(void)
{
    outb(0x80, 0x00);   /* port 0x80 is a POST diagnostic port, safe to write */
}

void pic_init(void)
{
    /* save current masks so we can restore them after remapping */
    u8 master_mask = inb(PIC_MASTER_DATA);
    u8 slave_mask  = inb(PIC_SLAVE_DATA);

    /* ICW1 — start initialization */
    outb(PIC_MASTER_CMD,  PIC_ICW1_INIT); io_wait();
    outb(PIC_SLAVE_CMD,   PIC_ICW1_INIT); io_wait();

    /* ICW2 -> set vector offsets */
    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET); io_wait();   /* IRQ0-7  -> vectors 32-39 */
    outb(PIC_SLAVE_DATA,  PIC_SLAVE_OFFSET);  io_wait();   /* IRQ8-15 -> vectors 40-47 */

    /* ICW3 -> master/slave wiring */
    outb(PIC_MASTER_DATA, 0x04); io_wait();   /* master: slave on IRQ2 (bit 2) */
    outb(PIC_SLAVE_DATA,  0x02); io_wait();   /* slave: cascade identity = 2    */

    /* ICW4 -> set 8086 mode */
    outb(PIC_MASTER_DATA, PIC_ICW4_8086); io_wait();
    outb(PIC_SLAVE_DATA,  PIC_ICW4_8086); io_wait();

    /* restore masks */
    outb(PIC_MASTER_DATA, master_mask);
    outb(PIC_SLAVE_DATA,  slave_mask);
}

/* send end of interrupt so PIC accepts next IRQ */
void pic_send_eoi(u8 irq)
{
    if (irq >= 8)
        outb(PIC_SLAVE_CMD, PIC_EOI);   /* slave must be told first */
    outb(PIC_MASTER_CMD, PIC_EOI);
}

/* mask (disable) an individual IRQ line */
void pic_mask_irq(u8 irq)
{
    u16 port;
    u8  bit;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
        bit  = irq;
    } else {
        port = PIC_SLAVE_DATA;
        bit  = irq - 8;
    }
    outb(port, inb(port) | (1 << bit));
}

/* unmask (enable) an individual IRQ line */
void pic_unmask_irq(u8 irq)
{
    u16 port;
    u8  bit;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
        bit  = irq;
    } else {
        port = PIC_SLAVE_DATA;
        bit  = irq - 8;
    }
    outb(port, inb(port) & ~(1 << bit));
}