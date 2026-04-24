; basic_ - Stage 2
; Loaded at 0x8000. Sets up GDT, switches to 32-bit protected mode,
; then calls kmain() in the C kernel.

; fixed
[BITS 16]

global stage2_start
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
    mov dword [0xB8000], 0x0A4B0A4F 

    call kmain

.hang:
    cli
    hlt
    jmp .hang

; GDT 
; Must live in same section so the linker patches the address correctly. 

gdt_start:
    ; took me 18 hours to fix this fucking thing 
    dq 0              ; 0: null
    dq 0x00CF9A000000FFFF  ; 1: kernel code  0x08
    dq 0x00CF92000000FFFF  ; 2: kernel data  0x10
    times 3 dq 0    
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start