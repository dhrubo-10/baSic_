/* baSic_ - kernel/serial.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * COM1 serial port driver — 115200 baud, 8N1
 * useful for debug output from QEMU: qemu ... -serial stdio
 */

#include "serial.h"
#include "../lib/kprintf.h"

#define COM1_BASE       0x3F8
#define COM1_DATA       (COM1_BASE + 0)
#define COM1_IER        (COM1_BASE + 1)   /* interrupt enable    */
#define COM1_FCR        (COM1_BASE + 2)   /* FIFO control        */
#define COM1_LCR        (COM1_BASE + 3)   /* line control        */
#define COM1_MCR        (COM1_BASE + 4)   /* modem control       */
#define COM1_LSR        (COM1_BASE + 5)   /* line status         */
#define COM1_DLAB_LO    (COM1_BASE + 0)   /* baud divisor low    */
#define COM1_DLAB_HI    (COM1_BASE + 1)   /* baud divisor high   */

#define LCR_8N1         0x03
#define LCR_DLAB        0x80
#define MCR_DTR_RTS     0x03
#define FCR_ENABLE      0xC7
#define LSR_THRE        0x20              /* transmit holding register empty */

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

static inline int serial_transmit_empty(void)
{
    return inb(COM1_LSR) & LSR_THRE;
}

void serial_init(void)
{
    outb(COM1_IER,     0x00);   /* disable interrupts         */
    outb(COM1_LCR,     LCR_DLAB);
    outb(COM1_DLAB_LO, 0x01);   /* divisor = 1 -> 115200 baud  */
    outb(COM1_DLAB_HI, 0x00);
    outb(COM1_LCR,     LCR_8N1);
    outb(COM1_FCR,     FCR_ENABLE);
    outb(COM1_MCR,     MCR_DTR_RTS);

    kprintf("[OK] serial: COM1 ready at 115200 baud\n");
}

void serial_putchar(char c)
{
    while (!serial_transmit_empty())
        ;
    outb(COM1_DATA, (u8)c);
}

void serial_print(const char *s)
{
    while (*s) {
        if (*s == '\n')
            serial_putchar('\r');
        serial_putchar(*s++);
    }
}