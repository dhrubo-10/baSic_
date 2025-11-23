BITS 16
org 0x7C00

LOADOFF     equ 0x7C00
BUFFER      equ 0x0600
PART_TABLE  equ 446
PENTRYSIZE  equ 16
MAGIC       equ 510

bootind     equ 0
sysind      equ 4
lowsec      equ 8

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    cli
    mov ss, ax
    mov sp, LOADOFF
    sti

    mov si, sp
    push si
    mov di, BUFFER
    mov cx, 512/2
    cld
    rep movsw
    jmp far [cs:BUFFER+migrate]

migrate:
findactive:
    test dl, dl
    jns nextdisk
    mov si, BUFFER+PART_TABLE
find:
    cmp byte [si+sysind], 0
    jz nextpart
    test byte [si+bootind], 0x80
    jz nextpart
loadpart:
    call load
    jc error1
bootstrap:
    ret
nextpart:
    add si, PENTRYSIZE
    cmp si, BUFFER+PART_TABLE+4*PENTRYSIZE
    jb find
    call print
    db "No active partition",0
    jmp reboot

nextdisk:
    inc dl
    test dl, dl
    js nexthd
    int 0x11
    shl ax, 1
    shl ax, 1
    and ah, 0x03
    cmp dl, ah
    ja nextdisk
    call load0
    jc nextdisk
    ret
nexthd:
    call load0
error1:
    jc error
    ret

load0:
    mov si, BUFFER+zero-lowsec
    jmp load

load:
    mov di, 3
retry:
    push dx
    push es
    push di
    mov ah, 0x08
    int 0x13
    pop di
    pop es
    and cl, 0x3F
    inc dh
    mov al, cl
    mul dh
    mov bx, ax
    mov ax, [si+lowsec]
    mov dx, [si+lowsec+2]
    cmp dx, (1024*255*63-255) >> 16
    jae bigdisk
    div bx
    xchg ax, dx
    mov ch, dl
    div cl
    xor dl, dl
    shr dx, 1
    shr dx, 1
    or dl, ah
    mov cl, dl
    inc cl
    pop dx
    mov dh, al
    mov bx, LOADOFF
    mov ax, 0x0201
    int 0x13
    jmp rdeval

bigdisk:
    mov bx, dx
    pop dx
    push si
    mov si, BUFFER+ext_rw
    mov [si+8], ax
    mov [si+10], bx
    mov ah, 0x42
    int 0x13
    pop si
rdeval:
    jnc rdok
    cmp ah, 0x80
    je rdbad
    dec di
    jl rdbad
    xor ah, ah
    int 0x13
    jnc retry
rdbad:
    stc
    ret
rdok:
    cmp word [LOADOFF+MAGIC], 0xAA55
    jne nosig
    ret
nosig:
    call print
    db "Not bootable",0
    jmp reboot

error:
    mov si, LOADOFF+errno+1
prnum:
    mov al, ah
    and al, 0x0F
    cmp al, 10
    jb digit
    add al, 7
digit:
    add [si], al
    dec si
    mov cl, 4
    shr ah, cl
    jnz prnum
    call print
    db "Read error ",0
errno:
    db "00",0
    jmp reboot

reboot:
    call print
    db ".  Hit any key to reboot.",0
    xor ah, ah
    int 0x16
    call print
    db 13,10,0
    int 0x19

print:
    pop si
prnext:
    lodsb
    test al, al
    jz prdone
    mov ah, 0x0E
    mov bx, 0x0001
    int 0x10
    jmp prnext
prdone:
    jmp si

ext_rw:
    db 0x10,0
    dw 1
    dw LOADOFF
    dw 0
    dd 0
zero:
    dd 0

times 510-($-$$) db 0
dw 0xAA55
