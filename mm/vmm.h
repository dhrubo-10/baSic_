/* baSic_ - mm/vmm.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * virtual memory manager — x86 two level paging (4KB pages)
 */

#ifndef VMM_H
#define VMM_H

#include "../include/types.h"

#define PAGE_SIZE       4096
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_USER       (1 << 2)

typedef u32 pde_t;  
typedef u32 pte_t;  

void vmm_init(void);
void vmm_map(u32 virt, u32 phys, u32 flags);
void vmm_unmap(u32 virt);

#endif