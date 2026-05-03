/* baSic_ - kernel/pci.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * PCI config space enumeration .c
 * scans bus 0, slots 0-31, func 0
 * rn it's enough for QEMU which puts everything on bus 0. will work for real hardware later.
 */
#include "pci.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

#define PCI_ADDR  0xCF8
#define PCI_DATA  0xCFC

static pci_device_t pci_devs[PCI_MAX_DEVICES];
static int          pci_count = 0;

static inline void outl(u16 port, u32 val)
{
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u32 inl(u16 port)
{
    u32 val;
    __asm__ volatile ("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

u32 pci_read(u8 bus, u8 slot, u8 func, u8 offset)
{
    u32 addr = (u32)(1 << 31)
             | ((u32)bus   << 16)
             | ((u32)slot  << 11)
             | ((u32)func  <<  8)
             | ((u32)offset & 0xFC);
    outl(PCI_ADDR, addr);
    return inl(PCI_DATA);
}

void pci_write(u8 bus, u8 slot, u8 func, u8 offset, u32 val)
{
    u32 addr = (u32)(1 << 31)
             | ((u32)bus   << 16)
             | ((u32)slot  << 11)
             | ((u32)func  <<  8)
             | ((u32)offset & 0xFC);
    outl(PCI_ADDR, addr);
    outl(PCI_DATA, val);
}

int pci_init(void)
{
    pci_count = 0;
    memset(pci_devs, 0, sizeof(pci_devs));

    for (u8 slot = 0; slot < 32; slot++) {
        u32 id = pci_read(0, slot, 0, 0);
        u16 vendor = (u16)(id & 0xFFFF);
        u16 device = (u16)(id >> 16);

        if (vendor == 0xFFFF) continue;  /* empty slot here. */
        if (pci_count >= PCI_MAX_DEVICES) break;

        u32 cls = pci_read(0, slot, 0, 8);

        pci_devs[pci_count].bus      = 0;
        pci_devs[pci_count].slot     = slot;
        pci_devs[pci_count].func     = 0;
        pci_devs[pci_count].vendor   = vendor;
        pci_devs[pci_count].device   = device;
        pci_devs[pci_count].class    = (u8)((cls >> 24) & 0xFF);
        pci_devs[pci_count].subclass = (u8)((cls >> 16) & 0xFF);

        kprintf("[PCI] %x:%x class=%x:%x slot=%d\n",
                vendor, device,
                pci_devs[pci_count].class,
                pci_devs[pci_count].subclass,
                slot);
        pci_count++;
    }

    kprintf("[OK] PCI: %d devices found\n", pci_count);
    return pci_count;
}

pci_device_t *pci_find(u16 vendor, u16 device)
{
    for (int i = 0; i < pci_count; i++)
        if (pci_devs[i].vendor == vendor && pci_devs[i].device == device)
            return &pci_devs[i];
    return NULL;
}