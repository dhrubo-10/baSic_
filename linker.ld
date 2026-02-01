ENTRY(_start)

SECTIONS
{
    . = 0x10000;  /* Kernel loads at 1MB */
    
    .text : {
        *(.multiboot)
        *(.text)
    }
    
    .rodata : {
        *(.rodata)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        *(COMMON)
        *(.bss)
    }
    
    /* End of kernel */
    kernel_end = .;
    
    /* Kernel stack */
    . = ALIGN(16);
    . += 0x4000;
    stack_top = .;
}