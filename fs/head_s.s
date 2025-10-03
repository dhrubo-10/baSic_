BITS 16
ORG 0x7C00          

LOAD_SEG    equ 0x1000   ; Secondary boot segment
BUFFER      equ 0x0600   ; Temp buffer

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00    ; stack at end of boot sector

    mov [boot_drive], dl   ; BIOS passes boot drive in DL

    ; Print "Booting..."
    mov si, msg_boot
    call print_string


    mov ax, LOAD_SEG
    mov es, ax
    xor bx, bx              ; ES:BX = load addr (0x1000:0x0000)
    mov ah, 0x02            ; BIOS read sectors
    mov al, 1               ; # sectors
    mov ch, 0               ; cylinder 0
    mov cl, 2               ; sector 2 (sector 1 = boot)
    mov dh, 0               ; head 0
    mov dl, [boot_drive]    ; drive number
    int 0x13
    jc disk_error

    ; Jump to loaded code
    jmp LOAD_SEG:0x0000


; Disk error handler
disk_error:
    mov si, msg_diskerr
    call print_string
hang: hlt
    jmp hang


; Print string routine (BIOS teletype mode)
print_string:
    mov ah, 0x0E
.next:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .next
.done:
    ret


boot_drive: db 0
msg_boot:   db "Booting secondary stage...", 0
msg_diskerr:db "Disk error! Press Ctrl+Alt+Del", 0


times 510-($-$$) db 0
dw 0xAA55
