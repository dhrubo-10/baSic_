/* baSic_ - kernel/e1000.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * Intel e1000 NIC: MMIO register access
 * QEMU emulates this at vendor=0x8086 device=0x100E
 * BAR0 is the MMIO base address
 */
#include "e1000.h"
#include "pci.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

/* e1000  -> reg offsets */
#define E1000_CTRL    0x0000
#define E1000_STATUS  0x0008
#define E1000_EERD    0x0014
#define E1000_ICR     0x00C0
#define E1000_IMS     0x00D0
#define E1000_RCTL    0x0100
#define E1000_TCTL    0x0400
#define E1000_RDBAL   0x2800
#define E1000_RDBAH   0x2804
#define E1000_RDLEN   0x2808
#define E1000_RDH     0x2810
#define E1000_RDT     0x2818
#define E1000_TDBAL   0x3800
#define E1000_TDBAH   0x3804
#define E1000_TDLEN   0x3808
#define E1000_TDH     0x3810
#define E1000_TDT     0x3818
#define E1000_RAL     0x5400
#define E1000_RAH     0x5404

/*ctrl  bits */
#define CTRL_RST      (1 << 26)
#define CTRL_SLU      (1 << 6)
#define RCTL_EN       (1 << 1)
#define RCTL_BAM      (1 << 15)
#define RCTL_BSIZE    (3 << 16)  
#define RCTL_SECRC    (1 << 26)
#define TCTL_EN       (1 << 1)
#define TCTL_PSP      (1 << 3)

typedef struct __attribute__((packed)) {
    u64 addr;
    u16 length;
    u8  cso;
    u8  cmd;
    u8  status;
    u8  css;
    u16 special;
} tx_desc_t;


typedef struct __attribute__((packed)) {
    u64 addr;
    u16 length;
    u16 checksum;
    u8  status;
    u8  errors;
    u16 special;
} rx_desc_t;

static u32 mmio_base = 0;
static u8  mac[6];

static tx_desc_t tx_ring[E1000_NUM_TX] __attribute__((aligned(16)));
static rx_desc_t rx_ring[E1000_NUM_RX] __attribute__((aligned(16)));
static u8        tx_buf[E1000_NUM_TX][E1000_BUF_SIZE];
static u8        rx_buf[E1000_NUM_RX][E1000_BUF_SIZE];

static u32 tx_tail = 0;
static u32 rx_tail = 0;

static inline u32 e1000_read(u32 reg)
{
    return *(volatile u32 *)(mmio_base + reg);
}

static inline void e1000_write(u32 reg, u32 val)
{
    *(volatile u32 *)(mmio_base + reg) = val;
}

static u16 eeprom_read(u8 addr)
{
    e1000_write(E1000_EERD, (u32)addr << 8 | 1);
    u32 val;
    int timeout = 10000;
    while (--timeout) {
        val = e1000_read(E1000_EERD);
        if (val & (1 << 4)) break;
    }
    return (u16)(val >> 16);
}

int e1000_init(void)
{
    pci_device_t *dev = pci_find(E1000_VENDOR, E1000_DEVICE);
    if (!dev) {
        kprintf("[e1000] not found on PCI bus\n");
        return 0;
    }

    /* read BAR0:  MMIO base */
    u32 bar0 = pci_read(dev->bus, dev->slot, dev->func, 0x10);
    mmio_base = bar0 & ~0xF;

    kprintf("[e1000] MMIO base: 0x%x\n", mmio_base);

    // reset */
    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | CTRL_RST);
    int i;
    for (i = 0; i < 100000; i++)
        __asm__ volatile ("nop");

    /* settikng link up */
    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | CTRL_SLU);

    /* read MAC from EEPROM */
    u16 w0 = eeprom_read(0);
    u16 w1 = eeprom_read(1);
    u16 w2 = eeprom_read(2);
    mac[0] = (u8)(w0 & 0xFF); mac[1] = (u8)(w0 >> 8);
    mac[2] = (u8)(w1 & 0xFF); mac[3] = (u8)(w1 >> 8);
    mac[4] = (u8)(w2 & 0xFF); mac[5] = (u8)(w2 >> 8);

    kprintf("[e1000] MAC: %x:%x:%x:%x:%x:%x\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* setup TX ring */
    memset(tx_ring, 0, sizeof(tx_ring));
    for (i = 0; i < E1000_NUM_TX; i++)
        tx_ring[i].addr = (u64)(u32)tx_buf[i];

    e1000_write(E1000_TDBAL, (u32)tx_ring);
    e1000_write(E1000_TDBAH, 0);
    e1000_write(E1000_TDLEN, E1000_NUM_TX * sizeof(tx_desc_t));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    e1000_write(E1000_TCTL, TCTL_EN | TCTL_PSP | (0x10 << 4) | (0x40 << 12));

    memset(rx_ring, 0, sizeof(rx_ring));
    for (i = 0; i < E1000_NUM_RX; i++) {
        rx_ring[i].addr   = (u64)(u32)rx_buf[i];
        rx_ring[i].status = 0;
    }

    e1000_write(E1000_RDBAL, (u32)rx_ring);
    e1000_write(E1000_RDBAH, 0);
    e1000_write(E1000_RDLEN, E1000_NUM_RX * sizeof(rx_desc_t));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, E1000_NUM_RX - 1);
    e1000_write(E1000_RCTL, RCTL_EN | RCTL_BAM | RCTL_SECRC);

    rx_tail = 0;
    tx_tail = 0;

    kprintf("[OK] e1000: NIC ready\n");
    return 1;
}

int e1000_send(const u8 *data, u16 len)
{
    if (!mmio_base || len > E1000_BUF_SIZE) return -1;

    tx_desc_t *desc = &tx_ring[tx_tail];
    memcpy(tx_buf[tx_tail], data, len);
    desc->length = len;
    desc->cmd    = 0x0B;  /* EOP | IFCS | RS */
    desc->status = 0;

    tx_tail = (tx_tail + 1) % E1000_NUM_TX;
    e1000_write(E1000_TDT, tx_tail);
    /* waiting for send. */
    int timeout = 100000;
    while (!(desc->status & 0xFF) && --timeout)
        __asm__ volatile ("nop");

    return (desc->status & 0xFF) ? (int)len : -1;
}

int e1000_recv(u8 *buf, u16 max_len)
{
    if (!mmio_base) return -1;

    rx_desc_t *desc = &rx_ring[rx_tail];
    if (!(desc->status & 0x01)) return 0;  
    
    u16 len = desc->length;
    if (len > max_len) len = max_len;
    memcpy(buf, rx_buf[rx_tail], len);

    desc->status = 0;
    e1000_write(E1000_RDT, rx_tail);
    rx_tail = (rx_tail + 1) % E1000_NUM_RX;

    return (int)len;
}

void e1000_get_mac(u8 out[6])
{
    memcpy(out, mac, 6);
}