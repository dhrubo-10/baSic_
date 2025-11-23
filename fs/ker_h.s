org 0x7C00                
%define LOADOFF 0x7C00
%define BOOTSEG 0x1000
%define BOOTOFF 0x0030

bits 16

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, LOADOFF
    sti

    ; save the boot device in DL for later use
    ; BIOS loads DL with boot drive (e.g., 0x00 floppy, 0x80 first HDD)
    mov [device], dl

    ; set ES to BOOTSEG where we'll load /boot
    mov ax, BOOTSEG
    mov es, ax
    xor bx, bx            ; start offset = 0x0000 within ES

    ; SI points to 'addresses' table just after code; installer/patcher will
    ; populate entries: dword LBA (little-endian), byte count (sectors)
    lea si, [addresses]

read_loop:
    mov di, si            ; di <- pointer to entry
    mov eax, [di]         ; read dword LBA (low 32 bits)

    mov cl, byte [di + 4] ; cl = sector count
    cmp cl, 0
    je  launch_boot       ; done if count == 0

    mov word [packet + 0], 0x10      ; packet size
    mov byte [packet + 2], 0x00      ; reserved (fill)
    mov word [packet + 3], cx        ; number_of_blocks = count (cx holds count)
    ; buffer offset/segment = ES:BX (we'll use BOOTSEG:BX)
    mov word [packet + 5], bx        ; buffer offset (16-bit)
    mov word [packet + 7], BOOTSEG   ; buffer segment (16-bit)
    ; starting LBA -> copy 32-bit EAX to the low dword of the 8-byte field
    mov word [packet + 9], ax        ; low word
    shr eax, 16
    mov word [packet + 11], ax       ; high word of low dword
    ; set the high dword of the 64-bit LBA to 0
    mov word [packet + 13], 0
    mov word [packet + 15], 0

    lea si, [packet]
    mov ah, 0x42
    mov dl, [device]      
    int 0x13
    jc read_error         


    movzx ax, cl
    mov bx, 0             
    mov cx, ax
    shl cx, 9             
    ; Well track offset in a 32-bit variable load_off (dword), easier:
    mov ax, [load_off + 0]
    mov dx, [load_off + 2]
    ; add cx (16-bit) into 32-bit dword
    add ax, cx
    adc dx, 0
    mov [load_off + 0], ax
    mov [load_off + 2], dx

    mov bx, [load_off + 0]
    mov [buf_off], bx

    ; advance to next table entry 5) bytes
    add si, 5
    jmp read_loop

read_error:
    ; Print a simple read error string using teletype (int 0x10 AH=0x0E)
    lea si, [err_str]
print_err:
    lodsb
    test al, al
    jz hang
    mov ah, 0x0E
    mov bh, 0
    mov bl, 7
    int 0x10
    jmp print_err

hang:
    cli
    hlt
    jmp hang

launch_boot:
   

device:    db 0
load_off:  times 2 db 0, 2 db 0 
buf_off:   dw 0


packet:
    dw 0x0010        ; packet size
    db 0x00          ; reserved
    dw 0x0000        ; number_of_blocks (to be filled)
    dw 0x0000        ; buffer offset (low)
    dw 0x0000        ; buffer segment
    dq 0x0000000000000000 ; starting LBA (8 bytes)

; Error string
err_str:   db "Disk read error!", 0

; The addresses table. Installed/patcher fills this with entries:
; format per entry
addresses:

    dd 0x00000000, 2   
    dd 0x00000000, 0   
times 510 - ($ - $$) db 0
dw 0xAA55
