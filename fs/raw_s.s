
BITS 16
ORG 0x7C00          

LOADOFF     EQU 0x7C00
BUFFER      EQU 0x0600
PART_TABLE  EQU 446
PENTRYSIZE  EQU 16
MAGIC       EQU 510

jmp start

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    cli
    mov ss, ax
    mov sp, LOADOFF
    sti

    ; Move to safe buffer
    mov si, sp
    mov di, BUFFER
    mov cx, 512/2
    cld
    rep movsw
    jmp BUFFER:migrate

migrate:
    ; Ask for disk
    mov dl, 0x80         ; Default first hard disk
    call load            ; Load its first sector
    jc fail
    jmp LOADOFF         
load:
    mov ah, 0x02        
    mov al, 1            ; 1 sector
    mov ch, 0            ; cylinder
    mov cl, 1            ; sector (1-based)
    mov dh, 0            ; head
    mov bx, LOADOFF
    int 0x13
    jc error
    cmp word [LOADOFF+MAGIC], 0xAA55
    jne badsig
    clc
    ret

error:
    stc
    ret

badsig:
    mov si, msg_bad
    call print
    stc
    ret

fail:
    mov si, msg_fail
    call print
    jmp $

; Print string at DS:SI
print:
.next:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .next
.done:
    ret

msg_bad db "Not bootable",0
msg_fail db "Disk read error",0

times 510-($-$$) db 0
dw 0xAA55

