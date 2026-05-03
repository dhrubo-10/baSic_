/* baSic_ - kernel/pci.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * PCI bus enumeration.. config space access via ports 0xCF8/0xCFC!.
 */
#ifndef PCI_H
#define PCI_H
#include "../include/types.h"

typedef struct {
    u8  bus;
    u8  slot;
    u8  func;
    u16 vendor;
    u16 device;
    u8  class;
    u8  subclass;
} pci_device_t;

#define PCI_MAX_DEVICES  32

u32  pci_read(u8 bus, u8 slot, u8 func, u8 offset);
void pci_write(u8 bus, u8 slot, u8 func, u8 offset, u32 val);
int  pci_init(void);
pci_device_t *pci_find(u16 vendor, u16 device);

#endif