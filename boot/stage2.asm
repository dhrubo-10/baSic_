; basic_ - Stage 2
; Loaded at 0x8000. Sets up GDT, switches to 32-bit protected mode,
; then calls kmain() in the C kernel.


[BITS 16]
EXTERN kmain            ; defined in kernel/main.c

section .text.entry

stage2_start:
    cli

    ; clear segment registers before entering protected mode
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; load GDT
    lgdt [gdt_descriptor]

    ; set PE bit in CR0 to enable protected mode
    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax

    ; far jump to flush the prefetch queue and load CS with code selector (0x08)
    jmp 0x08:protected_mode_entry
[BITS 32]

protected_mode_entry:
    ; load data segment selector (GDT entry 2 = 0x10) into all data regs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; set up a clean stack well above our kernel image
    mov esp, 0x9F000

    ; call into the C kernel
    mov esp, 0x9F000

    ; Write 'S' in green at top-left (0xB8000). If this appears, PM works.
    mov word [0xB8000], 0x0A53   ; 0x0A = light green attr, 0x53 = 'S'

     call kmain

    ; if kmain ever returns (it shouldn't), halt forever
.hang:
    cli
    hlt
    jmp .hang



; Global Descriptor Table
; Three entries: null, ring-0 code, ring-0 data.
; All segments are flat (base 0, limit 4 GB).
gdt_start:

; Entry 0 — Null descriptor (required)
gdt_null:
    dq 0x0000000000000000

; Entry 1 — Code segment  selector = 0x08
; Base=0, Limit=0xFFFFF, G=1(4KB), D/B=1(32-bit), P=1, DPL=0, S=1, Type=0xA (exec/read)
gdt_code:
    dw 0xFFFF           ; limit [15:0]
    dw 0x0000           ; base  [15:0]
    db 0x00             ; base  [23:16]
    db 10011010b        ; P=1 DPL=00 S=1 Type=1010 (code, exec/read)
    db 11001111b        ; G=1 D/B=1 0 0 limit[19:16]=1111
    db 0x00             ; base  [31:24]

; Entry 2 — Data segment  selector = 0x10
; Same but Type=0x2 (data, read/write)
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b        ; P=1 DPL=00 S=1 Type=0010 (data, read/write)
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; limit (size - 1)
    dd gdt_start                ; base address