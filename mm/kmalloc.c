#include "kmalloc.h"
#include "../inc/types.h"
#include "../kernel_base/io.h"

#define KHEAP_START 0x100000   
#define KHEAP_SIZE  0x100000  

static u8* heap = (u8*)KHEAP_START;
static size_t used = 0;

void* kmalloc(size_t size) {
    if (used + size >= KHEAP_SIZE) {
        panic("Out of kernel heap memory!");
        return NULL;
    }
    void* ptr = heap + used;
    used += size;
    return ptr;
}

void kfree(void* ptr) {
    (void)ptr; 
}
