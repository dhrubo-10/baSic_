/*
 * baSic_
 *
 * The boot MBR (boot.asm) loads this image at physical address 0x8000.
 * stage2.asm must be the very first thing in the output binary so the
 * jump from the MBR lands at stage2_start.
 */

ENTRY(stage2_start)

SECTIONS
{
    /* Load address: stage2 + kernel live together starting at 0x8000 */
    . = 0x8000;

    /* text */
    .text :
    {
        /* stage2 must come first — it is the entry point */
        *(.text)
        *(.text.*)
    }

    /*  readonly data */
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* initialised data */
    .data :
    {
        *(.data)
        *(.data.*)
    }

    /*  zero-initialised data */
    .bss :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Discard debug info and other unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
