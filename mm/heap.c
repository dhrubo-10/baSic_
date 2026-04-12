/* baSic_ - mm/heap.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel heap allocator
 * simple free-list allocator — each block has a header tracking size + free flag
 * heap lives at 3MB physical, grows upward, max 1MB
 */

#include "heap.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

#define HEAP_START  0x300000    /* 3 MB */
#define HEAP_SIZE   0x100000    /* 1 MB */
#define HEAP_MAGIC  0xB451C000  /* sanity check value */

typedef struct block_hdr {
    u32              magic;
    usize            size;      /* usable bytes after header */
    u8               free;
    struct block_hdr *next;
} block_hdr_t;

static block_hdr_t *heap_head = NULL;

void heap_init(void)
{
    heap_head = (block_hdr_t *)HEAP_START;
    heap_head->magic = HEAP_MAGIC;
    heap_head->size  = HEAP_SIZE - sizeof(block_hdr_t);
    heap_head->free  = 1;
    heap_head->next  = NULL;

    kprintf("[OK] heap: %d KB at %x\n", HEAP_SIZE / 1024, HEAP_START);
}

static void merge_free_blocks(void)
{
    block_hdr_t *cur = heap_head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += sizeof(block_hdr_t) + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

void *kmalloc(usize size)
{
    if (!size) return NULL;

    /* align size to 8 bytes */
    size = (size + 7) & ~7u;

    block_hdr_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            /* split block if there's enough room for a new header + at least 8 bytes */
            if (cur->size >= size + sizeof(block_hdr_t) + 8) {
                block_hdr_t *split = (block_hdr_t *)((u8 *)(cur + 1) + size);
                split->magic = HEAP_MAGIC;
                split->size  = cur->size - size - sizeof(block_hdr_t);
                split->free  = 1;
                split->next  = cur->next;
                cur->next    = split;
                cur->size    = size;
            }
            cur->free = 0;
            return (void *)(cur + 1);
        }
        cur = cur->next;
    }

    kprintf("[ERR] kmalloc: out of heap memory\n");
    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr) return;

    block_hdr_t *hdr = (block_hdr_t *)ptr - 1;
    if (hdr->magic != HEAP_MAGIC) {
        kprintf("[ERR] kfree: bad pointer %x\n", (u32)ptr);
        return;
    }

    hdr->free = 1;
    merge_free_blocks();
}