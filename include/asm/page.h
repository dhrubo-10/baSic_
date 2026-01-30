#ifndef _ASM_PAGE_H
#define _ASM_PAGE_H

#include <kernel_b/types.h>

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE - 1))

#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define PAGE_ALIGN_DOWN(addr) ((addr) & PAGE_MASK)

#define __PAGE_OFFSET   0xC0000000

#define __pa(x)         ((unsigned long)(x) - __PAGE_OFFSET)
#define __va(x)         ((void *)((unsigned long)(x) + __PAGE_OFFSET))

#define PFN_ALIGN(x)    (((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)
#define PFN_UP(x)       (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)     ((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)     ((x) << PAGE_SHIFT)
#define PHYS_PFN(x)     ((x) >> PAGE_SHIFT)

typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;

#define pte_val(x)      ((x).pte)
#define pmd_val(x)      ((x).pmd)
#define pgd_val(x)      ((x).pgd)
#define pgprot_val(x)   ((x).pgprot)

#define __pte(x)        ((pte_t) { (x) })
#define __pmd(x)        ((pmd_t) { (x) })
#define __pgd(x)        ((pgd_t) { (x) })
#define __pgprot(x)     ((pgprot_t) { (x) })

#define PAGE_NONE       __pgprot(0)
#define PAGE_READONLY   __pgprot(1)
#define PAGE_SHARED     __pgprot(3)
#define PAGE_COPY       __pgprot(5)
#define PAGE_KERNEL     __pgprot(7)

#define _PAGE_PRESENT   0x001
#define _PAGE_RW        0x002
#define _PAGE_USER      0x004
#define _PAGE_PWT       0x008
#define _PAGE_PCD       0x010
#define _PAGE_ACCESSED  0x020
#define _PAGE_DIRTY     0x040
#define _PAGE_PSE       0x080
#define _PAGE_GLOBAL    0x100

#define PAGE_FAULT_PROT (PROT_READ | PROT_WRITE)

#define VM_READ         0x00000001
#define VM_WRITE        0x00000002
#define VM_EXEC         0x00000004
#define VM_SHARED       0x00000008
#define VM_MAYREAD      0x00000010
#define VM_MAYWRITE     0x00000020
#define VM_MAYEXEC      0x00000040
#define VM_MAYSHARE     0x00000080
#define VM_GROWSDOWN    0x00000100
#define VM_GROWSUP      0x00000200
#define VM_SHM          0x00000400
#define VM_DENYWRITE    0x00000800
#define VM_EXECUTABLE   0x00001000
#define VM_LOCKED       0x00002000
#define VM_IO           0x00004000
#define VM_SEQ_READ     0x00008000
#define VM_RAND_READ    0x00010000
#define VM_DONTCOPY     0x00020000
#define VM_DONTEXPAND   0x00040000
#define VM_RESERVED     0x00080000
#define VM_ACCOUNT      0x00100000
#define VM_HUGETLB      0x00400000
#define VM_NONLINEAR    0x00800000

struct page {
    unsigned long flags;
    atomic_t _count;
    atomic_t _mapcount;
    unsigned long private;
    struct address_space *mapping;
    pgoff_t index;
    struct list_head lru;
    void *virtual;
};

#define PAGE_FLAG_RESERVED   0
#define PAGE_FLAG_LOCKED     1
#define PAGE_FLAG_ERROR      2
#define PAGE_FLAG_REFERENCED 3
#define PAGE_FLAG_UPTODATE   4
#define PAGE_FLAG_DIRTY      5
#define PAGE_FLAG_LRU        6
#define PAGE_FLAG_ACTIVE     7
#define PAGE_FLAG_SLAB       8
#define PAGE_FLAG_WRITEBACK  9
#define PAGE_FLAG_RECLAIM    10
#define PAGE_FLAG_BUDDY      11
#define PAGE_FLAG_HIGHMEM    12
#define PAGE_FLAG_KMEM       13

#define PageLocked(page)     test_bit(PAGE_FLAG_LOCKED, &(page)->flags)
#define PageError(page)      test_bit(PAGE_FLAG_ERROR, &(page)->flags)
#define PageReferenced(page) test_bit(PAGE_FLAG_REFERENCED, &(page)->flags)
#define PageUptodate(page)   test_bit(PAGE_FLAG_UPTODATE, &(page)->flags)
#define PageDirty(page)      test_bit(PAGE_FLAG_DIRTY, &(page)->flags)
#define PageLRU(page)        test_bit(PAGE_FLAG_LRU, &(page)->flags)
#define PageActive(page)     test_bit(PAGE_FLAG_ACTIVE, &(page)->flags)
#define PageSlab(page)       test_bit(PAGE_FLAG_SLAB, &(page)->flags)
#define PageWriteback(page)  test_bit(PAGE_FLAG_WRITEBACK, &(page)->flags)
#define PageReclaim(page)    test_bit(PAGE_FLAG_RECLAIM, &(page)->flags)
#define PageBuddy(page)      test_bit(PAGE_FLAG_BUDDY, &(page)->flags)
#define PageHighMem(page)    test_bit(PAGE_FLAG_HIGHMEM, &(page)->flags)

#define SetPageLocked(page)      set_bit(PAGE_FLAG_LOCKED, &(page)->flags)
#define ClearPageLocked(page)    clear_bit(PAGE_FLAG_LOCKED, &(page)->flags)
#define SetPageError(page)       set_bit(PAGE_FLAG_ERROR, &(page)->flags)
#define ClearPageError(page)     clear_bit(PAGE_FLAG_ERROR, &(page)->flags)
#define SetPageUptodate(page)    set_bit(PAGE_FLAG_UPTODATE, &(page)->flags)
#define ClearPageUptodate(page)  clear_bit(PAGE_FLAG_UPTODATE, &(page)->flags)
#define SetPageDirty(page)       set_bit(PAGE_FLAG_DIRTY, &(page)->flags)
#define ClearPageDirty(page)     clear_bit(PAGE_FLAG_DIRTY, &(page)->flags)

#define PageCount(page)      atomic_read(&(page)->_count)
#define page_count(page)     PageCount(page)
#define set_page_count(page, v) atomic_set(&(page)->_count, v)
#define page_mapcount(page)  atomic_read(&(page)->_mapcount)

#define put_page(page) ({ \
    if (page) { \
        if (atomic_dec_and_test(&(page)->_count)) \
            __free_page(page); \
    } \
})

#define get_page(page) ({ \
    if (page) atomic_inc(&(page)->_count); \
    page; \
})

#define virt_to_page(addr)   (mem_map + (((unsigned long)(addr) - __PAGE_OFFSET) >> PAGE_SHIFT))
#define page_to_virt(page)   ((void *)(((page) - mem_map) << PAGE_SHIFT) + __PAGE_OFFSET)

#define virt_to_pfn(addr)    (PFN_DOWN(__pa(addr)))
#define pfn_to_virt(pfn)     (__va(PFN_PHYS(pfn)))

#define offset_in_page(p)    ((unsigned long)(p) & (PAGE_SIZE - 1))

#define ARCH_PFN_OFFSET      (__PAGE_OFFSET >> PAGE_SHIFT)

#define MAX_PHYSMEM_BITS     36
#define MAX_PHYSMEM_RANGES   8

struct mem_range {
    unsigned long start;
    unsigned long end;
    unsigned long type;
};

extern struct mem_range physmem_map[MAX_PHYSMEM_RANGES];
extern int physmem_map_nr;

void __init bootmem_init(void);
void __init paging_init(void);
void __init zone_sizes_init(void);

unsigned long get_num_physpages(void);
unsigned long get_total_highmem_pages(void);

#define HAVE_ARCH_ALLOC_PAGE
#define HAVE_ARCH_FREE_PAGE

struct page *alloc_page(gfp_t gfp_mask);
void free_page(struct page *page);

#endif 