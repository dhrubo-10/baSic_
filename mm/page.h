#ifndef PAGE_H
#define PAGE_H

#include <kernel_b/types.h>

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

void paging_init();
void map_page(u32 virt, u32 phys, u32 flags);
u32  get_phys_addr(u32 virt);

extern u32 *page_directory;

#endif
