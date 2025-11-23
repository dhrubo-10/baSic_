#ifndef KMALLOC_H
#define KMALLOC_H

#include "../inc/types.h"

void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
