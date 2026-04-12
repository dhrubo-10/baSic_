; baSic_ - kernel/irq_stubs.asm
; Copyright (C) 2026 Dhrubo
; GPL v2 — see LICENSE
;
; hardware IRQ stubs for IRQ 0-15 (vectors 32-47 after PIC remap)

[BITS 32]

extern irq_handler      ; defined in irq.c

section .text

%macro IRQ_STUB 2
global irq%1
irq%1:
    cli
    push dword 0        ; dummy error code
    push dword %2       ; vector number (32 + IRQ number)
    jmp  irq_common
%endmacro

IRQ_STUB  0,  32    ; timer
IRQ_STUB  1,  33    ; keyboard
IRQ_STUB  2,  34    ; cascade (slave PIC)
IRQ_STUB  3,  35    ; COM2
IRQ_STUB  4,  36    ; COM1
IRQ_STUB  5,  37    ; LPT2
IRQ_STUB  6,  38    ; floppy
IRQ_STUB  7,  39    ; LPT1 / spurious
IRQ_STUB  8,  40    ; CMOS RTC
IRQ_STUB  9,  41    ; free
IRQ_STUB 10,  42    ; free
IRQ_STUB 11,  43    ; free
IRQ_STUB 12,  44    ; PS/2 mouse
IRQ_STUB 13,  45    ; FPU
IRQ_STUB 14,  46    ; primary ATA
IRQ_STUB 15,  47    ; secondary ATA

irq_common:
    pusha               ; save all general-purpose registers

    push esp            ; pass pointer to register struct to C handler
    call irq_handler
    add  esp, 4

    popa
    add  esp, 8         ; pop vector number and dummy error code
    sti
    iret