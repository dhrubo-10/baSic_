/* baSic_ - mm/vmm.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * virtual memory manager
 * sets up a page directory, identity maps the first 8MB (kernel space),
 * then enables paging via CR0
 */

#include "vmm.h"
#include "pmm.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

/* page directory aligned to 4KB placed at a known physical address */
static pde_t page_dir[1024]  ALIGNED(4096);

static pte_t *get_table(u32 virt, u32 flags)
{
    u32 pdi = virt >> 22;
    pde_t entry = page_dir[pdi];

    if (entry & PAGE_PRESENT)
        return (pte_t *)(entry & ~0xFFF);

    /* allocate a new page table */
    u32 phys = pmm_alloc();
    if (!phys)
        return NULL;

    memset((void *)phys, 0, PAGE_SIZE);
    page_dir[pdi] = phys | flags | PAGE_PRESENT;
    return (pte_t *)phys;
}

void vmm_map(u32 virt, u32 phys, u32 flags)
{
    pte_t *table = get_table(virt, flags);
    if (!table)
        return;
    u32 pti = (virt >> 12) & 0x3FF;
    table[pti] = (phys & ~0xFFF) | flags | PAGE_PRESENT;

    /* flush TLB for this page */
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap(u32 virt)
{
    pte_t *table = get_table(virt, 0);
    if (!table) return;
    u32 pti = (virt >> 12) & 0x3FF;
    table[pti] = 0;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_init(void)
{
    memset(page_dir, 0, sizeof(page_dir));

    /* identity map first 8 MB — kernel lives here */
    for (u32 addr = 0; addr < 0x800000; addr += PAGE_SIZE)
        vmm_map(addr, addr, PAGE_WRITE);

    /* load page directory into CR3 */
    __asm__ volatile ("mov %0, %%cr3" : : "r"((u32)page_dir));

    /* enable paging: set bit 31 of CR0 */
    u32 cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));

    kprintf("[OK] VMM: paging enabled, 8MB identity mapped\n");
}