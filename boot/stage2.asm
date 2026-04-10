; basic_ - Stage 2
; Loaded at 0x8000. Sets up GDT, switches to 32-bit protected mode,
; then calls kmain() in the C kernel.

// fixed..
[BITS 16]

extern kmain

section .text

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    lgdt [gdt_descriptor]

    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    jmp 0x08:flush

[BITS 32]
flush:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9F000

    ; sanity check — write green 'S' at top-left
    mov dword [0xB8000], 0x0A530A53

    call kmain

.hang:
    cli
    hlt
    jmp .hang

; GDT
; Must live in same section so the linker patches the address correctly. *** 

gdt_start:
    dq 0x0000000000000000     ; null
    dq 0x00CF9A000000FFFF     ; 0x08 code: base=0, limit=4GB, ring0, 32-bit
    dq 0x00CF92000000FFFF     ; 0x10 data: base=0, limit=4GB, ring0, 32-bit
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start