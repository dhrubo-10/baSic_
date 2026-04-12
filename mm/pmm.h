/* baSic_ - mm/pmm.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * physical memory manager ~ bitmap allocator, 4KB frames
 */

#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define PMM_FRAME_SIZE  4096    /* 4 KB per frame */

void  pmm_init(u32 mem_kb);
void  pmm_free(u32 addr);
u32   pmm_alloc(void);          /* returns physical address or 0 on OOM */
u32   pmm_free_frames(void);
u32   pmm_total_frames(void);

#endif