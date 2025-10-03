[BITS 16]
[ORG 0x100]        

section .text

global _creat, _open, _close, _exit, _read, _write, _lseek


_PSP:   times 256 db 0

mkfile:
    cld                 
    xor ax, ax
    ; In Minix/ACK this zeroed bss. For .COM we skip heap mgmt here.

    ; For simplicity, jump to _main with dummy argc/argv
    xor cx, cx          ; argc = 0
    mov ax, sp
    push ax             ; argv
    push cx             ; argc
    call _main

    push ax             ; push return value of main
    call _exit          ; exit(main(whaterver is here))

_creat:
    push bp
    mov bp, sp
    mov dx, [bp+4]      ; path (char *)
    xor cx, cx          ; ignore mode, default attrib
    mov ah, 0x3C        ; creat
    int 0x21
    jc seterrno
    pop bp
    ret

_open:
    push bp
    mov bp, sp
    mov dx, [bp+4]      ; path
    mov al, [bp+6]      ; oflag
    mov ah, 0x3D
    int 0x21
    jc seterrno
    pop bp
    ret

_close:
    push bp
    mov bp, sp
    mov bx, [bp+4]
    mov ah, 0x3E
    int 0x21
    jc seterrno
    pop bp
    ret

_exit:
    push bp
    mov bp, sp
    mov al, [bp+4]      ; exit code
    mov ah, 0x4C
    int 0x21
    hlt                 ; safety
    pop bp
    ret


_read:
    push bp
    mov bp, sp
    mov bx, [bp+4]
    mov dx, [bp+6]
    mov cx, [bp+8]
    mov ah, 0x3F
    int 0x21
    jc seterrno
    pop bp
    ret

_write:
    push bp
    mov bp, sp
    mov bx, [bp+4]
    mov dx, [bp+6]
    mov cx, [bp+8]
    mov ah, 0x40
    int 0x21
    jc seterrno
    pop bp
    ret

_lseek:
    push bp
    mov bp, sp
    mov bx, [bp+4]      ; fd
    mov dx, [bp+6]      ; offset low
    mov cx, [bp+8]      ; offset high
    mov al, [bp+10]     ; whence
    mov ah, 0x42
    int 0x21
    jc seterrno
    pop bp
    ret

seterrno:
    ; In Minix: mov (_errno), ax
    ; For DOS COM we just return -1
    mov ax, -1
    cwd                 ; sign-extend to dx:ax if needed
    ret
