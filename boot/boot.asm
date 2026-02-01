%if 0 
    Decided to scrap everything and writing it from scratch.
    Bcs of some complicated decisions made before on this project. 
    SO now writing it from scratch seems easier thn fixing those issues :")
%endif

bits 16
org 0x7C00

start:
    cli                     ; Disable interrupts
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Setup stack
    
    ; Enable A20 line
    call enable_a20
    
    ; Load kernel from disk
    mov bx, 0x1000          ; Load to 0x1000:0x0000
    mov es, bx
    xor bx, bx
    mov dh, 0               ; Head 0
    mov ch, 0               ; Cylinder 0
    mov cl, 2               ; Sector 2 (1 is boot sector)
    
    mov ah, 0x02            ; Read sectors
    mov al, 15              ; Number of sectors to read
    int 0x13
    jc disk_error
    
    ; Switch to protected mode
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    jmp CODE_SEG:protected_mode

enable_a20:
    ; Try BIOS method
    mov ax, 0x2401
    int 0x15
    jc .keyboard
    ret
    
.keyboard:
    ; Keyboard controller method
    cli
    
    call .wait_input
    mov al, 0xAD
    out 0x64, al
    
    call .wait_input
    mov al, 0xD0
    out 0x64, al
    
    call .wait_output
    in al, 0x60
    push eax
    
    call .wait_input
    mov al, 0xD1
    out 0x64, al
    
    call .wait_input
    pop eax
    or al, 2
    out 0x60, al
    
    call .wait_input
    mov al, 0xAE
    out 0x64, al
    
    call .wait_input
    sti
    ret

.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret

.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret

disk_error:
    mov si, error_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

; GDT
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

; Code segment
gdt_code:
    dw 0xFFFF       ; Limit (0-15)
    dw 0x0          ; Base (0-15)
    db 0x0          ; Base (16-23)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (16-19)
    db 0x0          ; Base (24-31)

; Data segment
gdt_data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

error_msg db "Disk error!", 0

; Protected mode code (32-bit)
bits 32

protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Clear screen
    mov edi, 0xB8000
    mov ecx, 80*25
    mov ax, 0x0F20
    rep stosw
    
    ; Jump to kernel
    jmp 0x10000

    hlt

times 510-($-$$) db 0
dw 0xAA55