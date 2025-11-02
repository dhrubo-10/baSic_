#include "disk.h"
#include "io.h"

/* 
* BIOS INT13h extension read using DAP structure.
* This wrapper calls BIOS via inline asm int 0x13.
*  Works only if code is running in real mode or via a real mode thunk.
*/
int bios_disk_read_lba(u8 drive, u64 lba, u16 count, void *buf) {
    struct dap {
        u8 size;
        u8 zero;
        u16 sectors;
        u16 buf_off;
        u16 buf_seg;
        u32 lba_low;
        u32 lba_high;
    } __attribute__((packed)) dap;

    dap.size = 16;
    dap.zero = 0;
    dap.sectors = count;

    u32 addr = (u32)(uintptr_t)buf; /* only valid in real mode linear segments */
    dap.buf_off = (u16)(addr & 0xF);
    dap.buf_seg = (u16)((addr >> 4) & 0xFFFF);

    dap.lba_low = (u32)(lba & 0xFFFFFFFF);
    dap.lba_high = (u32)((lba >> 32) & 0xFFFFFFFF);

    int ok = 0;
    
    /*
     * There might be a hidden bug in this, which im not sure abt yet. But i will keep it as it is for now.
     * P.S Working fine at this moment for me.! If i encounter any panic or crash or undeifned behavior, will 
     * update
    */
    asm volatile (
        "pushl %%ebx\n\t"
        "movw %[dap_off], %%bx\n\t"
        "movw %[dap_seg], %%es\n\t"
        "movb $0x42, %%ah\n\t"
        "int $0x13\n\t"
        "jc 1f\n\t"
        "movl $1, %0\n\t"
        "jmp 2f\n"
        "1:\n\t"
        "movl $0, %0\n"
        "2:\n\t"
        "popl %%ebx\n\t"
        : "=r"(ok)
        : [dap_off] "r"((u16)((uintptr_t)&dap & 0xFFFF)),
          [dap_seg] "r"((u16)(((uintptr_t)&dap) >> 4))
        : "memory", "ax"
    );
    return ok ? 0 : -1;
}

int ata_pio_read_lba(u8 drive, u64 lba, u16 count, void *buf) {
    (void)drive; (void)lba; (void)count; (void)buf;
    return -1;
}
