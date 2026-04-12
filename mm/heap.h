/* baSic_ - mm/heap.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * kernel heap,  kmalloc / kfree
 */

#ifndef HEAP_H
#define HEAP_H

#include "../include/types.h"

void  heap_init(void);
void *kmalloc(usize size);
void  kfree(void *ptr);

#endif