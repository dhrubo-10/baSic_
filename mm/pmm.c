/* baSic_ - mm/pmm.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * physical memory manager
 * bitmap: 1 bit per 4KB frame, 1 = used, 0 = free
 * bitmap lives at 1MB physical (0x100000), safe above realmode area
 */

#include "pmm.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

#define BITMAP_ADDR     0x100000    /* 1 MB — where bitmap lives */
#define KERNEL_END      0x200000    /* 2 MB — reserve everything below for kernel */

static u32  *bitmap      = (u32 *)BITMAP_ADDR;
static u32   total_frames = 0;
static u32   free_frames  = 0;
static u32   bitmap_words = 0;     /* number of u32s in bitmap */

static inline void bitmap_set(u32 frame)
{
    bitmap[frame / 32] |= (1u << (frame % 32));
}

static inline void bitmap_clear(u32 frame)
{
    bitmap[frame / 32] &= ~(1u << (frame % 32));
}

static inline int bitmap_test(u32 frame)
{
    return (bitmap[frame / 32] >> (frame % 32)) & 1;
}

void pmm_init(u32 mem_kb)
{
    total_frames = (mem_kb * 1024) / PMM_FRAME_SIZE;
    bitmap_words = (total_frames + 31) / 32;

    /* mark everything used to start */
    memset(bitmap, 0xFF, bitmap_words * 4);
    free_frames = 0;

    /* free frames above KERNEL_END */
    u32 first_free = KERNEL_END / PMM_FRAME_SIZE;
    for (u32 f = first_free; f < total_frames; f++) {
        bitmap_clear(f);
        free_frames++;
    }

    kprintf("[OK] PMM: %d MB total, %d frames free\n",
        mem_kb / 1024, free_frames);
}

u32 pmm_alloc(void)
{
    for (u32 i = 0; i < bitmap_words; i++) {
        if (bitmap[i] == 0xFFFFFFFF)
            continue;
        for (u32 bit = 0; bit < 32; bit++) {
            u32 frame = i * 32 + bit;
            if (frame >= total_frames)
                return 0;
            if (!bitmap_test(frame)) {
                bitmap_set(frame);
                free_frames--;
                return frame * PMM_FRAME_SIZE;
            }
        }
    }
    return 0;   //out of memory
}

void pmm_free(u32 addr)
{
    u32 frame = addr / PMM_FRAME_SIZE;
    if (frame >= total_frames)
        return;
    bitmap_clear(frame);
    free_frames++;
}

u32 pmm_free_frames(void)   { return free_frames;  }
u32 pmm_total_frames(void)  { return total_frames; }