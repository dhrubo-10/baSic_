; x86 boot
; Loads and runs another boot sector, with optional manual input via ALT key.

BITS 16
ORG 0x7C00

LOADOFF    equ 0x7C00
BUFFER     equ 0x0600
PART_TABLE equ 446
PENTRYSIZE equ 16
MAGIC      equ 510

MINIX_PART equ 0x81
sysind     equ 4
lowsec     equ 8

jmp start

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    cli
    mov ss, ax
    mov sp, LOADOFF
    sti

    ; move code to safe buffer
    mov si, sp
    mov di, BUFFER
    mov cx, 512/2
    cld
    rep movsw
    jmp BUFFER+migrate

migrate:
    mov bp, BUFFER+guide

altkey:
    mov ah, 0x02
    int 0x16
    test al, 0x08
    jz noalt
again:
    mov bp, zero
noalt:

    call print
    db 'd?', 8, 0

disk:
    mov dl, 0x80-0x30
    call getch
    add dl, al
    jns nonboot
    mov si, BUFFER+zero-lowsec
    cmp byte [bp], '#'
    jne notlogical
    lea si, [bp+1-lowsec]
notlogical:
    call load
    cmp byte [bp], '#'
    je boot

    call print
    db 'p?', 8, 0

part:
    call getch
    call gettable
    call sort
    call choose_load

    cmp byte [si+sysind], MINIX_PART
    jne waitboot

    call print
    db 's?', 8, 0

slice:
    call getch
    call gettable
    call choose_load

waitboot:
    call print
    db ' ?', 8, 0
    call getch
nonboot:
    jmp notboot

getch:
    mov al, [bp]
    test al, al
    jz getkey
    inc bp
    jmp gotkey
getkey:
    xor ah, ah
    int 0x16
gotkey:
    test dl, dl
    jns putch
    cmp al, 0x0D
    je boot

putch:
    mov ah, 0x0E
    mov bx, 0x0001
    int 0x10
    ret

print:
    mov cx, si
    pop si
.next:
    lodsb
    test al, al
    jz .done
    call putch
    jmp .next
.done:
    xchg si, cx
    jmp cx

boot:
    call print
    db 8, '  ', 13, 10, 0
    jmp LOADOFF-BUFFER

choose_load:
    sub al, '0'
    cmp al, 4
    ja nonboot
    mov ah, PENTRYSIZE
    mul ah
    add ax, BUFFER+PART_TABLE
    mov si, ax
    mov al, [si+sysind]
    test al, al
    jz nonboot
    jmp load

load:
    push dx
    push es
    mov ah, 0x08
    int 0x13
    pop es
    and cl, 0x3F
    inc dh
    mov al, cl
    mul dh
    mov bx, ax
    mov ax, [si+lowsec]
    mov dx, [si+lowsec+2]
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
    jnc rdok
    call print
    db 13, 10, 'Read error', 13, 10, 0
    jmp again
rdok:
    cmp word [LOADOFF+MAGIC], 0xAA55
    je sigok
notboot:
    call print
    db 13, 10, 'Not bootable', 13, 10, 0
    jmp again
sigok:
    ret

gettable:
    mov si, LOADOFF+PART_TABLE
    mov di, BUFFER+PART_TABLE
    mov cx, 4*PENTRYSIZE/2
    rep movsw
    ret

sort:
    mov cx, 4
.sortloop:
    mov si, BUFFER+PART_TABLE
.inner:
    lea di, [si+PENTRYSIZE]
    cmp byte [si+sysind], 0
    jz .swap
    mov bx, [di+lowsec]
    sub bx, [si+lowsec]
    mov bx, [di+lowsec+2]
    sbb bx, [si+lowsec+2]
    jae .order
.swap:
    mov bl, [si]
    xchg bl, [si+PENTRYSIZE]
    mov [si], bl
    inc si
    cmp si, di
    jb .swap
.order:
    mov si, di
    cmp si, BUFFER+PART_TABLE+3*PENTRYSIZE
    jb .inner
    loop .sortloop
    ret


ext_rw:
    db 0x10,0,1,0
    dw LOADOFF
    dw 0
    dd 0
zero: dd 0
guide: times 6 db 0

times 510-($-$$) db 0
dw 0xAA55
