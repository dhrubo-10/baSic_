/* baSic_ - kernel/e1000.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * Intel e1000 NIC driver, which is QEMU default NIC
 * vendor 0x8086 device 0x100E
 */
#ifndef E1000_H
#define E1000_H
#include "../include/types.h"

#define E1000_VENDOR  0x8086
#define E1000_DEVICE  0x100E

#define E1000_NUM_TX  8
#define E1000_NUM_RX  8
#define E1000_BUF_SIZE 2048

int  e1000_init(void);
int  e1000_send(const u8 *data, u16 len);
int  e1000_recv(u8 *buf, u16 max_len);
void e1000_get_mac(u8 mac[6]);

#endif