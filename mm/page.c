#include <mm/page.h>
#include <kernel_b/system.h>
#include <kernel_b/string.h>

u32 *page_directory = (u32*)0x200000;      
u32 *first_page_table = (u32*)0x201000;    

static void load_page_directory(u32 *pd) {
    asm volatile("mov %0, %%cr3" : : "r"(pd));
}

static void enable_paging() {
    u32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void paging_init() {
    memset(page_directory, 0, 4096);
    memset(first_page_table, 0, 4096);

    for (u32 i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 4096) | PAGE_PRESENT | PAGE_WRITE;
    }

    page_directory[0] = ((u32)first_page_table) | PAGE_PRESENT | PAGE_WRITE;

    load_page_directory(page_directory);
    enable_paging();
}

void map_page(u32 virt, u32 phys, u32 flags) {
    u32 pd_i = virt >> 22;
    u32 pt_i = (virt >> 12) & 0x3FF;

    u32 *pt;

    if (page_directory[pd_i] & PAGE_PRESENT) {
        pt = (u32*)(page_directory[pd_i] & ~0xFFF);
    } else {
        pt = (u32*)kmalloc(4096);
        memset(pt, 0, 4096);
        page_directory[pd_i] = ((u32)pt) | PAGE_PRESENT | PAGE_WRITE;
    }

    pt[pt_i] = phys | flags;
}

u32 get_phys_addr(u32 virt) {
    u32 pd_i = virt >> 22;
    u32 pt_i = (virt >> 12) & 0x3FF;

    if (!(page_directory[pd_i] & PAGE_PRESENT))
        return 0;

    u32 *pt = (u32*)(page_directory[pd_i] & ~0xFFF);
    if (!(pt[pt_i] & PAGE_PRESENT))
        return 0;

    return (pt[pt_i] & ~0xFFF) + (virt & 0xFFF);
}
